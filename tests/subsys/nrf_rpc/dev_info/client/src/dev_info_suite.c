/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "zephyr/ztest_assert.h"
#include <ncs_commit.h>
#include <mock_nrf_rpc_transport.h>
#include <test_rpc_env.h>

#include <dev_info_rpc_ids.h>
#include <nrf_rpc/nrf_rpc_dev_info.h>

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

ZTEST(dev_info_rpc_client, test_get_version)
{
	const char *commit = NCS_COMMIT_STRING;
	const char *version = 0;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(DEV_INFO_RPC_GET_VERSION),
				   RPC_RSP(0x6C, STRING_TO_TUPLE(NCS_COMMIT_STRING)));
	version = nrf_rpc_get_ncs_commit_sha();
	mock_nrf_rpc_tr_expect_done();

	zexpect_str_equal(commit, version);
}

ZTEST(dev_info_rpc_client, test_invoke_shell_cmd)
{

	const size_t argc = 2;
	static const char *const argv[] = {"say", "hello", NULL};
	const char *hello = "HelloWorld\0";
	char *reply = NULL;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(DEV_INFO_RPC_INVOKE_SHELL_CMD, 0x6a, 's', 'a', 'y', ' ', 'h', 'e', 'l', 'l',
			'o', '\0'),
		RPC_RSP(0x6b, 'H', 'e', 'l', 'l', 'o', 'W', 'o', 'r', 'l', 'd', '\0'));
	reply = nrf_rpc_invoke_remote_shell_cmd(argc, (char **)argv);
	mock_nrf_rpc_tr_expect_done();

	zexpect_str_equal(hello, reply);

	if (reply) {
		k_free(reply);
	}
}

ZTEST_SUITE(dev_info_rpc_client, NULL, NULL, tc_setup, NULL, NULL);
