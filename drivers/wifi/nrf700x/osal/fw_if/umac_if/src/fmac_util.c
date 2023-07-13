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
#include "fmac_util.h"
#include "host_rpu_umac_if.h"

bool wifi_nrf_util_is_multicast_addr(const unsigned char *addr)
{
	return (0x01 & *addr);
}


bool wifi_nrf_util_is_unicast_addr(const unsigned char *addr)
{
	return !wifi_nrf_util_is_multicast_addr(addr);
}


bool wifi_nrf_util_ether_addr_equal(const unsigned char *addr_1,
				    const unsigned char *addr_2)
{
	const unsigned short *a = (const unsigned short *)addr_1;
	const unsigned short *b = (const unsigned short *)addr_2;

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) == 0;
}


#ifdef CONFIG_WPA_SUPP
unsigned short wifi_nrf_util_rx_get_eth_type(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					     void *nwb)
{
	unsigned char *payload = NULL;

	payload = (unsigned char *)nwb;

	return payload[6] << 8 | payload[7];
}


unsigned short wifi_nrf_util_tx_get_eth_type(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					     void *nwb)
{
	unsigned char *payload = NULL;

	payload = (unsigned char *)nwb;

	return payload[12] << 8 | payload[13];
}


int wifi_nrf_util_get_skip_header_bytes(unsigned short eth_type)
{
	/* Ethernet-II snap header (RFC1042 for most EtherTypes) */
	unsigned char llc_header[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
	/* Bridge-Tunnel header (for EtherTypes ETH_P_AARP and ETH_P_IPX) */
	static unsigned char aarp_ipx_header[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8};
	int skip_header_bytes = 0;

	skip_header_bytes = sizeof(eth_type);

	if (eth_type == WIFI_NRF_FMAC_ETH_P_AARP ||
	    eth_type == WIFI_NRF_FMAC_ETH_P_IPX) {
		skip_header_bytes += sizeof(aarp_ipx_header);
	} else if (eth_type >= WIFI_NRF_FMAC_ETH_P_802_3_MIN) {
		skip_header_bytes += sizeof(llc_header);
	}

	return skip_header_bytes;
}


void wifi_nrf_util_convert_to_eth(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				  void *nwb,
				  struct wifi_nrf_fmac_ieee80211_hdr *hdr,
				  unsigned short eth_type)
{

	struct wifi_nrf_fmac_eth_hdr *ehdr = NULL;
	unsigned int len = 0;

	len = wifi_nrf_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
					   nwb);

	ehdr = (struct wifi_nrf_fmac_eth_hdr *)
		wifi_nrf_osal_nbuf_data_push(fmac_dev_ctx->fpriv->opriv,
					     nwb,
					     sizeof(struct wifi_nrf_fmac_eth_hdr));

	switch (hdr->fc & (WIFI_NRF_FCTL_TODS | WIFI_NRF_FCTL_FROMDS)) {
	case (WIFI_NRF_FCTL_TODS | WIFI_NRF_FCTL_FROMDS):
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->src,
				      hdr->addr_4,
				      WIFI_NRF_FMAC_ETH_ADDR_LEN);

		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->dst,
				      hdr->addr_1,
				      WIFI_NRF_FMAC_ETH_ADDR_LEN);
		break;
	case (WIFI_NRF_FCTL_FROMDS):
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->src,
				      hdr->addr_3,
				      WIFI_NRF_FMAC_ETH_ADDR_LEN);
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->dst,
				      hdr->addr_1,
				      WIFI_NRF_FMAC_ETH_ADDR_LEN);
		break;
	case (WIFI_NRF_FCTL_TODS):
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->src,
				      hdr->addr_2,
				      WIFI_NRF_FMAC_ETH_ADDR_LEN);
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->dst,
				      hdr->addr_3,
				      WIFI_NRF_FMAC_ETH_ADDR_LEN);
		break;
	default:
		/* Both FROM and TO DS bit is zero*/
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->src,
				      hdr->addr_2,
				      WIFI_NRF_FMAC_ETH_ADDR_LEN);
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      ehdr->dst,
				      hdr->addr_1,
				      WIFI_NRF_FMAC_ETH_ADDR_LEN);

	}

	if (eth_type >= WIFI_NRF_FMAC_ETH_P_802_3_MIN) {
		ehdr->proto = ((eth_type >> 8) | (eth_type << 8));
	} else {
		ehdr->proto = len;
	}
}


void wifi_nrf_util_rx_convert_amsdu_to_eth(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					   void *nwb)
{
	struct wifi_nrf_fmac_eth_hdr *ehdr = NULL;
	struct wifi_nrf_fmac_amsdu_hdr amsdu_hdr;
	unsigned int len = 0;
	unsigned short eth_type = 0;
	void *nwb_data = NULL;
	unsigned char amsdu_hdr_len = 0;

	amsdu_hdr_len = sizeof(struct wifi_nrf_fmac_amsdu_hdr);

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      &amsdu_hdr,
			      wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
							  nwb),
			      amsdu_hdr_len);

	nwb_data = (unsigned char *)wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
								nwb) + amsdu_hdr_len;

	eth_type = wifi_nrf_util_rx_get_eth_type(fmac_dev_ctx,
						 nwb_data);

	wifi_nrf_osal_nbuf_data_pull(fmac_dev_ctx->fpriv->opriv,
				     nwb,
				     (amsdu_hdr_len +
				      wifi_nrf_util_get_skip_header_bytes(eth_type)));

	len = wifi_nrf_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
					   nwb);

	ehdr = (struct wifi_nrf_fmac_eth_hdr *)
		wifi_nrf_osal_nbuf_data_push(fmac_dev_ctx->fpriv->opriv,
					     nwb,
					     sizeof(struct wifi_nrf_fmac_eth_hdr));

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      ehdr->src,
			      amsdu_hdr.src,
			      WIFI_NRF_FMAC_ETH_ADDR_LEN);

	wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
			      ehdr->dst,
			      amsdu_hdr.dst,
			      WIFI_NRF_FMAC_ETH_ADDR_LEN);

	if (eth_type >= WIFI_NRF_FMAC_ETH_P_802_3_MIN) {
		ehdr->proto = ((eth_type >> 8) | (eth_type << 8));
	} else {
		ehdr->proto = len;
	}
}


