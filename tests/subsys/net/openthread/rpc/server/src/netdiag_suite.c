/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_resource.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <openthread/netdiag.h>

/* Message address used when testing serialization of a function that takes otMessage* */
#define MSG_ADDR UINT32_MAX

/* Fake functions */

FAKE_VALUE_FUNC(otError, otThreadSetVendorName, otInstance *, const char *);
FAKE_VALUE_FUNC(otError, otThreadSetVendorModel, otInstance *, const char *);
FAKE_VALUE_FUNC(otError, otThreadSetVendorSwVersion, otInstance *, const char *);
FAKE_VALUE_FUNC(const char *, otThreadGetVendorName, otInstance *);
FAKE_VALUE_FUNC(const char *, otThreadGetVendorModel, otInstance *);
FAKE_VALUE_FUNC(const char *, otThreadGetVendorSwVersion, otInstance *);
FAKE_VALUE_FUNC(otError, otThreadGetNextDiagnosticTlv, const otMessage *, otNetworkDiagIterator *,
		otNetworkDiagTlv *);
FAKE_VALUE_FUNC(otError, otThreadSendDiagnosticGet, otInstance *, const otIp6Address *,
		const uint8_t *, uint8_t, otReceiveDiagnosticGetCallback, void *);
FAKE_VALUE_FUNC(otError, otThreadSendDiagnosticReset, otInstance *, const otIp6Address *,
		const uint8_t *, uint8_t);

#define FOREACH_FAKE(f)                                                                            \
	f(otThreadSetVendorName);                                                                  \
	f(otThreadSetVendorModel);                                                                 \
	f(otThreadSetVendorSwVersion);                                                             \
	f(otThreadGetVendorName);                                                                  \
	f(otThreadGetVendorModel);                                                                 \
	f(otThreadGetVendorSwVersion);                                                             \
	f(otThreadGetNextDiagnosticTlv);                                                           \
	f(otThreadSendDiagnosticGet);                                                              \
	f(otThreadSendDiagnosticReset);

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

