/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing TX data path specific function definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "list.h"
#include "queue.h"
#include "hal_api.h"
#include "fmac_tx.h"
#include "fmac_api.h"
#include "fmac_peer.h"
#include "hal_mem.h"
#include "fmac_util.h"

/* Set the coresponding bit of access category.
 * First 4 bits(0 to 3) represenst first spare desc access cateogories
 * Second 4 bits(4 to 7) represenst second spare desc access cateogories and so on
 */
static void set_spare_desc_q_map(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				 unsigned int desc,
				 int tx_done_q)
{
	unsigned short spare_desc_indx = 0;

	spare_desc_indx = (desc % (fmac_dev_ctx->fpriv->num_tx_tokens_per_ac *
				   WIFI_NRF_FMAC_AC_MAX));

	fmac_dev_ctx->tx_config.spare_desc_queue_map |=
		(1 << ((spare_desc_indx * SPARE_DESC_Q_MAP_SIZE) + tx_done_q));
}


/* Clear the coresponding bit of access category.
 * First 4 bits(0 to 3) represenst first spare desc access cateogories
 * Second 4 bits(4 to 7) represenst second spare desc access cateogories and so on
 */
static void clear_spare_desc_q_map(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				   unsigned int desc,
				   int tx_done_q)
{
	unsigned short spare_desc_indx = 0;

	spare_desc_indx = (desc % (fmac_dev_ctx->fpriv->num_tx_tokens_per_ac *
				   WIFI_NRF_FMAC_AC_MAX));

	fmac_dev_ctx->tx_config.spare_desc_queue_map &=
		~(1 << ((spare_desc_indx * SPARE_DESC_Q_MAP_SIZE) + tx_done_q));
}

/*Get the spare descriptor queue map */
static unsigned short get_spare_desc_q_map(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					   unsigned int desc)
{
	unsigned short spare_desc_indx = 0;

	spare_desc_indx = (desc % (fmac_dev_ctx->fpriv->num_tx_tokens_per_ac *
				   WIFI_NRF_FMAC_AC_MAX));

	return	(fmac_dev_ctx->tx_config.spare_desc_queue_map >> (spare_desc_indx *
			SPARE_DESC_Q_MAP_SIZE)) & 0x000F;
}


int pending_frames_count(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			 int peer_id)
{
	int count = 0;
	int ac = 0;
	void *queue = NULL;

	for (ac = WIFI_NRF_FMAC_AC_VO; ac >= 0; --ac) {
		queue = fmac_dev_ctx->tx_config.data_pending_txq[peer_id][ac];
		count += wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv, queue);
	}

	return count;
}


enum wifi_nrf_status update_pend_q_bmp(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				       unsigned int ac,
				       int peer_id)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	void *pend_pkt_q = NULL;
	int len = 0;
	unsigned char vif_id = 0;
	unsigned char *bmp = NULL;

	if (!fmac_dev_ctx) {
		goto out;
	}

	vif_id = fmac_dev_ctx->tx_config.peers[peer_id].if_idx;
	vif_ctx = fmac_dev_ctx->vif_ctx[vif_id];

	if (vif_ctx->if_type == NRF_WIFI_IFTYPE_AP &&
	    peer_id < MAX_PEERS) {
		bmp = &fmac_dev_ctx->tx_config.peers[peer_id].pend_q_bmp;
		pend_pkt_q = fmac_dev_ctx->tx_config.data_pending_txq[peer_id][ac];

		len = wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv, pend_pkt_q);

		if (len == 0) {
			*bmp = *bmp & ~(1 << ac);
		} else {
			*bmp = *bmp | (1 << ac);
		}

		status = hal_rpu_mem_write(fmac_dev_ctx->hal_dev_ctx,
					   (RPU_MEM_UMAC_PEND_Q_BMP +
					    (sizeof(struct sap_pend_frames_bitmap) * peer_id) +
					    WIFI_NRF_FMAC_ETH_ADDR_LEN),
					   bmp,
					   sizeof(unsigned char));
	} else {
		status = WIFI_NRF_STATUS_SUCCESS;
	}
out:
	return status;
}


void tx_desc_free(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
		  unsigned int desc,
		  int queue)
{
	struct wifi_nrf_fmac_priv *fpriv = NULL;
	int bit = -1;
	int pool_id = -1;

	fpriv = fmac_dev_ctx->fpriv;

	bit = (desc % TX_DESC_BUCKET_BOUND);
	pool_id = (desc / TX_DESC_BUCKET_BOUND);

	if (!(fmac_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] & (1 << bit))) {
		return;
	}

	fmac_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] &= (~(1 << bit));

	fmac_dev_ctx->tx_config.outstanding_descs[queue]--;

	if (desc >= (fpriv->num_tx_tokens_per_ac * WIFI_NRF_FMAC_AC_MAX)) {
		clear_spare_desc_q_map(fmac_dev_ctx, desc, queue);
	}

}


