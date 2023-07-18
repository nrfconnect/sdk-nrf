/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing RX data path specific function definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "hal_api.h"
#include "fmac_rx.h"
#include "fmac_util.h"


static enum wifi_nrf_status
wifi_nrf_fmac_map_desc_to_pool(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			       unsigned int desc_id,
			       struct wifi_nrf_fmac_rx_pool_map_info *pool_info)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int pool_id = 0;

	for (pool_id = 0; pool_id < MAX_NUM_OF_RX_QUEUES; pool_id++) {
		if ((desc_id >= fmac_dev_ctx->fpriv->rx_desc[pool_id]) &&
		    (desc_id < (fmac_dev_ctx->fpriv->rx_desc[pool_id] +
				fmac_dev_ctx->fpriv->rx_buf_pools[pool_id].num_bufs))) {
			pool_info->pool_id = pool_id;
			pool_info->buf_id = (desc_id - fmac_dev_ctx->fpriv->rx_desc[pool_id]);
			status = WIFI_NRF_STATUS_SUCCESS;
			goto out;
		}
	}
out:
	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_rx_cmd_send(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					       enum wifi_nrf_fmac_rx_cmd_type cmd_type,
					       unsigned int desc_id)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_buf_map_info *rx_buf_info = NULL;
	struct host_rpu_rx_buf_info rx_cmd;
	struct wifi_nrf_fmac_rx_pool_map_info pool_info;
	unsigned long nwb = 0;
	unsigned long nwb_data = 0;
	unsigned long phy_addr = 0;
	unsigned int buf_len = 0;

	status = wifi_nrf_fmac_map_desc_to_pool(fmac_dev_ctx,
						desc_id,
						&pool_info);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: wifi_nrf_fmac_map_desc_to_pool failed\n",
				      __func__);
		goto out;
	}

	rx_buf_info = &fmac_dev_ctx->rx_buf_info[desc_id];

	buf_len = fmac_dev_ctx->fpriv->rx_buf_pools[pool_info.pool_id].buf_sz + RX_BUF_HEADROOM;

	if (cmd_type == WIFI_NRF_FMAC_RX_CMD_TYPE_INIT) {
		if (rx_buf_info->mapped) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Init RX command called for already mapped RX buffer(%d)\n",
					      __func__,
					      desc_id);
			status = WIFI_NRF_STATUS_FAIL;
			goto out;
		}

		nwb = (unsigned long)wifi_nrf_osal_nbuf_alloc(fmac_dev_ctx->fpriv->opriv,
							      buf_len);

		if (!nwb) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: No space for allocating RX buffer\n",
					      __func__);
			status = WIFI_NRF_STATUS_FAIL;
			goto out;
		}

		nwb_data = (unsigned long)wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
								      (void *)nwb);

		*(unsigned int *)(nwb_data) = desc_id;

		phy_addr = wifi_nrf_hal_buf_map_rx(fmac_dev_ctx->hal_dev_ctx,
						   nwb_data,
						   buf_len,
						   pool_info.pool_id,
						   pool_info.buf_id);

		if (!phy_addr) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: wifi_nrf_hal_buf_map_rx failed\n",
					      __func__);
			status = WIFI_NRF_STATUS_FAIL;
			goto out;
		}

		rx_buf_info->nwb = nwb;
		rx_buf_info->mapped = true;

		wifi_nrf_osal_mem_set(fmac_dev_ctx->fpriv->opriv,
				      &rx_cmd,
				      0x0,
				      sizeof(rx_cmd));

		rx_cmd.addr = (unsigned int)phy_addr;

		status = wifi_nrf_hal_data_cmd_send(fmac_dev_ctx->hal_dev_ctx,
						    WIFI_NRF_HAL_MSG_TYPE_CMD_DATA_RX,
						    &rx_cmd,
						    sizeof(rx_cmd),
						    desc_id,
						    pool_info.pool_id);
	} else if (cmd_type == WIFI_NRF_FMAC_RX_CMD_TYPE_DEINIT) {
		/* TODO: Need to initialize a command and send it to LMAC
		 * when LMAC is capable of handling deinit command
		 */
		if (!rx_buf_info->mapped) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Deinit RX command called for unmapped RX buffer(%d)\n",
					      __func__,
					      desc_id);
			status = WIFI_NRF_STATUS_FAIL;
			goto out;
		}

		nwb_data = wifi_nrf_hal_buf_unmap_rx(fmac_dev_ctx->hal_dev_ctx,
						     0,
						     pool_info.pool_id,
						     pool_info.buf_id);

		if (!nwb_data) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: wifi_nrf_hal_buf_unmap_rx failed\n",
					      __func__);
			goto out;
		}

		wifi_nrf_osal_nbuf_free(fmac_dev_ctx->fpriv->opriv,
					(void *)rx_buf_info->nwb);
		rx_buf_info->nwb = 0;
		rx_buf_info->mapped = false;
		status = WIFI_NRF_STATUS_SUCCESS;
	} else {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unknown cmd_type (%d)\n",
				      __func__,
				      cmd_type);
		goto out;
	}
