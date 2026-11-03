/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Debug shell for the nRF71 coexistence driver.
 */

#include <stddef.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <nrf71_coex_if.h>

#include "nrf71_sr_coex_internal.h"

struct cd_shell_stat {
	const char *name;
	size_t offset;
};

#define CD_SHELL_STAT(type, field) {#field, offsetof(struct type, field)}

static const struct cd_shell_stat cd_cm_stats[] = {
	CD_SHELL_STAT(cm_stats_t, init_count),
	CD_SHELL_STAT(cm_stats_t, ppw_start_msgs_cnt),
	CD_SHELL_STAT(cm_stats_t, ppw_stop_msgs_cnt),
	CD_SHELL_STAT(cm_stats_t, ppw_stop_timeout_cnt),
	CD_SHELL_STAT(cm_stats_t, num_wifi_pti_windows),
	CD_SHELL_STAT(cm_stats_t, num_sr_pti_windows),
	CD_SHELL_STAT(cm_stats_t, wifi_ccconf_wifi_window),
	CD_SHELL_STAT(cm_stats_t, wifi_ccconf_sr_window),
	CD_SHELL_STAT(cm_stats_t, ppw_invalid_params_cnt),
	CD_SHELL_STAT(cm_stats_t, ppw_invalid_request_cnt),
	CD_SHELL_STAT(cm_stats_t, cfg_band_sel_5g_cnt),
	CD_SHELL_STAT(cm_stats_t, cfg_band_sel_2pt4g_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_req_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_rel_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_gnt_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_rel_timeout_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_req_non2pt4g_band_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_req_2pt4g_band_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_decison_in_favour_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_decison_not_in_favour_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_no_gnt_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_no_gnt_cnt_non_2pt4g),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_rel_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_rel_cnt_non_2pt4g),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_gnt_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, wifi_sw_client_gnt_cnt_non_2pt4g),
	CD_SHELL_STAT(cm_stats_t, wifi_beacon_rx_req_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_conn_req_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_calib_req_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_scan_req_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_beacon_rx_rel_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_conn_rel_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_calib_rel_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_scan_rel_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_beacon_rx_gnt_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_conn_gnt_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_calib_gnt_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_scan_gnt_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_beacon_rx_no_gnt_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_conn_no_gnt_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_calib_no_gnt_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_scan_no_gnt_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_beacon_rx_rel_to_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_conn_rel_to_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_calib_rel_to_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_scan_rel_to_cnt),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_success_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_fail_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_success_cnt_non_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_fail_cnt_non_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_inact2listen_ps_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_listen2inact_ps_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_inact2listen_calib_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_listen2inact_calib_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_inact2listen_ps_cnt_non_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_listen2inact_ps_cnt_non_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_inact2listen_calib_cnt_non_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_listen2inact_calib_cnt_non_2pt4g),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_2pt4g_band_cnt),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_non2pt4g_band_cnt),
	CD_SHELL_STAT(cm_stats_t, sr_rx_prot_force_wifi_cnt_2pt4g),
	CD_SHELL_STAT(cm_stats_t, coex_enable_cnt),
	CD_SHELL_STAT(cm_stats_t, coex_disable_cnt),
	CD_SHELL_STAT(cm_stats_t, antswc_inhibit_cfg_cnt),
	CD_SHELL_STAT(cm_stats_t, antswc_anten_override_cfg_cnt),
	CD_SHELL_STAT(cm_stats_t, antswc_pa_override_cfg_cnt),
	CD_SHELL_STAT(cm_stats_t, antswc_lnasw_override_cfg_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_ccconf_save_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_ccconf_restore_cnt),
	CD_SHELL_STAT(cm_stats_t, cmd_update_coex_params_cnt),
	CD_SHELL_STAT(cm_stats_t, cmd_update_user_params_cnt),
	CD_SHELL_STAT(cm_stats_t, cmd_enable_coex_cnt),
	CD_SHELL_STAT(cm_stats_t, cmd_allocate_ppw_cnt),
	CD_SHELL_STAT(cm_stats_t, cmd_set_pti_ranges_cnt),
	CD_SHELL_STAT(cm_stats_t, cmd_get_stats_cnt),
	CD_SHELL_STAT(cm_stats_t, cmd_sw_client_req_cnt),
	CD_SHELL_STAT(cm_stats_t, stats_event_to_host_cnt),
	CD_SHELL_STAT(cm_stats_t, sw_client_event_to_host_cnt),
	CD_SHELL_STAT(cm_stats_t, cd2cm_null_cmd_buf_cnt),
	CD_SHELL_STAT(cm_stats_t, cd2cm_invalid_msg_id_cnt),
	CD_SHELL_STAT(cm_stats_t, coex_params_buf_error),
	CD_SHELL_STAT(cm_stats_t, user_params_buf_error),
	CD_SHELL_STAT(cm_stats_t, en_coex_buf_error),
	CD_SHELL_STAT(cm_stats_t, ppw_buf_error),
	CD_SHELL_STAT(cm_stats_t, set_pti_ranges_buf_error),
	CD_SHELL_STAT(cm_stats_t, sw_client_req_buf_error),
	CD_SHELL_STAT(cm_stats_t, event_buf_unavailable_cnt),
	CD_SHELL_STAT(cm_stats_t, unknown_event_to_host),
	CD_SHELL_STAT(cm_stats_t, wifi_wrong_sw_client_id_cnt),
	CD_SHELL_STAT(cm_stats_t, wifi_wrong_sw_client_request_type_cnt),
	CD_SHELL_STAT(cm_stats_t, wrong_antswc_ctrl_type_cnt),
	CD_SHELL_STAT(cm_stats_t, wrong_wifi_hw_client_pti_level_cnt),
	CD_SHELL_STAT(cm_stats_t, wrong_input_populate_pti_cnt),
	CD_SHELL_STAT(cm_stats_t, wrong_sr_rx_prot_scenario_cnt),
	CD_SHELL_STAT(cm_stats_t, wrong_sr_rx_prot_perc_prob_cnt),
	CD_SHELL_STAT(cm_stats_t, wrong_wifi_sw_client_id_cnt),
	CD_SHELL_STAT(cm_stats_t, wrong_wifi_sw_client_perc_prob_cnt),
	CD_SHELL_STAT(cm_stats_t, zero_ppw_win_duration_cnt),
	CD_SHELL_STAT(cm_stats_t, zero_ppw_timeout_cnt),
};

