/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DECT NR+ RPC client internal API (subsystem sources and in-tree tests only).
 * Applications and samples must use @file net/dect/dect_rpc.h for supported hooks.
 */

#ifndef DECT_RPC_CLIENT_NET_H_
#define DECT_RPC_CLIENT_NET_H_

#include <net/dect/dect_rpc.h>
#include <zephyr/net/net_l2.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_DECT_NR_RPC_CLIENT)

NET_L2_DECLARE_PUBLIC(DECT_RPC_L2);

#if defined(CONFIG_DECT_NR_RPC_NET_IF)

#include <zephyr/net/net_if.h>

int dect_rpc_client_sync_addrs(struct net_if *iface);

int dect_rpc_client_ping(k_timeout_t timeout);

#if defined(CONFIG_DECT_NR_RPC_CONN_MGR)
int dect_rpc_client_connect(void);
int dect_rpc_client_disconnect(void);
#endif /* CONFIG_DECT_NR_RPC_CONN_MGR */

#endif /* CONFIG_DECT_NR_RPC_NET_IF */

#endif /* CONFIG_DECT_NR_RPC_CLIENT */

#ifdef __cplusplus
}
#endif

#endif /* DECT_RPC_CLIENT_NET_H_ */
