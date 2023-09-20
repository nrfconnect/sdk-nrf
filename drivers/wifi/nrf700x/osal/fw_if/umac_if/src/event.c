/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing event specific definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "queue.h"
#include "host_rpu_umac_if.h"
#include "hal_mem.h"
#include "fmac_rx.h"
#include "fmac_tx.h"
#include "fmac_peer.h"
#include "fmac_cmd.h"
#include "fmac_ap.h"
#include "fmac_util.h"

#ifdef CONFIG_NRF700X_DATA_TX
static enum wifi_nrf_status
wifi_nrf_fmac_if_carr_state_event_proc(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				       unsigned char *umac_head,
				       enum wifi_nrf_fmac_if_carr_state carr_state)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	unsigned char if_idx = 0;
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct wifi_nrf_fmac_priv_def *def_priv = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	if (!fmac_dev_ctx || !umac_head) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);

		goto out;
	}

	if (!def_priv->callbk_fns.if_carr_state_chg_callbk_fn) {
		wifi_nrf_osal_log_dbg(fmac_dev_ctx->fpriv->opriv,
				      "%s: No callback handler registered\n",
				      __func__);

		status = WIFI_NRF_STATUS_SUCCESS;
		goto out;
	}

	if_idx = ((struct nrf_wifi_data_carrier_state *)umac_head)->wdev_id;

	if (if_idx >= MAX_NUM_VIFS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid wdev_id recd from UMAC %d\n",
				      __func__,
				      if_idx);
		goto out;
	}

	vif_ctx = def_dev_ctx->vif_ctx[if_idx];

	status = def_priv->callbk_fns.if_carr_state_chg_callbk_fn(vif_ctx->os_vif_ctx,
									     carr_state);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Interface carrier state change callback function failed for VIF idx = %d\n",
				      __func__,
				      if_idx);
		goto out;
	}
out:
	return status;
}
#endif /* CONFIG_NRF700X_DATA_TX */

#ifdef CONFIG_NRF700X_STA_MODE
static void umac_event_connect(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			       void *event_data)
{
	unsigned char if_index = 0;
	int peer_id = -1;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	struct nrf_wifi_umac_event_new_station *event = NULL;
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	event = (struct nrf_wifi_umac_event_new_station *)event_data;

	if_index = event->umac_hdr.ids.wdev_id;

	vif_ctx = def_dev_ctx->vif_ctx[if_index];
	if (if_index >= MAX_NUM_VIFS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid wdev_id recd from UMAC %d\n",
				      __func__,
				      if_index);
		return;
	}

	if (event->umac_hdr.cmd_evnt == NRF_WIFI_UMAC_EVENT_NEW_STATION) {
		if (vif_ctx->if_type == 2) {
			wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
					      vif_ctx->bssid,
					      event->mac_addr,
					      NRF_WIFI_ETH_ADDR_LEN);
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
	} else if (event->umac_hdr.cmd_evnt == NRF_WIFI_UMAC_EVENT_DEL_STATION) {
		peer_id = wifi_nrf_fmac_peer_get_id(fmac_dev_ctx, event->mac_addr);
		if (peer_id != -1) {
			wifi_nrf_fmac_peer_remove(fmac_dev_ctx,
						  if_index,
						  peer_id);
		}
	}

	return;

}
#endif /* CONFIG_NRF700X_STA_MODE */