unsigned int tx_desc_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			 int queue)
{
	struct wifi_nrf_fmac_priv *fpriv = NULL;
	unsigned int cnt = 0;
	int curr_bit = 0;
	unsigned int desc = 0;
	int pool_id = 0;

	fpriv = fmac_dev_ctx->fpriv;

	desc = fpriv->num_tx_tokens;

	/* First search for a reserved desc */

	for (cnt = 0; cnt < fpriv->num_tx_tokens_per_ac; cnt++) {
		curr_bit = ((queue + (WIFI_NRF_FMAC_AC_MAX * cnt)));
		curr_bit = ((queue + (WIFI_NRF_FMAC_AC_MAX * cnt)) % TX_DESC_BUCKET_BOUND);
		pool_id = ((queue + (WIFI_NRF_FMAC_AC_MAX * cnt)) / TX_DESC_BUCKET_BOUND);

		if ((((fmac_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] >>
		       curr_bit)) & 1)) {
			continue;
		} else {
			fmac_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] |=
				(1 << curr_bit);
			desc = queue + (WIFI_NRF_FMAC_AC_MAX * cnt);
			fmac_dev_ctx->tx_config.outstanding_descs[queue]++;
			break;
		}
	}

	/* If reserved desc is not found search for a spare desc
	 * (only for non beacon queues)
	 */
	if (cnt == fpriv->num_tx_tokens_per_ac) {
		for (desc = fpriv->num_tx_tokens_per_ac * WIFI_NRF_FMAC_AC_MAX;
		     desc < fpriv->num_tx_tokens;
		     desc++) {
			curr_bit = (desc % TX_DESC_BUCKET_BOUND);
			pool_id = (desc / TX_DESC_BUCKET_BOUND);

			if ((fmac_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] >> curr_bit) & 1) {
				continue;
			} else {
				fmac_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] |=
					(1 << curr_bit);
				fmac_dev_ctx->tx_config.outstanding_descs[queue]++;
				/* Keep a note which queue has been assigned the
				 * spare desc. Need for processing of TX_DONE
				 * event as queue number is not being provided
				 * by UMAC.
				 * First nibble epresent first spare desc
				 * (B3B2B1B0: VO-VI-BE-BK)
				 * Second nibble represent second spare desc
				 * (B7B6B5B4 : V0-VI-BE-BK)
				 * Third nibble represent second spare desc
				 * (B11B10B9B8 : V0-VI-BE-BK)
				 * Fourth nibble represent second spare desc
				 * (B15B14B13B12 : V0-VI-BE-BK)
				 */
				set_spare_desc_q_map(fmac_dev_ctx, desc, queue);
				break;
			}
		}
	}


	return desc;
}


int tx_aggr_check(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
		  void *first_nwb,
		  int ac,
		  int peer)
{
	void *nwb = NULL;
	void *pending_pkt_queue = NULL;
	bool aggr = true;

	if (fmac_dev_ctx->tx_config.peers[peer].is_legacy) {
		return false;
	}

	pending_pkt_queue = fmac_dev_ctx->tx_config.data_pending_txq[peer][ac];

	if (wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv,
				 pending_pkt_queue) == 0) {
		return false;
	}

	nwb = wifi_nrf_utils_q_peek(fmac_dev_ctx->fpriv->opriv,
				    pending_pkt_queue);

	if (nwb) {
		if (!wifi_nrf_util_ether_addr_equal(wifi_nrf_util_get_dest(fmac_dev_ctx,
									   nwb),
						    wifi_nrf_util_get_dest(fmac_dev_ctx,
									   first_nwb))) {
			aggr = false;
		}

		if (!wifi_nrf_util_ether_addr_equal(wifi_nrf_util_get_src(fmac_dev_ctx,
									  nwb),
						    wifi_nrf_util_get_src(fmac_dev_ctx,
									  first_nwb))) {
			aggr = false;
		}
	}


	return aggr;
}


int get_peer_from_wakeup_q(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			   unsigned int ac)
{
	int peer_id = -1;
	struct peers_info *peer = NULL;
	void *pend_q = NULL;
	unsigned int pend_q_len;
	void *client_q = NULL;
	void *list_node = NULL;

	client_q = fmac_dev_ctx->tx_config.wakeup_client_q;

	list_node = wifi_nrf_osal_llist_get_node_head(fmac_dev_ctx->fpriv->opriv,
						      client_q);

	while (list_node) {
		peer = wifi_nrf_osal_llist_node_data_get(fmac_dev_ctx->fpriv->opriv,
							 list_node);

		if (peer != NULL && peer->ps_token_count) {

			pend_q = fmac_dev_ctx->tx_config.data_pending_txq[peer->peer_id][ac];
			pend_q_len = wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv, pend_q);

			if (pend_q_len) {
				peer->ps_token_count--;
				return peer->peer_id;
			}
		}

		list_node = wifi_nrf_osal_llist_get_node_nxt(fmac_dev_ctx->fpriv->opriv,
							     client_q,
							     list_node);
	}

	return peer_id;
}


int tx_curr_peer_opp_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			 unsigned int ac)
{

	unsigned int i = 0;
	unsigned int curr_peer_opp = 0;
	unsigned int init_peer_opp = 0;
	unsigned int pend_q_len;
	void *pend_q = NULL;
	int peer_id = -1;
	unsigned char ps_state = 0;

	if (ac == WIFI_NRF_FMAC_AC_MC) {
		return MAX_PEERS;
	}

	peer_id = get_peer_from_wakeup_q(fmac_dev_ctx, ac);

	if (peer_id != -1) {
		return peer_id;
	}

	init_peer_opp = fmac_dev_ctx->tx_config.curr_peer_opp[ac];

	for (i = 0; i < MAX_PEERS; i++) {
		curr_peer_opp = (init_peer_opp + i) % MAX_PEERS;

		ps_state = fmac_dev_ctx->tx_config.peers[curr_peer_opp].ps_state;

		if (ps_state == NRF_WIFI_CLIENT_PS_MODE) {
			continue;
		}

		pend_q = fmac_dev_ctx->tx_config.data_pending_txq[curr_peer_opp][ac];
		pend_q_len = wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv,
						  pend_q);

		if (pend_q_len) {
			fmac_dev_ctx->tx_config.curr_peer_opp[ac] =
				(curr_peer_opp + 1) % MAX_PEERS;
			break;
		}
	}

	if (i != MAX_PEERS) {
		peer_id = curr_peer_opp;
	}

	return peer_id;
}

