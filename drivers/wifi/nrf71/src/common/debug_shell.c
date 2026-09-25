/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief NRF Wi-Fi debug shell module
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/shell/shell.h>
#include <common/fw_if/nrf71_wifi_ctrl.h>
#include <common/util.h>
#include <common/mem_mgmt.h>
#include <system/fmac_api.h>
#include <system/fmac_cmd.h>
#include <system/core.h>

extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;
static struct nrf_wifi_ctx_zep *dbg_ctx = &rpu_drv_priv_zep.rpu_ctx_zep;

static int nrf_wifi_dbg_dump_rpu_stats(const struct shell *sh,
					size_t argc,
					const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct rpu_sys_op_stats stats;
	enum rpu_stats_type stats_type = RPU_STATS_TYPE_ALL;
	int ret;

	if (argc == 2) {
		const char *type  = argv[1];

		if (!strcmp(type, "umac")) {
			stats_type = RPU_STATS_TYPE_UMAC;
		} else if (!strcmp(type, "lmac")) {
			stats_type = RPU_STATS_TYPE_LMAC;
		} else if (!strcmp(type, "phy")) {
			stats_type = RPU_STATS_TYPE_PHY;
		} else if (!strcmp(type, "all")) {
			stats_type = RPU_STATS_TYPE_ALL;
		} else {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "Invalid stats type %s\n",
				      type);
			return -ENOEXEC;
		}
	}

	k_mutex_lock(&dbg_ctx->rpu_lock, K_FOREVER);
	if (!dbg_ctx->rpu_ctx) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "RPU context not initialized\n");
		ret = -ENOEXEC;
		goto unlock;
	}
	fmac_dev_ctx = dbg_ctx->rpu_ctx;

	memset(&stats, 0, sizeof(struct rpu_sys_op_stats));
	status = nrf_wifi_sys_fmac_stats_get(fmac_dev_ctx, stats_type, &stats);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Failed to get stats\n");
		ret = -ENOEXEC;
		goto unlock;
	}

	if (stats_type == RPU_STATS_TYPE_UMAC || stats_type == RPU_STATS_TYPE_ALL) {
		struct rpu_umac_stats *umac = &stats.fw.umac;

		shell_fprintf(sh, SHELL_INFO,
				  "UMAC TX debug stats:\n"
				  "======================\n"
				  "tx_cmd: %u\n"
				  "tx_non_coalesce_pkts_rcvd_from_host: %u\n"
				  "tx_coalesce_pkts_rcvd_from_host: %u\n"
				  "tx_max_coalesce_pkts_rcvd_from_host: %u\n"
				  "tx_cmds_max_used: %u\n"
				  "tx_cmds_currently_in_use: %u\n"
				  "tx_done_events_send_to_host: %u\n"
				  "tx_done_success_pkts_to_host: %u\n"
				  "tx_done_failure_pkts_to_host: %u\n"
				  "tx_cmds_with_crypto_pkts_rcvd_from_host: %u\n"
				  "tx_cmds_with_non_crypto_pkts_rcvd_from_host: %u\n"
				  "tx_cmds_with_broadcast_pkts_rcvd_from_host: %u\n"
				  "tx_cmds_with_multicast_pkts_rcvd_from_host: %u\n"
				  "tx_cmds_with_unicast_pkts_rcvd_from_host: %u\n"
				  "xmit: %u\n"
				  "send_addba_req: %u\n"
				  "addba_resp: %u\n"
				  "softmac_tx: %u\n"
				  "internal_pkts: %u\n"
				  "external_pkts: %u\n"
				  "tx_cmds_to_lmac: %u\n"
				  "tx_dones_from_lmac: %u\n"
				  "total_cmds_to_lmac: %u\n"
				  "cmdq_empty: %u\n"
				  "tx_packet_data_count: %u\n"
				  "tx_packet_mgmt_count: %u\n"
				  "tx_packet_beacon_count: %u\n"
				  "tx_packet_probe_req_count: %u\n"
				  "tx_packet_auth_count: %u\n"
				  "tx_packet_deauth_count: %u\n"
				  "tx_packet_assoc_req_count: %u\n"
				  "tx_packet_disassoc_count: %u\n"
				  "tx_packet_action_count: %u\n"
				  "tx_packet_other_mgmt_count: %u\n"
				  "tx_packet_non_mgmt_data_count: %u\n"
				  "tx_packet_eapol_count: %u\n\n",
				  umac->tx_dbg_params.tx_cmd,
				  umac->tx_dbg_params.tx_non_coalesce_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_coalesce_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_max_coalesce_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_cmds_max_used,
				  umac->tx_dbg_params.tx_cmds_currently_in_use,
				  umac->tx_dbg_params.tx_done_events_send_to_host,
				  umac->tx_dbg_params.tx_done_success_pkts_to_host,
				  umac->tx_dbg_params.tx_done_failure_pkts_to_host,
				  umac->tx_dbg_params.tx_cmds_with_crypto_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_cmds_with_non_crypto_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_cmds_with_broadcast_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_cmds_with_multicast_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_cmds_with_unicast_pkts_rcvd_from_host,
				  umac->tx_dbg_params.xmit,
				  umac->tx_dbg_params.send_addba_req,
				  umac->tx_dbg_params.addba_resp,
				  umac->tx_dbg_params.softmac_tx,
				  umac->tx_dbg_params.internal_pkts,
				  umac->tx_dbg_params.external_pkts,
				  umac->tx_dbg_params.tx_cmds_to_lmac,
				  umac->tx_dbg_params.tx_dones_from_lmac,
				  umac->tx_dbg_params.total_cmds_to_lmac,
				  umac->tx_dbg_params.cmdq_empty,
				  umac->tx_dbg_params.tx_packet_data_count,
				  umac->tx_dbg_params.tx_packet_mgmt_count,
				  umac->tx_dbg_params.tx_packet_beacon_count,
				  umac->tx_dbg_params.tx_packet_probe_req_count,
				  umac->tx_dbg_params.tx_packet_auth_count,
				  umac->tx_dbg_params.tx_packet_deauth_count,
				  umac->tx_dbg_params.tx_packet_assoc_req_count,
				  umac->tx_dbg_params.tx_packet_disassoc_count,
				  umac->tx_dbg_params.tx_packet_action_count,
				  umac->tx_dbg_params.tx_packet_other_mgmt_count,
				  umac->tx_dbg_params.tx_packet_non_mgmt_data_count,
				  umac->tx_dbg_params.tx_packet_eapol_count);

		shell_fprintf(sh, SHELL_INFO,
				  "UMAC RX debug stats\n"
				  "======================\n"
				  "lmac_events: %u\n"
				  "rx_events: %u\n"
				  "rx_coalesce_events: %u\n"
				  "total_rx_pkts_from_lmac: %u\n"
				  "max_refill_gap: %u\n"
				  "current_refill_gap: %u\n"
				  "out_of_order_mpdus: %u\n"
				  "reorder_free_mpdus: %u\n"
				  "umac_consumed_pkts: %u\n"
				  "host_consumed_pkts: %u\n"
				  "reordering_ampdu: %u\n"
				  "userspace_offload_frames: %u\n"
				  "rx_packet_total_count: %u\n"
				  "rx_packet_data_count: %u\n"
				  "rx_packet_qos_data_count: %u\n"
				  "rx_packet_protected_data_count: %u\n"
				  "rx_packet_mgmt_count: %u\n"
				  "rx_packet_beacon_count: %u\n"
				  "rx_packet_probe_resp_count: %u\n"
				  "rx_packet_auth_count: %u\n"
				  "rx_packet_deauth_count: %u\n"
				  "rx_packet_assoc_resp_count: %u\n"
				  "rx_packet_disassoc_count: %u\n"
				  "rx_packet_action_count: %u\n"
				  "rx_packet_probe_req_count: %u\n"
				  "rx_packet_other_mgmt_count: %u\n"
				  "max_coalesce_pkts: %u\n"
				  "null_skb_pointer_from_lmac: %u\n"
				  "null_skb_pointer_from_host: %u\n"
				  "null_skb_pointer_resubmitted: %u\n"
				  "reorder_flush_pkt_count: %u\n"
				  "unsecured_data_error: %u\n"
				  "pkts_in_null_skb_pointer_event: %u\n"
				  "rx_buffs_resubmit_cnt: %u\n"
				  "rx_packet_amsdu_cnt: %u\n"
				  "rx_packet_mpdu_cnt: %u\n"
				  "rx_err_secondary_pkt: %u\n"
				  "rx_err_invalid_pkt_info_type: %u\n"
				  "unexpected_mgmt_pkt: %u\n\n",
				  umac->rx_dbg_params.lmac_events,
				  umac->rx_dbg_params.rx_events,
				  umac->rx_dbg_params.rx_coalesce_events,
				  umac->rx_dbg_params.total_rx_pkts_from_lmac,
				  umac->rx_dbg_params.max_refill_gap,
				  umac->rx_dbg_params.current_refill_gap,
				  umac->rx_dbg_params.out_of_order_mpdus,
				  umac->rx_dbg_params.reorder_free_mpdus,
				  umac->rx_dbg_params.umac_consumed_pkts,
				  umac->rx_dbg_params.host_consumed_pkts,
				  umac->rx_dbg_params.reordering_ampdu,
				  umac->rx_dbg_params.userspace_offload_frames,
				  umac->rx_dbg_params.rx_packet_total_count,
				  umac->rx_dbg_params.rx_packet_data_count,
				  umac->rx_dbg_params.rx_packet_qos_data_count,
				  umac->rx_dbg_params.rx_packet_protected_data_count,
				  umac->rx_dbg_params.rx_packet_mgmt_count,
				  umac->rx_dbg_params.rx_packet_beacon_count,
				  umac->rx_dbg_params.rx_packet_probe_resp_count,
				  umac->rx_dbg_params.rx_packet_auth_count,
				  umac->rx_dbg_params.rx_packet_deauth_count,
				  umac->rx_dbg_params.rx_packet_assoc_resp_count,
				  umac->rx_dbg_params.rx_packet_disassoc_count,
				  umac->rx_dbg_params.rx_packet_action_count,
				  umac->rx_dbg_params.rx_packet_probe_req_count,
				  umac->rx_dbg_params.rx_packet_other_mgmt_count,
				  umac->rx_dbg_params.max_coalesce_pkts,
				  umac->rx_dbg_params.null_skb_pointer_from_lmac,
				  umac->rx_dbg_params.null_skb_pointer_from_host,
				  umac->rx_dbg_params.null_skb_pointer_resubmitted,
				  umac->rx_dbg_params.reorder_flush_pkt_count,
				  umac->rx_dbg_params.unsecured_data_error,
				  umac->rx_dbg_params.pkts_in_null_skb_pointer_event,
				  umac->rx_dbg_params.rx_buffs_resubmit_cnt,
				  umac->rx_dbg_params.rx_packet_amsdu_cnt,
				  umac->rx_dbg_params.rx_packet_mpdu_cnt,
				  umac->rx_dbg_params.rx_err_secondary_pkt,
				  umac->rx_dbg_params.rx_err_invalid_pkt_info_type,
				  umac->rx_dbg_params.unexpected_mgmt_pkt);

		shell_fprintf(sh, SHELL_INFO,
				  "UMAC control path stats\n"
				  "======================\n"
				  "cmd_init: %u\n"
				  "event_init_done: %u\n"
				  "cmd_rf_test: %u\n"
				  "cmd_connect: %u\n"
				  "cmd_get_stats: %u\n"
				  "event_ps_state: %u\n"
				  "cmd_set_reg: %u\n"
				  "cmd_get_reg: %u\n"
				  "cmd_req_set_reg: %u\n"
				  "cmd_trigger_scan: %u\n"
				  "event_scan_done: %u\n"
				  "cmd_get_scan: %u\n"
				  "scan_cmd_fail: %u\n"
				  "umac_scan_req: %u\n"
				  "umac_scan_complete: %u\n"
				  "umac_scan_busy: %u\n"
				  "umac_scan_abort: %u\n"
				  "umac_scan_abort_complete: %u\n"
				  "umac_scan_abort_fail: %u\n"
				  "cmd_auth: %u\n"
				  "cmd_assoc: %u\n"
				  "cmd_deauth: %u\n"
				  "cmd_register_frame: %u\n"
				  "cmd_frame: %u\n"
				  "cmd_del_key: %u\n"
				  "cmd_new_key: %u\n"
				  "cmd_set_key: %u\n"
				  "cmd_get_key: %u\n"
				  "event_beacon_hint: %u\n"
				  "event_reg_change: %u\n"
				  "event_wiphy_reg_change: %u\n"
				  "cmd_set_station: %u\n"
				  "cmd_new_station: %u\n"
				  "cmd_del_station: %u\n"
				  "cmd_new_interface: %u\n"
				  "cmd_set_interface: %u\n"
				  "cmd_get_interface: %u\n"
				  "cmd_set_ifflags: %u\n"
				  "cmd_set_ifflags_done: %u\n"
				  "cmd_set_bss: %u\n"
				  "cmd_set_wiphy: %u\n"
				  "cmd_start_ap: %u\n"
				  "LMAC_CMD_PS: %u\n"
				  "CURR_STATE: %u\n\n",
				  umac->cmd_evnt_dbg_params.cmd_init,
				  umac->cmd_evnt_dbg_params.event_init_done,
				  umac->cmd_evnt_dbg_params.cmd_rf_test,
				  umac->cmd_evnt_dbg_params.cmd_connect,
				  umac->cmd_evnt_dbg_params.cmd_get_stats,
				  umac->cmd_evnt_dbg_params.event_ps_state,
				  umac->cmd_evnt_dbg_params.cmd_set_reg,
				  umac->cmd_evnt_dbg_params.cmd_get_reg,
				  umac->cmd_evnt_dbg_params.cmd_req_set_reg,
				  umac->cmd_evnt_dbg_params.cmd_trigger_scan,
				  umac->cmd_evnt_dbg_params.event_scan_done,
				  umac->cmd_evnt_dbg_params.cmd_get_scan,
				  umac->cmd_evnt_dbg_params.scan_cmd_fail,
				  umac->cmd_evnt_dbg_params.umac_scan_req,
				  umac->cmd_evnt_dbg_params.umac_scan_complete,
				  umac->cmd_evnt_dbg_params.umac_scan_busy,
				  umac->cmd_evnt_dbg_params.umac_scan_abort,
				  umac->cmd_evnt_dbg_params.umac_scan_abort_complete,
				  umac->cmd_evnt_dbg_params.umac_scan_abort_fail,
				  umac->cmd_evnt_dbg_params.cmd_auth,
				  umac->cmd_evnt_dbg_params.cmd_assoc,
				  umac->cmd_evnt_dbg_params.cmd_deauth,
				  umac->cmd_evnt_dbg_params.cmd_register_frame,
				  umac->cmd_evnt_dbg_params.cmd_frame,
				  umac->cmd_evnt_dbg_params.cmd_del_key,
				  umac->cmd_evnt_dbg_params.cmd_new_key,
				  umac->cmd_evnt_dbg_params.cmd_set_key,
				  umac->cmd_evnt_dbg_params.cmd_get_key,
				  umac->cmd_evnt_dbg_params.event_beacon_hint,
				  umac->cmd_evnt_dbg_params.event_reg_change,
				  umac->cmd_evnt_dbg_params.event_wiphy_reg_change,
				  umac->cmd_evnt_dbg_params.cmd_set_station,
				  umac->cmd_evnt_dbg_params.cmd_new_station,
				  umac->cmd_evnt_dbg_params.cmd_del_station,
				  umac->cmd_evnt_dbg_params.cmd_new_interface,
				  umac->cmd_evnt_dbg_params.cmd_set_interface,
				  umac->cmd_evnt_dbg_params.cmd_get_interface,
				  umac->cmd_evnt_dbg_params.cmd_set_ifflags,
				  umac->cmd_evnt_dbg_params.cmd_set_ifflags_done,
				  umac->cmd_evnt_dbg_params.cmd_set_bss,
				  umac->cmd_evnt_dbg_params.cmd_set_wiphy,
				  umac->cmd_evnt_dbg_params.cmd_start_ap,
				  umac->cmd_evnt_dbg_params.LMAC_CMD_PS,
				  umac->cmd_evnt_dbg_params.CURR_STATE);

			shell_fprintf(sh, SHELL_INFO,
				  "UMAC interface stats\n"
				  "======================\n"
				  "tx_unicast_pkt_count: %u\n"
				  "tx_multicast_pkt_count: %u\n"
				  "tx_broadcast_pkt_count: %u\n"
				  "tx_bytes: %u\n"
				  "rx_unicast_pkt_count: %u\n"
				  "rx_multicast_pkt_count: %u\n"
				  "rx_broadcast_pkt_count: %u\n"
				  "rx_beacon_success_count: %u\n"
				  "rx_beacon_miss_count: %u\n"
				  "rx_bytes: %u\n"
				  "rx_checksum_error_count: %u\n\n"
				  "replay_attack_drop_cnt: %u\n\n",
				  umac->interface_data_stats.tx_unicast_pkt_count,
				  umac->interface_data_stats.tx_multicast_pkt_count,
				  umac->interface_data_stats.tx_broadcast_pkt_count,
				  umac->interface_data_stats.tx_bytes,
				  umac->interface_data_stats.rx_unicast_pkt_count,
				  umac->interface_data_stats.rx_multicast_pkt_count,
				  umac->interface_data_stats.rx_broadcast_pkt_count,
				  umac->interface_data_stats.rx_beacon_success_count,
				  umac->interface_data_stats.rx_beacon_miss_count,
				  umac->interface_data_stats.rx_bytes,
				  umac->interface_data_stats.rx_checksum_error_count,
				  umac->interface_data_stats.replay_attack_drop_cnt);
	}

	if (stats_type == RPU_STATS_TYPE_LMAC || stats_type == RPU_STATS_TYPE_ALL) {
		struct rpu_lmac_stats *lmac = &stats.fw.lmac;

		shell_fprintf(sh, SHELL_INFO,
			      "LMAC stats\n"
				  "======================\n"
				  "reset_cmd_cnt: %u\n"
				  "reset_complete_event_cnt: %u\n"
				  "unable_gen_event: %u\n"
				  "ch_prog_cmd_cnt: %u\n"
				  "channel_prog_done: %u\n"
				  "tx_pkt_cnt: %u\n"
				  "tx_pkt_done_cnt: %u\n"
				  "scan_pkt_cnt: %u\n"
				  "internal_pkt_cnt: %u\n"
				  "internal_pkt_done_cnt: %u\n"
				  "ack_resp_cnt: %u\n"
				  "tx_timeout: %u\n"
				  "deagg_isr: %u\n"
				  "deagg_inptr_desc_empty: %u\n"
				  "deagg_circular_buffer_full: %u\n"
				  "lmac_rxisr_cnt: %u\n"
				  "rx_decryptcnt: %u\n"
				  "process_decrypt_fail: %u\n"
				  "prepa_rx_event_fail: %u\n"
				  "rx_core_pool_full_cnt: %u\n"
				  "rx_mpdu_crc_success_cnt: %u\n"
				  "rx_mpdu_crc_fail_cnt: %u\n"
				  "rx_ofdm_crc_success_cnt: %u\n"
				  "rx_ofdm_crc_fail_cnt: %u\n"
				  "rxDSSSCrcSuccessCnt: %u\n"
				  "rxDSSSCrcFailCnt: %u\n"
				  "rx_crypto_start_cnt: %u\n"
				  "rx_crypto_done_cnt: %u\n"
				  "rx_event_buf_full: %u\n"
				  "rx_extram_buf_full: %u\n"
				  "scan_req: %u\n"
				  "scan_complete: %u\n"
				  "scan_abort_req: %u\n"
				  "scan_abort_complete: %u\n"
				  "internal_buf_pool_null: %u\n"
				  "rpu_hw_lockup_count: %u\n"
				  "rpu_hw_lockup_recovery_done: %u\n"
				  "SQIThresholdCmdsCnt: %u\n"
				  "SQIConfigCmdsCnt: %u\n"
				  "SQIEventsCnt: %u\n"
				  "SleepType: %u\n"
				  "warmBootCnt: %u\n\n",
				  lmac->reset_cmd_cnt,
				  lmac->reset_complete_event_cnt,
				  lmac->unable_gen_event,
				  lmac->ch_prog_cmd_cnt,
				  lmac->channel_prog_done,
				  lmac->tx_pkt_cnt,
				  lmac->tx_pkt_done_cnt,
				  lmac->scan_pkt_cnt,
				  lmac->internal_pkt_cnt,
				  lmac->internal_pkt_done_cnt,
				  lmac->ack_resp_cnt,
				  lmac->tx_timeout,
				  lmac->deagg_isr,
				  lmac->deagg_inptr_desc_empty,
				  lmac->deagg_circular_buffer_full,
				  lmac->lmac_rxisr_cnt,
				  lmac->rx_decryptcnt,
				  lmac->process_decrypt_fail,
				  lmac->prepa_rx_event_fail,
				  lmac->rx_core_pool_full_cnt,
				  lmac->rx_mpdu_crc_success_cnt,
				  lmac->rx_mpdu_crc_fail_cnt,
				  lmac->rx_ofdm_crc_success_cnt,
				  lmac->rx_ofdm_crc_fail_cnt,
				  lmac->rxDSSSCrcSuccessCnt,
				  lmac->rxDSSSCrcFailCnt,
				  lmac->rx_crypto_start_cnt,
				  lmac->rx_crypto_done_cnt,
				  lmac->rx_event_buf_full,
				  lmac->rx_extram_buf_full,
				  lmac->scan_req,
				  lmac->scan_complete,
				  lmac->scan_abort_req,
				  lmac->scan_abort_complete,
				  lmac->internal_buf_pool_null,
				  lmac->rpu_hw_lockup_count,
				  lmac->rpu_hw_lockup_recovery_done,
				  lmac->SQIThresholdCmdsCnt,
				  lmac->SQIConfigCmdsCnt,
				  lmac->SQIEventsCnt,
				  lmac->SleepType,
				  lmac->warmBootCnt);
	}

	if (stats_type == RPU_STATS_TYPE_PHY || stats_type == RPU_STATS_TYPE_ALL) {
		struct rpu_phy_stats *phy = &stats.fw.phy;

		shell_fprintf(sh, SHELL_INFO,
			      "PHY stats\n"
				  "======================\n"
				  "rssi_avg: %d\n"
				  "pdout_val: %u\n"
				  "ofdm_crc32_pass_cnt: %u\n"
				  "ofdm_crc32_fail_cnt: %u\n"
				  "dsss_crc32_pass_cnt: %u\n"
				  "dsss_crc32_fail_cnt: %u\n\n",
				  phy->rssi_avg,
				  phy->pdout_val,
				  phy->ofdm_crc32_pass_cnt,
				  phy->ofdm_crc32_fail_cnt,
				  phy->dsss_crc32_pass_cnt,
				  phy->dsss_crc32_fail_cnt);
	}

	ret = 0;
