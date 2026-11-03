/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Tests the "dect status" shell command on the DECT NR+ RPC client: verifies it sends
 * a DECT_RPC_CMD_IF_STATUS request and correctly decodes/prints a canned response.
 * Exercises subsys/net/dect/rpc/client/dect_rpc_shell.c (cmd_dect_status() and
 * print_dect_status_shell()) through the real shell command dispatcher, using Zephyr's
 * dummy shell backend to capture output without a real UART.
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <status_codec_helpers.h>
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

ZTEST(dect_rpc_client_status, test_dect_status_shell_empty)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char *output;
	size_t size;
	int err;

	shell_backend_dummy_clear_output(sh);

	/* First (and only, in this test) IF_STATUS request in the binary: dect_rpc_req_seq
	 * (static, file-scope in client/dect_rpc_shell.c) starts at 0.
	 */
	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_IF_STATUS, CBOR_UINT_SMALL(0)),
				   RPC_RSP(STATUS_RSP_NO_DECT_IF));
	err = shell_execute_cmd(sh, "dect status");
	mock_dect_rpc_tr_expect_done();

	zassert_equal(err, 0, "Expected 'dect status' to succeed, got %d", err);

	output = shell_backend_dummy_get_output(sh, &size);
	/* Match without a trailing newline: the dummy shell backend is defined with
	 * SHELL_FLAG_OLF_CRLF, which rewrites every '\n' written by shell_fprintf() to
	 * "\r\n", so a plain '\n' in these expected strings would never match.
	 */
	zassert_not_null(strstr(output, "DECT NR+ status (from DECT NR+ RPC server):"),
			 "Missing status header in output: %s", output);
	zassert_not_null(strstr(output, "  Modem activated:              no"),
			 "Modem activated field not reflected correctly: %s", output);
	zassert_not_null(strstr(output, "  Cluster running:              no"),
			 "Cluster running field not reflected correctly: %s", output);
	zassert_not_null(strstr(output, "  Border router global IPv6 address: not set"),
			 "Border router prefix should be reported as not set: %s", output);
}

ZTEST(dect_rpc_client_status, test_dect_status_shell_full)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	struct dect_status_info sample;
	const char *output;
	size_t size;
	int err;

	dect_rpc_test_status_fill_sample(&sample);

	shell_backend_dummy_clear_output(sh);

	/* test_dect_status_shell_cmd runs first (alphabetically earlier name) and uses seq 0. */
	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_IF_STATUS, CBOR_UINT_SMALL(1)),
				   dect_rpc_test_status_rpc_rsp_from(&sample));
	err = shell_execute_cmd(sh, "dect status");
	mock_dect_rpc_tr_expect_done();

	zassert_ok(err, "dect status failed: %d", err);

	output = shell_backend_dummy_get_output(sh, &size);
	zassert_not_null(strstr(output, "  Modem activated:              yes"), "%s", output);
	zassert_not_null(strstr(output, "  Cluster running:              yes"), "%s", output);
	zassert_not_null(strstr(output, "  Cluster channel:              42"), "%s", output);
	zassert_not_null(strstr(output, "  Network beacon running:       yes"), "%s", output);
	zassert_not_null(strstr(output, "0xabcd1234"), "%s", output);
	zassert_not_null(strstr(output, "    Child #1 long RD ID:"), "%s", output);
	zassert_not_null(strstr(output, "    Child #2 long RD ID:"), "%s", output);
	zassert_not_null(strstr(output, "  Border router global IPv6 prefix/64:"), "%s", output);
	zassert_not_null(strstr(output, "  Modem FW version:             fw-1.2.3"), "%s", output);
}

ZTEST_SUITE(dect_rpc_client_status, NULL, NULL, tc_setup, NULL, NULL);
