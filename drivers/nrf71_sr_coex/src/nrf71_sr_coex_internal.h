/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal interface between the coexistence driver core and its
 *        Coexistence Manager (CM) command plumbing.
 *
 * nrf71_sr_coex_cm.c builds the CD2CM command messages (from the firmware
 * interface structures in nrf71_coex_if.h) and posts them over the Wi-Fi FMAC
 * coexistence transport. The driver core (nrf71_sr_coex.c) owns the state
 * machine, the public coex_cd_* API, and CM2CD event handling, and drives the
 * command layer through this interface.
 */

#ifndef NRF71_SR_COEX_INTERNAL_H__
#define NRF71_SR_COEX_INTERNAL_H__

#include <stdbool.h>

#include <common/fw_if/nrf71_coex_if.h>

/** Build and post @c CD2CM_ENABLE_COEXISTENCE. */
int coex_cm_enable(bool enable);

/** Build and post @c CD2CM_SET_PRIORITY_RANGES. */
int coex_cm_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range);

/** Build and post @c CD2CM_UPDATE_COEX_USER_PARAMS. */
int coex_cm_update_user_params(const struct coex_user_params_t *user_params);

/** Build and post @c CD2CM_UPDATE_COEX_PARAMS (NRF_COEX_PARAMS blob). */
int coex_cm_update_coex_params(void);

/** Build and post @c CD2CM_GET_STATS. */
int coex_cm_get_stats(void);

#endif /* NRF71_SR_COEX_INTERNAL_H__ */
