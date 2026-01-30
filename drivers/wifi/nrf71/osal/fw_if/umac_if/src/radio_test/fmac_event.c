/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing event specific definitions in the
 * Radio test mode for the FMAC IF Layer of the Wi-Fi driver.
 */

#include "queue.h"

#include <nrf71_wifi_ctrl.h>
#include "radio_test/fmac_structs.h"
#include "common/hal_mem.h"
#include "common/fmac_util.h"

static enum nrf_wifi_status umac_event_rt_stats_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
							void *event)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_rt_umac_event_stats *stats = NULL;

	if (!event) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	if (!fmac_dev_ctx->stats_req) {
		nrf_wifi_osal_log_err("%s: Stats recd when req was not sent!",
				      __func__);
		goto out;
	}

	stats = ((struct nrf_wifi_rt_umac_event_stats *)event);

	nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fw_stats,
			      &stats->fw,
			      sizeof(stats->fw));

	fmac_dev_ctx->stats_req = false;

	status = NRF_WIFI_STATUS_SUCCESS;

out:
	return status;
}


static enum nrf_wifi_status umac_event_rt_rf_test_process(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	void *event)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_event_rftest *rf_test_event = NULL;
	struct nrf_wifi_temperature_params rf_test_get_temperature;
	struct nrf_wifi_rf_get_rf_rssi rf_get_rf_rssi;
	struct nrf_wifi_rf_test_xo_calib xo_calib_params;
	struct nrf_wifi_rf_get_xo_value rf_get_xo_value_params;
	struct nrf_wifi_rt_fmac_dev_ctx *def_dev_ctx;
	struct nrf_wifi_rf_test_capture_params rf_test_capture_params;
	struct nrf_wifi_bat_volt_params bat_volt_params;
	unsigned int bat_volt;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!event) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	rf_test_event = ((struct nrf_wifi_event_rftest *)event);

	if (rf_test_event->rf_test_info.rfevent[0] != def_dev_ctx->rf_test_type) {
		nrf_wifi_osal_log_err("%s: Invalid event (%d) for RF test (%d)",
				      __func__,
				      rf_test_event->rf_test_info.rfevent[0],
				      def_dev_ctx->rf_test_type);
		goto out;
	}

	switch (rf_test_event->rf_test_info.rfevent[0]) {
	case NRF_WIFI_RF_TEST_EVENT_RX_ADC_CAP:
	case NRF_WIFI_RF_TEST_EVENT_RX_STAT_PKT_CAP:
	case NRF_WIFI_RF_TEST_EVENT_RX_DYN_PKT_CAP:
		status = hal_rpu_mem_read(fmac_dev_ctx->hal_dev_ctx,
					  def_dev_ctx->rf_test_cap_data,
					  RPU_MEM_RF_TEST_CAP_BASE,
					  def_dev_ctx->rf_test_cap_sz);

		nrf_wifi_osal_mem_cpy(&rf_test_capture_params,
				      (const unsigned char *)
					  &rf_test_event->rf_test_info.rfevent[0],
				      sizeof(rf_test_capture_params));

		def_dev_ctx->capture_status = rf_test_capture_params.capture_status;

		break;
	case NRF_WIFI_RF_TEST_EVENT_TX_TONE_START:
	case NRF_WIFI_RF_TEST_EVENT_DPD_ENABLE:
		break;

	case NRF_WIFI_RF_TEST_GET_TEMPERATURE:
		nrf_wifi_osal_mem_cpy(&rf_test_get_temperature,
				(const unsigned char *)&rf_test_event->rf_test_info.rfevent[0],
				sizeof(rf_test_get_temperature));

		if (rf_test_get_temperature.readTemperatureStatus) {
			nrf_wifi_osal_log_err("Temperature reading failed");
		} else {
			nrf_wifi_osal_log_info("Temperature = %d deg C",
						rf_test_get_temperature.temperature);
		}
		break;
	case NRF_WIFI_RF_TEST_EVENT_GET_BAT_VOLT:
		nrf_wifi_osal_mem_cpy(&bat_volt_params,
				      (const unsigned char *)
					  &rf_test_event->rf_test_info.rfevent[0],
				      sizeof(bat_volt_params));
		if (bat_volt_params.cmd_status) {
			nrf_wifi_osal_log_err("Battery Volatge reading failed");
		} else {
			bat_volt = (VBAT_OFFSET_MILLIVOLT +
			(VBAT_SCALING_FACTOR * bat_volt_params.voltage));

			nrf_wifi_osal_log_info("Battery voltage = %d mV",
						bat_volt);
		}
		break;
	case NRF_WIFI_RF_TEST_EVENT_RF_RSSI:
		nrf_wifi_osal_mem_cpy(&rf_get_rf_rssi,
				(const unsigned char *)&rf_test_event->rf_test_info.rfevent[0],
				sizeof(rf_get_rf_rssi));

		nrf_wifi_osal_log_info("RF RSSI value is = %d",
				       rf_get_rf_rssi.agc_status_val);
		break;
	case NRF_WIFI_RF_TEST_EVENT_XO_CALIB:
		nrf_wifi_osal_mem_cpy(&xo_calib_params,
				(const unsigned char *)&rf_test_event->rf_test_info.rfevent[0],
				sizeof(xo_calib_params));

		nrf_wifi_osal_log_info("XO value configured is = %d",
				       xo_calib_params.xo_val);
		break;
	case NRF_WIFI_RF_TEST_XO_TUNE:
		nrf_wifi_osal_mem_cpy(&rf_get_xo_value_params,
				(const unsigned char *)&rf_test_event->rf_test_info.rfevent[0],
				sizeof(rf_get_xo_value_params));

		nrf_wifi_osal_log_info("Best XO value is = %d",
				       rf_get_xo_value_params.xo_value);
		break;
	default:
		break;
	}

	def_dev_ctx->rf_test_type = NRF_WIFI_RF_TEST_MAX;
	status = NRF_WIFI_STATUS_SUCCESS;