unlock:
	k_mutex_unlock(&dbg_ctx->rpu_lock);
	return ret;
}

static int nrf_wifi_dbg_clear_rpu_stats(const struct shell *sh,
					 size_t argc,
					 const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	enum rpu_stats_type stats_type = RPU_STATS_TYPE_ALL;
	int ret = 0;

	if (argc == 2) {
		const char *type = argv[1];

		if (!strcmp(type, "umac")) {
			stats_type = RPU_STATS_TYPE_UMAC;
		} else if (!strcmp(type, "lmac")) {
			stats_type = RPU_STATS_TYPE_LMAC;
		} else if (!strcmp(type, "phy")) {
			stats_type = RPU_STATS_TYPE_PHY;
		} else if (!strcmp(type, "all")) {
			stats_type = RPU_STATS_TYPE_ALL;
		} else {
			shell_fprintf(sh, SHELL_ERROR, "Invalid stats type %s\n", type);
			return -ENOEXEC;
		}
	}

	k_mutex_lock(&dbg_ctx->rpu_lock, K_FOREVER);
	if (!dbg_ctx->rpu_ctx) {
		shell_fprintf(sh, SHELL_ERROR, "RPU context not initialized\n");
		ret = -ENOEXEC;
		goto unlock_clear;
	}
	fmac_dev_ctx = dbg_ctx->rpu_ctx;

	status = umac_cmd_sys_clear_stats(fmac_dev_ctx, stats_type);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh, SHELL_ERROR, "Failed to clear RPU stats\n");
		ret = -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_INFO, "RPU stats cleared\n");
	}
