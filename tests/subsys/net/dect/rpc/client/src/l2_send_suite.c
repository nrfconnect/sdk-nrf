/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Client DECT_RPC_L2 send path -> DECT_RPC_CMD_IF_SEND (dect_rpc_l2_send in dect_rpc_if.c).
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/ztest.h>

#include "dect_rpc_client_net.h"

#define TEST_IPV6_PKT                                                                              \
	0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x40, 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
		0, 0, 0, 0x01, 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x56, 0x78, 0x9a, 0xbc

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	ARG_UNUSED(f);

	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
}

ZTEST(dect_rpc_client_l2_send, test_l2_send_rejects_oversized_pkt)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
	struct net_pkt *pkt;
	static uint8_t huge[501];
	enum net_verdict ret;

	zassert_not_null(iface, "DECT RPC client iface missing");

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(huge), AF_INET6, IPPROTO_UDP, K_NO_WAIT);
	zassert_not_null(pkt, "net_pkt alloc failed");
	zassert_ok(net_pkt_write(pkt, huge, sizeof(huge)));

	ret = net_if_try_send_data(iface, pkt, K_NO_WAIT);

	zassert_equal(ret, NET_DROP, "oversized pkt must not be sent, got %d", ret);
}

ZTEST_SUITE(dect_rpc_client_l2_send, NULL, NULL, tc_setup, NULL, NULL);
