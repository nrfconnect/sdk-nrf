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

static bool is_twt_emergency_pkt(struct nrf_wifi_osal_priv *opriv, void *nwb)
{
	unsigned char priority = nrf_wifi_osal_nbuf_get_priority(opriv, nwb);

	return  priority == NRF_WIFI_AC_TWT_PRIORITY_EMERGENCY;
}

/* Can be extended for other cases as well */
static bool can_xmit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       void *nwb)
{
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	return is_twt_emergency_pkt(fmac_dev_ctx->fpriv->opriv, nwb) ||
	    def_dev_ctx->twt_sleep_status == NRF_WIFI_FMAC_TWT_STATE_AWAKE;
}

/* Set the coresponding bit of access category.
 * First 4 bits(0 to 3) represenst first spare desc access cateogories
 * Second 4 bits(4 to 7) represenst second spare desc access cateogories and so on
 */
static void set_spare_desc_q_map(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				 unsigned int desc,
				 int tx_done_q)
{
	unsigned short spare_desc_indx = 0;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	nrf_wifi_osal_assert(fmac_dev_ctx->fpriv->opriv,
			     def_priv->num_tx_tokens_per_ac,
			     0,
			     NRF_WIFI_ASSERT_NOT_EQUAL_TO,
			     "num_tx_tokens_per_ac is zero");

	spare_desc_indx = (desc % (def_priv->num_tx_tokens_per_ac *
				   NRF_WIFI_FMAC_AC_MAX));

	def_dev_ctx->tx_config.spare_desc_queue_map |=
		(1 << ((spare_desc_indx * SPARE_DESC_Q_MAP_SIZE) + tx_done_q));
}


/* Clear the coresponding bit of access category.
 * First 4 bits(0 to 3) represenst first spare desc access cateogories
 * Second 4 bits(4 to 7) represenst second spare desc access cateogories and so on
 */
static void clear_spare_desc_q_map(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				   unsigned int desc,
				   int tx_done_q)
{
	unsigned short spare_desc_indx = 0;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	nrf_wifi_osal_assert(fmac_dev_ctx->fpriv->opriv,
			     def_priv->num_tx_tokens_per_ac,
			     0,
			     NRF_WIFI_ASSERT_NOT_EQUAL_TO,
			     "num_tx_tokens_per_ac is zero");

	spare_desc_indx = (desc % (def_priv->num_tx_tokens_per_ac *
				   NRF_WIFI_FMAC_AC_MAX));

	def_dev_ctx->tx_config.spare_desc_queue_map &=
		~(1 << ((spare_desc_indx * SPARE_DESC_Q_MAP_SIZE) + tx_done_q));
}

/*Get the spare descriptor queue map */
static unsigned short get_spare_desc_q_map(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   unsigned int desc)
{
	unsigned short spare_desc_indx = 0;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	spare_desc_indx = (desc % (def_priv->num_tx_tokens_per_ac *
				   NRF_WIFI_FMAC_AC_MAX));

	return	(def_dev_ctx->tx_config.spare_desc_queue_map >> (spare_desc_indx *
			SPARE_DESC_Q_MAP_SIZE)) & 0x000F;
}


int pending_frames_count(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			 int peer_id)
{
	int count = 0;
	int ac = 0;
	void *queue = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	for (ac = NRF_WIFI_FMAC_AC_VO; ac >= 0; --ac) {
		queue = def_dev_ctx->tx_config.data_pending_txq[peer_id][ac];
		count += nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv, queue);
	}

	return count;
}


enum nrf_wifi_status update_pend_q_bmp(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				       unsigned int ac,
				       int peer_id)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	void *pend_pkt_q = NULL;
	int len = 0;
	unsigned char vif_id = 0;
	unsigned char *bmp = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!fmac_dev_ctx) {
		goto out;
	}

	vif_id = def_dev_ctx->tx_config.peers[peer_id].if_idx;
	vif_ctx = def_dev_ctx->vif_ctx[vif_id];

	if (vif_ctx->if_type == NRF_WIFI_IFTYPE_AP &&
	    peer_id < MAX_PEERS) {
		bmp = &def_dev_ctx->tx_config.peers[peer_id].pend_q_bmp;
		pend_pkt_q = def_dev_ctx->tx_config.data_pending_txq[peer_id][ac];

		len = nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv, pend_pkt_q);

		if (len == 0) {
			*bmp = *bmp & ~(1 << ac);
		} else {
			*bmp = *bmp | (1 << ac);
		}

		status = hal_rpu_mem_write(fmac_dev_ctx->hal_dev_ctx,
					   (RPU_MEM_UMAC_PEND_Q_BMP +
					    (sizeof(struct sap_pend_frames_bitmap) * peer_id) +
					    NRF_WIFI_FMAC_ETH_ADDR_LEN),
					   bmp,
					   sizeof(unsigned char));
	} else {
		status = NRF_WIFI_STATUS_SUCCESS;
	}
out:
	return status;
}


void tx_desc_free(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		  unsigned int desc,
		  int queue)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	int bit = -1;
	int pool_id = -1;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;

	fpriv = fmac_dev_ctx->fpriv;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fpriv);

	bit = (desc % TX_DESC_BUCKET_BOUND);
	pool_id = (desc / TX_DESC_BUCKET_BOUND);

	if (!(def_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] & (1 << bit))) {
		return;
	}

	def_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] &= (~(1 << bit));

	def_dev_ctx->tx_config.outstanding_descs[queue]--;

	if (desc >= (def_priv->num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX)) {
		clear_spare_desc_q_map(fmac_dev_ctx, desc, queue);
	}

}


