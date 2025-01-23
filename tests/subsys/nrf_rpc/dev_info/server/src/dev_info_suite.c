/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ncs_commit.h>
#include <mock_nrf_rpc_transport.h>
#include <test_rpc_env.h>

#include <dev_info_rpc_ids.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

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

ZTEST(dev_info_rpc_server, test_get_server_version)
{
	const char *commit = NCS_COMMIT_STRING;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x6C, STRING_TO_TUPLE(commit)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(DEV_INFO_RPC_GET_VERSION));
	mock_nrf_rpc_tr_expect_done();
}

ZTEST(dev_info_rpc_server, test_invoke_shell_cmd)
{
	mock_nrf_rpc_tr_expect_add(
		RPC_RSP(0x78, 0x55, 'R', 'e', 'm', 'o', 't', 'e', ' ', 's', 'h', 'e', 'l', 'l', ' ',
			'i', 's', ' ', 'd', 'i', 's', 'a', 'b', 'l', 'e', 'd', ' ', 'o', 'n', ' ',
			't', 'h', 'e', ' ', 's', 'e', 'r', 'v', 'e', 'r', '.', ' ', 'E', 'n', 'a',
			'b', 'l', 'e', ' ', 'C', 'O', 'N', 'F', 'I', 'G', '_', 'N', 'R', 'F', '_',
			'R', 'P', 'C', '_', 'R', 'E', 'M', 'O', 'T', 'E', '_', 'S', 'H', 'E', 'L',
			'L', ' ', 't', 'o', ' ', 'u', 's', 'e', ' ', 'i', 't', '.'),
		NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(DEV_INFO_RPC_INVOKE_SHELL_CMD, 0x6a, 's', 'a', 'y', ' ',
					'h', 'e', 'l', 'l', 'o', '\0'));
	mock_nrf_rpc_tr_expect_done();
}

ZTEST_SUITE(dev_info_rpc_server, NULL, NULL, tc_setup, NULL, NULL);
