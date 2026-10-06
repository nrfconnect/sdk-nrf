/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence Manager (CM) command construction and transport.
 *
 * Each function here builds CD2CM message and submits it through
 * coex_cd_cm_send_and_wait(), which posts the command and blocks until the
 * matching CM2CD completion event is received from the CM.
 *
 * CD2CM command                  CM2CD completion event
 * -------------------------  ---------------------------------
 * CD2CM_ENABLE_COEXISTENCE       CM2CD_ENABLE_COEXISTENCE_EVENT
 * CD2CM_SET_PRIORITY_RANGES      CM2CD_SET_PRIORITY_RANGES_EVENT
 * CD2CM_UPDATE_COEX_USER_PARAMS  CM2CD_UPDATE_COEX_USER_PARAMS_EVENT
 * CD2CM_UPDATE_COEX_PARAMS       CM2CD_UPDATE_COEX_PARAMS_EVENT
 * CD2CM_GET_STATS                CM2CD_STATISTICS_EVENT
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

/* Wi-Fi FMAC coexistence transport (cmd/event/reg access). */
#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
/* CD2CM / CM2CD message structures shared with RPU firmware. */
#include <nrf71_coex_if.h>
/* CD to Short-Range driver interface. */
#include <nrf71_cd_sr_if.h>

#include <nrf71_sr_coex_api.h>
#include "nrf71_sr_coex_internal.h"

LOG_MODULE_DECLARE(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

/**
 * Post a CD2CM command to the CM over the Wi-Fi FMAC path.
 *
 * Transport-only helper: it hands the buffer to the Wi-Fi driver and does not
 * wait for a reply. Callers that need completion use coex_cd_cm_send_and_wait().
 *
 * The transport reports -ENODEV while the RPU is down. That is remapped to
 * -EACCES so callers can tell "coexistence is not available yet" apart from a
 * genuinely missing device.
 */
int coex_cm_send(const void *cmd, size_t len)
{
	int ret = nrf71_wifi_coex_cmd_send(cmd, len);

	if (ret == -ENODEV) {
		/* CM transport not ready (RPU not up yet). */
		LOG_DBG("CD2CM command not sent: transport not ready");
		return -EACCES;
	}

	return ret;
}

/**
 * Post CD2CM_ENABLE_COEXISTENCE and wait for CM2CD_ENABLE_COEXISTENCE_EVENT.
 *
 * Usually called as the last step of the driver's bring-up sequence with
 * enable=true.
 */
int coex_cm_enable(bool enable)
{
	struct cd2cm_enable_coexistence_t cmd = {
		.message_id = CD2CM_ENABLE_COEXISTENCE,
		.coex_en_or_dis = enable ? COEX_ENABLE : COEX_DISABLE,
	};

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_ENABLE_COEXISTENCE_EVENT);
}

/**
 * Post CD2CM_SET_PRIORITY_RANGES and wait for CM2CD_SET_PRIORITY_RANGES_EVENT.
 *
 * Tells the CM which PTI (priority) value ranges Wi-Fi and SR may use.
 */
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

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_SET_PRIORITY_RANGES_EVENT);
}

/**
 * Post CD2CM_UPDATE_COEX_USER_PARAMS and wait for CM2CD_UPDATE_COEX_USER_PARAMS_EVENT.
 *
 * Carries user-tunable settings: protection probabilities (0 to 100 percent)
 * and the shared-antenna allocation mode.
 */
int coex_cm_update_user_params(const struct coex_user_params_t *user_params)
{
	struct cd2cm_coex_user_params_t cmd;

	if (user_params == NULL) {
		return -EINVAL;
	}

	cmd.message_id = CD2CM_UPDATE_COEX_USER_PARAMS;
	cmd.user_params = *user_params;
	cmd.user_params.message_id = CD2CM_UPDATE_COEX_USER_PARAMS;

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_UPDATE_COEX_USER_PARAMS_EVENT);
}

/**
 * Post CD2CM_UPDATE_COEX_PARAMS with the built-in parameter blob and wait for
 * CM2CD_UPDATE_COEX_PARAMS_EVENT.
 *
 * NRF_COEX_PARAMS is a compile-time hex string of internal CM tuning values, so
 * it is decoded to binary before sending. hex2bin() returns 0 on a malformed
 * string. To send modified values, use coex_cm_update_coex_params_blob().
 */
int coex_cm_update_coex_params(void)
{
	uint8_t cmd[sizeof(uint32_t) + (sizeof(NRF_COEX_PARAMS) / 2U)];
	size_t blob_len;

	/* Set the message ID in the command header. */
	sys_put_le32(CD2CM_UPDATE_COEX_PARAMS, cmd);

	/* Decode straight into the command buffer after the message ID. */
	blob_len = hex2bin(NRF_COEX_PARAMS, strlen(NRF_COEX_PARAMS),
			   &cmd[sizeof(uint32_t)],
			   sizeof(cmd) - sizeof(uint32_t));
	if (blob_len == 0U) {
		LOG_ERR("Malformed NRF_COEX_PARAMS");
		return -EINVAL;
	}

	return coex_cd_cm_send_and_wait(cmd, sizeof(uint32_t) + blob_len,
					CM2CD_UPDATE_COEX_PARAMS_EVENT);
}

/**
 * Post CD2CM_UPDATE_COEX_PARAMS with a caller-supplied blob and wait for
 * CM2CD_UPDATE_COEX_PARAMS_EVENT.
 *
 * Same wire format as coex_cm_update_coex_params(), but the caller provides the
 * binary blob. Used by the application or test bench to patch individual bytes
 * (for example the LNA switch control byte). Blobs longer than
 * CD2CM_COEX_PARAMS_MAX_BLOB_LEN are rejected before any CM transaction starts.
 */
int coex_cm_update_coex_params_blob(const uint8_t *blob, size_t blob_len)
{
	uint8_t cmd[sizeof(uint32_t) + CD2CM_COEX_PARAMS_MAX_BLOB_LEN];

	if ((blob == NULL) || (blob_len == 0U) ||
	    (blob_len > CD2CM_COEX_PARAMS_MAX_BLOB_LEN)) {
		return -EINVAL;
	}

	/* Set the message ID in the command header. */
	sys_put_le32(CD2CM_UPDATE_COEX_PARAMS, cmd);

	/* Copy the caller-supplied binary blob after the message ID. */
	memcpy(&cmd[sizeof(uint32_t)], blob, blob_len);

	return coex_cd_cm_send_and_wait(cmd, sizeof(uint32_t) + blob_len,
					CM2CD_UPDATE_COEX_PARAMS_EVENT);
}

/**
 * Post CD2CM_GET_STATS and wait for CM2CD_STATISTICS_EVENT.
 *
 * The command carries only a message id. The statistics payload arrives with
 * the completion event and is retained by the driver in coex_event_handler().
 */
int coex_cm_get_stats(void)
{
	struct cd2cm_get_coex_stats_t cmd = {
		.message_id = CD2CM_GET_STATS,
	};

	return coex_cd_cm_send_and_wait(&cmd, sizeof(cmd), CM2CD_STATISTICS_EVENT);
}
