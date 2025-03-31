/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/netdiag.h>

/* Message address used when testing serialization of a function that takes otMessage* */
#define MSG_ADDR UINT32_MAX

#define TEST_STRING_GETTER(test_name, cmd, data, data_cbor, func_to_call)                          \
	ZTEST(ot_rpc_netdiag, test_name)                                                           \
	{                                                                                          \
		const char *result;                                                                \
		char data_exp[] = data;                                                            \
												   \
		mock_nrf_rpc_tr_expect_add(                                                        \
			RPC_CMD(cmd),                                                              \
			RPC_RSP(data_cbor));                                                       \
		result = func_to_call(NULL);                                                       \
		mock_nrf_rpc_tr_expect_done();                                                     \
												   \
		zassert_str_equal(result, data_exp);                                               \
	}

#define TEST_STRING_SETTER(test_name, cmd, data, data_cbor, func_to_call, result_exp)              \
	ZTEST(ot_rpc_netdiag, test_name)                                                           \
	{                                                                                          \
		otError error;                                                                     \
		char data_exp[] = data;                                                            \
												   \
		mock_nrf_rpc_tr_expect_add(                                                        \
			RPC_CMD(cmd, data_cbor),                                                   \
			RPC_RSP(result_exp));                                                      \
		error = func_to_call(NULL, data_exp);                                              \
		mock_nrf_rpc_tr_expect_done();                                                     \
												   \
		zassert_equal(error, result_exp);                                                  \
	}

/* Fake functions */

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

/* Test serialization of otThreadGetVendorName() */
TEST_STRING_GETTER(test_otThreadGetVendorName, OT_RPC_CMD_THREAD_GET_VENDOR_NAME,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadGetVendorName);

/* Test serialization of otThreadGetVendorName() with empty string */
TEST_STRING_GETTER(test_otThreadGetVendorName_empty, OT_RPC_CMD_THREAD_GET_VENDOR_NAME,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorName);

/* Test serialization of otThreadGetVendorModel() */
TEST_STRING_GETTER(test_otThreadGetVendorModel, OT_RPC_CMD_THREAD_GET_VENDOR_MODEL,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadGetVendorModel);

/* Test serialization of otThreadGetVendorModel() with empty string */
TEST_STRING_GETTER(test_otThreadGetVendorModel_empty, OT_RPC_CMD_THREAD_GET_VENDOR_MODEL,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorModel);

/* Test serialization of otThreadGetVendorSwVersion() */
TEST_STRING_GETTER(test_otThreadGetVendorSwVersion, OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION,
		   NETDIAG_VENDOR_DATA_16, CBOR_NETDIAG_VENDOR_DATA_16, otThreadGetVendorSwVersion);

/* Test serialization of otThreadGetVendorName() with empty string */
TEST_STRING_GETTER(test_otThreadGetVendorSwVersion_empty, OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorSwVersion);

/* Test serialization of otThreadSetVendorName() */
TEST_STRING_SETTER(test_otThreadSetVendorName, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32,
		   otThreadSetVendorName, OT_ERROR_NONE);

/* Test serialization of otThreadSetVendorName() with empty string */
TEST_STRING_SETTER(test_otThreadSetVendorName_empty, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorName, OT_ERROR_NONE);

/* Test serialization of otThreadSetVendorModel() */
TEST_STRING_SETTER(test_otThreadSetVendorModel, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32,
		   otThreadSetVendorModel, OT_ERROR_NONE);

/* Test serialization of otThreadSetVendorModel() */
TEST_STRING_SETTER(test_otThreadSetVendorModel_empty, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorModel, OT_ERROR_NONE);

/* Test serialization of otThreadSetVendorSwVersion() */
TEST_STRING_SETTER(test_otThreadSetVendorSwVersion, OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
		   NETDIAG_VENDOR_DATA_16, CBOR_NETDIAG_VENDOR_DATA_16,
		   otThreadSetVendorSwVersion, OT_ERROR_NONE);

