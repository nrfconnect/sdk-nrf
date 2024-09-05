/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/cli.h>
#include <openthread/ip6.h>
#include <openthread/thread.h>
#include <openthread/netdata.h>

#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

/* Fake functions */

DEFINE_FFF_GLOBALS;
FAKE_VOID_FUNC(otCliInit, otInstance *, otCliOutputCallback, void *);
FAKE_VOID_FUNC(otCliInputLine, char *);
FAKE_VALUE_FUNC(const otNetifAddress *, otIp6GetUnicastAddresses, otInstance *);
FAKE_VALUE_FUNC(const otNetifMulticastAddress *, otIp6GetMulticastAddresses, otInstance *);
FAKE_VALUE_FUNC(otError, otIp6SubscribeMulticastAddress, otInstance *, const otIp6Address *);
FAKE_VALUE_FUNC(otError, otIp6UnsubscribeMulticastAddress, otInstance *, const otIp6Address *);
FAKE_VALUE_FUNC(otError, otIp6SetEnabled, otInstance *, bool);
FAKE_VALUE_FUNC(bool, otIp6IsEnabled, otInstance *);
FAKE_VALUE_FUNC(otError, otThreadSetEnabled, otInstance *, bool);
FAKE_VALUE_FUNC(otDeviceRole, otThreadGetDeviceRole, otInstance *);
FAKE_VALUE_FUNC(otError, otThreadSetLinkMode, otInstance *, otLinkModeConfig);
FAKE_VALUE_FUNC(otLinkModeConfig, otThreadGetLinkMode, otInstance *);
FAKE_VALUE_FUNC(otError, otLinkSetPollPeriod, otInstance *, uint32_t);
FAKE_VALUE_FUNC(uint32_t, otLinkGetPollPeriod, otInstance *);
FAKE_VALUE_FUNC(const otExtAddress *, otLinkGetExtendedAddress, otInstance *);
FAKE_VALUE_FUNC(otError, otSetStateChangedCallback, otInstance *, otStateChangedCallback, void *);
FAKE_VOID_FUNC(otRemoveStateChangeCallback, otInstance *, otStateChangedCallback, void *);
FAKE_VOID_FUNC(otLinkSetMaxFrameRetriesDirect, otInstance *, uint8_t);
FAKE_VOID_FUNC(otLinkSetMaxFrameRetriesIndirect, otInstance *, uint8_t);
FAKE_VALUE_FUNC(otError, otLinkSetEnabled, otInstance *, bool);
FAKE_VOID_FUNC(otLinkGetFactoryAssignedIeeeEui64, otInstance *, otExtAddress *);
FAKE_VALUE_FUNC(otError, otNetDataGet, otInstance *, bool, uint8_t *, uint8_t *);
FAKE_VALUE_FUNC(otError, otNetDataGetNextService, otInstance *, otNetworkDataIterator *,
		otServiceConfig *);
FAKE_VALUE_FUNC(otError, otNetDataGetNextOnMeshPrefix, otInstance *, otNetworkDataIterator *,
		otBorderRouterConfig *);

#define FOREACH_FAKE(f)                                                                            \
	f(otCliInit);                                                                              \
	f(otCliInputLine);                                                                         \
	f(otIp6GetUnicastAddresses);                                                               \
	f(otIp6GetMulticastAddresses);                                                             \
	f(otIp6SubscribeMulticastAddress);                                                         \
	f(otIp6UnsubscribeMulticastAddress);                                                       \
	f(otIp6SetEnabled);                                                                        \
	f(otIp6IsEnabled);                                                                         \
	f(otThreadSetEnabled);                                                                     \
	f(otThreadGetDeviceRole);                                                                  \
	f(otThreadSetLinkMode);                                                                    \
	f(otThreadGetLinkMode);                                                                    \
	f(otLinkSetPollPeriod);                                                                    \
	f(otLinkGetPollPeriod);                                                                    \
	f(otLinkGetExtendedAddress);                                                               \
	f(otSetStateChangedCallback);                                                              \
	f(otRemoveStateChangeCallback);                                                            \
	f(otLinkSetMaxFrameRetriesDirect);                                                         \
	f(otLinkSetMaxFrameRetriesIndirect);                                                       \
	f(otLinkSetEnabled);                                                                       \
	f(otLinkGetFactoryAssignedIeeeEui64);                                                      \
	f(otNetDataGet);                                                                           \
	f(otNetDataGetNextService);                                                                \
	f(otNetDataGetNextOnMeshPrefix);

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
 * Test reception of otIp6SetEnabled(false) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_server, test_otIp6SetEnabled_false)
{
	otIp6SetEnabled_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_SET_ENABLED, CBOR_FALSE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6SetEnabled_fake.call_count, 1);
	zassert_equal(otIp6SetEnabled_fake.arg1_val, false);
}