int _tx_pending_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			unsigned int desc,
			unsigned int ac)
{
	int len = 0;
	void *pend_pkt_q = NULL;
	void *txq = NULL;
	struct tx_pkt_info *pkt_info = NULL;
	int peer_id = -1;
	void *nwb = NULL;
	void *first_nwb = NULL;
	int max_txq_len = fmac_dev_ctx->fpriv->data_config.max_tx_aggregation;

	peer_id = tx_curr_peer_opp_get(fmac_dev_ctx, ac);

	/* No pending frames for any peer in that AC. */
	if (peer_id == -1) {
		return 0;
	}

	pend_pkt_q = fmac_dev_ctx->tx_config.data_pending_txq[peer_id][ac];

	if (wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv,
				 pend_pkt_q) == 0) {
		return 0;
	}

	pkt_info = &fmac_dev_ctx->tx_config.pkt_info_p[desc];
	txq = pkt_info->pkt;

	/* Aggregate Only MPDU's with same RA, same Rate,
	 * same Rate flags, same Tx Info flags
	 */
	if (wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv,
				 pend_pkt_q)) {
		first_nwb = wifi_nrf_utils_q_peek(fmac_dev_ctx->fpriv->opriv,
						  pend_pkt_q);
	}

	while (wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv,
				    pend_pkt_q)) {
		nwb = wifi_nrf_utils_q_peek(fmac_dev_ctx->fpriv->opriv,
					    pend_pkt_q);

		if ((!tx_aggr_check(fmac_dev_ctx,
				    first_nwb,
				    ac,
				    peer_id) ||
		     (wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv,
					   txq)) >= max_txq_len)) {
			break;
		}

		nwb = wifi_nrf_utils_q_dequeue(fmac_dev_ctx->fpriv->opriv,
					       pend_pkt_q);

		wifi_nrf_utils_list_add_tail(fmac_dev_ctx->fpriv->opriv,
					     txq,
					     nwb);
	}

	/* If our criterion rejects all pending frames, or
	 * pend_q is empty, send only 1
	 */
	if (!wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv,
				  txq)) {
		nwb = wifi_nrf_utils_q_dequeue(fmac_dev_ctx->fpriv->opriv,
					       pend_pkt_q);

		wifi_nrf_utils_list_add_tail(fmac_dev_ctx->fpriv->opriv,
					     txq,
					     nwb);
	}

	len = wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv, txq);

	if (len > 0) {
		fmac_dev_ctx->tx_config.pkt_info_p[desc].peer_id = peer_id;
	}


	update_pend_q_bmp(fmac_dev_ctx, ac, peer_id);

	return len;
}


enum wifi_nrf_status tx_cmd_prep_callbk_fn(void *callbk_data,
					   void *nbuf)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_buf_map_info *tx_buf_info = NULL;
	unsigned long nwb = 0;
	unsigned long nwb_data = 0;
	unsigned long phy_addr = 0;
	struct tx_cmd_prep_info *info = NULL;
	struct nrf_wifi_tx_buff *config = NULL;
	unsigned int desc_id = 0;
	unsigned int buf_len = 0;

	info = (struct tx_cmd_prep_info *)callbk_data;
	fmac_dev_ctx = info->fmac_dev_ctx;
	config = info->config;

	nwb = (unsigned long)nbuf;

	desc_id = (config->tx_desc_num *
		   fmac_dev_ctx->fpriv->data_config.max_tx_aggregation) + config->num_tx_pkts;

	tx_buf_info = &fmac_dev_ctx->tx_buf_info[desc_id];

	if (tx_buf_info->mapped) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Init_TX cmd called for already mapped TX buffer(%d)\n",
				      __func__,
				      desc_id);

		status = WIFI_NRF_STATUS_FAIL;
		goto out;
	}

	nwb_data = (unsigned long)wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
							      (void *)nwb);

	buf_len = wifi_nrf_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
					       (void *)nwb);

	phy_addr = wifi_nrf_hal_buf_map_tx(fmac_dev_ctx->hal_dev_ctx,
					   nwb_data,
					   buf_len,
					   desc_id);

	if (!phy_addr) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: wifi_nrf_hal_buf_map_tx failed\n",
				      __func__);
		status = WIFI_NRF_STATUS_FAIL;
		goto out;
	}

	tx_buf_info->nwb = nwb;
	tx_buf_info->mapped = true;

	config->tx_buff_info[config->num_tx_pkts].ddr_ptr =
		(unsigned long long)phy_addr;

	config->tx_buff_info[config->num_tx_pkts].pkt_length = buf_len;
	config->num_tx_pkts++;

	status = WIFI_NRF_STATUS_SUCCESS;
out:
	return status;
}