out:
	return status;
}


#ifdef CONFIG_NRF700X_RX_WQ_ENABLED
void wifi_nrf_fmac_rx_tasklet(void *data)
{
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = (struct wifi_nrf_fmac_dev_ctx *)data;
	struct nrf_wifi_rx_buff *config = NULL;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	config = (struct nrf_wifi_rx_buff *)wifi_nrf_utils_q_dequeue(
		fmac_dev_ctx->fpriv->opriv,
		fmac_dev_ctx->rx_tasklet_event_q);

	if (!config) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: No RX config available\n",
				      __func__);
		goto out;
	}

	status = wifi_nrf_fmac_rx_event_process(fmac_dev_ctx,
						config);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: wifi_nrf_fmac_rx_event_process failed\n",
				      __func__);
		goto out;
	}
out:
	wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       config);
}
#endif /* CONFIG_NRF700X_RX_WQ_ENABLED */

enum wifi_nrf_status wifi_nrf_fmac_rx_event_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
						    struct nrf_wifi_rx_buff *config)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	struct wifi_nrf_fmac_buf_map_info *rx_buf_info = NULL;
	struct wifi_nrf_fmac_rx_pool_map_info pool_info;
	void *nwb = NULL;
	void *nwb_data = NULL;
	unsigned int num_pkts = 0;
	unsigned int desc_id = 0;
	unsigned int i = 0;
	unsigned int pkt_len = 0;
#ifdef CONFIG_NRF700X_STA_MODE
	struct wifi_nrf_fmac_ieee80211_hdr hdr;
	unsigned short eth_type = 0;
	unsigned int size = 0;
#endif /* CONFIG_NRF700X_STA_MODE */

	vif_ctx = fmac_dev_ctx->vif_ctx[config->wdev_id];

#ifdef CONFIG_NRF700X_STA_MODE
	fmac_dev_ctx->fpriv->callbk_fns.process_rssi_from_rx(vif_ctx->os_vif_ctx,
							     config->signal);
