/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing command specific definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include <common/mem_mgmt.h>
#include <common/wifi_ipc.h>

#include <common/fw_if/nrf71_wifi_ctrl.h>
#include <common/fmac_structs_common.h>
#include <common/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

struct host_rpu_msg *umac_cmd_alloc(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				    int type,
				    int len)
{
	struct host_rpu_msg *umac_cmd = NULL;

	umac_cmd = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*umac_cmd) + len);

	if (!umac_cmd) {
		LOG_ERR("%s: Failed to allocate UMAC cmd",
				      __func__);
		goto out;
	}

	umac_cmd->type = type;
	umac_cmd->hdr.len = sizeof(*umac_cmd) + len;

out:
	return umac_cmd;
}


enum nrf_wifi_status umac_cmd_cfg(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				  void *params,
				  int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;

	if (!fmac_dev_ctx->fw_init_done) {
		struct nrf_wifi_umac_hdr *umac_hdr = NULL;

		umac_hdr = (struct nrf_wifi_umac_hdr *)params;
		LOG_ERR("%s: UMAC buff config not yet done(%d)",
				      __func__,
				      umac_hdr->cmd_evnt);
		goto out;
	}

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC,
				  len);

	if (!umac_cmd) {
		LOG_ERR("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	nrf_wifi_mem_cpy(umac_cmd->msg,
			      params,
			      len);

	status = nrf_wifi_ipc_cmd_send(fmac_dev_ctx,
				       umac_cmd,
				       (sizeof(*umac_cmd) + len));

	LOG_DBG("%s: Command %d sent to RPU",
			      __func__,
			      ((struct nrf_wifi_umac_hdr *)params)->cmd_evnt);

out:
	return status;
}


enum nrf_wifi_status umac_cmd_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_sys_deinit *umac_cmd_data = NULL;
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
	umac_cmd_data = (struct nrf_wifi_cmd_sys_deinit *)(umac_cmd->msg);
	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_DEINIT;
	umac_cmd_data->sys_head.len = len;
	status = nrf_wifi_ipc_cmd_send(fmac_dev_ctx,
				       umac_cmd,
				       (sizeof(*umac_cmd) + len));
out:
	return status;
}

enum nrf_wifi_status umac_cmd_srcoex(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				     void *cmd,
				     unsigned int cmd_len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_coex_config *umac_cmd_data = NULL;
	int len = 0;

	len = sizeof(*umac_cmd_data)+cmd_len;

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	if (!umac_cmd) {
		LOG_ERR("%s: umac_cmd_alloc failed",
				      __func__);
		goto out;
	}

	umac_cmd_data = (struct nrf_wifi_cmd_coex_config *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_SRCOEX;
	umac_cmd_data->sys_head.len = len;
	umac_cmd_data->coex_config_info.len = cmd_len;

	nrf_wifi_mem_cpy(umac_cmd_data->coex_config_info.coex_cmd,
			      cmd,
			      cmd_len);

	status = nrf_wifi_ipc_cmd_send(fmac_dev_ctx,
				       umac_cmd,
				       (sizeof(*umac_cmd) + len));
out:
	return status;


}


enum nrf_wifi_status umac_cmd_prog_stats_reset(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	struct nrf_wifi_cmd_reset_stats *umac_cmd_data = NULL;
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

	umac_cmd_data = (struct nrf_wifi_cmd_reset_stats *)(umac_cmd->msg);

	umac_cmd_data->sys_head.cmd_event = NRF_WIFI_CMD_RESET_STATISTICS;
	umac_cmd_data->sys_head.len = len;

	status = nrf_wifi_ipc_cmd_send(fmac_dev_ctx,
				       umac_cmd,
				       (sizeof(*umac_cmd) + len));

out:
	return status;
}

enum nrf_wifi_status umac_cmd_lmac_tuning_params(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
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

	status = nrf_wifi_ipc_cmd_send(fmac_dev_ctx,
				       umac_cmd,
				       (sizeof(*umac_cmd) + len));

out:
	return status;
}
