/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/ots.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_INTERVAL  1000

#define BT_GATT_OTS_UNSPECIFIED_OBJ_TYPE 0x2ACA

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_UUID16_ALL, 0x25, 0x18) /* OTS Service */
};

static u8_t objects[2][100];
static u32_t obj_cnt;

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");

	dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	dk_set_led_off(CON_STATUS_LED);
}

static struct bt_conn_cb conn_callbacks = {
	.connected        = connected,
	.disconnected     = disconnected,
};

static int ots_obj_created(struct bt_gatt_ots *ots, struct bt_conn *conn,
			   u64_t id, const struct bt_gatt_ots_obj_init *init)
{
	u32_t *id_trunc = (u32_t *) &id;

	if (obj_cnt >= ARRAY_SIZE(objects)) {
		printk("No space left for Object with 0x%08X ID\n", *id_trunc);
		return -ENOMEM;
	}

	printk("Object with 0x%08X ID has been created\n", *id_trunc);
	obj_cnt++;

	return 0;
}

static void ots_obj_deleted(struct bt_gatt_ots *ots, struct bt_conn *conn,
			    u64_t id)
{
	u32_t *id_trunc = (u32_t *) &id;

	printk("Object with 0x%08X ID has been deleted\n", *id_trunc);

	obj_cnt--;
}

static void ots_obj_selected(struct bt_gatt_ots *ots, struct bt_conn *conn,
			     u64_t id)
{
	u32_t *id_trunc = (u32_t *) &id;

	printk("Object with 0x%08X ID has been selected\n", *id_trunc);
}

static u32_t ots_obj_read(struct bt_gatt_ots *ots, struct bt_conn *conn,
			  u64_t id, u8_t **data, u32_t len, u32_t offset)
{
	u32_t *id_trunc = (u32_t *) &id;
	u32_t obj_index = (id % 2);

	if (!data) {
		printk("Object with 0x%08X ID has been successfully read\n",
		       *id_trunc);

		return 0;
	}

	*data = &objects[obj_index][offset];

	/* Send the first object data in 20 byte packets. */
	if (obj_index == 0) {
		len = (len < 20) ? len : 20;
	}

	printk("Object with 0x%08X ID is being read\n"
		"Offset = %d, Length = %d\n",
		*id_trunc, offset, len);

	return len;
}

static struct bt_gatt_ots_cb ots_callbacks = {
	.obj_created = ots_obj_created,
	.obj_deleted = ots_obj_deleted,
	.obj_selected = ots_obj_selected,
	.obj_read = ots_obj_read,
};

static int ots_init(void)
{
	int err;
	struct bt_gatt_ots *ots;
	struct bt_gatt_ots_init ots_init;
	struct bt_gatt_ots_obj_init obj_init;

	ots = bt_gatt_ots_instance_get();
	if (!ots) {
		printk("Failed to retrieve OTS instance\n");
		return -ENOMEM;
	}

	/* Configure OTS initialization. */
	memset(&ots_init, 0, sizeof(ots_init));
	BT_GATT_OTS_OACP_SET_FEAT_READ(ots_init.features.oacp);
	BT_GATT_OTS_OLCP_SET_FEAT_GO_TO(ots_init.features.olcp);
	ots_init.cb = &ots_callbacks;

	/* Initialize OTS instance. */
	err = bt_gatt_ots_init(ots, &ots_init);
	if (err) {
		printk("Failed to init OTS (err:%d)\n", err);
		return err;
	}

	/* Prepare first object demo data and add it to the server. */
	for (u32_t i = 0; i < sizeof(objects[0]); i++) {
		objects[0][i] = i + 1;
	}

	memset(&obj_init, 0, sizeof(obj_init));
	obj_init.name = "first_object.txt";
	obj_init.type.uuid.type = BT_UUID_TYPE_16;
	obj_init.type.uuid_16.val = BT_GATT_OTS_UNSPECIFIED_OBJ_TYPE;
	obj_init.size = sizeof(objects[0]);
	BT_GATT_OTS_OBJ_SET_PROP_READ(obj_init.props);

	err = bt_gatt_ots_object_add(ots, &obj_init);
	if (err) {
		printk("Failed to add an object to OTS (err: %d)\n", err);
		return err;
	}

	/* Prepare second object demo data and add it to the server. */
	for (u32_t i = 0; i < sizeof(objects[1]); i++) {
		objects[1][i] = i * 2;
	}

	memset(&obj_init, 0, sizeof(obj_init));
	obj_init.name = "second_object.gif";
	obj_init.type.uuid.type = BT_UUID_TYPE_16;
	obj_init.type.uuid_16.val = BT_GATT_OTS_UNSPECIFIED_OBJ_TYPE;
	obj_init.size = sizeof(objects[1]);
	BT_GATT_OTS_OBJ_SET_PROP_READ(obj_init.props);

	err = bt_gatt_ots_object_add(ots, &obj_init);
	if (err) {
		printk("Failed to add an object to OTS (err: %d)\n", err);
		return err;
	}

	return 0;
}

void main(void)
{
	int blink_status = 0;
	int err;

	printk("Starting Bluetooth Peripheral OTS example\n");

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = ots_init();
	if (err) {
		printk("Failed to init OTS (err:%d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
