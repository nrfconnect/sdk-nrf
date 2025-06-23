/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/link.h>

/* Fake functions */

FAKE_VALUE_FUNC(otError, otLinkSetPollPeriod, otInstance *, uint32_t);
FAKE_VALUE_FUNC(uint32_t, otLinkGetPollPeriod, otInstance *);
FAKE_VALUE_FUNC(const otExtAddress *, otLinkGetExtendedAddress, otInstance *);
FAKE_VOID_FUNC(otLinkSetMaxFrameRetriesDirect, otInstance *, uint8_t);
FAKE_VOID_FUNC(otLinkSetMaxFrameRetriesIndirect, otInstance *, uint8_t);
FAKE_VALUE_FUNC(otError, otLinkSetEnabled, otInstance *, bool);
FAKE_VOID_FUNC(otLinkGetFactoryAssignedIeeeEui64, otInstance *, otExtAddress *);
FAKE_VALUE_FUNC(uint8_t, otLinkGetChannel, otInstance *);
FAKE_VALUE_FUNC(otPanId, otLinkGetPanId, otInstance *);
FAKE_VALUE_FUNC(const otMacCounters *, otLinkGetCounters, otInstance *);

#define FOREACH_FAKE(f)                                                                            \
	f(otLinkSetPollPeriod);                                                                    \
	f(otLinkGetPollPeriod);                                                                    \
	f(otLinkGetExtendedAddress);                                                               \
	f(otLinkSetMaxFrameRetriesDirect);                                                         \
	f(otLinkSetMaxFrameRetriesIndirect);                                                       \
	f(otLinkSetEnabled);                                                                       \
	f(otLinkGetFactoryAssignedIeeeEui64);                                                      \
	f(otLinkGetChannel);                                                                       \
	f(otLinkGetPanId);                                                                         \
	f(otLinkGetCounters);

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
 * Test reception of otLinkSetPollPeriod(0) command.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_link, test_otLinkSetPollPeriod_0)
{
	otLinkSetPollPeriod_fake.return_val = OT_ERROR_INVALID_ARGS;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x07), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_SET_POLL_PERIOD, 0x00));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkSetPollPeriod_fake.call_count, 1);
	zassert_equal(otLinkSetPollPeriod_fake.arg1_val, 0);
}

/*
 * Test reception of otLinkSetPollPeriod(0x3ffffff) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_link, test_otLinkSetPollPeriod_max)
{
	otLinkSetPollPeriod_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_LINK_SET_POLL_PERIOD, 0x1a, 0x03, 0xff, 0xff, 0xff));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkSetPollPeriod_fake.call_count, 1);
	zassert_equal(otLinkSetPollPeriod_fake.arg1_val, 0x3ffffff);
}

/*
 * Test reception of otLinkGetPollPeriod() command.
 * Test serialization of the result: 10.
 */
ZTEST(ot_rpc_link, test_otLinkGetPollPeriod_min)
{
	otLinkGetPollPeriod_fake.return_val = 10;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x0a), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_GET_POLL_PERIOD));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkGetPollPeriod_fake.call_count, 1);
}

/*
 * Test reception of otLinkGetPollPeriod() command.
 * Test serialization of the result: 0x3ffffff.
 */
ZTEST(ot_rpc_link, test_otLinkGetPollPeriod_max)
{
	otLinkGetPollPeriod_fake.return_val = 0x3ffffff;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x1a, 0x03, 0xff, 0xff, 0xff), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_GET_POLL_PERIOD));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkGetPollPeriod_fake.call_count, 1);
}

/*
 * Test reception of otLinkSetMaxFrameRetriesDirect() command.
 */
ZTEST(ot_rpc_link, test_otLinkSetMaxFrameRetriesDirect)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_DIRECT, CBOR_UINT8(0x55)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkSetMaxFrameRetriesDirect_fake.call_count, 1);
	zassert_equal(otLinkSetMaxFrameRetriesDirect_fake.arg1_val, 0x55);
}

/*
 * Test reception of otLinkSetMaxFrameRetriesIndirect() command.
 */
ZTEST(ot_rpc_link, test_otLinkSetMaxFrameRetriesIndirect)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_INDIRECT, CBOR_UINT8(0xAA)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkSetMaxFrameRetriesIndirect_fake.call_count, 1);
	zassert_equal(otLinkSetMaxFrameRetriesIndirect_fake.arg1_val, 0xAA);
}

/*
 * Test reception of otLinkSetEnabled() command.
 */
ZTEST(ot_rpc_link, test_otLinkSetEnabled)
{
	otLinkGetPollPeriod_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_SET_ENABLED, CBOR_TRUE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkSetEnabled_fake.call_count, 1);
	zassert_equal(otLinkSetEnabled_fake.arg1_val, true);
}

