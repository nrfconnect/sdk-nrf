/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_MDM_RX_POOL_H_
#define DECT_MDM_RX_POOL_H_

#include <stddef.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate an RX net_pkt with attached buffer(s) for DECT.
 *
 * When CONFIG_DECT_MDM_RX_PRIVATE_POOL=y the net_pkt and its fragments come
 * from a DECT-only slab/buffer pool that other interfaces cannot consume,
 * preventing eth0 RX bursts from starving dect0 RX.
 *
 * When CONFIG_DECT_MDM_RX_PRIVATE_POOL=n this falls through to
 * net_pkt_rx_alloc_with_buffer() and uses the shared global RX pools, so
 * behaviour matches the previous implementation.
 *
 * Safe to call from ISR context (uses K_NO_WAIT internally).
 *
 * @param iface DECT network interface.
 * @param len   Required payload size in bytes.
 *
 * @return Cursor-initialised net_pkt with family AF_UNSPEC, or NULL on
 *         allocation failure.
 */
struct net_pkt *dect_mdm_rx_pkt_alloc(struct net_if *iface, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* DECT_MDM_RX_POOL_H_ */
