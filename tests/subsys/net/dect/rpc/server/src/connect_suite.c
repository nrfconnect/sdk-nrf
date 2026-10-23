/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DECT_RPC_CMD_CONNECT / DISCONNECT decoders: conn_mgr on the server DECT iface.
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>
#include "dect_mdm_mock_test_seam.h"
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

ZTEST(dect_rpc_server_connect, test_connect_conn_mgr)
{
	int join_before = dect_mdm_mock_network_join_call_count;

	mock_dect_rpc_tr_expect_add(RPC_RSP(CBOR_UINT_SMALL(0)), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_CONNECT));
	mock_dect_rpc_tr_expect_done();

	zassert_equal(dect_mdm_mock_network_join_call_count, join_before + 1,
		      "conn_mgr connect must reach HAL network_join_req on PT");
}

ZTEST(dect_rpc_server_connect, test_disconnect_conn_mgr)
{
	int unjoin_before = dect_mdm_mock_network_unjoin_call_count;

	mock_dect_rpc_tr_expect_add(RPC_RSP(CBOR_UINT_SMALL(0)), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_DISCONNECT));
	mock_dect_rpc_tr_expect_done();

	zassert_equal(dect_mdm_mock_network_unjoin_call_count, unjoin_before + 1,
		      "conn_mgr disconnect must reach HAL network_unjoin_req on PT");
}

ZTEST_SUITE(dect_rpc_server_connect, NULL, suite_setup, tc_setup, NULL, NULL);