#endif /* CONFIG_NRF700X_STA_MODE */
	num_pkts = config->rx_pkt_cnt;

	for (i = 0; i < num_pkts; i++) {
		desc_id = config->rx_buff_info[i].descriptor_id;
		pkt_len = config->rx_buff_info[i].rx_pkt_len;

		if (desc_id >= fmac_dev_ctx->fpriv->num_rx_bufs) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Invalid desc_id %d\n",
					      __func__,
					      desc_id);
			status = WIFI_NRF_STATUS_FAIL;
			goto out;
		}

		status = wifi_nrf_fmac_map_desc_to_pool(fmac_dev_ctx,
							desc_id,
							&pool_info);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: wifi_nrf_fmac_map_desc_to_pool failed\n",
					      __func__);
			goto out;
		}

		nwb_data = (void *)wifi_nrf_hal_buf_unmap_rx(fmac_dev_ctx->hal_dev_ctx,
							     pkt_len,
							     pool_info.pool_id,
							     pool_info.buf_id);

		if (!nwb_data) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: wifi_nrf_hal_buf_unmap_rx failed\n",
					      __func__);
			goto out;
		}

		rx_buf_info = &fmac_dev_ctx->rx_buf_info[desc_id];
		nwb = (void *)rx_buf_info->nwb;

		wifi_nrf_osal_nbuf_data_put(fmac_dev_ctx->fpriv->opriv,
					    nwb,
					    pkt_len + RX_BUF_HEADROOM);
		wifi_nrf_osal_nbuf_data_pull(fmac_dev_ctx->fpriv->opriv,
					     nwb,
					     RX_BUF_HEADROOM);
		nwb_data = wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
						       nwb);

		rx_buf_info->nwb = 0;
		rx_buf_info->mapped = false;

		if (config->rx_pkt_type == NRF_WIFI_RX_PKT_DATA) {
#ifdef CONFIG_NRF700X_STA_MODE
			switch (config->rx_buff_info[i].pkt_type) {
			case PKT_TYPE_MPDU:
				wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
						      &hdr,
						      nwb_data,
						      sizeof(struct wifi_nrf_fmac_ieee80211_hdr));

				eth_type = wifi_nrf_util_rx_get_eth_type(fmac_dev_ctx,
									 ((char *)nwb_data +
									  config->mac_header_len));

				size = config->mac_header_len +
					wifi_nrf_util_get_skip_header_bytes(eth_type);

				/* Remove hdr len and llc header/length */
				wifi_nrf_osal_nbuf_data_pull(fmac_dev_ctx->fpriv->opriv,
							     nwb,
							     size);

				wifi_nrf_util_convert_to_eth(fmac_dev_ctx,
							     nwb,
							     &hdr,
							     eth_type);
				break;
			case PKT_TYPE_MSDU_WITH_MAC:
				wifi_nrf_osal_nbuf_data_pull(fmac_dev_ctx->fpriv->opriv,
							     nwb,
							     config->mac_header_len);

				wifi_nrf_util_rx_convert_amsdu_to_eth(fmac_dev_ctx,
								      nwb);
				break;
			case PKT_TYPE_MSDU:
				wifi_nrf_util_rx_convert_amsdu_to_eth(fmac_dev_ctx,
								      nwb);
				break;
			default:
				wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
						      "%s: Invalid pkt_type=%d\n",
						      __func__,
						      (config->rx_buff_info[i].pkt_type));
				status = WIFI_NRF_STATUS_FAIL;
				goto out;
			}
			fmac_dev_ctx->fpriv->callbk_fns.rx_frm_callbk_fn(vif_ctx->os_vif_ctx,
									 nwb);
#endif /* CONFIG_NRF700X_STA_MODE */
		} else if (config->rx_pkt_type == NRF_WIFI_RX_PKT_BCN_PRB_RSP) {
#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
			fmac_dev_ctx->fpriv->callbk_fns.rx_bcn_prb_resp_callbk_fn(
							vif_ctx->os_vif_ctx,
							nwb,
							config->frequency,
							config->signal);
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
			wifi_nrf_osal_nbuf_free(fmac_dev_ctx->fpriv->opriv,
						nwb);
		} else {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Invalid frame type recd %d\n",
					      __func__,
					      config->rx_pkt_type);
			status = WIFI_NRF_STATUS_FAIL;
			goto out;
		}

		status = wifi_nrf_fmac_rx_cmd_send(fmac_dev_ctx,
						   WIFI_NRF_FMAC_RX_CMD_TYPE_INIT,
						   desc_id);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: wifi_nrf_fmac_rx_cmd_send failed\n",
					      __func__);
			goto out;
		}
	}
out:
	return status;
}
