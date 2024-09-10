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

#include <openthread/link.h>

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

/* Test serialization of failed otLinkSetPollPeriod(rdn) */
ZTEST(ot_rpc_link, test_otLinkSetPollPeriod_0)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_SET_POLL_PERIOD, 0x00), RPC_RSP(0x07));
	error = otLinkSetPollPeriod(NULL, 0);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test serialization of max allowed otLinkSetPollPeriod(0x3ffffff) */
ZTEST(ot_rpc_link, test_otLinkSetPollPeriod_max)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_LINK_SET_POLL_PERIOD, 0x1a, 0x03, 0xff, 0xff, 0xff),
		RPC_RSP(0x00));
	error = otLinkSetPollPeriod(NULL, 0x3ffffff);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otLinkGetPollPeriod() returning min allowed 10 */
ZTEST(ot_rpc_link, test_otLinkGetPollPeriod_min)
{
	uint32_t period;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_GET_POLL_PERIOD), RPC_RSP(0x0a));
	period = otLinkGetPollPeriod(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(period, 10);
}

/* Test serialization of otLinkGetPollPeriod() returning max allowed 0x3ffffff */
ZTEST(ot_rpc_link, test_otLinkGetPollPeriod_max)
{
	uint32_t period;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_GET_POLL_PERIOD),
				   RPC_RSP(0x1a, 0x03, 0xff, 0xff, 0xff));
	period = otLinkGetPollPeriod(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(period, 0x3ffffff);
}

/* Test serialization of otLinkSetMaxFrameRetriesDirect() */
ZTEST(ot_rpc_link, test_otLinkSetMaxFrameRetriesDirect)
{
	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_DIRECT, CBOR_UINT8(0x55)), RPC_RSP());
	otLinkSetMaxFrameRetriesDirect(NULL, 0x55);
	mock_nrf_rpc_tr_expect_done();
}

/* Test serialization of otLinkSetMaxFrameRetriesIndirect() */
ZTEST(ot_rpc_link, test_otLinkSetMaxFrameRetriesIndirect)
{
	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_INDIRECT, CBOR_UINT8(0xAA)),
		RPC_RSP());
	otLinkSetMaxFrameRetriesIndirect(NULL, 0xAA);
	mock_nrf_rpc_tr_expect_done();
}

/* Test serialization of otLinkSetEnabled() */
ZTEST(ot_rpc_link, test_otLinkSetEnabled)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_SET_ENABLED, CBOR_TRUE),
				   RPC_RSP(OT_ERROR_NONE));
	error = otLinkSetEnabled(NULL, true);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otLinkGetFactoryAssignedIeeeEui64() */
ZTEST(ot_rpc_link, test_otLinkGetFactoryAssignedIeeeEui64)
{
	otExtAddress ext_addr;
	otExtAddress expected_ext_addr = {{INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)}};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_GET_FACTORY_ASSIGNED_EUI64),
				   RPC_RSP(EXT_ADDR));
	otLinkGetFactoryAssignedIeeeEui64(NULL, &ext_addr);
	mock_nrf_rpc_tr_expect_done();
	zassert_mem_equal(expected_ext_addr.m8, ext_addr.m8, OT_EXT_ADDRESS_SIZE);
}

ZTEST_SUITE(ot_rpc_link, NULL, NULL, tc_setup, NULL, NULL);
