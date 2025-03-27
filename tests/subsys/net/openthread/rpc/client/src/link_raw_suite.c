/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/link_raw.h>

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

/* Test serialization of otLinkRawGetRadioTime() which returns 0 */
ZTEST(ot_rpc_link_raw, test_otLinkRawGetRadioTime_0)
{
	uint64_t time;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_RAW_GET_RADIO_TIME), RPC_RSP(0));
	time = otLinkRawGetRadioTime(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(time, 0);
}

/* Test serialization of otLinkRawGetRadioTime() which returns UINT64_MAX */
ZTEST(ot_rpc_link_raw, test_otLinkRawGetRadioTime_max)
{
	uint64_t time;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_RAW_GET_RADIO_TIME),
				   RPC_RSP(CBOR_UINT64(UINT64_MAX)));
	time = otLinkRawGetRadioTime(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(time, UINT64_MAX);
}

ZTEST_SUITE(ot_rpc_link_raw, NULL, NULL, tc_setup, NULL, NULL);
