/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing TX data path specific function definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_core.h>

#include <common/mem_mgmt.h>
#include <common/nbuf_mgmt.h>
#include <common/util.h>
#include <common/wifi_ipc.h>
#include <system/fmac_api.h>
#include <system/fmac_peer.h>
#include <system/fmac_tx.h>
#include <common/util.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

static void tx_stats_track_max(unsigned int *max, unsigned int val)
{
	if (val > *max) {
		*max = val;
	}
}

static bool is_twt_emergency_pkt(void *nwb)
{
	unsigned char priority = nrf_wifi_nbuf_get_priority(nwb);

	return  priority == NRF_WIFI_AC_TWT_PRIORITY_EMERGENCY;
}

/* Can be extended for other cases as well */
static bool can_xmit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       void *nwb)
{
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	return is_twt_emergency_pkt(nwb) ||
	    sys_dev_ctx->twt_sleep_status == NRF_WIFI_FMAC_TWT_STATE_AWAKE;
}

static unsigned int spare_desc_ring_size(const struct nrf_wifi_sys_fmac_priv *sys_fpriv)
{
	if (sys_fpriv->num_tx_tokens_per_ac == 0) {
		return 0;
	}

	return (unsigned int)sys_fpriv->num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX;
}

static unsigned short spare_desc_index(const struct nrf_wifi_sys_fmac_priv *sys_fpriv,
				       unsigned int desc)
{
	unsigned int ring_size = spare_desc_ring_size(sys_fpriv);

	if (ring_size == 0) {
		LOG_ERR("%s: spare descriptor ring size is zero", __func__);
		return 0;
	}

	return desc % ring_size;
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
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	spare_desc_indx = spare_desc_index(sys_fpriv, desc);

	sys_dev_ctx->tx_config.spare_desc_queue_map |=
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
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	spare_desc_indx = spare_desc_index(sys_fpriv, desc);

	sys_dev_ctx->tx_config.spare_desc_queue_map &=
		~(1 << ((spare_desc_indx * SPARE_DESC_Q_MAP_SIZE) + tx_done_q));
}

/*Get the spare descriptor queue map */
static unsigned short get_spare_desc_q_map(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   unsigned int desc)
{
	unsigned short spare_desc_indx = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	spare_desc_indx = spare_desc_index(sys_fpriv, desc);

	return	(sys_dev_ctx->tx_config.spare_desc_queue_map >> (spare_desc_indx *
			SPARE_DESC_Q_MAP_SIZE)) & 0x000F;
}


static unsigned char *nrf_wifi_get_dest(void *nwb)
{
	return nrf_wifi_nbuf_data_get(nwb);
}


static unsigned char *nrf_wifi_get_src(void *nwb)
{
	return (unsigned char *)nrf_wifi_nbuf_data_get(nwb) + NRF_WIFI_FMAC_ETH_ADDR_LEN;
}

static int nrf_wifi_get_tid(void *nwb)
{
	unsigned short ether_type = 0;
	int priority = 0;
	unsigned short vlan_tci = 0;
	unsigned char vlan_priority = 0;
	unsigned int mpls_hdr = 0;
	unsigned char mpls_tc_qos = 0;
	unsigned char tos = 0;
	unsigned char dscp = 0;
	unsigned short ipv6_hdr = 0;
	void *nwb_data = NULL;

	nwb_data = nrf_wifi_nbuf_data_get(nwb);

	ether_type = nrf_wifi_util_tx_get_eth_type(nwb_data);

	nwb_data = (unsigned char *)nrf_wifi_nbuf_data_get(nwb) + NRF_WIFI_FMAC_ETH_HDR_LEN;

	switch (ether_type & NRF_WIFI_FMAC_ETH_TYPE_MASK) {
	/* If VLAN 802.1Q (0x8100) ||
	 * 802.1AD(0x88A8) FRAME calculate priority accordingly
	 */
	case NRF_WIFI_FMAC_ETH_P_8021Q:
	case NRF_WIFI_FMAC_ETH_P_8021AD:
		vlan_tci = (((unsigned char *)nwb_data)[4] << 8) |
			(((unsigned char *)nwb_data)[5]);
		vlan_priority = ((vlan_tci & NRF_WIFI_FMAC_VLAN_PRIO_MASK)
				 >> NRF_WIFI_FMAC_VLAN_PRIO_SHIFT);
		priority = vlan_priority;
		break;
	/* If MPLS MC(0x8840) / UC(0x8847) frame calculate priority
	 * accordingly
	 */
	case NRF_WIFI_FMAC_ETH_P_MPLS_UC:
	case NRF_WIFI_FMAC_ETH_P_MPLS_MC:
		mpls_hdr = (((unsigned char *)nwb_data)[0] << 24) |
			(((unsigned char *)nwb_data)[1] << 16) |
			(((unsigned char *)nwb_data)[2] << 8)  |
			(((unsigned char *)nwb_data)[3]);
		mpls_tc_qos = (mpls_hdr & (NRF_WIFI_FMAC_MPLS_LS_TC_MASK)
			       >> NRF_WIFI_FMAC_MPLS_LS_TC_SHIFT);
		priority = mpls_tc_qos;
		break;
	/* If IP (0x0800) frame calculate priority accordingly */
	case NRF_WIFI_FMAC_ETH_P_IP:
		/*get the tos filed*//*DA+SA+ETH+(VER+IHL)*/
		tos = (((unsigned char *)nwb_data)[1]);
		/*get the dscp value */
		dscp = (tos & 0xfc);
		priority = dscp >> 5;
		break;
	case NRF_WIFI_FMAC_ETH_P_IPV6:
		/* Get the TOS filled DA+SA+ETH */
		ipv6_hdr = (((unsigned char *)nwb_data)[0] << 8) |
			((unsigned char *)nwb_data)[1];
		dscp = (((ipv6_hdr & NRF_WIFI_FMAC_IPV6_TOS_MASK)
			 >> NRF_WIFI_FMAC_IPV6_TOS_SHIFT) & 0xfc);
		priority = dscp >> 5;
		break;
	/* If Media Independent (0x8917)
	 * frame calculate priority accordingly.
	 */
	case NRF_WIFI_FMAC_ETH_P_80221:
		/* 802.21 is always network control traffic */
		priority = 0x07;
		break;
	default:
		priority = 0;
	}

	return priority;
}

#ifdef NRF_WIFI_QOS_NOACK_POLICY
struct check_tid_info {
	int target_tid;
	bool tid_match_found;
};

static enum nrf_wifi_status check_tid_callbk_fn(void *callbk_data,
						void *nbuf)
{
	struct check_tid_info *info = NULL;
	int tid = 0;

	info = (struct check_tid_info *)callbk_data;

	tid = nrf_wifi_get_tid(nbuf);

	if (tid == info->target_tid) {
		info->tid_match_found = true;
	}

	return NRF_WIFI_STATUS_SUCCESS;
}

static bool has_matching_tid(sys_dlist_t *txq, int target_tid)
{
	struct check_tid_info info;
	struct nrf_wifi_nwb *nwb;

	info.target_tid = target_tid;
	info.tid_match_found = false;

	SYS_DLIST_FOR_EACH_CONTAINER(txq, nwb, queue_node) {
		(void)check_tid_callbk_fn(&info, nwb);
		if (info.tid_match_found) {
			return true;
		}
	}

	return false;
}
#endif /* NRF_WIFI_QOS_NOACK_POLICY */


int pending_frames_count(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			 int peer_id)
{
	int count = 0;
	int ac = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	for (ac = NRF_WIFI_FMAC_AC_VO; ac >= 0; --ac) {
		count += (unsigned int)sys_dlist_len(
			&sys_dev_ctx->tx_config.pend_pkt_q[peer_id][ac]);
	}

	return count;
}


