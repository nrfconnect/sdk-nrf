/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ncs_commit.h>
#include <mock_nrf_rpc_transport.h>
#include <test_rpc_env.h>
#include <rpc_utils_group.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();
}

ZTEST(rpc_utils_server, test_get_server_version)
{
	const char *commit = NCS_COMMIT_STRING;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x6C, STRING_TO_TUPLE(commit)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(RPC_UTIL_DEV_INFO_GET_VERSION));
	mock_nrf_rpc_tr_expect_done();
}

static int cmd_hello(struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "Hello world");
	return 0;
}

SHELL_CMD_REGISTER(hello, NULL, "Help text", cmd_hello);

ZTEST(rpc_utils_server, test_invoke_shell_cmd)
{
	/*
	 * shell_print() macro only works when the shell is started, and the shell is started
	 * on another thread. To make unit tests more robust, start the shell explicitly.
	 */
	zassert_ok(shell_start(shell_backend_dummy_get_ptr()));
	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x6f, '\r', '\n', 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o',
					   'r', 'l', 'd', '\r', '\n'),
				   NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(RPC_UTIL_DEV_INFO_INVOKE_SHELL_CMD, 0x65, 'h', 'e', 'l', 'l', 'o'));
	mock_nrf_rpc_tr_expect_done();
}

ZTEST_SUITE(rpc_utils_server, NULL, NULL, tc_setup, NULL, NULL);