unsigned int tx_desc_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			 int queue)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	unsigned int cnt = 0;
	int curr_bit = 0;
	unsigned int desc = 0;
	int pool_id = 0;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	fpriv = fmac_dev_ctx->fpriv;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fpriv);

	desc = def_priv->num_tx_tokens;

	/* First search for a reserved desc */

	for (cnt = 0; cnt < def_priv->num_tx_tokens_per_ac; cnt++) {
		curr_bit = ((queue + (NRF_WIFI_FMAC_AC_MAX * cnt)));
		curr_bit = ((queue + (NRF_WIFI_FMAC_AC_MAX * cnt)) % TX_DESC_BUCKET_BOUND);
		pool_id = ((queue + (NRF_WIFI_FMAC_AC_MAX * cnt)) / TX_DESC_BUCKET_BOUND);

		if ((((def_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] >>
		       curr_bit)) & 1)) {
			continue;
		} else {
			def_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] |=
				(1 << curr_bit);
			desc = queue + (NRF_WIFI_FMAC_AC_MAX * cnt);
			def_dev_ctx->tx_config.outstanding_descs[queue]++;
			break;
		}
	}

	/* If reserved desc is not found search for a spare desc
	 * (only for non beacon queues)
	 */
	if (cnt == def_priv->num_tx_tokens_per_ac) {
		for (desc = def_priv->num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX;
		     desc < def_priv->num_tx_tokens;
		     desc++) {
			curr_bit = (desc % TX_DESC_BUCKET_BOUND);
			pool_id = (desc / TX_DESC_BUCKET_BOUND);

			if ((def_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] >> curr_bit) & 1) {
				continue;
			} else {
				def_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] |=
					(1 << curr_bit);
				def_dev_ctx->tx_config.outstanding_descs[queue]++;
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


int tx_aggr_check(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		  void *first_nwb,
		  int ac,
		  int peer)
{
	void *nwb = NULL;
	void *pending_pkt_queue = NULL;
	bool aggr = true;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (def_dev_ctx->tx_config.peers[peer].is_legacy) {
		return false;
	}

	pending_pkt_queue = def_dev_ctx->tx_config.data_pending_txq[peer][ac];

	if (nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv,
				 pending_pkt_queue) == 0) {
		return false;
	}

	nwb = nrf_wifi_utils_q_peek(fmac_dev_ctx->fpriv->opriv,
				    pending_pkt_queue);

	if (nwb) {
		if (!nrf_wifi_util_ether_addr_equal(nrf_wifi_util_get_dest(fmac_dev_ctx,
									   nwb),
						    nrf_wifi_util_get_dest(fmac_dev_ctx,
									   first_nwb))) {
			aggr = false;
		}

		if (!nrf_wifi_util_ether_addr_equal(nrf_wifi_util_get_src(fmac_dev_ctx,
									  nwb),
						    nrf_wifi_util_get_src(fmac_dev_ctx,
									  first_nwb))) {
			aggr = false;
		}
	}


	return aggr;
}


int get_peer_from_wakeup_q(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			   unsigned int ac)
{
	int peer_id = -1;
	struct peers_info *peer = NULL;
	void *pend_q = NULL;
	unsigned int pend_q_len;
	void *client_q = NULL;
	void *list_node = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	client_q = def_dev_ctx->tx_config.wakeup_client_q;

	list_node = nrf_wifi_osal_llist_get_node_head(fmac_dev_ctx->fpriv->opriv,
						      client_q);

	while (list_node) {
		peer = nrf_wifi_osal_llist_node_data_get(fmac_dev_ctx->fpriv->opriv,
							 list_node);

		if (peer != NULL && peer->ps_token_count) {

			pend_q = def_dev_ctx->tx_config.data_pending_txq[peer->peer_id][ac];
			pend_q_len = nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv, pend_q);

			if (pend_q_len) {
				peer->ps_token_count--;
				return peer->peer_id;
			}
		}

		list_node = nrf_wifi_osal_llist_get_node_nxt(fmac_dev_ctx->fpriv->opriv,
							     client_q,
							     list_node);
	}

	return peer_id;
}


int tx_curr_peer_opp_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			 unsigned int ac)
{
	unsigned int i = 0;
	unsigned int curr_peer_opp = 0;
	unsigned int init_peer_opp = 0;
	unsigned int pend_q_len;
	void *pend_q = NULL;
	int peer_id = -1;
	unsigned char ps_state = 0;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (ac == NRF_WIFI_FMAC_AC_MC) {
		return MAX_PEERS;
	}

	peer_id = get_peer_from_wakeup_q(fmac_dev_ctx, ac);

	if (peer_id != -1) {
		return peer_id;
	}

	init_peer_opp = def_dev_ctx->tx_config.curr_peer_opp[ac];

	for (i = 0; i < MAX_PEERS; i++) {
		curr_peer_opp = (init_peer_opp + i) % MAX_PEERS;

		ps_state = def_dev_ctx->tx_config.peers[curr_peer_opp].ps_state;

		if (ps_state == NRF_WIFI_CLIENT_PS_MODE) {
			continue;
		}

		pend_q = def_dev_ctx->tx_config.data_pending_txq[curr_peer_opp][ac];
		pend_q_len = nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv,
						  pend_q);

		if (pend_q_len) {
			def_dev_ctx->tx_config.curr_peer_opp[ac] =
				(curr_peer_opp + 1) % MAX_PEERS;
			break;
		}
	}

	if (i != MAX_PEERS) {
		peer_id = curr_peer_opp;
	}

	return peer_id;
}