#ifndef CONFIG_NRF700X_RADIO_TEST
static enum wifi_nrf_status umac_event_ctrl_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						    void *event_data,
						    unsigned int event_len)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_SUCCESS;
	struct nrf_wifi_umac_hdr *umac_hdr = NULL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	struct wifi_nrf_fmac_callbk_fns *callbk_fns = NULL;
	struct nrf_wifi_umac_event_vif_state *evnt_vif_state = NULL;
	unsigned char if_id = 0;
	unsigned int event_num = 0;
	bool more_res = false;
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct wifi_nrf_fmac_priv_def *def_priv = NULL;

	if (!fmac_dev_ctx || !event_data) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);
		goto out;
	}

	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!def_priv || !def_dev_ctx) {
		goto out;
	}

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

	vif_ctx = def_dev_ctx->vif_ctx[if_id];
	callbk_fns = &def_priv->callbk_fns;

	wifi_nrf_osal_log_dbg(fmac_dev_ctx->fpriv->opriv,
			      "%s: Event %d received from UMAC\n",
			      __func__,
			      event_num);

	switch (umac_hdr->cmd_evnt) {
	case NRF_WIFI_UMAC_EVENT_TRIGGER_SCAN_START:
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
	case NRF_WIFI_UMAC_EVENT_SCAN_DONE:
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
	case NRF_WIFI_UMAC_EVENT_SCAN_ABORTED:
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
	case NRF_WIFI_UMAC_EVENT_SCAN_DISPLAY_RESULT:
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
	case NRF_WIFI_UMAC_EVENT_IFFLAGS_STATUS:
		evnt_vif_state = (struct nrf_wifi_umac_event_vif_state *)event_data;

		if (evnt_vif_state->status < 0)
			goto out;

		vif_ctx->ifflags = true;
		break;
#ifdef CONFIG_NRF700X_STA_MODE
	case NRF_WIFI_UMAC_EVENT_TWT_SLEEP:
		if (callbk_fns->twt_sleep_callbk_fn)
			callbk_fns->twt_sleep_callbk_fn(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case NRF_WIFI_UMAC_EVENT_SCAN_RESULT:
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
	case NRF_WIFI_UMAC_EVENT_AUTHENTICATE:
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
	case NRF_WIFI_UMAC_EVENT_ASSOCIATE:
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
	case NRF_WIFI_UMAC_EVENT_DEAUTHENTICATE:
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
	case NRF_WIFI_UMAC_EVENT_DISASSOCIATE:
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
	case NRF_WIFI_UMAC_EVENT_FRAME:
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
	case NRF_WIFI_UMAC_EVENT_GET_TX_POWER:
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
	case NRF_WIFI_UMAC_EVENT_GET_CHANNEL:
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
	case NRF_WIFI_UMAC_EVENT_GET_STATION:
		if (callbk_fns->get_station_callbk_fn)
			callbk_fns->get_station_callbk_fn(vif_ctx->os_vif_ctx,
						     event_data,
						     event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case NRF_WIFI_UMAC_EVENT_NEW_INTERFACE:
		if (callbk_fns->get_interface_callbk_fn)
			callbk_fns->get_interface_callbk_fn(vif_ctx->os_vif_ctx,
						     event_data,
						     event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case NRF_WIFI_UMAC_EVENT_COOKIE_RESP:
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
	case NRF_WIFI_UMAC_EVENT_FRAME_TX_STATUS:
		if (callbk_fns->mgmt_tx_status)
			callbk_fns->mgmt_tx_status(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case NRF_WIFI_UMAC_EVENT_UNPROT_DEAUTHENTICATE:
	case NRF_WIFI_UMAC_EVENT_UNPROT_DISASSOCIATE:
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
	case NRF_WIFI_UMAC_EVENT_SET_INTERFACE:
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
	case NRF_WIFI_UMAC_EVENT_CONFIG_TWT:
		if (callbk_fns->twt_config_callbk_fn)
			callbk_fns->twt_config_callbk_fn(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case NRF_WIFI_UMAC_EVENT_TEARDOWN_TWT:
		if (callbk_fns->twt_teardown_callbk_fn)
			callbk_fns->twt_teardown_callbk_fn(vif_ctx->os_vif_ctx,
							   event_data,
							   event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case NRF_WIFI_UMAC_EVENT_NEW_WIPHY:
		if (callbk_fns->event_get_wiphy)
			callbk_fns->event_get_wiphy(vif_ctx->os_vif_ctx,
						    event_data,
						    event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case NRF_WIFI_UMAC_EVENT_CMD_STATUS:
	case NRF_WIFI_UMAC_EVENT_BEACON_HINT:
	case NRF_WIFI_UMAC_EVENT_CONNECT:
	case NRF_WIFI_UMAC_EVENT_DISCONNECT:
		/* Nothing to be done */
		break;
	case NRF_WIFI_UMAC_EVENT_GET_REG:
		if (callbk_fns->event_get_reg)
			callbk_fns->event_get_reg(vif_ctx->os_vif_ctx,
						      event_data,
						      event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case NRF_WIFI_UMAC_EVENT_GET_POWER_SAVE_INFO:
		if (callbk_fns->event_get_ps_info)
			callbk_fns->event_get_ps_info(vif_ctx->os_vif_ctx,
						      event_data,
						      event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
#ifdef CONFIG_NRF700X_STA_MODE
	case NRF_WIFI_UMAC_EVENT_NEW_STATION:
	case NRF_WIFI_UMAC_EVENT_DEL_STATION:
		umac_event_connect(fmac_dev_ctx,
				   event_data);
		break;
#endif /* CONFIG_NRF700X_STA_MODE */
#ifdef CONFIG_NRF700X_P2P_MODE
	case NRF_WIFI_UMAC_EVENT_REMAIN_ON_CHANNEL:
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
	case NRF_WIFI_UMAC_EVENT_CANCEL_REMAIN_ON_CHANNEL:
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
#endif /* CONFIG_NRF700X_P2P_MODE */
	case NRF_WIFI_UMAC_EVENT_GET_CONNECTION_INFO:
		if (callbk_fns->get_conn_info_callbk_fn)
			callbk_fns->get_conn_info_callbk_fn(vif_ctx->os_vif_ctx,
							    event_data,
							    event_len);
		else
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No callback registered for event %d\n",
					      __func__,
					      umac_hdr->cmd_evnt);
		break;
	case NRF_WIFI_UMAC_EVENT_REG_CHANGE:
		/* TODO: Inform the user space about the regulatory change */
		break;
#endif /* CONFIG_NRF700X_STA_MODE */
	default:
		wifi_nrf_osal_log_dbg(fmac_dev_ctx->fpriv->opriv,
				      "%s: No callback registered for event %d\n",
				      __func__,
				      umac_hdr->cmd_evnt);
		break;
	}

	wifi_nrf_osal_log_dbg(fmac_dev_ctx->fpriv->opriv,
			      "%s: Event %d processed\n",
			      __func__,
			      event_num);

out:
	return status;
}

static enum wifi_nrf_status
wifi_nrf_fmac_data_event_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				 void *umac_head)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_SUCCESS;
	int event = -1;
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!fmac_dev_ctx) {
		goto out;
	}

	if (!umac_head) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);
		goto out;
	}

	event = ((struct nrf_wifi_umac_head *)umac_head)->cmd;

	wifi_nrf_osal_log_dbg(fmac_dev_ctx->fpriv->opriv,
			      "%s: Event %d received from UMAC\n",
			      __func__,
			      event);

	switch (event) {
	case NRF_WIFI_CMD_RX_BUFF:
#ifdef CONFIG_NRF700X_RX_DONE_WQ_ENABLED
		struct nrf_wifi_rx_buff *config = wifi_nrf_osal_mem_zalloc(
			fmac_dev_ctx->fpriv->opriv,
			sizeof(struct nrf_wifi_rx_buff));
		if (!config) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Failed to allocate memory (RX)\n",
					      __func__);
			status = WIFI_NRF_STATUS_FAIL;
			break;
		}
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
					config,
					umac_head,
					sizeof(struct nrf_wifi_rx_buff));
		status = wifi_nrf_utils_q_enqueue(fmac_dev_ctx->fpriv->opriv,
					 def_dev_ctx->rx_config.rx_tasklet_event_q,
					 config);
		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Failed to enqueue RX buffer\n",
					      __func__);
			wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
					       config);
			break;
		}
		wifi_nrf_osal_tasklet_schedule(fmac_dev_ctx->fpriv->opriv,
					       def_dev_ctx->rx_tasklet);