int tx_cmd_prepare(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
		   struct host_rpu_msg *umac_cmd,
		   int desc,
		   void *txq,
		   int peer_id)
{
	struct nrf_wifi_tx_buff *config = NULL;
	int len = 0;
	void *nwb = NULL;
	void *nwb_data = NULL;
	unsigned int txq_len = 0;
	unsigned int max_txq_len = 0;
	unsigned char *data = NULL;
	struct tx_cmd_prep_info info;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned char vif_id = fmac_dev_ctx->tx_config.peers[peer_id].if_idx;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = fmac_dev_ctx->vif_ctx[vif_id];

	txq_len = wifi_nrf_utils_list_len(fmac_dev_ctx->fpriv->opriv,
					  txq);

	max_txq_len = fmac_dev_ctx->fpriv->data_config.max_tx_aggregation;

	if (txq_len == 0 || txq_len > max_txq_len) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: txq_len = %d\n",
				      __func__,
				      txq_len);
		return -1;
	}

	nwb = wifi_nrf_utils_list_peek(fmac_dev_ctx->fpriv->opriv,
				       txq);

	fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p[desc] = txq_len;

	config = (struct nrf_wifi_tx_buff *)(umac_cmd->msg);

	data = wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
					   nwb);

	len = wifi_nrf_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
					   nwb);

	config->umac_head.cmd = NRF_WIFI_CMD_TX_BUFF;

	config->umac_head.len += sizeof(struct nrf_wifi_tx_buff);
	config->umac_head.len += sizeof(struct nrf_wifi_tx_buff_info) * txq_len;

	config->tx_desc_num = desc;

	config->mac_hdr_info.umac_fill_flags =
		ADDR2_POPULATED | ADDR3_POPULATED;

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      config->mac_hdr_info.dest,
			      wifi_nrf_util_get_dest(fmac_dev_ctx, nwb),
			      NRF_WIFI_ETH_ADDR_LEN);

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      config->mac_hdr_info.src,
			      wifi_nrf_util_get_src(fmac_dev_ctx, nwb),
			      NRF_WIFI_ETH_ADDR_LEN);

	nwb_data = wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
					       nwb);
	config->mac_hdr_info.etype =
		wifi_nrf_util_tx_get_eth_type(fmac_dev_ctx,
					      nwb_data);

	config->mac_hdr_info.dscp_or_tos =
		wifi_nrf_util_get_tid(fmac_dev_ctx, nwb);

	config->num_tx_pkts = 0;

	info.fmac_dev_ctx = fmac_dev_ctx;
	info.config = config;

	status = wifi_nrf_utils_list_traverse(fmac_dev_ctx->fpriv->opriv,
					      txq,
					      &info,
					      tx_cmd_prep_callbk_fn);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: build_mac80211_hdr failed\n",
				      __func__);
		return -1;
	}

	fmac_dev_ctx->host_stats.total_tx_pkts += config->num_tx_pkts;
	config->wdev_id = fmac_dev_ctx->tx_config.peers[peer_id].if_idx;

	if ((vif_ctx->if_type == NRF_WIFI_IFTYPE_AP ||
	    vif_ctx->if_type == NRF_WIFI_IFTYPE_AP_VLAN ||
	    vif_ctx->if_type == NRF_WIFI_IFTYPE_MESH_POINT) &&
		pending_frames_count(fmac_dev_ctx, peer_id) != 0) {
		config->mac_hdr_info.more_data = 1;
	}

	if (fmac_dev_ctx->tx_config.peers[peer_id].ps_token_count == 0) {
		wifi_nrf_utils_list_del_node(fmac_dev_ctx->fpriv->opriv,
					     fmac_dev_ctx->tx_config.wakeup_client_q,
					     &fmac_dev_ctx->tx_config.peers[peer_id]);

		config->mac_hdr_info.eosp = 1;

		if (fmac_dev_ctx->tx_config.peers[peer_id].ps_state == NRF_WIFI_CLIENT_PS_MODE) {
		}
	} else {
		config->mac_hdr_info.eosp = 0;
	}

	return 0;
}


enum wifi_nrf_status tx_cmd_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				 void *txq,
				 int desc,
				 int peer_id)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	unsigned int len = 0;

	len += sizeof(struct nrf_wifi_tx_buff_info);
	len *= wifi_nrf_utils_list_len(fmac_dev_ctx->fpriv->opriv, txq);

	len += sizeof(struct nrf_wifi_tx_buff);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_DATA,
				  len);

	status = tx_cmd_prepare(fmac_dev_ctx,
				umac_cmd,
				desc,
				txq,
				peer_id);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: tx_cmd_prepare failed\n",
				      __func__);

		goto out;
	}

	status = wifi_nrf_hal_data_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    WIFI_NRF_HAL_MSG_TYPE_CMD_DATA_TX,
					    umac_cmd,
					    sizeof(*umac_cmd) + len,
					    desc,
					    0);

	wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       umac_cmd);
out:
	return status;
}


enum wifi_nrf_status tx_pending_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					unsigned int desc,
					unsigned int ac)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	if (!fmac_dev_ctx) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid params\n",
				      __func__);
		goto out;
	}

	if (_tx_pending_process(fmac_dev_ctx, desc, ac) > 0) {
		status = tx_cmd_init(fmac_dev_ctx,
				     fmac_dev_ctx->tx_config.pkt_info_p[desc].pkt,
				     desc,
				     fmac_dev_ctx->tx_config.pkt_info_p[desc].peer_id);
	} else {
		tx_desc_free(fmac_dev_ctx,
			     desc,
			     ac);

		status = WIFI_NRF_STATUS_SUCCESS;
	}

out:
	return status;
}


enum wifi_nrf_status tx_enqueue(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				void *nwb,
				unsigned int ac,
				unsigned int peer_id)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	void *queue = NULL;
	int qlen = 0;

	if (!fmac_dev_ctx || !nwb) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid params\n",
				      __func__);
		goto out;
	}

	queue = fmac_dev_ctx->tx_config.data_pending_txq[peer_id][ac];

	qlen = wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv, queue);

	if (qlen >= CONFIG_NRF700X_MAX_TX_PENDING_QLEN) {
		goto out;
	}

	wifi_nrf_utils_q_enqueue(fmac_dev_ctx->fpriv->opriv,
				 queue,
				 nwb);

	status = update_pend_q_bmp(fmac_dev_ctx, ac, peer_id);