size_t _tx_pending_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
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

	int max_txq_len, avail_ampdu_len_per_token;
	int ampdu_len = 0;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	max_txq_len = def_priv->data_config.max_tx_aggregation;
	avail_ampdu_len_per_token = def_priv->avail_ampdu_len_per_token;

	peer_id = tx_curr_peer_opp_get(fmac_dev_ctx, ac);

	/* No pending frames for any peer in that AC. */
	if (peer_id == -1) {
		return 0;
	}

	pend_pkt_q = def_dev_ctx->tx_config.data_pending_txq[peer_id][ac];

	if (nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv,
				 pend_pkt_q) == 0) {
		return 0;
	}

	pkt_info = &def_dev_ctx->tx_config.pkt_info_p[desc];
	txq = pkt_info->pkt;

	/* Aggregate Only MPDU's with same RA, same Rate,
	 * same Rate flags, same Tx Info flags
	 */
	if (nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv,
				 pend_pkt_q)) {
		first_nwb = nrf_wifi_utils_q_peek(fmac_dev_ctx->fpriv->opriv,
						  pend_pkt_q);
	}

	while (nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv,
				    pend_pkt_q)) {
		nwb = nrf_wifi_utils_q_peek(fmac_dev_ctx->fpriv->opriv,
					    pend_pkt_q);

		ampdu_len += TX_BUF_HEADROOM +
			nrf_wifi_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
						     (void *)nwb);

		if (ampdu_len >= avail_ampdu_len_per_token) {
			break;
		}

		if (!can_xmit(fmac_dev_ctx, nwb) ||
			(!tx_aggr_check(fmac_dev_ctx, first_nwb, ac, peer_id)) ||
			(nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv,
					      txq) >= max_txq_len)) {
			break;
		}

		nwb = nrf_wifi_utils_q_dequeue(fmac_dev_ctx->fpriv->opriv,
					       pend_pkt_q);

		nrf_wifi_utils_list_add_tail(fmac_dev_ctx->fpriv->opriv,
					     txq,
					     nwb);
	}

	/* If our criterion rejects all pending frames, or
	 * pend_q is empty, send only 1
	 */
	if (!nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv, txq)) {
		nwb = nrf_wifi_utils_q_dequeue(fmac_dev_ctx->fpriv->opriv,
					       pend_pkt_q);

		if (!can_xmit(fmac_dev_ctx, nwb)) {
			return 0;
		}

		nrf_wifi_utils_list_add_tail(fmac_dev_ctx->fpriv->opriv,
					     txq,
					     nwb);
	}

	len = nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv, txq);

	if (len > 0) {
		def_dev_ctx->tx_config.pkt_info_p[desc].peer_id = peer_id;
	}

	update_pend_q_bmp(fmac_dev_ctx, ac, peer_id);

	return len;
}


enum nrf_wifi_status tx_cmd_prep_callbk_fn(void *callbk_data,
					   void *nbuf)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_buf_map_info *tx_buf_info = NULL;
	unsigned long nwb = 0;
	unsigned long nwb_data = 0;
	unsigned long phy_addr = 0;
	struct tx_cmd_prep_info *info = NULL;
	struct nrf_wifi_tx_buff *config = NULL;
	unsigned int desc_id = 0;
	unsigned int buf_len = 0;
	unsigned char frame_indx = 0;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;

	info = (struct tx_cmd_prep_info *)callbk_data;
	fmac_dev_ctx = info->fmac_dev_ctx;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	config = info->config;
	frame_indx = config->num_tx_pkts;

	nwb = (unsigned long)nbuf;

	desc_id = (config->tx_desc_num *
		   def_priv->data_config.max_tx_aggregation) + frame_indx;

	tx_buf_info = &def_dev_ctx->tx_buf_info[desc_id];

	if (tx_buf_info->mapped) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Init_TX cmd called for already mapped TX buffer(%d)\n",
				      __func__,
				      desc_id);

		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	nwb_data = (unsigned long)nrf_wifi_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
							      (void *)nwb);

	buf_len = nrf_wifi_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
					       (void *)nwb);

	phy_addr = nrf_wifi_hal_buf_map_tx(fmac_dev_ctx->hal_dev_ctx,
					   nwb_data,
					   buf_len,
					   desc_id,
					   config->tx_desc_num,
					   frame_indx);

	if (!phy_addr) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: nrf_wifi_hal_buf_map_tx failed\n",
				      __func__);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	tx_buf_info->nwb = nwb;
	tx_buf_info->mapped = true;

	config->tx_buff_info[frame_indx].ddr_ptr =
		(unsigned long long)phy_addr;

	config->tx_buff_info[frame_indx].pkt_length = buf_len;
	config->num_tx_pkts++;

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}


