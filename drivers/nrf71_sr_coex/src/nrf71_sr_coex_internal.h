/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal interface between the Coexistence Driver (CD) core and its
 *        Coexistence Manager (CM).
 *
 * nrf71_sr_coex_cm.c builds CD2CM command messages and posts them through
 * coex_cd_cm_send_and_wait(), which blocks until the matching CM2CD event
 * arrives before returning.
 */

#ifndef NRF71_SR_COEX_INTERNAL_H__
#define NRF71_SR_COEX_INTERNAL_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <nrf71_coex_if.h>

/**
 * Post a CD2CM command over the Wi-Fi FMAC transport.
 *
 * Does not wait for a CM2CD completion event. Used internally by
 * coex_cd_cm_send_and_wait().
 */
int coex_cm_send(const void *cmd, size_t len);

/**
 * Post a CD2CM command and block until the expected CM2CD event arrives.
 *
 * The expected_event argument must be the CM2CD completion event that matches
 * the posted CD2CM command (for example CM2CD_SET_PRIORITY_RANGES_EVENT after
 * CD2CM_SET_PRIORITY_RANGES).
 */
int coex_cd_cm_send_and_wait(const void *cmd, size_t len, enum cm_event_to_host_t expected_event);

/** Post CD2CM_ENABLE_COEXISTENCE and wait for CM2CD_ENABLE_COEXISTENCE_EVENT. */
int coex_cm_enable(bool enable);

/** Post CD2CM_SET_PRIORITY_RANGES and wait for CM2CD_SET_PRIORITY_RANGES_EVENT. */
int coex_cm_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range);

/** Post CD2CM_UPDATE_COEX_USER_PARAMS and wait for CM2CD_UPDATE_COEX_USER_PARAMS_EVENT. */
int coex_cm_update_user_params(const struct coex_user_params_t *user_params);

/** Post CD2CM_UPDATE_COEX_PARAMS and wait for CM2CD_UPDATE_COEX_PARAMS_EVENT. */
int coex_cm_update_coex_params(void);

/** Post CD2CM_GET_STATS and wait for CM2CD_STATISTICS_EVENT. */
int coex_cm_get_stats(void);

/**
 * Post CD2CM_UPDATE_COEX_PARAMS with a caller-supplied blob and wait for
 * CM2CD_UPDATE_COEX_PARAMS_EVENT.
 */
int coex_cm_update_coex_params_blob(const uint8_t *blob, size_t blob_len);

#endif /* NRF71_SR_COEX_INTERNAL_H__ */
