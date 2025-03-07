/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common_fakes.h"

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/thread.h>

/* Fake functions */
FAKE_VALUE_FUNC(otError, otThreadDiscover, otInstance *, uint32_t, uint16_t, bool, bool,
		otHandleActiveScanResult, void *);
FAKE_VALUE_FUNC(otError, otThreadSetEnabled, otInstance *, bool);
FAKE_VALUE_FUNC(otDeviceRole, otThreadGetDeviceRole, otInstance *);
FAKE_VALUE_FUNC(otError, otThreadSetLinkMode, otInstance *, otLinkModeConfig);
FAKE_VALUE_FUNC(otLinkModeConfig, otThreadGetLinkMode, otInstance *);
FAKE_VALUE_FUNC(uint16_t, otThreadGetVersion);
FAKE_VALUE_FUNC(uint8_t, otThreadGetLeaderRouterId, otInstance *);
FAKE_VALUE_FUNC(uint8_t, otThreadGetLeaderWeight, otInstance *);
FAKE_VALUE_FUNC(uint32_t, otThreadGetPartitionId, otInstance *);
FAKE_VALUE_FUNC(const char *, otThreadGetNetworkName, otInstance *);
FAKE_VALUE_FUNC(const otExtendedPanId *, otThreadGetExtendedPanId, otInstance *);
FAKE_VALUE_FUNC(const otMeshLocalPrefix *, otThreadGetMeshLocalPrefix, otInstance *);

#define FOREACH_FAKE(f)                                                                            \
	f(otThreadDiscover);                                                                       \
	f(otThreadSetEnabled);                                                                     \
	f(otThreadGetDeviceRole);                                                                  \
	f(otThreadSetLinkMode);                                                                    \
	f(otThreadGetLinkMode);                                                                    \
	f(otThreadGetVersion);                                                                     \
	f(otThreadGetLeaderRouterId);                                                              \
	f(otThreadGetLeaderWeight);                                                                \
	f(otThreadGetPartitionId);                                                                 \
	f(otThreadGetNetworkName);                                                                 \
	f(nrf_rpc_cbkproxy_out_get);

extern uint64_t ot_thread_discover_cb_encoder(uint32_t callback_slot, uint32_t _rsv0,
					      uint32_t _rsv1, uint32_t _ret,
					      otActiveScanResult *result, void *context);

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
 * Test reception of otThreadDiscover command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_thread, test_otThreadDiscover)
{
	nrf_rpc_cbkproxy_out_get_fake.return_val = (void *)0xfacecafe;
	otThreadDiscover_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_DISCOVER, CBOR_UINT32(0x7fff800),
					CBOR_UINT16(0x4321), CBOR_FALSE, CBOR_TRUE, 0,
					CBOR_UINT32(0xcafeface)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadDiscover_fake.call_count, 1);
	zassert_equal(otThreadDiscover_fake.arg1_val, 0x7fff800);
	zassert_equal(otThreadDiscover_fake.arg2_val, 0x4321);
	zassert_equal(otThreadDiscover_fake.arg3_val, false);
	zassert_equal(otThreadDiscover_fake.arg4_val, true);
	zassert_equal_ptr(otThreadDiscover_fake.arg5_val, (void *)0xfacecafe);
	zassert_equal_ptr(otThreadDiscover_fake.arg6_val, (void *)0xcafeface);
}

/*
 * Test sending of otThreadDiscover result over server's callback.
 */
ZTEST(ot_rpc_thread, test_tx_discover_cb)
{
	otActiveScanResult result = {
		.mExtAddress.m8 = {INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)},
		.mNetworkName.m8 = {INT_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE), 0},
		.mExtendedPanId.m8 = {INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)},
		.mSteeringData.m8 = {INT_SEQUENCE(OT_STEERING_DATA_MAX_LENGTH)},
		.mPanId = 0x1234,
		.mJoinerUdpPort = 0x4321,
		.mChannel = 0x22,
		.mRssi = 0x33,
		.mLqi = 0x44,
		.mVersion = 0x0f,
		.mIsNative = 0,
		.mDiscover = 1,
		.mIsJoinable = 0};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_DISCOVER_CB, EXT_ADDR, NWK_NAME,
					   EXT_PAN_ID, STEERING_DATA, CBOR_UINT16(0x1234),
					   CBOR_UINT16(0x4321), CBOR_UINT8(0x22), CBOR_UINT8(0x33),
					   CBOR_UINT8(0x44), 0x0f, CBOR_FALSE, CBOR_TRUE,
					   CBOR_FALSE, CBOR_UINT32(0xdeadbeef), 0),
				   RPC_RSP());
	(void)ot_thread_discover_cb_encoder(0, 0, 0, 0, &result, (void *)0xdeadbeef);
	mock_nrf_rpc_tr_expect_done();
}

