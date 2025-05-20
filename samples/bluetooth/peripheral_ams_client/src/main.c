/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/ams_client.h>

#include <zephyr/settings/settings.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

#define MAX_MSG_SIZE 100

enum {
	IS_UPDATE_TRACK,
	HAS_NEXT_TRACK,
	HAS_PREVIOUS_TRACK
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_LIMITED | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_SOLICIT128, BT_UUID_AMS_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct k_work adv_work;

static struct bt_ams_client ams_c;

static const enum bt_ams_player_attribute_id entity_update_player[] = {
	BT_AMS_PLAYER_ATTRIBUTE_ID_NAME,
	BT_AMS_PLAYER_ATTRIBUTE_ID_PLAYBACK_INFO,
	BT_AMS_PLAYER_ATTRIBUTE_ID_VOLUME
};

static const enum bt_ams_track_attribute_id entity_update_track[] = {
	BT_AMS_TRACK_ATTRIBUTE_ID_ARTIST,
	BT_AMS_TRACK_ATTRIBUTE_ID_ALBUM,
	BT_AMS_TRACK_ATTRIBUTE_ID_TITLE,
	BT_AMS_TRACK_ATTRIBUTE_ID_DURATION
};

static atomic_t appl_flags;

static char msg_buff[MAX_MSG_SIZE + 1];

static void ea_read_cb(struct bt_ams_client *ams_c, uint8_t err, const uint8_t *data, size_t len)
{
	if (err) {
		printk("AMS EA read error 0x%02X\n", err);
	} else if (len > MAX_MSG_SIZE) {
		printk("AMS EA data size is too big\n");
	} else {
		memcpy(msg_buff, data, len);
		msg_buff[len] = '\0';
		printk("AMS EA: %s\n", msg_buff);
	}
}

static void ea_write_cb(struct bt_ams_client *ams_c, uint8_t err)
{
	if (!err) {
		err = bt_ams_read_entity_attribute(ams_c, ea_read_cb);
	} else {
		printk("AMS EA write error 0x%02X\n", err);
	}
}

static void notify_rc_cb(struct bt_ams_client *ams_c,
			 const uint8_t *data, size_t len)
{
	char str_hex[4];
	bool has_next_track = false;
	bool has_previous_track = false;
	enum bt_ams_remote_command_id cmd_id;

	if (len > 0) {
		/* Each data byte is converted to hexadecimal string, which takes 2 bytes.
		 * A comma is added to the hexadecimal except the first data byte.
		 * The first byte converted takes 2 bytes buffer and subsequent byte
		 * converted takes 3 bytes each.
		 */
		if (len * 3 - 1 > MAX_MSG_SIZE) {
			printk("AMS RC data size is too big\n");
		} else {
			/* Print the accepted Remote Command values. */
			sprintf(msg_buff, "%02X", data[0]);

			for (size_t i = 1; i < len; i++) {
				sprintf(str_hex, ",%02X", data[i]);
				strcat(msg_buff, str_hex);
			}

			printk("AMS RC: %s\n", msg_buff);
		}
	}

	/* Check if track commands are available. */
	for (size_t i = 0; i < len; i++) {
		cmd_id = data[i];
		if (cmd_id == BT_AMS_REMOTE_COMMAND_ID_NEXT_TRACK) {
			has_next_track = true;
		} else if (cmd_id == BT_AMS_REMOTE_COMMAND_ID_PREVIOUS_TRACK) {
			has_previous_track = true;
		}
	}
	atomic_set_bit_to(&appl_flags, HAS_NEXT_TRACK, has_next_track);
	atomic_set_bit_to(&appl_flags, HAS_PREVIOUS_TRACK, has_previous_track);
}

static void notify_eu_cb(struct bt_ams_client *ams_c,
			 const struct bt_ams_entity_update_notif *notif,
			 int err)
{
	uint8_t attr_val;
	char str_hex[9];

	if (!err) {
		switch (notif->ent_attr.entity) {
		case BT_AMS_ENTITY_ID_PLAYER:
			attr_val = notif->ent_attr.attribute.player;
			break;
		case BT_AMS_ENTITY_ID_QUEUE:
			attr_val = notif->ent_attr.attribute.queue;
			break;
		case BT_AMS_ENTITY_ID_TRACK:
			attr_val = notif->ent_attr.attribute.track;
			break;
		default:
			err = -EINVAL;
		}
	}

	if (err) {
		printk("AMS EU invalid\n");
	} else if (notif->len > MAX_MSG_SIZE) {
		printk("AMS EU data size is too big\n");
	} else {
		sprintf(str_hex, "%02X,%02X,%02X",
			notif->ent_attr.entity, attr_val, notif->flags);
		memcpy(msg_buff, notif->data, notif->len);
		msg_buff[notif->len] = '\0';
		printk("AMS EU: %s %s\n", str_hex, msg_buff);

		/* Read truncated song title. */
		if (notif->ent_attr.entity == BT_AMS_ENTITY_ID_TRACK &&
		    notif->ent_attr.attribute.track == BT_AMS_TRACK_ATTRIBUTE_ID_TITLE &&
		    (notif->flags & (0x1U << BT_AMS_ENTITY_UPDATE_FLAG_TRUNCATED))) {
			err = bt_ams_write_entity_attribute(ams_c, &notif->ent_attr, ea_write_cb);
			if (err) {
				printk("Cannot write to Entity Attribute (err %d)", err);
			}
		}
	}
}

static void rc_write_cb(struct bt_ams_client *ams_c, uint8_t err)
{
	if (err) {
		printk("AMS RC write error 0x%02X\n", err);
	}
}

static void eu_write_cb(struct bt_ams_client *ams_c, uint8_t err)
{
	if (err) {
		printk("AMS EU write error 0x%02X\n", err);
	}
}

static void enable_notifications(struct bt_ams_client *ams_c)
{
	int err;
	struct bt_ams_entity_attribute_list entity_attribute_list;

	err = bt_ams_subscribe_remote_command(ams_c, notify_rc_cb);
	if (err) {
		printk("Cannot subscribe to Remote Command notification (err %d)\n",
		       err);
	}

	err = bt_ams_subscribe_entity_update(ams_c, notify_eu_cb);
	if (err) {
		printk("Cannot subscribe to Entity Update notification (err %d)\n",
		       err);
	}

	entity_attribute_list.entity = BT_AMS_ENTITY_ID_PLAYER;
	entity_attribute_list.attribute.player = entity_update_player;
	entity_attribute_list.attribute_count = ARRAY_SIZE(entity_update_player);

	err = bt_ams_write_entity_update(ams_c, &entity_attribute_list, eu_write_cb);
	if (err) {
		printk("Cannot write to Entity Update (err %d)\n",
		       err);
	}
}

static void discover_completed_cb(struct bt_gatt_dm *dm, void *ctx)
{
	int err;
	struct bt_ams_client *ams_c = (struct bt_ams_client *)ctx;

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_ams_handles_assign(dm, ams_c);
	if (err) {
		printk("Could not assign AMS client handles, error: %d\n", err);
	} else {
		enable_notifications(ams_c);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n",
		       err);
	}
}