static enum nrf_wifi_status update_pend_q_bmp(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				       unsigned int ac,
				       int peer_id)
{
	return NRF_WIFI_STATUS_SUCCESS;
}


static void tx_desc_free(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		  unsigned int desc,
		  int queue)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	int bit = -1;
	int pool_id = -1;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;

	fpriv = fmac_dev_ctx->fpriv;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fpriv);

	bit = (desc % TX_DESC_BUCKET_BOUND);
	pool_id = (desc / TX_DESC_BUCKET_BOUND);

	if (!(sys_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] & (1 << bit))) {
		return;
	}

	sys_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] &= (~(1 << bit));

	sys_dev_ctx->tx_config.outstanding_descs[queue]--;

	if (desc >= (sys_fpriv->num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX)) {
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
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	fpriv = fmac_dev_ctx->fpriv;
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fpriv);

	desc = sys_fpriv->num_tx_tokens;

	/* First search for a reserved desc */

	for (cnt = 0; cnt < sys_fpriv->num_tx_tokens_per_ac; cnt++) {
		curr_bit = ((queue + (NRF_WIFI_FMAC_AC_MAX * cnt)));
		curr_bit = ((queue + (NRF_WIFI_FMAC_AC_MAX * cnt)) % TX_DESC_BUCKET_BOUND);
		pool_id = ((queue + (NRF_WIFI_FMAC_AC_MAX * cnt)) / TX_DESC_BUCKET_BOUND);

		if ((((sys_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] >>
		       curr_bit)) & 1)) {
			continue;
		} else {
			sys_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] |=
				(1 << curr_bit);
			desc = queue + (NRF_WIFI_FMAC_AC_MAX * cnt);
			sys_dev_ctx->tx_config.outstanding_descs[queue]++;
			sys_dev_ctx->tx_config.token_stats.reserved_token_get[queue]++;
			break;
		}
	}

	/* If reserved desc is not found search for a spare desc
	 * (only for non beacon queues)
	 */
	if (cnt == sys_fpriv->num_tx_tokens_per_ac) {
		for (desc = sys_fpriv->num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX;
		     desc < sys_fpriv->num_tx_tokens;
		     desc++) {
			curr_bit = (desc % TX_DESC_BUCKET_BOUND);
			pool_id = (desc / TX_DESC_BUCKET_BOUND);

			if ((sys_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] >> curr_bit) & 1) {
				continue;
			} else {
				sys_dev_ctx->tx_config.buf_pool_bmp_p[pool_id] |=
					(1 << curr_bit);
				sys_dev_ctx->tx_config.outstanding_descs[queue]++;
				sys_dev_ctx->tx_config.token_stats.spare_token_get[queue]++;
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

	if (desc < NRF71_MAX_TX_TOKENS) {
		sys_dev_ctx->tx_config.token_stats.token_acq[desc]++;
		tx_stats_track_max(
			&sys_dev_ctx->tx_config.token_stats.max_outstanding_descs[queue],
			sys_dev_ctx->tx_config.outstanding_descs[queue]);
	} else {
		/* No token was available for this AC. This is the normal steady
		 * state while the tokens of the AC are in flight: the packet
		 * stays on the pending queue and is picked up from the TX done
		 * path. It is only an anomaly if the AC has nothing in flight.
		 */
		sys_dev_ctx->tx_config.token_stats.token_get_busy[queue]++;

		if (sys_dev_ctx->tx_config.outstanding_descs[queue] == 0) {
			sys_dev_ctx->tx_config.token_stats.token_get_starved[queue]++;
		}
	}

	return desc;
}


static int tx_aggr_check(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		  void *first_nwb,
		  int ac,
		  int peer)
{
	struct nrf_wifi_nwb *queue_nwb = NULL;
	sys_dlist_t *pending_pkt_queue = NULL;
	bool aggr = true;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (sys_dev_ctx->tx_config.peers[peer].is_legacy) {
		return false;
	}

#ifdef NRF71_RAW_DATA_TX
	if (nrf_wifi_nbuf_is_raw_tx(first_nwb)) {
		return false;
	}
#endif /* NRF71_RAW_DATA_TX */

	pending_pkt_queue = &sys_dev_ctx->tx_config.pend_pkt_q[peer][ac];

	if (sys_dlist_is_empty(pending_pkt_queue)) {
		return false;
	}

	queue_nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(pending_pkt_queue, queue_nwb, queue_node);

	if (queue_nwb) {
		if (!nrf_wifi_util_ether_addr_equal(nrf_wifi_get_dest(queue_nwb),
						    nrf_wifi_get_dest(first_nwb))) {
			aggr = false;
		}

		if (!nrf_wifi_util_ether_addr_equal(nrf_wifi_get_src(queue_nwb),
						    nrf_wifi_get_src(first_nwb))) {
			aggr = false;
		}
	}


	return aggr;
}


static int get_peer_from_wakeup_q(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			   unsigned int ac)
{
	int peer_id = -1;
	struct peers_info *peer = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	SYS_DLIST_FOR_EACH_CONTAINER(&sys_dev_ctx->tx_config.wakeup_client_q, peer,
				     wakeup_node) {
		if (peer->ps_token_count &&
		    !sys_dlist_is_empty(
			    &sys_dev_ctx->tx_config.pend_pkt_q[peer->peer_id][ac])) {
			peer->ps_token_count--;
			return peer->peer_id;
		}
	}

	return peer_id;
}


static int tx_curr_peer_opp_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			 unsigned int ac)
{
	unsigned int i = 0;
	unsigned int curr_peer_opp = 0;
	unsigned int init_peer_opp = 0;
	int peer_id = -1;
	unsigned char ps_state = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (ac == NRF_WIFI_FMAC_AC_MC) {
		return MAX_PEERS;
	}

	peer_id = get_peer_from_wakeup_q(fmac_dev_ctx, ac);

	if (peer_id != -1) {
		return peer_id;
	}

	init_peer_opp = sys_dev_ctx->tx_config.curr_peer_opp[ac];

	for (i = 0; i < MAX_PEERS; i++) {
		curr_peer_opp = (init_peer_opp + i) % MAX_PEERS;

		ps_state = sys_dev_ctx->tx_config.peers[curr_peer_opp].ps_state;

		if (ps_state == NRF_WIFI_CLIENT_PS_MODE) {
			continue;
		}

		if (!sys_dlist_is_empty(
			    &sys_dev_ctx->tx_config.pend_pkt_q[curr_peer_opp][ac])) {
			sys_dev_ctx->tx_config.curr_peer_opp[ac] =
				(curr_peer_opp + 1) % MAX_PEERS;
			break;
		}
	}

	if (i != MAX_PEERS) {
		peer_id = curr_peer_opp;
	}

	return peer_id;
}

