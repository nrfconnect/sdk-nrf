/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing event specific definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "host_rpu_umac_if.h"
#include "fmac_rx.h"
#include "fmac_tx.h"
#include "fmac_peer.h"
#include "fmac_cmd.h"
#include "fmac_ap.h"

static enum wifi_nrf_status
wifi_nrf_fmac_if_state_chg_event_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					 unsigned char *umac_head,
					 enum wifi_nrf_fmac_if_state if_state)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	unsigned char if_idx = 0;

	if (!fmac_dev_ctx || !umac_head) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);

		goto out;
	}

	if (!fmac_dev_ctx->fpriv->callbk_fns.if_state_chg_callbk_fn) {
		wifi_nrf_osal_log_dbg(fmac_dev_ctx->fpriv->opriv,
				      "%s: No callback handler registered\n",
				      __func__);

		status = WIFI_NRF_STATUS_SUCCESS;
		goto out;
	}

	if_idx = ((struct img_data_carrier_state *)umac_head)->wdev_id;

	if (if_idx >= MAX_NUM_VIFS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid wdev_id recd from UMAC %d\n",
				      __func__,
				      if_idx);
		goto out;
	}

	vif_ctx = fmac_dev_ctx->vif_ctx[if_idx];

	status = fmac_dev_ctx->fpriv->callbk_fns.if_state_chg_callbk_fn(vif_ctx->os_vif_ctx,
									if_state);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: If state change callback function failed for VIF idx = %d\n",
				      __func__,
				      if_idx);
		goto out;
	}
out:
	return status;
}


static void umac_event_connect(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			       void *event_data)
{
	unsigned char if_index = 0;
	int peer_id = -1;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	struct img_umac_event_new_station *event = NULL;

	event = (struct img_umac_event_new_station *)event_data;

	if_index = event->umac_hdr.ids.wdev_id;

	vif_ctx = fmac_dev_ctx->vif_ctx[if_index];
	if (if_index >= MAX_NUM_VIFS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid wdev_id recd from UMAC %d\n",
				      __func__,
				      if_index);
		return;
	}

	if (event->umac_hdr.cmd_evnt == IMG_UMAC_EVENT_NEW_STATION) {
		if (vif_ctx->if_type == 2) {
			wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
					      vif_ctx->bssid,
					      event->mac_addr,
					      IMG_ETH_ALEN);
		}
		peer_id = wifi_nrf_fmac_peer_get_id(fmac_dev_ctx, event->mac_addr);

		if (peer_id == -1) {
			peer_id = wifi_nrf_fmac_peer_add(fmac_dev_ctx,
							 if_index,
							 event->mac_addr,
							 event->is_sta_legacy,
							 event->wme);

			if (peer_id == -1) {
				wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
						      "%s:Can't add new station.\n",
						      __func__);
				return;
			}
		}
	} else if (event->umac_hdr.cmd_evnt == IMG_UMAC_EVENT_DEL_STATION) {
		peer_id = wifi_nrf_fmac_peer_get_id(fmac_dev_ctx, event->mac_addr);
		if (peer_id != -1) {
			wifi_nrf_fmac_peer_remove(fmac_dev_ctx,
						  if_index,
						  peer_id);
		}
	}

	return;

}