unlock_clear:
	k_mutex_unlock(&dbg_ctx->rpu_lock);
	return ret;
}

static void dump_umac_cmd_evnt_dbg_params(const struct shell *sh, const uint8_t *p)
{
	struct umac_cmd_evnt_dbg_params s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  cmd_init=%u event_init_done=%u cmd_rf_test=%u\n",
		      s.cmd_init, s.event_init_done, s.cmd_rf_test);
	shell_fprintf(sh, SHELL_INFO, "  cmd_connect=%u cmd_get_stats=%u event_ps_state=%u\n",
		      s.cmd_connect, s.cmd_get_stats, s.event_ps_state);
	shell_fprintf(sh, SHELL_INFO, "  cmd_set_reg=%u cmd_get_reg=%u cmd_req_set_reg=%u\n",
		      s.cmd_set_reg, s.cmd_get_reg, s.cmd_req_set_reg);
	shell_fprintf(sh, SHELL_INFO, "  cmd_trigger_scan=%u event_scan_done=%u cmd_get_scan=%u\n",
		      s.cmd_trigger_scan, s.event_scan_done, s.cmd_get_scan);
	shell_fprintf(sh, SHELL_INFO, "  scan_cmd_fail=%u umac_scan_req=%u umac_scan_complete=%u\n",
		      s.scan_cmd_fail, s.umac_scan_req, s.umac_scan_complete);
	shell_fprintf(sh, SHELL_INFO,
		      "  umac_scan_busy=%u umac_scan_abort=%u umac_scan_abort_complete=%u\n",
		      s.umac_scan_busy, s.umac_scan_abort, s.umac_scan_abort_complete);
	shell_fprintf(sh, SHELL_INFO, "  umac_scan_abort_fail=%u cmd_auth=%u cmd_assoc=%u\n",
		      s.umac_scan_abort_fail, s.cmd_auth, s.cmd_assoc);
	shell_fprintf(sh, SHELL_INFO, "  cmd_deauth=%u cmd_register_frame=%u cmd_frame=%u\n",
		      s.cmd_deauth, s.cmd_register_frame, s.cmd_frame);
	shell_fprintf(sh, SHELL_INFO, "  cmd_del_key=%u cmd_new_key=%u cmd_set_key=%u\n",
		      s.cmd_del_key, s.cmd_new_key, s.cmd_set_key);
	shell_fprintf(sh, SHELL_INFO, "  cmd_get_key=%u event_beacon_hint=%u event_reg_change=%u\n",
		      s.cmd_get_key, s.event_beacon_hint, s.event_reg_change);
	shell_fprintf(sh, SHELL_INFO,
		      "  event_wiphy_reg_change=%u cmd_set_station=%u cmd_new_station=%u\n",
		      s.event_wiphy_reg_change, s.cmd_set_station, s.cmd_new_station);
	shell_fprintf(sh, SHELL_INFO,
		      "  cmd_del_station=%u cmd_new_interface=%u cmd_set_interface=%u\n",
		      s.cmd_del_station, s.cmd_new_interface, s.cmd_set_interface);
	shell_fprintf(sh, SHELL_INFO,
		      "  cmd_get_interface=%u cmd_set_ifflags=%u cmd_set_ifflags_done=%u\n",
		      s.cmd_get_interface, s.cmd_set_ifflags, s.cmd_set_ifflags_done);
	shell_fprintf(sh, SHELL_INFO, "  cmd_set_bss=%u cmd_set_wiphy=%u cmd_start_ap=%u\n",
		      s.cmd_set_bss, s.cmd_set_wiphy, s.cmd_start_ap);
	shell_fprintf(sh, SHELL_INFO, "  LMAC_CMD_PS=%u CURR_STATE=%u\n",
		      s.LMAC_CMD_PS, s.CURR_STATE);
}

static void dump_umac_tx_dbg_params(const struct shell *sh, const uint8_t *p)
{
	struct umac_tx_dbg_params s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  tx_cmd=%u tx_non_coalesce_pkts_rcvd_from_host=%u\n",
		      s.tx_cmd, s.tx_non_coalesce_pkts_rcvd_from_host);
	shell_fprintf(sh, SHELL_INFO, "  tx_coalesce_pkts_rcvd_from_host=%u\n",
		      s.tx_coalesce_pkts_rcvd_from_host);
	shell_fprintf(sh, SHELL_INFO,
		      "  tx_max_coalesce_pkts_rcvd_from_host=%u tx_cmds_max_used=%u\n",
		      s.tx_max_coalesce_pkts_rcvd_from_host, s.tx_cmds_max_used);
	shell_fprintf(sh, SHELL_INFO,
		      "  tx_cmds_currently_in_use=%u tx_done_events_send_to_host=%u\n",
		      s.tx_cmds_currently_in_use, s.tx_done_events_send_to_host);
	shell_fprintf(sh, SHELL_INFO,
		      "  tx_done_success_pkts_to_host=%u tx_done_failure_pkts_to_host=%u\n",
		      s.tx_done_success_pkts_to_host, s.tx_done_failure_pkts_to_host);
	shell_fprintf(sh, SHELL_INFO, "  tx_cmds_with_crypto_pkts_rcvd_from_host=%u\n",
		      s.tx_cmds_with_crypto_pkts_rcvd_from_host);
	shell_fprintf(sh, SHELL_INFO, "  tx_cmds_with_non_crypto_pkts_rcvd_from_host=%u\n",
		      s.tx_cmds_with_non_crypto_pkts_rcvd_from_host);
	shell_fprintf(sh, SHELL_INFO, "  tx_cmds_with_broadcast_pkts_rcvd_from_host=%u\n",
		      s.tx_cmds_with_broadcast_pkts_rcvd_from_host);
	shell_fprintf(sh, SHELL_INFO, "  tx_cmds_with_multicast_pkts_rcvd_from_host=%u\n",
		      s.tx_cmds_with_multicast_pkts_rcvd_from_host);
	shell_fprintf(sh, SHELL_INFO,
		      "  tx_cmds_with_unicast_pkts_rcvd_from_host=%u xmit=%u send_addba_req=%u\n",
		      s.tx_cmds_with_unicast_pkts_rcvd_from_host, s.xmit, s.send_addba_req);
	shell_fprintf(sh, SHELL_INFO, "  addba_resp=%u softmac_tx=%u internal_pkts=%u\n",
		      s.addba_resp, s.softmac_tx, s.internal_pkts);
	shell_fprintf(sh, SHELL_INFO,
		      "  external_pkts=%u tx_cmds_to_lmac=%u tx_dones_from_lmac=%u\n",
		      s.external_pkts, s.tx_cmds_to_lmac, s.tx_dones_from_lmac);
	shell_fprintf(sh, SHELL_INFO,
		      "  total_cmds_to_lmac=%u cmdq_empty=%u tx_packet_data_count=%u\n",
		      s.total_cmds_to_lmac, s.cmdq_empty, s.tx_packet_data_count);
	shell_fprintf(sh, SHELL_INFO, "  tx_packet_mgmt_count=%u tx_packet_beacon_count=%u\n",
		      s.tx_packet_mgmt_count, s.tx_packet_beacon_count);
	shell_fprintf(sh, SHELL_INFO, "  tx_packet_probe_req_count=%u tx_packet_auth_count=%u\n",
		      s.tx_packet_probe_req_count, s.tx_packet_auth_count);
	shell_fprintf(sh, SHELL_INFO, "  tx_packet_deauth_count=%u tx_packet_assoc_req_count=%u\n",
		      s.tx_packet_deauth_count, s.tx_packet_assoc_req_count);
	shell_fprintf(sh, SHELL_INFO, "  tx_packet_disassoc_count=%u tx_packet_action_count=%u\n",
		      s.tx_packet_disassoc_count, s.tx_packet_action_count);
	shell_fprintf(sh, SHELL_INFO,
		      "  tx_packet_other_mgmt_count=%u tx_packet_non_mgmt_data_count=%u\n",
		      s.tx_packet_other_mgmt_count, s.tx_packet_non_mgmt_data_count);
	shell_fprintf(sh, SHELL_INFO, "  tx_packet_eapol_count=%u\n",
		      s.tx_packet_eapol_count);
}

static void dump_umac_rx_dbg_params(const struct shell *sh, const uint8_t *p)
{
	struct umac_rx_dbg_params s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  lmac_events=%u rx_events=%u rx_coalesce_events=%u\n",
		      s.lmac_events, s.rx_events, s.rx_coalesce_events);
	shell_fprintf(sh, SHELL_INFO,
		      "  total_rx_pkts_from_lmac=%u max_refill_gap=%u current_refill_gap=%u\n",
		      s.total_rx_pkts_from_lmac, s.max_refill_gap, s.current_refill_gap);
	shell_fprintf(sh, SHELL_INFO,
		      "  out_of_order_mpdus=%u reorder_free_mpdus=%u umac_consumed_pkts=%u\n",
		      s.out_of_order_mpdus, s.reorder_free_mpdus, s.umac_consumed_pkts);
	shell_fprintf(sh, SHELL_INFO,
		      "  host_consumed_pkts=%u reordering_ampdu=%u userspace_offload_frames=%u\n",
		      s.host_consumed_pkts, s.reordering_ampdu, s.userspace_offload_frames);
	shell_fprintf(sh, SHELL_INFO, "  rx_packet_total_count=%u rx_packet_data_count=%u\n",
		      s.rx_packet_total_count, s.rx_packet_data_count);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_packet_qos_data_count=%u rx_packet_protected_data_count=%u\n",
		      s.rx_packet_qos_data_count, s.rx_packet_protected_data_count);
	shell_fprintf(sh, SHELL_INFO, "  rx_packet_mgmt_count=%u rx_packet_beacon_count=%u\n",
		      s.rx_packet_mgmt_count, s.rx_packet_beacon_count);
	shell_fprintf(sh, SHELL_INFO, "  rx_packet_probe_resp_count=%u rx_packet_auth_count=%u\n",
		      s.rx_packet_probe_resp_count, s.rx_packet_auth_count);
	shell_fprintf(sh, SHELL_INFO, "  rx_packet_deauth_count=%u rx_packet_assoc_resp_count=%u\n",
		      s.rx_packet_deauth_count, s.rx_packet_assoc_resp_count);
	shell_fprintf(sh, SHELL_INFO, "  rx_packet_disassoc_count=%u rx_packet_action_count=%u\n",
		      s.rx_packet_disassoc_count, s.rx_packet_action_count);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_packet_probe_req_count=%u rx_packet_other_mgmt_count=%u\n",
		      s.rx_packet_probe_req_count, s.rx_packet_other_mgmt_count);
	shell_fprintf(sh, SHELL_INFO, "  max_coalesce_pkts=%u null_skb_pointer_from_lmac=%u\n",
		      s.max_coalesce_pkts, s.null_skb_pointer_from_lmac);
	shell_fprintf(sh, SHELL_INFO,
		      "  null_skb_pointer_from_host=%u null_skb_pointer_resubmitted=%u\n",
		      s.null_skb_pointer_from_host, s.null_skb_pointer_resubmitted);
	shell_fprintf(sh, SHELL_INFO, "  unexpected_mgmt_pkt=%u reorder_flush_pkt_count=%u\n",
		      s.unexpected_mgmt_pkt, s.reorder_flush_pkt_count);
	shell_fprintf(sh, SHELL_INFO,
		      "  unsecured_data_error=%u pkts_in_null_skb_pointer_event=%u\n",
		      s.unsecured_data_error, s.pkts_in_null_skb_pointer_event);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_buffs_resubmit_cnt=%u rx_packet_amsdu_cnt=%u rx_packet_mpdu_cnt=%u\n",
		      s.rx_buffs_resubmit_cnt, s.rx_packet_amsdu_cnt, s.rx_packet_mpdu_cnt);
	shell_fprintf(sh, SHELL_INFO, "  rx_err_secondary_pkt=%u rx_err_invalid_pkt_info_type=%u\n",
		      s.rx_err_secondary_pkt, s.rx_err_invalid_pkt_info_type);
}

