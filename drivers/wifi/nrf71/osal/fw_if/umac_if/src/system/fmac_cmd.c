/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing command specific declarations for the
 * system mode in the FMAC IF Layer of the Wi-Fi driver.
 */

#include <common/mem_mgmt.h>
#include "system/fmac_structs.h"
#include "system/fmac_cmd.h"
#include <common/util.h>
#include "common/hal_api_common.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

enum nrf_wifi_status umac_cmd_sys_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				       unsigned int *rf_params_addr,
				       unsigned int vtf_buffer_start_address,
				       struct nrf_wifi_data_config_params *config,
#ifdef NRF_WIFI_LOW_POWER
				       int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
				       unsigned int phy_calib,
				       unsigned char op_band,
				       bool beamforming,
				       struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
				       unsigned char *country_code)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_sys_init *umac_cmd_data = NULL;
	unsigned int len = 0;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;

	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		LOG_ERR("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_sys_init *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_INIT;
	umac_cmd_data->sys_head.len = len;

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

	LOG_DBG("RPU LPM type: %s",
		umac_cmd_data->sys_params.sleep_enable == 2 ? "HW" :
		umac_cmd_data->sys_params.sleep_enable == 1 ? "SW" : "DISABLED");

#ifdef NRF_WIFI_MGMT_BUFF_OFFLOAD
	umac_cmd_data->mgmt_buff_offload =  1;
	LOG_DBG("Management buffer offload enabled");
#endif /* NRF_WIFI_MGMT_BUFF_OFFLOAD */
#ifdef NRF_WIFI_FEAT_KEEPALIVE
	umac_cmd_data->keep_alive_enable = KEEP_ALIVE_ENABLED;
	umac_cmd_data->keep_alive_period = NRF_WIFI_KEEPALIVE_PERIOD_S;
	LOG_DBG("Keepalive enabled with period %d",
				   umac_cmd_data->keep_alive_enable);
#endif /* NRF_WIFI_FEAT_KEEPALIVE */

	nrf_wifi_mem_cpy(umac_cmd_data->rx_buf_pools,
			      sys_fpriv->rx_buf_pools,
			      sizeof(umac_cmd_data->rx_buf_pools));

	nrf_wifi_mem_cpy(&umac_cmd_data->data_config_params,
			      config,
			      sizeof(umac_cmd_data->data_config_params));

	umac_cmd_data->temp_vbat_config_params.temp_based_calib_en = NRF_WIFI_TEMP_CALIB_ENABLE;
	umac_cmd_data->temp_vbat_config_params.temp_calib_bitmap = NRF_WIFI_DEF_PHY_TEMP_CALIB;
	umac_cmd_data->temp_vbat_config_params.vbat_calibp_bitmap = NRF_WIFI_DEF_PHY_VBAT_CALIB;
	umac_cmd_data->temp_vbat_config_params.temp_vbat_mon_period = NRF_WIFI_TEMP_CALIB_PERIOD;
	umac_cmd_data->temp_vbat_config_params.vth_low = NRF_WIFI_VBAT_LOW;
	umac_cmd_data->temp_vbat_config_params.vth_hi = NRF_WIFI_VBAT_HIGH;
	umac_cmd_data->temp_vbat_config_params.temp_threshold = NRF_WIFI_TEMP_CALIB_THRESHOLD;
	umac_cmd_data->temp_vbat_config_params.vth_very_low = NRF_WIFI_VBAT_VERYLOW;

	umac_cmd_data->op_band = op_band;

	umac_cmd_data->sys_params.vtf_buffer_addr = vtf_buffer_start_address;

	nrf_wifi_mem_cpy(&umac_cmd_data->sys_params.rf_params_addr,
			      rf_params_addr,
			      sizeof(umac_cmd_data->sys_params.rf_params_addr));

	nrf_wifi_mem_cpy(&umac_cmd_data->sys_params.tx_pwr_ctrl_params,
			      tx_pwr_ctrl_params,
			      sizeof(umac_cmd_data->sys_params.tx_pwr_ctrl_params));


	nrf_wifi_mem_cpy(umac_cmd_data->country_code,
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

#ifdef WIFI_MGMT_RAW_SCAN_RESULTS
	umac_cmd_data->raw_scan_enable = 1;
#else
	umac_cmd_data->raw_scan_enable = 0;
#endif /* WIFI_MGMT_RAW_SCAN_RESULTS */

	umac_cmd_data->max_ps_poll_fail_cnt = NRF_WIFI_MAX_PS_POLL_FAIL_CNT;

	#ifdef NRF_WIFI_RX_STBC_HT
		umac_cmd_data->stbc_enable_in_ht = 1;
	#endif /* NRF_WIFI_RX_STBC_HT */

	#ifdef NRF_WIFI_DYNAMIC_BANDWIDTH_SIGNALLING
		umac_cmd_data->dbs_war_ctrl = 1;
	#endif /* NRF_WIFI_DYNAMIC_BANDWIDTH_SIGNALLING */

	#ifdef NRF_WIFI_DYNAMIC_ED
		umac_cmd_data->dynamic_ed = 1;
	#endif /* NRF_WIFI_DYNAMIC_ED */

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}

enum nrf_wifi_status umac_cmd_sys_prog_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						  enum rpu_stats_type stats_type)
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
		LOG_ERR("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_get_stats *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_GET_STATS;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->stats_type = stats_type;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}

enum nrf_wifi_status umac_cmd_sys_clear_stats(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      enum rpu_stats_type stats_type)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_clear_stats *umac_cmd_data = NULL;
	int len = 0;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		LOG_ERR("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_clear_stats *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_CLEAR_STATS;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->stats_type = stats_type;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}

enum nrf_wifi_status umac_cmd_sys_debug_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						  enum rpu_stats_type stats_type)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_umac_cmd_debug_stats *umac_cmd_data = NULL;
	int len = 0;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		LOG_ERR("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_umac_cmd_debug_stats *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_DEBUG_STATS;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->stats_type = stats_type;
	umac_cmd_data->periodic_stats_enable = 0;
	umac_cmd_data->periodic_stats_interval = 0;
	umac_cmd_data->stats_ctrl = 0;
	umac_cmd_data->stats_addr = 0;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}

enum nrf_wifi_status umac_cmd_sys_umac_int_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_sys_head *sys_head = NULL;
	int len = 0;

	len = sizeof(*sys_head);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		LOG_ERR("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	sys_head = (struct nrf_wifi_sys_head *)(umac_cmd->msg);

	sys_head->cmd_event = NRF_WIFI_CMD_UMAC_INT_STATS;
	sys_head->len = len;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}

enum nrf_wifi_status umac_cmd_sys_he_ltf_gi(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					    unsigned char he_ltf,
					    unsigned char he_gi,
					    unsigned char enabled)
{
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_he_gi_ltf_config *umac_cmd_data = NULL;
	int len = 0;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		LOG_ERR("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_he_gi_ltf_config *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_HE_GI_LTF_CONFIG;
	umac_cmd_data->sys_head.len = len;

	if (enabled) {
		nrf_wifi_mem_cpy(&umac_cmd_data->he_ltf,
				      &he_ltf,
				      sizeof(he_ltf));
		nrf_wifi_mem_cpy(&umac_cmd_data->he_gi_type,
				      &he_gi,
				      sizeof(he_gi));
	}

	nrf_wifi_mem_cpy(&umac_cmd_data->enable,
			      &enabled,
			      sizeof(enabled));

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));
out:
	return status;
}

