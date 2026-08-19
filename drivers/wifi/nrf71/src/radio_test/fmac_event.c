/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing event specific definitions in the
 * Radio test mode for the FMAC IF Layer of the Wi-Fi driver.
 */

#include <common/llist_mgmt.h>
#include <common/mem_mgmt.h>

#include <common/fw_if/nrf71_wifi_ctrl.h>
#include <radio_test/fmac_structs.h>
#include <common/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

static enum nrf_wifi_status umac_event_rt_stats_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
							void *event)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_rt_umac_event_stats *stats = NULL;

	if (!event) {
		LOG_ERR("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	if (!fmac_dev_ctx->stats_req) {
		LOG_ERR("%s: Stats recd when req was not sent!",
				      __func__);
		goto out;
	}

	stats = ((struct nrf_wifi_rt_umac_event_stats *)event);

	nrf_wifi_mem_cpy(fmac_dev_ctx->fw_stats,
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
	struct nrf_wifi_rf_get_xo_value rf_get_xo_value_params;
	struct nrf_wifi_rt_fmac_dev_ctx *def_dev_ctx;
	struct nrf_wifi_rf_test_capture_params rf_test_capture_params;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!event) {
		LOG_ERR("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	rf_test_event = ((struct nrf_wifi_event_rftest *)event);

	if (rf_test_event->rf_test_info.rfevent[0] != def_dev_ctx->rf_test_type) {
		LOG_ERR("%s: Invalid event (%d) for RF test (%d)",
				      __func__,
				      rf_test_event->rf_test_info.rfevent[0],
				      def_dev_ctx->rf_test_type);
		goto out;
	}

	switch (rf_test_event->rf_test_info.rfevent[0]) {
	case NRF_WIFI_RF_TEST_EVENT_RX_ADC_CAP:
	case NRF_WIFI_RF_TEST_EVENT_RX_STAT_PKT_CAP:
	case NRF_WIFI_RF_TEST_EVENT_RX_DYN_PKT_CAP:

		nrf_wifi_mem_cpy(&rf_test_capture_params,
				      (const unsigned char *)
					  &rf_test_event->rf_test_info.rfevent[0],
				      sizeof(rf_test_capture_params));

		def_dev_ctx->capture_status = rf_test_capture_params.capture_status;

		break;
	case NRF_WIFI_RF_TEST_EVENT_TX_TONE_START:
		break;

	case NRF_WIFI_RF_TEST_EVENT_XO_TUNE:
		nrf_wifi_mem_cpy(&rf_get_xo_value_params,
				(const unsigned char *)&rf_test_event->rf_test_info.rfevent[0],
				sizeof(rf_get_xo_value_params));

		def_dev_ctx->xo_offset = rf_get_xo_value_params.xo_offset;
		def_dev_ctx->xo_tune_status = rf_get_xo_value_params.status;

		switch (rf_get_xo_value_params.status) {
		case 0:
			LOG_INF("XO tune successful, optimal XO offset = %d",
					       rf_get_xo_value_params.xo_offset);
			break;
		case 1:
			LOG_ERR("XO tune failed: tone not detected");
			break;
		case 2:
			LOG_ERR("XO tune failed: gain failure (high)");
			break;
		case 3:
			LOG_ERR("XO tune failed: gain failure (low)");
			break;
		case 4:
			LOG_ERR("XO tune failed: gain failure (timeout)");
			break;
		default:
			LOG_ERR("XO tune failed: unknown status (%d)",
					      rf_get_xo_value_params.status);
			break;
		}
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
		LOG_ERR("%s: Unknown event recd: %d",
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
		LOG_ERR("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	umac_hdr = event_data;
	if_id = umac_hdr->ids.wdev_id;
	event_num = umac_hdr->cmd_evnt;

	if (if_id >= MAX_NUM_VIFS) {
		LOG_ERR("%s: Invalid wdev_id recd from UMAC %d",
				      __func__,
				      if_id);

		goto out;
	}

#ifdef NRF_WIFI_CMD_EVENT_LOG
	LOG_INF("%s: Event %d received from UMAC",
			      __func__,
			      event_num);
#else
	LOG_DBG("%s: Event %d received from UMAC",
			      __func__,
			      event_num);
#endif /* NRF_WIFI_CMD_EVENT_LOG */

	switch (umac_hdr->cmd_evnt) {
	case NRF_WIFI_UMAC_EVENT_GET_REG:
		get_reg_event = (struct nrf_wifi_reg *)event_data;

		nrf_wifi_mem_cpy(&fmac_dev_ctx->alpha2,
				      &get_reg_event->nrf_wifi_alpha2,
				      sizeof(get_reg_event->nrf_wifi_alpha2));

		fmac_dev_ctx->reg_chan_count = get_reg_event->num_channels;

		nrf_wifi_mem_cpy(fmac_dev_ctx->reg_chan_info,
				      &get_reg_event->chn_info,
				      fmac_dev_ctx->reg_chan_count *
				      sizeof(struct nrf_wifi_get_reg_chn_info));

		fmac_dev_ctx->alpha2_valid = true;
		break;
	case NRF_WIFI_UMAC_EVENT_REG_CHANGE:
		reg_change_event = (struct nrf_wifi_event_regulatory_change *)event_data;

		fmac_dev_ctx->reg_change = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL,
							       sizeof(*reg_change_event));

		if (!fmac_dev_ctx->reg_change) {
			LOG_ERR("%s: Failed to allocate memory for reg_change",
					      __func__);
			goto out;
		}

		nrf_wifi_mem_cpy(fmac_dev_ctx->reg_change,
				      reg_change_event,
				      sizeof(*reg_change_event));
		fmac_dev_ctx->reg_set_status = true;
		break;
	default:
		LOG_DBG("%s: No callback registered for event %d",
				      __func__,
				      umac_hdr->cmd_evnt);
		break;
	}

	LOG_DBG("%s: Event %d processed",
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
	LOG_INF("%s: Event type %d recd",
			      __func__,
			      rpu_msg->type);
#else
	LOG_DBG("%s: Event type %d recd",
			      __func__,
			      rpu_msg->type);
#endif /* NRF_WIFI_CMD_EVENT_LOG */

	switch (rpu_msg->type) {
	case NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC:
		status = umac_event_ctrl_process(fmac_dev_ctx,
						 rpu_msg->msg,
						 rpu_msg->hdr.len);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: umac_event_ctrl_process failed",
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
