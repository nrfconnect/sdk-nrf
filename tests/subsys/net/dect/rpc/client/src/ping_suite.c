/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Tests dect_rpc_client_ping(): verifies the client sends a DECT_RPC_CMD_PING command
 * over the (mocked) transport, and correctly reports success or timeout depending on
 * whether/what response it receives. This exercises subsys/net/dect/rpc/client/dect_rpc_if.c
 * directly; the mock transport only stands in for the real UART link.
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "dect_rpc_client_net.h"

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

/*
 * Both scenarios live in one ZTEST case (rather than two) so the order in which the
 * shared dect_rpc_ping_seq counter (static, file-scope in client/dect_rpc_if.c) is
 * consumed is deterministic: seq=0 for the first ping, seq=1..3 for the second.
 */
ZTEST(dect_rpc_client_ping, test_ping)
{
	int err;

	/* 1. Server responds promptly: dect_rpc_client_ping() must return 0. */
	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_PING, CBOR_UINT_SMALL(0)),
				   RPC_RSP(CBOR_UINT_SMALL(0)));
	err = dect_rpc_client_ping(K_MSEC(2000));
	mock_dect_rpc_tr_expect_done();
	zassert_equal(err, 0, "Expected successful ping, got %d", err);

	/*
	 * 2. No response is ever sent back. nrf_rpc_cbor_cmd() (used by
	 * dect_rpc_client_ping()) blocks *internally* on CONFIG_NRF_RPC_RESPONSE_TIMEOUT
	 * for every attempt (see wait_for_response() in nrf_rpc_cmd_common(),
	 * nrfxlib/nrf_rpc/nrf_rpc.c) and returns non-zero on timeout -- so
	 * dect_rpc_client_ping()'s own retry loop (on `ret != 0`) fires all
	 * DECT_RPC_PING_ATTEMPTS (3) times, each sending a new PING with an incrementing
	 * sequence number, before giving up with -EIO. The `timeout` parameter passed to
	 * dect_rpc_client_ping() only bounds the *outer* semaphore wait after a *successful*
	 * send+response cycle (scenario 1 above); it does not shorten this retry loop.
	 *
	 * Do NOT call mock_dect_rpc_tr_expect_done() here: with no response ever queued, the
	 * mock's completion semaphore is never signaled and the call would block forever.
	 * mock_dect_rpc_tr_expect_reset() clears the expectations instead (the outgoing bytes
	 * of each attempt were already checked synchronously inside send()).
	 */
	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_PING, CBOR_UINT_SMALL(1)), NO_RSP);
	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_PING, CBOR_UINT_SMALL(2)), NO_RSP);
	mock_dect_rpc_tr_expect_add(RPC_CMD(DECT_RPC_CMD_PING, CBOR_UINT_SMALL(3)), NO_RSP);
	err = dect_rpc_client_ping(K_MSEC(200));
	mock_dect_rpc_tr_expect_reset();
	zassert_equal(err, -EIO, "Expected -EIO after exhausting retries, got %d", err);
}

ZTEST_SUITE(dect_rpc_client_ping, NULL, NULL, tc_setup, NULL, NULL);