static const struct cd_shell_stat cd_cm_patch_stats[] = {
	CD_SHELL_STAT(cm_fsm_patch_stats_t, cmd_update_coex_params_cnt_patch),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, cmd_update_user_params_cnt_patch),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, cmd_enable_coex_cnt_patch),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, cmd_allocate_ppw_cnt_patch),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, cmd_set_pti_ranges_cnt_patch),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, cmd_get_stats_cnt_patch),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, cmd_wifi_sw_client_req_cnt_patch),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, cm_coex_process_cmd_cnt_patch),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, wifi_sw_client_event_to_host_cnt),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, coex_params_event_to_host_cnt),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, user_params_event_to_host_cnt),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, enable_coex_event_to_host_cnt),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, allocate_ppw_event_to_host_cnt),
	CD_SHELL_STAT(cm_fsm_patch_stats_t, set_pti_ranges_event_to_host_cnt),
};

static void cd_shell_print_stats(const struct shell *sh, const void *stats,
				 const struct cd_shell_stat *table, size_t count)
{
	unsigned int val;

	for (size_t i = 0; i < count; i++) {
		memcpy(&val, (const uint8_t *)stats + table[i].offset, sizeof(val));
		shell_print(sh, "  %-48s %u", table[i].name, val);
	}
}

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint8_t chan_map[CD_BLE_CHAN_MAP_SIZE];

	shell_print(sh, "CM configuration: %s", cd_wifi_state_name());

	if (IS_ENABLED(CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP)) {
		if (cd_ble_last_chan_map(chan_map)) {
			shell_print(sh, "BLE channel map: %02x %02x %02x %02x %02x", chan_map[0],
				    chan_map[1], chan_map[2], chan_map[3], chan_map[4]);
		} else {
			shell_print(sh, "BLE channel map: default");
		}
	}

	return 0;
}

static int cmd_stats(const struct shell *sh, size_t argc, char **argv)
{
	struct cm_stats_t stats;
	struct cm_fsm_patch_stats_t patch_stats;
	bool patch_valid = false;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = cd_stats_fetch(&stats, &patch_stats, &patch_valid);
	if (ret != 0) {
		shell_error(sh, "Failed to get CM statistics (%d)", ret);
		return ret;
	}

	shell_print(sh, "CM statistics:");
	cd_shell_print_stats(sh, &stats, cd_cm_stats, ARRAY_SIZE(cd_cm_stats));

	if (patch_valid) {
		shell_print(sh, "CM patch statistics:");
		cd_shell_print_stats(sh, &patch_stats, cd_cm_patch_stats,
				     ARRAY_SIZE(cd_cm_patch_stats));
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(nrf71_coex_cmds,
	SHELL_CMD_ARG(status, NULL, "Show the CM configuration state", cmd_status, 1, 0),
	SHELL_CMD_ARG(stats, NULL, "Read and print the CM statistics", cmd_stats, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(nrf71_coex, &nrf71_coex_cmds, "nRF71 Wi-Fi / SR coexistence", NULL);
