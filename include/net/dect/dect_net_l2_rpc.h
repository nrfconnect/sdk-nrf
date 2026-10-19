/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief DECT NR+ L2 hooks for DECT NR+ RPC server (experimental).
 */

#ifndef DECT_NET_L2_RPC_H_
#define DECT_NET_L2_RPC_H_

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_DECT_NR_RPC_SERVER) || defined(__DOXYGEN__)

/**
 * @brief Handle an IPv6 packet received on the DECT iface for RPC forwarding.
 *
 * Called from @c dect_net_l2_recv() when the RPC client session is active.
 * Do not unref @p pkt; L2 unrefs it after return.
 *
 * @param iface DECT network interface.
 * @param pkt   Received packet.
 */
typedef void (*dect_net_l2_rpc_forward_cb_t)(struct net_if *iface, struct net_pkt *pkt);

/** @brief Register the RPC forward callback (NULL clears). */
void dect_net_l2_rpc_forward_register(dect_net_l2_rpc_forward_cb_t cb);

/**
 * @brief Notify that DECT iface link state may have changed.
 *
 * @param iface DECT network interface.
 */
typedef void (*dect_net_l2_link_state_cb_t)(struct net_if *iface);

/** @brief Register the link-state callback (NULL clears). */
void dect_net_l2_link_state_register(dect_net_l2_link_state_cb_t cb);

/**
 * @brief Enable or disable RPC forwarding of received IPv6 in @c dect_net_l2_recv().
 *
 * @param connected true to forward to the registered forward callback; false for local L2 RX.
 */
void dect_net_l2_rpc_client_set_connected(bool connected);

#endif /* CONFIG_DECT_NR_RPC_SERVER || __DOXYGEN__ */

#ifdef __cplusplus
}
#endif

#endif /* DECT_NET_L2_RPC_H_ */
