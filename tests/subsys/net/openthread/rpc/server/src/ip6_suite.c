/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/cli.h>
#include <openthread/ip6.h>
#include <openthread/netdata.h>

/* Fake functions */

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(const otNetifAddress *, otIp6GetUnicastAddresses, otInstance *);
FAKE_VALUE_FUNC(const otNetifMulticastAddress *, otIp6GetMulticastAddresses, otInstance *);
FAKE_VALUE_FUNC(otError, otIp6SubscribeMulticastAddress, otInstance *, const otIp6Address *);
FAKE_VALUE_FUNC(otError, otIp6UnsubscribeMulticastAddress, otInstance *, const otIp6Address *);
FAKE_VALUE_FUNC(otError, otIp6SetEnabled, otInstance *, bool);
FAKE_VALUE_FUNC(bool, otIp6IsEnabled, otInstance *);

#define FOREACH_FAKE(f)                                                                            \
	f(otIp6GetUnicastAddresses);                                                               \
	f(otIp6GetMulticastAddresses);                                                             \
	f(otIp6SubscribeMulticastAddress);                                                         \
	f(otIp6UnsubscribeMulticastAddress);                                                       \
	f(otIp6SetEnabled);                                                                        \
	f(otIp6IsEnabled);

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
static otError subscribe_multicast_ff02_1_fake(otInstance *instance, const otIp6Address *addr)
{
	const otIp6Address exp_addr = {.mFields.m8 = {MADDR_FF02_1}};

	zassert_mem_equal(addr, &exp_addr, sizeof(exp_addr));

	return OT_ERROR_NONE;
}

ZTEST(ot_rpc_ip6, test_otIp6SubscribeMulticastAddress)
{
	otIp6SubscribeMulticastAddress_fake.custom_fake = subscribe_multicast_ff02_1_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_SUBSCRIBE_MADDR, 0x50, MADDR_FF02_1));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6SubscribeMulticastAddress_fake.call_count, 1);
}

/*
 * Test reception of otIp6SubscribeMulticastAddress(ff02::1) command.
 * Test serialization of the result: OT_ERROR_FAILED.
 */
static otError subscribe_multicast_ff02_1_failed_fake(otInstance *instance,
						      const otIp6Address *addr)
{
	const otIp6Address exp_addr = {.mFields.m8 = {MADDR_FF02_1}};

	zassert_mem_equal(addr, &exp_addr, sizeof(exp_addr));

	return OT_ERROR_FAILED;
}

ZTEST(ot_rpc_ip6, test_otIp6SubscribeMulticastAddress_failed)
{
	otIp6SubscribeMulticastAddress_fake.custom_fake = subscribe_multicast_ff02_1_failed_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x01), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_SUBSCRIBE_MADDR, 0x50, MADDR_FF02_1));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6SubscribeMulticastAddress_fake.call_count, 1);
}

/*
 * Test reception of otIp6UnubscribeMulticastAddress(ff02::1) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
static otError unsubscribe_multicast_ff02_1_fake(otInstance *instance, const otIp6Address *addr)
{
	const otIp6Address exp_addr = {.mFields.m8 = {MADDR_FF02_1}};

	zassert_mem_equal(addr, &exp_addr, sizeof(exp_addr));

	return OT_ERROR_NONE;
}

ZTEST(ot_rpc_ip6, test_otIp6UnsubscribeMulticastAddress)
{
	otIp6UnsubscribeMulticastAddress_fake.custom_fake = unsubscribe_multicast_ff02_1_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x00), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR, 0x50, MADDR_FF02_1));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6UnsubscribeMulticastAddress_fake.call_count, 1);
}

/*
 * Test reception of otIp6UnubscribeMulticastAddress(ff02::1) command.
 * Test serialization of the result: OT_ERROR_FAILED.
 */
