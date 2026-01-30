/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing RX data path specific function definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "system/hal_api.h"
#include "system/fmac_rx.h"
#include "common/fmac_util.h"
#include "system/fmac_promisc.h"

#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
#include "system/fmac_api.h"
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */

static enum nrf_wifi_status
nrf_wifi_fmac_map_desc_to_pool(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				   unsigned int desc_id,
				   struct nrf_wifi_fmac_rx_pool_map_info *pool_info)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	unsigned int pool_id = 0;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	for (pool_id = 0; pool_id < MAX_NUM_OF_RX_QUEUES; pool_id++) {
		if ((desc_id >= sys_fpriv->rx_desc[pool_id]) &&
			(desc_id < (sys_fpriv->rx_desc[pool_id] +
				sys_fpriv->rx_buf_pools[pool_id].num_bufs))) {
			pool_info->pool_id = pool_id;
			pool_info->buf_id = (desc_id - sys_fpriv->rx_desc[pool_id]);
			status = NRF_WIFI_STATUS_SUCCESS;
			goto out;
		}
	}
out:
	return status;
}
#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
unsigned long nrf_wifi_fmac_get_rx_buf_map_addr(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						   unsigned int desc_id)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *sys_priv = NULL;
	struct nrf_wifi_fmac_buf_map_info *rx_buf_info = NULL;
	struct nrf_wifi_fmac_rx_pool_map_info pool_info;
	unsigned long nwb_data = 0;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	status = nrf_wifi_fmac_map_desc_to_pool(fmac_dev_ctx,
						desc_id,
						&pool_info);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_fmac_map_desc_to_pool failed\n", __func__);
		goto out;
	}

	rx_buf_info = &sys_dev_ctx->rx_buf_info[desc_id];
	if (rx_buf_info->mapped) {
		nwb_data = (unsigned long)nrf_wifi_osal_nbuf_data_get((void *)rx_buf_info->nwb);
		return nwb_data;
	}

	nrf_wifi_osal_log_err("%s: rx buffer not mapped  for desc_id= %d\n",
			      __func__,
			      desc_id);

out:
	return 0;
}
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */

