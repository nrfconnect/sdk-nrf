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

int wifi_nrf_fmac_peer_get_id(struct wifi_nrf_fmac_dev_ctx *fmac_ctx,
			      const unsigned char *mac_addr);

int wifi_nrf_fmac_peer_add(struct wifi_nrf_fmac_dev_ctx *fmac_ctx,
			   unsigned char if_idx,
			   const unsigned char *mac_addr,
			   unsigned char is_legacy,
			   unsigned char qos_supported);

void wifi_nrf_fmac_peer_remove(struct wifi_nrf_fmac_dev_ctx *fmac_ctx,
			       unsigned char if_idx,
			       int peer_id);

void wifi_nrf_fmac_peers_flush(struct wifi_nrf_fmac_dev_ctx *fmac_ctx,
			       unsigned char if_idx);

#endif /* __FMAC_PEER_H__ */
