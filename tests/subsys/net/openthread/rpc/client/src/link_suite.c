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

/* Test serialization of otLinkGetChannel() */
ZTEST(ot_rpc_link, test_otLinkGetChannel)
{
	uint8_t channel;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_GET_CHANNEL),
				   RPC_RSP(CBOR_UINT8(UINT8_MAX)));
	channel = otLinkGetChannel(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(channel, UINT8_MAX);
}

/* Test serialization of otLinkGetExtendedAddress() */
ZTEST(ot_rpc_link, test_otLinkGetExtendedAddress)
{
	const otExtAddress exp_extaddr = {{INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)}};
	const otExtAddress *extaddr;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_GET_EXTENDED_ADDRESS),
				   RPC_RSP(EXT_ADDR));
	extaddr = otLinkGetExtendedAddress(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_mem_equal(extaddr, &exp_extaddr, OT_EXT_ADDRESS_SIZE);
}

/* Test serialization of otLinkGetPanId() */
ZTEST(ot_rpc_link, test_otLinkGetPanId)
{
	otPanId panid;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_GET_PAN_ID),
				   RPC_RSP(CBOR_UINT16(UINT16_MAX)));
	panid = otLinkGetPanId(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(panid, UINT16_MAX);
}

/* Test serialization of otLinkGetCounters() */
ZTEST(ot_rpc_link, test_otLinkGetCounters)
{
	const otMacCounters *counters;

#define COUNTER(i) (UINT32_MAX - i)
#define CBOR_COUNTER(i) CBOR_UINT32(COUNTER(i))

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_GET_COUNTERS),
				   RPC_RSP(CBOR_COUNTER(0), CBOR_COUNTER(1), CBOR_COUNTER(2),
					   CBOR_COUNTER(3), CBOR_COUNTER(4), CBOR_COUNTER(5),
					   CBOR_COUNTER(6), CBOR_COUNTER(7), CBOR_COUNTER(8),
					   CBOR_COUNTER(9), CBOR_COUNTER(10), CBOR_COUNTER(11),
					   CBOR_COUNTER(12), CBOR_COUNTER(13), CBOR_COUNTER(14),
					   CBOR_COUNTER(15), CBOR_COUNTER(16), CBOR_COUNTER(17),
					   CBOR_COUNTER(18), CBOR_COUNTER(19), CBOR_COUNTER(20),
					   CBOR_COUNTER(21), CBOR_COUNTER(22), CBOR_COUNTER(23),
					   CBOR_COUNTER(24), CBOR_COUNTER(25), CBOR_COUNTER(26),
					   CBOR_COUNTER(27), CBOR_COUNTER(28), CBOR_COUNTER(29),
					   CBOR_COUNTER(30), CBOR_COUNTER(31), CBOR_COUNTER(32),
					   CBOR_COUNTER(33)));
	counters = otLinkGetCounters(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(counters->mTxTotal, COUNTER(0));
	zassert_equal(counters->mTxUnicast, COUNTER(1));
	zassert_equal(counters->mTxBroadcast, COUNTER(2));
	zassert_equal(counters->mTxAckRequested, COUNTER(3));
	zassert_equal(counters->mTxAcked, COUNTER(4));
	zassert_equal(counters->mTxNoAckRequested, COUNTER(5));
	zassert_equal(counters->mTxData, COUNTER(6));
	zassert_equal(counters->mTxDataPoll, COUNTER(7));
	zassert_equal(counters->mTxBeacon, COUNTER(8));
	zassert_equal(counters->mTxBeaconRequest, COUNTER(9));
	zassert_equal(counters->mTxOther, COUNTER(10));
	zassert_equal(counters->mTxRetry, COUNTER(11));
	zassert_equal(counters->mTxDirectMaxRetryExpiry, COUNTER(12));
	zassert_equal(counters->mTxIndirectMaxRetryExpiry, COUNTER(13));
	zassert_equal(counters->mTxErrCca, COUNTER(14));
	zassert_equal(counters->mTxErrAbort, COUNTER(15));
	zassert_equal(counters->mTxErrBusyChannel, COUNTER(16));
	zassert_equal(counters->mRxTotal, COUNTER(17));
	zassert_equal(counters->mRxUnicast, COUNTER(18));
	zassert_equal(counters->mRxBroadcast, COUNTER(19));
	zassert_equal(counters->mRxData, COUNTER(20));
	zassert_equal(counters->mRxDataPoll, COUNTER(21));
	zassert_equal(counters->mRxBeacon, COUNTER(22));
	zassert_equal(counters->mRxBeaconRequest, COUNTER(23));
	zassert_equal(counters->mRxOther, COUNTER(24));
	zassert_equal(counters->mRxAddressFiltered, COUNTER(25));
	zassert_equal(counters->mRxDestAddrFiltered, COUNTER(26));
	zassert_equal(counters->mRxDuplicated, COUNTER(27));
	zassert_equal(counters->mRxErrNoFrame, COUNTER(28));
	zassert_equal(counters->mRxErrUnknownNeighbor, COUNTER(29));
	zassert_equal(counters->mRxErrInvalidSrcAddr, COUNTER(30));
	zassert_equal(counters->mRxErrSec, COUNTER(31));
	zassert_equal(counters->mRxErrFcs, COUNTER(32));
	zassert_equal(counters->mRxErrOther, COUNTER(33));

#undef COUNTER
#undef CBOR_COUNTER
}

ZTEST_SUITE(ot_rpc_link, NULL, NULL, tc_setup, NULL, NULL);
