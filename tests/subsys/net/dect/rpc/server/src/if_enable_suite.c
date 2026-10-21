/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>
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

ZTEST(dect_rpc_server_if_enable, test_if_enable_true)
{
	mock_dect_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_IF_ENABLE, CBOR_TRUE));
	mock_dect_rpc_tr_expect_done();
}

ZTEST(dect_rpc_server_if_enable, test_if_enable_false)
{
	mock_dect_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_IF_ENABLE, CBOR_FALSE));
	mock_dect_rpc_tr_expect_done();
}

ZTEST_SUITE(dect_rpc_server_if_enable, NULL, suite_setup, tc_setup, NULL, NULL);