out:
	return status;
}


enum wifi_nrf_status tx_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				unsigned char if_idx,
				void *nbuf,
				unsigned int ac,
				unsigned int peer_id)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_priv *fpriv = NULL;
	void *pend_pkt_q = NULL;
	void *first_nwb = NULL;
	unsigned char ps_state = 0;
	bool aggr_status = false;
	int max_cmds = 0;

	fpriv = fmac_dev_ctx->fpriv;

	status = tx_enqueue(fmac_dev_ctx,
			    nbuf,
			    ac,
			    peer_id);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		goto out;
	}

	ps_state = fmac_dev_ctx->tx_config.peers[peer_id].ps_state;

	if (ps_state == NRF_WIFI_CLIENT_PS_MODE) {
		goto out;
	}

	pend_pkt_q = fmac_dev_ctx->tx_config.data_pending_txq[peer_id][ac];

	/* If outstanding_descs for a particular
	 * access category >= NUM_TX_DESCS_PER_AC means there are already
	 * pending packets for that access category. So now see if frames
	 * can be aggregated depending upon access category depending
	 * upon SA, RA & AC
	 */

	if ((fmac_dev_ctx->tx_config.outstanding_descs[ac]) >= fpriv->num_tx_tokens_per_ac) {
		if (wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv,
					 pend_pkt_q)) {
			first_nwb = wifi_nrf_utils_q_peek(fmac_dev_ctx->fpriv->opriv,
							  pend_pkt_q);

			aggr_status = true;

			if (!wifi_nrf_util_ether_addr_equal(wifi_nrf_util_get_dest(fmac_dev_ctx,
										   nbuf),
							    wifi_nrf_util_get_dest(fmac_dev_ctx,
										   first_nwb))) {
				aggr_status = false;
			}

			if (!wifi_nrf_util_ether_addr_equal(wifi_nrf_util_get_src(fmac_dev_ctx,
										  nbuf),
							    wifi_nrf_util_get_src(fmac_dev_ctx,
										  first_nwb))) {
				aggr_status = false;
			}
		}

		if (aggr_status) {
			max_cmds = fmac_dev_ctx->fpriv->data_config.max_tx_aggregation;

			if (wifi_nrf_utils_q_len(fmac_dev_ctx->fpriv->opriv,
						 pend_pkt_q) < max_cmds) {
				goto out;
			}

		}
			goto out;
	}
out:
	return status;
}


unsigned int tx_buff_req_free(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			      unsigned int tx_desc_num,
			      unsigned char *ac)
{
	unsigned int pkts_pend = 0;
	unsigned int desc = tx_desc_num;
	int tx_done_q = 0, start_ac, end_ac, cnt = 0;
	unsigned short tx_done_spare_desc_q_map = 0;

	/* Determine the Queue from the descriptor */
	/* Reserved desc */
	if (desc < (fmac_dev_ctx->fpriv->num_tx_tokens_per_ac * WIFI_NRF_FMAC_AC_MAX)) {
		tx_done_q = (desc % WIFI_NRF_FMAC_AC_MAX);
		start_ac = end_ac = tx_done_q;
	} else {
		/* Derive the queue here as it is not given by UMAC. */
		if (desc >= (fmac_dev_ctx->fpriv->num_tx_tokens_per_ac * WIFI_NRF_FMAC_AC_MAX)) {
			tx_done_spare_desc_q_map = get_spare_desc_q_map(fmac_dev_ctx, desc);

			if (tx_done_spare_desc_q_map & (1 << WIFI_NRF_FMAC_AC_BK))
				tx_done_q = WIFI_NRF_FMAC_AC_BK;
			else if (tx_done_spare_desc_q_map & (1 << WIFI_NRF_FMAC_AC_BE))
				tx_done_q = WIFI_NRF_FMAC_AC_BE;
			else if (tx_done_spare_desc_q_map & (1 << WIFI_NRF_FMAC_AC_VI))
				tx_done_q = WIFI_NRF_FMAC_AC_VI;
			else if (tx_done_spare_desc_q_map & (1 << WIFI_NRF_FMAC_AC_VO))
				tx_done_q = WIFI_NRF_FMAC_AC_VO;
		}

		/* Spare desc:
		 * Loop through all AC's
		 */
		start_ac = WIFI_NRF_FMAC_AC_VO;
		end_ac = WIFI_NRF_FMAC_AC_BK;
	}

	if (fmac_dev_ctx->twt_sleep_status ==
	    WIFI_NRF_FMAC_TWT_STATE_SLEEP) {
		tx_desc_free(fmac_dev_ctx,
			     desc,
			     tx_done_q);
		goto out;
	} else {
		for (cnt = start_ac; cnt >= end_ac; cnt--) {
			pkts_pend = _tx_pending_process(fmac_dev_ctx, desc, cnt);

			if (pkts_pend) {
				*ac = (unsigned char)cnt;

				/* Spare Token Case*/
				if (tx_done_q != *ac) {
					/* Adjust the counters */
					fmac_dev_ctx->tx_config.outstanding_descs[tx_done_q]--;
					fmac_dev_ctx->tx_config.outstanding_descs[*ac]++;

					/* Update the queue_map */
					/* Clear the last access category. */
					clear_spare_desc_q_map(fmac_dev_ctx, desc, tx_done_q);
					/* Set the new access category. */
					set_spare_desc_q_map(fmac_dev_ctx, desc, *ac);
				}
				break;
			}
		}

	}

	if (!pkts_pend) {
		/* Mark the desc as available */
		tx_desc_free(fmac_dev_ctx,
			     desc,
			     tx_done_q);
	}

out:
	return pkts_pend;
}


