/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


/**
 * @brief File containing event specific definitions in the System mode
 * for the FMAC IF Layer of the Wi-Fi driver.
 */

#include "queue.h"

#include <nrf71_wifi_ctrl.h>
#include "system/fmac_rx.h"
#include "system/fmac_tx.h"
#include "system/fmac_peer.h"
#include "system/fmac_ap.h"
#include "system/fmac_event.h"
#include "common/fmac_util.h"

#ifdef NRF71_SYSTEM_WITH_RAW_MODES
static enum nrf_wifi_status
nrf_wifi_fmac_if_mode_set_event_proc(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				     struct nrf_wifi_event_raw_config_mode *mode_event)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_SUCCESS;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx;
	struct tx_config *config;
	struct nrf_wifi_fmac_vif_ctx *vif;
	unsigned char if_idx = mode_event->if_index;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	config = &sys_dev_ctx->tx_config;
	vif = sys_dev_ctx->vif_ctx[if_idx];

	if (!mode_event->status) {
		vif->mode = mode_event->op_mode;
#ifdef NRF71_RAW_DATA_TX
		vif->txinjection_mode = false;
#endif /* NRF71_RAW_DATA_TX */
#ifdef NRF71_PROMISC_DATA_RX
		vif->promisc_mode = false;
#endif /* NRF71_PROMISC_DATA_RX */
		if ((mode_event->op_mode & NRF_WIFI_STA_MODE)
			== NRF_WIFI_STA_MODE) {
			mode_event->op_mode ^= NRF_WIFI_STA_MODE;
			vif->if_type = NRF_WIFI_IFTYPE_STATION;
			config->peers[MAX_PEERS].peer_id = -1;
			config->peers[MAX_PEERS].if_idx = -1;

#if defined(NRF71_RAW_DATA_TX) && defined(NRF71_PROMISC_DATA_RX)
			if ((mode_event->op_mode ^
			    (NRF_WIFI_PROMISCUOUS_MODE |
			     NRF_WIFI_TX_INJECTION_MODE)) == 0) {
				vif->if_type
					= NRF_WIFI_STA_PROMISC_TX_INJECTOR;
				config->peers[MAX_PEERS].peer_id
					= MAX_PEERS;
				config->peers[MAX_PEERS].if_idx
					= if_idx;
				vif->txinjection_mode = true;
				vif->promisc_mode = true;
			}
#endif /* NRF71_RAW_DATA_TX && defined NRF71_PROMISC_DATA_RX */
#ifdef NRF71_RAW_DATA_TX
			if ((mode_event->op_mode ^
			     NRF_WIFI_TX_INJECTION_MODE) == 0) {
				config->peers[MAX_PEERS].peer_id
					= MAX_PEERS;
				config->peers[MAX_PEERS].if_idx
					= if_idx;
				vif->if_type = NRF_WIFI_STA_TX_INJECTOR;
				vif->txinjection_mode = true;
			}
#endif /* NRF71_RAW_DATA_TX */
#ifdef NRF71_PROMISC_DATA_RX
			if ((mode_event->op_mode ^
			     NRF_WIFI_PROMISCUOUS_MODE) == 0) {
				vif->if_type = NRF_WIFI_STA_PROMISC;
				vif->promisc_mode = true;
			}
#endif /* NRF71_PROMISC_DATA_RX */
			goto out;
		}
#ifdef NRF71_RAW_DATA_RX
		if ((mode_event->op_mode & NRF_WIFI_MONITOR_MODE)
			== NRF_WIFI_MONITOR_MODE) {
			mode_event->op_mode ^= NRF_WIFI_MONITOR_MODE;
			vif->if_type = NRF_WIFI_IFTYPE_MONITOR;
			config->peers[MAX_PEERS].peer_id = -1;
			config->peers[MAX_PEERS].if_idx = -1;
#ifdef NRF71_RAW_DATA_TX
			if ((mode_event->op_mode ^
			     NRF_WIFI_TX_INJECTION_MODE) == 0) {
				config->peers[MAX_PEERS].peer_id
					= MAX_PEERS;
				config->peers[MAX_PEERS].if_idx
					= if_idx;
				vif->if_type = NRF_WIFI_MONITOR_TX_INJECTOR;
				vif->txinjection_mode = true;
			}
#endif
			goto out;
		}
#endif
	} else {
		nrf_wifi_osal_log_err("%s: Set mode failed!",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
	}
out:
	return status;
}
#endif /* NRF71_SYSTEM_WITH_RAW_MODES */