static otError unsubscribe_multicast_ff02_1_failed_fake(otInstance *instance,
							const otIp6Address *addr)
{
	const otIp6Address exp_addr = {.mFields.m8 = {MADDR_FF02_1}};

	zassert_mem_equal(addr, &exp_addr, sizeof(exp_addr));

	return OT_ERROR_FAILED;
}

ZTEST(ot_rpc_ip6, test_otIp6UnsubscribeMulticastAddress_failed)
{
	otIp6UnsubscribeMulticastAddress_fake.custom_fake =
		unsubscribe_multicast_ff02_1_failed_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x01), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR, 0x50, MADDR_FF02_1));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6UnsubscribeMulticastAddress_fake.call_count, 1);
}

/*
 * Test reception of otIp6GetUnicastAddresses() command.
 * Test serialization of the result:
 * - fe80:aabb:aabb:aabb:aabb:aabb:aabb:aa01
 * - fe80:aabb:aabb:aabb:aabb:aabb:aabb:aa02
 */
ZTEST(ot_rpc_ip6, test_otIp6GetUnicastAddresses)
{
	const uint16_t flags1 = BIT(OT_RPC_NETIF_ADDRESS_PREFERRED_OFFSET) |
				BIT(OT_RPC_NETIF_ADDRESS_VALID_OFFSET) |
				BIT(OT_RPC_NETIF_ADDRESS_RLOC_OFFSET) |
				BIT(OT_RPC_NETIF_ADDRESS_MESH_LOCAL_OFFSET);
	const uint16_t flags2 = BIT(OT_RPC_NETIF_ADDRESS_SCOPE_VALID_OFFSET) |
				(ADDR_SCOPE << OT_RPC_NETIF_ADDRESS_SCOPE_OFFSET);

	otNetifAddress addr[2] = {
		{
			.mAddress.mFields.m8 = {ADDR_1},
			.mPrefixLength = ADDR_PREFIX_LENGTH,
			.mAddressOrigin = OT_ADDRESS_ORIGIN_MANUAL,
			.mPreferred = true,
			.mValid = true,
			.mRloc = true,
			.mMeshLocal = true,
		},
		{
			.mAddress.mFields.m8 = {ADDR_2},
			.mPrefixLength = ADDR_PREFIX_LENGTH,
			.mAddressOrigin = OT_ADDRESS_ORIGIN_DHCPV6,
			.mScopeOverrideValid = true,
			.mScopeOverride = ADDR_SCOPE,
		},
	};

	addr[0].mNext = &addr[1];
	otIp6GetUnicastAddresses_fake.return_val = addr;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x9f, 0x50, ADDR_1, CBOR_UINT8(ADDR_PREFIX_LENGTH),
					   OT_ADDRESS_ORIGIN_MANUAL, CBOR_UINT16(flags1), 0xff,
					   0x9f, 0x50, ADDR_2, CBOR_UINT8(ADDR_PREFIX_LENGTH),
					   OT_ADDRESS_ORIGIN_DHCPV6, CBOR_UINT8(flags2), 0xff),
				   NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_GET_UNICAST_ADDRESSES));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6GetUnicastAddresses_fake.call_count, 1);
}

/*
 * Test reception of otIp6GetMulticastAddresses() command.
 * Test serialization of the result:
 * - fe02::1
 * - fe02::2
 */
ZTEST(ot_rpc_ip6, test_otIp6GetMulticastAddresses)
{
	otNetifMulticastAddress addr[2] = {
		{
			.mAddress.mFields.m8 = {MADDR_FF02_1},
		},
		{
			.mAddress.mFields.m8 = {MADDR_FF02_2},
		},
	};

	addr[0].mNext = &addr[1];
	otIp6GetMulticastAddresses_fake.return_val = addr;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x50, MADDR_FF02_1, 0x50, MADDR_FF02_2), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_IP6_GET_MULTICAST_ADDRESSES));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otIp6GetMulticastAddresses_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_ip6, NULL, NULL, tc_setup, NULL, NULL);
