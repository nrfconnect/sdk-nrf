/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence Manager (CM) patch statistics.
 */

#ifndef NRF71_COEX_PATCH_IF_H__
#define NRF71_COEX_PATCH_IF_H__

#include <nrf71_coex_if.h>

#if !defined(NRF71_COEX_PATCH_STATS_IN_COEX_IF)

/** Patch-local command entry and CM2CD event counters. */
struct cm_fsm_patch_stats_t {
	/** CD2CM command entry counts. */
	unsigned int cmd_update_coex_params_cnt_patch;
	unsigned int cmd_update_user_params_cnt_patch;
	unsigned int cmd_enable_coex_cnt_patch;
	unsigned int cmd_allocate_ppw_cnt_patch;
	unsigned int cmd_set_pti_ranges_cnt_patch;
	unsigned int cmd_get_stats_cnt_patch;
	unsigned int cmd_wifi_sw_client_req_cnt_patch;
	unsigned int cm_coex_process_cmd_cnt_patch;
	/** CM2CD event counters. */
	unsigned int wifi_sw_client_event_to_host_cnt;
	unsigned int coex_params_event_to_host_cnt;
	unsigned int user_params_event_to_host_cnt;
	unsigned int enable_coex_event_to_host_cnt;
	unsigned int allocate_ppw_event_to_host_cnt;
	unsigned int set_pti_ranges_event_to_host_cnt;
} __NRF_WIFI_PKD;

#endif /* !NRF71_COEX_PATCH_STATS_IN_COEX_IF */

#endif /* NRF71_COEX_PATCH_IF_H__ */
