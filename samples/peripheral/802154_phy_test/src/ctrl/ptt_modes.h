/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: modes declaration intended for usage inside control module */

#ifndef PTT_MODES_H__
#define PTT_MODES_H__

#include "ptt_config.h"
#include "ptt_errors.h"
#include "ptt_utils.h"

/** device modes */
enum ptt_mode_t {
	PTT_MODE_ZB_PERF_DUT = 0, /**< device under test mode */
	PTT_MODE_ZB_PERF_CMD, /**< command mode */
	PTT_MODE_N /**< mode count */
};

/** @brief Switch current mode to a given
 *
 *  Uninitialize current mode, reset shared resources and initialize a new mode.
 *  If a given mode matches with current mode, current mode resets to state as after initialization.
 *
 *  @param new_mode - a new mode to switch
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_mode_switch(enum ptt_mode_t new_mode);

/** @brief Initialize default mode
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_mode_default_init(void);

/** @brief Uninitialize current mode
 *
 *  @param none
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_mode_uninit(void);

/** @brief Initialize Zigbee RF Performance Test Plan CMD mode
 *
 *  @param none
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_zb_perf_cmd_mode_init(void);

/** @brief Uninitialize Zigbee RF Performance Test Plan CMD mode
 *
 *  @param none
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_zb_perf_cmd_mode_uninit(void);

/** @brief Initialize Zigbee RF Performance Test Plan DUT mode
 *
 *  @param none
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_zb_perf_dut_mode_init(void);

/** @brief Uninitialize Zigbee RF Performance Test Plan DUT mode
 *
 *  @param none
 *
 *  @return enum ptt_ret returns PTT_RET_SUCCESS, or error code
 */
enum ptt_ret ptt_zb_perf_dut_mode_uninit(void);

#endif /* PTT_MODES_H__ */
