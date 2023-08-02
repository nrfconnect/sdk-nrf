/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic Electronic Shelf Label Service (ESL) application
 */

#include <stdio.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <host/id.h>
#include <zephyr/bluetooth/controller.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

#include "esl.h"
#include "esl_hw_impl.h"
LOG_MODULE_REGISTER(peripheral_esl, CONFIG_PERIPHERAL_ESL_LOG_LEVEL);

BT_ESL_DEF(esl_obj);

extern struct bt_conn *auth_conn;
static struct bt_esl_init_param init_param;

#if defined(AUTH_PASSKEY_MANUAL)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	auth_conn = bt_conn_ref(conn);

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u", addr, passkey);
	LOG_INF("Press Button 1 to confirm, Button 2 to reject.");
}
#endif
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing failed conn: %s, reason %d", addr, reason);
}

static void pairing_confirm(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_auth_pairing_confirm(conn);

	LOG_INF("Pairing confirmed: %s", addr);
}

void bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	char addr[BT_ADDR_STR_LEN];

	bt_addr_le_to_str(peer, addr, sizeof(addr));
	LOG_INF("Bond deleted for %s, id %u", addr, id);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
#if defined(AUTH_PASSKEY_MANUAL)
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
#endif
	.pairing_confirm = pairing_confirm,
	.cancel = auth_cancel,
};
static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {.pairing_complete = pairing_complete,
							       .pairing_failed = pairing_failed,
							       .bond_deleted = bond_deleted};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;

	if (buttons & DK_BTN1_MSK) {
		printk("#DEBUG#FACTORYRESET:1\n");
		bt_esl_factory_reset();
	}
}

static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}
}

int main(void)
{
	int err = 0;
	uint8_t own_addr_type;

	configure_gpio();
	if (IS_ENABLED(CONFIG_BT_ESL_PTS)) {
		uint8_t pub_addr[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

		bt_ctlr_set_public_addr(pub_addr);
		own_addr_type = BT_ADDR_LE_PUBLIC;
	} else {
		own_addr_type = BT_ADDR_LE_RANDOM;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("bt_enable error %d", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	/* Assign hardware characteristics*/
	hw_chrc_init(&init_param);

	/* Assign hardware cb */
	init_param.cb.sensor_init = sensor_init;
	init_param.cb.sensor_control = sensor_control;
	init_param.cb.display_init = display_init;
	init_param.cb.display_control = display_control;
	init_param.cb.led_init = led_init;
	init_param.cb.led_control = led_control;
	init_param.cb.display_unassociated = display_unassociated;
	init_param.cb.display_associated = display_associated;
#if defined(CONFIG_BT_ESL_IMAGE_AVAILABLE)
	init_param.cb.buffer_img = buffer_img;
	init_param.cb.write_img_to_storage = write_img_to_storage;
	init_param.cb.read_img_from_storage = read_img_from_storage;
	init_param.cb.read_img_size_from_storage = read_img_size_from_storage;
	init_param.cb.delete_imgs = delete_imgs_from_storage;

	err = ots_storage_init();
	if (err != 0) {
		LOG_ERR("Failed to init image storage (err:%d)\n", err);
		return err;
	}

#endif /* CONFIG_BT_ESL_IMAGE_AVAILABLE*/
	err = bt_esl_init(&esl_obj, &init_param);
	printk("bt_esl_init (err %d)\n", err);
	if (err != 0) {
		LOG_ERR("Failed to initialize ESL service (err: %d)", err);
		return err;
	}

	bt_id_set_scan_own_addr(true, &own_addr_type);

	if (IS_ENABLED(CONFIG_BT_ESL_SECURITY_ENABLED)) {
		err = bt_conn_auth_cb_register(&conn_auth_callbacks);
		if (err) {
			LOG_ERR("Failed to register authorization callbacks.");
			return err;
		}

		err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
		if (err) {
			LOG_ERR("Failed to register authentication information callbacks.");
			return err;
		}
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}
