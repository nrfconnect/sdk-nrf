/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_acl_headset.h"

#include <zephyr.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>

#include "macros_common.h"
#include "board.h"
#include "ble_acl_common.h"
#include "channel_assignment.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(ble, CONFIG_LOG_BLE_LEVEL);

#define BT_LE_ADV_FAST_CONN                                                                        \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, BT_GAP_ADV_FAST_INT_MIN_1,                      \
			BT_GAP_ADV_FAST_INT_MAX_1, NULL)

/* Advertising data for peer connection */
static const struct bt_data ad_peer_l[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME_PEER_L, DEVICE_NAME_PEER_L_LEN),
};

static const struct bt_data ad_peer_r[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME_PEER_R, DEVICE_NAME_PEER_R_LEN),
};

/* Connection to the gateway device - the other nRF5340 Audio device
 * This is the device we are streaming audio to/from.
 */
static struct bt_conn *headset_conn_peer;

void work_adv_start(struct k_work *item)
{
	enum audio_channel channel;
	int ret;

	ret = channel_assignment_get(&channel);
	if (ret) {
		/* Channel is not assigned yet: use default */
		channel = AUDIO_CHANNEL_DEFAULT;
	}

	if (channel != AUDIO_CHANNEL_RIGHT) {
		/* If anything else than right, default to left */
		ret = bt_le_adv_start(BT_LE_ADV_FAST_CONN, ad_peer_l, ARRAY_SIZE(ad_peer_l), NULL,
				      0);
	} else {
		ret = bt_le_adv_start(BT_LE_ADV_FAST_CONN, ad_peer_r, ARRAY_SIZE(ad_peer_r), NULL,
				      0);
	}

	if (ret) {
		LOG_ERR("Advertising failed to start (ret %d)", ret);
	}
}

void ble_acl_headset_on_connected(struct bt_conn *conn)
{
	LOG_DBG("Connected - nRF5340 Audio headset");
	headset_conn_peer = bt_conn_ref(conn);
	ble_acl_common_conn_peer_set(headset_conn_peer);
}