#ifdef NRF71_DATA_TX
static enum nrf_wifi_status
nrf_wifi_fmac_if_carr_state_event_proc(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				       unsigned char *umac_head,
				       enum nrf_wifi_fmac_if_carr_state carr_state)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	unsigned char if_idx = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	if (!fmac_dev_ctx || !umac_head) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);

		goto out;
	}

	if (!sys_fpriv->callbk_fns.if_carr_state_chg_callbk_fn) {
		nrf_wifi_osal_log_dbg("%s: No callback handler registered",
				      __func__);

		status = NRF_WIFI_STATUS_SUCCESS;
		goto out;
	}

	if_idx = ((struct nrf_wifi_data_carrier_state *)umac_head)->wdev_id;

	if (if_idx >= MAX_NUM_VIFS) {
		nrf_wifi_osal_log_err("%s: Invalid wdev_id recd from UMAC %d",
				      __func__,
				      if_idx);
		goto out;
	}

	vif_ctx = sys_dev_ctx->vif_ctx[if_idx];

	status = sys_fpriv->callbk_fns.if_carr_state_chg_callbk_fn(vif_ctx->os_vif_ctx,
									     carr_state);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: IF carrier state change failed for VIF idx = %d",
				      __func__,
				      if_idx);
		goto out;
	}
out:
	return status;
}
#endif /* NRF71_DATA_TX */


static enum nrf_wifi_status umac_event_sys_stats_process(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	void *event)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_sys_umac_event_stats *stats = NULL;

	if (!event) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	if (!fmac_dev_ctx->stats_req) {
		nrf_wifi_osal_log_dbg("%s: Stats recd when req was not sent!",
				      __func__);
		status = NRF_WIFI_STATUS_SUCCESS;
		goto out;
	}

	stats = ((struct nrf_wifi_sys_umac_event_stats *)event);

	nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fw_stats,
			      &stats->fw,
			      sizeof(stats->fw));

	fmac_dev_ctx->stats_req = false;

	status = NRF_WIFI_STATUS_SUCCESS;

out:
	return status;
}


static enum nrf_wifi_status umac_event_sys_proc_events(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						       struct host_rpu_msg *rpu_msg)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char *sys_head = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx;

	if (!fmac_dev_ctx || !rpu_msg) {
		return status;
	}


	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	sys_head = (unsigned char *)rpu_msg->msg;

	switch (((struct nrf_wifi_sys_head *)sys_head)->cmd_event) {
	case NRF_WIFI_EVENT_STATS:
		status = umac_event_sys_stats_process(fmac_dev_ctx,
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
#ifdef NRF71_RAW_DATA_TX
	case NRF_WIFI_EVENT_RAW_TX_DONE:
		status = nrf_wifi_fmac_rawtx_done_event_process(fmac_dev_ctx,
						(struct nrf_wifi_event_raw_tx_done *)sys_head);
		break;
#endif
#ifdef NRF71_SYSTEM_WITH_RAW_MODES
	case NRF_WIFI_EVENT_MODE_SET_DONE:
		status = nrf_wifi_fmac_if_mode_set_event_proc(fmac_dev_ctx,
						(struct nrf_wifi_event_raw_config_mode *)sys_head);
		break;
#endif
#if defined(NRF71_RAW_DATA_TX) || defined(NRF71_RAW_DATA_RX)
	case NRF_WIFI_EVENT_CHANNEL_SET_DONE:
		struct nrf_wifi_event_set_channel *channel_event;

		channel_event = (struct nrf_wifi_event_set_channel *)sys_head;
		if (!channel_event->status) {
			sys_dev_ctx->vif_ctx[channel_event->if_index]->channel =
			channel_event->chan.primary_num;
		}
		status = NRF_WIFI_STATUS_SUCCESS;
		break;
#endif /* NRF71_RAW_DATA_TX */
#if defined(NRF71_RAW_DATA_RX) || defined(NRF71_PROMISC_DATA_RX)
	case NRF_WIFI_EVENT_FILTER_SET_DONE:
		struct nrf_wifi_event_raw_config_filter *filter_event;

		filter_event = (struct nrf_wifi_event_raw_config_filter *)sys_head;
		if (!filter_event->status) {
			sys_dev_ctx->vif_ctx[filter_event->if_index]->packet_filter =
								filter_event->filter;
		}
		status = NRF_WIFI_STATUS_SUCCESS;
		break;
#endif /* NRF71_RAW_DATA_RX || NRF71_PROMISC_DATA_RX */
	default:
		nrf_wifi_osal_log_err("%s: Unknown event recd: %d",
				      __func__,
				      ((struct nrf_wifi_sys_head *)sys_head)->cmd_event);
		break;
	}
	return status;
}