/* Test serialization of otThreadSetVendorSwVersion() with empty string */
TEST_STRING_SETTER(test_otThreadSetVendorSwVersion_empty, OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorSwVersion, OT_ERROR_NONE);

FAKE_VOID_FUNC(handle_receive_diagnostic_get, otError, otMessage *, const otMessageInfo *, void *);

static void fake_handle_receive_diagnostic_get(otError error, otMessage *message,
					       const otMessageInfo *message_info, void *context)
{
	otIp6Address expected_soc_addr = {.mFields.m8 = {ADDR_1}};
	otIp6Address expected_peer_addr = {.mFields.m8 = {ADDR_2}};

	zassert_equal(error, OT_ERROR_NONE);
	zassert_equal((ot_rpc_res_tab_key)message, RESOURCE_TABLE_KEY);
	zassert_mem_equal(message_info->mSockAddr.mFields.m8, expected_soc_addr.mFields.m8,
			  OT_IP6_ADDRESS_SIZE);
	zassert_mem_equal(message_info->mPeerAddr.mFields.m8, expected_peer_addr.mFields.m8,
			  OT_IP6_ADDRESS_SIZE);
	zassert_equal(message_info->mSockPort, PORT_1);
	zassert_equal(message_info->mPeerPort, PORT_2);
	zassert_equal(message_info->mHopLimit, HOP_LIMIT);
	zassert_equal(message_info->mEcn, 3);
	zassert_true(message_info->mIsHostInterface);
	zassert_true(message_info->mAllowZeroHopLimit);
	zassert_true(message_info->mMulticastLoop);
}

/*
 * Test serialization of otThreadSendDiagnosticGet().
 * Test reception of otReceiveDiagnosticGetCallback.
 */
ZTEST(ot_rpc_netdiag, test_otThreadSendDiagnosticGet)
{
	otError error;
	uint8_t tlvs[] = {NETDIAG_TLVS};
	uint8_t count = NETDIAG_TLVS_LEN;
	const otIp6Address address = {.mFields.m8 = {ADDR_1}};

	RESET_FAKE(handle_receive_diagnostic_get);

	handle_receive_diagnostic_get_fake.custom_fake = fake_handle_receive_diagnostic_get;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET, CBOR_ADDR1, CBOR_NETDIAG_TLVS),
		RPC_RSP(OT_ERROR_NONE));
	error = otThreadSendDiagnosticGet(NULL, &address, tlvs, count,
					  handle_receive_diagnostic_get, NULL);
	zassert_equal(error, OT_ERROR_NONE);
	mock_nrf_rpc_tr_expect_done();

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET_CB,
					OT_ERROR_NONE, RESOURCE_TABLE_KEY, CBOR_MSG_INFO));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(handle_receive_diagnostic_get_fake.call_count, 1);
}

/* Test serialization of otThreadSendDiagnosticReset() */
ZTEST(ot_rpc_netdiag, test_otThreadSendDiagnosticReset)
{
	otError error;
	uint8_t tlvs[] = {NETDIAG_TLVS};
	uint8_t count = NETDIAG_TLVS_LEN;
	const otIp6Address address = {.mFields.m8 = {ADDR_1}};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_RESET, CBOR_ADDR1, CBOR_NETDIAG_TLVS),
		RPC_RSP(OT_ERROR_NONE));
	error = otThreadSendDiagnosticReset(NULL, &address, tlvs, count);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

