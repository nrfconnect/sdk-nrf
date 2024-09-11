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

#define FOREACH_FAKE(f)                                                                            \
	f(otLinkSetPollPeriod);                                                                    \
	f(otLinkGetPollPeriod);                                                                    \
	f(otLinkGetExtendedAddress);                                                               \
	f(otLinkSetMaxFrameRetriesDirect);                                                         \
	f(otLinkSetMaxFrameRetriesIndirect);                                                       \
	f(otLinkSetEnabled);                                                                       \
	f(otLinkGetFactoryAssignedIeeeEui64);

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

ZTEST_SUITE(ot_rpc_link, NULL, NULL, tc_setup, NULL, NULL);