static enum nrf_wifi_status
nrf_wifi_fmac_data_event_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				 void *umac_head)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_SUCCESS;
	int event = -1;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!fmac_dev_ctx) {
		goto out;
	}

	if (!umac_head) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	event = ((struct nrf_wifi_umac_head *)umac_head)->cmd;

#ifdef NRF_WIFI_CMD_EVENT_LOG
	nrf_wifi_osal_log_info("%s: Event %d received from UMAC",
			      __func__,
			      event);
#else
	nrf_wifi_osal_log_dbg("%s: Event %d received from UMAC",
			      __func__,
			      event);
#endif /* NRF_WIFI_CMD_EVENT_LOG */

	switch (event) {
	case NRF_WIFI_CMD_RX_BUFF:
#ifdef NRF71_RX_DONE_WQ_ENABLED
		struct nrf_wifi_rx_buff *config = nrf_wifi_osal_mem_zalloc(
			sizeof(struct nrf_wifi_rx_buff));
		if (!config) {
			nrf_wifi_osal_log_err("%s: Failed to allocate memory (RX)",
					      __func__);
			status = NRF_WIFI_STATUS_FAIL;
			break;
		}
		nrf_wifi_osal_mem_cpy(config,
				      umac_head,
				      sizeof(struct nrf_wifi_rx_buff));
		status = nrf_wifi_utils_q_enqueue(sys_dev_ctx->rx_config.rx_tasklet_event_q,
						  config);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: Failed to enqueue RX buffer",
					      __func__);
			nrf_wifi_osal_mem_free(config);
			break;
		}
		nrf_wifi_osal_tasklet_schedule(sys_dev_ctx->rx_tasklet);
#else
		status = nrf_wifi_fmac_rx_event_process(fmac_dev_ctx,
							umac_head);
#endif /* NRF71_RX_DONE_WQ_ENABLED */
		break;
#ifdef NRF71_DATA_TX
	case NRF_WIFI_CMD_TX_BUFF_DONE:
#ifdef NRF71_TX_DONE_WQ_ENABLED
		struct nrf_wifi_tx_buff_done *config = nrf_wifi_osal_mem_zalloc(
					sizeof(struct nrf_wifi_tx_buff_done));
		if (!config) {
			nrf_wifi_osal_log_err("%s: Failed to allocate memory (TX)",
					      __func__);
			status = NRF_WIFI_STATUS_FAIL;
			break;
		}
		nrf_wifi_osal_mem_cpy(config,
				      umac_head,
				      sizeof(struct nrf_wifi_tx_buff_done));
		status = nrf_wifi_utils_q_enqueue(sys_dev_ctx->tx_config.tx_done_tasklet_event_q,
						  config);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: Failed to enqueue TX buffer",
					      __func__);
			nrf_wifi_osal_mem_free(config);
			break;
		}
		nrf_wifi_osal_tasklet_schedule(sys_dev_ctx->tx_done_tasklet);
#else
		status = nrf_wifi_fmac_tx_done_event_process(fmac_dev_ctx,
								umac_head);
#endif /* NRF71_TX_DONE_WQ_ENABLED */
		break;
	case NRF_WIFI_CMD_CARRIER_ON:
		status = nrf_wifi_fmac_if_carr_state_event_proc(fmac_dev_ctx,
								umac_head,
								NRF_WIFI_FMAC_IF_CARR_STATE_ON);
		break;
	case NRF_WIFI_CMD_CARRIER_OFF:
		status = nrf_wifi_fmac_if_carr_state_event_proc(fmac_dev_ctx,
								umac_head,
								NRF_WIFI_FMAC_IF_CARR_STATE_OFF);
		break;