static size_t _tx_pending_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			unsigned int desc,
			unsigned int ac)
{
	int len = 0;
	sys_dlist_t *pend_pkt_q = NULL;
	sys_dlist_t *txq = NULL;
	struct tx_pkt_info *pkt_info = NULL;
	int peer_id = -1;
	struct nrf_wifi_nwb *nwb = NULL;
	struct nrf_wifi_nwb *first_nwb = NULL;

	int max_txq_len, avail_ampdu_len_per_token;
	int ampdu_len = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	struct tx_token_stats *stats = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	max_txq_len = sys_fpriv->data_config.max_tx_aggregation;
	avail_ampdu_len_per_token = sys_fpriv->avail_ampdu_len_per_token;

#ifdef NRF71_RAW_DATA_TX
	struct nrf_wifi_nwb *pend_head = NULL;

	/* Check for Raw packets first, if not found, then check for
	 * regular packets.
	 */
	pend_pkt_q = &sys_dev_ctx->tx_config.pend_pkt_q[MAX_PEERS][ac];
	pend_head = SYS_DLIST_PEEK_HEAD_CONTAINER(pend_pkt_q, pend_head, queue_node);
	if (!(sys_dlist_len(pend_pkt_q) > 0 &&
	      nrf_wifi_nbuf_is_raw_tx(pend_head))) {
#endif
		peer_id = tx_curr_peer_opp_get(fmac_dev_ctx, ac);

		/* No pending frames for any peer in that AC. */
		if (peer_id == -1) {
			return 0;
		}

		pend_pkt_q = &sys_dev_ctx->tx_config.pend_pkt_q[peer_id][ac];
#ifdef NRF71_RAW_DATA_TX
	}
#endif

	if (sys_dlist_is_empty(pend_pkt_q)) {
		return 0;
	}

	pkt_info = &sys_dev_ctx->tx_config.pkt_info_p[desc];
	txq = &pkt_info->pkt_q;

	/* Aggregate Only MPDU's with same RA, same Rate,
	 * same Rate flags, same Tx Info flags
	 */
	first_nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(pend_pkt_q, first_nwb, queue_node);

	stats = &sys_dev_ctx->tx_config.token_stats;

	while (!sys_dlist_is_empty(pend_pkt_q)) {
		nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(pend_pkt_q, nwb, queue_node);

		ampdu_len += TX_BUF_HEADROOM +
			nrf_wifi_nbuf_data_size(nwb);

		if (ampdu_len >= avail_ampdu_len_per_token) {
			/* The token is size limited: it cannot hold this packet
			 * on top of what has already been added.
			 */
			stats->aggr_stop_size++;
			break;
		}

		if (!can_xmit(fmac_dev_ctx, nwb)) {
			stats->aggr_stop_twt++;
			break;
		}

		if (!tx_aggr_check(fmac_dev_ctx, first_nwb, ac, peer_id)) {
			stats->aggr_stop_mismatch++;
			break;
		}

		if (sys_dlist_len(txq) >= max_txq_len) {
			/* The token is count limited (CONFIG_NRF71_MAX_TX_AGGREGATION). */
			stats->aggr_stop_count++;
			break;
		}

		sys_dlist_remove(&nwb->queue_node);

		sys_dnode_init(&nwb->queue_node);
		sys_dlist_append(txq, &nwb->queue_node);
	}

	if (sys_dlist_len(pend_pkt_q) == 0) {
		/* Nothing more to aggregate: the host simply ran out of packets. */
		stats->aggr_stop_q_empty++;
	}

	/* If our criterion rejects all pending frames, or
	 * pend_q is empty, send only 1
	 */
	if (sys_dlist_is_empty(txq)) {
		stats->aggr_forced_single++;
		nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(pend_pkt_q, nwb, queue_node);

		if (!nwb || !can_xmit(fmac_dev_ctx, nwb)) {
			return 0;
		}

		sys_dlist_remove(&nwb->queue_node);

		sys_dnode_init(&nwb->queue_node);
		sys_dlist_append(txq, &nwb->queue_node);
	}

	len = (int)sys_dlist_len(txq);

	if (len > 0) {
		sys_dev_ctx->tx_config.pkt_info_p[desc].peer_id = peer_id;
	}

	update_pend_q_bmp(fmac_dev_ctx, ac, peer_id);

	return len;
}

#ifdef NRF71_RAW_DATA_TX
enum nrf_wifi_status rawtx_cmd_prep_callbk_fn(void *callbk_data,
					      void *nbuf)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned long nwb = 0;
	unsigned long nwb_data = 0;
	struct tx_cmd_prep_raw_info *info = NULL;
	struct nrf_wifi_cmd_raw_tx *config = NULL;
	unsigned int buf_len = 0;
	unsigned char frame_indx = 0;

	info = (struct tx_cmd_prep_raw_info *)callbk_data;
	config = info->raw_config;
	frame_indx = info->num_tx_pkts;

	nwb = (unsigned long)nbuf;

	nwb_data = (unsigned long)nrf_wifi_nbuf_data_get((void *)nwb);
	buf_len = nrf_wifi_nbuf_data_size((void *)nwb);

	config->raw_tx_info.pkt_length[frame_indx] = buf_len;
	LOG_DBG("%s: frame pointer for data is 0x%lx", __func__, nwb_data);
	config->raw_tx_info.frame_ddr_pointer[frame_indx] = (unsigned long long)nwb_data;
	info->num_tx_pkts++;

	status = NRF_WIFI_STATUS_SUCCESS;
	return status;
}
#endif /* NRF71_RAW_DATA_TX */

static enum nrf_wifi_status tx_cmd_prep_callbk_fn(void *callbk_data,
					   void *nbuf)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	unsigned long nwb = 0;
	unsigned long nwb_data = 0;
	struct tx_cmd_prep_info *info = NULL;
	struct nrf_wifi_tx_buff *config = NULL;
	unsigned int buf_len = 0;
	unsigned char frame_indx = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	info = (struct tx_cmd_prep_info *)callbk_data;
	fmac_dev_ctx = info->fmac_dev_ctx;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	config = info->config;
	frame_indx = config->num_tx_pkts;

	nwb = (unsigned long)nbuf;

	nwb_data = (unsigned long)nrf_wifi_nbuf_data_get((void *)nwb);

	buf_len = nrf_wifi_nbuf_data_size((void *)nwb);
	info->total_data_len += buf_len;
	config->tx_buff_info[frame_indx].ddr_ptr =
		(unsigned long long)nwb_data;
	config->tx_buff_info[frame_indx].pkt_length = buf_len;
	if (!nrf_wifi_nbuf_get_chksum_done(nbuf)) {
		config->csum_bitmap |= (1u << frame_indx);
	}
	config->num_tx_pkts++;

	status = NRF_WIFI_STATUS_SUCCESS;
	return status;
}

#ifdef NRF71_RAW_DATA_TX
enum nrf_wifi_status rawtx_cmd_prepare(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				       struct host_rpu_msg *umac_cmd,
				       int desc,
				       sys_dlist_t *txq,
				       int peer_id)
{
	struct nrf_wifi_cmd_raw_tx *config = NULL;
	struct tx_cmd_prep_raw_info info;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned char vif_id;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_nwb *nwb;
	unsigned int txq_len;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	vif_id = sys_dev_ctx->tx_config.peers[peer_id].if_idx;
	vif_ctx = sys_dev_ctx->vif_ctx[vif_id];

	txq_len = (unsigned int)sys_dlist_len(txq);

	if (txq_len == 0) {
		LOG_ERR("%s: txq_len = %d",
				      __func__,
				      txq_len);
		goto err;
	}

	struct nrf_wifi_nwb *first_nwb;

	first_nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(txq, first_nwb, queue_node);

	sys_dev_ctx->tx_config.send_pkt_coalesce_count_p[desc] = txq_len;
	config = (struct nrf_wifi_cmd_raw_tx *)(umac_cmd->msg);

	config->sys_head.cmd_event = NRF_WIFI_CMD_RAW_TX_PKT;
	config->sys_head.len = sizeof(*config);
	config->if_index = vif_id;
	config->raw_tx_info.desc_num = desc;

	/* Check first packet in queue for per-packet raw TX config */
	struct raw_tx_pkt_header *raw_tx_hdr = NULL;

	if (first_nwb && nrf_wifi_nbuf_is_raw_tx(first_nwb)) {
		raw_tx_hdr = nrf_wifi_nbuf_get_raw_tx_hdr(first_nwb);
		if (raw_tx_hdr) {
			config->raw_tx_info.queue_num = raw_tx_hdr->queue;
			config->raw_tx_info.rate = raw_tx_hdr->data_rate;
			config->raw_tx_info.rate_flags = raw_tx_hdr->tx_mode;
		}
	}

	info.fmac_dev_ctx = fmac_dev_ctx;
	info.raw_config = config;
	info.num_tx_pkts = 0;

	SYS_DLIST_FOR_EACH_CONTAINER(txq, nwb, queue_node) {
		status = rawtx_cmd_prep_callbk_fn(&info, nwb);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: failed", __func__);
			goto err;
		}
	}

	config->raw_tx_info.num_frames = info.num_tx_pkts;
	config->raw_tx_info.aggregation = AGGR_ENABLE;
	sys_dev_ctx->host_stats.total_tx_pkts += info.num_tx_pkts;

	return NRF_WIFI_STATUS_SUCCESS;