enum nrf_wifi_status tx_cmd_prepare(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
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
	unsigned char *data = NULL;
	struct tx_cmd_prep_info info;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;
	unsigned char vif_id;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	vif_id = def_dev_ctx->tx_config.peers[peer_id].if_idx;
	vif_ctx = def_dev_ctx->vif_ctx[vif_id];

	txq_len = nrf_wifi_utils_list_len(fmac_dev_ctx->fpriv->opriv,
					  txq);

	if (txq_len == 0) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: txq_len = %d\n",
				      __func__,
				      txq_len);
		goto err;
	}

	nwb = nrf_wifi_utils_list_peek(fmac_dev_ctx->fpriv->opriv,
				       txq);

	def_dev_ctx->tx_config.send_pkt_coalesce_count_p[desc] = txq_len;

	config = (struct nrf_wifi_tx_buff *)(umac_cmd->msg);

	data = nrf_wifi_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
					   nwb);

	len = nrf_wifi_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
					   nwb);

	config->umac_head.cmd = NRF_WIFI_CMD_TX_BUFF;

	config->umac_head.len += sizeof(struct nrf_wifi_tx_buff);
	config->umac_head.len += sizeof(struct nrf_wifi_tx_buff_info) * txq_len;

	config->tx_desc_num = desc;

	nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      config->mac_hdr_info.dest,
			      nrf_wifi_util_get_dest(fmac_dev_ctx, nwb),
			      NRF_WIFI_ETH_ADDR_LEN);

	nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      config->mac_hdr_info.src,
			      nrf_wifi_util_get_src(fmac_dev_ctx, nwb),
			      NRF_WIFI_ETH_ADDR_LEN);

	nwb_data = nrf_wifi_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
					       nwb);
	config->mac_hdr_info.etype =
		nrf_wifi_util_tx_get_eth_type(fmac_dev_ctx,
					      nwb_data);

	config->mac_hdr_info.dscp_or_tos =
		nrf_wifi_util_get_tid(fmac_dev_ctx, nwb);

	if (is_twt_emergency_pkt(fmac_dev_ctx->fpriv->opriv, nwb)) {
		config->mac_hdr_info.dscp_or_tos |= DSCP_OR_TOS_TWT_EMERGENCY_TX;
	}

	config->num_tx_pkts = 0;

	info.fmac_dev_ctx = fmac_dev_ctx;
	info.config = config;

	status = nrf_wifi_utils_list_traverse(fmac_dev_ctx->fpriv->opriv,
					      txq,
					      &info,
					      tx_cmd_prep_callbk_fn);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: build_mac80211_hdr failed\n",
				      __func__);
		goto err;
	}

	def_dev_ctx->host_stats.total_tx_pkts += config->num_tx_pkts;
	config->wdev_id = def_dev_ctx->tx_config.peers[peer_id].if_idx;

	if ((vif_ctx->if_type == NRF_WIFI_IFTYPE_AP ||
	    vif_ctx->if_type == NRF_WIFI_IFTYPE_AP_VLAN ||
	    vif_ctx->if_type == NRF_WIFI_IFTYPE_MESH_POINT) &&
		pending_frames_count(fmac_dev_ctx, peer_id) != 0) {
		config->mac_hdr_info.more_data = 1;
	}

	if (def_dev_ctx->tx_config.peers[peer_id].ps_token_count == 0) {
		nrf_wifi_utils_list_del_node(fmac_dev_ctx->fpriv->opriv,
					     def_dev_ctx->tx_config.wakeup_client_q,
					     &def_dev_ctx->tx_config.peers[peer_id]);

		config->mac_hdr_info.eosp = 1;

		if (def_dev_ctx->tx_config.peers[peer_id].ps_state == NRF_WIFI_CLIENT_PS_MODE) {
		}
	} else {
		config->mac_hdr_info.eosp = 0;
	}

	return NRF_WIFI_STATUS_SUCCESS;
err:
	return NRF_WIFI_STATUS_FAIL;
}


enum nrf_wifi_status tx_cmd_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				 void *txq,
				 int desc,
				 int peer_id)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	unsigned int len = 0;

	len += sizeof(struct nrf_wifi_tx_buff_info);
	len *= nrf_wifi_utils_list_len(fmac_dev_ctx->fpriv->opriv, txq);

	len += sizeof(struct nrf_wifi_tx_buff);

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_DATA,
				  len);

	status = tx_cmd_prepare(fmac_dev_ctx,
				umac_cmd,
				desc,
				txq,
				peer_id);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: tx_cmd_prepare failed\n",
				      __func__);

		goto out;
	}

	status = nrf_wifi_hal_data_cmd_send(fmac_dev_ctx->hal_dev_ctx,
					    NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_TX,
					    umac_cmd,
					    sizeof(*umac_cmd) + len,
					    desc,
					    0);

	nrf_wifi_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       umac_cmd);
out:
	return status;
}


enum nrf_wifi_status tx_pending_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					unsigned int desc,
					unsigned int ac)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!fmac_dev_ctx) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid params\n",
				      __func__);
		goto out;
	}

	if (_tx_pending_process(fmac_dev_ctx, desc, ac)) {
		status = tx_cmd_init(fmac_dev_ctx,
				     def_dev_ctx->tx_config.pkt_info_p[desc].pkt,
				     desc,
				     def_dev_ctx->tx_config.pkt_info_p[desc].peer_id);
	} else {
		tx_desc_free(fmac_dev_ctx,
			     desc,
			     ac);

		status = NRF_WIFI_STATUS_SUCCESS;
	}

out:
	return status;
}


enum nrf_wifi_status tx_enqueue(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				void *nwb,
				unsigned int ac,
				unsigned int peer_id)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	void *queue = NULL;
	int qlen = 0;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!fmac_dev_ctx || !nwb) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid params\n",
				      __func__);
		goto out;
	}

	queue = def_dev_ctx->tx_config.data_pending_txq[peer_id][ac];

	qlen = nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv, queue);

	if (qlen >= CONFIG_NRF700X_MAX_TX_PENDING_QLEN) {
		goto out;
	}

	if (is_twt_emergency_pkt(fmac_dev_ctx->fpriv->opriv, nwb)) {
		nrf_wifi_utils_q_enqueue_head(fmac_dev_ctx->fpriv->opriv,
					      queue,
					      nwb);
	} else {
		nrf_wifi_utils_q_enqueue(fmac_dev_ctx->fpriv->opriv,
					 queue,
					 nwb);
	}

	status = update_pend_q_bmp(fmac_dev_ctx, ac, peer_id);

out:
	return status;
}


