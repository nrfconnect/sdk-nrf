/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(CONFIG_DECT_NR_RPC_SHELL)

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

ZTEST(dect_rpc_server_shell_l2, test_shell_unknown_subcmd)
{
	mock_dect_rpc_tr_expect_add(
		RPC_EVT(DECT_RPC_CMD_SHELL_LINE,
			CBOR_BSTR8(27, 'U', 'n', 'k', 'n', 'o', 'w', 'n', ' ', 's', 'u', 'b', 'c',
				   'o', 'm', 'm', 'a', 'n', 'd', ':', ' ', 'n', 'o', 's', 'u', 'c',
				   'h', '\n')),
		NO_RSP);
	mock_dect_rpc_tr_expect_add(RPC_RSP(CBOR_UINT_SMALL(1)), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_SHELL, CBOR_UINT_SMALL(1),
					CBOR_BSTR(6, 'n', 'o', 's', 'u', 'c', 'h')));
	k_msleep(100);
	mock_dect_rpc_tr_expect_done();
}

#define SHELL_STATUS_LINE_COUNT 8

ZTEST(dect_rpc_server_shell_l2, test_shell_status_success)
{
	int i;

	for (i = 0; i < SHELL_STATUS_LINE_COUNT; i++) {
		MOCK_DECT_RPC_EXPECT_ANY_SEND;
	}
	mock_dect_rpc_tr_expect_add(RPC_RSP(CBOR_UINT_SMALL(0)), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_SHELL, CBOR_UINT_SMALL(1),
					CBOR_BSTR(6, 's', 't', 'a', 't', 'u', 's')));
	k_msleep(200);
	mock_dect_rpc_tr_expect_done();
}

ZTEST_SUITE(dect_rpc_server_shell_l2, NULL, suite_setup, tc_setup, NULL, NULL);

#endif /* CONFIG_DECT_NR_RPC_SHELL */
