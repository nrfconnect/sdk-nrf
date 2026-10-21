/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>
#include "dect_mdm_mock_test_seam.h"
#include "rpc_test_link_state_expect.h"
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

ZTEST(dect_rpc_server_if_send, test_if_send_short_payload_rejected)
{
	int baseline = dect_mdm_mock_send_call_count;

	mock_dect_rpc_tr_expect_add(RPC_RSP(0x35), NO_RSP);
	mock_dect_rpc_tr_receive(RPC_CMD(
		DECT_RPC_CMD_IF_SEND,
		CBOR_BSTR(20, 0x60, 0, 0, 0, 0, 0, 0x3b, 0x40, 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0,
			 0, 0x01)));
	mock_dect_rpc_tr_expect_done();

	zassert_equal(dect_mdm_mock_send_call_count, baseline,
		      "Server must not reach the driver send() for a too-short payload");
}

ZTEST(dect_rpc_server_if_send, test_if_send_success)
{
	static const uint8_t expected_pkt[40] = {
		0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x40, 0xfe, 0x80, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0x01, 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x56, 0x78,
		0x9a, 0xbc,
	};

	EXPECT_LINK_STATE_TRANSITION(CBOR_TRUE, CBOR_FALSE);
	zassert_ok(server_dect_fixture_associate());
	k_msleep(100);
	mock_dect_rpc_tr_expect_reset();

	mock_dect_rpc_tr_expect_add(RPC_RSP(CBOR_UINT8(40)), NO_RSP);
#define TEST_IPV6_PKT                                                                              \
	0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x40, 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
		0, 0, 0, 0x01, 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x56, 0x78, 0x9a, 0xbc

	mock_dect_rpc_tr_receive(RPC_CMD(DECT_RPC_CMD_IF_SEND, CBOR_BSTR8(40, TEST_IPV6_PKT)));
	mock_dect_rpc_tr_expect_done();

	zassert_equal(dect_mdm_mock_last_send_long_rd_id, SERVER_DECT_FIXTURE_PARENT_LONG_RD_ID,
		      "Wrong long_rd_id derived from the IPv6 destination address");
	zassert_equal(dect_mdm_mock_last_send_len, sizeof(expected_pkt),
		      "Wrong sent packet length");
	zassert_mem_equal(dect_mdm_mock_last_send_data, expected_pkt, sizeof(expected_pkt),
			  "Sent packet payload does not match what was requested");

	EXPECT_LINK_STATE_TRANSITION(CBOR_TRUE, CBOR_TRUE);
	zassert_ok(server_dect_fixture_release());
	k_msleep(100);
	mock_dect_rpc_tr_expect_reset();
}

ZTEST_SUITE(dect_rpc_server_if_send, NULL, suite_setup, tc_setup, NULL, NULL);
