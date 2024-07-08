/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/ip6.h>
#include <openthread/thread.h>

#include <ot_rpc_ids.h>

/* CBOR constants */

#define CBOR_FALSE 0xf4
#define CBOR_TRUE  0xf5

/* Macros for constructing nRF RPC packets for the OpenThread command group. */

#define RPC_PKT(bytes...)                                                                          \
	(mock_nrf_rpc_pkt_t)                                                                       \
	{                                                                                          \
		.data = (uint8_t[]){bytes}, .len = sizeof((uint8_t[]){bytes}),                     \
	}

#define RPC_INIT_REQ	  RPC_PKT(0x04, 0x00, 0xff, 0x00, 0xff, 0x00, 'o', 't')
#define RPC_INIT_RSP	  RPC_PKT(0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 'o', 't')
#define RPC_CMD(cmd, ...) RPC_PKT(0x80, cmd, 0xff, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, 0xf6)
#define RPC_RSP(...)	  RPC_PKT(0x01, 0xff, 0x00, 0x00, 0x00 __VA_OPT__(,) __VA_ARGS__, 0xf6)

/* Other test data */

#define MADDR_FF02_1 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01

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
ZTEST(ot_rpc_client, test_otIp6SetEnabled_false)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_SET_ENABLED, CBOR_FALSE), RPC_RSP(0x00));
	error = otIp6SetEnabled(NULL, false);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otIp6SetEnabled(true) */
ZTEST(ot_rpc_client, test_otIp6SetEnabled_true)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_SET_ENABLED, CBOR_TRUE), RPC_RSP(0x00));
	error = otIp6SetEnabled(NULL, true);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of failed otIp6SetEnabled(true) */
ZTEST(ot_rpc_client, test_otIp6SetEnabled_true_failed)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_SET_ENABLED, CBOR_TRUE), RPC_RSP(0x01));
	error = otIp6SetEnabled(NULL, true);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_FAILED);
}

/* Test serialization of otIp6IsEnabled returning false */
ZTEST(ot_rpc_client, test_otIp6IsEnabled_false)
{
	bool enabled;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_IS_ENABLED), RPC_RSP(CBOR_FALSE));
	enabled = otIp6IsEnabled(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_false(enabled);
}

/* Test serialization of otIp6IsEnabled returning true */
ZTEST(ot_rpc_client, test_otIp6IsEnabled_true)
{
	bool enabled;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_IS_ENABLED), RPC_RSP(CBOR_TRUE));
	enabled = otIp6IsEnabled(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_true(enabled);
}

/* Test serialization of otIp6SubscribeMulticastAddress(ff02::1) */
ZTEST(ot_rpc_client, test_otIp6SubscribeMulticastAddress)
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
ZTEST(ot_rpc_client, test_otIp6SubscribeMulticastAddress_failed)
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
ZTEST(ot_rpc_client, test_otIp6UnsubscribeMulticastAddress)
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
ZTEST(ot_rpc_client, test_otIp6UnsubscribeMulticastAddress_failed)
{
	otError error;
	otIp6Address addr = {.mFields.m8 = {MADDR_FF02_1}};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR, 0x50, MADDR_FF02_1),
				   RPC_RSP(0x01));
	error = otIp6UnsubscribeMulticastAddress(NULL, &addr);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_FAILED);
}

/* Test serialization of otThreadSetEnabled(false) */
ZTEST(ot_rpc_client, test_otThreadSetEnabled_false)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_ENABLED, CBOR_FALSE),
				   RPC_RSP(0x00));
	error = otThreadSetEnabled(NULL, false);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otThreadSetEnabled(true) */
ZTEST(ot_rpc_client, test_otThreadSetEnabled_true)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_ENABLED, CBOR_TRUE),
				   RPC_RSP(0x00));
	error = otThreadSetEnabled(NULL, true);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of failed otThreadSetEnabled(true) */
ZTEST(ot_rpc_client, test_otThreadSetEnabled_true_failed)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_ENABLED, CBOR_TRUE),
				   RPC_RSP(0x01));
	error = otThreadSetEnabled(NULL, true);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_FAILED);
}