enum wifi_nrf_status tx_done_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				     struct nrf_wifi_tx_buff_done *config)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_priv *fpriv = NULL;
	void *nwb = NULL;
	void *nwb_list = NULL;
	unsigned int desc = 0;
	unsigned int frame = 0;
	unsigned int desc_id = 0;
	unsigned long virt_addr = 0;
	struct wifi_nrf_fmac_buf_map_info *tx_buf_info = NULL;
	struct tx_pkt_info *pkt_info = NULL;
	unsigned int pkt = 0;
	unsigned int pkts_pending = 0;
	unsigned char queue = 0;
	void *txq = NULL;

	fpriv = fmac_dev_ctx->fpriv;

	desc = config->tx_desc_num;

	if (desc > fpriv->num_tx_tokens) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "Invalid desc\n");
		goto out;
	}

	pkt_info = &fmac_dev_ctx->tx_config.pkt_info_p[desc];
	nwb_list = pkt_info->pkt;

	for (frame = 0;
	     frame < fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p[desc];
	     frame++) {
		desc_id = (desc * fmac_dev_ctx->fpriv->data_config.max_tx_aggregation) + frame;

		tx_buf_info = &fmac_dev_ctx->tx_buf_info[desc_id];

		if (!tx_buf_info->mapped) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Deinit_TX cmd called for unmapped TX buf(%d)\n",
					      __func__,
					      desc_id);
			status = WIFI_NRF_STATUS_FAIL;
			goto out;
		}

		virt_addr = wifi_nrf_hal_buf_unmap_tx(fmac_dev_ctx->hal_dev_ctx,
						      desc_id);

		if (!virt_addr) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: wifi_nrf_hal_buf_unmap_tx failed\n",
					      __func__);
			status = WIFI_NRF_STATUS_FAIL;
			goto out;
		}

		/* TODO: See why we can't free the nwb here itself instead of
		 * later as is being done now
		 */
		tx_buf_info->nwb = 0;
		tx_buf_info->mapped = false;
	}

	pkt = 0;

	while (wifi_nrf_utils_q_len(fpriv->opriv,
				    nwb_list)) {
		nwb = wifi_nrf_utils_q_dequeue(fpriv->opriv,
					       nwb_list);

		if (!nwb) {
			continue;
		}

		wifi_nrf_osal_nbuf_free(fmac_dev_ctx->fpriv->opriv,
					nwb);
		pkt++;
	}

	fmac_dev_ctx->host_stats.total_tx_done_pkts += pkt;

	pkts_pending = tx_buff_req_free(fmac_dev_ctx, config->tx_desc_num, &queue);

	if (pkts_pending) {
		if (fmac_dev_ctx->twt_sleep_status ==
		    WIFI_NRF_FMAC_TWT_STATE_AWAKE) {

			pkt_info = &fmac_dev_ctx->tx_config.pkt_info_p[desc];

			txq = pkt_info->pkt;

			status = tx_cmd_init(fmac_dev_ctx,
					     txq,
					     desc,
					     pkt_info->peer_id);
		} else {
			status = WIFI_NRF_STATUS_SUCCESS;
		}

	} else {
		status = WIFI_NRF_STATUS_SUCCESS;
	}
out:
	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_tx_done_event_process(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
							 struct nrf_wifi_tx_buff_done *config)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	if (!fmac_dev_ctx) {
		goto out;
	}

	if (!config) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);

		goto out;
	}

	wifi_nrf_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
				    fmac_dev_ctx->tx_config.tx_lock);

	status = tx_done_process(fmac_dev_ctx,
				 config);

	wifi_nrf_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
				   fmac_dev_ctx->tx_config.tx_lock);

out:
	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Failed\n",
				      __func__);
	}

	return status;
}


enum wifi_nrf_status wifi_nrf_fmac_tx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				      int if_id,
				      void *nbuf,
				      unsigned int ac,
				      unsigned int peer_id)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int desc = 0;
	struct wifi_nrf_fmac_priv *fpriv = NULL;

	fpriv = fmac_dev_ctx->fpriv;

	wifi_nrf_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
				    fmac_dev_ctx->tx_config.tx_lock);


	if (fpriv->num_tx_tokens == 0) {
		goto out;
	}

	status = tx_process(fmac_dev_ctx,
			    if_id,
			    nbuf,
			    ac,
			    peer_id);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		goto out;
	}


	if (fmac_dev_ctx->twt_sleep_status ==
	    WIFI_NRF_FMAC_TWT_STATE_SLEEP) {
		goto out;
	}

	desc = tx_desc_get(fmac_dev_ctx, ac);

	if (desc == fpriv->num_tx_tokens) {
		goto out;
	}

	status = tx_pending_process(fmac_dev_ctx,
				    desc,
				    ac);
out:
	/* TODO: for goto cases also Returning success always
	 * (same as in CL-5741850).
	 * Check again
	 */
	status = WIFI_NRF_STATUS_SUCCESS;

	wifi_nrf_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
				   fmac_dev_ctx->tx_config.tx_lock);

	return status;
}


