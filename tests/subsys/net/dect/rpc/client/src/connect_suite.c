/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * "dect connect" / "dect disconnect" via conn_mgr on the RPC net_if, and direct
 * dect_rpc_client_connect() / disconnect() for RPC timeout paths.
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>

#include <string.h>

#include <nrf_rpc_errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/ztest.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

#include "dect_rpc_client_net.h"

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static struct net_if *rpc_client_iface(void)
{
	return net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
}

static void tc_setup(void *f)
{
	ARG_UNUSED(f);

	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
}

ZTEST(dect_rpc_client_connect, test_dect_connect_shell_success)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char *output;
	size_t size;
	int err;

	zassert_not_null(rpc_client_iface(), "RPC client net_if missing");

	shell_backend_dummy_clear_output(sh);

	MOCK_DECT_RPC_EXPECT_ANY_SEND;
	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_CONNECT), RPC_RSP(CBOR_UINT_SMALL(0)));
	err = shell_execute_cmd(sh, "dect connect");
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "Expected 'dect connect' to succeed, got %d", err);

	output = shell_backend_dummy_get_output(sh, &size);
	zassert_not_null(strstr(output, "Connect initiated on server (via RPC)"),
			 "Missing connect success message: %s", output);
}

ZTEST(dect_rpc_client_connect, test_dect_disconnect_shell_success)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char *output;
	size_t size;
	int err;

	zassert_not_null(rpc_client_iface(), "RPC client net_if missing");

	shell_backend_dummy_clear_output(sh);

	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_DISCONNECT), RPC_RSP(CBOR_UINT_SMALL(0)));
	err = shell_execute_cmd(sh, "dect disconnect");
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "Expected 'dect disconnect' to succeed, got %d", err);

	output = shell_backend_dummy_get_output(sh, &size);
	zassert_not_null(strstr(output, "Disconnect initiated on server (via RPC)"),
			 "Missing disconnect success message: %s", output);
}

ZTEST(dect_rpc_client_connect, test_disconnect_rpc_timeout)
{
	int err;

	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_DISCONNECT), NO_RSP);
	err = dect_rpc_client_disconnect();
	mock_dect_rpc_tr_expect_reset();

	zassert_equal(err, -NRF_ETIMEDOUT,
		      "Expected RPC response timeout, got %d", err);
}

ZTEST(dect_rpc_client_connect, test_connect_rpc_timeout)
{
	int err;

	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_CONNECT), NO_RSP);
	err = dect_rpc_client_connect();
	mock_dect_rpc_tr_expect_reset();

	zassert_equal(err, -NRF_ETIMEDOUT,
		      "Expected RPC response timeout, got %d", err);
}

ZTEST_SUITE(dect_rpc_client_connect, NULL, NULL, tc_setup, NULL, NULL);
