/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define MIN_ADV_INT 800
#define MAX_ADV_INT 801

/* Advertising parameter for connectable advertising */
static const struct bt_le_adv_param *adv_param =
		BT_LE_ADV_PARAM((BT_LE_ADV_OPT_CONN |
			BT_LE_ADV_OPT_USE_IDENTITY),
			/* Connectable advertising and use identity address */
			MIN_ADV_INT, /* Min Advertising Interval 500ms (800*0.625ms) */
			MAX_ADV_INT, /* Max Advertising Interval 500.625ms (801*0.625ms) */
			NULL /* Set to NULL for undirected advertising */
		);

LOG_MODULE_REGISTER(peripheral_unit,
	LOG_LEVEL_INF); /* Sets up logging for peripheral module.
			 * Log level set to info
			 */

/* Defines for name */
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static struct k_work adv_work; /* Structure used to submit work */

/* Advertising data */
static const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS,
			(BT_LE_AD_GENERAL |
			BT_LE_AD_NO_BREDR)), /* Discoverable and only support for BLE */
		BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME,
			DEVICE_NAME_LEN), /* Full name of device */
};

/* Function to resume advertising after a disconnection */
static void adv_work_handler(struct k_work *work)
{
	int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err) {
		LOG_ERR("Advertising failed to start (err: %d)", err);
		return;
	}
	LOG_INF("Advertising successfully started");
}

/* Function to submit the work item adv_work to the system work queue */
static void advertising_start(void) { k_work_submit(&adv_work); }

/* Function for notifying when a connection is stopped and the resource becomes
 * available again. Reinitiates advertising
 */
static void recycled_cb(void)
{
	LOG_INF("Connection object available from previous conn. Disconnect is "
		"complete!");
	advertising_start();
}

/* Define the callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
		.recycled = recycled_cb,
};

int main(void)
{
	int err;

	LOG_INF("Starting the peripheral unit");

	/* Change the random static address */
	bt_addr_le_t addr;
	/* Defining address and buffer location */
	err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr);
	if (err < 0) {
		LOG_ERR("Invalid BT address (err: %d)", err);
	}
	/* Setting the address */
	err = bt_id_create(&addr, NULL);
	if (err < 0) {
		LOG_ERR("Creating new ID failed (err: %d)", err);
	}

	/* Initialize bluetooth subsystem */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err: %d)", err);
		return -1;
	}
	LOG_INF("Bluetooth initialized");

	/* Start connectable advertising */
	k_work_init(&adv_work, adv_work_handler);
	advertising_start();

	LOG_INF("Advertising successfully started");
}
