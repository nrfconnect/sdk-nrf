/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Override compiler definition to use size-bounded string copying and concatenation function */
#define _BSD_SOURCE
#include "bt_mgmt_dfu_internal.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "string.h"
#include "macros_common.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mgmt_dfu, CONFIG_BT_MGMT_DFU_LOG_LEVEL);

/* These defined name only used by DFU */
#define DEVICE_NAME_DFU	    CONFIG_BT_DFU_DEVICE_NAME
#define DEVICE_NAME_DFU_LEN (sizeof(DEVICE_NAME_DFU) - 1)

static const struct bt_data ad_peer[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86, 0xd3,
		      0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
};

/* Set aside space for name to be in scan response */
static struct bt_data sd_peer[1];

static void smp_adv(void)
{
	int ret;

	ret = bt_le_adv_start(BT_LE_ADV_CONN, ad_peer, ARRAY_SIZE(ad_peer), sd_peer,
			      ARRAY_SIZE(sd_peer));
	if (ret) {
		LOG_ERR("SMP_SVR Advertising failed to start (ret %d)", ret);
		return;
	}

	/* NOTE: The string below is used by the Nordic CI system */
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
	int ret;
	static char name[CONFIG_BT_DEVICE_NAME_MAX];

	strlcpy(name, CONFIG_BT_DEVICE_NAME, CONFIG_BT_DEVICE_NAME_MAX);
	ret = strlcat(name, "_", CONFIG_BT_DEVICE_NAME_MAX);
	if (ret >= CONFIG_BT_DEVICE_NAME_MAX) {
		LOG_ERR("Failed to set full BT name, will truncate");
	}

#if (CONFIG_AUDIO_DEV == GATEWAY)
	ret = strlcat(name, GW_TAG, CONFIG_BT_DEVICE_NAME_MAX);
	if (ret >= CONFIG_BT_DEVICE_NAME_MAX) {
		LOG_ERR("Failed to set full BT name, will truncate");
	}
#else
	enum audio_channel channel;

	channel_assignment_get(&channel);

	if (channel == AUDIO_CH_L) {
		ret = strlcat(name, CH_L_TAG, CONFIG_BT_DEVICE_NAME_MAX);
		if (ret >= CONFIG_BT_DEVICE_NAME_MAX) {
			LOG_ERR("Failed to set full BT name, will truncate");
		}
	} else {
		ret = strlcat(name, CH_R_TAG, CONFIG_BT_DEVICE_NAME_MAX);
		if (ret >= CONFIG_BT_DEVICE_NAME_MAX) {
			LOG_ERR("Failed to set full BT name, will truncate");
		}
	}

#endif
	ret = strlcat(name, "_DFU", CONFIG_BT_DEVICE_NAME_MAX);
	if (ret >= CONFIG_BT_DEVICE_NAME_MAX) {
		LOG_ERR("Failed to set full BT name, will truncate");
	}

	sd_peer[0].type = BT_DATA_NAME_COMPLETE;
	sd_peer[0].data_len = strlen(name);
	sd_peer[0].data = name;
}

void bt_mgmt_dfu_start(void)
{
	LOG_INF("Entering SMP server mode");

	bt_conn_cb_register(&dfu_conn_callbacks);
	dfu_set_bt_name();
	smp_adv();

	while (1) {
		k_sleep(K_MSEC(100));
	}
}
