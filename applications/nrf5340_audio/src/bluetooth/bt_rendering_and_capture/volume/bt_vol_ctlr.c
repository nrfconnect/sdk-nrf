/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_vol_ctlr_internal.h"

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/vcp.h>

#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_vol_ctlr, CONFIG_BT_VOL_LOG_LEVEL);

static struct bt_vcp_vol_ctlr *vcs_client_peer[CONFIG_BT_MAX_CONN];

/**
 * @brief	Get the index of the first available vcs_client_peer.
 *
 * @retval	Index if success.
 * @retval	-ENOMEM if no available indexes.
 */
static int vcs_client_peer_index_free_get(void)
{
	for (int i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (vcs_client_peer[i] == NULL) {
			return i;
		}
	}

	LOG_WRN("No more indexes for VCS peer clients");

	return -ENOMEM;
}

/**
 * @brief	Check if the given @p conn has a vcs_client_peer pointer.
 *
 * @param[in]	conn	The connection pointer to be checked.
 *
 * @retval	True if vcs_client_peer exists.
 * @retval	False otherwise.
 */
static bool vcs_client_peer_exists(struct bt_conn *conn)
{
	int ret;

	struct bt_conn *result_conn = NULL;

	for (int i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		ret = bt_vcp_vol_ctlr_conn_get(vcs_client_peer[i], &result_conn);

		if (!ret && conn == result_conn) {
			return true;
		}

		if (ret == -ENOTCONN) {
			/* VCS client no longer connected, free the index */
			vcs_client_peer[i] = NULL;
			return false;
		}
	}

	return false;
}

/**
 * @brief	Callback handler for the volume state.
 *
 * @note	This callback handler will be triggered if volume state has changed,
 *		or the playback was muted or unmuted.
 */
static void vcs_state_ctlr_cb_handler(struct bt_vcp_vol_ctlr *vcs, int err, uint8_t volume,
				      uint8_t mute)
{
	int ret;

	if (err) {
		LOG_ERR("VCS state callback error: %d", err);
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (vcs == vcs_client_peer[i]) {
			LOG_DBG("VCS state from remote device %d:", i);
			continue;
		}

		LOG_DBG("Sync with other devices %d", i);

		if (vcs_client_peer[i] == NULL) {
			/* Skip */
			continue;
		}

		ret = bt_vcp_vol_ctlr_set_vol(vcs_client_peer[i], volume);
		if (ret) {
			LOG_DBG("Failed to sync volume to remote device %d, err = "
				"%d",
				i, ret);
		}
	}
}

/**
 * @brief	Callback handler for the VCS controller flags.
 *
 * @note	This callback handler will be triggered if VCS flags changed.
 */
static void vcs_flags_ctlr_cb_handler(struct bt_vcp_vol_ctlr *vcs, int err, uint8_t flags)
{
	if (err) {
		LOG_ERR("VCS flag callback error: %d", err);
	} else {
		LOG_DBG("Volume flags = 0x%01X", flags);
	}
}

/**
 * @brief	Callback handler for the finished VCS discovery.
 *
 * @note	This callback handler will be triggered when the VCS discovery has finished.
 */
static void vcs_discover_cb_handler(struct bt_vcp_vol_ctlr *vcs, int err, uint8_t vocs_count,
				    uint8_t aics_count)
{
	if (err) {
		LOG_WRN("VCS discover finished callback error: %d", err);
	} else {
		LOG_INF("VCS discover finished");
	}
}

int bt_vol_ctlr_set(uint8_t volume)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (vcs_client_peer[i] != NULL) {
			ret = bt_vcp_vol_ctlr_set_vol(vcs_client_peer[i], volume);
			if (ret) {
				LOG_WRN("Failed to set volume for remote channel %d, ret = "
					"%d",
					i, ret);
			}
		}
	}

	return 0;
}

int bt_vol_ctlr_up(void)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (vcs_client_peer[i] != NULL) {
			ret = bt_vcp_vol_ctlr_unmute_vol_up(vcs_client_peer[i]);
			if (ret) {
				LOG_WRN("Failed to volume up for remote channel %d, ret = "
					"%d",
					i, ret);
			}
		}
	}

	return 0;
}

int bt_vol_ctlr_down(void)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (vcs_client_peer[i] != NULL) {
			ret = bt_vcp_vol_ctlr_unmute_vol_down(vcs_client_peer[i]);
			if (ret) {
				LOG_WRN("Failed to volume down for remote channel %d, ret "
					"= %d",
					i, ret);
			}
		}
	}

	return 0;
}

int bt_vol_ctlr_mute(void)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (vcs_client_peer[i] != NULL) {
			ret = bt_vcp_vol_ctlr_mute(vcs_client_peer[i]);
			if (ret) {
				LOG_WRN("Failed to mute for remote channel %d, ret "
					"= %d",
					i, ret);
			}
		}
	}

	return 0;
}

int bt_vol_ctlr_unmute(void)
{

	int ret;

	for (int i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (vcs_client_peer[i] != NULL) {
			ret = bt_vcp_vol_ctlr_unmute(vcs_client_peer[i]);
			if (ret) {
				LOG_WRN("Failed to unmute for remote channel %d, "
					"ret = %d",
					i, ret);
			}
		}
	}

	return 0;
}

int bt_vol_ctlr_discover(struct bt_conn *conn)
{

	int ret, index;

	if (vcs_client_peer_exists(conn)) {
		return -EAGAIN;
	}

	index = vcs_client_peer_index_free_get();
	if (index < 0) {
		return index;
	}

	ret = bt_vcp_vol_ctlr_discover(conn, &vcs_client_peer[index]);
	return ret;
}

int bt_vol_ctlr_init(void)
{
	static struct bt_vcp_vol_ctlr_cb vcs_client_callback;

	vcs_client_callback.discover = vcs_discover_cb_handler;
	vcs_client_callback.state = vcs_state_ctlr_cb_handler;
	vcs_client_callback.flags = vcs_flags_ctlr_cb_handler;

	return bt_vcp_vol_ctlr_cb_register(&vcs_client_callback);
}