#define TEST_STRING_SETTER(test_name, cmd, data, data_cbor, func_to_call, max_length, result_exp)  \
	static otError fake_##test_name(otInstance *instance, const char *arg)                     \
	{                                                                                          \
		char data_exp[] = data;                                                            \
                                                                                                   \
		ARG_UNUSED(instance);                                                              \
                                                                                                   \
		if (strlen(arg) > max_length) {                                                    \
			return OT_ERROR_INVALID_ARGS;                                              \
		}                                                                                  \
                                                                                                   \
		zassert_str_equal(func_to_call##_fake.arg1_val, data_exp);                         \
                                                                                                   \
		return OT_ERROR_NONE;                                                              \
	}                                                                                          \
                                                                                                   \
	ZTEST(ot_rpc_netdiag, test_name)                                                           \
	{                                                                                          \
		func_to_call##_fake.custom_fake = fake_##test_name;                                \
                                                                                                   \
		mock_nrf_rpc_tr_expect_add(RPC_RSP(result_exp), NO_RSP);                           \
		mock_nrf_rpc_tr_receive(RPC_CMD(cmd, data_cbor));                                  \
		mock_nrf_rpc_tr_expect_done();                                                     \
                                                                                                   \
		zassert_equal(func_to_call##_fake.call_count, 1);                                  \
	}

#define TEST_STRING_GETTER(test_name, cmd, data, data_cbor, func_to_call)                          \
	ZTEST(ot_rpc_netdiag, test_name)                                                           \
	{                                                                                          \
		char data_exp[] = data;                                                            \
		func_to_call##_fake.return_val = data_exp;                                         \
		mock_nrf_rpc_tr_expect_add(RPC_RSP(data_cbor), NO_RSP);                            \
		mock_nrf_rpc_tr_receive(RPC_CMD(cmd));                                             \
		mock_nrf_rpc_tr_expect_done();                                                     \
												   \
		zassert_equal(func_to_call##_fake.call_count, 1);                                  \
	}


/*
 * Test reception of otThreadGetVendorName() command.
 * Test serialization of the result: NETDIAG_VENDOR_DATA_32.
 */
TEST_STRING_GETTER(test_otThreadGetVendorName, OT_RPC_CMD_THREAD_GET_VENDOR_NAME,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadGetVendorName);

/*
 * Test reception of otThreadGetVendorName() command with empty string.
 * Test serialization of the result: "".
 */
TEST_STRING_GETTER(test_otThreadGetVendorName_empty, OT_RPC_CMD_THREAD_GET_VENDOR_NAME,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorName);

/*
 * Test reception of otThreadGetVendorModel() command.
 * Test serialization of the result: NETDIAG_VENDOR_DATA_32.
 */
TEST_STRING_GETTER(test_otThreadGetVendorModel, OT_RPC_CMD_THREAD_GET_VENDOR_MODEL,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadGetVendorModel);

/*
 * Test reception of otThreadGetVendorModel() command with empty string.
 * Test serialization of the result: "".
 */
TEST_STRING_GETTER(test_otThreadGetVendorModel_empty, OT_RPC_CMD_THREAD_GET_VENDOR_MODEL,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorModel);

/*
 * Test reception of otThreadGetVendorSwVersion() command.
 * Test serialization of the result: NETDIAG_VENDOR_DATA_16.
 */
TEST_STRING_GETTER(test_otThreadGetVendorSwVersion, OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION,
		   NETDIAG_VENDOR_DATA_16, CBOR_NETDIAG_VENDOR_DATA_16, otThreadGetVendorSwVersion);

/*
 * Test reception of otThreadGetVendorSwVersion() command with empty string.
 * Test serialization of the result: "".
 */
TEST_STRING_GETTER(test_otThreadGetVendorSwVersion_empty, OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION,
		   "", CBOR_EMPTY_STRING, otThreadGetVendorSwVersion);

/*
 * Test reception of otThreadSetVendorName() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorName, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadSetVendorName,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorName() command with empty string.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorName_empty, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorName,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorName() command with too long string.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
TEST_STRING_SETTER(test_otThreadSetVendorName_too_long, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
		   NETDIAG_VENDOR_DATA_33, CBOR_NETDIAG_VENDOR_DATA_33, otThreadSetVendorName,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH, OT_ERROR_INVALID_ARGS);

/*
 * Test reception of otThreadSetVendorModel() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorModel, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
		   NETDIAG_VENDOR_DATA_32, CBOR_NETDIAG_VENDOR_DATA_32, otThreadSetVendorModel,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorModel() command with empty string.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorModel_empty, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorModel,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorModel() command with too long string.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
TEST_STRING_SETTER(test_otThreadSetVendorModel_too_long, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
		   NETDIAG_VENDOR_DATA_33, CBOR_NETDIAG_VENDOR_DATA_33, otThreadSetVendorModel,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH, OT_ERROR_INVALID_ARGS);

/*
 * Test reception of otThreadSetVendorSwVersion() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorSwVersion, OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
		   NETDIAG_VENDOR_DATA_16, CBOR_NETDIAG_VENDOR_DATA_16, otThreadSetVendorSwVersion,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorSwVersion() command with empty string.
 * Test serialization of the result: OT_ERROR_NONE.
 */
TEST_STRING_SETTER(test_otThreadSetVendorSwVersion_empty, OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
		   "", CBOR_EMPTY_STRING, otThreadSetVendorSwVersion,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH, OT_ERROR_NONE);

/*
 * Test reception of otThreadSetVendorSwVersion() command with too long string.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
TEST_STRING_SETTER(test_otThreadSetVendorSwVersion_too_long,
		   OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
		   NETDIAG_VENDOR_DATA_17, CBOR_NETDIAG_VENDOR_DATA_17, otThreadSetVendorSwVersion,
		   OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH, OT_ERROR_INVALID_ARGS);


static otError ot_thread_send_diagnostic_reset(otInstance *instance, const otIp6Address *address,
					       const uint8_t *tlvs, uint8_t count)
{
	const uint8_t expected_tlvs[] = {NETDIAG_TLVS};
	const otIp6Address expected_address = {.mFields.m8 = {ADDR_1}};

	zassert_mem_equal(address->mFields.m8, expected_address.mFields.m8, OT_IP6_ADDRESS_SIZE);
	zassert_mem_equal(tlvs, expected_tlvs, NETDIAG_TLVS_LEN);
	zassert_equal(count, NETDIAG_TLVS_LEN);

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadSendDiagnosticReset() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_netdiag, test_otThreadSendDiagnosticReset)
{
	otThreadSendDiagnosticReset_fake.custom_fake = ot_thread_send_diagnostic_reset;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_RESET, CBOR_ADDR1,
					CBOR_NETDIAG_TLVS));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadSendDiagnosticReset_fake.call_count, 1);
}

static otError ot_thread_send_diagnostic_get(otInstance *instance, const otIp6Address *address,
					     const uint8_t *tlvs, uint8_t count,
					     otReceiveDiagnosticGetCallback callback, void *context)
{
	const uint8_t expected_tlvs[] = {NETDIAG_TLVS};
	const otIp6Address expected_address = {.mFields.m8 = {ADDR_1}};

	zassert_mem_equal(address->mFields.m8, expected_address.mFields.m8, OT_IP6_ADDRESS_SIZE);
	zassert_mem_equal(tlvs, expected_tlvs, NETDIAG_TLVS_LEN);
	zassert_equal(count, NETDIAG_TLVS_LEN);

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadSendDiagnosticGet() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_netdiag, test_otThreadSendDiagnosticGet)
{
	otMessageInfo message_info = {
		.mSockAddr = {.mFields.m8 = {ADDR_1}},
		.mPeerAddr = {.mFields.m8 = {ADDR_2}},
		.mSockPort = PORT_1,
		.mPeerPort = PORT_2,
		.mHopLimit = HOP_LIMIT,
		.mEcn = 3,
		.mIsHostInterface = true,
		.mAllowZeroHopLimit = true,
		.mMulticastLoop = true,
	};

	otThreadSendDiagnosticGet_fake.custom_fake = ot_thread_send_diagnostic_get;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET, CBOR_ADDR1,
					CBOR_NETDIAG_TLVS));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otThreadSendDiagnosticGet_fake.call_count, 1);

	/* Verify that callback is encoded correctly */
	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET_CB, OT_ERROR_NONE, RESOURCE_TABLE_KEY,
			CBOR_MSG_INFO), RPC_RSP());
	otThreadSendDiagnosticGet_fake.arg4_val(OT_ERROR_NONE, (otMessage *)MSG_ADDR,
						&message_info, NULL);
	mock_nrf_rpc_tr_expect_done();
}

