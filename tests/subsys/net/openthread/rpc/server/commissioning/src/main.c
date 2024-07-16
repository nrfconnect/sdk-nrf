/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <zephyr/net/openthread.h>
#include <openthread/link.h>
#include <openthread/cli.h>
#include <openthread/ip6.h>
#include <openthread/thread.h>

#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

/* Fake functions */

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(otError, otThreadDiscover, otInstance *, uint32_t, uint16_t, bool, bool,
		otHandleActiveScanResult, void *);
FAKE_VALUE_FUNC(bool, otDatasetIsCommissioned, otInstance *);
FAKE_VALUE_FUNC(otError, otDatasetSetActiveTlvs, otInstance *, const otOperationalDatasetTlvs *);
FAKE_VALUE_FUNC(otError, otDatasetGetActiveTlvs, otInstance *, otOperationalDatasetTlvs *);
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

#define FOREACH_FAKE(f)                                                                            \
	f(otThreadDiscover);                                                                       \
	f(otDatasetIsCommissioned);                                                                \
	f(otDatasetSetActiveTlvs);                                                                 \
	f(otDatasetGetActiveTlvs)

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
ZTEST(ot_rpc_commissioning_server, test_otThreadDiscover)
{
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
	zassert_equal_ptr(otThreadDiscover_fake.arg5_val, NULL);
	zassert_equal_ptr(otThreadDiscover_fake.arg6_val, (void *)0xcafeface);
}

/*
 * Test reception of otDatasetIsCommissioned command.
 * Test serialization of the result: true or false.
 */
ZTEST(ot_rpc_commissioning_server, test_otDatasetIsCommissioned)
{
	otDatasetIsCommissioned_fake.return_val = true;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_TRUE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATASET_IS_COMMISSIONED));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetIsCommissioned_fake.call_count, 1);

	otDatasetIsCommissioned_fake.return_val = false;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_FALSE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATASET_IS_COMMISSIONED));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetIsCommissioned_fake.call_count, 2);
}

/*
 * Test reception of otDatasetSetActiveTlvs command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_commissioning_server, test_otDatasetSetActiveTlvs)
{
	const otOperationalDatasetTlvs expected_dataset = {
		{INT_SEQUENCE(OT_OPERATIONAL_DATASET_MAX_LENGTH)},
		OT_OPERATIONAL_DATASET_MAX_LENGTH};

	otDatasetSetActiveTlvs_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATA_SET_ACTIVE_TLVS, TLVS));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetSetActiveTlvs_fake.call_count, 1);
	zassert_mem_equal(otDatasetSetActiveTlvs_fake.arg1_val, &expected_dataset,
			  sizeof(otOperationalDatasetTlvs));
}

/*
 * Test reception of otDatasetSetActiveTlvs command with NULL pointer.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_commissioning_server, test_otDatasetSetActiveTlvs_negative)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_INVALID_ARGS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATA_SET_ACTIVE_TLVS, CBOR_NULL));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetSetActiveTlvs_fake.call_count, 0);
}

static otError custom_fake(otInstance *instance, otOperationalDatasetTlvs *dataset)
{
	otOperationalDatasetTlvs expected_dataset = {
		{INT_SEQUENCE(OT_OPERATIONAL_DATASET_MAX_LENGTH)},
		OT_OPERATIONAL_DATASET_MAX_LENGTH};

	memcpy(dataset, &expected_dataset, sizeof(otOperationalDatasetTlvs));

	return OT_ERROR_NONE;
}

/*
 * Test reception of otDatasetGetActiveTlvs command.
 * Test serialization of the result: TLVS data.
 */
ZTEST(ot_rpc_commissioning_server, test_otDatasetGetActiveTlvs)
{
	otDatasetGetActiveTlvs_fake.custom_fake = custom_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(TLVS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATA_GET_ACTIVE_TLVS));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetGetActiveTlvs_fake.call_count, 1);
}

/*
 * Test reception of otDatasetGetActiveTlvs command.
 * Test serialization of the result: NULL as tlvs data has not been found.
 */
ZTEST(ot_rpc_commissioning_server, test_otDatasetGetActiveTlvs_null)
{
	otDatasetGetActiveTlvs_fake.return_val = OT_ERROR_NOT_FOUND;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_NULL), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATA_GET_ACTIVE_TLVS));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetGetActiveTlvs_fake.call_count, 1);
}

/*
 * Test sending of otThreadDiscover result over server's callback.
 */
ZTEST(ot_rpc_commissioning_server, test_tx_discover_cb)
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
ZTEST(ot_rpc_commissioning_server, test_tx_discover_cb_null)
{
	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_DISCOVER_CB, CBOR_NULL, CBOR_UINT32(0xdeadbeef), 0),
		RPC_RSP());
	(void)ot_thread_discover_cb_encoder(0, 0, 0, 0, NULL, (void *)0xdeadbeef);
	mock_nrf_rpc_tr_expect_done();
}

ZTEST_SUITE(ot_rpc_commissioning_server, NULL, NULL, tc_setup, NULL, NULL);
