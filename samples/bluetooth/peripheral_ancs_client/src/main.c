/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/ancs_client.h>
#include <bluetooth/services/gattp.h>

#include <zephyr/settings/settings.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

#define KEY_REQ_NOTI_ATTR DK_BTN1_MSK
#define KEY_REQ_APP_ATTR DK_BTN2_MSK
#define KEY_POS_ACTION DK_BTN3_MSK
#define KEY_NEG_ACTION DK_BTN4_MSK

/* Allocated size for attribute data. */
#define ATTR_DATA_SIZE BT_ANCS_ATTR_DATA_MAX

enum {
	DISCOVERY_ANCS_ONGOING,
	DISCOVERY_ANCS_SUCCEEDED,
	SERVICE_CHANGED_INDICATED
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_LIMITED | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_SOLICIT128, BT_UUID_ANCS_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct bt_ancs_client ancs_c;

static struct k_work adv_work;

static struct bt_gattp gattp;

static atomic_t discovery_flags;

/* Local copy to keep track of the newest arriving notifications. */
static struct bt_ancs_evt_notif notification_latest;
/* Local copy of the newest notification attribute. */
static struct bt_ancs_attr notif_attr_latest;
/* Local copy of the newest app attribute. */
static struct bt_ancs_attr notif_attr_app_id_latest;
/* Buffers to store attribute data. */
static uint8_t attr_appid[ATTR_DATA_SIZE];
static uint8_t attr_title[ATTR_DATA_SIZE];
static uint8_t attr_subtitle[ATTR_DATA_SIZE];
static uint8_t attr_message[ATTR_DATA_SIZE];
static uint8_t attr_message_size[ATTR_DATA_SIZE];
static uint8_t attr_date[ATTR_DATA_SIZE];
static uint8_t attr_posaction[ATTR_DATA_SIZE];
static uint8_t attr_negaction[ATTR_DATA_SIZE];
static uint8_t attr_disp_name[ATTR_DATA_SIZE];

/* String literals for the iOS notification categories.
 * Used then printing to UART.
 */
static const char *lit_catid[BT_ANCS_CATEGORY_ID_COUNT] = {
	"Other",
	"Incoming Call",
	"Missed Call",
	"Voice Mail",
	"Social",
	"Schedule",
	"Email",
	"News",
	"Health And Fitness",
	"Business And Finance",
	"Location",
	"Entertainment"
};

/* String literals for the iOS notification event types.
 * Used then printing to UART.
 */
static const char *lit_eventid[BT_ANCS_EVT_ID_COUNT] = { "Added",
							 "Modified",
							 "Removed" };

/* String literals for the iOS notification attribute types.
 * Used when printing to UART.
 */
static const char *lit_attrid[BT_ANCS_NOTIF_ATTR_COUNT] = {
	"App Identifier",
	"Title",
	"Subtitle",
	"Message",
	"Message Size",
	"Date",
	"Positive Action Label",
	"Negative Action Label"
};

/* String literals for the iOS notification attribute types.
 * Used When printing to UART.
 */
static const char *lit_appid[BT_ANCS_APP_ATTR_COUNT] = { "Display Name" };

static void discover_ancs_first(struct bt_conn *conn);
static void discover_ancs_again(struct bt_conn *conn);
static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs_c,
		int err, const struct bt_ancs_evt_notif *notif);
static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs_c,
		const struct bt_ancs_attr_response *response);

static void enable_ancs_notifications(struct bt_ancs_client *ancs_c)
{
	int err;

	err = bt_ancs_subscribe_notification_source(ancs_c,
			bt_ancs_notification_source_handler);
	if (err) {
		printk("Failed to enable Notification Source notification (err %d)\n",
		       err);
	}

	err = bt_ancs_subscribe_data_source(ancs_c,
			bt_ancs_data_source_handler);
	if (err) {
		printk("Failed to enable Data Source notification (err %d)\n",
		       err);
	}
}

