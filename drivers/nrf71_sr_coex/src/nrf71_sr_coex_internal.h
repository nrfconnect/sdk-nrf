/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal interface between the Coexistence Driver (CD) core and its
 *        Coexistence Manager (CM).
 *
 * nrf71_sr_coex_cm.c builds CD2CM command messages and posts them over the
 * Wi-Fi FMAC coexistence transport.
 */

#ifndef NRF71_SR_COEX_INTERNAL_H__
#define NRF71_SR_COEX_INTERNAL_H__

#include <stdbool.h>

#include <nrf71_coex_if.h>

/** Post CD2CM_ENABLE_COEXISTENCE. */
int coex_cm_enable(bool enable);

/** Post CD2CM_SET_PRIORITY_RANGES. */
int coex_cm_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range);

/** Post CD2CM_UPDATE_COEX_USER_PARAMS. */
int coex_cm_update_user_params(const struct coex_user_params_t *user_params);

/** Post CD2CM_UPDATE_COEX_PARAMS. */
int coex_cm_update_coex_params(void);

/** Post CD2CM_GET_STATS. */
int coex_cm_get_stats(void);

#endif /* NRF71_SR_COEX_INTERNAL_H__ */
