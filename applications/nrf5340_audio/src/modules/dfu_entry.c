/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Override compiler definition to use size-bounded string copying and concatenation function */
#define _BSD_SOURCE
#include "string.h"
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>

#include "button_assignments.h"
#include "ble_core.h"
#include "button_handler.h"
#include "macros_common.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dfu, CONFIG_MODULE_DFU_ENTRY_LOG_LEVEL);

/* These defined name only used by DFU */
#define DEVICE_NAME_DFU CONFIG_BT_DFU_DEVICE_NAME
#define DEVICE_NAME_DFU_LEN (sizeof(DEVICE_NAME_DFU) - 1)

/* Advertising data for SMP_SVR UUID */
static struct bt_le_adv_param adv_param;
static const struct bt_data ad_peer[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86, 0xd3,
		      0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
};

static void smp_adv(void)
{
	int ret;

	ret = bt_le_adv_start(&adv_param, ad_peer, ARRAY_SIZE(ad_peer), NULL, 0);
	if (ret) {
		LOG_ERR("SMP_SVR Advertising failed to start (ret %d)", ret);
		return;
	}

	LOG_INF("Regular SMP_SVR advertising started");
}

/* These callbacks are to override callback registed in module le_audio_ */
static void dfu_connected_cb(struct bt_conn *conn, uint8_t err)
{
	LOG_INF("SMP connected\n");
}

static void dfu_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("SMP disconnected 0x%02x\n", reason);
}

static struct bt_conn_cb dfu_conn_callbacks = {
	.connected = dfu_connected_cb,
	.disconnected = dfu_disconnected_cb,
};

static void dfu_set_bt_name(void)
{
	char name[CONFIG_BT_DEVICE_NAME_MAX] = { 0 };

	strlcpy(name, CONFIG_BT_DEVICE_NAME, CONFIG_BT_DEVICE_NAME_MAX);
	strlcat(name, "_", CONFIG_BT_DEVICE_NAME_MAX);
#if (CONFIG_AUDIO_DEV == GATEWAY)
	strlcat(name, GW_TAG, CONFIG_BT_DEVICE_NAME_MAX);
#else
	enum audio_channel channel;

	channel_assignment_get(&channel);

	if (channel == AUDIO_CH_L) {
		strlcat(name, CH_L_TAG, CONFIG_BT_DEVICE_NAME_MAX);
	} else {
		strlcat(name, CH_R_TAG, CONFIG_BT_DEVICE_NAME_MAX);
	}

#endif
	strlcat(name, "_DFU", CONFIG_BT_DEVICE_NAME_MAX);
	bt_set_name(name);
}

static void on_ble_core_ready_dfu_entry(void)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bt_conn_cb_register(&dfu_conn_callbacks);
	adv_param = *BT_LE_ADV_CONN_NAME;
	dfu_set_bt_name();
	smp_adv();
}

void dfu_entry_check(void)
{
	int ret;
	bool pressed;

	ret = button_pressed(BUTTON_4, &pressed);
	if (ret) {
		return;
	}

	if (pressed) {
		LOG_INF("Enter SMP_SVR service only status");
		ret = ble_core_init(on_ble_core_ready_dfu_entry);
		ERR_CHK(ret);

		while (1) {
			k_sleep(K_MSEC(100));
		}
	}
}