static void discover_ancs_completed_cb(struct bt_gatt_dm *dm, void *ctx)
{
	int err;
	struct bt_ancs_client *ancs_c = (struct bt_ancs_client *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	printk("The discovery procedure for ANCS succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_ancs_handles_assign(dm, ancs_c);
	if (err) {
		printk("Could not init ANCS client object, error: %d\n", err);
	} else {
		atomic_set_bit(&discovery_flags, DISCOVERY_ANCS_SUCCEEDED);
		enable_ancs_notifications(ancs_c);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n",
		       err);
	}

	atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	discover_ancs_again(conn);
}

static void discover_ancs_not_found_cb(struct bt_conn *conn, void *ctx)
{
	printk("ANCS could not be found during the discovery\n");

	atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	discover_ancs_again(conn);
}

static void discover_ancs_error_found_cb(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure for ANCS failed, err %d\n", err);

	atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	discover_ancs_again(conn);
}

static const struct bt_gatt_dm_cb discover_ancs_cb = {
	.completed = discover_ancs_completed_cb,
	.service_not_found = discover_ancs_not_found_cb,
	.error_found = discover_ancs_error_found_cb,
};

static void indicate_sc_cb(struct bt_gattp *gattp,
			   const struct bt_gattp_handle_range *handle_range,
			   int err)
{
	if (!err) {
		atomic_set_bit(&discovery_flags, SERVICE_CHANGED_INDICATED);
		discover_ancs_again(gattp->conn);
	}
}

static void enable_gattp_indications(struct bt_gattp *gattp)
{
	int err;

	err = bt_gattp_subscribe_service_changed(gattp, indicate_sc_cb);
	if (err) {
		printk("Cannot subscribe to Service Changed indication (err %d)\n",
		       err);
	}
}

static void discover_gattp_completed_cb(struct bt_gatt_dm *dm, void *ctx)
{
	int err;
	struct bt_gattp *gattp = (struct bt_gattp *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	/* Checks if the service is empty.
	 * Discovery Manager handles empty services.
	 */
	if (bt_gatt_dm_attr_cnt(dm) > 1) {
		printk("The discovery procedure for GATT Service succeeded\n");

		bt_gatt_dm_data_print(dm);

		err = bt_gattp_handles_assign(dm, gattp);
		if (err) {
			printk("Could not init GATT Service client object, error: %d\n", err);
		} else {
			enable_gattp_indications(gattp);
		}
	} else {
		printk("GATT Service could not be found during the discovery\n");
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n",
		       err);
	}

	discover_ancs_first(conn);
}

static void discover_gattp_not_found_cb(struct bt_conn *conn, void *ctx)
{
	printk("GATT Service could not be found during the discovery\n");

	discover_ancs_first(conn);
}

static void discover_gattp_error_found_cb(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure for GATT Service failed, err %d\n", err);

	discover_ancs_first(conn);
}

static const struct bt_gatt_dm_cb discover_gattp_cb = {
	.completed = discover_gattp_completed_cb,
	.service_not_found = discover_gattp_not_found_cb,
	.error_found = discover_gattp_error_found_cb,
};

static void discover_gattp(struct bt_conn *conn)
{
	int err;

	err = bt_gatt_dm_start(conn, BT_UUID_GATT, &discover_gattp_cb, &gattp);
	if (err) {
		printk("Failed to start discovery for GATT Service (err %d)\n", err);
	}
}

static void discover_ancs(struct bt_conn *conn, bool retry)
{
	int err;

	/* 1 Service Discovery at a time */
	if (atomic_test_and_set_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING)) {
		return;
	}

	/* If ANCS is found, do not discover again. */
	if (atomic_test_bit(&discovery_flags, DISCOVERY_ANCS_SUCCEEDED)) {
		atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
		return;
	}

	/* Check that Service Changed indication is received before discovering ANCS again. */
	if (retry) {
		if (!atomic_test_and_clear_bit(&discovery_flags, SERVICE_CHANGED_INDICATED)) {
			atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
			return;
		}
	}

	err = bt_gatt_dm_start(conn, BT_UUID_ANCS, &discover_ancs_cb, &ancs_c);
	if (err) {
		printk("Failed to start discovery for ANCS (err %d)\n", err);
		atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	}
}

static void discover_ancs_first(struct bt_conn *conn)
{
	discover_ancs(conn, false);
}

static void discover_ancs_again(struct bt_conn *conn)
{
	discover_ancs(conn, true);
}

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
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);

		if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
			discovery_flags = ATOMIC_INIT(0);
			discover_gattp(conn);
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

/**@brief Function for printing an iOS notification.
 *
 * @param[in] notif  Pointer to the iOS notification.
 */
static void notif_print(const struct bt_ancs_evt_notif *notif)
{
	printk("\nNotification\n");
	printk("Event:       %s\n", lit_eventid[notif->evt_id]);
	printk("Category ID: %s\n", lit_catid[notif->category_id]);
	printk("Category Cnt:%u\n", (unsigned int)notif->category_count);
	printk("UID:         %u\n", (unsigned int)notif->notif_uid);

	printk("Flags:\n");
	if (notif->evt_flags.silent) {
		printk(" Silent\n");
	}
	if (notif->evt_flags.important) {
		printk(" Important\n");
	}
	if (notif->evt_flags.pre_existing) {
		printk(" Pre-existing\n");
	}
	if (notif->evt_flags.positive_action) {
		printk(" Positive Action\n");
	}
	if (notif->evt_flags.negative_action) {
		printk(" Negative Action\n");
	}
}

/**@brief Function for printing iOS notification attribute data.
 *
 * @param[in] attr Pointer to an iOS notification attribute.
 */
static void notif_attr_print(const struct bt_ancs_attr *attr)
{
	if (attr->attr_len != 0) {
		printk("%s: %s\n", lit_attrid[attr->attr_id],
		       (char *)attr->attr_data);
	} else if (attr->attr_len == 0) {
		printk("%s: (N/A)\n", lit_attrid[attr->attr_id]);
	}
}

/**@brief Function for printing iOS notification attribute data.
 *
 * @param[in] attr Pointer to an iOS App attribute.
 */