enum nrf_wifi_fmac_tx_status tx_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				unsigned char if_idx,
				void *nbuf,
				unsigned int ac,
				unsigned int peer_id)
{
	enum nrf_wifi_fmac_tx_status status;
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;
	void *pend_pkt_q = NULL;
	void *first_nwb = NULL;
	unsigned char ps_state = 0;
	bool aggr_status = false;
	int max_cmds = 0;

	fpriv = fmac_dev_ctx->fpriv;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fpriv);

	status = tx_enqueue(fmac_dev_ctx,
			    nbuf,
			    ac,
			    peer_id);

	if (status != NRF_WIFI_FMAC_TX_STATUS_SUCCESS) {
		goto err;
	}

	ps_state = def_dev_ctx->tx_config.peers[peer_id].ps_state;

	if (ps_state == NRF_WIFI_CLIENT_PS_MODE) {
		goto out;
	}

	pend_pkt_q = def_dev_ctx->tx_config.data_pending_txq[peer_id][ac];

	/* If outstanding_descs for a particular
	 * access category >= NUM_TX_DESCS_PER_AC means there are already
	 * pending packets for that access category. So now see if frames
	 * can be aggregated depending upon access category depending
	 * upon SA, RA & AC
	 */

	if ((def_dev_ctx->tx_config.outstanding_descs[ac]) >= def_priv->num_tx_tokens_per_ac) {
		if (nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv,
					 pend_pkt_q)) {
			first_nwb = nrf_wifi_utils_q_peek(fmac_dev_ctx->fpriv->opriv,
							  pend_pkt_q);

			aggr_status = true;

			if (!nrf_wifi_util_ether_addr_equal(nrf_wifi_util_get_dest(fmac_dev_ctx,
										   nbuf),
							    nrf_wifi_util_get_dest(fmac_dev_ctx,
										   first_nwb))) {
				aggr_status = false;
			}

			if (!nrf_wifi_util_ether_addr_equal(nrf_wifi_util_get_src(fmac_dev_ctx,
										  nbuf),
							    nrf_wifi_util_get_src(fmac_dev_ctx,
										  first_nwb))) {
				aggr_status = false;
			}
		}

		if (aggr_status) {
			max_cmds = def_priv->data_config.max_tx_aggregation;

			if (nrf_wifi_utils_q_len(fmac_dev_ctx->fpriv->opriv,
						 pend_pkt_q) < max_cmds) {
				goto out;
			}
		}
	}
	return NRF_WIFI_FMAC_TX_STATUS_SUCCESS;
out:
	return NRF_WIFI_FMAC_TX_STATUS_QUEUED;
err:
	return NRF_WIFI_FMAC_TX_STATUS_FAIL;
}


unsigned int tx_buff_req_free(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			      unsigned int tx_desc_num,
			      unsigned char *ac)
{
	unsigned int pkts_pend = 0;
	unsigned int desc = tx_desc_num;
	int tx_done_q = 0, start_ac, end_ac, cnt = 0;
	unsigned short tx_done_spare_desc_q_map = 0;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	/* Determine the Queue from the descriptor */
	/* Reserved desc */
	if (desc < (def_priv->num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX)) {
		tx_done_q = (desc % NRF_WIFI_FMAC_AC_MAX);
		start_ac = end_ac = tx_done_q;
	} else {
		/* Derive the queue here as it is not given by UMAC. */
		if (desc >= (def_priv->num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX)) {
			tx_done_spare_desc_q_map = get_spare_desc_q_map(fmac_dev_ctx, desc);

			if (tx_done_spare_desc_q_map & (1 << NRF_WIFI_FMAC_AC_BK))
				tx_done_q = NRF_WIFI_FMAC_AC_BK;
			else if (tx_done_spare_desc_q_map & (1 << NRF_WIFI_FMAC_AC_BE))
				tx_done_q = NRF_WIFI_FMAC_AC_BE;
			else if (tx_done_spare_desc_q_map & (1 << NRF_WIFI_FMAC_AC_VI))
				tx_done_q = NRF_WIFI_FMAC_AC_VI;
			else if (tx_done_spare_desc_q_map & (1 << NRF_WIFI_FMAC_AC_VO))
				tx_done_q = NRF_WIFI_FMAC_AC_VO;
		}

		/* Spare desc:
		 * Loop through all AC's
		 */
		start_ac = NRF_WIFI_FMAC_AC_VO;
		end_ac = NRF_WIFI_FMAC_AC_BK;
	}

	for (cnt = start_ac; cnt >= end_ac; cnt--) {
		pkts_pend = _tx_pending_process(fmac_dev_ctx, desc, cnt);

		if (pkts_pend) {
			*ac = (unsigned char)cnt;

			/* Spare Token Case*/
			if (tx_done_q != *ac) {
				/* Adjust the counters */
				def_dev_ctx->tx_config.outstanding_descs[tx_done_q]--;
				def_dev_ctx->tx_config.outstanding_descs[*ac]++;

				/* Update the queue_map */
				/* Clear the last access category. */
				clear_spare_desc_q_map(fmac_dev_ctx, desc, tx_done_q);
				/* Set the new access category. */
				set_spare_desc_q_map(fmac_dev_ctx, desc, *ac);
			}
			break;
		}
	}

	if (!pkts_pend) {
		/* Mark the desc as available */
		tx_desc_free(fmac_dev_ctx,
			     desc,
			     tx_done_q);
	}

	return pkts_pend;
}