#ifdef NRF71_STA_MODE
int nrf_wifi_get_skip_header_bytes(unsigned short eth_type)
{
	/* Ethernet-II snap header (RFC1042 for most EtherTypes) */
	unsigned char llc_header[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
	/* Bridge-Tunnel header (for EtherTypes ETH_P_AARP and ETH_P_IPX) */
	static unsigned char aarp_ipx_header[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8};
	int skip_header_bytes = 0;

	skip_header_bytes = sizeof(eth_type);

	if (eth_type == NRF_WIFI_FMAC_ETH_P_AARP ||
		eth_type == NRF_WIFI_FMAC_ETH_P_IPX) {
		skip_header_bytes += sizeof(aarp_ipx_header);
	} else if (eth_type >= NRF_WIFI_FMAC_ETH_P_802_3_MIN) {
		skip_header_bytes += sizeof(llc_header);
	}

	return skip_header_bytes;
}

static void nrf_wifi_convert_amsdu_to_eth(void *nwb)
{
	struct nrf_wifi_fmac_eth_hdr *ehdr = NULL;
	struct nrf_wifi_fmac_amsdu_hdr amsdu_hdr;
	unsigned int len = 0;
	unsigned short eth_type = 0;
	void *nwb_data = NULL;
	unsigned char amsdu_hdr_len = 0;

	amsdu_hdr_len = sizeof(struct nrf_wifi_fmac_amsdu_hdr);

	nrf_wifi_osal_mem_cpy(&amsdu_hdr,
				  nrf_wifi_osal_nbuf_data_get(nwb),
				  amsdu_hdr_len);

	nwb_data = (unsigned char *)nrf_wifi_osal_nbuf_data_get(nwb) + amsdu_hdr_len;

	eth_type = nrf_wifi_util_rx_get_eth_type(nwb_data);

	nrf_wifi_osal_nbuf_data_pull(nwb,
					 (amsdu_hdr_len +
					  nrf_wifi_get_skip_header_bytes(eth_type)));

	len = nrf_wifi_osal_nbuf_data_size(nwb);

	ehdr = (struct nrf_wifi_fmac_eth_hdr *)
		nrf_wifi_osal_nbuf_data_push(nwb,
						 sizeof(struct nrf_wifi_fmac_eth_hdr));

	nrf_wifi_osal_mem_cpy(ehdr->src,
				  amsdu_hdr.src,
				  NRF_WIFI_FMAC_ETH_ADDR_LEN);

	nrf_wifi_osal_mem_cpy(ehdr->dst,
				  amsdu_hdr.dst,
				  NRF_WIFI_FMAC_ETH_ADDR_LEN);

	if (eth_type >= NRF_WIFI_FMAC_ETH_P_802_3_MIN) {
		ehdr->proto = ((eth_type >> 8) | (eth_type << 8));
	} else {
		ehdr->proto = len;
	}
}

static void nrf_wifi_convert_to_eth(void *nwb,
					struct nrf_wifi_fmac_ieee80211_hdr *hdr,
					unsigned short eth_type)
{

	struct nrf_wifi_fmac_eth_hdr *ehdr = NULL;
	unsigned int len = 0;

	len = nrf_wifi_osal_nbuf_data_size(nwb);

	ehdr = (struct nrf_wifi_fmac_eth_hdr *)
		nrf_wifi_osal_nbuf_data_push(nwb,
						 sizeof(struct nrf_wifi_fmac_eth_hdr));

	switch (hdr->fc & (NRF_WIFI_FCTL_TODS | NRF_WIFI_FCTL_FROMDS)) {
	case (NRF_WIFI_FCTL_TODS | NRF_WIFI_FCTL_FROMDS):
		nrf_wifi_osal_mem_cpy(ehdr->src,
					  hdr->addr_4,
					  NRF_WIFI_FMAC_ETH_ADDR_LEN);

		nrf_wifi_osal_mem_cpy(ehdr->dst,
					  hdr->addr_1,
					  NRF_WIFI_FMAC_ETH_ADDR_LEN);
		break;
	case (NRF_WIFI_FCTL_FROMDS):
		nrf_wifi_osal_mem_cpy(ehdr->src,
					  hdr->addr_3,
					  NRF_WIFI_FMAC_ETH_ADDR_LEN);
		nrf_wifi_osal_mem_cpy(ehdr->dst,
					  hdr->addr_1,
					  NRF_WIFI_FMAC_ETH_ADDR_LEN);
		break;
	case (NRF_WIFI_FCTL_TODS):
		nrf_wifi_osal_mem_cpy(ehdr->src,
					  hdr->addr_2,
					  NRF_WIFI_FMAC_ETH_ADDR_LEN);
		nrf_wifi_osal_mem_cpy(ehdr->dst,
					  hdr->addr_3,
					  NRF_WIFI_FMAC_ETH_ADDR_LEN);
		break;
	default:
		/* Both FROM and TO DS bit is zero*/
		nrf_wifi_osal_mem_cpy(ehdr->src,
					  hdr->addr_2,
					  NRF_WIFI_FMAC_ETH_ADDR_LEN);
		nrf_wifi_osal_mem_cpy(ehdr->dst,
					  hdr->addr_1,
					  NRF_WIFI_FMAC_ETH_ADDR_LEN);

	}

	if (eth_type >= NRF_WIFI_FMAC_ETH_P_802_3_MIN) {
		ehdr->proto = ((eth_type >> 8) | (eth_type << 8));
	} else {
		ehdr->proto = len;
	}
}
#endif /* NRF71_STA_MODE */

enum nrf_wifi_status nrf_wifi_fmac_rx_cmd_send(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						   enum nrf_wifi_fmac_rx_cmd_type cmd_type,
						   unsigned int desc_id)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_buf_map_info *rx_buf_info = NULL;
	unsigned int rx_addr;
	struct nrf_wifi_fmac_rx_pool_map_info pool_info;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	unsigned long nwb = 0;
	unsigned long nwb_data = 0;
	unsigned int buf_len = 0;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	status = nrf_wifi_fmac_map_desc_to_pool(fmac_dev_ctx,
						desc_id,
						&pool_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_fmac_map_desc_to_pool failed",
					  __func__);
		goto out;
	}

	rx_buf_info = &sys_dev_ctx->rx_buf_info[desc_id];

	buf_len = sys_fpriv->rx_buf_pools[pool_info.pool_id].buf_sz + RX_BUF_HEADROOM;

	if (cmd_type == NRF_WIFI_FMAC_RX_CMD_TYPE_INIT) {
		if (rx_buf_info->mapped) {
			nrf_wifi_osal_log_err("%s: RX init called for mapped RX buffer(%d)",
						  __func__,
						  desc_id);
			status = NRF_WIFI_STATUS_FAIL;
			goto out;
		}

		nwb = (unsigned long)nrf_wifi_osal_nbuf_alloc(buf_len);

		if (!nwb) {
			nrf_wifi_osal_log_err("%s: No space for allocating RX buffer",
						  __func__);
			status = NRF_WIFI_STATUS_FAIL;
			goto out;
		}

		nwb_data = (unsigned long)nrf_wifi_osal_nbuf_data_get((void *)nwb);

		*(unsigned int *)(nwb_data) = desc_id;

		rx_addr = (unsigned int)nwb_data;
#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
		/**
		 * Do not map nwb_data to rx_buf_info here. Map nwb. Driver
		 * always maps from network buffer pointer. nwb->data pointer
		 * is offset from nwb pointer. nwb has length and other fields
		 * which are overwritten if nwb pointer is set to nwb->data and
		 * sent to Firmware particularly when firmware provides packet
		 * to driver for nrf71 on RX.
		 * TODO: If this feature is standalone and not only for nrf71,
		 * It needs to be relooked to map for nrf71 and other products
		 * properly.
		 */
		rx_buf_info->nwb =  (unsigned int)nwb;
		rx_buf_info->mapped = true;
#else
		status = nrf_wifi_sys_hal_data_cmd_send(fmac_dev_ctx->hal_dev_ctx,
							NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_RX,
							&rx_addr,
							sizeof(rx_addr),
							desc_id,
							pool_info.pool_id);
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */
	} else if (cmd_type == NRF_WIFI_FMAC_RX_CMD_TYPE_DEINIT) {

		nrf_wifi_osal_nbuf_free((void *)rx_buf_info->nwb);
		status = NRF_WIFI_STATUS_SUCCESS;
	} else {
		nrf_wifi_osal_log_err("%s: Unknown cmd_type (%d)",
					  __func__,
					  cmd_type);
		goto out;
	}