static enum wifi_nrf_status umac_event_ctrl_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						    void *event_data,
						    unsigned int event_len)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_SUCCESS;
	struct img_umac_hdr *umac_hdr = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	struct wifi_nrf_fmac_callbk_fns *callbk_fns = NULL;
	struct img_umac_event_vif_state *evnt_vif_state = NULL;
	unsigned char if_id = 0;
	unsigned int event_num = 0;
	bool more_res = false;

	umac_hdr = event_data;
	if_id = umac_hdr->ids.wdev_id;
	event_num = umac_hdr->cmd_evnt;

	if (if_id >= MAX_NUM_VIFS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid wdev_id recd from UMAC %d\n",
				      __func__,
				      if_id);

		goto out;
	}

	vif_ctx = fmac_dev_ctx->vif_ctx[if_id];
	callbk_fns = &fmac_dev_ctx->fpriv->callbk_fns;

	switch (umac_hdr->cmd_evnt) {
	case IMG_UMAC_EVENT_TRIGGER_SCAN_START:
		if (callbk_fns->scan_start_callbk_fn)
			callbk_fns->scan_start_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_SCAN_DONE:
		if (callbk_fns->scan_done_callbk_fn)
			callbk_fns->scan_done_callbk_fn(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_SCAN_ABORTED:
		if (callbk_fns->scan_abort_callbk_fn)
			callbk_fns->scan_abort_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_SCAN_RESULT:
		if (umac_hdr->seq != 0)
			more_res = true;

		if (callbk_fns->scan_res_callbk_fn)
			callbk_fns->scan_res_callbk_fn(vif_ctx->os_vif_ctx,
						       event_data,
						       event_len,
						       more_res);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_SCAN_DISPLAY_RESULT:
		if (umac_hdr->seq != 0)
			more_res = true;

		if (callbk_fns->disp_scan_res_callbk_fn)
			callbk_fns->disp_scan_res_callbk_fn(vif_ctx->os_vif_ctx,
							    event_data,
							    event_len,
							    more_res);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_AUTHENTICATE:
		if (callbk_fns->auth_resp_callbk_fn)
			callbk_fns->auth_resp_callbk_fn(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_ASSOCIATE:
		if (callbk_fns->assoc_resp_callbk_fn)
			callbk_fns->assoc_resp_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_DEAUTHENTICATE:
		if (callbk_fns->deauth_callbk_fn)
			callbk_fns->deauth_callbk_fn(vif_ctx->os_vif_ctx,
						     event_data,
						     event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_DISASSOCIATE:
		if (callbk_fns->disassoc_callbk_fn)
			callbk_fns->disassoc_callbk_fn(vif_ctx->os_vif_ctx,
						       event_data,
						       event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_FRAME:
		if (callbk_fns->mgmt_rx_callbk_fn)
			callbk_fns->mgmt_rx_callbk_fn(vif_ctx->os_vif_ctx,
						      event_data,
						      event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_GET_TX_POWER:
		if (callbk_fns->tx_pwr_get_callbk_fn)
			callbk_fns->tx_pwr_get_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_GET_CHANNEL:
		if (callbk_fns->chnl_get_callbk_fn)
			callbk_fns->chnl_get_callbk_fn(vif_ctx->os_vif_ctx,
						       event_data,
						       event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_GET_STATION:
		if (callbk_fns->sta_get_callbk_fn)
			callbk_fns->sta_get_callbk_fn(vif_ctx->os_vif_ctx,
						      event_data,
						      event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_COOKIE_RESP:
		if (callbk_fns->cookie_rsp_callbk_fn)
			callbk_fns->cookie_rsp_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_FRAME_TX_STATUS:
		if (callbk_fns->tx_status_callbk_fn)
			callbk_fns->tx_status_callbk_fn(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_UNPROT_DEAUTHENTICATE:
	case IMG_UMAC_EVENT_UNPROT_DISASSOCIATE:
		if (callbk_fns->unprot_mlme_mgmt_rx_callbk_fn)
			callbk_fns->unprot_mlme_mgmt_rx_callbk_fn(vif_ctx->os_vif_ctx,
								  event_data,
								  event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_IFFLAGS_STATUS:
		evnt_vif_state = (struct img_umac_event_vif_state *)event_data;

		if (evnt_vif_state->status < 0)
			goto out;

		vif_ctx->ifflags = true;
		break;
	case IMG_UMAC_EVENT_SET_INTERFACE:
		if (callbk_fns->set_if_callbk_fn)
			callbk_fns->set_if_callbk_fn(vif_ctx->os_vif_ctx,
						     event_data,
						     event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_CMD_STATUS:
	case IMG_UMAC_EVENT_BEACON_HINT:
	case IMG_UMAC_EVENT_CONNECT:
	case IMG_UMAC_EVENT_DISCONNECT:
		/* Nothing to be done */
		break;
	case IMG_UMAC_EVENT_NEW_STATION:
	case IMG_UMAC_EVENT_DEL_STATION:
		umac_event_connect(fmac_dev_ctx,
				   event_data);
		break;
	case IMG_UMAC_EVENT_REMAIN_ON_CHANNEL:
		if (callbk_fns->roc_callbk_fn)
			callbk_fns->roc_callbk_fn(vif_ctx->os_vif_ctx,
						  event_data,
						  event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case IMG_UMAC_EVENT_CANCEL_REMAIN_ON_CHANNEL:
		if (callbk_fns->roc_cancel_callbk_fn)
			callbk_fns->roc_cancel_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	default:
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: No callback registered for event %d\n",
				      __func__,
				      umac_hdr->cmd_evnt);
		break;
	}

out:
	return status;
}


static enum wifi_nrf_status
wifi_nrf_fmac_data_event_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				 void *umac_head)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	int event = -1;

	if (!fmac_dev_ctx) {
		goto out;
	}

	if (!umac_head) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);
		goto out;
	}

	event = ((struct img_umac_head *)umac_head)->cmd;

	switch (event) {
	case IMG_CMD_RX_BUFF:
		status = wifi_nrf_fmac_rx_event_process(fmac_dev_ctx,
							umac_head);
		break;
	case IMG_CMD_TX_BUFF_DONE:
		status = wifi_nrf_fmac_tx_done_event_process(fmac_dev_ctx,
							     umac_head);
		break;
	case IMG_CMD_CARRIER_ON:
		status = wifi_nrf_fmac_if_state_chg_event_process(fmac_dev_ctx,
								  umac_head,
								  WIFI_NRF_FMAC_IF_STATE_UP);
		break;
	case IMG_CMD_CARRIER_OFF:
		status = wifi_nrf_fmac_if_state_chg_event_process(fmac_dev_ctx,
								  umac_head,
								  WIFI_NRF_FMAC_IF_STATE_DOWN);
		break;
	case IMG_CMD_PM_MODE:
		status = sap_client_update_pmmode(fmac_dev_ctx,
						  umac_head);
		break;
	case IMG_CMD_PS_GET_FRAMES:
		status = sap_client_ps_get_frames(fmac_dev_ctx,
						  umac_head);
		break;
	default:
		break;
	}

out:
	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Failed for event = %d\n",
				      __func__,
				      event);
	}

	return status;
}


static enum wifi_nrf_status
wifi_nrf_fmac_data_events_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				  struct host_rpu_msg *rpu_msg)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned char *umac_head = NULL;
	int host_rpu_length_left = 0;

	if (!fmac_dev_ctx || !rpu_msg) {
		goto out;
	}

	umac_head = (unsigned char *)rpu_msg->msg;
	host_rpu_length_left = rpu_msg->hdr.len - sizeof(struct host_rpu_msg);

	while (host_rpu_length_left > 0) {
		status = wifi_nrf_fmac_data_event_process(fmac_dev_ctx,
							  umac_head);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: umac_process_data_event failed\n",
					      __func__);
			goto out;
		}

		host_rpu_length_left -= ((struct img_umac_head *)umac_head)->len;
		umac_head += ((struct img_umac_head *)umac_head)->len;
	}
