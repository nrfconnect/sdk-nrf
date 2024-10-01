/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_vol_rend_internal.h"

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/vcp.h>

#include "macros_common.h"
#include "bt_rendering_and_capture.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_vol_rend, CONFIG_BT_VOL_LOG_LEVEL);

/**
 * @brief	Callback handler for the volume state.
 *
 * @note	This callback handler will be triggered if volume state has changed,
 *		or the playback was muted or unmuted from the volume_controller.
 */
static void vcs_state_rend_cb_handler(struct bt_conn *conn, int err, uint8_t volume, uint8_t mute)
{
	int ret;

	if (err) {
		LOG_ERR("VCS state callback error: %d", err);
		return;
	}
	LOG_INF("Volume = %d, mute state = %d", volume, mute);

	/* Send to bt_rend */
	ret = bt_r_and_c_volume_set(volume, true);
	if (ret) {
		LOG_WRN("Failed to set volume");
	}

	if (mute) {
		ret = bt_r_and_c_volume_mute(true);
		if (ret) {
			LOG_WRN("Error muting volume");
		}
	}
}

/**
 * @brief	Callback handler for the changed VCS renderer flags.
 *
 * @note	This callback handler will be triggered if the VCS flags has changed.
 */
static void vcs_flags_rend_cb_handler(struct bt_conn *conn, int err, uint8_t flags)
{
	if (err) {
		LOG_ERR("VCS flag callback error: %d", err);
	} else {
		LOG_DBG("Volume flags = 0x%01X", flags);
	}
}

int bt_vol_rend_set(uint8_t volume)
{

	return bt_vcp_vol_rend_set_vol(volume);
}

int bt_vol_rend_up(void)
{
	return bt_vcp_vol_rend_unmute_vol_up();
}

int bt_vol_rend_down(void)
{

	return bt_vcp_vol_rend_unmute_vol_down();
}

int bt_vol_rend_mute(void)
{
	return bt_vcp_vol_rend_mute();
}

int bt_vol_rend_unmute(void)
{
	return bt_vcp_vol_rend_unmute();
}

int bt_vol_rend_init(void)
{
	int ret;
	struct bt_vcp_vol_rend_register_param vcs_param;
	static struct bt_vcp_vol_rend_cb vcs_server_callback;

	vcs_server_callback.state = vcs_state_rend_cb_handler;
	vcs_server_callback.flags = vcs_flags_rend_cb_handler;
	vcs_param.cb = &vcs_server_callback;
	vcs_param.mute = BT_VCP_STATE_UNMUTED;
	vcs_param.step = CONFIG_BT_AUDIO_VOL_STEP_SIZE;
	vcs_param.volume = CONFIG_BT_AUDIO_VOL_DEFAULT;

	ret = bt_vcp_vol_rend_register(&vcs_param);
	if (ret) {
		return ret;
	}

	return 0;
}
