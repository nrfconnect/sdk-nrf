/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing peer handling specific declarations for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_PEER_H__
#define __FMAC_PEER_H__

#include "fmac_structs.h"

int nrf_wifi_fmac_peer_get_id(struct nrf_wifi_fmac_dev_ctx *fmac_ctx,
			      const unsigned char *mac_addr);

int nrf_wifi_fmac_peer_add(struct nrf_wifi_fmac_dev_ctx *fmac_ctx,
			   unsigned char if_idx,
			   const unsigned char *mac_addr,
			   unsigned char is_legacy,
			   unsigned char qos_supported);

void nrf_wifi_fmac_peer_remove(struct nrf_wifi_fmac_dev_ctx *fmac_ctx,
			       unsigned char if_idx,
			       int peer_id);

void nrf_wifi_fmac_peers_flush(struct nrf_wifi_fmac_dev_ctx *fmac_ctx,
			       unsigned char if_idx);

#endif /* __FMAC_PEER_H__ */