enum nrf_wifi_status umac_cmd_sys_lmac_tuning_params(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_lmac_tuning_params *umac_cmd_data = NULL;
	unsigned int len = 0;

	len = sizeof(*umac_cmd_data);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		LOG_ERR("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_lmac_tuning_params *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_LMAC_TUNING_PARAMS;
	umac_cmd_data->sys_head.len = len;

	nrf_wifi_mem_set(&umac_cmd_data->params, 0, sizeof(umac_cmd_data->params));

	/* Hardware latency compensation tuning parameters used to achieve the
	 * target SIFS timing. These values have been calibrated through lab
	 * measurements. Changing these values may result in protocol timing
	 * violations.
	 */
	umac_cmd_data->params.ofdm_sifs_value = NRF_WIFI_LMAC_OFDM_SIFS_VALUE;
	umac_cmd_data->params.dsss_sifs_value = NRF_WIFI_LMAC_DSSS_SIFS_VALUE;

	umac_cmd_data->params.cfg_bet_enable = 0;
#ifdef NRF_WIFI_LP_RX
	umac_cmd_data->params.cfg_bet_enable = 1;
	umac_cmd_data->params.lp_rx_enable = 1;
#else
	umac_cmd_data->params.lp_rx_enable = 0;
	umac_cmd_data->params.cfg_bet_enable = 0;
#endif /* NRF_WIFI_LP_RX */

	/* Internal tuning parameter for ACK timeout in the firmware. Accounts for
	 * internal hardware latencies and is calibrated in the lab. Changing
	 * this value may cause unexpected system behavior. Not expected to
	 * require modification under normal use.
	 */
	umac_cmd_data->params.ack_margin = NRF_WIFI_LMAC_ACK_MARGIN_VALUE;

	/* Data path watchdog timer in microseconds. The firmware monitors data
	 * path health and raises a warning event if a pending TX remains queued
	 * longer than this duration. Intended for debug use only; configure a
	 * significantly higher value to avoid false warnings. Not expected to
	 * require modification under normal use.
	 */
	umac_cmd_data->params.data_path_watch_dog_timer =
		NRF_WIFI_LMAC_DATA_PATH_WATCH_DOG_TIMER_VALUE;

	/* Broadcast wait period in microseconds. Duration the system stays awake
	 * after detecting a broadcast TIM bit in the beacon before entering
	 * sleep. If no broadcast frame is received within this time, the system
	 * assumes the frame was lost (for example due to channel congestion or
	 * collisions) and returns to sleep rather than waiting until the next
	 * beacon. A value around 20 ms is a reasonable default; a higher value
	 * reduces the risk of missing broadcast frames at the cost of longer
	 * wake time. Broadcast frames are typically retransmitted at higher
	 * protocol layers and prioritized by the AP after the beacon.
	 */
	umac_cmd_data->params.bcst_wait_period = NRF_WIFI_LMAC_BCST_WAIT_PERIOD_VALUE;

	/* Parameter controlling system sleep behavior after receiving a frame.
	 * After receiving a frame, the system remains active for the specified
	 * duration (in microseconds) before it is allowed to enter sleep.
	 *
	 * This parameter is not expected to require modification under normal use.
	 */
	umac_cmd_data->params.inactivity_timer_after_rx =
		NRF_WIFI_LMAC_INACTIVITY_TIMER_AFTER_RX_VALUE;

	/* Enable the firmware algorithm that corrects misbehaving AP TSF values.
	 * Enabled by default; this parameter exists to disable the feature if
	 * field issues are observed. Not expected to require modification under
	 * normal use.
	 */
	umac_cmd_data->params.tsf_correction_enable = 1;

	/* Maximum number of consecutive missed beacons before the system stays
	 * awake indefinitely to receive the next beacon. In DTIM power save, the
	 * system normally wakes for each beacon and may sleep after the beacon
	 * wait timeout if the beacon is not received. This cycle cannot repeat
	 * indefinitely; once the miss count exceeds this threshold, the system
	 * remains awake until a beacon is received. The ideal value is 1 and
	 * may be increased up to 3 to reduce data loss.
	 */
	umac_cmd_data->params.allowed_bcn_miss_before_wakeup =
		NRF_WIFI_LMAC_ALLOWED_BCN_MISS_BEFORE_WAKEUP_VALUE;

	/* Beacon wait time in microseconds. Duration the system waits for a
	 * beacon before attempting sleep. A value around 20 ms balances
	 * avoiding missed beacons on busy channels against returning to sleep
	 * promptly when a beacon is lost due to collisions, rather than waiting
	 * until the next beacon interval. A higher value may be used to wait
	 * for a beacon after every wakeup in DTIM power save.
	 */
	umac_cmd_data->params.beacon_wait_time = NRF_WIFI_LMAC_BEACON_WAIT_TIME_VALUE;

	/* Enable TX/RX checksum offloading in the LMAC. Per-packet behavior is
	 * still controlled by the UMAC based on Kconfig; this parameter enables
	 * local LMAC offloading support. Setting either field to 0 disables
	 * offloading even when enabled in Kconfig. Enabled by default and not
	 * expected to require modification under normal use.
	 */
	umac_cmd_data->params.offloadTXChecksum = 1;
	umac_cmd_data->params.offloadRXChecksum = 1;

	/* Parameter controlling system sleep behavior while transmitting a raw frame.
	 * After transmitting a raw frame, the system remains active for the
	 * specified duration (in microseconds) before it is allowed to enter sleep.
	 *
	 * This parameter is not expected to require modification under normal use.
	 */
	umac_cmd_data->params.raw_tx_inactivity_timer = NRF_WIFI_LMAC_RAW_TX_INACTIVITY_TIMER_VALUE;

	/* Parameter controlling system sleep behavior while attempting a connection.
	 * After transmitting a management frame, the system remains active for the
	 * specified duration (in microseconds) before it is allowed to enter sleep.
	 *
	 * During the pre-association phase, the firmware attempts to keep the system
	 * in a low-power state to reduce power consumption. However, entering sleep
	 * too aggressively while establishing a connection may lead to connection
	 * reliability issues. Therefore, a separate parameter is provided for the
	 * pre-connection phase with a more relaxed timeout.
	 *
	 * Once connected, the inactivity timer configured through the power-save
	 * command is used to track inactivity and control sleep behavior.
	 *
	 * This parameter is not expected to require modification under normal use.
	 */
	umac_cmd_data->params.connection_inactivity_timer =
		NRF_WIFI_LMAC_PRE_CONNECTION_INACTIVITY_TIMER_VALUE;

	/* Compensation offsets for FTM measurements. Currently set to zero. The
	 * required values will be updated after FTM characterization, if
	 * compensation is determined to be necessary.
	 */
	umac_cmd_data->params.ftm_delay = 0;
	umac_cmd_data->params.tod_offset = 0;
	umac_cmd_data->params.toa_offset = 0;
	umac_cmd_data->params.clock_mode = 0;

	status = nrf_wifi_hal_ctrl_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    umac_cmd,
					    (sizeof(*umac_cmd) + len));

out:
	return status;
}