err:
	return NRF_WIFI_STATUS_FAIL;
}
#endif /* NRF71_RAW_DATA_TX */

/* Account one TX command against the token (descriptor) that carries it.
 *
 * "occupancy" is what the aggregation logic in _tx_pending_process() budgets
 * against the per-token size cap (avail_ampdu_len_per_token): the packet data
 * plus TX_BUF_HEADROOM for every packet. Comparing it against that cap shows
 * how well each token is filled, which is the real limit for large packets,
 * while max_tx_aggregation is the limit for small ones.
 */
static void tx_cmd_stats_update(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				unsigned int desc,
				unsigned int num_pkts,
				unsigned int data_len)
{
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);
	struct tx_token_stats *stats = &sys_dev_ctx->tx_config.token_stats;
	unsigned int occupancy = data_len + (num_pkts * TX_BUF_HEADROOM);
	unsigned int cap = sys_fpriv->avail_ampdu_len_per_token;
	unsigned int bucket;

	stats->tx_cmds++;
	stats->tx_cmd_pkts += num_pkts;
	stats->tx_cmd_bytes += occupancy;
	stats->tx_cmd_data_bytes += data_len;
	tx_stats_track_max(&stats->max_cmd_bytes, occupancy);

	if ((num_pkts > 0) && (num_pkts <= MAX_TX_AGG_SIZE)) {
		stats->pkts_per_cmd[num_pkts - 1]++;
	}

	if (cap > 0) {
		bucket = (occupancy * TX_TOKEN_FILL_BUCKETS) / cap;
		if (bucket >= TX_TOKEN_FILL_BUCKETS) {
			bucket = TX_TOKEN_FILL_BUCKETS - 1;
		}
		stats->fill_hist[bucket]++;
	}

	if (desc >= NRF71_MAX_TX_TOKENS) {
		return;
	}

	stats->token_cmds[desc]++;
	stats->token_pkts[desc] += num_pkts;
	stats->token_bytes[desc] += occupancy;
	tx_stats_track_max(&stats->token_max_pkts[desc], num_pkts);
	tx_stats_track_max(&stats->token_max_bytes[desc], occupancy);

	/* Start of the in-flight window of this command. */
	stats->token_issue_cyc[desc] = k_cycle_get_32();

	if (stats->window_start_cyc == 0) {
		stats->window_start_cyc = stats->token_issue_cyc[desc];
	}

	if (stats->cmds_in_flight == 0) {
		unsigned int now = k_cycle_get_32();

		if (stats->pipe_transition_cyc != 0) {
			stats->pipe_idle_us +=
				k_cyc_to_us_floor32(now - stats->pipe_transition_cyc);
		}
		stats->pipe_transition_cyc = now;
	}

	stats->cmds_in_flight++;
	tx_stats_track_max(&stats->max_cmds_in_flight, stats->cmds_in_flight);
}

/* Close the in-flight window of the command on this token. */
static void tx_done_stats_update(struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx,
				 unsigned int desc)
{
	struct tx_token_stats *stats = &sys_dev_ctx->tx_config.token_stats;
	unsigned int now = k_cycle_get_32();
	unsigned int inflight_us;

	if ((desc >= NRF71_MAX_TX_TOKENS) || (stats->token_issue_cyc[desc] == 0)) {
		return;
	}

	inflight_us = k_cyc_to_us_floor32(now - stats->token_issue_cyc[desc]);
	stats->token_issue_cyc[desc] = 0;
	stats->token_inflight_us[desc] += inflight_us;
	tx_stats_track_max(&stats->token_max_inflight_us[desc], inflight_us);

	if (stats->cmds_in_flight == 0) {
		return;
	}

	stats->cmds_in_flight--;

	if (stats->cmds_in_flight == 0) {
		if (stats->pipe_transition_cyc != 0) {
			stats->pipe_busy_us +=
				k_cyc_to_us_floor32(now - stats->pipe_transition_cyc);
		}
		stats->pipe_transition_cyc = now;
	}
}

static enum nrf_wifi_status tx_cmd_prepare(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		   struct host_rpu_msg *umac_cmd,
		   int desc,
		   sys_dlist_t *txq,
		   int peer_id)
{
	struct nrf_wifi_tx_buff *config = NULL;
	int len = 0;
	struct nrf_wifi_nwb *nwb = NULL;
	void *nwb_data = NULL;
	unsigned char *data = NULL;
	struct tx_cmd_prep_info info;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	unsigned char vif_id;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	unsigned int txq_len;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	vif_id = sys_dev_ctx->tx_config.peers[peer_id].if_idx;
	vif_ctx = sys_dev_ctx->vif_ctx[vif_id];

	txq_len = (unsigned int)sys_dlist_len(txq);

	if (txq_len == 0) {
		LOG_ERR("%s: txq_len = %d",
				      __func__,
				      txq_len);
		goto err;
	}

	nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(txq, nwb, queue_node);

	sys_dev_ctx->tx_config.send_pkt_coalesce_count_p[desc] = txq_len;

	config = (struct nrf_wifi_tx_buff *)(umac_cmd->msg);

	data = nrf_wifi_nbuf_data_get(nwb);

	len = nrf_wifi_nbuf_data_size(nwb);

	config->umac_head.cmd = NRF_WIFI_CMD_TX_BUFF;

	config->umac_head.len += sizeof(struct nrf_wifi_tx_buff);
	config->umac_head.len += sizeof(struct nrf_wifi_tx_buff_info) * txq_len;

	config->tx_desc_num = desc;

	nrf_wifi_mem_cpy(config->mac_hdr_info.dest,
			      nrf_wifi_get_dest(nwb),
			      NRF_WIFI_ETH_ADDR_LEN);

	nrf_wifi_mem_cpy(config->mac_hdr_info.src,
			      nrf_wifi_get_src(nwb),
			      NRF_WIFI_ETH_ADDR_LEN);

	nwb_data = nrf_wifi_nbuf_data_get(nwb);
	config->mac_hdr_info.etype =
		nrf_wifi_util_tx_get_eth_type(nwb_data);

	config->mac_hdr_info.tx_flags =
		nrf_wifi_get_tid(nwb) & NRF_WIFI_TX_FLAGS_DSCP_TOS_MASK;

	if (is_twt_emergency_pkt(nwb)) {
		config->mac_hdr_info.tx_flags |= NRF_WIFI_TX_FLAG_TWT_EMERGENCY_TX;
	}

	if (nrf_wifi_nbuf_get_chksum_done(nwb)) {
		config->mac_hdr_info.tx_flags |= NRF_WIFI_TX_FLAG_CHKSUM_AVAILABLE;
	}

	config->csum_bitmap = 0;

#ifdef NRF_WIFI_QOS_NOACK_POLICY
	if (has_matching_tid(txq, NRF_WIFI_QOS_NOACK_POLICY_TID)) {
		config->mac_hdr_info.tx_flags |= NRF_WIFI_TX_FLAG_QOS_CTL_ACK_POLICY_NOACK;
	}
#endif /* NRF_WIFI_QOS_NOACK_POLICY */

	config->num_tx_pkts = 0;

	info.fmac_dev_ctx = fmac_dev_ctx;
	info.config = config;
	info.total_data_len = 0;

	SYS_DLIST_FOR_EACH_CONTAINER(txq, nwb, queue_node) {
		status = tx_cmd_prep_callbk_fn(&info, nwb);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: build_mac80211_hdr failed", __func__);
			goto err;
		}
	}

	sys_dev_ctx->host_stats.total_tx_pkts += config->num_tx_pkts;

	tx_cmd_stats_update(fmac_dev_ctx,
			    desc,
			    config->num_tx_pkts,
			    info.total_data_len);

	config->wdev_id = sys_dev_ctx->tx_config.peers[peer_id].if_idx;

	if ((vif_ctx->if_type == NRF_WIFI_IFTYPE_AP ||
	    vif_ctx->if_type == NRF_WIFI_IFTYPE_AP_VLAN ||
	    vif_ctx->if_type == NRF_WIFI_IFTYPE_MESH_POINT) &&
		pending_frames_count(fmac_dev_ctx, peer_id) != 0) {
		config->mac_hdr_info.more_data = 1;
	}

	if (sys_dev_ctx->tx_config.peers[peer_id].ps_token_count == 0) {
		sys_dnode_t *wakeup_node = &sys_dev_ctx->tx_config.peers[peer_id].wakeup_node;

		if (sys_dnode_is_linked(wakeup_node)) {
			sys_dlist_remove(wakeup_node);
		}

		config->mac_hdr_info.eosp = 1;

	} else {
		config->mac_hdr_info.eosp = 0;
	}

	return NRF_WIFI_STATUS_SUCCESS;