#endif /* NRF71_DATA_TX */
#ifdef NRF71_AP_MODE
	case NRF_WIFI_CMD_PM_MODE:
		status = sap_client_update_pmmode(fmac_dev_ctx,
						  umac_head);
		break;
	case NRF_WIFI_CMD_PS_GET_FRAMES:
		status = sap_client_ps_get_frames(fmac_dev_ctx,
						  umac_head);
		break;
#endif /* NRF71_AP_MODE */
	default:
		break;
	}

out:
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: Failed for event = %d",
				      __func__,
				      event);
	}

	return status;
}


static enum nrf_wifi_status
nrf_wifi_fmac_data_events_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				  struct host_rpu_msg *rpu_msg)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char *umac_head = NULL;
	int host_rpu_length_left = 0;

	if (!fmac_dev_ctx || !rpu_msg) {
		goto out;
	}

	umac_head = (unsigned char *)rpu_msg->msg;
	host_rpu_length_left = rpu_msg->hdr.len - sizeof(struct host_rpu_msg);

	while (host_rpu_length_left > 0) {
		status = nrf_wifi_fmac_data_event_process(fmac_dev_ctx,
							  umac_head);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: umac_process_data_event failed",
					      __func__);
			goto out;
		}

		host_rpu_length_left -= ((struct nrf_wifi_umac_head *)umac_head)->len;
		umac_head += ((struct nrf_wifi_umac_head *)umac_head)->len;
	}
out:
	return status;
}


#ifdef NRF71_STA_MODE
static void umac_event_connect(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       void *event_data)
{
	unsigned char if_index = 0;
	int peer_id = -1;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	struct nrf_wifi_umac_event_new_station *event = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	event = (struct nrf_wifi_umac_event_new_station *)event_data;

	if_index = event->umac_hdr.ids.wdev_id;

	vif_ctx = sys_dev_ctx->vif_ctx[if_index];
	if (if_index >= MAX_NUM_VIFS) {
		nrf_wifi_osal_log_err("%s: Invalid wdev_id recd from UMAC %d",
				      __func__,
				      if_index);
		return;
	}

	if (event->umac_hdr.cmd_evnt == NRF_WIFI_UMAC_EVENT_NEW_STATION) {
		if (vif_ctx->if_type == 2) {
			nrf_wifi_osal_mem_cpy(vif_ctx->bssid,
					      event->mac_addr,
					      NRF_WIFI_ETH_ADDR_LEN);
		}
		peer_id = nrf_wifi_fmac_peer_get_id(fmac_dev_ctx, event->mac_addr);

		if (peer_id == -1) {
			peer_id = nrf_wifi_fmac_peer_add(fmac_dev_ctx,
							 if_index,
							 event->mac_addr,
							 event->is_sta_legacy,
							 event->wme);

			if (peer_id == -1) {
				nrf_wifi_osal_log_err("%s:Can't add new station.",
						      __func__);
				return;
			}
		}
	} else if (event->umac_hdr.cmd_evnt == NRF_WIFI_UMAC_EVENT_DEL_STATION) {
		peer_id = nrf_wifi_fmac_peer_get_id(fmac_dev_ctx, event->mac_addr);
		if (peer_id != -1) {
			nrf_wifi_fmac_peer_remove(fmac_dev_ctx,
						  if_index,
						  peer_id);
		}
	}

	return;

}
#endif /* NRF71_STA_MODE */