static void dump_umac_scan_dbg_params(const struct shell *sh, const uint8_t *p)
{
	struct umac_scan_dbg_params s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  scan_req_from_host=%u display_scan_req_from_host=%u\n",
		      s.scan_req_from_host, s.display_scan_req_from_host);
	shell_fprintf(sh, SHELL_INFO, "  connect_scan_req_from_host=%u scan_req_to_lmac_2g=%u\n",
		      s.connect_scan_req_from_host, s.scan_req_to_lmac_2g);
	shell_fprintf(sh, SHELL_INFO, "  scan_req_to_lmac_5g=%u scan_req_to_lmac_6g=%u\n",
		      s.scan_req_to_lmac_5g, s.scan_req_to_lmac_6g);
	shell_fprintf(sh, SHELL_INFO,
		      "  scan_complete_from_lmac_2g=%u scan_complete_from_lmac_5g=%u\n",
		      s.scan_complete_from_lmac_2g, s.scan_complete_from_lmac_5g);
	shell_fprintf(sh, SHELL_INFO, "  scan_complete_from_lmac_6g=%u scan_done_to_host=%u\n",
		      s.scan_complete_from_lmac_6g, s.scan_done_to_host);
}

static void dump_nrf_wifi_interface_stats(const struct shell *sh, const uint8_t *p)
{
	struct nrf_wifi_interface_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  tx_unicast_pkt_count=%u tx_multicast_pkt_count=%u\n",
		      s.tx_unicast_pkt_count, s.tx_multicast_pkt_count);
	shell_fprintf(sh, SHELL_INFO,
		      "  tx_broadcast_pkt_count=%u tx_bytes=%u rx_unicast_pkt_count=%u\n",
		      s.tx_broadcast_pkt_count, s.tx_bytes, s.rx_unicast_pkt_count);
	shell_fprintf(sh, SHELL_INFO, "  rx_multicast_pkt_count=%u rx_broadcast_pkt_count=%u\n",
		      s.rx_multicast_pkt_count, s.rx_broadcast_pkt_count);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_beacon_success_count=%u rx_beacon_miss_count=%u rx_bytes=%u\n",
		      s.rx_beacon_success_count, s.rx_beacon_miss_count, s.rx_bytes);
	shell_fprintf(sh, SHELL_INFO, "  rx_checksum_error_count=%u replay_attack_drop_cnt=%u\n",
		      s.rx_checksum_error_count, s.replay_attack_drop_cnt);
}

static void dump_umac_sleep_stats(const struct shell *sh, const uint8_t *p)
{
	struct umac_sleep_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  sleep_req=%u sleep_req_succ=%u sleep_req_fail=%u\n",
		      s.sleep_req, s.sleep_req_succ, s.sleep_req_fail);
	shell_fprintf(sh, SHELL_INFO,
		      "  sleep_status_to_lmac=%u umac_init=%u umac_init_warmboot=%u\n",
		      s.sleep_status_to_lmac, s.umac_init, s.umac_init_warmboot);
	shell_fprintf(sh, SHELL_INFO,
		      "  reorder_not_empty=%u outstanding_cmds=%u umac_init_coldboot=%u\n",
		      s.reorder_not_empty, s.outstanding_cmds, s.umac_init_coldboot);
	shell_fprintf(sh, SHELL_INFO, "  not_station=%u authenticating=%u associating=%u\n",
		      s.not_station, s.authenticating, s.associating);
	shell_fprintf(sh, SHELL_INFO,
		      "  cmd_processing=%u tx_done_pending=%u tx_cmds_currently_in_use=%u\n",
		      s.cmd_processing, s.tx_done_pending, s.tx_cmds_currently_in_use);
	shell_fprintf(sh, SHELL_INFO, "  events_pending_list_gg=%u HPQM_CMD_NOT_EMPTY=%u\n",
		      s.events_pending_list_gg, s.HPQM_CMD_NOT_EMPTY);
	shell_fprintf(sh, SHELL_INFO,
		      "  HPQM_EVENTQ_NOT_EMPTY=%u pending_addba_resp=%u get_channel=%u\n",
		      s.HPQM_EVENTQ_NOT_EMPTY, s.pending_addba_resp, s.get_channel);
	shell_fprintf(sh, SHELL_INFO, "  rx_pending=%u pre_init=%u rx_mbox_pending=%u\n",
		      s.rx_pending, s.pre_init, s.rx_mbox_pending);
	shell_fprintf(sh, SHELL_INFO,
		      "  tx_mbox_pending=%u pending_cmd_resubmit=%u umac_goto_sleep=%u\n",
		      s.tx_mbox_pending, s.pending_cmd_resubmit, s.umac_goto_sleep);
	shell_fprintf(sh, SHELL_INFO, "  max_twt_awake_cnt=%u\n",
		      s.max_twt_awake_cnt);
}

static void dump_nrf_wifi_misc_stats(const struct shell *sh, const uint8_t *p)
{
	struct nrf_wifi_misc_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  rx_mbox_post=%u rx_mbox_receive=%u timer_mbox_post=%u\n",
		      s.rx_mbox_post, s.rx_mbox_receive, s.timer_mbox_post);
	shell_fprintf(sh, SHELL_INFO, "  timer_mbox_rcv=%u work_mbox_post=%u work_mbox_rcv=%u\n",
		      s.timer_mbox_rcv, s.work_mbox_post, s.work_mbox_rcv);
	shell_fprintf(sh, SHELL_INFO,
		      "  tasklet_mbox_post=%u tasklet_mbox_rcv=%u alloc_buf_fail=%u\n",
		      s.tasklet_mbox_post, s.tasklet_mbox_rcv, s.alloc_buf_fail);
	shell_fprintf(sh, SHELL_INFO, "  twt_req_sent=%u twt_info_sent_to_lmac=%u\n",
		      s.twt_req_sent, s.twt_info_sent_to_lmac);
	shell_fprintf(sh, SHELL_INFO,
		      "  twt_teardown_info_sent_to_lmac=%u ftm_req_info_sent_to_lmac=%u\n",
		      s.twt_teardown_info_sent_to_lmac, s.ftm_req_info_sent_to_lmac);
	shell_fprintf(sh, SHELL_INFO, "  ftm_resp_rcvd=%u gas_req_sent=%u gas_resp_received=%u\n",
		      s.ftm_resp_rcvd, s.gas_req_sent, s.gas_resp_received);
	shell_fprintf(sh, SHELL_INFO,
		      "  gas_comeback_req_sent=%u neighbor_req_sent=%u neighbor_resp_received=%u\n",
		      s.gas_comeback_req_sent, s.neighbor_req_sent, s.neighbor_resp_received);
	shell_fprintf(sh, SHELL_INFO,
		      "  ipc_tx_init_fail=%u ipc_rx_init_fail=%u ipc_bind_fail=%u\n",
		      s.ipc_tx_init_fail, s.ipc_rx_init_fail, s.ipc_bind_fail);
	shell_fprintf(sh, SHELL_INFO, "  ipc_ring_buf_size_fail=%u ipc_mem_config_fail=%u\n",
		      s.ipc_ring_buf_size_fail, s.ipc_mem_config_fail);
}

static void dump_umac_raw_stats(const struct shell *sh, const uint8_t *p)
{
	struct umac_raw_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO,
		      "  raw_tx_from_host=%u raw_tx_to_lmac=%u raw_tx_dones_from_lmac=%u\n",
		      s.raw_tx_from_host, s.raw_tx_to_lmac, s.raw_tx_dones_from_lmac);
	shell_fprintf(sh, SHELL_INFO, "  raw_tx_dones_to_host=%u total_rx_pkts_from_lmac=%u\n",
		      s.raw_tx_dones_to_host, s.total_rx_pkts_from_lmac);
	shell_fprintf(sh, SHELL_INFO,
		      "  total_raw_rx_pkts_from_lmac=%u valid_raw_rx_pkts_from_lmac=%u\n",
		      s.total_raw_rx_pkts_from_lmac, s.valid_raw_rx_pkts_from_lmac);
	shell_fprintf(sh, SHELL_INFO, "  raw_rx_pkts_to_host=%u\n",
		      s.raw_rx_pkts_to_host);
}