err:
	return NRF_WIFI_STATUS_FAIL;
}

#ifdef NRF71_RAW_DATA_TX
enum nrf_wifi_status rawtx_cmd_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				    sys_dlist_t *txq,
				    int desc,
				    int peer_id)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	unsigned int len = 0;
	unsigned int txq_len;
	struct nrf_wifi_nwb *nwb = NULL;

	txq_len = (unsigned int)sys_dlist_len(txq);

	len += sizeof(struct nrf_wifi_cmd_raw_tx);
	len *= txq_len;

	umac_cmd = umac_cmd_alloc(fmac_dev_ctx,
				  NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
				  len);

	status = rawtx_cmd_prepare(fmac_dev_ctx,
				   umac_cmd,
				   desc,
				   txq,
				   peer_id);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: rawtx_cmd_prepare failed",
				      __func__);

		goto out;
	}

	status = nrf_wifi_ipc_cmd_send(fmac_dev_ctx,
				       umac_cmd,
				       (sizeof(*umac_cmd) + len));

	while (!sys_dlist_is_empty(txq)) {
		nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(txq, nwb, queue_node);
		sys_dlist_remove(&nwb->queue_node);
		nrf_wifi_nbuf_free(nwb);
	}
out:
	return status;
}
#endif /* NRF71_RAW_DATA_TX */

enum nrf_wifi_status tx_cmd_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				 sys_dlist_t *txq,
				 int desc,
				 int peer_id)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct host_rpu_msg *umac_cmd = NULL;
	unsigned int len = 0;
	unsigned int txq_len;

	txq_len = (unsigned int)sys_dlist_len(txq);

	len += sizeof(struct nrf_wifi_tx_buff_info);
	len *= txq_len;

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
		LOG_ERR("%s: tx_cmd_prepare failed",
				      __func__);

		goto out;
	}

	status = nrf_wifi_ipc_cmd_send(fmac_dev_ctx,
				       umac_cmd,
				       sizeof(*umac_cmd) + len);
out:
	return status;
}


enum nrf_wifi_status tx_pending_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					unsigned int desc,
					unsigned int ac)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_nwb *first_nwb = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!fmac_dev_ctx) {
		LOG_ERR("%s: Invalid params",
				      __func__);
		goto out;
	}

	if (_tx_pending_process(fmac_dev_ctx, desc, ac)) {
		first_nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(
			&sys_dev_ctx->tx_config.pkt_info_p[desc].pkt_q, first_nwb, queue_node);
		/* Should never happen, but just in case */
		if (!first_nwb) {
			LOG_ERR("%s: No pending packets in txq",
					      __func__);
			goto out;
		}
#ifdef NRF71_RAW_DATA_TX
		if (nrf_wifi_nbuf_is_raw_tx(first_nwb)) {
			status = rawtx_cmd_init(fmac_dev_ctx,
						&sys_dev_ctx->tx_config.pkt_info_p[desc].pkt_q,
						desc,
						sys_dev_ctx->tx_config.pkt_info_p[desc].peer_id);
		} else {
#endif
			status = tx_cmd_init(fmac_dev_ctx,
					     &sys_dev_ctx->tx_config.pkt_info_p[desc].pkt_q,
					     desc,
					     sys_dev_ctx->tx_config.pkt_info_p[desc].peer_id);
#ifdef NRF71_RAW_DATA_TX
		}
#endif
	} else {
		tx_desc_free(fmac_dev_ctx,
			     desc,
			     ac);

		status = NRF_WIFI_STATUS_SUCCESS;
	}

out:
	return status;
}


static enum nrf_wifi_status tx_enqueue(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				void *nwb,
				unsigned int ac,
				unsigned int peer_id)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	sys_dlist_t *queue = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_nwb *nwb_ptr = nwb;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!fmac_dev_ctx || !nwb) {
		LOG_ERR("%s: Invalid params",
				      __func__);
		goto out;
	}

	queue = &sys_dev_ctx->tx_config.pend_pkt_q[peer_id][ac];

	if (sys_dlist_len(queue) >= NRF71_MAX_TX_PENDING_QLEN) {
		sys_dev_ctx->tx_config.token_stats.pend_q_full_drops[ac]++;
		goto out;
	}

	sys_dnode_init(&nwb_ptr->queue_node);

	if (is_twt_emergency_pkt(nwb)) {
		sys_dlist_prepend(queue, &nwb_ptr->queue_node);
	} else {
		sys_dlist_append(queue, &nwb_ptr->queue_node);
	}

	tx_stats_track_max(&sys_dev_ctx->tx_config.token_stats.max_pending_qlen[ac],
			   (unsigned int)sys_dlist_len(queue) + 1);

	status = update_pend_q_bmp(fmac_dev_ctx, ac, peer_id);

out:
	return status;
}


