/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing command specific declarations for the
 * offloaded raw TX mode in the FMAC IF Layer of the Wi-Fi driver.
 */

#include "offload_raw_tx/fmac_cmd.h"
#include "common/hal_api_common.h"

enum nrf_wifi_status umac_cmd_off_raw_tx_init(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct nrf_wifi_phy_rf_params *rf_params,
	bool rf_params_valid,
#ifdef NRF_WIFI_LOW_POWER
	int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
					      unsigned int phy_calib,
					      unsigned char op_band,
					      bool beamforming,
	struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
	struct nrf_wifi_board_params *board_params,
	unsigned char *country_code)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_sys_init *umac_cmd_data = NULL;
	unsigned int len = 0;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_sys_init *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_INIT;
	umac_cmd_data->sys_head.len = len;


	umac_cmd_data->sys_params.rf_params_valid = rf_params_valid;

	if (rf_params_valid) {
		nrf_wifi_osal_mem_cpy(umac_cmd_data->sys_params.rf_params,
				      rf_params,
				      NRF_WIFI_RF_PARAMS_SIZE);
	}


	umac_cmd_data->sys_params.phy_calib = phy_calib;
	umac_cmd_data->sys_params.hw_bringup_time = HW_DELAY;
	umac_cmd_data->sys_params.sw_bringup_time = SW_DELAY;
	umac_cmd_data->sys_params.bcn_time_out = BCN_TIMEOUT;
	umac_cmd_data->sys_params.calib_sleep_clk = CALIB_SLEEP_CLOCK_ENABLE;
#ifdef NRF_WIFI_LOW_POWER
	umac_cmd_data->sys_params.sleep_enable = sleep_type;
#endif /* NRF_WIFI_LOW_POWER */
	umac_cmd_data->discon_timeout = NRF_WIFI_AP_DEAD_DETECT_TIMEOUT;
#ifdef NRF_WIFI_RPU_RECOVERY
	umac_cmd_data->watchdog_timer_val =
		(NRF_WIFI_RPU_RECOVERY_PS_ACTIVE_TIMEOUT_MS) / 1000;
#else
	/* Disable watchdog */
	umac_cmd_data->watchdog_timer_val = 0xFFFFFF;
#endif /* NRF_WIFI_RPU_RECOVERY */

	nrf_wifi_osal_log_dbg("RPU LPM type: %s",
		umac_cmd_data->sys_params.sleep_enable == 2 ? "HW" :
		umac_cmd_data->sys_params.sleep_enable == 1 ? "SW" : "DISABLED");

#ifdef NRF_WIFI_MGMT_BUFF_OFFLOAD
	umac_cmd_data->mgmt_buff_offload =  1;
	nrf_wifi_osal_log_dbg("Management buffer offload enabled");
#endif /* NRF_WIFI_MGMT_BUFF_OFFLOAD */
#ifdef NRF_WIFI_FEAT_KEEPALIVE
	umac_cmd_data->keep_alive_enable = KEEP_ALIVE_ENABLED;
	umac_cmd_data->keep_alive_period = NRF_WIFI_KEEPALIVE_PERIOD_S;
	nrf_wifi_osal_log_dbg("Keepalive enabled with period %d",
				   umac_cmd_data->keep_alive_enable);
#endif /* NRF_WIFI_FEAT_KEEPALIVE */

	umac_cmd_data->op_band = op_band;

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->sys_params.rf_params[PCB_LOSS_BYTE_2G_OFST],
			      &board_params->pcb_loss_2g,
			      NUM_PCB_LOSS_OFFSET);

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->sys_params.rf_params[ANT_GAIN_2G_OFST],
			      &tx_pwr_ctrl_params->ant_gain_2g,
			      NUM_ANT_GAIN);

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->sys_params.rf_params[BAND_2G_LW_ED_BKF_DSSS_OFST],
			      &tx_pwr_ctrl_params->band_edge_2g_lo_dss,
			      NUM_EDGE_BACKOFF);

	nrf_wifi_osal_mem_cpy(umac_cmd_data->country_code,
			      country_code,
			      NRF_WIFI_COUNTRY_CODE_LEN);

#ifdef NRF71_RPU_EXTEND_TWT_SP
	 umac_cmd_data->feature_flags |= TWT_EXTEND_SP_EDCA;
#endif
#ifdef CONFIG_WIFI_NRF71_SCAN_DISABLE_DFS_CHANNELS
	umac_cmd_data->feature_flags |= DISABLE_DFS_CHANNELS;
#endif /* NRF71_SCAN_DISABLE_DFS_CHANNELS */

	if (!beamforming) {
		umac_cmd_data->disable_beamforming = 1;
	}

#if defined(NRF_WIFI_PS_INT_PS)
	umac_cmd_data->ps_exit_strategy = INT_PS;
#else
	umac_cmd_data->ps_exit_strategy = EVERY_TIM;
#endif  /* NRF_WIFI_PS_INT_PS */

	umac_cmd_data->display_scan_bss_limit = NRF_WIFI_DISPLAY_SCAN_BSS_LIMIT;

#ifdef NRF_WIFI_COEX_DISABLE_PRIORITY_WINDOW_FOR_SCAN
	umac_cmd_data->coex_disable_ptiwin_for_wifi_scan = 1;
#else
	umac_cmd_data->coex_disable_ptiwin_for_wifi_scan = 0;
#endif /* NRF_WIFI_COEX_DISABLE_PRIORITY_WINDOW_FOR_SCAN */

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}

enum nrf_wifi_status umac_cmd_off_raw_tx_prog_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_get_stats *umac_cmd_data = NULL;
	int len = 0;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_get_stats *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_GET_STATS;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->stats_type = RPU_STATS_TYPE_OFFLOADED_RAW_TX;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}


enum nrf_wifi_status umac_cmd_off_raw_tx_conf(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      struct nrf_wifi_offload_ctrl_params *off_ctrl_params,
					      struct nrf_wifi_offload_tx_ctrl *offload_tx_params)
{
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_offload_raw_tx_params *umac_cmd_data = NULL;
	int len = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!fmac_dev_ctx->fw_init_done) {
		nrf_wifi_osal_log_err("%s: UMAC buff config not yet done", __func__);
		goto out;
	}

	if (!off_ctrl_params) {
		nrf_wifi_osal_log_err("%s: offloaded raw tx control params is NULL", __func__);
		goto out;
	}

	if (!offload_tx_params) {
		nrf_wifi_osal_log_err("%s: offload raw tx params is NULL", __func__);
		goto out;
	}

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed", __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_offload_raw_tx_params *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_OFFLOAD_RAW_TX_PARAMS;
	umac_cmd_data->sys_head.len = len;

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->ctrl_info,
			      off_ctrl_params,
			      sizeof(*off_ctrl_params));

	nrf_wifi_osal_mem_cpy(&umac_cmd_data->tx_params,
			      offload_tx_params,
			      sizeof(*offload_tx_params));

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}

enum nrf_wifi_status umac_cmd_off_raw_tx_ctrl(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      unsigned char ctrl_type)
{
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_offload_raw_tx_ctrl *umac_cmd_data = NULL;
	int len = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!fmac_dev_ctx->fw_init_done) {
		nrf_wifi_osal_log_err("%s: UMAC buff config not yet done", __func__);
		goto out;
	}

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		nrf_wifi_osal_log_err("%s: umac_cmd_alloc failed", __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_offload_raw_tx_ctrl *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_OFFLOAD_RAW_TX_CTRL;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->ctrl_type = ctrl_type;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}