static enum nrf_wifi_status umac_event_ctrl_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						    void *event_data,
						    unsigned int event_len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_SUCCESS;
	struct nrf_wifi_umac_hdr *umac_hdr = NULL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	struct nrf_wifi_fmac_callbk_fns *callbk_fns = NULL;
	struct nrf_wifi_umac_event_vif_state *evnt_vif_state = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	bool more_res = false;
	unsigned char if_id = 0;
	unsigned int event_num = 0;

	if (!fmac_dev_ctx || !event_data) {
		nrf_wifi_osal_log_err("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!sys_fpriv || !sys_dev_ctx) {
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

	vif_ctx = sys_dev_ctx->vif_ctx[if_id];
	if (!vif_ctx) {
		nrf_wifi_osal_log_err("%s: Invalid vif_ctx",
				      __func__);
		goto out;
	}
	callbk_fns = &sys_fpriv->callbk_fns;
	if (!callbk_fns) {
		nrf_wifi_osal_log_err("%s: Invalid callbk_fns",
				      __func__);
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
		if (callbk_fns->event_get_reg) {
			callbk_fns->event_get_reg(vif_ctx->os_vif_ctx,
						  event_data,
						  event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_REG_CHANGE:
		if (callbk_fns->reg_change_callbk_fn) {
			callbk_fns->reg_change_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_TRIGGER_SCAN_START:
		if (callbk_fns->scan_start_callbk_fn) {
			callbk_fns->scan_start_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_SCAN_DONE:
		if (callbk_fns->scan_done_callbk_fn) {
			callbk_fns->scan_done_callbk_fn(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_SCAN_ABORTED:
		if (callbk_fns->scan_abort_callbk_fn) {
			callbk_fns->scan_abort_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_IFFLAGS_STATUS:
		evnt_vif_state = (struct nrf_wifi_umac_event_vif_state *)event_data;

		if (evnt_vif_state->status < 0) {
			nrf_wifi_osal_log_err("%s: Failed to set interface flags: %d",
					      __func__,
						evnt_vif_state->status);
			goto out;
		}
		vif_ctx->ifflags = true;
		break;
#if defined(NRF71_STA_MODE) || defined(NRF71_RAW_DATA_RX)
	case NRF_WIFI_UMAC_EVENT_SET_INTERFACE:
		if (callbk_fns->set_if_callbk_fn) {
			callbk_fns->set_if_callbk_fn(vif_ctx->os_vif_ctx,
						     event_data,
						     event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
#endif
#ifdef NRF71_STA_MODE
	case NRF_WIFI_UMAC_EVENT_TWT_SLEEP:
		if (callbk_fns->twt_sleep_callbk_fn) {
			callbk_fns->twt_sleep_callbk_fn(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_SCAN_RESULT:
		if (umac_hdr->seq != 0) {
			more_res = true;
		}

		if (callbk_fns->scan_res_callbk_fn) {
			callbk_fns->scan_res_callbk_fn(vif_ctx->os_vif_ctx,
						       event_data,
						       event_len,
						       more_res);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_AUTHENTICATE:
		if (callbk_fns->auth_resp_callbk_fn) {
			callbk_fns->auth_resp_callbk_fn(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_ASSOCIATE:
		if (callbk_fns->assoc_resp_callbk_fn) {
			callbk_fns->assoc_resp_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_DEAUTHENTICATE:
		if (callbk_fns->deauth_callbk_fn) {
			callbk_fns->deauth_callbk_fn(vif_ctx->os_vif_ctx,
						     event_data,
						     event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_DISASSOCIATE:
		if (callbk_fns->disassoc_callbk_fn) {
			callbk_fns->disassoc_callbk_fn(vif_ctx->os_vif_ctx,
						       event_data,
						       event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_FRAME:
		if (callbk_fns->mgmt_rx_callbk_fn) {
			callbk_fns->mgmt_rx_callbk_fn(vif_ctx->os_vif_ctx,
						      event_data,
						      event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_GET_TX_POWER:
		if (callbk_fns->tx_pwr_get_callbk_fn) {
			callbk_fns->tx_pwr_get_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_GET_CHANNEL:
		if (callbk_fns->chnl_get_callbk_fn) {
			callbk_fns->chnl_get_callbk_fn(vif_ctx->os_vif_ctx,
						       event_data,
						       event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_GET_STATION:
		if (callbk_fns->get_station_callbk_fn) {
			callbk_fns->get_station_callbk_fn(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_NEW_INTERFACE:
		if (callbk_fns->get_interface_callbk_fn) {
			callbk_fns->get_interface_callbk_fn(vif_ctx->os_vif_ctx,
							    event_data,
							    event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_COOKIE_RESP:
		if (callbk_fns->cookie_rsp_callbk_fn) {
			callbk_fns->cookie_rsp_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_FRAME_TX_STATUS:
		if (callbk_fns->mgmt_tx_status) {
			callbk_fns->mgmt_tx_status(vif_ctx->os_vif_ctx,
						   event_data,
						   event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_UNPROT_DEAUTHENTICATE:
	case NRF_WIFI_UMAC_EVENT_UNPROT_DISASSOCIATE:
		if (callbk_fns->unprot_mlme_mgmt_rx_callbk_fn) {
			callbk_fns->unprot_mlme_mgmt_rx_callbk_fn(vif_ctx->os_vif_ctx,
								  event_data,
								  event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_CONFIG_TWT:
		if (callbk_fns->twt_config_callbk_fn) {
			callbk_fns->twt_config_callbk_fn(vif_ctx->os_vif_ctx,
							event_data,
							event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_TEARDOWN_TWT:
		if (callbk_fns->twt_teardown_callbk_fn) {
			callbk_fns->twt_teardown_callbk_fn(vif_ctx->os_vif_ctx,
							   event_data,
							   event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_NEW_WIPHY:
		if (callbk_fns->event_get_wiphy) {
			callbk_fns->event_get_wiphy(vif_ctx->os_vif_ctx,
						    event_data,
						    event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_CMD_STATUS:
#if WIFI_NRF71_LOG_LEVEL >= NRF_WIFI_LOG_LEVEL_DBG
		struct nrf_wifi_umac_event_cmd_status *cmd_status =
			(struct nrf_wifi_umac_event_cmd_status *)event_data;
#endif
		nrf_wifi_osal_log_dbg("%s: Command %d -> status %d",
				      __func__,
				      cmd_status->cmd_id,
				      cmd_status->cmd_status);
		break;
	case NRF_WIFI_UMAC_EVENT_BEACON_HINT:
	case NRF_WIFI_UMAC_EVENT_CONNECT:
	case NRF_WIFI_UMAC_EVENT_DISCONNECT:
		/* Nothing to be done */
		break;
	case NRF_WIFI_UMAC_EVENT_GET_POWER_SAVE_INFO:
		if (callbk_fns->event_get_ps_info) {
			callbk_fns->event_get_ps_info(vif_ctx->os_vif_ctx,
						      event_data,
						      event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
#ifdef NRF71_STA_MODE
	case NRF_WIFI_UMAC_EVENT_NEW_STATION:
	case NRF_WIFI_UMAC_EVENT_DEL_STATION:
		umac_event_connect(fmac_dev_ctx,
				   event_data);
		break;
#endif /* NRF71_STA_MODE */
#ifdef NRF71_P2P_MODE
	case NRF_WIFI_UMAC_EVENT_REMAIN_ON_CHANNEL:
		if (callbk_fns->roc_callbk_fn) {
			callbk_fns->roc_callbk_fn(vif_ctx->os_vif_ctx,
						  event_data,
						  event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
	case NRF_WIFI_UMAC_EVENT_CANCEL_REMAIN_ON_CHANNEL:
		if (callbk_fns->roc_cancel_callbk_fn) {
			callbk_fns->roc_cancel_callbk_fn(vif_ctx->os_vif_ctx,
							 event_data,
							 event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
#endif /* NRF71_P2P_MODE */
	case NRF_WIFI_UMAC_EVENT_GET_CONNECTION_INFO:
		if (callbk_fns->get_conn_info_callbk_fn) {
			callbk_fns->get_conn_info_callbk_fn(vif_ctx->os_vif_ctx,
							    event_data,
							    event_len);
		} else {
			nrf_wifi_osal_log_err("%s: No callback registered for event %d",
					      __func__,
					      umac_hdr->cmd_evnt);
		}
		break;
#endif /* NRF71_STA_MODE */
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


enum nrf_wifi_status nrf_wifi_sys_fmac_event_callback(void *mac_dev_ctx,
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
	case NRF_WIFI_HOST_RPU_MSG_TYPE_DATA:
		status = nrf_wifi_fmac_data_events_process(fmac_dev_ctx,
							   rpu_msg);
		break;
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
		status = umac_event_sys_proc_events(fmac_dev_ctx,
						    rpu_msg);
		break;
	default:
		goto out;
	}

out:
	return status;
}
