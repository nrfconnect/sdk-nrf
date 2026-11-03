/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>

#include <string.h>

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
	ARG_UNUSED(f);

	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
}

ZTEST(dect_rpc_client_shell_run, test_dect_run_shell_rpc)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	shell_backend_dummy_clear_output(sh);

	mock_dect_rpc_tr_expect_add(
		RPC_CMD(DECT_RPC_CMD_SHELL, CBOR_UINT_SMALL(1),
			CBOR_BSTR(7, 's', 't', 'a', 't', 'u', 's', 0)),
		RPC_RSP(CBOR_UINT_SMALL(0)));
	err = shell_execute_cmd(sh, "dect run status");
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "dect run status failed: %d", err);
}

ZTEST(dect_rpc_client_shell_run, test_dect_run_too_few_args)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	zassert_equal(shell_execute_cmd(sh, "dect run"), -EINVAL);
}

ZTEST_SUITE(dect_rpc_client_shell_run, NULL, NULL, tc_setup, NULL, NULL);