static void discover_service_not_found_cb(struct bt_conn *conn, void *ctx)
{
	printk("The service could not be found during the discovery\n");
}

static void discover_error_found_cb(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure failed, err %d\n", err);
}

static const struct bt_gatt_dm_cb discover_cb = {
	.completed = discover_completed_cb,
	.service_not_found = discover_service_not_found_cb,
	.error_found = discover_error_found_cb,
};

static void adv_work_handler(struct k_work *work)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	int sec_err;
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected %s\n", addr);

	dk_set_led_on(CON_STATUS_LED);

	sec_err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (sec_err) {
		printk("Failed to set security (err %d)\n",
		       sec_err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Disconnected from %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	dk_set_led_off(CON_STATUS_LED);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	int dm_err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);

		if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
			dm_err = bt_gatt_dm_start(conn, BT_UUID_AMS, &discover_cb, &ams_c);
			if (dm_err) {
				printk("Failed to start discovery (err %d)\n", dm_err);
			}
		}
	} else {
		printk("Security failed: %s level %u err %d %s\n", addr, level, err,
		       bt_security_err_to_str(err));
	}
}

static void recycled_cb(void)
{
	printk("Connection object available from previous conn. Disconnect is complete!\n");
	advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
	.recycled = recycled_cb,
};

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);

	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d %s\n", addr, reason,
	       bt_security_err_to_str(reason));

	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