#else
		status = wifi_nrf_fmac_rx_event_process(fmac_dev_ctx,
							umac_head);
#endif /* CONFIG_NRF700X_RX_DONE_WQ_ENABLED */
		break;
#ifdef CONFIG_NRF700X_DATA_TX
	case NRF_WIFI_CMD_TX_BUFF_DONE:
#ifdef CONFIG_NRF700X_TX_DONE_WQ_ENABLED
		struct nrf_wifi_tx_buff_done *config = wifi_nrf_osal_mem_zalloc(
					fmac_dev_ctx->fpriv->opriv,
					sizeof(struct nrf_wifi_tx_buff_done));
		if (!config) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Failed to allocate memory (TX)\n",
					      __func__);
			status = WIFI_NRF_STATUS_FAIL;
			break;
		}
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
					config,
					umac_head,
					sizeof(struct nrf_wifi_tx_buff_done));
		status = wifi_nrf_utils_q_enqueue(fmac_dev_ctx->fpriv->opriv,
			def_dev_ctx->tx_config.tx_done_tasklet_event_q,
			config);
		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Failed to enqueue TX buffer\n",
					      __func__);
			wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
					       config);
			break;
		}
		wifi_nrf_osal_tasklet_schedule(fmac_dev_ctx->fpriv->opriv,
				def_dev_ctx->tx_done_tasklet);
