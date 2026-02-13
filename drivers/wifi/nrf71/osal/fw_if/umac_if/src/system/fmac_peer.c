/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing peer handling specific definitions for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#include "system/fmac_peer.h"

#include <nrf71_wifi_ctrl.h>
#include "common/fmac_util.h"

int nrf_wifi_fmac_peer_get_id(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			      const unsigned char *mac_addr)
{
	int i;
	struct peers_info *peer;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (nrf_wifi_util_is_multicast_addr(mac_addr)) {
		return MAX_PEERS;
	}

	for (i = 0; i < MAX_PEERS; i++) {
		peer = &sys_dev_ctx->tx_config.peers[i];
		if (peer->peer_id == -1) {
			continue;
		}

		if ((nrf_wifi_util_ether_addr_equal(mac_addr,
						    (void *)peer->ra_addr))) {
			return peer->peer_id;
		}
	}
	return -1;
}

int nrf_wifi_fmac_peer_add(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			   unsigned char if_idx,
			   const unsigned char *mac_addr,
			   unsigned char is_legacy,
			   unsigned char qos_supported)
{
	int i;
	struct peers_info *peer;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	vif_ctx = sys_dev_ctx->vif_ctx[if_idx];

	if (nrf_wifi_util_is_multicast_addr(mac_addr)
	    && (vif_ctx->if_type == NRF_WIFI_IFTYPE_AP)) {

		sys_dev_ctx->tx_config.peers[MAX_PEERS].if_idx = if_idx;
		sys_dev_ctx->tx_config.peers[MAX_PEERS].peer_id = MAX_PEERS;
		sys_dev_ctx->tx_config.peers[MAX_PEERS].is_legacy = 1;


		return MAX_PEERS;
	}

	for (i = 0; i < MAX_PEERS; i++) {
		peer = &sys_dev_ctx->tx_config.peers[i];

		if (peer->peer_id == -1) {
			nrf_wifi_osal_mem_cpy(peer->ra_addr,
					      mac_addr,
					      NRF_WIFI_ETH_ADDR_LEN);
			peer->if_idx = if_idx;
			peer->peer_id = i;
			peer->is_legacy = is_legacy;
			peer->qos_supported = qos_supported;
			return i;
		}
	}
	nrf_wifi_osal_log_err("%s: Failed !! No Space Available",
			      __func__);

	return -1;
}


void nrf_wifi_fmac_peer_remove(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       unsigned char if_idx,
			       int peer_id)
{
	struct peers_info *peer;
	struct nrf_wifi_fmac_vif_ctx *vif_ctx = NULL;
	struct nrf_wifi_sys_fmac_dev_ctx *sys_dev_ctx = NULL;

	if (peer_id == -1 || peer_id >= MAX_PEERS) {
		return;
	}

	sys_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	vif_ctx = sys_dev_ctx->vif_ctx[if_idx];
	peer = &sys_dev_ctx->tx_config.peers[peer_id];

	if (!peer || peer->peer_id == -1 || peer->peer_id >= MAX_PEERS ||
	    peer->if_idx != if_idx) {
		return;
	}

	nrf_wifi_osal_mem_set(peer,
			      0x0,
			      sizeof(struct peers_info));
	peer->peer_id = -1;
}


void nrf_wifi_fmac_peers_flush(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			       unsigned char if_idx)
{
}