static void dump_lmac_common_stats(const struct shell *sh, const uint8_t *p)
{
	struct lmac_common_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO,
		      "  general_purpose_timer_isr_cnt=%u lmac_task_inprogress=%u current_cmd=%u\n",
		      s.general_purpose_timer_isr_cnt, s.lmac_task_inprogress, s.current_cmd);
	shell_fprintf(sh, SHELL_INFO,
		      "  cmds_going_to_wait_list=%u unexpected_internal_cmd_during_umac_wait=%u\n",
		      s.cmds_going_to_wait_list, s.unexpected_internal_cmd_during_umac_wait);
	shell_fprintf(sh, SHELL_INFO, "  lmac_rx_isr_inprogress=%u hw_timer_isr_inprogress=%u\n",
		      s.lmac_rx_isr_inprogress, s.hw_timer_isr_inprogress);
	shell_fprintf(sh, SHELL_INFO, "  deagg_isr_inprogress=%u tx_isr_inprogress=%u\n",
		      s.deagg_isr_inprogress, s.tx_isr_inprogress);
	shell_fprintf(sh, SHELL_INFO,
		      "  channel_switch_inprogress=%u rpu_lockup_event=%u rpu_lockup_cnt=%u\n",
		      s.channel_switch_inprogress, s.rpu_lockup_event, s.rpu_lockup_cnt);
	shell_fprintf(sh, SHELL_INFO, "  rpu_lockup_recovery_done=%u reset_cmd_cnt=%u\n",
		      s.rpu_lockup_recovery_done, s.reset_cmd_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  reset_complete_event_cnt=%u commad_ent_default=%u lmac_enable_cnt=%u\n",
		      s.reset_complete_event_cnt, s.commad_ent_default, s.lmac_enable_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  lmac_disable_cnt=%u lmac_error_cnt=%u unable_gen_event=%u\n",
		      s.lmac_disable_cnt, s.lmac_error_cnt, s.unable_gen_event);
	shell_fprintf(sh, SHELL_INFO,
		      "  mem_pool_full_cnt=%u ch_prog_cmd_cnt=%u channel_prog_done=%u\n",
		      s.mem_pool_full_cnt, s.ch_prog_cmd_cnt, s.channel_prog_done);
	shell_fprintf(sh, SHELL_INFO,
		      "  connect_lost_status=%u tx_core_pool_full_cnt=%u patch_debug_cnt=%u\n",
		      s.connect_lost_status, s.tx_core_pool_full_cnt, s.patch_debug_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  fw_error_event_cnt=%u tx_deinit_cmd_cnt=%u tx_deinit_done_cnt=%u\n",
		      s.fw_error_event_cnt, s.tx_deinit_cmd_cnt, s.tx_deinit_done_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  internal_buf_pool_null=%u rx_buffer_cmd=%u wait_for_tx_done_loop=%u\n",
		      s.internal_buf_pool_null, s.rx_buffer_cmd, s.wait_for_tx_done_loop);
	shell_fprintf(sh, SHELL_INFO,
		      "  cca_busy=%u temp_measure_window_expired=%u temp_inter_pool_full=%u\n",
		      s.cca_busy, s.temp_measure_window_expired, s.temp_inter_pool_full);
	shell_fprintf(sh, SHELL_INFO,
		      "  lmac_internal_cmd=%u measure_temp_vbat=%u fresh_calib_cnt=%u\n",
		      s.lmac_internal_cmd, s.measure_temp_vbat, s.fresh_calib_cnt);
	shell_fprintf(sh, SHELL_INFO, "  coex_event_cnt=%u coex_cmd_cnt=%u coex_isr_cnt=%u\n",
		      s.coex_event_cnt, s.coex_cmd_cnt, s.coex_isr_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  block_wlan_traffic_cnt=%u un_block_wlan_traffic_cnt=%u p2p_no_a_cnt=%u\n",
		      s.block_wlan_traffic_cnt, s.un_block_wlan_traffic_cnt, s.p2p_no_a_cnt);
	shell_fprintf(sh, SHELL_INFO, "  scan_timer_task_pending=%u scan_timer_task_complete=%u\n",
		      s.scan_timer_task_pending, s.scan_timer_task_complete);
	shell_fprintf(sh, SHELL_INFO, "  coex_request_fail_cnt=%u\n",
		      s.coex_request_fail_cnt);
}

static void dump_phy_if_stats(const struct shell *sh, const uint8_t *p)
{
	struct phy_if_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO,
		      "  phy_init_fail_cnt=%u phy_ch_switch_fail_cnt=%u phy_config_fail_cnt=%u\n",
		      s.phy_init_fail_cnt, s.phy_ch_switch_fail_cnt, s.phy_config_fail_cnt);
	shell_fprintf(sh, SHELL_INFO, "  phy_config_power_cnt=%u rf_test_cmd=%u cmd_pwr_mon=%u\n",
		      s.phy_config_power_cnt, s.rf_test_cmd, s.cmd_pwr_mon);
	shell_fprintf(sh, SHELL_INFO, "  cmd_pwr_mon_all=%u vbat_mon=%u temp=%u\n",
		      s.cmd_pwr_mon_all, s.vbat_mon, s.temp);
	shell_fprintf(sh, SHELL_INFO, "  lfc_err=%u\n",
		      s.lfc_err);
}

static void dump_lmac_tx_stats(const struct shell *sh, const uint8_t *p)
{
	struct lmac_tx_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  tx_pkt_cnt=%u tx_pkt_done_cnt=%u tx_pkt_success=%u\n",
		      s.tx_pkt_cnt, s.tx_pkt_done_cnt, s.tx_pkt_success);
	shell_fprintf(sh, SHELL_INFO, "  tx_pkt_underrun=%u unknown_status=%u agg_tx_timeout=%u\n",
		      s.tx_pkt_underrun, s.unknown_status, s.agg_tx_timeout);
	shell_fprintf(sh, SHELL_INFO,
		      "  agg_phy_cca_abort=%u bt_abort_at_start=%u bt_abort_at_end=%u\n",
		      s.agg_phy_cca_abort, s.bt_abort_at_start, s.bt_abort_at_end);
	shell_fprintf(sh, SHELL_INFO,
		      "  scan_pkt_cnt=%u internal_pkt_cnt=%u internal_pkt_done_cnt=%u\n",
		      s.scan_pkt_cnt, s.internal_pkt_cnt, s.internal_pkt_done_cnt);
	shell_fprintf(sh, SHELL_INFO, "  dcp_submit_cnt=%u edca_isr_cnt=%u tx_dma_complete=%u\n",
		      s.dcp_submit_cnt, s.edca_isr_cnt, s.tx_dma_complete);
	shell_fprintf(sh, SHELL_INFO, "  ack_resp_cnt=%u tx_timeout=%u tx_drop_cnt=%u\n",
		      s.ack_resp_cnt, s.tx_timeout, s.tx_drop_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  blocked_tx_pkt_cnt=%u dbg_tx_invalid_tx_vect=%u dbg_tx_abort_cnt=%u\n",
		      s.blocked_tx_pkt_cnt, s.dbg_tx_invalid_tx_vect, s.dbg_tx_abort_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  ignore_ack_frame=%u edca_tx_abort_cnt=%u tx_block_ack_fail_cnt=%u\n",
		      s.ignore_ack_frame, s.edca_tx_abort_cnt, s.tx_block_ack_fail_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  data_frame_cnt=%u qos_data_frame_cnt=%u ctrl_frame_cnt=%u\n",
		      s.data_frame_cnt, s.qos_data_frame_cnt, s.ctrl_frame_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  mgmt_frame_cnt=%u beacon_frame_cnt=%u auth_req_frame_cnt=%u\n",
		      s.mgmt_frame_cnt, s.beacon_frame_cnt, s.auth_req_frame_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  deauth_frame_cnt=%u assoc_req_frame_cnt=%u assoc_response_frame_cnt=%u\n",
		      s.deauth_frame_cnt, s.assoc_req_frame_cnt, s.assoc_response_frame_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  re_assoc_req_frame_cnt=%u re_assoc_response_frame_cnt=%u\n",
		      s.re_assoc_req_frame_cnt, s.re_assoc_response_frame_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  de_assoc_req_cnt=%u probe_req_cnt=%u probe_response_cnt=%u\n",
		      s.de_assoc_req_cnt, s.probe_req_cnt, s.probe_response_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  ht_ctrl_field=%u keep_alive_frame_success=%u keep_alive_frame_fail=%u\n",
		      s.ht_ctrl_field, s.keep_alive_frame_success, s.keep_alive_frame_fail);
}

static void dump_lmac_rx_stats(const struct shell *sh, const uint8_t *p)
{
	struct lmac_rx_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO,
		      "  deagg_isr=%u lmac_rxisr_cnt=%u lmac_rx_isr_dropped_cnt=%u\n",
		      s.deagg_isr, s.lmac_rxisr_cnt, s.lmac_rx_isr_dropped_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_ctrl_mem_full=%u rx_total_mpdu_cnt=%u rx_mpdu_crc_fail_cnt=%u\n",
		      s.rx_ctrl_mem_full, s.rx_total_mpdu_cnt, s.rx_mpdu_crc_fail_cnt);
	shell_fprintf(sh, SHELL_INFO, "  rx_mpdu_crc_success_cnt=%u rx_ofdm_crc_success_cnt=%u\n",
		      s.rx_mpdu_crc_success_cnt, s.rx_ofdm_crc_success_cnt);
	shell_fprintf(sh, SHELL_INFO, "  rx_ofdm_crc_fail_cnt=%u rx_dsss_crc_success_cnt=%u\n",
		      s.rx_ofdm_crc_fail_cnt, s.rx_dsss_crc_success_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_dsss_crc_fail_cnt=%u dbg_mic_error=%u dbg_icv_error=%u\n",
		      s.rx_dsss_crc_fail_cnt, s.dbg_mic_error, s.dbg_icv_error);
	shell_fprintf(sh, SHELL_INFO, "  ndpa_frame_cnt=%u rts_cts=%u rx_ack_cnt=%u\n",
		      s.ndpa_frame_cnt, s.rts_cts, s.rx_ack_cnt);
	shell_fprintf(sh, SHELL_INFO, "  rx_ba_cnt=%u rx_ack_process_cnt=%u rx_ba_process_cnt=%u\n",
		      s.rx_ba_cnt, s.rx_ack_process_cnt, s.rx_ba_process_cnt);
	shell_fprintf(sh, SHELL_INFO, "  rx_mcst_filter_fail=%u rx_mcst_filter_success=%u\n",
		      s.rx_mcst_filter_fail, s.rx_mcst_filter_success);
	shell_fprintf(sh, SHELL_INFO,
		      "  mcst_bcst_frame_for_dut=%u rx_ucast_frame=%u frame_not_for_dut=%u\n",
		      s.mcst_bcst_frame_for_dut, s.rx_ucast_frame, s.frame_not_for_dut);
	shell_fprintf(sh, SHELL_INFO, "  ucast_frame_for_dut=%u bcst_frame=%u packets_to_host=%u\n",
		      s.ucast_frame_for_dut, s.bcst_frame, s.packets_to_host);
	shell_fprintf(sh, SHELL_INFO, "  packets_dropped_in_lmac=%u deagg_inptr_desc_empty=%u\n",
		      s.packets_dropped_in_lmac, s.deagg_inptr_desc_empty);
	shell_fprintf(sh, SHELL_INFO,
		      "  deagg_circular_buffer_full=%u rx_decrypt_cnt=%u process_decrypt_fail=%u\n",
		      s.deagg_circular_buffer_full, s.rx_decrypt_cnt, s.process_decrypt_fail);
	shell_fprintf(sh, SHELL_INFO,
		      "  prepa_rx_event_fail=%u rx_bcst_frame=%u rx_mcst_frame=%u\n",
		      s.prepa_rx_event_fail, s.rx_bcst_frame, s.rx_mcst_frame);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_unicast_frame=%u unicast_addr_match=%u unicast_addr_mismatch=%u\n",
		      s.rx_unicast_frame, s.unicast_addr_match, s.unicast_addr_mismatch);
	shell_fprintf(sh, SHELL_INFO, "  bcst_mcst_ours=%u bcst_mcst_non_network_frames=%u\n",
		      s.bcst_mcst_ours, s.bcst_mcst_non_network_frames);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_crypto_start_cnt=%u rx_crypto_done_cnt=%u rx_dma_start_cnt=%u\n",
		      s.rx_crypto_start_cnt, s.rx_crypto_done_cnt, s.rx_dma_start_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_dma_done_cnt=%u rx_hdr_dma_job_cnt=%u rx_deadlock_cnt=%u\n",
		      s.rx_dma_done_cnt, s.rx_hdr_dma_job_cnt, s.rx_deadlock_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_event_buf_full=%u rx_extram_buf_full=%u key_not_found=%u\n",
		      s.rx_event_buf_full, s.rx_extram_buf_full, s.key_not_found);
	shell_fprintf(sh, SHELL_INFO, "  rx_key_found=%u rx_packet=%u amsdu_packet_cnt=%u\n",
		      s.rx_key_found, s.rx_packet, s.amsdu_packet_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  amsdu_process_fail_cnt=%u rx_frag_packet_cnt=%u rx_defrag_fail_cnt=%u\n",
		      s.amsdu_process_fail_cnt, s.rx_frag_packet_cnt, s.rx_defrag_fail_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  unexpected_cnt=%u unexpected_cnt1=%u rx_job_success_cnt=%u\n",
		      s.unexpected_cnt, s.unexpected_cnt1, s.rx_job_success_cnt);
	shell_fprintf(sh, SHELL_INFO, "  channel_switch_announcement_cnt=%u\n",
		      s.channel_switch_announcement_cnt);
}

