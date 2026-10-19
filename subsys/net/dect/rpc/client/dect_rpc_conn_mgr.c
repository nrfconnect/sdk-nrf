/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Connection Manager binding for the DECT NR+ RPC client net_if. connect/disconnect
 * are proxied to the RPC server (DECT modem), which runs conn_mgr on the real DECT iface.
 * L4 readiness on the client is derived by conn_mgr_monitor from mirrored IPv6 + if up.
 */

#include "dect_rpc_common.h"
#include "dect_rpc_client_net.h"

#include <nrf_rpc.h>

#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_connectivity_impl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dect_rpc_conn_mgr, CONFIG_NET_DECT_RPC_LOG_LEVEL);

static bool dect_rpc_conn_has_connection_config(struct conn_mgr_conn_binding *const binding)
{
	ARG_UNUSED(binding);

	return NRF_RPC_GROUP_STATUS(dect_rpc_group);
}

static int dect_rpc_conn_connect(struct conn_mgr_conn_binding *const binding)
{
	ARG_UNUSED(binding);

	return dect_rpc_client_connect();
}

static int dect_rpc_conn_disconnect(struct conn_mgr_conn_binding *const binding)
{
	ARG_UNUSED(binding);

	return dect_rpc_client_disconnect();
}

static void dect_rpc_conn_init(struct conn_mgr_conn_binding *const binding)
{
	/* DECT behavior is on the server (NET_L2_DECT_CONN_MGR_*). Keep the tunnel
	 * iface explicit: no conn_mgr auto-connect on admin-up, no auto-down.
	 */
	conn_mgr_binding_set_flag(binding, CONN_MGR_IF_NO_AUTO_CONNECT, true);
	conn_mgr_binding_set_flag(binding, CONN_MGR_IF_NO_AUTO_DOWN, true);
}

static struct conn_mgr_conn_api dect_rpc_conn_api = {
	.has_connection_config = dect_rpc_conn_has_connection_config,
	.connect = dect_rpc_conn_connect,
	.disconnect = dect_rpc_conn_disconnect,
	.init = dect_rpc_conn_init,
};

CONN_MGR_CONN_DEFINE(CONNECTIVITY_DECT_RPC_MGMT, &dect_rpc_conn_api);
