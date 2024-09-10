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

ZTEST_SUITE(ot_rpc_ip6, NULL, NULL, tc_setup, NULL, NULL);