int wifi_nrf_util_get_tid(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
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

	nwb_data = wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
					       nwb);

	ether_type = wifi_nrf_util_tx_get_eth_type(fmac_dev_ctx,
						   nwb_data);

	nwb_data = (unsigned char *)wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
								nwb) + WIFI_NRF_FMAC_ETH_HDR_LEN;

	switch (ether_type & WIFI_NRF_FMAC_ETH_TYPE_MASK) {
	/* If VLAN 802.1Q (0x8100) ||
	 * 802.1AD(0x88A8) FRAME calculate priority accordingly
	 */
	case WIFI_NRF_FMAC_ETH_P_8021Q:
	case WIFI_NRF_FMAC_ETH_P_8021AD:
		vlan_tci = (((unsigned char *)nwb_data)[4] << 8) |
			(((unsigned char *)nwb_data)[5]);
		vlan_priority = ((vlan_tci & WIFI_NRF_FMAC_VLAN_PRIO_MASK)
				 >> WIFI_NRF_FMAC_VLAN_PRIO_SHIFT);
		priority = vlan_priority;
		break;
	/* If MPLS MC(0x8840) / UC(0x8847) frame calculate priority
	 * accordingly
	 */
	case WIFI_NRF_FMAC_ETH_P_MPLS_UC:
	case WIFI_NRF_FMAC_ETH_P_MPLS_MC:
		mpls_hdr = (((unsigned char *)nwb_data)[0] << 24) |
			(((unsigned char *)nwb_data)[1] << 16) |
			(((unsigned char *)nwb_data)[2] << 8)  |
			(((unsigned char *)nwb_data)[3]);
		mpls_tc_qos = (mpls_hdr & (WIFI_NRF_FMAC_MPLS_LS_TC_MASK)
			       >> WIFI_NRF_FMAC_MPLS_LS_TC_SHIFT);
		priority = mpls_tc_qos;
		break;
	/* If IP (0x0800) frame calculate priority accordingly */
	case WIFI_NRF_FMAC_ETH_P_IP:
		/*get the tos filed*//*DA+SA+ETH+(VER+IHL)*/
		tos = (((unsigned char *)nwb_data)[1]);
		/*get the dscp value */
		dscp = (tos & 0xfc);
		priority = dscp >> 5;
		break;
	case WIFI_NRF_FMAC_ETH_P_IPV6:
		/* Get the TOS filled DA+SA+ETH */
		ipv6_hdr = (((unsigned char *)nwb_data)[0] << 8) |
			((unsigned char *)nwb_data)[1];
		dscp = (((ipv6_hdr & WIFI_NRF_FMAC_IPV6_TOS_MASK)
			 >> WIFI_NRF_FMAC_IPV6_TOS_SHIFT) & 0xfc);
		priority = dscp >> 5;
		break;
	/* If Media Independent (0x8917)
	 * frame calculate priority accordingly.
	 */
	case WIFI_NRF_FMAC_ETH_P_80221:
		/* 802.21 is always network control traffic */
		priority = 0x07;
		break;
	default:
		priority = 0;
	}

	return priority;
}


int wifi_nrf_util_get_vif_indx(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			       const unsigned char *mac_addr)
{
	int i = 0;
	int vif_index = -1;

	for (i = 0; i < MAX_PEERS; i++) {
		if (!wifi_nrf_util_ether_addr_equal(fmac_dev_ctx->tx_config.peers[i].ra_addr,
						    mac_addr)) {
			vif_index = fmac_dev_ctx->tx_config.peers[i].if_idx;
			break;
		}
	}

	if (vif_index == -1) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Invalid vif_index = %d",
				      __func__,
				      vif_index);
	}

	return vif_index;
}


unsigned char *wifi_nrf_util_get_dest(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				      void *nwb)
{
	return wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
					   nwb);
}


unsigned char *wifi_nrf_util_get_ra(struct wifi_nrf_fmac_vif_ctx *vif,
				    void *nwb)
{
	if (vif->if_type == NRF_WIFI_IFTYPE_STATION) {
		return vif->bssid;
	}

	return wifi_nrf_osal_nbuf_data_get(vif->fmac_dev_ctx->fpriv->opriv,
					   nwb);
}


unsigned char *wifi_nrf_util_get_src(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				     void *nwb)
{
	return (unsigned char *)wifi_nrf_osal_nbuf_data_get(fmac_dev_ctx->fpriv->opriv,
							    nwb) + WIFI_NRF_FMAC_ETH_ADDR_LEN;
}

#endif /* CONFIG_WPA_SUPP */


bool wifi_nrf_util_is_arr_zero(unsigned char *arr,
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