/* Test serialization of otThreadGetDeviceRole() returning 'disabled' */
ZTEST(ot_rpc_client, test_otThreadGetDeviceRole_disabled)
{
	otDeviceRole role;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_DEVICE_ROLE), RPC_RSP(0x00));
	role = otThreadGetDeviceRole(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(role, OT_DEVICE_ROLE_DISABLED);
}

/* Test serialization of otThreadGetDeviceRole() returning 'leader' */
ZTEST(ot_rpc_client, test_otThreadGetDeviceRole_leader)
{
	otDeviceRole role;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_DEVICE_ROLE), RPC_RSP(0x04));
	role = otThreadGetDeviceRole(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(role, OT_DEVICE_ROLE_LEADER);
}

/* Test serialization of otThreadSetLinkMode(-) */
ZTEST(ot_rpc_client, test_otThreadSetLinkMode_none)
{
	otError error;
	otLinkModeConfig mode;

	memset(&mode, 0, sizeof(mode));

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_LINK_MODE, 0x00), RPC_RSP(0x00));
	error = otThreadSetLinkMode(NULL, mode);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otThreadSetLinkMode(rdn) */
ZTEST(ot_rpc_client, test_otThreadSetLinkMode_rdn)
{
	otError error;
	otLinkModeConfig mode;

	memset(&mode, 0, sizeof(mode));
	mode.mRxOnWhenIdle = true;
	mode.mDeviceType = true;
	mode.mNetworkData = true;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_LINK_MODE, 0x07), RPC_RSP(0x00));
	error = otThreadSetLinkMode(NULL, mode);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of failed otThreadSetLinkMode(n) */
ZTEST(ot_rpc_client, test_otThreadSetLinkMode_n)
{
	otError error;
	otLinkModeConfig mode;

	memset(&mode, 0, sizeof(mode));
	mode.mNetworkData = true;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_SET_LINK_MODE, 0x04), RPC_RSP(0x07));
	error = otThreadSetLinkMode(NULL, mode);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test serialization of otThreadGetLinkMode() returning '-' */
ZTEST(ot_rpc_client, test_otThreadGetLinkMode_none)
{
	otLinkModeConfig mode;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_LINK_MODE), RPC_RSP(0x00));
	mode = otThreadGetLinkMode(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_false(mode.mRxOnWhenIdle);
	zassert_false(mode.mDeviceType);
	zassert_false(mode.mNetworkData);
}

/* Test serialization of otThreadGetLinkMode() returning 'rdn' */
ZTEST(ot_rpc_client, test_otThreadGetLinkMode_rdn)
{
	otLinkModeConfig mode;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_GET_LINK_MODE), RPC_RSP(0x07));
	mode = otThreadGetLinkMode(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_true(mode.mRxOnWhenIdle);
	zassert_true(mode.mDeviceType);
	zassert_true(mode.mNetworkData);
}

/* Test serialization of failed otLinkSetPollPeriod(rdn) */
ZTEST(ot_rpc_client, test_otLinkSetPollPeriod_0)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_SET_POLL_PERIOD, 0x00), RPC_RSP(0x07));
	error = otLinkSetPollPeriod(NULL, 0);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test serialization of max allowed otLinkSetPollPeriod(0x3ffffff) */
ZTEST(ot_rpc_client, test_otLinkSetPollPeriod_max)
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
ZTEST(ot_rpc_client, test_otLinkGetPollPeriod_min)
{
	uint32_t period;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_GET_POLL_PERIOD), RPC_RSP(0x0a));
	period = otLinkGetPollPeriod(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(period, 10);
}

/* Test serialization of otLinkGetPollPeriod() returning max allowed 0x3ffffff */
ZTEST(ot_rpc_client, test_otLinkGetPollPeriod_max)
{
	uint32_t period;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_LINK_GET_POLL_PERIOD),
				   RPC_RSP(0x1a, 0x03, 0xff, 0xff, 0xff));
	period = otLinkGetPollPeriod(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(period, 0x3ffffff);
}

ZTEST_SUITE(ot_rpc_client, NULL, NULL, tc_setup, NULL, NULL);