static enum nrf_wifi_fmac_tx_status tx_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				unsigned char if_idx,
				void *nbuf,
				unsigned int ac,
				unsigned int peer_id)
{
	enum nrf_wifi_fmac_tx_status status;
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	sys_dlist_t *pend_pkt_q = NULL;
	struct nrf_wifi_nwb *first_nwb = NULL;
	unsigned char ps_state = 0;
	bool aggr_status = false;
	int max_cmds = 0;

	fpriv = fmac_dev_ctx->fpriv;
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fpriv);

	status = (enum nrf_wifi_fmac_tx_status)tx_enqueue(fmac_dev_ctx,
						  nbuf,
						  ac,
						  peer_id);

	if (status != NRF_WIFI_FMAC_TX_STATUS_SUCCESS) {
		goto err;
	}

	ps_state = sys_dev_ctx->tx_config.peers[peer_id].ps_state;

	if (ps_state == NRF_WIFI_CLIENT_PS_MODE) {
		goto out;
	}

	pend_pkt_q = &sys_dev_ctx->tx_config.pend_pkt_q[peer_id][ac];

	/* If outstanding_descs for a particular
	 * access category >= NUM_TX_DESCS_PER_AC means there are already
	 * pending packets for that access category. So now see if frames
	 * can be aggregated depending upon access category depending
	 * upon SA, RA & AC
	 */

	if ((sys_dev_ctx->tx_config.outstanding_descs[ac]) >= sys_fpriv->num_tx_tokens_per_ac) {
		if (!sys_dlist_is_empty(pend_pkt_q)) {
			first_nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(pend_pkt_q, first_nwb,
								  queue_node);

			aggr_status = true;

			if (!nrf_wifi_util_ether_addr_equal(nrf_wifi_get_dest(nbuf),
							    nrf_wifi_get_dest(first_nwb))) {
				aggr_status = false;
			}

			if (!nrf_wifi_util_ether_addr_equal(nrf_wifi_get_src(nbuf),
							    nrf_wifi_get_src(first_nwb))) {
				aggr_status = false;
			}
		}

		if (aggr_status) {
			max_cmds = sys_fpriv->data_config.max_tx_aggregation;

			if (sys_dlist_len(pend_pkt_q) < max_cmds) {
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
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	/* Determine the Queue from the descriptor */
	/* Reserved desc */
	if (desc < (sys_fpriv->num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX)) {
		tx_done_q = (desc % NRF_WIFI_FMAC_AC_MAX);
		start_ac = end_ac = tx_done_q;
	} else {
		/* Derive the queue here as it is not given by UMAC. */
		if (desc >= (sys_fpriv->num_tx_tokens_per_ac * NRF_WIFI_FMAC_AC_MAX)) {
			tx_done_spare_desc_q_map = get_spare_desc_q_map(fmac_dev_ctx, desc);

			if (tx_done_spare_desc_q_map & (1 << NRF_WIFI_FMAC_AC_BK)) {
				tx_done_q = NRF_WIFI_FMAC_AC_BK;
			} else if (tx_done_spare_desc_q_map & (1 << NRF_WIFI_FMAC_AC_BE)) {
				tx_done_q = NRF_WIFI_FMAC_AC_BE;
			} else if (tx_done_spare_desc_q_map & (1 << NRF_WIFI_FMAC_AC_VI)) {
				tx_done_q = NRF_WIFI_FMAC_AC_VI;
			} else if (tx_done_spare_desc_q_map & (1 << NRF_WIFI_FMAC_AC_VO)) {
				tx_done_q = NRF_WIFI_FMAC_AC_VO;
			}
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
				sys_dev_ctx->tx_config.outstanding_descs[tx_done_q]--;
				sys_dev_ctx->tx_config.outstanding_descs[*ac]++;
				sys_dev_ctx->tx_config.token_stats.spare_token_ac_switch++;

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


static enum nrf_wifi_status tx_done_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				     unsigned char tx_desc_num)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	unsigned int desc = 0;
	struct tx_pkt_info *pkt_info = NULL;
	unsigned int pkt = 0;
	unsigned int pkts_pending = 0;
	unsigned char queue = 0;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	struct nrf_wifi_nwb *nwb = NULL;

	fpriv = fmac_dev_ctx->fpriv;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	desc = tx_desc_num;

	if (desc > sys_fpriv->num_tx_tokens) {
		LOG_ERR("Invalid desc");
		goto out;
	}

	pkt_info = &sys_dev_ctx->tx_config.pkt_info_p[desc];

	sys_dev_ctx->tx_config.token_stats.tx_dones++;
	tx_done_stats_update(sys_dev_ctx, desc);

	pkt = 0;

	sys_dev_ctx->host_stats.total_tx_done_pkts += pkt;

	while (!sys_dlist_is_empty(&pkt_info->pkt_q)) {
		nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(&pkt_info->pkt_q, nwb, queue_node);
		sys_dlist_remove(&nwb->queue_node);
		nrf_wifi_nbuf_free(nwb);
	}

	pkts_pending = tx_buff_req_free(fmac_dev_ctx, tx_desc_num, &queue);

	if (pkts_pending) {
#ifdef NRF71_RAW_DATA_TX
		struct nrf_wifi_fmac_vif_ctx *vif_ctx;
		unsigned char if_idx;

		pkt_info = &sys_dev_ctx->tx_config.pkt_info_p[desc];

		/**
		 * we need to peek into the pending buffer to determine if
		 * packet is a raw packet or not
		 */
		nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(&pkt_info->pkt_q, nwb, queue_node);

		if (!nrf_wifi_nbuf_is_raw_tx(nwb)) {
#endif /* NRF71_RAW_DATA_TX */
			if (sys_dev_ctx->twt_sleep_status ==
			    NRF_WIFI_FMAC_TWT_STATE_AWAKE) {
				pkt_info = &sys_dev_ctx->tx_config.pkt_info_p[desc];
				status = tx_cmd_init(fmac_dev_ctx,
						     &pkt_info->pkt_q,
						     desc,
						     pkt_info->peer_id);
			} else {
				status = NRF_WIFI_STATUS_SUCCESS;
			}
#ifdef NRF71_RAW_DATA_TX
		} else {
			/**
			 * check if the if_type is STA_TX_INJECTOR
			 * if so, we need to check for TWT_SLEEP.
			 * for RAW TX, we use MAX-PEERS queue presently
			 */
			if_idx = sys_dev_ctx->tx_config.peers[MAX_PEERS].if_idx;
			vif_ctx = sys_dev_ctx->vif_ctx[if_idx];
			if ((vif_ctx->if_type == NRF_WIFI_STA_TX_INJECTOR) &&
			    (sys_dev_ctx->twt_sleep_status == NRF_WIFI_FMAC_TWT_STATE_SLEEP)) {
				status = NRF_WIFI_STATUS_SUCCESS;
			} else {
				status = rawtx_cmd_init(fmac_dev_ctx,
							&pkt_info->pkt_q,
							desc,
							pkt_info->peer_id);
			}
		}
#endif /* NRF71_RAW_DATA_TX */
	} else {
		status = NRF_WIFI_STATUS_SUCCESS;
	}
out:
	return status;
}

#ifdef NRF71_TX_DONE_WQ_ENABLED
static void tx_done_work_handler(struct k_work *work)
{
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx;
	struct nrf_wifi_fmac_tx_done_node *item;
	sys_dnode_t *node;

	sys_dev_ctx = CONTAINER_OF(work, struct nrf_wifi_sys_fmac_dev_ctx, tx_done_work);
	fmac_dev_ctx = (struct nrf_wifi_fmac_dev_ctx *)((uintptr_t)sys_dev_ctx -
		offsetof(struct nrf_wifi_fmac_dev_ctx, priv));

	nrf_wifi_ipc_rx_lock(fmac_dev_ctx);
	if (!nrf_wifi_ipc_rx_enabled(fmac_dev_ctx)) {
		goto out;
	}

	if (sys_dlist_is_empty(&sys_dev_ctx->tx_config.tx_done_event_q)) {
		LOG_ERR("%s: TX done event Q is empty", __func__);
		goto out;
	}

	node = sys_dlist_peek_head(&sys_dev_ctx->tx_config.tx_done_event_q);
	item = CONTAINER_OF(node, struct nrf_wifi_fmac_tx_done_node, node);
	sys_dlist_remove(node);

	(void)nrf_wifi_fmac_tx_done_event_process(fmac_dev_ctx, &item->buff);

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, item);
out:
	nrf_wifi_ipc_rx_unlock(fmac_dev_ctx);
}
#endif /* NRF71_TX_DONE_WQ_ENABLED */

#ifdef NRF71_RAW_DATA_TX
enum nrf_wifi_status nrf_wifi_fmac_rawtx_done_event_process(
		     struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		     struct nrf_wifi_event_raw_tx_done *config)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!fmac_dev_ctx || !config) {
		LOG_ERR("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	if (!sys_dev_ctx || !sys_dev_ctx->tx_config.tx_inited) {
		/* This is a valid case when the TX_DONE event is received
		 * during the driver deinit, so, silently ignore the failure.
		 */
		return NRF_WIFI_STATUS_SUCCESS;
	}

	k_mutex_lock(&sys_dev_ctx->tx_config.tx_lock, K_FOREVER);

	if (config->status == NRF_WIFI_STATUS_FAIL) {
		/**
		 * If the status indicates failure,
		 * increment raw TX failure count. The TX buffers
		 * still need to be freed.
		 */
		sys_dev_ctx->raw_pkt_stats.raw_pkt_send_failure += 1;
	}

	status = tx_done_process(fmac_dev_ctx,
				 config->desc_num);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Process raw tx done failed",
				      __func__);
		goto unlock;
	}