#else
		status = wifi_nrf_fmac_tx_done_event_process(fmac_dev_ctx,
								umac_head);
#endif /* CONFIG_NRF700X_TX_DONE_WQ_ENABLED */
		break;
	case NRF_WIFI_CMD_CARRIER_ON:
		status = wifi_nrf_fmac_if_carr_state_event_proc(fmac_dev_ctx,
								umac_head,
								WIFI_NRF_FMAC_IF_CARR_STATE_ON);
		break;
	case NRF_WIFI_CMD_CARRIER_OFF:
		status = wifi_nrf_fmac_if_carr_state_event_proc(fmac_dev_ctx,
								umac_head,
								WIFI_NRF_FMAC_IF_CARR_STATE_OFF);
		break;
#endif /* CONFIG_NRF700X_DATA_TX */
#ifdef CONFIG_NRF700X_AP_MODE
	case NRF_WIFI_CMD_PM_MODE:
		status = sap_client_update_pmmode(fmac_dev_ctx,
						  umac_head);
		break;
	case NRF_WIFI_CMD_PS_GET_FRAMES:
		status = sap_client_ps_get_frames(fmac_dev_ctx,
						  umac_head);
		break;
#endif /* CONFIG_NRF700X_AP_MODE */
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

		host_rpu_length_left -= ((struct nrf_wifi_umac_head *)umac_head)->len;
		umac_head += ((struct nrf_wifi_umac_head *)umac_head)->len;
	}
out:
	return status;
}
#else /* CONFIG_NRF700X_RADIO_TEST */
static enum wifi_nrf_status umac_event_rf_test_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						       void *event)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct nrf_wifi_event_rftest *rf_test_event = NULL;
	struct nrf_wifi_temperature_params rf_test_get_temperature;
	struct nrf_wifi_rf_get_rf_rssi rf_get_rf_rssi;
	struct nrf_wifi_rf_test_xo_calib xo_calib_params;
	struct nrf_wifi_rf_get_xo_value rf_get_xo_value_params;
	struct wifi_nrf_fmac_dev_ctx_rt *def_dev_ctx;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!event) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);
		goto out;
	}

	rf_test_event = ((struct nrf_wifi_event_rftest *)event);

	if (rf_test_event->rf_test_info.rfevent[0] != def_dev_ctx->rf_test_type) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid event type (%d) recd for RF test type (%d)\n",
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

		break;
	case NRF_WIFI_RF_TEST_EVENT_TX_TONE_START:
	case NRF_WIFI_RF_TEST_EVENT_DPD_ENABLE:
		break;

	case NRF_WIFI_RF_TEST_GET_TEMPERATURE:
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv, &rf_test_get_temperature,
			(const unsigned char *)&rf_test_event->rf_test_info.rfevent[0],
			sizeof(rf_test_get_temperature));

		if (rf_test_get_temperature.readTemperatureStatus) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
			"Temperature reading failed\n");
		} else {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
			"Temperature reading success: \t");

			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
			"The temperature is = %d degree celsius\n",
			rf_test_get_temperature.temperature);
		}
		break;
	case NRF_WIFI_RF_TEST_EVENT_RF_RSSI:
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv, &rf_get_rf_rssi,
			(const unsigned char *)&rf_test_event->rf_test_info.rfevent[0],
			sizeof(rf_get_rf_rssi));

		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
		"RF RSSI value is = %d\n",
		rf_get_rf_rssi.agc_status_val);
		break;
	case NRF_WIFI_RF_TEST_EVENT_XO_CALIB:
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv, &xo_calib_params,
			(const unsigned char *)&rf_test_event->rf_test_info.rfevent[0],
			sizeof(xo_calib_params));

		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
		"XO value configured is = %d\n",
		xo_calib_params.xo_val);
		break;
	case NRF_WIFI_RF_TEST_XO_TUNE:
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv, &rf_get_xo_value_params,
			(const unsigned char *)&rf_test_event->rf_test_info.rfevent[0],
			sizeof(rf_get_xo_value_params));

		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
		"Best XO value is = %d\n",
		rf_get_xo_value_params.xo_value);
		break;
	default:
		break;
	}

	def_dev_ctx->rf_test_type = NRF_WIFI_RF_TEST_MAX;
	status = WIFI_NRF_STATUS_SUCCESS;