static void toggle_entity_update_track(void)
{
	int err;
	struct bt_ams_entity_attribute_list entity_attribute_list;

	atomic_xor(&appl_flags, (1 << IS_UPDATE_TRACK));

	entity_attribute_list.entity = BT_AMS_ENTITY_ID_TRACK;
	entity_attribute_list.attribute.track = entity_update_track;

	if (atomic_test_bit(&appl_flags, IS_UPDATE_TRACK)) {
		printk("Entity Update to notify Track attributes\n");
		entity_attribute_list.attribute_count = ARRAY_SIZE(entity_update_track);
	} else {
		printk("Entity Update not to notify Track attributes\n");
		entity_attribute_list.attribute_count = 0;
	}

	err = bt_ams_write_entity_update(&ams_c, &entity_attribute_list, eu_write_cb);
	if (err) {
		printk("Cannot write to Entity Update (err %d)\n",
		       err);
	}
}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;
	int err = 0;

	if (buttons & DK_BTN1_MSK) {
		toggle_entity_update_track();
	}

	if (buttons & DK_BTN2_MSK) {
		printk("Play/Pause\n");
		err = bt_ams_write_remote_command(&ams_c,
						  BT_AMS_REMOTE_COMMAND_ID_TOGGLE_PLAY_PAUSE,
						  rc_write_cb);
	}

	if (buttons & DK_BTN3_MSK) {
		if (atomic_test_bit(&appl_flags, HAS_NEXT_TRACK)) {
			printk("Next Track\n");
			err = bt_ams_write_remote_command(&ams_c,
							  BT_AMS_REMOTE_COMMAND_ID_NEXT_TRACK,
							  rc_write_cb);
		} else {
			printk("Volume Up\n");
			err = bt_ams_write_remote_command(&ams_c,
							  BT_AMS_REMOTE_COMMAND_ID_VOLUME_UP,
							  rc_write_cb);
		}
	}

	if (buttons & DK_BTN4_MSK) {
		if (atomic_test_bit(&appl_flags, HAS_NEXT_TRACK)) {
			printk("Previous Track\n");
			err = bt_ams_write_remote_command(&ams_c,
							  BT_AMS_REMOTE_COMMAND_ID_PREVIOUS_TRACK,
							  rc_write_cb);
		} else {
			printk("Volume Down\n");
			err = bt_ams_write_remote_command(&ams_c,
							  BT_AMS_REMOTE_COMMAND_ID_VOLUME_DOWN,
							  rc_write_cb);
		}
	}

	if (err) {
		printk("Cannot write to Remote Command (err %d)\n", err);
	}
}

int main(void)
{
	int blink_status = 0;
	int err;

	printk("Starting Apple Media Service client sample\n");

	err = bt_ams_client_init(&ams_c);
	if (err) {
		printk("AMS client init failed (err %d)\n", err);
		return 0;
	}

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return 0;
	}

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return 0;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("BLE init failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks\n");
		return 0;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks.\n");
		return 0;
	}

	k_work_init(&adv_work, adv_work_handler);
	advertising_start();

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
