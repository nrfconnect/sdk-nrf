/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>
#include "rpc_test_link_state_expect.h"
#include "rpc_test_rsp_builder.h"
#include "server_dect_fixture.h"

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void *suite_setup(void)
{
	zassert_not_null(server_dect_fixture_ensure_booted(), "DECT stack did not boot");
	return NULL;
}

static void tc_setup(void *f)
{
	ARG_UNUSED(f);

	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
}

ZTEST(dect_rpc_server_get_addrs, test_if_get_addrs_booted)
{
	struct net_if *iface = server_dect_fixture_ensure_booted();
	mock_dect_rpc_pkt_t rsp = rpc_test_expect_get_addrs_rsp(iface);

	mock_dect_rpc_tr_expect_add(rsp, NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_IF_GET_ADDRS));
	mock_dect_rpc_tr_expect_done();
}

ZTEST(dect_rpc_server_get_addrs, test_if_get_addrs_associated)
{
	struct net_if *iface = server_dect_fixture_ensure_booted();
	mock_dect_rpc_pkt_t rsp;

	EXPECT_LINK_STATE_TRANSITION(CBOR_TRUE, CBOR_FALSE);
	zassert_ok(server_dect_fixture_associate());
	k_msleep(100);
	mock_dect_rpc_tr_expect_reset();
	k_msleep(100);

	rsp = rpc_test_expect_get_addrs_rsp(iface);
	mock_dect_rpc_tr_expect_add(rsp, NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_IF_GET_ADDRS));
	mock_dect_rpc_tr_expect_done();

	EXPECT_LINK_STATE_TRANSITION(CBOR_TRUE, CBOR_TRUE);
	zassert_ok(server_dect_fixture_release());
	k_msleep(100);
	mock_dect_rpc_tr_expect_reset();
}

ZTEST_SUITE(dect_rpc_server_get_addrs, NULL, suite_setup, tc_setup, NULL, NULL);