/*
 * Test reception of otIp6SetEnabled(true) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_server, test_otIp6SetEnabled_true)
{
	otIp6SetEnabled_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_SET_ENABLED, CBOR_TRUE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6SetEnabled_fake.call_count, 1);
	zassert_equal(otIp6SetEnabled_fake.arg1_val, true);
}

/*
 * Test reception of otIp6SetEnabled(true) command.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_server, test_otIp6SetEnabled_true_failed)
{
	otIp6SetEnabled_fake.return_val = OT_ERROR_INVALID_ARGS;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x07), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_SET_ENABLED, CBOR_TRUE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6SetEnabled_fake.call_count, 1);
	zassert_equal(otIp6SetEnabled_fake.arg1_val, true);
}

/*
 * Test reception of otIp6IsEnabled() command.
 * Test serialization of the result: false.
 */
ZTEST(ot_rpc_server, test_otIp6IsEnabled_false)
{
	otIp6IsEnabled_fake.return_val = false;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_FALSE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_IS_ENABLED));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6IsEnabled_fake.call_count, 1);
}

/*
 * Test reception of otIp6IsEnabled() command.
 * Test serialization of the result: true.
 */
ZTEST(ot_rpc_server, test_otIp6IsEnabled_true)
{
	otIp6IsEnabled_fake.return_val = true;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_TRUE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_IS_ENABLED));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6IsEnabled_fake.call_count, 1);
}

/*
 * Test reception of otIp6SubscribeMulticastAddress(ff02::1) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_server, test_otIp6SubscribeMulticastAddress)
{
	const otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	otIp6SubscribeMulticastAddress_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_SUBSCRIBE_MADDR, 0x50, MADDR_FF02_1));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6SubscribeMulticastAddress_fake.call_count, 1);
	zassert_mem_equal(otIp6SubscribeMulticastAddress_fake.arg1_val, &addr, sizeof(addr));
}

/*
 * Test reception of otIp6SubscribeMulticastAddress(ff02::1) command.
 * Test serialization of the result: OT_ERROR_FAILED.
 */
ZTEST(ot_rpc_server, test_otIp6SubscribeMulticastAddress_failed)
{
	const otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	otIp6SubscribeMulticastAddress_fake.return_val = OT_ERROR_FAILED;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x01), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_SUBSCRIBE_MADDR, 0x50, MADDR_FF02_1));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6SubscribeMulticastAddress_fake.call_count, 1);
	zassert_mem_equal(otIp6SubscribeMulticastAddress_fake.arg1_val, &addr, sizeof(addr));
}

/*
 * Test reception of otIp6UnubscribeMulticastAddress(ff02::1) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_server, test_otIp6UnsubscribeMulticastAddress)
{
	const otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	otIp6UnsubscribeMulticastAddress_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR, 0x50, MADDR_FF02_1));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6UnsubscribeMulticastAddress_fake.call_count, 1);
	zassert_mem_equal(otIp6UnsubscribeMulticastAddress_fake.arg1_val, &addr, sizeof(addr));
}

/*
 * Test reception of otIp6UnubscribeMulticastAddress(ff02::1) command.
 * Test serialization of the result: OT_ERROR_FAILED.
 */
ZTEST(ot_rpc_server, test_otIp6UnsubscribeMulticastAddress_failed)
{
	const otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	otIp6UnsubscribeMulticastAddress_fake.return_val = OT_ERROR_FAILED;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x01), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR, 0x50, MADDR_FF02_1));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6UnsubscribeMulticastAddress_fake.call_count, 1);
	zassert_mem_equal(otIp6UnsubscribeMulticastAddress_fake.arg1_val, &addr, sizeof(addr));
}

/*
 * Test reception of otThreadSetEnabled(false) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_server, test_otThreadSetEnabled_false)
{
	otThreadSetEnabled_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_SET_ENABLED, CBOR_FALSE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadSetEnabled_fake.call_count, 1);
	zassert_equal(otThreadSetEnabled_fake.arg1_val, false);
}

/*
 * Test reception of otThreadSetEnabled(false) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_server, test_otThreadSetEnabled_true)
{
	otThreadSetEnabled_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_SET_ENABLED, CBOR_TRUE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadSetEnabled_fake.call_count, 1);
	zassert_equal(otThreadSetEnabled_fake.arg1_val, true);
}

/*
 * Test reception of otThreadSetEnabled(false) command.
 * Test serialization of the result: OT_ERROR_FAILED.
 */
