/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing utility function definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "osal_api.h"
#include "fmac_api_common.h"
#include "fmac_util.h"
#include "host_rpu_umac_if.h"

bool nrf_wifi_util_is_multicast_addr(const unsigned char *addr)
{
	return (0x01 & *addr);
}


bool nrf_wifi_util_is_unicast_addr(const unsigned char *addr)
{
	return !nrf_wifi_util_is_multicast_addr(addr);
}


bool nrf_wifi_util_ether_addr_equal(const unsigned char *addr_1,
				    const unsigned char *addr_2)
{
	const unsigned short *a = (const unsigned short *)addr_1;
	const unsigned short *b = (const unsigned short *)addr_2;

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) == 0;
}


#ifdef CONFIG_NRF700X_STA_MODE
unsigned short nrf_wifi_util_rx_get_eth_type(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					     void *nwb)
{
	unsigned char *payload = NULL;

	payload = (unsigned char *)nwb;

	return payload[6] << 8 | payload[7];
}


unsigned short nrf_wifi_util_tx_get_eth_type(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					     void *nwb)
{
	unsigned char *payload = NULL;

	payload = (unsigned char *)nwb;

	return payload[12] << 8 | payload[13];
}


int nrf_wifi_util_get_skip_header_bytes(unsigned short eth_type)
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


void nrf_wifi_util_convert_to_eth(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				  void *nwb,
				  struct nrf_wifi_fmac_ieee80211_hdr *hdr,
				  unsigned short eth_type)
{

	struct nrf_wifi_fmac_eth_hdr *ehdr = NULL;
	unsigned int len = 0;

	len = nrf_wifi_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
					   nwb);

	ehdr = (struct nrf_wifi_fmac_eth_hdr *)
		nrf_wifi_osal_nbuf_data_push(fmac_dev_ctx->fpriv->opriv,
					     nwb,
					     sizeof(struct nrf_wifi_fmac_eth_hdr));

	switch (hdr->fc & (NRF_WIFI_FCTL_TODS | NRF_WIFI_FCTL_FROMDS)) {
	case (NRF_WIFI_FCTL_TODS | NRF_WIFI_FCTL_FROMDS):
		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->src,
				      hdr->addr_4,
				      NRF_WIFI_FMAC_ETH_ADDR_LEN);

		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->dst,
				      hdr->addr_1,
				      NRF_WIFI_FMAC_ETH_ADDR_LEN);
		break;
	case (NRF_WIFI_FCTL_FROMDS):
		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->src,
				      hdr->addr_3,
				      NRF_WIFI_FMAC_ETH_ADDR_LEN);
		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->dst,
				      hdr->addr_1,
				      NRF_WIFI_FMAC_ETH_ADDR_LEN);
		break;
	case (NRF_WIFI_FCTL_TODS):
		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->src,
				      hdr->addr_2,
				      NRF_WIFI_FMAC_ETH_ADDR_LEN);
		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->dst,
				      hdr->addr_3,
				      NRF_WIFI_FMAC_ETH_ADDR_LEN);
		break;
	default:
		/* Both FROM and TO DS bit is zero*/
		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->src,
				      hdr->addr_2,
				      NRF_WIFI_FMAC_ETH_ADDR_LEN);
		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->dst,
				      hdr->addr_1,
				      NRF_WIFI_FMAC_ETH_ADDR_LEN);

	}

	if (eth_type >= NRF_WIFI_FMAC_ETH_P_802_3_MIN) {
		ehdr->proto = ((eth_type >> 8) | (eth_type << 8));
	} else {
		ehdr->proto = len;
	}
}