enum nrf_wifi_status tx_done_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				     struct nrf_wifi_tx_buff_done *config)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	void *nwb = NULL;
	void *nwb_list = NULL;
	unsigned int desc = 0;
	unsigned int frame = 0;
	unsigned int desc_id = 0;
	unsigned long virt_addr = 0;
	struct nrf_wifi_fmac_buf_map_info *tx_buf_info = NULL;
	struct tx_pkt_info *pkt_info = NULL;
	unsigned int pkt = 0;
	unsigned int pkts_pending = 0;
	unsigned char queue = 0;
	void *txq = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;

	fpriv = fmac_dev_ctx->fpriv;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	desc = config->tx_desc_num;

	if (desc > def_priv->num_tx_tokens) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "Invalid desc\n");
		goto out;
	}

	pkt_info = &def_dev_ctx->tx_config.pkt_info_p[desc];
	nwb_list = pkt_info->pkt;

	for (frame = 0;
	     frame < def_dev_ctx->tx_config.send_pkt_coalesce_count_p[desc];
	     frame++) {
		desc_id = (desc * def_priv->data_config.max_tx_aggregation) + frame;

		tx_buf_info = &def_dev_ctx->tx_buf_info[desc_id];

		if (!tx_buf_info->mapped) {
			nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Deinit_TX cmd called for unmapped TX buf(%d)\n",
					      __func__,
					      desc_id);
			status = NRF_WIFI_STATUS_FAIL;
			goto out;
		}

		virt_addr = nrf_wifi_hal_buf_unmap_tx(fmac_dev_ctx->hal_dev_ctx,
						      desc_id);

		if (!virt_addr) {
			nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: nrf_wifi_hal_buf_unmap_tx failed\n",
					      __func__);
			status = NRF_WIFI_STATUS_FAIL;
			goto out;
		}

		/* TODO: See why we can't free the nwb here itself instead of
		 * later as is being done now
		 */
		tx_buf_info->nwb = 0;
		tx_buf_info->mapped = false;
	}

	pkt = 0;

	while (nrf_wifi_utils_q_len(fpriv->opriv,
				    nwb_list)) {
		nwb = nrf_wifi_utils_q_dequeue(fpriv->opriv,
					       nwb_list);

		if (!nwb) {
			continue;
		}

		nrf_wifi_osal_nbuf_free(fmac_dev_ctx->fpriv->opriv,
					nwb);
		pkt++;
	}

	def_dev_ctx->host_stats.total_tx_done_pkts += pkt;

	pkts_pending = tx_buff_req_free(fmac_dev_ctx, config->tx_desc_num, &queue);

	if (pkts_pending) {
		if (def_dev_ctx->twt_sleep_status ==
		    NRF_WIFI_FMAC_TWT_STATE_AWAKE) {

			pkt_info = &def_dev_ctx->tx_config.pkt_info_p[desc];

			txq = pkt_info->pkt;

			status = tx_cmd_init(fmac_dev_ctx,
					     txq,
					     desc,
					     pkt_info->peer_id);
		} else {
			status = NRF_WIFI_STATUS_SUCCESS;
		}

	} else {
		status = NRF_WIFI_STATUS_SUCCESS;
	}
out:
	return status;
}

#ifdef CONFIG_NRF700X_TX_DONE_WQ_ENABLED
static void tx_done_tasklet_fn(unsigned long data)
{
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = (struct nrf_wifi_fmac_dev_ctx *)data;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	void *tx_done_tasklet_event_q = (void *)def_dev_ctx->tx_config.tx_done_tasklet_event_q;

	struct nrf_wifi_tx_buff_done *config = nrf_wifi_utils_q_dequeue(
		fmac_dev_ctx->fpriv->opriv,
		tx_done_tasklet_event_q);

	if (!config) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: TX done event Q is empty\n",
				      __func__);
		return;
	}

	(void) nrf_wifi_fmac_tx_done_event_process(fmac_dev_ctx, config);

	nrf_wifi_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       config);
}
#endif /* CONFIG_NRF700X_TX_DONE_WQ_ENABLED */

enum nrf_wifi_status (nrf_wifi_fmac_tx_done_event_process)(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct nrf_wifi_tx_buff_done *config)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!fmac_dev_ctx) {
		goto out;
	}

	if (!config) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid parameters\n",
				      __func__);

		goto out;
	}

	nrf_wifi_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
				    def_dev_ctx->tx_config.tx_lock);

	status = tx_done_process(fmac_dev_ctx,
				 config);

	nrf_wifi_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
				   def_dev_ctx->tx_config.tx_lock);

out:
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Failed\n",
				      __func__);
	}

	return status;
}


enum nrf_wifi_fmac_tx_status nrf_wifi_fmac_tx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				      int if_id,
				      void *nbuf,
				      unsigned int ac,
				      unsigned int peer_id)
{
	enum nrf_wifi_fmac_tx_status status = NRF_WIFI_FMAC_TX_STATUS_FAIL;
	unsigned int desc = 0;
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;

	fpriv = fmac_dev_ctx->fpriv;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fpriv);

	nrf_wifi_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
				    def_dev_ctx->tx_config.tx_lock);


	if (def_priv->num_tx_tokens == 0) {
		goto out;
	}

	status = tx_process(fmac_dev_ctx,
			    if_id,
			    nbuf,
			    ac,
			    peer_id);

	if (status != NRF_WIFI_FMAC_TX_STATUS_SUCCESS) {
		goto out;
	}

	status = NRF_WIFI_FMAC_TX_STATUS_QUEUED;

	if (!can_xmit(fmac_dev_ctx, nbuf)) {
		goto out;
	}

	desc = tx_desc_get(fmac_dev_ctx, ac);

	if (desc == def_priv->num_tx_tokens) {
		goto out;
	}

	status = tx_pending_process(fmac_dev_ctx,
				    desc,
				    ac);
out:
	nrf_wifi_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
				   def_dev_ctx->tx_config.tx_lock);

	return status;
}