static void dump_lmac_scan_stats(const struct shell *sh, const uint8_t *p)
{
	struct lmac_scan_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  scan_req=%u scan_complete=%u scan_abort_req=%u\n",
		      s.scan_req, s.scan_complete, s.scan_abort_req);
	shell_fprintf(sh, SHELL_INFO,
		      "  scan_abort_complete=%u scan_probe_fail=%u scan_function_inprogress=%u\n",
		      s.scan_abort_complete, s.scan_probe_fail, s.scan_function_inprogress);
	shell_fprintf(sh, SHELL_INFO,
		      "  scan_event=%u current_scan_channel=%u current_scan_band=%u\n",
		      s.scan_event, s.current_scan_channel, s.current_scan_band);
}

static void dump_lmac_sleep_stats(const struct shell *sh, const uint8_t *p)
{
	struct lmac_sleep_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  sleep_command_in_lmac_task=%u sleep_disable_cnt=%u\n",
		      s.sleep_command_in_lmac_task, s.sleep_disable_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  wifi_powersave_disabled=%u total_boot_cnt=%u warm_boot_timer_isr=%u\n",
		      s.wifi_powersave_disabled, s.total_boot_cnt, s.warm_boot_timer_isr);
	shell_fprintf(sh, SHELL_INFO,
		      "  try_to_enter_sleep=%u pre_assoc_timer_cnt=%u wait_for_bcn_expired=%u\n",
		      s.try_to_enter_sleep, s.pre_assoc_timer_cnt, s.wait_for_bcn_expired);
	shell_fprintf(sh, SHELL_INFO,
		      "  rx_bcn_cnt=%u wait_for_bcn_cnt=%u wait_for_unicast_cnt=%u\n",
		      s.rx_bcn_cnt, s.wait_for_bcn_cnt, s.wait_for_unicast_cnt);
	shell_fprintf(sh, SHELL_INFO, "  wait_for_broadcast_cnt=%u cancel_warm_boot_timer=%u\n",
		      s.wait_for_broadcast_cnt, s.cancel_warm_boot_timer);
	shell_fprintf(sh, SHELL_INFO,
		      "  more_buffered_unicast=%u buffered_unicast=%u more_buffered_broadcast=%u\n",
		      s.more_buffered_unicast, s.buffered_unicast, s.more_buffered_broadcast);
	shell_fprintf(sh, SHELL_INFO,
		      "  buffered_broadcast=%u sleep_attempt_fail_power_save_off=%u\n",
		      s.buffered_broadcast, s.sleep_attempt_fail_power_save_off);
	shell_fprintf(sh, SHELL_INFO, "  sleep_attempt_fail_ftm_responder_active=%u\n",
		      s.sleep_attempt_fail_ftm_responder_active);
	shell_fprintf(sh, SHELL_INFO,
		      "  sleep_attempt_fail_vif_non_sta=%u sleep_attempt_fail_cmds_present=%u\n",
		      s.sleep_attempt_fail_vif_non_sta, s.sleep_attempt_fail_cmds_present);
	shell_fprintf(sh, SHELL_INFO, "  scan_in_progress=%u tx_pkt_pending=%u host_cmds=%u\n",
		      s.scan_in_progress, s.tx_pkt_pending, s.host_cmds);
	shell_fprintf(sh, SHELL_INFO, "  host_events=%u rx_in_progress=%u tx_in_progress=%u\n",
		      s.host_events, s.rx_in_progress, s.tx_in_progress);
	shell_fprintf(sh, SHELL_INFO,
		      "  sleep_request_failed=%u attempt_sleep_req_cnt=%u warm_boot_cnt=%u\n",
		      s.sleep_request_failed, s.attempt_sleep_req_cnt, s.warm_boot_cnt);
	shell_fprintf(sh, SHELL_INFO, "  wakeup_rpu_enabled=%u wakeup_now=%u sleep_failed=%u\n",
		      s.wakeup_rpu_enabled, s.wakeup_now, s.sleep_failed);
	shell_fprintf(sh, SHELL_INFO,
		      "  cmd_ps_from_host=%u power_save_indication_to_ap_success=%u\n",
		      s.cmd_ps_from_host, s.power_save_indication_to_ap_success);
	shell_fprintf(sh, SHELL_INFO,
		      "  power_save_indication_to_ap_fail=%u post_assoc_int_command=%u\n",
		      s.power_save_indication_to_ap_fail, s.post_assoc_int_command);
	shell_fprintf(sh, SHELL_INFO,
		      "  post_assoc_check_lmac_activity_fail=%u too_less_to_sleep=%u\n",
		      s.post_assoc_check_lmac_activity_fail, s.too_less_to_sleep);
	shell_fprintf(sh, SHELL_INFO,
		      "  sleep_time_in_us=%u sleep_request_to_umac=%u sleep_status_from_umac=%u\n",
		      s.sleep_time_in_us, s.sleep_request_to_umac, s.sleep_status_from_umac);
	shell_fprintf(sh, SHELL_INFO,
		      "  sleep_request_to_umac_failed=%u hal_event_cnt=%u rx_event_pending=%u\n",
		      s.sleep_request_to_umac_failed, s.hal_event_cnt, s.rx_event_pending);
	shell_fprintf(sh, SHELL_INFO, "  seq_nu=%u grtc_isrcnt=%u bb_isrcnt=%u\n",
		      s.seq_nu, s.grtc_isrcnt, s.bb_isrcnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  lp_bringup_time=%u hp_bringup_time=%u lp2hp_bringup_time=%u\n",
		      s.lp_bringup_time, s.hp_bringup_time, s.lp2hp_bringup_time);
}

static void dump_lmac_sleep_timing_stats(const struct shell *sh, const uint8_t *p)
{
	struct lmac_sleep_timing_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO,
		      "  index=%u bcn_delay_after_wakeup=%u last_one_minute_bcn_rcv_cnt=%u\n",
		      s.index, s.bcn_delay_after_wakeup, s.last_one_minute_bcn_rcv_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  last_one_second_bcn_rcv_cnt=%u current_one_minute_bcn_rcv_cnt=%u\n",
		      s.last_one_second_bcn_rcv_cnt, s.current_one_minute_bcn_rcv_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  current_one_second_bcn_rcv_cnt=%u temp_time_stamp_minute=%u\n",
		      s.current_one_second_bcn_rcv_cnt, s.temp_time_stamp_minute);
	shell_fprintf(sh, SHELL_INFO, "  temp_time_stamp_second=%u\n",
		      s.temp_time_stamp_second);

	for (int i = 0; i < SLEEP_DEBUG_BUFFER; i++) {
		shell_fprintf(sh, SHELL_INFO, "  wake_duration[%d]=%u sleep_duration[%d]=%u\n",
			      i, s.wake_duration[i], i, s.sleep_duration[i]);
	}

	/* The double members are printed as integers to avoid FP printf support */
	for (int i = 0; i < INTERVAL_COUNT; i++) {
		shell_fprintf(sh, SHELL_INFO, "  time_stamps[%d]=%u\n",
			      i, (unsigned int)s.time_stamps[i]);
		shell_fprintf(sh, SHELL_INFO,
			      "  last_wake_sleep_stats[%d]=%u,%u cur_wake_sleep_stats[%d]=%u,%u\n",
			      i, (unsigned int)s.last_wake_sleep_stats[i][0],
			      (unsigned int)s.last_wake_sleep_stats[i][1],
			      i, (unsigned int)s.cur_wake_sleep_stats[i][0],
			      (unsigned int)s.cur_wake_sleep_stats[i][1]);
	}
}

static void dump_lmac_twt_stats(const struct shell *sh, const uint8_t *p)
{
	struct lmac_twt_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  twt_isr=%u twt_isr_start=%u twt_isr_end=%u\n",
		      s.twt_isr, s.twt_isr_start, s.twt_isr_end);
	shell_fprintf(sh, SHELL_INFO, "  basic_trigger_cnt=%u twt_req_cnt=%u twt_teardown_cnt=%u\n",
		      s.basic_trigger_cnt, s.twt_req_cnt, s.twt_teardown_cnt);
	shell_fprintf(sh, SHELL_INFO, "  twt_isr_state=%u block_cnt=%u unblock_cnt=%u\n",
		      s.twt_isr_state, s.block_cnt, s.unblock_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  twt_sleep_req_to_umac=%u twt_sleep_resp_succ=%u twt_sleep_resp_fail=%u\n",
		      s.twt_sleep_req_to_umac, s.twt_sleep_resp_succ, s.twt_sleep_resp_fail);
	shell_fprintf(sh, SHELL_INFO,
		      "  boot_awake_window=%u boot_sleep_window=%u boot_next_twt_window=%u\n",
		      s.boot_awake_window, s.boot_sleep_window, s.boot_next_twt_window);
	shell_fprintf(sh, SHELL_INFO,
		      "  isr_awake_window=%u isr_sleep_window=%u isr_less_sleep_window=%u\n",
		      s.isr_awake_window, s.isr_sleep_window, s.isr_less_sleep_window);
	shell_fprintf(sh, SHELL_INFO, "  isr_next_twt_window=%u lmac_busy=%u isr_flush_cnt=%u\n",
		      s.isr_next_twt_window, s.lmac_busy, s.isr_flush_cnt);
	shell_fprintf(sh, SHELL_INFO, "  flush_cnt=%u resync_cnt=%u wait_for_bcn_tsf=%u\n",
		      s.flush_cnt, s.resync_cnt, s.wait_for_bcn_tsf);
	shell_fprintf(sh, SHELL_INFO,
		      "  twt_less_sleep_time=%u invalid_start_tsf1=%u invalid_start_tsf2=%u\n",
		      s.twt_less_sleep_time, s.invalid_start_tsf1, s.invalid_start_tsf2);
	shell_fprintf(sh, SHELL_INFO,
		      "  invalid_start_tsf3=%u invalid_target_wake_interval=%u ps_poll_sent=%u\n",
		      s.invalid_start_tsf3, s.invalid_target_wake_interval, s.ps_poll_sent);
	shell_fprintf(sh, SHELL_INFO, "  debug_sleep_time=%u time_stamp_before_sleep=%u\n",
		      s.debug_sleep_time, s.time_stamp_before_sleep);
	shell_fprintf(sh, SHELL_INFO, "  time_stamp_after_sleep=%u\n",
		      s.time_stamp_after_sleep);
}

static void dump_lmac_he_stats(const struct shell *sh, const uint8_t *p)
{
	struct lmac_he_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  trigger_pkt_cnt=%u trigger_aid_matched_cnt=%u basic=%u\n",
		      s.trigger_pkt_cnt, s.trigger_aid_matched_cnt, s.basic);
	shell_fprintf(sh, SHELL_INFO, "  bfrp=%u mu_bar=%u mu_rts=%u\n",
		      s.bfrp, s.mu_bar, s.mu_rts);
	shell_fprintf(sh, SHELL_INFO, "  bsrp=%u gcr_mu_bar=%u bqrp=%u\n",
		      s.bsrp, s.gcr_mu_bar, s.bqrp);
	shell_fprintf(sh, SHELL_INFO, "  nfrp=%u aid_matched_basic=%u aid_matched_bfrp=%u\n",
		      s.nfrp, s.aid_matched_basic, s.aid_matched_bfrp);
	shell_fprintf(sh, SHELL_INFO,
		      "  aid_matched_mu_bar=%u aid_matched_mu_rts=%u aid_matched_bsrp=%u\n",
		      s.aid_matched_mu_bar, s.aid_matched_mu_rts, s.aid_matched_bsrp);
	shell_fprintf(sh, SHELL_INFO,
		      "  aid_matched_gcr_mu_bar=%u aid_matched_bqrp=%u aid_matched_nfrp=%u\n",
		      s.aid_matched_gcr_mu_bar, s.aid_matched_bqrp, s.aid_matched_nfrp);
	shell_fprintf(sh, SHELL_INFO, "  edca_switch_cnt=%u mu_edca_triggered_cnt=%u\n",
		      s.edca_switch_cnt, s.mu_edca_triggered_cnt);
}