void nrf_wifi_util_rx_convert_amsdu_to_eth(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   void *nwb)
{
	struct nrf_wifi_fmac_eth_hdr *ehdr = NULL;
	struct nrf_wifi_fmac_amsdu_hdr amsdu_hdr;
	unsigned int len = 0;
	unsigned short eth_type = 0;
	void *nwb_data = NULL;
	unsigned char amsdu_hdr_len = 0;

	amsdu_hdr_len = sizeof(struct nrf_wifi_fmac_amsdu_hdr);

	nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &amsdu_hdr,
			      nrf_wifi_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
							  nwb),
			      amsdu_hdr_len);

	nwb_data = (unsigned char *)nrf_wifi_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
								nwb) + amsdu_hdr_len;

	eth_type = nrf_wifi_util_rx_get_eth_type(fmac_dev_ctx,
						 nwb_data);

	nrf_wifi_osal_nbuf_data_pull(fmac_dev_ctx->fpriv->opriv,
				     nwb,
				     (amsdu_hdr_len +
				      nrf_wifi_util_get_skip_header_bytes(eth_type)));

	len = nrf_wifi_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
					   nwb);

	ehdr = (struct nrf_wifi_fmac_eth_hdr *)
		nrf_wifi_osal_nbuf_data_push(fmac_dev_ctx->fpriv->opriv,
					     nwb,
					     sizeof(struct nrf_wifi_fmac_eth_hdr));

	nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      ehdr->src,
			      amsdu_hdr.src,
			      NRF_WIFI_FMAC_ETH_ADDR_LEN);

	nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      ehdr->dst,
			      amsdu_hdr.dst,
			      NRF_WIFI_FMAC_ETH_ADDR_LEN);

	if (eth_type >= NRF_WIFI_FMAC_ETH_P_802_3_MIN) {
		ehdr->proto = ((eth_type >> 8) | (eth_type << 8));
	} else {
		ehdr->proto = len;
	}
}


int nrf_wifi_util_get_tid(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			  void *nwb)
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

	nwb_data = nrf_wifi_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
					       nwb);

	ether_type = nrf_wifi_util_tx_get_eth_type(fmac_dev_ctx,
						   nwb_data);

	nwb_data = (unsigned char *)nrf_wifi_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
								nwb) + NRF_WIFI_FMAC_ETH_HDR_LEN;

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


int nrf_wifi_util_get_vif_indx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       const unsigned char *mac_addr)
{
	int i = 0;
	int vif_index = -1;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	for (i = 0; i < MAX_PEERS; i++) {
		if (!nrf_wifi_util_ether_addr_equal(def_dev_ctx->tx_config.peers[i].ra_addr,
						    mac_addr)) {
			vif_index = def_dev_ctx->tx_config.peers[i].if_idx;
			break;
		}
	}

	if (vif_index == -1) {
		nrf_wifi_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid vif_index = %d",
				      __func__,
				      vif_index);
	}

	return vif_index;
}


unsigned char *nrf_wifi_util_get_dest(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				      void *nwb)
{
	return nrf_wifi_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
					   nwb);
}


unsigned char *nrf_wifi_util_get_ra(struct nrf_wifi_fmac_vif_ctx *vif,
				    void *nwb)
{
	if (vif->if_type == NRF_WIFI_IFTYPE_STATION) {
		return vif->bssid;
	}

	return nrf_wifi_osal_nbuf_data_get(vif->fmac_dev_ctx->fpriv->opriv,
					   nwb);
}


unsigned char *nrf_wifi_util_get_src(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				     void *nwb)
{
	return (unsigned char *)nrf_wifi_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
							    nwb) + NRF_WIFI_FMAC_ETH_ADDR_LEN;
}

#endif /* CONFIG_NRF700X_STA_MODE */


bool nrf_wifi_util_is_arr_zero(unsigned char *arr,
			       unsigned int arr_sz)
{
	unsigned int i = 0;

	for (i = 0; i < arr_sz; i++) {
		if (arr[i] != 0) {
			return false;
		}
	}

	return true;
}

void *wifi_fmac_priv(struct nrf_wifi_fmac_priv *def)
{
	return &def->priv;
}

void *wifi_dev_priv(struct nrf_wifi_fmac_dev_ctx *def)
{
	return &def->priv;
}
