/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/link_raw.h>

/* Fake functions */

FAKE_VALUE_FUNC(uint64_t, otLinkRawGetRadioTime, otInstance *);

#define FOREACH_FAKE(f) f(otLinkRawGetRadioTime);

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void tc_setup(void *f)
{
	mock_nrf_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_nrf_rpc_tr_expect_reset();

	FOREACH_FAKE(RESET_FAKE);
	FFF_RESET_HISTORY();
}

/*
 * Test reception of otLinkRawGetRadioTime() command.
 * Test serialization of the result: 0.
 */
ZTEST(ot_rpc_link_raw, test_otLinkRawGetRadioTime_0)
{
	otLinkRawGetRadioTime_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_RAW_GET_RADIO_TIME));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkRawGetRadioTime_fake.call_count, 1);
}

/*
 * Test reception of otLinkRawGetRadioTime() command.
 * Test serialization of the result: UINT64_MAX.
 */
ZTEST(ot_rpc_link_raw, test_otLinkRawGetRadioTime_max)
{
	otLinkRawGetRadioTime_fake.return_val = UINT64_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT64(UINT64_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_RAW_GET_RADIO_TIME));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkRawGetRadioTime_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_link_raw, NULL, NULL, tc_setup, NULL, NULL);