out:
	return status;
}


static enum nrf_wifi_status umac_event_rt_proc_events(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						      struct host_rpu_msg *rpu_msg)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char *sys_head = NULL;

	struct nrf_wifi_rt_fmac_dev_ctx *def_dev_ctx_rt;
	struct nrf_wifi_umac_event_err_status *umac_status;

	if (!fmac_dev_ctx || !rpu_msg) {
		return status;
	}


	def_dev_ctx_rt = wifi_dev_priv(fmac_dev_ctx);
	sys_head = (unsigned char *)rpu_msg->msg;

	switch (((struct nrf_wifi_sys_head *)sys_head)->cmd_event) {
	case NRF_WIFI_EVENT_STATS:
		status = umac_event_rt_stats_process(fmac_dev_ctx,
						     sys_head);
		break;
	case NRF_WIFI_EVENT_INIT_DONE:
		fmac_dev_ctx->fw_init_done = 1;
		status = NRF_WIFI_STATUS_SUCCESS;
		break;
	case NRF_WIFI_EVENT_DEINIT_DONE:
		fmac_dev_ctx->fw_deinit_done = 1;
		status = NRF_WIFI_STATUS_SUCCESS;
		break;
	case NRF_WIFI_EVENT_RF_TEST:
		status = umac_event_rt_rf_test_process(fmac_dev_ctx,
						    sys_head);
		break;
	case NRF_WIFI_EVENT_RADIOCMD_STATUS:
		umac_status = ((struct nrf_wifi_umac_event_err_status *)sys_head);
		def_dev_ctx_rt->radio_cmd_status = umac_status->status;
		def_dev_ctx_rt->radio_cmd_done = true;
		status = NRF_WIFI_STATUS_SUCCESS;
		break;
	default:
		nrf_wifi_osal_log_err("%s: Unknown event recd: %d",
				      __func__,
				      ((struct nrf_wifi_sys_head *)sys_head)->cmd_event);
		break;
	}
	return status;
}


static enum nrf_wifi_status umac_event_ctrl_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						    void *event_data,
						    unsigned int event_len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_SUCCESS;
	struct nrf_wifi_umac_hdr *umac_hdr = NULL;
	struct nrf_wifi_reg *get_reg_event = NULL;
	struct nrf_wifi_event_regulatory_change *reg_change_event = NULL;
	unsigned char if_id = 0;
	unsigned int event_num = 0;

	if (!fmac_dev_ctx || !event_data) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	umac_hdr = event_data;
	if_id = umac_hdr->ids.wdev_id;
	event_num = umac_hdr->cmd_evnt;

	if (if_id >= MAX_NUM_VIFS) {
		nrf_wifi_osal_log_err("%s: Invalid wdev_id recd from UMAC %d",
				      __func__,
				      if_id);

		goto out;
	}

