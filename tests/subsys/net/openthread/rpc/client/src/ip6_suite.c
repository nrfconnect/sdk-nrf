/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <test_rpc_env.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/ip6.h>

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

/* Test serialization of otIp6SetEnabled(false) */
ZTEST(ot_rpc_ip6, test_otIp6SetEnabled_false)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_SET_ENABLED, CBOR_FALSE), RPC_RSP(0x00));
	error = otIp6SetEnabled(NULL, false);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otIp6SetEnabled(true) */
ZTEST(ot_rpc_ip6, test_otIp6SetEnabled_true)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_SET_ENABLED, CBOR_TRUE), RPC_RSP(0x00));
	error = otIp6SetEnabled(NULL, true);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of failed otIp6SetEnabled(true) */
ZTEST(ot_rpc_ip6, test_otIp6SetEnabled_true_failed)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_SET_ENABLED, CBOR_TRUE), RPC_RSP(0x01));
	error = otIp6SetEnabled(NULL, true);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_FAILED);
}

/* Test serialization of otIp6IsEnabled returning false */
ZTEST(ot_rpc_ip6, test_otIp6IsEnabled_false)
{
	bool enabled;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_IS_ENABLED), RPC_RSP(CBOR_FALSE));
	enabled = otIp6IsEnabled(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_false(enabled);
}

/* Test serialization of otIp6IsEnabled returning true */
ZTEST(ot_rpc_ip6, test_otIp6IsEnabled_true)
{
	bool enabled;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_IS_ENABLED), RPC_RSP(CBOR_TRUE));
	enabled = otIp6IsEnabled(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_true(enabled);
}

/* Test serialization of otIp6SubscribeMulticastAddress(ff02::1) */
ZTEST(ot_rpc_ip6, test_otIp6SubscribeMulticastAddress)
{
	otError error;
	otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_SUBSCRIBE_MADDR, 0x50, MADDR_FF02_1),
				   RPC_RSP(0x00));
	error = otIp6SubscribeMulticastAddress(NULL, &addr);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of failed otIp6SubscribeMulticastAddress(ff02::1) */
ZTEST(ot_rpc_ip6, test_otIp6SubscribeMulticastAddress_failed)
{
	otError error;
	otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_SUBSCRIBE_MADDR, 0x50, MADDR_FF02_1),
				   RPC_RSP(0x01));
	error = otIp6SubscribeMulticastAddress(NULL, &addr);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_FAILED);
}

/* Test serialization of otIp6UnsubscribeMulticastAddress(ff02::1) */
ZTEST(ot_rpc_ip6, test_otIp6UnsubscribeMulticastAddress)
{
	otError error;
	otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR, 0x50, MADDR_FF02_1),
				   RPC_RSP(0x00));
	error = otIp6UnsubscribeMulticastAddress(NULL, &addr);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of failed otIp6UnsubscribeMulticastAddress(ff02::1) */
ZTEST(ot_rpc_ip6, test_otIp6UnsubscribeMulticastAddress_failed)
{
	otError error;
	otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR, 0x50, MADDR_FF02_1),
				   RPC_RSP(0x01));
	error = otIp6UnsubscribeMulticastAddress(NULL, &addr);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_FAILED);
}

/* Test serialization of otIp6GetUnicastAddresses() */
ZTEST(ot_rpc_ip6, test_otIp6GetUnicastAddresses)
{
	const otIp6Address exp_addr1 = {.mFields.m8 = {ADDR_1}};
	const otIp6Address exp_addr2 = {.mFields.m8 = {ADDR_2}};
	const uint16_t exp_flags1 = BIT(OT_RPC_NETIF_ADDRESS_PREFERRED_OFFSET) |
				    BIT(OT_RPC_NETIF_ADDRESS_VALID_OFFSET) |
				    BIT(OT_RPC_NETIF_ADDRESS_RLOC_OFFSET) |
				    BIT(OT_RPC_NETIF_ADDRESS_MESH_LOCAL_OFFSET);
	const uint16_t exp_flags2 = BIT(OT_RPC_NETIF_ADDRESS_SCOPE_VALID_OFFSET) |
				    (ADDR_SCOPE << OT_RPC_NETIF_ADDRESS_SCOPE_OFFSET);

	const otNetifAddress *addr;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_GET_UNICAST_ADDRESSES),
				   RPC_RSP(0x9f, 0x50, ADDR_1, CBOR_UINT8(ADDR_PREFIX_LENGTH),
					   OT_ADDRESS_ORIGIN_MANUAL, CBOR_UINT16(exp_flags1), 0xff,
					   0x9f, 0x50, ADDR_2, CBOR_UINT8(ADDR_PREFIX_LENGTH),
					   OT_ADDRESS_ORIGIN_DHCPV6, CBOR_UINT16(exp_flags2),
					   0xff));
	addr = otIp6GetUnicastAddresses(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_not_null(addr);
	zassert_mem_equal(&addr->mAddress, &exp_addr1, OT_IP6_ADDRESS_SIZE);
	zassert_equal(addr->mPrefixLength, ADDR_PREFIX_LENGTH);
	zassert_equal(addr->mAddressOrigin, OT_ADDRESS_ORIGIN_MANUAL);
	zassert_true(addr->mPreferred);
	zassert_true(addr->mValid);
	zassert_false(addr->mScopeOverrideValid);
	zassert_true(addr->mRloc);
	zassert_true(addr->mMeshLocal);
	addr = addr->mNext;

	zassert_not_null(addr);
	zassert_mem_equal(&addr->mAddress, &exp_addr2, OT_IP6_ADDRESS_SIZE);
	zassert_equal(addr->mPrefixLength, ADDR_PREFIX_LENGTH);
	zassert_equal(addr->mAddressOrigin, OT_ADDRESS_ORIGIN_DHCPV6);
	zassert_false(addr->mPreferred);
	zassert_false(addr->mValid);
	zassert_true(addr->mScopeOverrideValid);
	zassert_equal(addr->mScopeOverride, ADDR_SCOPE);
	zassert_false(addr->mRloc);
	zassert_false(addr->mMeshLocal);
	addr = addr->mNext;

	zassert_is_null(addr);
}

/* Test serialization of otIp6GetMulticastAddresses() */
ZTEST(ot_rpc_ip6, test_otIp6GetMulticastAddresses)
{
	const otIp6Address exp_addr1 = {.mFields.m8 = {MADDR_FF02_1}};
	const otIp6Address exp_addr2 = {.mFields.m8 = {MADDR_FF02_2}};

	const otNetifMulticastAddress *maddr;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_GET_MULTICAST_ADDRESSES),
				   RPC_RSP(0x50, MADDR_FF02_1, 0x50, MADDR_FF02_2));
	maddr = otIp6GetMulticastAddresses(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_not_null(maddr);
	zassert_mem_equal(&maddr->mAddress, &exp_addr1, OT_IP6_ADDRESS_SIZE);
	maddr = maddr->mNext;

	zassert_not_null(maddr);
	zassert_mem_equal(&maddr->mAddress, &exp_addr2, OT_IP6_ADDRESS_SIZE);
	maddr = maddr->mNext;

	zassert_is_null(maddr);
}

ZTEST(ot_rpc_ip6, test_otIp6AddressToString)
{
	char buf[OT_IP6_ADDRESS_STRING_SIZE];
	otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	otIp6AddressToString(&addr, buf, sizeof(buf));
	zassert_equal(strcmp(buf, "ff02:0:0:0:0:0:0:1"), 0);
}

ZTEST_SUITE(ot_rpc_ip6, NULL, NULL, tc_setup, NULL, NULL);