static void dump_ftm_debug_stats(const struct shell *sh, const uint8_t *p)
{
	struct ftm_debug_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO,
		      "  timer_start_cnt=%u timer_cnt=%u ftm_initiator_cmd_cnt=%u\n",
		      s.timer_start_cnt, s.timer_cnt, s.ftm_initiator_cmd_cnt);
	shell_fprintf(sh, SHELL_INFO, "  ftm_initiator_cmd_dropped=%u ftm_closing_done_event=%u\n",
		      s.ftm_initiator_cmd_dropped, s.ftm_closing_done_event);
	shell_fprintf(sh, SHELL_INFO, "  beacon_captured=%u beacon_not_captured=%u ftm_event=%u\n",
		      s.beacon_captured, s.beacon_not_captured, s.ftm_event);
	shell_fprintf(sh, SHELL_INFO, "  ftm_event_failed=%u ftm_timer_expired=%u\n",
		      s.ftm_event_failed, s.ftm_timer_expired);
	shell_fprintf(sh, SHELL_INFO,
		      "  ftm_initiator_cpy_params_cnt=%u ftm_null_frm_send_success=%u\n",
		      s.ftm_initiator_cpy_params_cnt, s.ftm_null_frm_send_success);
	shell_fprintf(sh, SHELL_INFO,
		      "  ftm_null_frm_send_fail=%u chnl_switch_done=%u asap_case=%u\n",
		      s.ftm_null_frm_send_fail, s.chnl_switch_done, s.asap_case);
	shell_fprintf(sh, SHELL_INFO,
		      "  ftm_failed_no_response=%u initiate_ftm_burst_cnt=%u ftmi_trigger_cmd=%u\n",
		      s.ftm_failed_no_response, s.initiate_ftm_burst_cnt, s.ftmi_trigger_cmd);
	shell_fprintf(sh, SHELL_INFO,
		      "  ftm_fr_send_cmd=%u send_ftm_frame_cnt=%u send_ftm_frame_done_cnt=%u\n",
		      s.ftm_fr_send_cmd, s.send_ftm_frame_cnt, s.send_ftm_frame_done_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  trigger_ftm_burst_cnt=%u send_initial_ftm_request_frame_fail_cnt=%u\n",
		      s.trigger_ftm_burst_cnt, s.send_initial_ftm_request_frame_fail_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  send_initial_ftm_request_frame_done_cnt=%u off_channel_switch_cnt=%u\n",
		      s.send_initial_ftm_request_frame_done_cnt, s.off_channel_switch_cnt);
	shell_fprintf(sh, SHELL_INFO, "  return_to_working_channel_cnt=%u done_evt_failed_cnt=%u\n",
		      s.return_to_working_channel_cnt, s.done_evt_failed_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  done_evt_ap_busy_cnt=%u mem_error_cnt=%u ftm_request_received_cnt=%u\n",
		      s.done_evt_ap_busy_cnt, s.mem_error_cnt, s.ftm_request_received_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  ftm_request_initial_received_cnt=%u ftm_params_decode_error_cnt=%u\n",
		      s.ftm_request_initial_received_cnt, s.ftm_params_decode_error_cnt);
	shell_fprintf(sh, SHELL_INFO, "  ftm_request_incapable_cnt=%u ftm_request_failed_cnt=%u\n",
		      s.ftm_request_incapable_cnt, s.ftm_request_failed_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  ftm_non_asap_timer_set_cnt=%u ftm_response_frame_initial_cnt=%u\n",
		      s.ftm_non_asap_timer_set_cnt, s.ftm_response_frame_initial_cnt);
	shell_fprintf(sh, SHELL_INFO, "  ftm_response_frame_decode_error_cnt=%u\n",
		      s.ftm_response_frame_decode_error_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  ftm_response_frame_followup_cnt=%u ftm_response_frame_duplicate_cnt=%u\n",
		      s.ftm_response_frame_followup_cnt, s.ftm_response_frame_duplicate_cnt);
	shell_fprintf(sh, SHELL_INFO,
		      "  ftm_dialog_closed_cnt=%u scan_timer_in_ftmi=%u scan_timer_in_ftmr=%u\n",
		      s.ftm_dialog_closed_cnt, s.scan_timer_in_ftmi, s.scan_timer_in_ftmr);
	shell_fprintf(sh, SHELL_INFO,
		      "  scan_timer_in_scan=%u cordic_overflow=%u car2pol_overflow=%u\n",
		      s.scan_timer_in_scan, s.cordic_overflow, s.car2pol_overflow);
	shell_fprintf(sh, SHELL_INFO, "  pol2car_overflow=%u denom_zero=%u responder_enable=%u\n",
		      s.pol2car_overflow, s.denom_zero, s.responder_enable);
	shell_fprintf(sh, SHELL_INFO,
		      "  ftm_inv_req_cnt=%u lci_buffer_overflow=%u civic_buffer_overflow=%u\n",
		      s.ftm_inv_req_cnt, s.lci_buffer_overflow, s.civic_buffer_overflow);
}

static void dump_lp_rx_stats(const struct shell *sh, const uint8_t *p)
{
	struct lp_rx_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO,
		      "  scan_rf_mode_lp_rx=%u scan_rf_mode_hptrx=%u rf_mode_lp_rx=%u\n",
		      s.scan_rf_mode_lp_rx, s.scan_rf_mode_hptrx, s.rf_mode_lp_rx);
	shell_fprintf(sh, SHELL_INFO, "  rf_mode_hptrx=%u bet_isr=%u bet_bcn_abort=%u\n",
		      s.rf_mode_hptrx, s.bet_isr, s.bet_bcn_abort);
	shell_fprintf(sh, SHELL_INFO, "  bet_bcn_rf_switch=%u rf_mode_switch=%u\n",
		      s.bet_bcn_rf_switch, s.rf_mode_switch);
}

static void dump_lmac_sqi_stats(const struct shell *sh, const uint8_t *p)
{
	struct lmac_sqi_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO,
		      "  sqi_threshold_cmds_cnt=%u sqi_config_cmds_cnt=%u sqi_events_cnt=%u\n",
		      s.sqi_threshold_cmds_cnt, s.sqi_config_cmds_cnt, s.sqi_events_cnt);
}

static void dump_lmac_offload_raw_tx_stats(const struct shell *sh, const uint8_t *p)
{
	struct lmac_offload_raw_tx_stats s;

	memcpy(&s, p, sizeof(s));
	shell_fprintf(sh, SHELL_INFO, "  offLoad_raw_tx_state=%u offload_raw_tx_cnt=%u\n",
		      s.offLoad_raw_tx_state, s.offload_raw_tx_cnt);
	shell_fprintf(sh, SHELL_INFO, "  offload_raw_tx_complete_cnt=%u warm_boot_cnt=%u\n",
		      s.offload_raw_tx_complete_cnt, s.warm_boot_cnt);
}

static void dump_words(const struct shell *sh, const char *name, const uint8_t *p,
		       unsigned int num_words)
{
	unsigned int v;

	for (unsigned int i = 0; i < num_words; i++) {
		memcpy(&v, p + i * sizeof(v), sizeof(v));
		shell_fprintf(sh, SHELL_INFO, "  %s[%u]=0x%08x\n", name, i, v);
	}
}

static void dump_edca_config(const struct shell *sh, const uint8_t *p)
{
	dump_words(sh, "edca_config", p, 96);
}

static void dump_crypto_config(const struct shell *sh, const uint8_t *p)
{
	dump_words(sh, "crypto_config", p, 96);
}

static void dump_agg_config(const struct shell *sh, const uint8_t *p)
{
	dump_words(sh, "agg_config", p, 96);
}

static void dump_deagg_config(const struct shell *sh, const uint8_t *p)
{
	dump_words(sh, "deagg_config", p, 96);
}

static void dump_mac_ctrl_config(const struct shell *sh, const uint8_t *p)
{
	dump_words(sh, "mac_ctrl_config", p, 96);
}

static void dump_phy_stats(const struct shell *sh, const uint8_t *p)
{
	unsigned int phy_stats[ARRAY_SIZE(((struct phy_debug_stats *)0)->phy_stats)];

	memcpy(phy_stats, p, sizeof(phy_stats));
	for (unsigned int i = 0; i < ARRAY_SIZE(phy_stats); i++) {
		shell_fprintf(sh, SHELL_INFO, "  phy_stats[%u]=%u\n", i, phy_stats[i]);
	}
}

struct dbg_category {
	const char *name;
	unsigned int bit;
	void (*dump)(const struct shell *sh, const uint8_t *p);
};

static const struct dbg_category umac_cats[] = {
	{ "cmd_event", UMAC_CMD_EVENT_DEBUG_PARAMS, dump_umac_cmd_evnt_dbg_params },
	{ "tx", UMAC_TX_DEBUG_PARAMS, dump_umac_tx_dbg_params },
	{ "rx", UMAC_RX_DEBUG_PARAMS, dump_umac_rx_dbg_params },
	{ "sleep", UMAC_SLEEP_DEBUG_PARAMS, dump_umac_sleep_stats },
	{ "interface", UMAC_INTERFACE_DEBUG_PARAMS, dump_nrf_wifi_interface_stats },
	{ "raw", UMAC_RAWTXRX_DEBUG_PARAMS, dump_umac_raw_stats },
	{ "misc", UMAC_MISC_DEBUG_PARAMS, dump_nrf_wifi_misc_stats },
	{ "scan", UMAC_SCAN_DEBUG_PARAMS, dump_umac_scan_dbg_params },
};

static const struct dbg_category lmac_cats[] = {
	{ "common", LMAC_STATS_INIT_DEBUG_PARAMS, dump_lmac_common_stats },
	{ "phy_if", PHY_IF_DEBUG_STATS, dump_phy_if_stats },
	{ "tx", TX_DEBUG_PARAMS, dump_lmac_tx_stats },
	{ "rx", RX_DEBUG_PARAMS, dump_lmac_rx_stats },
	{ "scan", SCAN_DEBUG_PARAMS, dump_lmac_scan_stats },
	{ "sleep", SLEEP_DEBUG_PARAMS, dump_lmac_sleep_stats },
	{ "wake_sleep", WAKE_SLEEP_STATS, dump_lmac_sleep_timing_stats },
	{ "twt", TWT_DEBUG_PARAMS, dump_lmac_twt_stats },
	{ "he", LMAC_HE_DEBUGPARAMS, dump_lmac_he_stats },
	{ "ftm", FTM_DEBUG_STATS, dump_ftm_debug_stats },
	{ "lp_rx", LP_RX_STATS, dump_lp_rx_stats },
	{ "sqi", LMAC_SQI_STATS, dump_lmac_sqi_stats },
	{ "edca", EDCA_CONFIG, dump_edca_config },
	{ "crypto", CRYPTO_CONFIG, dump_crypto_config },
	{ "agg", AGG_CONFIG, dump_agg_config },
	{ "deagg", DEAGG_CONFIG, dump_deagg_config },
	{ "mac_ctrl", MAC_CTRL_CONFIG, dump_mac_ctrl_config },
	{ "offload_raw_tx", OFFLOAD_RAW_TX_STATS, dump_lmac_offload_raw_tx_stats },
};