enum nrf_wifi_status tx_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	void *q_ptr = NULL;
	unsigned int i = 0;
	unsigned int j = 0;

	if (!fmac_dev_ctx) {
		goto out;
	}

	fpriv = fmac_dev_ctx->fpriv;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fpriv);

	def_dev_ctx->tx_config.send_pkt_coalesce_count_p =
		nrf_wifi_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					 (sizeof(unsigned int) *
					  def_priv->num_tx_tokens));

	if (!def_dev_ctx->tx_config.send_pkt_coalesce_count_p) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate send_pkt_coalesce_count_p\n",
				      __func__);
		goto out;
	}

	for (i = 0; i < NRF_WIFI_FMAC_AC_MAX; i++) {
		for (j = 0; j < MAX_SW_PEERS; j++) {
			def_dev_ctx->tx_config.data_pending_txq[j][i] =
				nrf_wifi_utils_q_alloc(fpriv->opriv);

			if (!def_dev_ctx->tx_config.data_pending_txq[j][i]) {
				nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
						      "%s: Unable to allocate data_pending_txq\n",
						      __func__);
				goto coal_q_free;
			}
		}

		def_dev_ctx->tx_config.outstanding_descs[i] = 0;
	}

	/* Used to store the address of tx'ed skb and len of 802.11 hdr
	 * it will be used in tx complete.
	 */
	def_dev_ctx->tx_config.pkt_info_p = nrf_wifi_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
								      (sizeof(struct tx_pkt_info) *
								       def_priv->num_tx_tokens));

	if (!def_dev_ctx->tx_config.pkt_info_p) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate pkt_info_p\n",
				      __func__);
		goto tx_q_free;
	}

	for (i = 0; i < def_priv->num_tx_tokens; i++) {
		def_dev_ctx->tx_config.pkt_info_p[i].pkt = nrf_wifi_utils_list_alloc(fpriv->opriv);

		if (!def_dev_ctx->tx_config.pkt_info_p[i].pkt) {
			nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
					      "%s: Unable to allocate pkt list\n",
					      __func__);
			goto tx_q_setup_free;
		}
	}

	for (j = 0; j < NRF_WIFI_FMAC_AC_MAX; j++) {
		def_dev_ctx->tx_config.curr_peer_opp[j] = 0;
	}

	def_dev_ctx->tx_config.buf_pool_bmp_p =
		nrf_wifi_osal_mem_zalloc(fmac_dev_ctx->fpriv->opriv,
					 (sizeof(unsigned long) *
					  (def_priv->num_tx_tokens/TX_DESC_BUCKET_BOUND) + 1));

	if (!def_dev_ctx->tx_config.buf_pool_bmp_p) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate buf_pool_bmp_p\n",
				      __func__);
		goto tx_pkt_info_free;
	}

	nrf_wifi_osal_mem_set(fmac_dev_ctx->fpriv->opriv,
			      def_dev_ctx->tx_config.buf_pool_bmp_p,
			      0,
			      sizeof(long)*((def_priv->num_tx_tokens/TX_DESC_BUCKET_BOUND) + 1));

	for (i = 0; i < MAX_PEERS; i++) {
		def_dev_ctx->tx_config.peers[i].peer_id = -1;
	}

	def_dev_ctx->tx_config.tx_lock = nrf_wifi_osal_spinlock_alloc(fmac_dev_ctx->fpriv->opriv);

	if (!def_dev_ctx->tx_config.tx_lock) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate TX lock\n",
				      __func__);
		goto tx_buff_map_free;
	}

	nrf_wifi_osal_spinlock_init(fmac_dev_ctx->fpriv->opriv,
				    def_dev_ctx->tx_config.tx_lock);

	def_dev_ctx->tx_config.wakeup_client_q = nrf_wifi_utils_q_alloc(fpriv->opriv);

	if (!def_dev_ctx->tx_config.wakeup_client_q) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate Wakeup Client List\n",
				      __func__);
		goto tx_spin_lock_free;
	}

	def_dev_ctx->twt_sleep_status = NRF_WIFI_FMAC_TWT_STATE_AWAKE;

#ifdef CONFIG_NRF700X_TX_DONE_WQ_ENABLED
	def_dev_ctx->tx_done_tasklet = nrf_wifi_osal_tasklet_alloc(fpriv->opriv,
								    NRF_WIFI_TASKLET_TYPE_TX_DONE);
	if (!def_dev_ctx->tx_done_tasklet) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate tx_done_tasklet\n",
				      __func__);
		goto wakeup_client_q_free;
	}
	def_dev_ctx->tx_config.tx_done_tasklet_event_q = nrf_wifi_utils_q_alloc(fpriv->opriv);
	if (!def_dev_ctx->tx_config.tx_done_tasklet_event_q) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Unable to allocate tx_done_tasklet_event_q\n",
				      __func__);
		goto tx_done_tasklet_free;
	}

	nrf_wifi_osal_tasklet_init(fmac_dev_ctx->fpriv->opriv,
				   def_dev_ctx->tx_done_tasklet,
				   tx_done_tasklet_fn,
				   (unsigned long)fmac_dev_ctx);
#endif /* CONFIG_NRF700X_TX_DONE_WQ_ENABLED */
	return NRF_WIFI_STATUS_SUCCESS;
#ifdef CONFIG_NRF700X_TX_DONE_WQ_ENABLED
tx_done_tasklet_free:
	nrf_wifi_osal_tasklet_free(fpriv->opriv,
				   def_dev_ctx->tx_done_tasklet);
wakeup_client_q_free:
	nrf_wifi_utils_q_free(fpriv->opriv, def_dev_ctx->tx_config.wakeup_client_q);
#endif /* CONFIG_NRF700X_TX_DONE_WQ_ENABLED */
tx_spin_lock_free:
	nrf_wifi_osal_spinlock_free(fmac_dev_ctx->fpriv->opriv,
					def_dev_ctx->tx_config.tx_lock);
tx_buff_map_free:
	nrf_wifi_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
					def_dev_ctx->tx_config.buf_pool_bmp_p);
tx_pkt_info_free:
	for (i = 0; i < def_priv->num_tx_tokens; i++) {
		nrf_wifi_utils_list_free(fpriv->opriv,
						def_dev_ctx->tx_config.pkt_info_p[i].pkt);
	}
tx_q_setup_free:
	nrf_wifi_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
							def_dev_ctx->tx_config.pkt_info_p);
tx_q_free:
	for (i = 0; i < NRF_WIFI_FMAC_AC_MAX; i++) {
		for (j = 0; j < MAX_SW_PEERS; j++) {
			q_ptr = def_dev_ctx->tx_config.data_pending_txq[j][i];

			nrf_wifi_utils_q_free(fpriv->opriv,
							q_ptr);
		}
	}