out:
	return status;
}


#ifdef NRF71_RX_WQ_ENABLED
void nrf_wifi_fmac_rx_tasklet(void *data)
{
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = (struct nrf_wifi_fmac_dev_ctx *)data;
	struct nrf_wifi_rx_buff *config = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	enum NRF_WIFI_HAL_STATUS hal_status;

	nrf_wifi_sys_hal_lock_rx(fmac_dev_ctx->hal_dev_ctx);
	hal_status = nrf_wifi_hal_status_unlocked(fmac_dev_ctx->hal_dev_ctx);
	if (hal_status != NRF_WIFI_HAL_STATUS_ENABLED) {
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	config = (struct nrf_wifi_rx_buff *)nrf_wifi_utils_q_dequeue(
		sys_dev_ctx->rx_tasklet_event_q);

	if (!config) {
		nrf_wifi_osal_log_err("%s: No RX config available",
					  __func__);
		goto out;
	}

	status = nrf_wifi_fmac_rx_event_process(fmac_dev_ctx,
						config);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_fmac_rx_event_process failed",
					  __func__);
		goto out;
	}
out:
	nrf_wifi_osal_mem_free(config);
	nrf_wifi_sys_hal_unlock_rx(fmac_dev_ctx->hal_dev_ctx);
}
#endif /* NRF71_RX_WQ_ENABLED */

enum nrf_wifi_status nrf_wifi_fmac_rx_event_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
							struct nrf_wifi_rx_buff *config)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	struct nrf_wifi_fmac_buf_map_info *rx_buf_info = NULL;
	struct nrf_wifi_fmac_rx_pool_map_info pool_info;
#if defined(NRF71_RAW_DATA_RX) || defined(NRF71_PROMISC_DATA_RX)
	struct raw_rx_pkt_header raw_rx_hdr;
#if defined(NRF71_PROMISC_DATA_RX)
	unsigned short frame_control;
#endif
#endif /* NRF71_RAW_DATA_RX || NRF71_PROMISC_DATA_RX */
	void *nwb = NULL;
	void *nwb_data = NULL;
	unsigned int num_pkts = 0;
	unsigned int desc_id = 0;
	unsigned int i = 0;
	unsigned int pkt_len = 0;