enum wifi_nrf_status tx_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_priv *fpriv = NULL;
	void *mem_ptr = NULL;
	void *q_ptr = NULL;
	unsigned int i = 0;
	unsigned int j = 0;

	if (!fmac_dev_ctx) {
		goto out;
	}

	fpriv = fmac_dev_ctx->fpriv;

	fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p =
		wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					 (sizeof(unsigned int) *
					  fpriv->num_tx_tokens));

	if (!fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate send_pkt_coalesce_count_p\n",
				      __func__);
		goto out;
	}

	for (i = 0; i < WIFI_NRF_FMAC_AC_MAX; i++) {
		for (j = 0; j < MAX_SW_PEERS; j++) {
			fmac_dev_ctx->tx_config.data_pending_txq[j][i] =
				wifi_nrf_utils_q_alloc(fpriv->opriv);

			if (!fmac_dev_ctx->tx_config.data_pending_txq[j][i]) {
				wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
						      "%s: Unable to allocate data_pending_txq\n",
						      __func__);

				mem_ptr = fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p;

				wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
						       mem_ptr);

				goto out;
			}
		}

		fmac_dev_ctx->tx_config.outstanding_descs[i] = 0;
	}

	/* Used to store the address of tx'ed skb and len of 802.11 hdr
	 * it will be used in tx complete.
	 */
	fmac_dev_ctx->tx_config.pkt_info_p = wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
								      (sizeof(struct tx_pkt_info) *
								       fpriv->num_tx_tokens));

	if (!fmac_dev_ctx->tx_config.pkt_info_p) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate pkt_info_p\n",
				      __func__);

		for (i = 0; i < WIFI_NRF_FMAC_AC_MAX; i++) {
			for (j = 0; j < MAX_SW_PEERS; j++) {
				q_ptr = fmac_dev_ctx->tx_config.data_pending_txq[j][i];

				wifi_nrf_utils_q_free(fpriv->opriv,
						      q_ptr);
			}
		}

		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p);

		goto out;
	}

	for (i = 0; i < fpriv->num_tx_tokens; i++) {
		fmac_dev_ctx->tx_config.pkt_info_p[i].pkt = wifi_nrf_utils_list_alloc(fpriv->opriv);

		if (!fmac_dev_ctx->tx_config.pkt_info_p[i].pkt) {
			wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Unable to allocate pkt list\n",
					      __func__);

			wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
					       fmac_dev_ctx->tx_config.pkt_info_p);

			for (i = 0; i < WIFI_NRF_FMAC_AC_MAX; i++) {
				for (j = 0; j < MAX_SW_PEERS; j++) {
					q_ptr = fmac_dev_ctx->tx_config.data_pending_txq[j][i];

					wifi_nrf_utils_q_free(fpriv->opriv,
							      q_ptr);
				}
			}

			wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
					       fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p);

			goto out;
		}
	}

	for (j = 0; j < WIFI_NRF_FMAC_AC_MAX; j++) {
		fmac_dev_ctx->tx_config.curr_peer_opp[j] = 0;
	}

	fmac_dev_ctx->tx_config.buf_pool_bmp_p =
		wifi_nrf_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					 (sizeof(unsigned long) *
					  (fpriv->num_tx_tokens/TX_DESC_BUCKET_BOUND) + 1));

	if (!fmac_dev_ctx->tx_config.buf_pool_bmp_p) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate buf_pool_bmp_p\n",
				      __func__);

		for (i = 0; i < fpriv->num_tx_tokens; i++) {
			wifi_nrf_utils_list_free(fpriv->opriv,
						 fmac_dev_ctx->tx_config.pkt_info_p[i].pkt);
		}

		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       fmac_dev_ctx->tx_config.pkt_info_p);

		for (i = 0; i < WIFI_NRF_FMAC_AC_MAX; i++) {
			for (j = 0; j < MAX_SW_PEERS; j++) {
				q_ptr = fmac_dev_ctx->tx_config.data_pending_txq[j][i];

				wifi_nrf_utils_q_free(fpriv->opriv,
						      q_ptr);
			}
		}

		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p);

		goto out;
	}


	wifi_nrf_osal_mem_set(fmac_dev_ctx->fpriv->opriv,
			      fmac_dev_ctx->tx_config.buf_pool_bmp_p,
			      0,
			      sizeof(long)*((fpriv->num_tx_tokens/TX_DESC_BUCKET_BOUND) + 1));

	for (i = 0; i < MAX_PEERS; i++) {
		fmac_dev_ctx->tx_config.peers[i].peer_id = -1;
	}

	fmac_dev_ctx->tx_config.tx_lock = wifi_nrf_osal_spinlock_alloc(fmac_dev_ctx->fpriv->opriv);

	if (!fmac_dev_ctx->tx_config.tx_lock) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate TX lock\n",
				      __func__);

		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       fmac_dev_ctx->tx_config.buf_pool_bmp_p);

		for (i = 0; i < fpriv->num_tx_tokens; i++) {
			wifi_nrf_utils_list_free(fpriv->opriv,
						 fmac_dev_ctx->tx_config.pkt_info_p[i].pkt);
		}

		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       fmac_dev_ctx->tx_config.pkt_info_p);

		for (i = 0; i < WIFI_NRF_FMAC_AC_MAX; i++) {
			for (j = 0; j < MAX_SW_PEERS; j++) {
				q_ptr = fmac_dev_ctx->tx_config.data_pending_txq[j][i];

				wifi_nrf_utils_q_free(fpriv->opriv,
						      q_ptr);
			}
		}

		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p);

		goto out;
	}

	wifi_nrf_osal_spinlock_init(fmac_dev_ctx->fpriv->opriv,
				    fmac_dev_ctx->tx_config.tx_lock);

	fmac_dev_ctx->tx_config.wakeup_client_q = wifi_nrf_utils_q_alloc(fpriv->opriv);

	if (!fmac_dev_ctx->tx_config.wakeup_client_q) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate Wakeup Client List\n",
				      __func__);

		wifi_nrf_osal_spinlock_free(fmac_dev_ctx->fpriv->opriv,
					    fmac_dev_ctx->tx_config.tx_lock);

		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       fmac_dev_ctx->tx_config.buf_pool_bmp_p);

		for (i = 0; i < fpriv->num_tx_tokens; i++) {
			wifi_nrf_utils_list_free(fpriv->opriv,
						 fmac_dev_ctx->tx_config.pkt_info_p[i].pkt);
		}

		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       fmac_dev_ctx->tx_config.pkt_info_p);

		for (i = 0; i < WIFI_NRF_FMAC_AC_MAX; i++) {
			for (j = 0; j < MAX_SW_PEERS; j++) {
				q_ptr = fmac_dev_ctx->tx_config.data_pending_txq[j][i];

				wifi_nrf_utils_q_free(fpriv->opriv,
						      q_ptr);
			}
		}

		wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
				       fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p);

		goto out;
	}

	fmac_dev_ctx->twt_sleep_status = WIFI_NRF_FMAC_TWT_STATE_AWAKE;

	status = WIFI_NRF_STATUS_SUCCESS;
