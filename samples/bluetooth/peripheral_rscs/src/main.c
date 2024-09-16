/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <bluetooth/services/rscs.h>

#include <zephyr/settings/settings.h>

#include <dk_buttons_and_leds.h>

#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_INTERVAL  1000

static struct bt_rscs_measurement measurement;
static struct bt_conn *current_conn;

static const char *const sensor_location[] = {
	"Other",
	"Top of shoe",
	"In shoe",
	"Hip",
	"Front Wheel",
	"Left Crank",
	"Right Crank",
	"Left Pedal",
	"Right Pedal",
	"Front Hub",
	"Rear Dropout",
	"Chainstay",
	"Rear Wheel",
	"Rear Hub",
	"Chest",
	"Spider",
	"Chain Ring"
};

static void rsc_simulation(struct bt_rscs_measurement *measurement)
{
	static uint8_t i;
	uint32_t rand = sys_rand32_get();

	measurement->inst_speed = 256*1 + rand % (256*11);	/* 1-10 m/s */
	measurement->inst_cadence = 150 + rand % 31;		/* 150-180 1/min */
	measurement->is_inst_stride_len = false;
	measurement->is_total_distance = false;
	measurement->is_running = false;

	if (measurement->inst_speed > 3*256) {
		measurement->is_running = true;
	}

	if (!(i%2)) {
		measurement->is_inst_stride_len = true;
		measurement->inst_stride_length = 50 + rand % 101;	/* 50-150 cm */
	}
	if (!(i%3)) {
		measurement->is_total_distance = true;
		measurement->total_distance += 10 + rand % 50;
	}

	i++;
}

int set_cumulative(uint32_t total_distance)
{
#if IS_ENABLED(CONFIG_SERIAL)
	printk("Set total distance: %d\n", total_distance);
#endif /* CONFIG_SERIAL */
	measurement.total_distance = total_distance;
	return 0;
}

int calibration(void)
{
#if IS_ENABLED(CONFIG_SERIAL)
	printk("Start the calibration procedure\n");
#endif /* CONFIG_SERIAL */

	return 0;
}

void update_loc(uint8_t location)
{
	__ASSERT_NO_MSG(location < ARRAY_SIZE(sensor_location));
#if IS_ENABLED(CONFIG_SERIAL)
	printk("Update sensor location: %u - %s\n", location, sensor_location[location]);
#endif /* CONFIG_SERIAL */
}

void evt_handler(enum bt_rscs_evt evt)
{
	switch (evt) {
	case RSCS_EVT_MEAS_NOTIFY_ENABLE:
#if IS_ENABLED(CONFIG_SERIAL)
		printk("Measurement notify enable\n");
#endif /* CONFIG_SERIAL */
		break;
	case RSCS_EVT_MEAS_NOTIFY_DISABLE:
#if IS_ENABLED(CONFIG_SERIAL)
		printk("Measurement notify disable\n");
#endif /* CONFIG_SERIAL */
	default:
		break;
	}
}

static const struct bt_rscs_cp_cb control_point_cb = {
	.set_cumulative = set_cumulative,
	.calibration = calibration,
	.update_loc = update_loc,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
#if IS_ENABLED(CONFIG_SERIAL)
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
#endif /* CONFIG_SERIAL */
	} else {
#if IS_ENABLED(CONFIG_SERIAL)
		printk("Connected\n");
#endif /* CONFIG_SERIAL */
	}

	current_conn = bt_conn_ref(conn);

	//dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
#if IS_ENABLED(CONFIG_SERIAL)
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
#endif /* CONFIG_SERIAL */

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}

	//dk_set_led_off(CON_STATUS_LED);
}

#ifdef CONFIG_BT_RSCS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
#if IS_ENABLED(CONFIG_SERIAL)
		printk("Security changed: %s level %u\n", addr, level);
#endif /* CONFIG_SERIAL */
	} else {
#if IS_ENABLED(CONFIG_SERIAL)
		printk("Security failed: %s level %u err %d %s\n", addr, level, err,
		       bt_security_err_to_str(err));
#endif /* CONFIG_SERIAL */
	}
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
#ifdef CONFIG_BT_RSCS_SECURITY_ENABLED
	.security_changed = security_changed,
#endif
};

#if defined(CONFIG_BT_RSCS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

#if IS_ENABLED(CONFIG_SERIAL)
	printk("Passkey for %s: %06u\n", addr, passkey);