#ifdef NRF71_STA_MODE
	struct nrf_wifi_fmac_ieee80211_hdr hdr;
	unsigned short eth_type = 0;
	unsigned int size = 0;
#endif /* NRF71_STA_MODE */
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
	unsigned int buf_addr = 0;
	struct nrf_wifi_rx_buf *rx_buf_ipc = NULL, *rx_buf_info_iter = NULL;
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	vif_ctx = sys_dev_ctx->vif_ctx[config->wdev_id];

#ifdef NRF71_STA_MODE
	if (config->rx_pkt_type != NRF_WIFI_RAW_RX_PKT) {
		sys_fpriv->callbk_fns.process_rssi_from_rx(vif_ctx->os_vif_ctx,
							  config->signal);
	}
#endif /* NRF71_STA_MODE */
	num_pkts = config->rx_pkt_cnt;
#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
	rx_buf_ipc = nrf_wifi_osal_mem_zalloc(num_pkts * sizeof(struct nrf_wifi_rx_buf));
	rx_buf_info_iter = rx_buf_ipc;
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */

	for (i = 0; i < num_pkts; i++) {
		desc_id = config->rx_buff_info[i].descriptor_id;
		pkt_len = config->rx_buff_info[i].rx_pkt_len;

		if (desc_id >= sys_fpriv->num_rx_bufs) {
			nrf_wifi_osal_log_err("%s: Invalid desc_id %d",
						  __func__,
						  desc_id);
			status = NRF_WIFI_STATUS_FAIL;
			continue;
		}

		status = nrf_wifi_fmac_map_desc_to_pool(fmac_dev_ctx,
							desc_id,
							&pool_info);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: nrf_wifi_fmac_map_desc_to_pool failed",
						  __func__);
			status = NRF_WIFI_STATUS_FAIL;
			continue;
		}
		rx_buf_info = &sys_dev_ctx->rx_buf_info[desc_id];
		nwb = (void *)rx_buf_info->nwb;

		/**
		 * For nRF71 the RX_BUF_HEADROOM does not seem to be needed.
		 * Remove the same. What parameters does RX_BUF_HEADROOM have??
		 */
		nrf_wifi_osal_nbuf_data_put(nwb,
						pkt_len);
		nwb_data = nrf_wifi_osal_nbuf_data_get(nwb);
		rx_buf_info->nwb = 0;
		rx_buf_info->mapped = false;

#ifdef NRF71_PROMISC_DATA_RX
		nrf_wifi_osal_mem_cpy(&frame_control,
					  nwb_data,
					  sizeof(unsigned short));
#endif

		if (config->rx_pkt_type == NRF_WIFI_RX_PKT_DATA) {
#ifdef NRF71_PROMISC_DATA_RX
			if (vif_ctx->promisc_mode) {
				raw_rx_hdr.frequency = config->frequency;
				raw_rx_hdr.signal = config->signal;
				raw_rx_hdr.rate_flags = config->rate_flags;
				raw_rx_hdr.rate = config->rate;
				if (nrf_wifi_util_check_filt_setting(vif_ctx, &frame_control)) {
					sys_fpriv->callbk_fns.sniffer_callbk_fn(vif_ctx->os_vif_ctx,
										   nwb,
										   &raw_rx_hdr,
										   false);
				}
			}
#endif
#ifdef NRF71_STA_MODE
			switch (config->rx_buff_info[i].pkt_type) {
			case PKT_TYPE_MPDU:
				nrf_wifi_osal_mem_cpy(&hdr,
					nwb_data,
					sizeof(struct nrf_wifi_fmac_ieee80211_hdr));

				eth_type = nrf_wifi_util_rx_get_eth_type(
					((char *)nwb_data + config->mac_header_len));

				size = config->mac_header_len +
					nrf_wifi_get_skip_header_bytes(eth_type);

				/* Remove hdr len and llc header/length */
				nrf_wifi_osal_nbuf_data_pull(nwb,
								 size);

				nrf_wifi_convert_to_eth(nwb,
							&hdr,
							eth_type);
				break;
			case PKT_TYPE_MSDU_WITH_MAC:
				nrf_wifi_osal_nbuf_data_pull(nwb,
								 config->mac_header_len);

				nrf_wifi_convert_amsdu_to_eth(nwb);
				break;
			case PKT_TYPE_MSDU:
				nrf_wifi_convert_amsdu_to_eth(nwb);
				break;
			default:
				nrf_wifi_osal_log_err("%s: Invalid pkt_type=%d",
							  __func__,
							  (config->rx_buff_info[i].pkt_type));
				status = NRF_WIFI_STATUS_FAIL;
				continue;
			}
			sys_fpriv->callbk_fns.rx_frm_callbk_fn(vif_ctx->os_vif_ctx,
									 nwb);
#endif /* NRF71_STA_MODE */
		} else if (config->rx_pkt_type == NRF_WIFI_RX_PKT_BCN_PRB_RSP) {
#ifdef WIFI_MGMT_RAW_SCAN_RESULTS
			sys_fpriv->callbk_fns.rx_bcn_prb_resp_callbk_fn(
							vif_ctx->os_vif_ctx,
							nwb,
							config->frequency,
							config->signal);
#endif /* WIFI_MGMT_RAW_SCAN_RESULTS */
			nrf_wifi_osal_nbuf_free(nwb);
#ifdef NRF_WIFI_MGMT_BUFF_OFFLOAD
			continue;
#endif /* NRF_WIFI_MGMT_BUFF_OFFLOAD */
		}
