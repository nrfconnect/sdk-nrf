/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_dect_rpc_transport.h>
#include <dect_rpc_ids.h>
#include <test_rpc_env.h>
#include "dect_mdm_mock_test_seam.h"
#include "server_dect_fixture.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static mock_dect_rpc_pkt_t rpc_test_if_send_cmd_pkt(const uint8_t *payload, size_t payload_len)
{
	static uint8_t buf[5 + 3 + 512 + 1];
	static mock_dect_rpc_pkt_t pkt;
	size_t pos = 0;

	buf[pos++] = 0x80;
	buf[pos++] = DECT_RPC_CMD_IF_SEND;
	buf[pos++] = 0xff;
	buf[pos++] = 0x00;
	buf[pos++] = 0x00;

	if (payload_len <= 23) {
		buf[pos++] = (uint8_t)(0x40 | payload_len);
	} else if (payload_len <= 255) {
		buf[pos++] = 0x58;
		buf[pos++] = (uint8_t)payload_len;
	} else if (payload_len <= 65535) {
		buf[pos++] = 0x59;
		buf[pos++] = (payload_len >> 8) & 0xff;
		buf[pos++] = payload_len & 0xff;
	} else {
		__ASSERT_NO_MSG(false);
	}
	memcpy(&buf[pos], payload, payload_len);
	pos += payload_len;
	buf[pos++] = 0xf6;

	pkt.data = buf;
	pkt.len = pos;
	return pkt;
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

ZTEST(dect_rpc_server_if_send_errors, test_if_send_no_long_rd_id_in_dst)
{
	int baseline = dect_mdm_mock_send_call_count;
	static const uint8_t pkt[40] = {
		0x60, 0, 0, 0, 0, 0, 0x3b, 0x40, 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	mock_dect_rpc_tr_expect_add(RPC_RSP(CBOR_NINT(22)), NO_RSP);
	mock_dect_rpc_tr_receive(rpc_test_if_send_cmd_pkt(pkt, sizeof(pkt)));
	mock_dect_rpc_tr_expect_done();

	zassert_equal(dect_mdm_mock_send_call_count, baseline,
		      "Driver send must not run when long_rd_id is missing");
}

ZTEST(dect_rpc_server_if_send_errors, test_if_send_payload_too_large)
{
	int baseline = dect_mdm_mock_send_call_count;
	static uint8_t pkt[501];
	mock_dect_rpc_pkt_t cmd;

	memset(pkt, 0, sizeof(pkt));
	pkt[0] = 0x60;
	/* long_rd_id in dst (last 4 bytes of IPv6 dst at offset 24+12). */
	pkt[39] = 0x01;
	__ASSERT_NO_MSG(sizeof(pkt) > 500);

	cmd = rpc_test_if_send_cmd_pkt(pkt, sizeof(pkt));

	mock_dect_rpc_tr_expect_add(RPC_RSP(CBOR_NINT8(90)), NO_RSP);
	mock_dect_rpc_tr_receive(cmd);
	mock_dect_rpc_tr_expect_done();

	zassert_equal(dect_mdm_mock_send_call_count, baseline,
		      "Driver send must not run for oversized RPC payload");
}

ZTEST_SUITE(dect_rpc_server_if_send_errors, NULL, suite_setup, tc_setup, NULL, NULL);