static otError ot_thread_get_next_diagnostic_tlv_ext_addr(const otMessage *message,
							  otNetworkDiagIterator *iterator,
							  otNetworkDiagTlv *tlv)
{
	const otExtAddress address = {.m8 = {NETDIAG_TLV_EXT_ADDR}};

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS;
	memcpy(tlv->mData.mExtAddress.m8, address.m8, OT_EXT_ADDRESS_SIZE);

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_EXT_ADDR.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_ext_addr)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake = ot_thread_get_next_diagnostic_tlv_ext_addr;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_EXT_ADDR), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_eui64(const otMessage *message,
						       otNetworkDiagIterator *iterator,
						       otNetworkDiagTlv *tlv)
{
	const otExtAddress address = {.m8 = {NETDIAG_TLV_EXT_ADDR}};

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_EUI64;
	memcpy(tlv->mData.mEui64.m8, address.m8, OT_EXT_ADDRESS_SIZE);

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_EUI64.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_eui64)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake = ot_thread_get_next_diagnostic_tlv_eui64;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_EUI64), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_mode(const otMessage *message,
						      otNetworkDiagIterator *iterator,
						      otNetworkDiagTlv *tlv)
{
	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_MODE;
	tlv->mData.mMode.mRxOnWhenIdle = true;
	tlv->mData.mMode.mDeviceType = true;
	tlv->mData.mMode.mNetworkData = true;

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_MODE.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_mode)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake = ot_thread_get_next_diagnostic_tlv_mode;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_MODE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_connectivity(const otMessage *message,
							      otNetworkDiagIterator *iterator,
							      otNetworkDiagTlv *tlv)
{
	const uint8_t data[] = {NETDIAG_TLV_CONNECTIVITY};
	uint8_t data_field = 0;

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY;
	tlv->mData.mConnectivity.mParentPriority = data[data_field++];
	tlv->mData.mConnectivity.mLinkQuality3 = data[data_field++];
	tlv->mData.mConnectivity.mLinkQuality2 = data[data_field++];
	tlv->mData.mConnectivity.mLinkQuality1 = data[data_field++];
	tlv->mData.mConnectivity.mLeaderCost = data[data_field++];
	tlv->mData.mConnectivity.mIdSequence = data[data_field++];
	tlv->mData.mConnectivity.mActiveRouters = data[data_field++];
	tlv->mData.mConnectivity.mSedBufferSize = data[data_field++];
	tlv->mData.mConnectivity.mSedDatagramCount = data[data_field++];

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_CONNECTIVITY.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_connectivity)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_connectivity;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_CONNECTIVITY), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_route(const otMessage *message,
						       otNetworkDiagIterator *iterator,
						       otNetworkDiagTlv *tlv)
{
	const uint8_t data[] = {NETDIAG_TLV_ROUTE};
	uint8_t data_field = 0;

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_ROUTE;
	tlv->mData.mRoute.mIdSequence = data[data_field++];
	tlv->mData.mRoute.mRouteCount = data[data_field++];
	for (int i = 0; i < tlv->mData.mRoute.mRouteCount; i++) {
		tlv->mData.mRoute.mRouteData[i].mRouterId = data[data_field++];
		tlv->mData.mRoute.mRouteData[i].mLinkQualityIn = data[data_field++];
		tlv->mData.mRoute.mRouteData[i].mLinkQualityOut = data[data_field++];
		tlv->mData.mRoute.mRouteData[i].mRouteCost = data[data_field++];
	}

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_ROUTE.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_route)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake = ot_thread_get_next_diagnostic_tlv_route;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_ROUTE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_leader_data(const otMessage *message,
							     otNetworkDiagIterator *iterator,
							     otNetworkDiagTlv *tlv)
{
	const uint8_t data[] = {NETDIAG_TLV_LEADER_DATA};
	uint8_t data_field = 0;

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA;
	tlv->mData.mLeaderData.mPartitionId = data[data_field++];
	tlv->mData.mLeaderData.mWeighting = data[data_field++];
	tlv->mData.mLeaderData.mDataVersion = data[data_field++];
	tlv->mData.mLeaderData.mStableDataVersion = data[data_field++];
	tlv->mData.mLeaderData.mLeaderRouterId = data[data_field++];

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_LEADER_DATA.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_leader_data)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_leader_data;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_LEADER_DATA), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_mac_counters(const otMessage *message,
							      otNetworkDiagIterator *iterator,
							      otNetworkDiagTlv *tlv)
{
	const uint8_t data[] = {NETDIAG_TLV_MAC_COUNTERS};
	uint8_t data_field = 0;

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS;
	tlv->mData.mMacCounters.mIfInUnknownProtos = data[data_field++];
	tlv->mData.mMacCounters.mIfInErrors = data[data_field++];
	tlv->mData.mMacCounters.mIfOutErrors = data[data_field++];
	tlv->mData.mMacCounters.mIfInUcastPkts = data[data_field++];
	tlv->mData.mMacCounters.mIfInBroadcastPkts = data[data_field++];
	tlv->mData.mMacCounters.mIfInDiscards = data[data_field++];
	tlv->mData.mMacCounters.mIfOutUcastPkts = data[data_field++];
	tlv->mData.mMacCounters.mIfOutBroadcastPkts = data[data_field++];
	tlv->mData.mMacCounters.mIfOutDiscards = data[data_field++];

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_MAC_COUNTERS.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_mac_counters)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_mac_counters;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_MAC_COUNTERS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_mle_counters(const otMessage *message,
							      otNetworkDiagIterator *iterator,
							      otNetworkDiagTlv *tlv)
{
	const uint64_t data[] = {NETDIAG_TLV_MLE_COUNTERS};
	uint8_t data_field = 0;

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS;
	tlv->mData.mMleCounters.mDisabledRole = data[data_field++];
	tlv->mData.mMleCounters.mDetachedRole = data[data_field++];
	tlv->mData.mMleCounters.mChildRole = data[data_field++];
	tlv->mData.mMleCounters.mRouterRole = data[data_field++];
	tlv->mData.mMleCounters.mLeaderRole = data[data_field++];
	tlv->mData.mMleCounters.mAttachAttempts = data[data_field++];
	tlv->mData.mMleCounters.mPartitionIdChanges = data[data_field++];
	tlv->mData.mMleCounters.mBetterPartitionAttachAttempts = data[data_field++];
	tlv->mData.mMleCounters.mParentChanges = data[data_field++];
	tlv->mData.mMleCounters.mTrackedTime = data[data_field++];
	tlv->mData.mMleCounters.mDisabledTime = data[data_field++];
	tlv->mData.mMleCounters.mDetachedTime = data[data_field++];
	tlv->mData.mMleCounters.mChildTime = data[data_field++];
	tlv->mData.mMleCounters.mRouterTime = data[data_field++];
	tlv->mData.mMleCounters.mLeaderTime = data[data_field++];

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_MLE_COUNTERS.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_mle_counters)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_mle_counters;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_MLE_COUNTERS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_battery_level(const otMessage *message,
							       otNetworkDiagIterator *iterator,
							       otNetworkDiagTlv *tlv)
{
	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL;
	tlv->mData.mBatteryLevel = NETDIAG_TLV_BATTERY_LEVEL;

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_BATTERY_LEVEL.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_battery_level)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_battery_level;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_BATTERY_LEVEL), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_timeout(const otMessage *message,
							 otNetworkDiagIterator *iterator,
							 otNetworkDiagTlv *tlv)
{
	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT;
	tlv->mData.mTimeout = NETDIAG_TLV_TIMEOUT;

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_TIMEOUT.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_timeout)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake = ot_thread_get_next_diagnostic_tlv_timeout;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_TIMEOUT), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_max_child_timeout(const otMessage *message,
								   otNetworkDiagIterator *iterator,
								   otNetworkDiagTlv *tlv)
{
	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT;
	tlv->mData.mMaxChildTimeout = NETDIAG_TLV_MAX_CHILD_TIMEOUT;

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_MAX_CHILD_TIMEOUT.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_max_child_timeout)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_max_child_timeout;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_MAX_CHILD_TIMEOUT), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_short_address(const otMessage *message,
							       otNetworkDiagIterator *iterator,
							       otNetworkDiagTlv *tlv)
{
	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS;
	tlv->mData.mAddr16 = NETDIAG_TLV_SHORT_ADDRESS;

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_SHORT_ADDRESS.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_short_address)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_short_address;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_SHORT_ADDRESS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_supply_voltage(const otMessage *message,
								otNetworkDiagIterator *iterator,
								otNetworkDiagTlv *tlv)
{
	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE;
	tlv->mData.mSupplyVoltage = NETDIAG_TLV_SUPPLY_VOLTAGE;

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_SUPPLY_VOLTAGE.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_supply_voltage)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_supply_voltage;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_SUPPLY_VOLTAGE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_version(const otMessage *message,
							 otNetworkDiagIterator *iterator,
							 otNetworkDiagTlv *tlv)
{
	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_VERSION;
	tlv->mData.mVersion = NETDIAG_TLV_VERSION;

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_VERSION.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_version)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake = ot_thread_get_next_diagnostic_tlv_version;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_VERSION), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_vendor_name(const otMessage *message,
							     otNetworkDiagIterator *iterator,
							     otNetworkDiagTlv *tlv)
{
	const char data[] = {NETDIAG_TLV_VENDOR_NAME, '\0'};

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME;
	strcpy(tlv->mData.mVendorName, data);

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_VENDOR_NAME.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_vendor_name)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_vendor_name;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_VENDOR_NAME), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_vendor_model(const otMessage *message,
							      otNetworkDiagIterator *iterator,
							      otNetworkDiagTlv *tlv)
{
	const char data[] = {NETDIAG_TLV_VENDOR_MODEL, '\0'};

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL;
	strcpy(tlv->mData.mVendorName, data);

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_VENDOR_MODEL.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_vendor_model)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_vendor_model;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_VENDOR_MODEL), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_vendor_sw_version(const otMessage *message,
								   otNetworkDiagIterator *iterator,
								   otNetworkDiagTlv *tlv)
{
	const char data[] = {NETDIAG_TLV_VENDOR_SW_VERSION, '\0'};

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION;
	strcpy(tlv->mData.mVendorName, data);

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_VENDOR_SW_VERSION.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_vendor_sw_version)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_vendor_sw_version;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_VENDOR_SW_VERSION), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_vendor_app_url(const otMessage *message,
								otNetworkDiagIterator *iterator,
								otNetworkDiagTlv *tlv)
{
	const char data[] = {NETDIAG_TLV_VENDOR_APP_URL, '\0'};

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_APP_URL;
	strcpy(tlv->mData.mVendorAppUrl, data);

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_VENDOR_APP_URL.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_vendor_app_url)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_vendor_app_url;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_VENDOR_APP_URL), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_thread_stack_version(const otMessage *message,
								otNetworkDiagIterator *iterator,
								otNetworkDiagTlv *tlv)
{
	const char data[] = {NETDIAG_TLV_THREAD_STACK_VERSION, '\0'};

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION;
	strcpy(tlv->mData.mThreadStackVersion, data);

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_THREAD_STACK_VERSION.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_thread_stack_version)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_thread_stack_version;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_THREAD_STACK_VERSION), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_network_data(const otMessage *message,
							      otNetworkDiagIterator *iterator,
							      otNetworkDiagTlv *tlv)
{
	const uint8_t data[] = {NETDIAG_TLV_NETWORK_DATA};

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA;
	tlv->mData.mNetworkData.mCount = OT_NETWORK_BASE_TLV_MAX_LENGTH;
	memcpy(tlv->mData.mNetworkData.m8, data, OT_NETWORK_BASE_TLV_MAX_LENGTH);

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_NETWORK_DATA.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_network_data)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_network_data;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_NETWORK_DATA), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_channel_pages(const otMessage *message,
							       otNetworkDiagIterator *iterator,
							       otNetworkDiagTlv *tlv)
{
	const uint8_t data[] = {NETDIAG_TLV_CHANNEL_PAGES};

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES;
	tlv->mData.mChannelPages.mCount = OT_NETWORK_BASE_TLV_MAX_LENGTH;
	memcpy(tlv->mData.mChannelPages.m8, data, OT_NETWORK_BASE_TLV_MAX_LENGTH);

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_CHANNEL_PAGES.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_channel_pages)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_channel_pages;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_CHANNEL_PAGES), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_ip_addr_list(const otMessage *message,
							      otNetworkDiagIterator *iterator,
							      otNetworkDiagTlv *tlv)
{
	const otIp6Address address_1 = {.mFields.m8 = {ADDR_1}};
	const otIp6Address address_2 = {.mFields.m8 = {ADDR_2}};

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST;
	tlv->mData.mIp6AddrList.mCount = 2;
	memcpy(tlv->mData.mIp6AddrList.mList[0].mFields.m8, address_1.mFields.m8,
	       OT_IP6_ADDRESS_SIZE);
	memcpy(tlv->mData.mIp6AddrList.mList[1].mFields.m8, address_2.mFields.m8,
	       OT_IP6_ADDRESS_SIZE);

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_IP6_ADDR_LIST.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_ip_addr_list)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_ip_addr_list;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_IP6_ADDR_LIST), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

