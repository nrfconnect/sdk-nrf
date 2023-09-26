/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing utility declarations for the
 * FMAC IF Layer of the Wi-Fi driver.
 */
#ifndef __FMAC_UTIL_H__
#define __FMAC_UTIL_H__

#ifndef CONFIG_NRF700X_RADIO_TEST
#include <stdbool.h>
#include "fmac_structs.h"
#include "pack_def.h"


#define NRF_WIFI_FMAC_ETH_ADDR_LEN 6
#define NRF_WIFI_FMAC_ETH_HDR_LEN 14

#define NRF_WIFI_FMAC_FTYPE_DATA 0x0008
#define NRF_WIFI_FMAC_STYPE_DATA 0x0000
#define NRF_WIFI_FMAC_STYPE_QOS_DATA 0x0080

#define NRF_WIFI_FMAC_FCTL_FTYPE 0x000c
#define NRF_WIFI_FMAC_FCTL_PROTECTED 0x4000
#define NRF_WIFI_FMAC_FCTL_TODS 0x0100
#define NRF_WIFI_FMAC_FCTL_FROMDS 0x0200

#define NRF_WIFI_FMAC_CIPHER_SUITE_WEP40 0x000FAC01
#define NRF_WIFI_FMAC_CIPHER_SUITE_WEP104 0x000FAC05
#define NRF_WIFI_FMAC_CIPHER_SUITE_TKIP 0x000FAC02
#define NRF_WIFI_FMAC_CIPHER_SUITE_CCMP 0x000FAC04
#define NRF_WIFI_FMAC_CIPHER_SUITE_CCMP_256 0x000FAC0A
#define NRF_WIFI_FMAC_CIPHER_SUITE_OPEN 0x0
#define NRF_WIFI_FMAC_CIPHER_SUITE_SMS4 0x00147201

#define NRF_WIFI_FMAC_CCMP_HDR_LEN 8
#define NRF_WIFI_FMAC_CCMP_256_HDR_LEN 8
#define NRF_WIFI_FMAC_SMS4_HDR_LEN 18

#define NRF_WIFI_FMAC_WEP_IV_LEN 4
#define NRF_WIFI_FMAC_TKIP_IV_LEN 8

#define NRF_WIFI_FCTL_TODS 0x0100
#define NRF_WIFI_FCTL_FROMDS 0x0200
#define NRF_WIFI_FMAC_ETH_P_8021Q 0x8100 /* 802.1Q VLAN Extended Header */
#define NRF_WIFI_FMAC_ETH_P_8021AD 0x88A8 /* 802.1ad Service VLAN */
#define NRF_WIFI_FMAC_ETH_P_MPLS_UC 0x8847 /* MPLS Unicast traffic */
#define NRF_WIFI_FMAC_ETH_P_MPLS_MC 0x8848 /* MPLS Multicast traffic */
#define NRF_WIFI_FMAC_ETH_P_IP 0x0800 /* Internet Protocol packet */
#define NRF_WIFI_FMAC_ETH_P_IPV6 0x86DD /* IPv6 over bluebook */
#define NRF_WIFI_FMAC_ETH_P_80221 0x8917 /* IEEE 802.21 Media Independent Handover Protocol */
#define NRF_WIFI_FMAC_ETH_P_AARP 0x80F3 /* Appletalk AARP */
#define NRF_WIFI_FMAC_ETH_P_IPX 0x8137 /* IPX over DIX */
#define NRF_WIFI_FMAC_ETH_P_802_3_MIN 0x0600 /* If the value in the ethernet type is less than
					      * this value then the frame is Ethernet II.
					      * Else it is 802.3
					      */
#define NRF_WIFI_FMAC_VLAN_PRIO_SHIFT 0x0D /* 13 bit */
#define NRF_WIFI_FMAC_VLAN_PRIO_MASK 0xE000
#define NRF_WIFI_FMAC_MPLS_LS_TC_MASK 0x00000E00
#define NRF_WIFI_FMAC_MPLS_LS_TC_SHIFT 0x09
#define NRF_WIFI_FMAC_IPV6_TOS_MASK 0x0FF0
#define NRF_WIFI_FMAC_IPV6_TOS_SHIFT 0x04 /* 4bit */
#define NRF_WIFI_FMAC_ETH_TYPE_MASK 0xFFFF

struct nrf_wifi_fmac_ieee80211_hdr {
	unsigned short fc;
	unsigned short dur_id;
	unsigned char addr_1[NRF_WIFI_FMAC_ETH_ADDR_LEN];
	unsigned char addr_2[NRF_WIFI_FMAC_ETH_ADDR_LEN];
	unsigned char addr_3[NRF_WIFI_FMAC_ETH_ADDR_LEN];
	unsigned short seq_ctrl;
	unsigned char addr_4[NRF_WIFI_FMAC_ETH_ADDR_LEN];
} __NRF_WIFI_PKD;


struct nrf_wifi_fmac_eth_hdr {
	unsigned char dst[NRF_WIFI_FMAC_ETH_ADDR_LEN]; /* destination eth addr */
	unsigned char src[NRF_WIFI_FMAC_ETH_ADDR_LEN]; /* source ether addr */
	unsigned short proto; /* packet type ID field */
} __NRF_WIFI_PKD;


struct nrf_wifi_fmac_amsdu_hdr {
	unsigned char dst[NRF_WIFI_FMAC_ETH_ADDR_LEN]; /* destination eth addr */
	unsigned char src[NRF_WIFI_FMAC_ETH_ADDR_LEN]; /* source ether addr */
	unsigned short length; /* length*/
} __NRF_WIFI_PKD;

bool nrf_wifi_util_is_multicast_addr(const unsigned char *addr);

bool nrf_wifi_util_is_unicast_addr(const unsigned char *addr);

bool nrf_wifi_util_ether_addr_equal(const unsigned char *addr_1,
				    const unsigned char *addr_2);

int nrf_wifi_util_get_tid(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			  void *nwb);

int nrf_wifi_util_get_vif_indx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       const unsigned char *mac_addr);

unsigned char *nrf_wifi_util_get_ra(struct nrf_wifi_fmac_vif_ctx *vif,
				    void *nwb);

unsigned char *nrf_wifi_util_get_src(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				     void *nwb);

unsigned char *nrf_wifi_util_get_dest(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				      void *nwb);

unsigned short nrf_wifi_util_tx_get_eth_type(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					     void *nwb);

unsigned short nrf_wifi_util_rx_get_eth_type(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					     void *nwb);

int nrf_wifi_util_get_skip_header_bytes(unsigned short eth_type);

void nrf_wifi_util_convert_to_eth(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				  void *nwb,
				  struct nrf_wifi_fmac_ieee80211_hdr *hdr,
				  unsigned short eth_type);

void nrf_wifi_util_rx_convert_amsdu_to_eth(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   void *nwb);

bool nrf_wifi_util_is_arr_zero(unsigned char *arr,
			       unsigned int arr_sz);

#endif /* !CONFIG_NRF700X_RADIO_TEST */

void *wifi_fmac_priv(struct nrf_wifi_fmac_priv *def);
void *wifi_dev_priv(struct nrf_wifi_fmac_dev_ctx *def);

#endif /* __FMAC_UTIL_H__ */