out:
	return status;
}




static enum wifi_nrf_status umac_event_stats_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						     void *event)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct img_umac_event_stats *stats = NULL;

	if (!event) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);
		goto out;
	}

	if (!fmac_dev_ctx->stats_req) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Stats recd when req was not sent!\n",
				      __func__);
		goto out;
	}

	stats = ((struct img_umac_event_stats *)event);

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      fmac_dev_ctx->fw_stats,
			      &stats->fw,
			      sizeof(*fmac_dev_ctx->fw_stats));

	fmac_dev_ctx->stats_req = false;

	status = WIFI_NRF_STATUS_SUCCESS;

out:
	return status;
}


static enum wifi_nrf_status umac_process_sys_events(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						    struct host_rpu_msg *rpu_msg)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned char *sys_head = NULL;

	sys_head = (unsigned char *)rpu_msg->msg;

	switch (((struct img_sys_head *)sys_head)->cmd_event) {
	case IMG_EVENT_STATS:
		status = umac_event_stats_process(fmac_dev_ctx,
						  sys_head);
		break;
	case IMG_EVENT_INIT_DONE:
		fmac_dev_ctx->init_done = 1;
		status = WIFI_NRF_STATUS_SUCCESS;
		break;
	case IMG_EVENT_DEINIT_DONE:
		fmac_dev_ctx->deinit_done = 1;
		status = WIFI_NRF_STATUS_SUCCESS;
		break;
	default:
		status = WIFI_NRF_STATUS_FAIL;
		break;
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_event_callback(void *mac_dev_ctx,
						  void *rpu_event_data,
						  unsigned int rpu_event_len)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct host_rpu_msg *rpu_msg = NULL;
	struct img_umac_hdr *umac_hdr = NULL;
	unsigned int umac_msg_len = 0;
	int umac_msg_type = IMG_UMAC_EVENT_UNSPECIFIED;

	fmac_dev_ctx = (struct wifi_nrf_fmac_dev_ctx *)mac_dev_ctx;

	rpu_msg = (struct host_rpu_msg *)rpu_event_data;
	umac_hdr = (struct img_umac_hdr *)rpu_msg->msg;
	umac_msg_len = rpu_msg->hdr.len;
	umac_msg_type = umac_hdr->cmd_evnt;

	switch (rpu_msg->type) {
	case IMG_HOST_RPU_MSG_TYPE_DATA:
		status = wifi_nrf_fmac_data_events_process(fmac_dev_ctx,
							   rpu_msg);
		break;
	case IMG_HOST_RPU_MSG_TYPE_UMAC:
		status = umac_event_ctrl_process(fmac_dev_ctx,
						 rpu_msg->msg,
						 rpu_msg->hdr.len);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: umac_event_ctrl_process failed\n",
					      __func__);
			goto out;
		}
		break;
	case IMG_HOST_RPU_MSG_TYPE_SYSTEM:
		status = umac_process_sys_events(fmac_dev_ctx,
						 rpu_msg);
		break;
	default:
		goto out;
	}

out:
	return status;
}