unlock:
	k_mutex_unlock(&sys_dev_ctx->tx_config.tx_lock);
out:
	return status;
}
#endif

enum nrf_wifi_status (nrf_wifi_fmac_tx_done_event_process)(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct nrf_wifi_tx_buff_done *buff)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	if (!fmac_dev_ctx || !buff) {
		LOG_ERR("%s: Invalid parameters",
				      __func__);
		goto out;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	if (!sys_dev_ctx || !sys_dev_ctx->tx_config.tx_inited) {
		/* This is a valid case when the TX_DONE event is received
		 * during the driver deinit, so, silently ignore the failure.
		 */
		return NRF_WIFI_STATUS_SUCCESS;
	}

	k_mutex_lock(&sys_dev_ctx->tx_config.tx_lock, K_FOREVER);

	status = tx_done_process(fmac_dev_ctx,
				 buff->tx_desc_num);

	k_mutex_unlock(&sys_dev_ctx->tx_config.tx_lock);

out:
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed",
				      __func__);
	}

	return status;
}


static enum nrf_wifi_fmac_tx_status nrf_wifi_fmac_tx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				      int if_id,
				      void *nbuf,
				      unsigned int ac,
				      unsigned int peer_id)
{
	enum nrf_wifi_fmac_tx_status status = NRF_WIFI_FMAC_TX_STATUS_FAIL;
	unsigned int desc = 0;
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;

	fpriv = fmac_dev_ctx->fpriv;
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fpriv);

	k_mutex_lock(&sys_dev_ctx->tx_config.tx_lock, K_FOREVER);

	if (sys_fpriv->num_tx_tokens == 0) {
		goto out;
	}

	status = tx_process(fmac_dev_ctx,
			    if_id,
			    nbuf,
			    ac,
			    peer_id);

	if (status != NRF_WIFI_FMAC_TX_STATUS_SUCCESS) {
		if (status == NRF_WIFI_FMAC_TX_STATUS_QUEUED) {
			/* Held back in the host, e.g. waiting for more packets
			 * to aggregate or peer in power save.
			 */
			sys_dev_ctx->tx_config.token_stats.pkts_queued++;
		}
		goto out;
	}

	status = NRF_WIFI_FMAC_TX_STATUS_QUEUED;

	if (!can_xmit(fmac_dev_ctx, nbuf)) {
		sys_dev_ctx->tx_config.token_stats.pkts_queued++;
		goto out;
	}

	desc = tx_desc_get(fmac_dev_ctx, ac);

	if (desc == sys_fpriv->num_tx_tokens) {
		sys_dev_ctx->tx_config.token_stats.pkts_queued++;
		goto out;
	}

	status = (enum nrf_wifi_fmac_tx_status)tx_pending_process(fmac_dev_ctx,
					desc,
					ac);
out:
	k_mutex_unlock(&sys_dev_ctx->tx_config.tx_lock);

	return status;
}


enum nrf_wifi_status tx_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	unsigned int i = 0;
	unsigned int j = 0;

	if (!fmac_dev_ctx) {
		goto out;
	}

	fpriv = fmac_dev_ctx->fpriv;
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	sys_fpriv = wifi_fmac_priv(fpriv);

	sys_dev_ctx->tx_config.send_pkt_coalesce_count_p =
		nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, (sizeof(unsigned int) *
					  sys_fpriv->num_tx_tokens));

	if (!sys_dev_ctx->tx_config.send_pkt_coalesce_count_p) {
		LOG_ERR("%s: Unable to allocate send_pkt_coalesce_count_p",
				      __func__);
		goto out;
	}

	for (i = 0; i < NRF_WIFI_FMAC_AC_MAX; i++) {
		for (j = 0; j < MAX_SW_PEERS; j++) {
			sys_dlist_init(&sys_dev_ctx->tx_config.pend_pkt_q[j][i]);
		}

		sys_dev_ctx->tx_config.outstanding_descs[i] = 0;
	}

	/* Used to store the address of tx'ed skb and len of 802.11 hdr
	 * it will be used in tx complete.
	 */
	sys_dev_ctx->tx_config.pkt_info_p =
		nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL,
				    (sizeof(struct tx_pkt_info) *
				     sys_fpriv->num_tx_tokens));

	if (!sys_dev_ctx->tx_config.pkt_info_p) {
		LOG_ERR("%s: Unable to allocate pkt_info_p",
				      __func__);
		goto coal_q_free;
	}

	for (i = 0; i < sys_fpriv->num_tx_tokens; i++) {
		sys_dlist_init(&sys_dev_ctx->tx_config.pkt_info_p[i].pkt_q);
	}

	for (j = 0; j < NRF_WIFI_FMAC_AC_MAX; j++) {
		sys_dev_ctx->tx_config.curr_peer_opp[j] = 0;
	}

	sys_dev_ctx->tx_config.buf_pool_bmp_p =
		nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, (sizeof(unsigned long) *
					 (sys_fpriv->num_tx_tokens/TX_DESC_BUCKET_BOUND) + 1));

	if (!sys_dev_ctx->tx_config.buf_pool_bmp_p) {
		LOG_ERR("%s: Unable to allocate buf_pool_bmp_p",
				      __func__);
		goto tx_pkt_info_free;
	}

	nrf_wifi_mem_set(sys_dev_ctx->tx_config.buf_pool_bmp_p,
			      0,
			      sizeof(long)*((sys_fpriv->num_tx_tokens/TX_DESC_BUCKET_BOUND) + 1));

	for (i = 0; i <= MAX_PEERS; i++) {
		sys_dev_ctx->tx_config.peers[i].peer_id = -1;
		sys_dnode_init(&sys_dev_ctx->tx_config.peers[i].wakeup_node);
	}

	sys_dlist_init(&sys_dev_ctx->tx_config.wakeup_client_q);

	sys_dev_ctx->twt_sleep_status = NRF_WIFI_FMAC_TWT_STATE_AWAKE;

#ifdef NRF71_TX_DONE_WQ_ENABLED
	sys_dlist_init(&sys_dev_ctx->tx_config.tx_done_event_q);
	k_work_init(&sys_dev_ctx->tx_done_work, tx_done_work_handler);
