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

#if !defined(CONFIG_DECT_NR_RPC_SHELL)

ZTEST(dect_rpc_server_shell_rpc, test_shell_not_configured_on_server)
{
	mock_dect_rpc_tr_expect_add(RPC_RSP(SHELL_RSP_NOT_CONFIGURED), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_SHELL, CBOR_UINT_SMALL(1),
					CBOR_BSTR(7, 's', 't', 'a', 't', 'u', 's', 0)));
	mock_dect_rpc_tr_expect_done();
}

#endif /* !CONFIG_DECT_NR_RPC_SHELL */

ZTEST(dect_rpc_server_shell_rpc, test_shell_decode_error)
{
	mock_dect_rpc_tr_expect_add(
		RPC_RSP(CBOR_UINT_SMALL(1),
			CBOR_BSTR(12, 'd', 'e', 'c', 'o', 'd', 'e', ' ', 'e', 'r', 'r', 'o', 'r')),
		NO_RSP);
	/* argc=0 is invalid. */
	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_SHELL, CBOR_UINT_SMALL(0)));
	mock_dect_rpc_tr_expect_done();
}

ZTEST_SUITE(dect_rpc_server_shell_rpc, NULL, suite_setup, tc_setup, NULL, NULL);