static void app_attr_print(const struct bt_ancs_attr *attr)
{
	if (attr->attr_len != 0) {
		printk("%s: %s\n", lit_appid[attr->attr_id],
		       (char *)attr->attr_data);
	} else if (attr->attr_len == 0) {
		printk("%s: (N/A)\n", lit_appid[attr->attr_id]);
	}
}

/**@brief Function for printing out errors that originated from the Notification Provider (iOS).
 *
 * @param[in] err_code_np Error code received from NP.
 */
static void err_code_print(uint8_t err_code_np)
{
	switch (err_code_np) {
	case BT_ATT_ERR_ANCS_NP_UNKNOWN_COMMAND:
		printk("Error: Command ID was not recognized by the Notification Provider.\n");
		break;

	case BT_ATT_ERR_ANCS_NP_INVALID_COMMAND:
		printk("Error: Command failed to be parsed on the Notification Provider.\n");
		break;

	case BT_ATT_ERR_ANCS_NP_INVALID_PARAMETER:
		printk("Error: Parameter does not refer to an existing object on the Notification Provider.\n");
		break;

	case BT_ATT_ERR_ANCS_NP_ACTION_FAILED:
		printk("Error: Perform Notification Action Failed on the Notification Provider.\n");
		break;

	default:
		break;
	}
}

static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs_c,
		int err, const struct bt_ancs_evt_notif *notif)
{
	if (!err) {
		notification_latest = *notif;
		notif_print(&notification_latest);
	}
}

static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs_c,
		const struct bt_ancs_attr_response *response)
{
	switch (response->command_id) {
	case BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES:
		notif_attr_latest = response->attr;
		notif_attr_print(&notif_attr_latest);
		if (response->attr.attr_id ==
		    BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER) {
			notif_attr_app_id_latest = response->attr;
		}
		break;

	case BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES:
		app_attr_print(&response->attr);
		break;

	default:
		/* No implementation needed. */
		break;
	}
}

static void bt_ancs_write_response_handler(struct bt_ancs_client *ancs_c,
					   uint8_t err)
{
	err_code_print(err);
}

static int ancs_c_init(void)
{
	int err;

	err = bt_ancs_client_init(&ancs_c);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER,
		attr_appid, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_app_attr(&ancs_c,
		BT_ANCS_APP_ATTR_ID_DISPLAY_NAME,
		attr_disp_name, sizeof(attr_disp_name));
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_TITLE,
		attr_title, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_MESSAGE,
		attr_message, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_SUBTITLE,
		attr_subtitle, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_MESSAGE_SIZE,
		attr_message_size, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_DATE,
		attr_date, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_POSITIVE_ACTION_LABEL,
		attr_posaction, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_NEGATIVE_ACTION_LABEL,
		attr_negaction, ATTR_DATA_SIZE);

	return err;
}

static int gattp_init(void)
{
	return bt_gattp_init(&gattp);
}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;
	int err;

	if (buttons & KEY_REQ_NOTI_ATTR) {
		err = bt_ancs_request_attrs(&ancs_c, &notification_latest,
					    bt_ancs_write_response_handler);
		if (err) {
			printk("Failed requesting attributes for a notification (err: %d)\n",
			       err);
		}
	}

	if (buttons & KEY_REQ_APP_ATTR) {
		if (notif_attr_app_id_latest.attr_id ==
			    BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER &&
		    notif_attr_app_id_latest.attr_len != 0) {
			printk("Request for %s:\n",
			       notif_attr_app_id_latest.attr_data);
			err = bt_ancs_request_app_attr(
				&ancs_c, notif_attr_app_id_latest.attr_data,
				notif_attr_app_id_latest.attr_len,
				bt_ancs_write_response_handler);
			if (err) {
				printk("Failed requesting attributes for a given app (err: %d)\n",
				       err);
			}
		}
	}

	if (buttons & KEY_POS_ACTION) {
		if (notification_latest.evt_flags.positive_action) {
			printk("Performing Positive Action.\n");
			err = bt_ancs_notification_action(
				&ancs_c, notification_latest.notif_uid,
				BT_ANCS_ACTION_ID_POSITIVE,
				bt_ancs_write_response_handler);
			if (err) {
				printk("Failed performing action (err: %d)\n",
				       err);
			}
		}
	}

	if (buttons & KEY_NEG_ACTION) {
		if (notification_latest.evt_flags.negative_action) {
			printk("Performing Negative Action.\n");
			err = bt_ancs_notification_action(
				&ancs_c, notification_latest.notif_uid,
				BT_ANCS_ACTION_ID_NEGATIVE,
				bt_ancs_write_response_handler);
			if (err) {
				printk("Failed performing action (err: %d)\n",
				       err);
			}
		}
	}
}

int main(void)
{
	int blink_status = 0;
	int err;

	printk("Starting Apple Notification Center Service client sample\n");

	err = ancs_c_init();
	if (err) {
		printk("ANCS client init failed (err %d)\n", err);
		return 0;
	}

	err = gattp_init();
	if (err) {
		printk("GATT Service client init failed (err %d)\n", err);
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