#ifdef NRF_WIFI_CMD_EVENT_LOG
	nrf_wifi_osal_log_info("%s: Event %d received from UMAC",
			      __func__,
			      event_num);
#else
	nrf_wifi_osal_log_dbg("%s: Event %d received from UMAC",
			      __func__,
			      event_num);
#endif /* NRF_WIFI_CMD_EVENT_LOG */

	switch (umac_hdr->cmd_evnt) {
	case NRF_WIFI_UMAC_EVENT_GET_REG:
		get_reg_event = (struct nrf_wifi_reg *)event_data;

		nrf_wifi_osal_mem_cpy(&fmac_dev_ctx->alpha2,
				      &get_reg_event->nrf_wifi_alpha2,
				      sizeof(get_reg_event->nrf_wifi_alpha2));

		fmac_dev_ctx->reg_chan_count = get_reg_event->num_channels;

		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->reg_chan_info,
				      &get_reg_event->chn_info,
				      fmac_dev_ctx->reg_chan_count *
				      sizeof(struct nrf_wifi_get_reg_chn_info));

		fmac_dev_ctx->alpha2_valid = true;
		break;
	case NRF_WIFI_UMAC_EVENT_REG_CHANGE:
		reg_change_event = (struct nrf_wifi_event_regulatory_change *)event_data;

		fmac_dev_ctx->reg_change = nrf_wifi_osal_mem_zalloc(sizeof(*reg_change_event));

		if (!fmac_dev_ctx->reg_change) {
			nrf_wifi_osal_log_err("%s: Failed to allocate memory for reg_change",
					      __func__);
			goto out;
		}

		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->reg_change,
				      reg_change_event,
				      sizeof(*reg_change_event));
		fmac_dev_ctx->reg_set_status = true;
		break;
	default:
		nrf_wifi_osal_log_dbg("%s: No callback registered for event %d",
				      __func__,
				      umac_hdr->cmd_evnt);
		break;
	}

	nrf_wifi_osal_log_dbg("%s: Event %d processed",
			      __func__,
			      event_num);

out:
	return status;
}


enum nrf_wifi_status nrf_wifi_rt_fmac_event_callback(void *mac_dev_ctx,
						     void *rpu_event_data,
						     unsigned int rpu_event_len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct host_rpu_msg *rpu_msg = NULL;
	struct nrf_wifi_umac_hdr *umac_hdr = NULL;
	unsigned int umac_msg_len = 0;
	int umac_msg_type = NRF_WIFI_UMAC_EVENT_UNSPECIFIED;

	fmac_dev_ctx = (struct nrf_wifi_fmac_dev_ctx *)mac_dev_ctx;

	rpu_msg = (struct host_rpu_msg *)rpu_event_data;
	umac_hdr = (struct nrf_wifi_umac_hdr *)rpu_msg->msg;
	umac_msg_len = rpu_msg->hdr.len;
	umac_msg_type = umac_hdr->cmd_evnt;

#ifdef NRF_WIFI_CMD_EVENT_LOG
	nrf_wifi_osal_log_info("%s: Event type %d recd",
			      __func__,
			      rpu_msg->type);
#else
	nrf_wifi_osal_log_dbg("%s: Event type %d recd",
			      __func__,
			      rpu_msg->type);
#endif /* NRF_WIFI_CMD_EVENT_LOG */

	switch (rpu_msg->type) {
	case NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC:
		status = umac_event_ctrl_process(fmac_dev_ctx,
						 rpu_msg->msg,
						 rpu_msg->hdr.len);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: umac_event_ctrl_process failed",
					      __func__);
			goto out;
		}
		break;
	case NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM:
		status = umac_event_rt_proc_events(fmac_dev_ctx,
						   rpu_msg);
		break;
	default:
		goto out;
	}

out:
	return status;
}