coal_q_free:
	nrf_wifi_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
					def_dev_ctx->tx_config.send_pkt_coalesce_count_p);
out:
	return NRF_WIFI_STATUS_FAIL;
}


void tx_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;
	unsigned int i = 0;
	unsigned int j = 0;

	fpriv = fmac_dev_ctx->fpriv;

	def_priv = wifi_fmac_priv(fpriv);
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

#ifdef CONFIG_NRF700X_TX_DONE_WQ_ENABLED
	/* TODO: Need to deinit network buffers? */
	nrf_wifi_osal_tasklet_free(fpriv->opriv,
				   def_dev_ctx->tx_done_tasklet);
	nrf_wifi_utils_q_free(fpriv->opriv,
			      def_dev_ctx->tx_config.tx_done_tasklet_event_q);
#endif /* CONFIG_NRF700X_TX_DONE_WQ_ENABLED */
	nrf_wifi_utils_q_free(fpriv->opriv,
			      def_dev_ctx->tx_config.wakeup_client_q);

	nrf_wifi_osal_spinlock_free(fmac_dev_ctx->fpriv->opriv,
				    def_dev_ctx->tx_config.tx_lock);

	nrf_wifi_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       def_dev_ctx->tx_config.buf_pool_bmp_p);

	for (i = 0; i < def_priv->num_tx_tokens; i++) {
		if (def_dev_ctx->tx_config.pkt_info_p) {
			nrf_wifi_utils_list_free(fpriv->opriv,
						 def_dev_ctx->tx_config.pkt_info_p[i].pkt);
		}
	}

	nrf_wifi_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       def_dev_ctx->tx_config.pkt_info_p);

	for (i = 0; i < NRF_WIFI_FMAC_AC_MAX; i++) {
		for (j = 0; j < MAX_SW_PEERS; j++) {
			nrf_wifi_utils_q_free(fpriv->opriv,
					      def_dev_ctx->tx_config.data_pending_txq[j][i]);
		}
	}

	nrf_wifi_osal_mem_free(fmac_dev_ctx->fpriv->opriv,
			       def_dev_ctx->tx_config.send_pkt_coalesce_count_p);

	nrf_wifi_osal_mem_set(fmac_dev_ctx->fpriv->opriv,
			      &def_dev_ctx->tx_config,
			      0,
			      sizeof(struct tx_config));
}


static int map_ac_from_tid(int tid)
{
	const int map_1d_to_ac[8] = {
		NRF_WIFI_FMAC_AC_BE, /*UP 0, 802.1D(BE), AC(BE) */
		NRF_WIFI_FMAC_AC_BK, /*UP 1, 802.1D(BK), AC(BK) */
		NRF_WIFI_FMAC_AC_BK, /*UP 2, 802.1D(BK), AC(BK) */
		NRF_WIFI_FMAC_AC_BE, /*UP 3, 802.1D(EE), AC(BE) */
		NRF_WIFI_FMAC_AC_VI, /*UP 4, 802.1D(CL), AC(VI) */
		NRF_WIFI_FMAC_AC_VI, /*UP 5, 802.1D(VI), AC(VI) */
		NRF_WIFI_FMAC_AC_VO, /*UP 6, 802.1D(VO), AC(VO) */
		NRF_WIFI_FMAC_AC_VO  /*UP 7, 802.1D(NC), AC(VO) */
	};

	return map_1d_to_ac[tid & 7];
}


static int get_ac(unsigned int tid,
		  unsigned char *ra)
{
	if (nrf_wifi_util_is_multicast_addr(ra)) {
		return NRF_WIFI_FMAC_AC_MC;
	}

	return map_ac_from_tid(tid);
}


enum nrf_wifi_status nrf_wifi_fmac_start_xmit(void *dev_ctx,
					      unsigned char if_idx,
					      void *nbuf)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	enum nrf_wifi_fmac_tx_status tx_status = NRF_WIFI_FMAC_TX_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	unsigned char *ra = NULL;
	int tid = 0;
	int ac = 0;
	int peer_id = -1;

	if (!nbuf) {
		goto out;
	}

	fmac_dev_ctx = dev_ctx;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (nrf_wifi_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
					 nbuf) < NRF_WIFI_FMAC_ETH_HDR_LEN) {
		goto out;
	}

	ra = nrf_wifi_util_get_ra(def_dev_ctx->vif_ctx[if_idx], nbuf);

	peer_id = nrf_wifi_fmac_peer_get_id(fmac_dev_ctx, ra);

	if (peer_id == -1) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Got packet for unknown PEER\n",
				      __func__);

		goto out;
	} else if (peer_id == MAX_PEERS) {
		ac = NRF_WIFI_FMAC_AC_MC;
	} else {
		if (def_dev_ctx->tx_config.peers[peer_id].qos_supported) {
			tid = nrf_wifi_util_get_tid(fmac_dev_ctx, nbuf);
			ac = get_ac(tid, ra);
		} else {
			ac = NRF_WIFI_FMAC_AC_BE;
		}
	}

	tx_status = nrf_wifi_fmac_tx(fmac_dev_ctx,
				  if_idx,
				  nbuf,
				  ac,
				  peer_id);

	if (tx_status == NRF_WIFI_FMAC_TX_STATUS_FAIL) {
		nrf_wifi_osal_log_dbg(fmac_dev_ctx->fpriv->opriv,
				      "%s: Failed to send packet\n",
				      __func__);
		goto out;
	}

	return NRF_WIFI_STATUS_SUCCESS;
out:
	if (nbuf) {
		nrf_wifi_osal_nbuf_free(fmac_dev_ctx->fpriv->opriv,
			nbuf);
	}
	return status;
}