#endif /* CONFIG_SERIAL */
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

#if IS_ENABLED(CONFIG_SERIAL)
	printk("Pairing cancelled: %s\n", addr);
#endif /* CONFIG_SERIAL */
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

#if IS_ENABLED(CONFIG_SERIAL)
	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
#endif /* CONFIG_SERIAL */
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

#if IS_ENABLED(CONFIG_SERIAL)
	printk("Pairing failed conn: %s, reason %d %s\n", addr, reason,
	       bt_security_err_to_str(reason));
#endif /* CONFIG_SERIAL */
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif


static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_RSCS_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1)
};

#define BT_LE_ADV_CONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
				       BT_GAP_PER_ADV_SLOW_INT_MIN*2, \
				       BT_GAP_PER_ADV_SLOW_INT_MAX*2, NULL)

#pragma GCC push_options
#pragma GCC optimize("-O0")
//bool wait = false;

int main(void)
{
	int err;
	uint32_t blink_status = 0;

#if IS_ENABLED(CONFIG_SERIAL)
	printk("Starting Running Speed and Cadence peripheral example\n");
#endif /* CONFIG_SERIAL */

	//err = dk_leds_init();
// 	if (err) {
// #if IS_ENABLED(CONFIG_SERIAL)
// 		printk("LEDs init failed (err %d)\n", err);
// #endif /* CONFIG_SERIAL */
// 		return 0;
// 	}

// 	if (IS_ENABLED(CONFIG_BT_RSCS_SECURITY_ENABLED)) {
// 		err = bt_conn_auth_cb_register(&conn_auth_callbacks);
// 		if (err) {
// #if IS_ENABLED(CONFIG_SERIAL)
// 			printk("Failed to register authorization callbacks.\n");
// #endif /* CONFIG_SERIAL */
// 			return 0;
// 		}

// 		err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
// 		if (err) {
// #if IS_ENABLED(CONFIG_SERIAL)
// 			printk("Failed to register authorization info callbacks.\n");
// #endif /* CONFIG_SERIAL */
// 			return 0;
// 		}
// 	}

	//while(wait);
	//k_sleep(K_FOREVER);
	//k_msleep(10000);
	printk("back\n");

	err = bt_enable(NULL);
	if (err) {
#if IS_ENABLED(CONFIG_SERIAL)
		printk("Bluetooth init failed (err %d)\n", err);
#endif /* CONFIG_SERIAL */
		return 0;
	}

// #if IS_ENABLED(CONFIG_SERIAL)
// 	printk("Bluetooth initialized\n");
// #endif /* CONFIG_SERIAL */

// 	if (IS_ENABLED(CONFIG_SETTINGS)) {
// 		settings_load();
// 	}

// 	struct bt_rscs_init_params rscs_init = {0};

// 	rscs_init.features.inst_stride_len = 1;
// 	rscs_init.features.multi_sensor_loc = 1;
// 	rscs_init.features.sensor_calib_proc = 1;
// 	rscs_init.features.total_distance = 1;
// 	rscs_init.features.walking_or_running = 1;

// 	rscs_init.supported_locations.other = 1;
// 	rscs_init.supported_locations.top_of_shoe = 1;
// 	rscs_init.supported_locations.in_shoe = 1;
// 	rscs_init.supported_locations.hip = 1;

// 	rscs_init.location = RSC_LOC_HIP;

// 	rscs_init.cp_cb = &control_point_cb;
// 	rscs_init.evt_handler = evt_handler;

// 	err = bt_rscs_init(&rscs_init);
// 	if (err) {
// #if IS_ENABLED(CONFIG_SERIAL)
// 		printk("Failed to init RSCS (err %d)\n", err);
// #endif /* CONFIG_SERIAL */
// 		return 0;
// 	}
	printk("start adv\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("start adv failed\n");

#if IS_ENABLED(CONFIG_SERIAL)
		printk("Advertising failed to start (err %d)\n", err);
#endif /* CONFIG_SERIAL */
		return 0;
	}

#if IS_ENABLED(CONFIG_SERIAL)
	printk("Advertising successfully started\n");
#endif /* CONFIG_SERIAL */

	for (;;) {
		printk("Loop\n");
		k_msleep(2000);

	// 	//dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
	// 	k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));

	// 	rsc_simulation(&measurement);
	// 	bt_rscs_measurement_send(current_conn, &measurement);
	}
}
#pragma GCC pop_options