/*
 * Test sending of absence of otThreadDiscover result over server's callback.
 */
ZTEST(ot_rpc_thread, test_tx_discover_cb_null)
{
	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_DISCOVER_CB, CBOR_NULL, CBOR_UINT32(0xdeadbeef), 0),
		RPC_RSP());
	(void)ot_thread_discover_cb_encoder(0, 0, 0, 0, NULL, (void *)0xdeadbeef);
	mock_nrf_rpc_tr_expect_done();
}

/*
 * Test reception of otThreadSetEnabled(false) command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_thread, test_otThreadSetEnabled_false)
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
ZTEST(ot_rpc_thread, test_otThreadSetEnabled_true)
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
ZTEST(ot_rpc_thread, test_otThreadSetEnabled_true_failed)
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
ZTEST(ot_rpc_thread, test_otThreadGetDeviceRole_disabled)
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
ZTEST(ot_rpc_thread, test_otThreadGetDeviceRole_leader)
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
ZTEST(ot_rpc_thread, test_otThreadSetLinkMode_none)
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
ZTEST(ot_rpc_thread, test_otThreadSetLinkMode_rdn)
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
ZTEST(ot_rpc_thread, test_otThreadSetLinkMode_n)
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
ZTEST(ot_rpc_thread, test_otThreadGetLinkMode_none)
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
ZTEST(ot_rpc_thread, test_otThreadGetLinkMode_rdn)
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
 * Test reception of otThreadGetVersion() command.
 * Test serialization of the result: 1.4.
 */
ZTEST(ot_rpc_thread, test_otThreadGetVersion)
{
	otThreadGetVersion_fake.return_val = OT_THREAD_VERSION_1_4;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_THREAD_VERSION_1_4), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_VERSION));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetVersion_fake.call_count, 1);
}

/*
 * Test reception of otThreadGetExtendedPanId() command.
 */
ZTEST(ot_rpc_thread, test_otThreadGetExtendedPanId)
{
	const otExtendedPanId ext_pan_id = {{INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)}};

	otThreadGetExtendedPanId_fake.return_val = &ext_pan_id;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(EXT_PAN_ID), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_EXTENDED_PANID));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetExtendedPanId_fake.call_count, 1);
}

/*
 * Test reception of otThreadGetLeaderRouterId() command.
 * Test serialization of the result: UINT8_MAX.
 */
ZTEST(ot_rpc_thread, test_otThreadGetLeaderRouterId)
{
	otThreadGetLeaderRouterId_fake.return_val = UINT8_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT8(UINT8_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_LEADER_ROUTER_ID));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetLeaderRouterId_fake.call_count, 1);
}

/*
 * Test reception of otThreadGetLeaderWeight() command.
 * Test serialization of the result: UINT8_MAX.
 */
ZTEST(ot_rpc_thread, test_otThreadGetLeaderWeight)
{
	otThreadGetLeaderWeight_fake.return_val = UINT8_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT8(UINT8_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_LEADER_WEIGHT));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetLeaderWeight_fake.call_count, 1);
}

/*
 * Test reception of otThreadGetMeshLocalPrefix() command.
 */
ZTEST(ot_rpc_thread, test_otThreadGetMeshLocalPrefix)
{
	const otMeshLocalPrefix prefix = {{INT_SEQUENCE(OT_MESH_LOCAL_PREFIX_SIZE)}};

	otThreadGetMeshLocalPrefix_fake.return_val = &prefix;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(EXT_PAN_ID), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_MESH_LOCAL_PREFIX));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetMeshLocalPrefix_fake.call_count, 1);
}

/*
 * Test reception of otThreadGetNetworkName() command.
 */
ZTEST(ot_rpc_thread, test_otThreadGetNetworkName)
{
	const char name[] = {STR_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE), '\0'};

	otThreadGetNetworkName_fake.return_val = name;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0x50, STR_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NETWORK_NAME));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetNetworkName_fake.call_count, 1);
}

/*
 * Test reception of otThreadGetPartitionId() command.
 * Test serialization of the result: UINT32_MAX.
 */
ZTEST(ot_rpc_thread, test_otThreadGetPartitionId)
{
	otThreadGetPartitionId_fake.return_val = UINT32_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT32(UINT32_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_PARTITION_ID));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadGetPartitionId_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_thread, NULL, NULL, tc_setup, NULL, NULL);
