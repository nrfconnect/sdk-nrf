/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "dect_rpc_client_net.h"
#include <dect_rpc_ids.h>
#include <mock_dect_rpc_transport.h>
#include <net/dect/dect_rpc.h>
#include <test_rpc_env.h>

#include <nrf_rpc.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/ztest.h>

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void *suite_setup(void)
{
	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
	return NULL;
}

ZTEST(dect_rpc_client_auto_sync, test_notify_rpc_init_done_schedules_sync)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));

	zassert_not_null(iface, "DECT RPC client iface missing");

	mock_dect_rpc_tr_expect_add((mock_dect_rpc_pkt_t){ .data = NULL, .len = 0 },
				   RPC_RSP(GET_ADDRS_RSP_BOOTED_UNASSOCIATED));
	dect_rpc_client_notify_rpc_init_done();
	k_msleep(100);
	mock_dect_rpc_tr_expect_done();

	zassert_equal(net_if_get_mtu(iface), 1280, "auto-sync should apply GET_ADDRS MTU");
}

ZTEST_SUITE(dect_rpc_client_auto_sync, NULL, suite_setup, NULL, NULL, NULL);