ZTEST(ot_rpc_server, test_otThreadSetEnabled_true_failed)
{
	otThreadSetEnabled_fake.return_val = OT_ERROR_FAILED;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x01), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_SET_ENABLED, CBOR_TRUE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadSetEnabled_fake.call_count, 1);
	zassert_equal(otThreadSetEnabled_fake.arg1_val, true);
}

/*
 * Test reception of otThreadGetDeviceRole() command.
 * Test serialization of the result: disabled.
 */
ZTEST(ot_rpc_server, test_otThreadGetDeviceRole_disabled)
{
	otThreadGetDeviceRole_fake.return_val = OT_DEVICE_ROLE_DISABLED;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_DEVICE_ROLE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetDeviceRole_fake.call_count, 1);
}

/*
 * Test reception of otThreadGetDeviceRole() command.
 * Test serialization of the result: leader.
 */
ZTEST(ot_rpc_server, test_otThreadGetDeviceRole_leader)
{
	otThreadGetDeviceRole_fake.return_val = OT_DEVICE_ROLE_LEADER;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x04), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_DEVICE_ROLE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetDeviceRole_fake.call_count, 1);
}

/*
 * Test reception of otThreadSetLinkMode(none) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_server, test_otThreadSetLinkMode_none)
{
	otThreadSetLinkMode_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_SET_LINK_MODE, 0x00));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadSetLinkMode_fake.call_count, 1);
	zassert_false(otThreadSetLinkMode_fake.arg1_val.mRxOnWhenIdle);
	zassert_false(otThreadSetLinkMode_fake.arg1_val.mDeviceType);
	zassert_false(otThreadSetLinkMode_fake.arg1_val.mNetworkData);
}

/*
 * Test reception of otThreadSetLinkMode(rdn) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_server, test_otThreadSetLinkMode_rdn)
{
	otThreadSetLinkMode_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_SET_LINK_MODE, 0x07));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadSetLinkMode_fake.call_count, 1);
	zassert_true(otThreadSetLinkMode_fake.arg1_val.mRxOnWhenIdle);
	zassert_true(otThreadSetLinkMode_fake.arg1_val.mDeviceType);
	zassert_true(otThreadSetLinkMode_fake.arg1_val.mNetworkData);
}

/*
 * Test reception of otThreadSetLinkMode(n) command.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_server, test_otThreadSetLinkMode_n)
{
	otThreadSetLinkMode_fake.return_val = OT_ERROR_INVALID_ARGS;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x07), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_SET_LINK_MODE, 0x04));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadSetLinkMode_fake.call_count, 1);
	zassert_false(otThreadSetLinkMode_fake.arg1_val.mRxOnWhenIdle);
	zassert_false(otThreadSetLinkMode_fake.arg1_val.mDeviceType);
	zassert_true(otThreadSetLinkMode_fake.arg1_val.mNetworkData);
}

/*
 * Test reception of otThreadGetLinkMode() command.
 * Test serialization of the result: none.
 */
ZTEST(ot_rpc_server, test_otThreadGetLinkMode_none)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_LINK_MODE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetLinkMode_fake.call_count, 1);
}

/*
 * Test reception of otThreadGetLinkMode() command.
 * Test serialization of the result: rdn.
 */
ZTEST(ot_rpc_server, test_otThreadGetLinkMode_rdn)
{
	otThreadGetLinkMode_fake.return_val.mRxOnWhenIdle = true;
	otThreadGetLinkMode_fake.return_val.mDeviceType = true;
	otThreadGetLinkMode_fake.return_val.mNetworkData = true;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x07), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_LINK_MODE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetLinkMode_fake.call_count, 1);
}

/*
 * Test reception of otLinkSetPollPeriod(0) command.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_server, test_otLinkSetPollPeriod_0)
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
ZTEST(ot_rpc_server, test_otLinkSetPollPeriod_max)
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
ZTEST(ot_rpc_server, test_otLinkGetPollPeriod_min)
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
ZTEST(ot_rpc_server, test_otLinkGetPollPeriod_max)
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
ZTEST(ot_rpc_server, test_otLinkSetMaxFrameRetriesDirect)
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
ZTEST(ot_rpc_server, test_otLinkSetMaxFrameRetriesIndirect)
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
ZTEST(ot_rpc_server, test_otLinkSetEnabled)
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
ZTEST(ot_rpc_server, test_otLinkGetFactoryAssignedIeeeEui64)
{
	otLinkGetFactoryAssignedIeeeEui64_fake.custom_fake = factory_assigned_eui64_get_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(EXT_ADDR), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_LINK_GET_FACTORY_ASSIGNED_EUI64));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otLinkGetFactoryAssignedIeeeEui64_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_server, NULL, NULL, tc_setup, NULL, NULL);
