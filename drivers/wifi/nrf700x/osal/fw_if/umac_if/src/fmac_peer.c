/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing peer handling specific definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "hal_mem.h"
#include "fmac_peer.h"
#include "host_rpu_umac_if.h"
#include "fmac_util.h"

int wifi_nrf_fmac_peer_get_id(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			      const unsigned char *mac_addr)
{
	int i;
	struct peers_info *peer;

	if (wifi_nrf_util_is_multicast_addr(mac_addr)) {
		return MAX_PEERS;
	}

	for (i = 0; i < MAX_PEERS; i++) {
		peer = &fmac_dev_ctx->tx_config.peers[i];
		if (peer->peer_id == -1) {
			continue;
		}

		if ((wifi_nrf_util_ether_addr_equal(mac_addr,
						    (void *)peer->ra_addr))) {
			return peer->peer_id;
		}
	}
	return -1;
}

int wifi_nrf_fmac_peer_add(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			   unsigned char if_idx,
			   const unsigned char *mac_addr,
			   unsigned char is_legacy,
			   unsigned char qos_supported)
{
	int i;
	struct peers_info *peer;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;

	vif_ctx = fmac_dev_ctx->vif_ctx[if_idx];


	if (wifi_nrf_util_is_multicast_addr(mac_addr)
	    && (vif_ctx->if_type == NRF_WIFI_IFTYPE_AP)) {

		fmac_dev_ctx->tx_config.peers[MAX_PEERS].if_idx = if_idx;
		fmac_dev_ctx->tx_config.peers[MAX_PEERS].peer_id = MAX_PEERS;
		fmac_dev_ctx->tx_config.peers[MAX_PEERS].is_legacy = 1;


		return MAX_PEERS;
	}

	for (i = 0; i < MAX_PEERS; i++) {
		peer = &fmac_dev_ctx->tx_config.peers[i];

		if (peer->peer_id == -1) {
			wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
					      peer->ra_addr,
					      mac_addr,
					      NRF_WIFI_ETH_ADDR_LEN);
			peer->if_idx = if_idx;
			peer->peer_id = i;
			peer->is_legacy = is_legacy;
			peer->qos_supported = qos_supported;
			if (vif_ctx->if_type == NRF_WIFI_IFTYPE_AP) {
				hal_rpu_mem_write(fmac_dev_ctx->hal_dev_ctx,
						  (RPU_MEM_UMAC_PEND_Q_BMP +
						   sizeof(struct sap_pend_frames_bitmap) * i),
						  peer->ra_addr,
						  WIFI_NRF_FMAC_ETH_ADDR_LEN);
			}
			return i;
		}
	}

	wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
			      "%s: Failed !! No Space Available",
			      __func__);

	return -1;
}


void wifi_nrf_fmac_peer_remove(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			       unsigned char if_idx,
			       int peer_id)
{
	struct peers_info *peer;
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;

	vif_ctx = fmac_dev_ctx->vif_ctx[if_idx];
	peer = &fmac_dev_ctx->tx_config.peers[peer_id];

	wifi_nrf_osal_mem_set(fmac_dev_ctx->fpriv->opriv,
			      peer,
			      0x0,
			      sizeof(struct peers_info));
	peer->peer_id = -1;

	if (vif_ctx->if_type == NRF_WIFI_IFTYPE_AP) {
		hal_rpu_mem_write(fmac_dev_ctx->hal_dev_ctx,
				  (RPU_MEM_UMAC_PEND_Q_BMP +
				   (sizeof(struct sap_pend_frames_bitmap) * peer_id)),
				  peer->ra_addr,
				  WIFI_NRF_FMAC_ETH_ADDR_LEN);
	}
}


void wifi_nrf_fmac_peers_flush(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
			       unsigned char if_idx)
{
	struct wifi_nrf_fmac_vif_ctx *vif_ctx = NULL;
	unsigned int i = 0;
	struct peers_info *peer = NULL;

	vif_ctx = fmac_dev_ctx->vif_ctx[if_idx];
	fmac_dev_ctx->tx_config.peers[MAX_PEERS].peer_id = -1;

	for (i = 0; i < MAX_PEERS; i++) {
		peer = &fmac_dev_ctx->tx_config.peers[i];
		if (peer->if_idx == if_idx) {

			wifi_nrf_osal_mem_set(fmac_dev_ctx->fpriv->opriv,
					      peer,
					      0x0,
					      sizeof(struct peers_info));
			peer->peer_id = -1;

			if (vif_ctx->if_type == NRF_WIFI_IFTYPE_AP) {
				hal_rpu_mem_write(fmac_dev_ctx->hal_dev_ctx,
						  (RPU_MEM_UMAC_PEND_Q_BMP +
						   sizeof(struct sap_pend_frames_bitmap) * i),
						  peer->ra_addr,
						  WIFI_NRF_FMAC_ETH_ADDR_LEN);
			}
		}
	}
}
