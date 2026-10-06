/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence Driver (CD) APIs for Coexistence Manager (CM) configuration,
 *        runtime commands, and retained CM statistics.
 *
 * Each coex_cd_* API that talks to the CM posts a CD2CM command and blocks
 * until the matching CM2CD completion event is received before returning to the
 * caller.
 *
 * CD2CM command                  CM2CD completion event
 * -------------------------  ---------------------------------
 * CD2CM_ENABLE_COEXISTENCE       CM2CD_ENABLE_COEXISTENCE_EVENT
 * CD2CM_SET_PRIORITY_RANGES      CM2CD_SET_PRIORITY_RANGES_EVENT
 * CD2CM_UPDATE_COEX_USER_PARAMS  CM2CD_UPDATE_COEX_USER_PARAMS_EVENT
 * CD2CM_UPDATE_COEX_PARAMS       CM2CD_UPDATE_COEX_PARAMS_EVENT
 * CD2CM_GET_STATS                CM2CD_STATISTICS_EVENT
 */

#ifndef NRF71_SR_COEX_API_H__
#define NRF71_SR_COEX_API_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <nrf71_coex_if.h>

/** Largest blob accepted by coex_cd_update_coex_params_blob(), in bytes. */
#define CD2CM_COEX_PARAMS_MAX_BLOB_LEN 128U

/** Post CD2CM_ENABLE_COEXISTENCE and wait for CM2CD_ENABLE_COEXISTENCE_EVENT. */
int coex_cd_enable(bool enable);

/** Post CD2CM_SET_PRIORITY_RANGES and wait for CM2CD_SET_PRIORITY_RANGES_EVENT. */
int coex_cd_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range);

/** Post CD2CM_UPDATE_COEX_USER_PARAMS and wait for CM2CD_UPDATE_COEX_USER_PARAMS_EVENT. */
int coex_cd_update_user_params(const struct coex_user_params_t *user_params);

/**
 * Post CD2CM_UPDATE_COEX_PARAMS (default NRF_COEX_PARAMS blob) and wait for
 * CM2CD_UPDATE_COEX_PARAMS_EVENT.
 */
int coex_cd_update_coex_params(void);

/**
 * Post CD2CM_UPDATE_COEX_PARAMS with a caller-supplied blob and wait for
 * CM2CD_UPDATE_COEX_PARAMS_EVENT.
 */
int coex_cd_update_coex_params_blob(const uint8_t *blob, size_t blob_len);

/**
 * Post CD2CM_GET_STATS, wait for CM2CD_STATISTICS_EVENT, and retain the
 * latest statistics snapshot inside the driver.
 */
int coex_cd_get_stats(void);

/**
 * Copy the cm_stats_t retained from the last CM2CD_STATISTICS_EVENT into
 * stats.
 *
 * The copy is taken under the driver lock, so it is never torn by a statistics
 * event arriving concurrently.
 *
 * @retval 0        Snapshot copied.
 * @retval -EINVAL  stats is NULL.
 * @retval -ENODATA No statistics snapshot is retained.
 */
int coex_cd_get_last_stats(struct cm_stats_t *stats);

/**
 * Copy the patch statistics retained from the last CM2CD_STATISTICS_EVENT into
 * @p patch_stats.
 *
 * @retval 0        Snapshot copied.
 * @retval -EINVAL  patch_stats is NULL.
 * @retval -ENODATA The last statistics event carried no patch trailer, or no
 *                  statistics snapshot is retained.
 */
int coex_cd_get_last_patch_stats(struct cm_fsm_patch_stats_t *patch_stats);

#endif /* NRF71_SR_COEX_API_H__ */