static const struct dbg_category phy_cats[] = {
	{ "rx_dbg", PHY_RX_DEBUG_STATS, dump_phy_stats },
	{ "sw_dbg", PHY_SW_DBG_STATS, dump_phy_stats },
	{ "rssi_hist", PHY_RSSI_HIST_STATS, dump_phy_stats },
	{ "dc_rssi_snr", PHY_DC_RSSI_SNR_STATS, dump_phy_stats },
};

static int debug_stats_get_one(const struct shell *sh, enum rpu_stats_type stats_type,
			       const char *type_name, const struct dbg_category *cat)
{
	enum nrf_wifi_status status;
	struct nrf_wifi_rpu_debug_stats stats;
	unsigned int category;

	k_mutex_lock(&dbg_ctx->rpu_lock, K_FOREVER);
	if (!dbg_ctx->rpu_ctx) {
		k_mutex_unlock(&dbg_ctx->rpu_lock);
		shell_fprintf(sh, SHELL_ERROR, "RPU context not initialized\n");
		return -ENOEXEC;
	}

	memset(&stats, 0, sizeof(stats));
	status = nrf_wifi_sys_fmac_debug_stats_get(dbg_ctx->rpu_ctx, stats_type, cat->bit,
						   &stats);
	k_mutex_unlock(&dbg_ctx->rpu_lock);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh, SHELL_ERROR,
			      "Failed to get %s %s debug stats (timeout or error)\n",
			      type_name, cat->name);
		return -ENOEXEC;
	}

	/* stats_category is the first member of the UMAC, LMAC and PHY unions */
	memcpy(&category, &stats, sizeof(category));
	shell_fprintf(sh, SHELL_INFO, "%s debug stats %s (category=0x%x):\n",
		      type_name, cat->name, category);
	if (category != cat->bit) {
		shell_fprintf(sh, SHELL_WARNING, "  Requested category 0x%x, not decoded\n",
			      cat->bit);
		return -ENOEXEC;
	}

	cat->dump(sh, (const uint8_t *)&stats + sizeof(category));
	return 0;
}

static int debug_stats_get_type(const struct shell *sh, enum rpu_stats_type stats_type,
				const char *type_name, const struct dbg_category *cats,
				size_t num_cats, const char *cat_name)
{
	int ret = 0;

	for (size_t i = 0; i < num_cats; i++) {
		if (cat_name && strcmp(cat_name, cats[i].name)) {
			continue;
		}
		if (debug_stats_get_one(sh, stats_type, type_name, &cats[i])) {
			ret = -ENOEXEC;
		}
		if (cat_name) {
			return ret;
		}
	}

	if (cat_name) {
		shell_fprintf(sh, SHELL_ERROR, "Invalid %s category %s. Valid:", type_name,
			      cat_name);
		for (size_t i = 0; i < num_cats; i++) {
			shell_fprintf(sh, SHELL_ERROR, " %s", cats[i].name);
		}
		shell_fprintf(sh, SHELL_ERROR, "\n");
		return -ENOEXEC;
	}

	return ret;
}

static int nrf_wifi_dbg_debug_stats(const struct shell *sh,
				     size_t argc,
				     const char *argv[])
{
	const char *type = (argc > 1) ? argv[1] : "all";
	const char *cat_name = (argc > 2) ? argv[2] : NULL;
	bool all = !strcmp(type, "all");
	int ret = 0;

	if (all && cat_name) {
		shell_fprintf(sh, SHELL_ERROR, "A category needs type umac, lmac or phy\n");
		return -ENOEXEC;
	}

	if (!all && strcmp(type, "umac") && strcmp(type, "lmac") && strcmp(type, "phy")) {
		shell_fprintf(sh, SHELL_ERROR, "Invalid type %s (umac|lmac|phy|all)\n", type);
		return -ENOEXEC;
	}

	if (all || !strcmp(type, "umac")) {
		ret |= debug_stats_get_type(sh, RPU_STATS_TYPE_UMAC, "UMAC", umac_cats,
					    ARRAY_SIZE(umac_cats), cat_name);
	}
	if (all || !strcmp(type, "lmac")) {
		ret |= debug_stats_get_type(sh, RPU_STATS_TYPE_LMAC, "LMAC", lmac_cats,
					    ARRAY_SIZE(lmac_cats), cat_name);
	}
	if (all || !strcmp(type, "phy")) {
		ret |= debug_stats_get_type(sh, RPU_STATS_TYPE_PHY, "PHY", phy_cats,
					    ARRAY_SIZE(phy_cats), cat_name);
	}

	return ret ? -ENOEXEC : 0;
}

static int nrf_wifi_dbg_umac_int_stats(const struct shell *sh,
					size_t argc,
					const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct umac_int_stats stats;
	int ret = 0;
	unsigned int i;

	(void)argc;
	(void)argv;

	k_mutex_lock(&dbg_ctx->rpu_lock, K_FOREVER);
	if (!dbg_ctx->rpu_ctx) {
		shell_fprintf(sh, SHELL_ERROR, "RPU context not initialized\n");
		ret = -ENOEXEC;
		goto unlock_umac;
	}
	fmac_dev_ctx = dbg_ctx->rpu_ctx;

	memset(&stats, 0, sizeof(stats));
	status = nrf_wifi_sys_fmac_umac_int_stats_get(fmac_dev_ctx, &stats);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh, SHELL_ERROR,
			      "Failed to get UMAC int stats (timeout or error)\n");
		ret = -ENOEXEC;
		goto unlock_umac;
	}

	shell_fprintf(sh, SHELL_INFO, "UMAC internal (memory) stats received\n");
	shell_fprintf(sh, SHELL_INFO, "Scratch dynamic memory pools (first 8):\n");
	for (i = 0; i < 8; i++) {
		shell_fprintf(sh, SHELL_INFO, "  [%u] buffer_size=%u num_pool_items=%u\n",
			      i,
			      stats.scratch_dynamic_memory_info[i].buffer_size,
			      stats.scratch_dynamic_memory_info[i].num_pool_items);
	}
	shell_fprintf(sh, SHELL_INFO, "Retention dynamic memory pools (first 8):\n");
	for (i = 0; i < 8; i++) {
		shell_fprintf(sh, SHELL_INFO, "  [%u] buffer_size=%u num_pool_items=%u\n",
			      i,
			      stats.retention_dynamic_memory_info[i].buffer_size,
			      stats.retention_dynamic_memory_info[i].num_pool_items);
	}
unlock_umac:
	k_mutex_unlock(&dbg_ctx->rpu_lock);
	return ret;
}

#ifdef CONFIG_NRF71_STA_MODE
static int nrf_wifi_dbg_tx_stats(const struct shell *sh,
				  size_t argc,
				  const char *argv[])
{
	int vif_index = -1;
	int peer_index = 0;
	int max_vif_index = MAX(MAX_NUM_APS, MAX_NUM_STAS);
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	unsigned int tx_pending_pkts = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	int ret;

	vif_index = atoi(argv[1]);
	if ((vif_index < 0) || (vif_index >= max_vif_index)) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid vif index(%d).\n",
			      vif_index);
		shell_help(sh);
		return -ENOEXEC;
	}

	k_mutex_lock(&dbg_ctx->rpu_lock, K_FOREVER);
	if (!dbg_ctx->rpu_ctx) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "RPU context not initialized\n");
		ret = -ENOEXEC;
		goto unlock;
	}

	fmac_dev_ctx = dbg_ctx->rpu_ctx;
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	/* TODO: Get peer_index from shell once AP mode is supported */
	shell_fprintf(sh,
		SHELL_INFO,
		"************* Tx Stats: vif(%d) peer(0) ***********\n",
		vif_index);

	for (int i = 0; i < NRF_WIFI_FMAC_AC_MAX ; i++) {
		tx_pending_pkts = (unsigned int)sys_dlist_len(
			&sys_dev_ctx->tx_config.pend_pkt_q[peer_index][i]);

		shell_fprintf(
			sh,
			SHELL_INFO,
			"Outstanding tokens: ac: %d -> %d (pending_q_len: %d)\n",
			i,
			sys_dev_ctx->tx_config.outstanding_descs[i],
			tx_pending_pkts);
	}

	ret = 0;

unlock:
	k_mutex_unlock(&dbg_ctx->rpu_lock);
	return ret;
}
#endif /* CONFIG_NRF71_STA_MODE */

static int nrf_wifi_dbg_heap(const struct shell *sh, size_t argc, char **argv)
{
	struct k_heap *ctrl_pool;
	struct k_heap *data_pool;
	struct sys_memory_stats stats;
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	nrf_wifi_mem_get_heaps(&ctrl_pool, &data_pool);

	err = sys_heap_runtime_stats_get(&ctrl_pool->heap, &stats);
	if (err) {
		shell_error(sh, "Failed to read control heap statistics (err %d)", err);
		return -ENOEXEC;
	}
	shell_print(sh, "Control pool:");
	shell_print(sh, "  free:           %zu", stats.free_bytes);
	shell_print(sh, "  allocated:      %zu", stats.allocated_bytes);
	shell_print(sh, "  max. allocated: %zu", stats.max_allocated_bytes);

	err = sys_heap_runtime_stats_get(&data_pool->heap, &stats);
	if (err) {
		shell_error(sh, "Failed to read data heap statistics (err %d)", err);
		return -ENOEXEC;
	}
	shell_print(sh, "Data pool:");
	shell_print(sh, "  free:           %zu", stats.free_bytes);
	shell_print(sh, "  allocated:      %zu", stats.allocated_bytes);
	shell_print(sh, "  max. allocated: %zu", stats.max_allocated_bytes);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	nrf71_dbg,
	SHELL_CMD_ARG(rpu_stats,
		      NULL,
		      "Display RPU stats "
		      "Parameters: umac or lmac or phy or all (default)",
		      nrf_wifi_dbg_dump_rpu_stats,
		      1,
		      1),
	SHELL_CMD_ARG(clear_rpu_stats,
		      NULL,
		      "Clear RPU stats. Parameters: umac|lmac|phy|all (default)",
		      nrf_wifi_dbg_clear_rpu_stats,
		      1,
		      1),
	SHELL_CMD_ARG(debug_stats,
		      NULL,
		      "Request debug stats from RPU.\n"
		      "Usage: debug_stats [umac|lmac|phy|all (default)] [category]\n"
		      "Without a category, all categories of the type are shown.\n"
		      "umac: cmd_event tx rx sleep interface raw misc scan\n"
		      "lmac: common phy_if tx rx scan sleep wake_sleep twt he ftm\n"
		      "      lp_rx sqi edca crypto agg deagg mac_ctrl offload_raw_tx\n"
		      "phy: rx_dbg sw_dbg rssi_hist dc_rssi_snr",
		      nrf_wifi_dbg_debug_stats,
		      1,
		      2),
	SHELL_CMD_ARG(umac_int_stats,
		      NULL,
		      "Request UMAC internal (memory pool) stats from RPU",
		      nrf_wifi_dbg_umac_int_stats,
		      1,
		      0),
#ifdef CONFIG_NRF71_STA_MODE
	SHELL_CMD_ARG(tx_stats,
		      NULL,
		      "Displays transmit statistics\n"
			  "vif_index: 0 - 1\n",
		      nrf_wifi_dbg_tx_stats,
		      2,
		      0),
#endif /* CONFIG_NRF71_STA_MODE */
	SHELL_CMD_ARG(heap,
		      NULL,
		      "Control and data pool heap usage statistics",
		      nrf_wifi_dbg_heap,
		      1,
		      0),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((nrf71), dbg, &nrf71_dbg, "nRF71 debug commands\n", NULL, 0, 0);