static void get_next_diag_tlv(otNetworkDiagTlv *diag_tlv)
{
	const otMessage *message = (otMessage *)RESOURCE_TABLE_KEY;
	otNetworkDiagIterator iterator = OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT;
	otError error;

	memset(diag_tlv, 0, sizeof(otNetworkDiagTlv));
	error = otThreadGetNextDiagnosticTlv(message, &iterator, diag_tlv);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
	zassert_equal(iterator, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT + 1);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_EXT_ADDR.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_ext_addr)
{
	const otExtAddress expected_address = {.m8 = {NETDIAG_TLV_EXT_ADDR}};
	otNetworkDiagTlv diag_tlv;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_EXT_ADDR));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS);
	zassert_mem_equal(diag_tlv.mData.mExtAddress.m8, expected_address.m8, OT_EXT_ADDRESS_SIZE);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_EUI64.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_eui64)
{
	const otExtAddress expected_address = {.m8 = {NETDIAG_TLV_EUI64}};
	otNetworkDiagTlv diag_tlv;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_EUI64));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_EUI64);
	zassert_mem_equal(diag_tlv.mData.mEui64.m8, expected_address.m8, OT_EXT_ADDRESS_SIZE);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_MODE.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_mode)
{
	otNetworkDiagTlv diag_tlv;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_MODE));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_MODE);
	zassert_true(diag_tlv.mData.mMode.mRxOnWhenIdle);
	zassert_true(diag_tlv.mData.mMode.mDeviceType);
	zassert_true(diag_tlv.mData.mMode.mNetworkData);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_CONNECTIVITY.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_connectivity)
{
	otNetworkDiagTlv diag_tlv;
	const uint8_t expected_data[] = {NETDIAG_TLV_CONNECTIVITY};
	uint8_t data_field = 0;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_CONNECTIVITY));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY);
	zassert_equal(diag_tlv.mData.mConnectivity.mParentPriority, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mConnectivity.mLinkQuality3, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mConnectivity.mLinkQuality2, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mConnectivity.mLinkQuality1, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mConnectivity.mLeaderCost, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mConnectivity.mIdSequence, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mConnectivity.mActiveRouters, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mConnectivity.mSedBufferSize, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mConnectivity.mSedDatagramCount, expected_data[data_field++]);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_ROUTE.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_route)
{
	otNetworkDiagTlv diag_tlv;
	const uint8_t expected_data[] = {NETDIAG_TLV_ROUTE};
	uint8_t data_field = 0;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_ROUTE));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_ROUTE);
	zassert_equal(diag_tlv.mData.mRoute.mIdSequence, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mRoute.mRouteCount, expected_data[data_field++]);
	for (int i = 0; i < diag_tlv.mData.mRoute.mRouteCount; i++) {
		zassert_equal(diag_tlv.mData.mRoute.mRouteData[i].mRouterId,
			      expected_data[data_field++]);
		zassert_equal(diag_tlv.mData.mRoute.mRouteData[i].mLinkQualityIn,
			      expected_data[data_field++]);
		zassert_equal(diag_tlv.mData.mRoute.mRouteData[i].mLinkQualityOut,
			      expected_data[data_field++]);
		zassert_equal(diag_tlv.mData.mRoute.mRouteData[i].mRouteCost,
			      expected_data[data_field++]);
	}
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_LEADER_DATA.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_leader_data)
{
	otNetworkDiagTlv diag_tlv;
	const uint8_t expected_data[] = {NETDIAG_TLV_LEADER_DATA};
	uint8_t data_field = 0;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_LEADER_DATA));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA);
	zassert_equal(diag_tlv.mData.mLeaderData.mPartitionId, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mLeaderData.mWeighting, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mLeaderData.mDataVersion, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mLeaderData.mStableDataVersion, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mLeaderData.mLeaderRouterId, expected_data[data_field++]);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_MAC_COUNTERS.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_mac_counters)
{
	otNetworkDiagTlv diag_tlv;
	const uint8_t expected_data[] = {NETDIAG_TLV_MAC_COUNTERS};
	uint8_t data_field = 0;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_MAC_COUNTERS));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS);
	zassert_equal(diag_tlv.mData.mMacCounters.mIfInUnknownProtos, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMacCounters.mIfInErrors, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMacCounters.mIfOutErrors, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMacCounters.mIfInUcastPkts, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMacCounters.mIfInBroadcastPkts, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMacCounters.mIfInDiscards, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMacCounters.mIfOutUcastPkts, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMacCounters.mIfOutBroadcastPkts, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMacCounters.mIfOutDiscards, expected_data[data_field++]);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_MLE_COUNTERS.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_mle_counters)
{
	otNetworkDiagTlv diag_tlv;
	const uint64_t expected_data[] = {NETDIAG_TLV_MLE_COUNTERS};
	uint8_t data_field = 0;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_MLE_COUNTERS));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS);
	zassert_equal(diag_tlv.mData.mMleCounters.mDisabledRole, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mDetachedRole, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mChildRole, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mRouterRole, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mLeaderRole, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mAttachAttempts, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mPartitionIdChanges, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mBetterPartitionAttachAttempts,
		      expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mParentChanges, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mTrackedTime, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mDisabledTime, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mDetachedTime, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mChildTime, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mRouterTime, expected_data[data_field++]);
	zassert_equal(diag_tlv.mData.mMleCounters.mLeaderTime, expected_data[data_field++]);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_BATTERY_LEVEL.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_battery_level)
{
	otNetworkDiagTlv diag_tlv;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_BATTERY_LEVEL));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL);
	zassert_equal(diag_tlv.mData.mBatteryLevel, NETDIAG_TLV_BATTERY_LEVEL);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_TIMEOUT.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_timeout)
{
	otNetworkDiagTlv diag_tlv;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_TIMEOUT));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT);
	zassert_equal(diag_tlv.mData.mTimeout, NETDIAG_TLV_TIMEOUT);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_MAX_CHILD_TIMEOUT.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_max_child_timeout)
{
	otNetworkDiagTlv diag_tlv;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_MAX_CHILD_TIMEOUT));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT);
	zassert_equal(diag_tlv.mData.mMaxChildTimeout, NETDIAG_TLV_MAX_CHILD_TIMEOUT);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_SHORT_ADDRESS.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_short_address)
{
	otNetworkDiagTlv diag_tlv;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_SHORT_ADDRESS));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS);
	zassert_equal(diag_tlv.mData.mAddr16, NETDIAG_TLV_SHORT_ADDRESS);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_SUPPLY_VOLTAGE.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_supply_voltage)
{
	otNetworkDiagTlv diag_tlv;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_SUPPLY_VOLTAGE));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE);
	zassert_equal(diag_tlv.mData.mSupplyVoltage, NETDIAG_TLV_SUPPLY_VOLTAGE);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_VERSION.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_version)
{
	otNetworkDiagTlv diag_tlv;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_VERSION));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_VERSION);
	zassert_equal(diag_tlv.mData.mVersion, NETDIAG_TLV_VERSION);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_VENDOR_NAME.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_vendor_name)
{
	otNetworkDiagTlv diag_tlv;
	const char expected_data[] = {NETDIAG_TLV_VENDOR_NAME, '\0'};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_VENDOR_NAME));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME);
	zassert_str_equal(diag_tlv.mData.mVendorName, expected_data);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_VENDOR_MODEL.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_vendor_model)
{
	otNetworkDiagTlv diag_tlv;
	const char expected_data[] = {NETDIAG_TLV_VENDOR_MODEL, '\0'};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_VENDOR_MODEL));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL);
	zassert_str_equal(diag_tlv.mData.mVendorModel, expected_data);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_VENDOR_SW_VERSION.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_vendor_sw_version)
{
	otNetworkDiagTlv diag_tlv;
	const char expected_data[] = {NETDIAG_TLV_VENDOR_SW_VERSION, '\0'};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_VENDOR_SW_VERSION));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION);
	zassert_str_equal(diag_tlv.mData.mVendorSwVersion, expected_data);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_VENDOR_APP_URL.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_vendor_app_url)
{
	otNetworkDiagTlv diag_tlv;
	const char expected_data[] = {NETDIAG_TLV_VENDOR_APP_URL, '\0'};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_VENDOR_APP_URL));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_APP_URL);
	zassert_str_equal(diag_tlv.mData.mVendorAppUrl, expected_data);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_THREAD_STACK_VERSION.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_thread_stack_version)
{
	otNetworkDiagTlv diag_tlv;
	const char expected_data[] = {NETDIAG_TLV_THREAD_STACK_VERSION, '\0'};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
			CBOR_NETDIAG_TLV_THREAD_STACK_VERSION));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION);
	zassert_str_equal(diag_tlv.mData.mThreadStackVersion, expected_data);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_NETWORK_DATA.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_network_data)
{
	otNetworkDiagTlv diag_tlv;
	const uint8_t expected_data[] = {NETDIAG_TLV_NETWORK_DATA};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_NETWORK_DATA));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA);
	zassert_equal(diag_tlv.mData.mNetworkData.mCount, OT_NETWORK_BASE_TLV_MAX_LENGTH);
	zassert_mem_equal(diag_tlv.mData.mNetworkData.m8, expected_data,
			  OT_NETWORK_BASE_TLV_MAX_LENGTH);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_CHANNEL_PAGES.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_channel_pages)
{
	otNetworkDiagTlv diag_tlv;
	const uint8_t expected_data[] = {NETDIAG_TLV_CHANNEL_PAGES};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_CHANNEL_PAGES));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES);
	zassert_equal(diag_tlv.mData.mChannelPages.mCount, OT_NETWORK_BASE_TLV_MAX_LENGTH);
	zassert_mem_equal(diag_tlv.mData.mChannelPages.m8, expected_data,
			  OT_NETWORK_BASE_TLV_MAX_LENGTH);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_IP6_ADDR_LIST.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_ip_addr_list)
{
	otNetworkDiagTlv diag_tlv;
	const otIp6Address expected_address_1 = {.mFields.m8 = {ADDR_1}};
	const otIp6Address expected_address_2 = {.mFields.m8 = {ADDR_2}};

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_IP6_ADDR_LIST));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST);
	zassert_equal(diag_tlv.mData.mIp6AddrList.mCount, 2);
	zassert_mem_equal(diag_tlv.mData.mIp6AddrList.mList[0].mFields.m8,
			  expected_address_1.mFields.m8, OT_IP6_ADDRESS_SIZE);
	zassert_mem_equal(diag_tlv.mData.mIp6AddrList.mList[1].mFields.m8,
			  expected_address_2.mFields.m8, OT_IP6_ADDRESS_SIZE);
}