out:
	return status;
}


void tx_deinit(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx)
{
	struct wifi_nrf_fmac_priv *fpriv = NULL;
	unsigned int i = 0;
	unsigned int j = 0;

	fpriv = fmac_dev_ctx->fpriv;

	/* TODO: Need to deinit network buffers? */

	wifi_nrf_utils_q_free(fpriv->opriv,
			      fmac_dev_ctx->tx_config.wakeup_client_q);

	wifi_nrf_osal_spinlock_free(fmac_dev_ctx->fpriv->opriv,
				    fmac_dev_ctx->tx_config.tx_lock);

	wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       fmac_dev_ctx->tx_config.buf_pool_bmp_p);

	for (i = 0; i < fpriv->num_tx_tokens; i++) {
		if (fmac_dev_ctx->tx_config.pkt_info_p) {
			wifi_nrf_utils_list_free(fpriv->opriv,
						 fmac_dev_ctx->tx_config.pkt_info_p[i].pkt);
		}
	}

	wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       fmac_dev_ctx->tx_config.pkt_info_p);

	for (i = 0; i < WIFI_NRF_FMAC_AC_MAX; i++) {
		for (j = 0; j < MAX_SW_PEERS; j++) {
			wifi_nrf_utils_q_free(fpriv->opriv,
					      fmac_dev_ctx->tx_config.data_pending_txq[j][i]);
		}
	}

	wifi_nrf_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       fmac_dev_ctx->tx_config.send_pkt_coalesce_count_p);

	wifi_nrf_osal_mem_set(fmac_dev_ctx->fpriv->opriv,
			      &fmac_dev_ctx->tx_config,
			      0,
			      sizeof(struct tx_config));
}


static int map_ac_from_tid(int tid)
{
	const int map_1d_to_ac[8] = {
		WIFI_NRF_FMAC_AC_BE, /*UP 0, 802.1D(BE), AC(BE) */
		WIFI_NRF_FMAC_AC_BK, /*UP 1, 802.1D(BK), AC(BK) */
		WIFI_NRF_FMAC_AC_BK, /*UP 2, 802.1D(BK), AC(BK) */
		WIFI_NRF_FMAC_AC_BE, /*UP 3, 802.1D(EE), AC(BE) */
		WIFI_NRF_FMAC_AC_VI, /*UP 4, 802.1D(CL), AC(VI) */
		WIFI_NRF_FMAC_AC_VI, /*UP 5, 802.1D(VI), AC(VI) */
		WIFI_NRF_FMAC_AC_VO, /*UP 6, 802.1D(VO), AC(VO) */
		WIFI_NRF_FMAC_AC_VO  /*UP 7, 802.1D(NC), AC(VO) */
	};

	return map_1d_to_ac[tid & 7];
}


static int get_ac(unsigned int tid,
		  unsigned char *ra)
{
	if (wifi_nrf_util_is_multicast_addr(ra)) {
		return WIFI_NRF_FMAC_AC_MC;
	}

	return map_ac_from_tid(tid);
}


enum wifi_nrf_status wifi_nrf_fmac_start_xmit(void *dev_ctx,
					      unsigned char if_idx,
					      void *nbuf)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	unsigned char *ra = NULL;
	int tid = 0;
	int ac = 0;
	int peer_id = -1;

	if (!nbuf) {
		goto out;
	}

	fmac_dev_ctx = dev_ctx;

	if (wifi_nrf_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
					 nbuf) < WIFI_NRF_FMAC_ETH_HDR_LEN) {
		goto out;
	}

	ra = wifi_nrf_util_get_ra(fmac_dev_ctx->vif_ctx[if_idx], nbuf);

	peer_id = wifi_nrf_fmac_peer_get_id(fmac_dev_ctx, ra);

	if (peer_id == -1) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Got packet for unknown PEER\n",
				      __func__);

		goto out;
	} else if (peer_id == MAX_PEERS) {
		ac = WIFI_NRF_FMAC_AC_MC;
	} else {
		if (fmac_dev_ctx->tx_config.peers[peer_id].qos_supported) {
			tid = wifi_nrf_util_get_tid(fmac_dev_ctx, nbuf);
			ac = get_ac(tid, ra);
		} else {
			ac = WIFI_NRF_FMAC_AC_BE;
		}
	}


	status = wifi_nrf_fmac_tx(fmac_dev_ctx,
				  if_idx,
				  nbuf,
				  ac,
				  peer_id);

	return WIFI_NRF_STATUS_SUCCESS;
out:
	if (nbuf) {
		wifi_nrf_osal_nbuf_free(fmac_dev_ctx->fpriv->opriv,
			nbuf);
	}
	return status;
}
