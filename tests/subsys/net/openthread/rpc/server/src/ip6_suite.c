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

#include <openthread/cli.h>
#include <openthread/ip6.h>
#include <openthread/netdata.h>

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
ZTEST(ot_rpc_ip6, test_otIp6SetEnabled_false)
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
ZTEST(ot_rpc_ip6, test_otIp6SetEnabled_true)
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
ZTEST(ot_rpc_ip6, test_otIp6SetEnabled_true_failed)
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
ZTEST(ot_rpc_ip6, test_otIp6IsEnabled_false)
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
ZTEST(ot_rpc_ip6, test_otIp6IsEnabled_true)
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
ZTEST(ot_rpc_ip6, test_otIp6SubscribeMulticastAddress)
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
ZTEST(ot_rpc_ip6, test_otIp6SubscribeMulticastAddress_failed)
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
ZTEST(ot_rpc_ip6, test_otIp6UnsubscribeMulticastAddress)
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
ZTEST(ot_rpc_ip6, test_otIp6UnsubscribeMulticastAddress_failed)
{
	const otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	otIp6UnsubscribeMulticastAddress_fake.return_val = OT_ERROR_FAILED;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x01), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR, 0x50, MADDR_FF02_1));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6UnsubscribeMulticastAddress_fake.call_count, 1);
	zassert_mem_equal(otIp6UnsubscribeMulticastAddress_fake.arg1_val, &addr, sizeof(addr));
}

ZTEST_SUITE(ot_rpc_ip6, NULL, NULL, tc_setup, NULL, NULL);