static void factory_assigned_eui64_get_fake(otInstance *instance, otExtAddress *eui64)
{
	otExtAddress ext_addr = {{INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)}};

	memcpy(eui64->m8, ext_addr.m8, OT_EXT_ADDRESS_SIZE);
}

/*
 * Test reception of otLinkGetFactoryAssignedIeeeEui64() command.
 */
ZTEST(ot_rpc_link, test_otLinkGetFactoryAssignedIeeeEui64)
{
	otLinkGetFactoryAssignedIeeeEui64_fake.custom_fake = factory_assigned_eui64_get_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(EXT_ADDR), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_GET_FACTORY_ASSIGNED_EUI64));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkGetFactoryAssignedIeeeEui64_fake.call_count, 1);
}

/*
 * Test reception of otLinkGetChannel() command.
 * Test serialization of the result: UINT8_MAX.
 */
ZTEST(ot_rpc_link, test_otLinkGetChannel)
{
	otLinkGetChannel_fake.return_val = UINT8_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT8(UINT8_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_GET_CHANNEL));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkGetChannel_fake.call_count, 1);
}

/*
 * Test reception of otLinkGetExtendedAddress() command.
 */
ZTEST(ot_rpc_link, test_otLinkGetExtendedAddress)
{
	const otExtAddress ext_addr = {{INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)}};

	otLinkGetExtendedAddress_fake.return_val = &ext_addr;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(EXT_ADDR), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_GET_EXTENDED_ADDRESS));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkGetExtendedAddress_fake.call_count, 1);
}

/*
 * Test reception of otLinkGetPanId() command.
 * Test serialization of the result: UINT16_MAX.
 */
ZTEST(ot_rpc_link, test_otLinkGetPanId)
{
	otLinkGetPanId_fake.return_val = UINT16_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT16(UINT16_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_GET_PAN_ID));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkGetPanId_fake.call_count, 1);
}

/*
 * Test reception of otLinkGetCounters() command.
 * Test serialization of the result: counters.
 */
ZTEST(ot_rpc_link, test_otLinkGetCounters)
{
	otMacCounters counters;

#define COUNTER(i) (UINT32_MAX - i)
#define CBOR_COUNTER(i) CBOR_UINT32(COUNTER(i))

	counters.mTxTotal = COUNTER(0);
	counters.mTxUnicast = COUNTER(1);
	counters.mTxBroadcast = COUNTER(2);
	counters.mTxAckRequested = COUNTER(3);
	counters.mTxAcked = COUNTER(4);
	counters.mTxNoAckRequested = COUNTER(5);
	counters.mTxData = COUNTER(6);
	counters.mTxDataPoll = COUNTER(7);
	counters.mTxBeacon = COUNTER(8);
	counters.mTxBeaconRequest = COUNTER(9);
	counters.mTxOther = COUNTER(10);
	counters.mTxRetry = COUNTER(11);
	counters.mTxDirectMaxRetryExpiry = COUNTER(12);
	counters.mTxIndirectMaxRetryExpiry = COUNTER(13);
	counters.mTxErrCca = COUNTER(14);
	counters.mTxErrAbort = COUNTER(15);
	counters.mTxErrBusyChannel = COUNTER(16);
	counters.mRxTotal = COUNTER(17);
	counters.mRxUnicast = COUNTER(18);
	counters.mRxBroadcast = COUNTER(19);
	counters.mRxData = COUNTER(20);
	counters.mRxDataPoll = COUNTER(21);
	counters.mRxBeacon = COUNTER(22);
	counters.mRxBeaconRequest = COUNTER(23);
	counters.mRxOther = COUNTER(24);
	counters.mRxAddressFiltered = COUNTER(25);
	counters.mRxDestAddrFiltered = COUNTER(26);
	counters.mRxDuplicated = COUNTER(27);
	counters.mRxErrNoFrame = COUNTER(28);
	counters.mRxErrUnknownNeighbor = COUNTER(29);
	counters.mRxErrInvalidSrcAddr = COUNTER(30);
	counters.mRxErrSec = COUNTER(31);
	counters.mRxErrFcs = COUNTER(32);
	counters.mRxErrOther = COUNTER(33);
	otLinkGetCounters_fake.return_val = &counters;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_COUNTER(0), CBOR_COUNTER(1), CBOR_COUNTER(2),
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
					   CBOR_COUNTER(33)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_GET_COUNTERS));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkGetCounters_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_link, NULL, NULL, tc_setup, NULL, NULL);