/*
 * Test serialization of otThreadGetNextDiagnosticTlv().
 * Test decoding of CBOR_NETDIAG_TLV_CHILD_TABLE.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_child_table)
{
	otNetworkDiagTlv diag_tlv;
	const uint8_t expected_data[] = {NETDIAG_TLV_CHILD_TABLE};
	uint8_t data_field = 0;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, RESOURCE_TABLE_KEY,
			OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT),
		RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR, CBOR_NETDIAG_TLV_CHILD_TABLE));

	get_next_diag_tlv(&diag_tlv);

	zassert_equal(diag_tlv.mType, OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE);
	zassert_equal(diag_tlv.mData.mChildTable.mCount, NETDIAG_CHILD_TABLE_LEN);
	for (int i = 0; i < diag_tlv.mData.mChildTable.mCount; i++) {
		zassert_equal(diag_tlv.mData.mChildTable.mTable[i].mTimeout,
			      expected_data[data_field++]);
		zassert_equal(diag_tlv.mData.mChildTable.mTable[i].mLinkQuality,
			      expected_data[data_field++]);
		zassert_equal(diag_tlv.mData.mChildTable.mTable[i].mChildId,
			      expected_data[data_field++]);
		zassert_true(diag_tlv.mData.mChildTable.mTable[i].mMode.mRxOnWhenIdle);
		zassert_true(diag_tlv.mData.mChildTable.mTable[i].mMode.mDeviceType);
		zassert_true(diag_tlv.mData.mChildTable.mTable[i].mMode.mNetworkData);

		/* Skip the 0x7 data field responsible for setting mMode flags. */
		data_field++;
	}
}

ZTEST_SUITE(ot_rpc_netdiag, NULL, NULL, tc_setup, NULL, NULL);