#endif /* NRF71_TX_DONE_WQ_ENABLED */
	k_mutex_init(&sys_dev_ctx->tx_config.tx_lock);
	sys_dev_ctx->tx_config.tx_inited = true;
	return NRF_WIFI_STATUS_SUCCESS;
tx_pkt_info_free:
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, sys_dev_ctx->tx_config.pkt_info_p);
coal_q_free:
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL,
			  sys_dev_ctx->tx_config.send_pkt_coalesce_count_p);
out:
	return NRF_WIFI_STATUS_FAIL;
}


void tx_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_fmac_priv *fpriv = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;
	unsigned int i = 0;
	unsigned int j = 0;

	fpriv = fmac_dev_ctx->fpriv;

	sys_fpriv = wifi_fmac_priv(fpriv);
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	sys_dev_ctx->tx_config.tx_inited = false;

#ifdef NRF71_TX_DONE_WQ_ENABLED
	struct k_work_sync sync;

	k_work_cancel_sync(&sys_dev_ctx->tx_done_work, &sync);
#endif /* NRF71_TX_DONE_WQ_ENABLED */

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, sys_dev_ctx->tx_config.buf_pool_bmp_p);

	for (i = 0; i < sys_fpriv->num_tx_tokens; i++) {
		if (sys_dev_ctx->tx_config.pkt_info_p) {
			struct nrf_wifi_nwb *nwb;

			while (!sys_dlist_is_empty(&sys_dev_ctx->tx_config.pkt_info_p[i].pkt_q)) {
				nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(
					&sys_dev_ctx->tx_config.pkt_info_p[i].pkt_q, nwb,
					queue_node);
				sys_dlist_remove(&nwb->queue_node);
				nrf_wifi_nbuf_free(nwb);
			}
		}
	}

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, sys_dev_ctx->tx_config.pkt_info_p);

	for (i = 0; i < NRF_WIFI_FMAC_AC_MAX; i++) {
		for (j = 0; j < MAX_SW_PEERS; j++) {
			struct nrf_wifi_nwb *nwb;

			while (!sys_dlist_is_empty(&sys_dev_ctx->tx_config.pend_pkt_q[j][i])) {
				nwb = SYS_DLIST_PEEK_HEAD_CONTAINER(
					&sys_dev_ctx->tx_config.pend_pkt_q[j][i], nwb,
					queue_node);
				sys_dlist_remove(&nwb->queue_node);
				nrf_wifi_nbuf_free(nwb);
			}
		}
	}

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL,
			  sys_dev_ctx->tx_config.send_pkt_coalesce_count_p);

	nrf_wifi_mem_set(&sys_dev_ctx->tx_config,
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


#ifdef NRF71_RAW_DATA_TX
static bool nrf_wifi_raw_pkt_mode_enabled(struct nrf_wifi_fmac_vif_ctx *vif)
{
	if ((vif->if_type == NRF_WIFI_STA_TX_INJECTOR) ||
	    (vif->if_type == NRF_WIFI_MONITOR_TX_INJECTOR) ||
	    (vif->if_type == NRF_WIFI_STA_PROMISC_TX_INJECTOR)) {
		return true;
	}
	return false;
}

enum nrf_wifi_status nrf_wifi_fmac_start_rawpkt_xmit(void *dev_ctx,
						     unsigned char if_idx,
						     void *nwb)
{
	enum nrf_wifi_fmac_tx_status tx_status = NRF_WIFI_FMAC_TX_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	struct raw_tx_pkt_header *raw_tx_hdr = NULL;
	int ac;
	int peer_id;

	if (!nwb || !dev_ctx) {
		/**
		 * Handling an abnormal case.
		 * return failure as network buffer and device
		 * context are NULL
		 */
		LOG_ERR("%s: Network buffer or device context is NULL",
				      __func__);
		goto fail;
	}

	fmac_dev_ctx = (struct nrf_wifi_fmac_dev_ctx *)dev_ctx;
	if (!fmac_dev_ctx) {
		LOG_ERR("%s: fmac_dev_ctx is NULL", __func__);
		goto fail;
	}
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	if (!sys_dev_ctx) {
		LOG_ERR("%s: sys_dev_ctx is NULL", __func__);
		goto fail;
	}

	sys_dev_ctx->raw_pkt_stats.raw_pkts_from_stack += 1;

	/**
	 * only allow raw packet to be transmitted if interface type allows it
	 * do not queue the packet if interface type does not allow raw tx
	 */
	if (!nrf_wifi_raw_pkt_mode_enabled(sys_dev_ctx->vif_ctx[if_idx])) {
		LOG_ERR("%s: raw_packet mode is not enabled",
				      __func__);
		goto fail;
	}

	raw_tx_hdr = nrf_wifi_nbuf_set_raw_tx_hdr(nwb, sizeof(struct raw_tx_pkt_header));
	if (!raw_tx_hdr) {
		LOG_ERR("%s: Failed to get raw tx header",
				      __func__);
		goto fail;
	}

	peer_id = MAX_PEERS;
	ac = raw_tx_hdr->queue;
	if (ac >= NRF_WIFI_FMAC_AC_MAX) {
		LOG_ERR("%s: Invalid access category %d",
				      __func__,
				      ac);
		goto fail;
	}

	tx_status = nrf_wifi_fmac_tx(fmac_dev_ctx,
				     if_idx,
				     nwb,
				     ac,
				     peer_id);
	if (tx_status == NRF_WIFI_FMAC_TX_STATUS_FAIL) {
		LOG_DBG("%s: Failed to send packet",
				      __func__);
		goto fail;
	} else {
		/**
		 * Increment success count.
		 * can be added to shell command to obtain statistics
		 */
		sys_dev_ctx->raw_pkt_stats.raw_pkt_send_success += 1;
	}

	return NRF_WIFI_STATUS_SUCCESS;
fail:
	if (nwb) {
		nrf_wifi_nbuf_free(nwb);
	}

	if (sys_dev_ctx) {
		sys_dev_ctx->raw_pkt_stats.raw_pkt_send_failure += 1;
	}

	return NRF_WIFI_STATUS_FAIL;
}
#endif /* NRF71_RAW_DATA_TX */

enum nrf_wifi_status nrf_wifi_fmac_start_xmit(void *dev_ctx,
					      unsigned char if_idx,
					      void *nbuf)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	enum nrf_wifi_fmac_tx_status tx_status = NRF_WIFI_FMAC_TX_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;
	unsigned char *ra = NULL;
	int tid = 0;
	int ac = 0;
	int peer_id = -1;

	if (!nbuf) {
		goto out;
	}

	fmac_dev_ctx = dev_ctx;
	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (nrf_wifi_nbuf_data_size(nbuf) < NRF_WIFI_FMAC_ETH_HDR_LEN) {
		goto out;
	}

	ra = nrf_wifi_util_get_ra(sys_dev_ctx->vif_ctx[if_idx], nbuf);

	peer_id = nrf_wifi_fmac_peer_get_id(fmac_dev_ctx, ra);

	if (peer_id == -1) {
		LOG_ERR("%s: Got packet for unknown PEER",
				      __func__);

		goto out;
	} else if (peer_id == MAX_PEERS) {
		ac = NRF_WIFI_FMAC_AC_MC;
	} else {
		if (sys_dev_ctx->tx_config.peers[peer_id].qos_supported) {
			tid = nrf_wifi_get_tid(nbuf);
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
		LOG_DBG("%s: Failed to send packet",
				      __func__);
		goto out;
	}

	return NRF_WIFI_STATUS_SUCCESS;
out:
	if (nbuf) {
		nrf_wifi_nbuf_free(nbuf);
	}
	return status;
}