#if defined(NRF71_RAW_DATA_RX) || defined(NRF71_PROMISC_DATA_RX)
		else if (config->rx_pkt_type == NRF_WIFI_RAW_RX_PKT) {
			raw_rx_hdr.frequency = config->frequency;
			raw_rx_hdr.signal = config->signal;
			raw_rx_hdr.rate_flags = config->rate_flags;
			raw_rx_hdr.rate = config->rate;
#if defined(NRF71_PROMISC_DATA_RX)
			if (nrf_wifi_util_check_filt_setting(vif_ctx, &frame_control)) {
#endif
				sys_fpriv->callbk_fns.sniffer_callbk_fn(vif_ctx->os_vif_ctx,
						nwb,
						&raw_rx_hdr,
						true);
#if defined(NRF71_PROMISC_DATA_RX)
			}
#endif
#if defined(NRF71_PROMISC_DATA_RX)
			/**
			 * In the case of Monitor mode, the sniffer callback function
			 * will free the packet. For promiscuous mode, if the packet
			 * is not meant to be sent up the stack, the packet needs
			 * to be freed here.
			 */
			else {
				nrf_wifi_osal_nbuf_free(nwb);
			}
#endif
		}
#endif /* NRF71_RAW_DATA_RX || NRF71_PROMISC_DATA_RX */
		else {
			nrf_wifi_osal_log_err("%s: Invalid frame type received %d",
						  __func__,
						  config->rx_pkt_type);
			status = NRF_WIFI_STATUS_FAIL;
			nrf_wifi_osal_nbuf_free(nwb);
			continue;
		}

		status = nrf_wifi_fmac_rx_cmd_send(fmac_dev_ctx,
						   NRF_WIFI_FMAC_RX_CMD_TYPE_INIT,
						   desc_id);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err("%s: nrf_wifi_fmac_rx_cmd_send failed",
						  __func__);
			continue;
		}
#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
		buf_addr = (unsigned int) nrf_wifi_fmac_get_rx_buf_map_addr(fmac_dev_ctx, desc_id);
		if (buf_addr) {
			rx_buf_info_iter->skb_pointer = buf_addr;
			rx_buf_info_iter->skb_desc_no = desc_id;
			rx_buf_info_iter++;
		} else {
			nrf_wifi_osal_log_err("%s: UMAC rx buff not mapped for desc_id = %d\n",
					      __func__, desc_id);
			status = NRF_WIFI_STATUS_FAIL;
			continue;
		}
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */
	}
#ifdef NRF_WIFI_RX_BUFF_PROG_UMAC
	status = nrf_wifi_fmac_prog_rx_buf_info(fmac_dev_ctx,
						rx_buf_ipc,
						num_pkts);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: UMAC rx buff programming failed\n",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
	} else {
		nrf_wifi_osal_log_dbg("%s: UMAC rx buff refill programmed for num_buffs= %d\n",
				      __func__, num_pkts);
		nrf_wifi_osal_mem_free(rx_buf_ipc);
	}
#endif /*NRF_WIFI_RX_BUFF_PROG_UMAC */
	return status;
}
