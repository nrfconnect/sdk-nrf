/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing event specific definitions in the
 * Offloaded raw TX modefor the FMAC IF Layer of the Wi-Fi driver.
 */

#include "queue.h"
#include <nrf71_wifi_ctrl.h>
#include "common/hal_mem.h"
#include "offload_raw_tx/fmac_structs.h"
#include "common/fmac_util.h"
static enum nrf_wifi_status umac_event_off_raw_tx_stats_process(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	void *event)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_off_raw_tx_umac_event_stats *stats = NULL;

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

	stats = ((struct nrf_wifi_off_raw_tx_umac_event_stats *)event);

	nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fw_stats,
			      &stats->fw,
			      sizeof(stats->fw));

	fmac_dev_ctx->stats_req = false;

	status = NRF_WIFI_STATUS_SUCCESS;

out:
	return status;
}




static enum nrf_wifi_status umac_event_off_raw_tx_proc_events(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct host_rpu_msg *rpu_msg)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char *sys_head = NULL;
	struct nrf_wifi_off_raw_tx_fmac_dev_ctx *dev_ctx_off_raw_tx;
	struct nrf_wifi_umac_event_err_status *umac_status;

	if (!fmac_dev_ctx || !rpu_msg) {
		return status;
	}

	dev_ctx_off_raw_tx = wifi_dev_priv(fmac_dev_ctx);
	sys_head = (unsigned char *)rpu_msg->msg;

	switch (((struct nrf_wifi_sys_head *)sys_head)->cmd_event) {
	case NRF_WIFI_EVENT_STATS:
		status = umac_event_off_raw_tx_stats_process(fmac_dev_ctx,
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
	case NRF_WIFI_EVENT_OFFLOADED_RAWTX_STATUS:
		umac_status = ((struct nrf_wifi_umac_event_err_status *)sys_head);
		dev_ctx_off_raw_tx->off_raw_tx_cmd_status = umac_status->status;
		dev_ctx_off_raw_tx->off_raw_tx_cmd_done = false;
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


enum nrf_wifi_status nrf_wifi_off_raw_tx_fmac_event_callback(void *mac_dev_ctx,
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
		status = umac_event_off_raw_tx_proc_events(fmac_dev_ctx,
							   rpu_msg);
		break;
	default:
		goto out;
	}

out:
	return status;
}
