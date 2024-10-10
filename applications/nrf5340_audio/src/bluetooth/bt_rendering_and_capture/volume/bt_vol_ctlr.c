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
#include <zephyr/bluetooth/gatt.h>
/* Int. header used until https://github.com/zephyrproject-rtos/zephyr/issues/71614 is resolved. */
#include <../../../../subsys/bluetooth/audio/vcp_internal.h>

#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_vol_ctlr, CONFIG_BT_VOL_LOG_LEVEL);

struct vcs_peer {
	struct bt_vcp_vol_ctlr *peer;
	uint8_t master_vol;
	bool unknown_vol;
};

static struct vcs_peer vcs_client_peer[CONFIG_BT_MAX_CONN];

/**
 * @brief	Get the index of the first available vcs_client_peer.
 *
 * @retval	Index if success.
 * @retval	-ENOMEM if no available indexes.
 */
static int vcs_client_peer_index_free_get(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (vcs_client_peer[i].peer == NULL) {
			return i;
		}
	}

	LOG_ERR("No more indexes for VCS peer clients");

	return -ENOMEM;
}

/**
 * @brief	Check if a given peer is connected.
 *
 * @param[in]	vcs	The given volume control server
 *
 * @retval	True if connected, false otherwise.
 */
static bool vcs_peer_connected(struct bt_vcp_vol_ctlr const *const vcs)
{
	int ret;
	struct bt_conn *result_conn = NULL;

	ret = bt_vcp_vol_ctlr_conn_get(vcs, &result_conn);
	if (ret) {
		return false;
	}
	return true;
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

	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		ret = bt_vcp_vol_ctlr_conn_get(vcs_client_peer[i].peer, &result_conn);

		if (!ret && conn == result_conn) {
			return true;
		}

		if (ret == -ENOTCONN) {
			/* VCS client no longer connected, free the index */
			vcs_client_peer[i].peer = NULL;
			return false;
		}
	}

	return false;
}

/**
 * @brief Get the index of a given Volume Control Server.
 *
 * @param[in] vcs The given volume control server.
 * @return	Index of the server. -EFAULT if not found.
 */
static int vcs_peer_index_get(struct bt_vcp_vol_ctlr const *const vcs)
{
	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (!vcs_peer_alive(vcs_client_peer[i].peer)) {
			/* Skip */
			continue;
		}

		if (vcs == vcs_client_peer[i].peer) {
			return i;
		}
	}

	return -EFAULT;
}

/**
 * @brief Get the common volume of all devices
 *
 * @return int	Volume on success,
 *		-EFAULT if no common volume was found.
 *		This can happen if no adjustments have been made or no devices are connected.
 */
static int common_vol_get(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (vcs_peer_connected(vcs_client_peer[i].peer) &&
		    vcs_client_peer[i].unknown_vol == false) {
			return vcs_client_peer[i].master_vol;
		}
	}

	return -EFAULT;
}

/**
 * @brief Update the volume of all connected Volume Control Servers.
 *
 * @param vcs[in]		The Volume Control Server who initiated a volume change.
 * @param volume_out[in]	The volume to set.
 * @param include_caller[in]	If true, also send an update to the initiating VCS.
 */
static void update_volume_of_all_peers(struct bt_vcp_vol_ctlr const *const vcs, uint8_t volume_out,
				       bool include_caller)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		LOG_DBG("Saving vol %u for dev %u", volume_out, i);
		vcs_client_peer[i].master_vol = volume_out;
		vcs_client_peer[i].unknown_vol = false;

		if (!vcs_peer_connected(vcs_client_peer[i].peer)) {
			LOG_DBG("Peer %u is not connected", i);
			continue;
		}

		if (vcs == vcs_client_peer[i].peer && !include_caller) {
			LOG_DBG("Caller not included");
			continue;
		}

		ret = bt_vcp_vol_ctlr_set_vol(vcs_client_peer[i].peer, volume_out);

		if (ret) {
			LOG_ERR("Vol sync failed to remote device %u, err = %d", i, ret);
		}
	}
}

static void vol_set_cb_handler(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	int idx;

	if (err) {
		LOG_WRN("VCS vol set callback error: %d", err);
		return;
	}

	idx = vcs_peer_index_get(vol_ctlr);
	if (idx < 0) {
		LOG_ERR("VCS peer not found");
		return;
	}

	LOG_DBG("Saving vol %u for dev %u", vol_ctlr->cp_val.volume, idx);
	vcs_client_peer[idx].master_vol = vol_ctlr->cp_val.volume;
	vcs_client_peer[idx].unknown_vol = false;
}

