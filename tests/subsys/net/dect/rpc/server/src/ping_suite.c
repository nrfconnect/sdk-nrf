/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Tests the server's DECT_RPC_CMD_PING decoder (dect_rpc_cmd_ping() in
 * server/dect_rpc_if.c): given an incoming PING command, does the server respond
 * with exactly the expected "0" response? Mirrors client/src/ping_suite.c, which
 * tests the same command from the client's side.
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

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

ZTEST(dect_rpc_server_ping, test_ping_command)
{
	/* Request payload content (the sequence number) is ignored by the server -- see
	 * nrf_rpc_decode_skip(ctx) in dect_rpc_cmd_ping() -- so its exact value does not
	 * matter here.
	 */
	mock_dect_rpc_tr_expect_add(RPC_RSP(CBOR_UINT_SMALL(0)), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_PING, CBOR_UINT_SMALL(0)));
	mock_dect_rpc_tr_expect_done();
}

ZTEST_SUITE(dect_rpc_server_ping, NULL, NULL, tc_setup, NULL, NULL);