static otError ot_thread_get_next_diagnostic_tlv_child_table(const otMessage *message,
							     otNetworkDiagIterator *iterator,
							     otNetworkDiagTlv *tlv)
{
	const uint8_t data[] = {NETDIAG_TLV_CHILD_TABLE};
	uint8_t data_field = 0;

	tlv->mType = OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE;
	tlv->mData.mChildTable.mCount = NETDIAG_CHILD_TABLE_LEN;
	for (int i = 0; i < tlv->mData.mChildTable.mCount; i++) {
		tlv->mData.mChildTable.mTable[i].mTimeout = data[data_field++];
		tlv->mData.mChildTable.mTable[i].mLinkQuality = data[data_field++];
		tlv->mData.mChildTable.mTable[i].mChildId = data[data_field++];
		tlv->mData.mChildTable.mTable[i].mMode.mRxOnWhenIdle = true;
		tlv->mData.mChildTable.mTable[i].mMode.mDeviceType = true;
		tlv->mData.mChildTable.mTable[i].mMode.mNetworkData = true;

		/* Skip the 0x7 data field responsible for setting mMode flags. */
		data_field++;
	}

	zassert_equal(message, (otMessage *)MSG_ADDR);
	(*iterator)++;

	return OT_ERROR_NONE;
}

/*
 * Test reception of otThreadGetNextDiagnosticTlv() command.
 * Test serialization of the result: OT_ERROR_NONE with CBOR_NETDIAG_TLV_CHILD_TABLE.
 */
ZTEST(ot_rpc_netdiag, test_otThreadGetNextDiagnosticTlv_child_table)
{
	ot_rpc_res_tab_key message_key = ot_res_tab_msg_alloc((otMessage *)MSG_ADDR);

	otThreadGetNextDiagnosticTlv_fake.custom_fake =
		ot_thread_get_next_diagnostic_tlv_child_table;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE, NETDIAG_NEXT_ITERATOR,
					   CBOR_NETDIAG_TLV_CHILD_TABLE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
					message_key, OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT));
	mock_nrf_rpc_tr_expect_done();

	ot_res_tab_msg_free(message_key);
	zassert_equal(otThreadGetNextDiagnosticTlv_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_netdiag, NULL, NULL, tc_setup, NULL, NULL);