static void vcs_state_cb_handler(struct bt_vcp_vol_ctlr *vcs, int err, uint8_t dev_volume,
				 uint8_t mute)
{
	int ret;
	int current_idx = vcs_peer_index_get(vcs);

	if (err) {
		LOG_ERR("VCS state callback error: %d", err);
		return;
	}

	if (current_idx < 0) {
		LOG_ERR("VCS peer not found");
		return;
	}

	LOG_INF("VCS state ctrl cb idx: %u vol: %u mute: %u", current_idx, dev_volume, mute);

	if (vcs_client_peer[current_idx].unknown_vol) {
		LOG_DBG("VCS default state from device: %u", current_idx);
		uint8_t common_vol;

		ret = common_vol_get();
		if (ret < 0) {
			LOG_DBG("Saving vol %u for renderer %u", dev_volume, current_idx);
			vcs_client_peer[current_idx].master_vol = dev_volume;
			vcs_client_peer[current_idx].unknown_vol = false;
			return;
		}

		common_vol = ret;

		if (common_vol == dev_volume) {
			/* Volume of this device is the same as the common volume. No update */
			return;
		}

		LOG_DBG("Stored vol found. Writing vol %u to remote device %u", common_vol,
			current_idx);
		update_volume_of_all_peers(vcs, common_vol, true);

	} else {
		/* Write vol change to all other devices. No need to update caller */
		update_volume_of_all_peers(vcs, dev_volume, false);
	}
}

static void vcs_flags_cb_handler(struct bt_vcp_vol_ctlr *vcs, int err, uint8_t flags)
{
	if (err) {
		LOG_ERR("VCS flag callback error: %d", err);
	} else {
		LOG_DBG("Volume flags = 0x%01X", flags);
	}
}

static void vcs_discover_cb_handler(struct bt_vcp_vol_ctlr *vcs, int err, uint8_t vocs_count,
				    uint8_t aics_count)
{
	int ret;

	if (err) {
		LOG_ERR("VCS discover finished callback error: %d", err);
		return;
	}

	/* If a renderer is re-discoverered, reset parameters */
	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (vcs == vcs_client_peer[i].peer) {
			LOG_DBG("Resetting peer %u", i);
			vcs_client_peer[i].master_vol = 0;
			vcs_client_peer[i].unknown_vol = true;
		}
	}

	ret = bt_vcp_vol_ctlr_read_state(vcs);
	if (ret) {
		LOG_ERR("Failed to read VCS state: %d", ret);
	}
}

int bt_vol_ctlr_set(uint8_t volume)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (!vcs_peer_connected(vcs_client_peer[i].peer)) {
			continue;
		}

		ret = bt_vcp_vol_ctlr_set_vol(vcs_client_peer[i].peer, volume);
		if (ret) {
			LOG_ERR("Failed to set vol for remote dev %u, ret = %d", i, ret);
		}
	}

	return 0;
}

int bt_vol_ctlr_up(void)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (!vcs_peer_connected(vcs_client_peer[i].peer)) {
			continue;
		}

		int vol = vcs_client_peer[i].master_vol + CONFIG_BT_AUDIO_VOL_STEP_SIZE;

		vol = CLAMP(vol, 0, UINT8_MAX);
		LOG_DBG("bt_vol_ctlr_up for index %u vol: %u", i, vol);
		ret = bt_vcp_vol_ctlr_set_vol(vcs_client_peer[i].peer, vol);
		if (ret) {
			LOG_ERR("Failed vol up for remote dev %u, ret = %d", i, ret);
		}
	}

	return 0;
}

int bt_vol_ctlr_down(void)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {

		if (!vcs_peer_connected(vcs_client_peer[i].peer)) {
			continue;
		}

		int vol = vcs_client_peer[i].master_vol - CONFIG_BT_AUDIO_VOL_STEP_SIZE;

		vol = CLAMP(vol, 0, UINT8_MAX);
		LOG_DBG("bt_vol_ctlr_down for index %u vol: %u", i, vol);
		ret = bt_vcp_vol_ctlr_set_vol(vcs_client_peer[i].peer, vol);
		if (ret) {
			LOG_ERR("Failed vol down for remote channel %u, ret = %d", i, ret);
		}
	}

	return 0;
}

int bt_vol_ctlr_mute(void)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (!vcs_peer_connected(vcs_client_peer[i].peer)) {
			continue;
		}

		ret = bt_vcp_vol_ctlr_mute(vcs_client_peer[i].peer);
		if (ret) {
			LOG_ERR("Failed to mute for remote dev %u, ret = %d", i, ret);
		}
	}

	return 0;
}

int bt_vol_ctlr_unmute(void)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		if (!vcs_peer_connected(vcs_client_peer[i].peer)) {
			continue;
		}

		ret = bt_vcp_vol_ctlr_unmute(vcs_client_peer[i].peer);
		if (ret) {
			LOG_ERR("Failed to unmute for remote dev %u, ret = %d", i, ret);
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

	ret = bt_vcp_vol_ctlr_discover(conn, &vcs_client_peer[index].peer);
	return ret;
}

int bt_vol_ctlr_init(void)
{
	static struct bt_vcp_vol_ctlr_cb vcs_client_callback;

	vcs_client_callback.discover = vcs_discover_cb_handler;
	vcs_client_callback.state = vcs_state_cb_handler;
	vcs_client_callback.flags = vcs_flags_cb_handler;
	vcs_client_callback.vol_set = vol_set_cb_handler;

	for (size_t i = 0; i < ARRAY_SIZE(vcs_client_peer); i++) {
		vcs_client_peer[i].peer = NULL;
		vcs_client_peer[i].master_vol = 0;
		vcs_client_peer[i].unknown_vol = true;
	}

	return bt_vcp_vol_ctlr_cb_register(&vcs_client_callback);
}
