/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence Manager (CM) command construction and transport.
 *
 * Builds the CD2CM command messages defined by the firmware interface
 * (nrf71_coex_if.h) and posts them to the Coexistence Manager over the Wi-Fi
 * FMAC coexistence transport. Each command mirrors the reference test bench
 * (coex_manager_tb.c) but is written in Zephyr style; the driver core supplies
 * the payload contents and owns the state machine and event handling.
 *
 * Only the Phase 1 command set is built here (enable, priority ranges, user and
 * internal parameters, statistics). Short-Range software-client requests and
 * Periodic Priority Window commands are Phase 2 and are not issued.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
#include <common/fw_if/nrf71_coex_if.h>

#include "nrf71_sr_coex_internal.h"

LOG_MODULE_DECLARE(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

/* Post a marshalled CD2CM command to the CM over the Wi-Fi FMAC path. */
static int coex_cm_send(const void *cmd, size_t len)
{
	int ret = nrf71_wifi_coex_cmd_send(cmd, len);

	if (ret == -ENODEV) {
		/* CM transport not ready (RPU not up yet). Map to -EACCES to
		 * match the CD public-API "CM not ready" semantics.
		 */
		LOG_DBG("CD2CM command not sent: transport not ready");
		return -EACCES;
	}

	return ret;
}

int coex_cm_enable(bool enable)
{
	struct cd2cm_enable_coexistence_t cmd = {
		.message_id = CD2CM_ENABLE_COEXISTENCE,
		.coex_en_or_dis = enable ? COEX_ENABLE : COEX_DISABLE,
	};

	return coex_cm_send(&cmd, sizeof(cmd));
}

int coex_cm_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range)
{
	struct cd2cm_set_priority_ranges_t cmd;

	if ((wifi_range == NULL) || (sr_range == NULL)) {
		return -EINVAL;
	}

	cmd.message_id = CD2CM_SET_PRIORITY_RANGES;
	cmd.wifi_pti_range = *wifi_range;
	cmd.sr_pti_range = *sr_range;

	return coex_cm_send(&cmd, sizeof(cmd));
}

int coex_cm_update_user_params(const struct coex_user_params_t *user_params)
{
	struct cd2cm_coex_user_params_t cmd;

	if (user_params == NULL) {
		return -EINVAL;
	}

	cmd.message_id = CD2CM_UPDATE_COEX_USER_PARAMS;
	cmd.user_params = *user_params;
	cmd.user_params.message_id = CD2CM_UPDATE_COEX_USER_PARAMS;

	return coex_cm_send(&cmd, sizeof(cmd));
}

int coex_cm_update_coex_params(void)
{
	/* CD2CM_UPDATE_COEX_PARAMS carries an internally-managed parameter blob
	 * (NRF_COEX_PARAMS, defined in nrf71_coex_if.h). The message layout is a
	 * 4-byte message_id followed by the decoded blob, which is the binary
	 * representation of the internal parameter structure that the CM casts
	 * directly. CD treats the value as opaque and forwards it unchanged.
	 */
	uint8_t cmd[sizeof(uint32_t) + (sizeof(NRF_COEX_PARAMS) / 2U)];
	size_t blob_len;

	sys_put_le32(CD2CM_UPDATE_COEX_PARAMS, cmd);

	blob_len = hex2bin(NRF_COEX_PARAMS, strlen(NRF_COEX_PARAMS), &cmd[sizeof(uint32_t)],
			   sizeof(cmd) - sizeof(uint32_t));
	if (blob_len == 0U) {
		LOG_ERR("Malformed NRF_COEX_PARAMS");
		return -EINVAL;
	}

	return coex_cm_send(cmd, sizeof(uint32_t) + blob_len);
}

int coex_cm_get_stats(void)
{
	struct cd2cm_get_coex_stats_t cmd = {
		.message_id = CD2CM_GET_STATS,
	};

	return coex_cm_send(&cmd, sizeof(cmd));
}