out:
	return status;
}


#endif /* !CONFIG_NRF700X_RADIO_TEST */


static enum wifi_nrf_status umac_event_stats_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						     void *event)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct nrf_wifi_umac_event_stats *stats = NULL;

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

	stats = ((struct nrf_wifi_umac_event_stats *)event);

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
#ifdef CONFIG_NRF700X_RADIO_TEST
	struct wifi_nrf_fmac_dev_ctx_rt *def_dev_ctx;
	struct nrf_wifi_umac_event_err_status *umac_status =
		((struct nrf_wifi_umac_event_err_status *)sys_head);

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
#endif /* CONFIG_NRF700X_RADIO_TEST */

	if (!fmac_dev_ctx || !rpu_msg) {
		return status;
	}

	sys_head = (unsigned char *)rpu_msg->msg;

	switch (((struct nrf_wifi_sys_head *)sys_head)->cmd_event) {
	case NRF_WIFI_EVENT_STATS:
		status = umac_event_stats_process(fmac_dev_ctx,
						  sys_head);
		break;
	case NRF_WIFI_EVENT_INIT_DONE:
		fmac_dev_ctx->fw_init_done = 1;
		status = WIFI_NRF_STATUS_SUCCESS;
		break;
	case NRF_WIFI_EVENT_DEINIT_DONE:
		fmac_dev_ctx->fw_deinit_done = 1;
		status = WIFI_NRF_STATUS_SUCCESS;
		break;
#ifdef CONFIG_NRF700X_RADIO_TEST
	case NRF_WIFI_EVENT_RF_TEST:
		status = umac_event_rf_test_process(fmac_dev_ctx,
						    sys_head);
		break;
	case NRF_WIFI_EVENT_RADIOCMD_STATUS:
		def_dev_ctx->radio_cmd_status = umac_status->status;
		def_dev_ctx->radio_cmd_done = true;
		status = WIFI_NRF_STATUS_SUCCESS;
		break;
#endif /* CONFIG_NRF700X_RADIO_TEST */
	default:
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unknown event recd: %d\n",
				      __func__,
				      ((struct nrf_wifi_sys_head *)sys_head)->cmd_event);
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
	struct nrf_wifi_umac_hdr *umac_hdr = NULL;
	unsigned int umac_msg_len = 0;
	int umac_msg_type = NRF_WIFI_UMAC_EVENT_UNSPECIFIED;

	fmac_dev_ctx = (struct wifi_nrf_fmac_dev_ctx *)mac_dev_ctx;

	rpu_msg = (struct host_rpu_msg *)rpu_event_data;
	umac_hdr = (struct nrf_wifi_umac_hdr *)rpu_msg->msg;
	umac_msg_len = rpu_msg->hdr.len;
	umac_msg_type = umac_hdr->cmd_evnt;

	wifi_nrf_osal_log_dbg(fmac_dev_ctx->fpriv->opriv,
			      "%s: Event type %d recd\n",
			      __func__,
			      rpu_msg->type);

	switch (rpu_msg->type) {
#ifndef CONFIG_NRF700X_RADIO_TEST
	case NRF_WIFI_HOST_RPU_MSG_TYPE_DATA:
		status = wifi_nrf_fmac_data_events_process(fmac_dev_ctx,
							   rpu_msg);
		break;
	case NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC:
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
#endif /* !CONFIG_NRF700X_RADIO_TEST */
	case NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM:
		status = umac_process_sys_events(fmac_dev_ctx,
						 rpu_msg);
		break;
	default:
		goto out;
	}

out:
	return status;
}
