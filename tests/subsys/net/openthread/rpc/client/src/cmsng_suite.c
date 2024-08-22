/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <nrf_rpc/nrf_rpc_cbkproxy.h>

#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>
#include <test_rpc_env.h>

static int slot_cnt;
static bool cb_called;

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

/* Test serialization of otThreadDiscover() */
ZTEST(ot_rpc_commissioning_cli, test_otThreadDiscover)
{
	otError error;
	uint32_t scan_channels = 0x7fff800;
	uint16_t pan_id = 0x4321;
	bool joiner = false;
	bool enable_eui64_filtering = true;
	otHandleActiveScanResult cb = (otHandleActiveScanResult)0xdeadbeef;
	void *cb_ctx = (void *)0xcafeface;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_THREAD_DISCOVER, CBOR_UINT32(0x7fff800),
					   CBOR_UINT16(0x4321), CBOR_FALSE, CBOR_TRUE, slot_cnt,
					   CBOR_UINT32(0xcafeface)),
				   RPC_RSP(OT_ERROR_NONE));
	error = otThreadDiscover(NULL, scan_channels, pan_id, joiner, enable_eui64_filtering, cb,
				 cb_ctx);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otDatasetIsCommissioned() */
ZTEST(ot_rpc_commissioning_cli, test_otDatasetIsCommissioned)
{
	bool result;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATASET_IS_COMMISSIONED), RPC_RSP(CBOR_TRUE));
	result = otDatasetIsCommissioned(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_true(result);

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATASET_IS_COMMISSIONED),
				   RPC_RSP(CBOR_FALSE));
	result = otDatasetIsCommissioned(NULL);
	mock_nrf_rpc_tr_expect_done();
	zassert_false(result);
}

/* Test serialization of otDatasetSetActiveTlvs() */
ZTEST(ot_rpc_commissioning_cli, test_otDatasetSetActiveTlvs)
{
	otError error;

	const otOperationalDatasetTlvs dataset = {
		{INT_SEQUENCE(OT_OPERATIONAL_DATASET_MAX_LENGTH)},
		OT_OPERATIONAL_DATASET_MAX_LENGTH};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATASET_SET_ACTIVE_TLVS, TLVS),
				   RPC_RSP(OT_ERROR_NONE));
	error = otDatasetSetActiveTlvs(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test incoming parameters validation of otDatasetSetActiveTlvs() */
ZTEST(ot_rpc_commissioning_cli, test_otDatasetSetActiveTlvs_negative)
{
	otError error;
	const otOperationalDatasetTlvs dataset = {{0}, OT_OPERATIONAL_DATASET_MAX_LENGTH + 1};

	error = otDatasetSetActiveTlvs(NULL, &dataset);
	zassert_equal(error, OT_ERROR_INVALID_ARGS);
	error = otDatasetSetActiveTlvs(NULL, NULL);
	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test serialization of otDatasetGetActiveTlvs() */
ZTEST(ot_rpc_commissioning_cli, test_otDatasetGetActiveTlvs)
{
	otError error;
	otOperationalDatasetTlvs dataset;
	const otOperationalDatasetTlvs expected_dataset = {
		{INT_SEQUENCE(OT_OPERATIONAL_DATASET_MAX_LENGTH)},
		OT_OPERATIONAL_DATASET_MAX_LENGTH};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS), RPC_RSP(TLVS));
	error = otDatasetGetActiveTlvs(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
	zassert_mem_equal(&dataset, &expected_dataset, sizeof(otOperationalDatasetTlvs));
}

/* Test NULL replay on otDatasetGetActiveTlvs() and parameter validation. */
ZTEST(ot_rpc_commissioning_cli, test_otDatasetGetActiveTlvs_null)
{
	otError error;
	otOperationalDatasetTlvs dataset;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS), RPC_RSP(CBOR_NULL));
	error = otDatasetGetActiveTlvs(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NOT_FOUND);

	error = otDatasetGetActiveTlvs(NULL, NULL);
	zassert_equal(error, OT_ERROR_NOT_FOUND);
}

static void discover_cb(otActiveScanResult *result, void *ctx)
{
	uint8_t ext_addr[] = {INT_SEQUENCE(OT_EXT_ADDRESS_SIZE)};
	uint8_t nwk_name[] = {INT_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE), 0};
	uint8_t ext_pan_id[] = {INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)};
	uint8_t steer_data[] = {INT_SEQUENCE(OT_STEERING_DATA_MAX_LENGTH)};

	cb_called = true;
	zassert_not_null(result);
	zassert_not_null(ctx);
	zassert_mem_equal(result->mExtAddress.m8, ext_addr, OT_EXT_ADDRESS_SIZE);
	zassert_mem_equal(result->mNetworkName.m8, nwk_name, OT_NETWORK_NAME_MAX_SIZE + 1);
	zassert_mem_equal(result->mExtendedPanId.m8, ext_pan_id, OT_EXT_PAN_ID_SIZE);
	zassert_mem_equal(result->mSteeringData.m8, steer_data, OT_STEERING_DATA_MAX_LENGTH);
	zassert_equal(result->mPanId, 0x1234);
	zassert_equal(result->mJoinerUdpPort, 0x4321);
	zassert_equal(result->mChannel, 0x22);
	zassert_equal(result->mRssi, 0x33);
	zassert_equal(result->mLqi, 0x44);
	zassert_equal(result->mVersion, 0x0f);
	zassert_false(!!result->mIsNative);
	zassert_true(!!result->mDiscover);
	zassert_false(!!result->mIsJoinable);
	zassert_equal_ptr(ctx, (void *)0xdeadbeef);
}

/* Test reception of discover callback with active scan result. */
ZTEST(ot_rpc_commissioning_cli, test_discover_cb_handler)
{

	int in_slot = nrf_rpc_cbkproxy_in_set(discover_cb);

	slot_cnt++;
	cb_called = false;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_DISCOVER_CB, EXT_ADDR, NWK_NAME,
					EXT_PAN_ID, STEERING_DATA, CBOR_UINT16(0x1234),
					CBOR_UINT16(0x4321), CBOR_UINT8(0x22), CBOR_UINT8(0x33),
					CBOR_UINT8(0x44), 0x0f, CBOR_FALSE, CBOR_TRUE, CBOR_FALSE,
					CBOR_UINT32(0xdeadbeef), in_slot));
	mock_nrf_rpc_tr_expect_done();

	zassert_true(cb_called);
}

static void discover_empty_cb(otActiveScanResult *result, void *ctx)
{
	cb_called = true;
	zassert_is_null(result);
	zassert_not_null(ctx);
	zassert_equal_ptr(ctx, (void *)0xdeadbeef);
}

/* Test reception of discover callback with empty active scan result. */
ZTEST(ot_rpc_commissioning_cli, test_discover_cb_handler_empty)
{

	int in_slot = nrf_rpc_cbkproxy_in_set(discover_empty_cb);

	slot_cnt++;
	cb_called = false;
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_THREAD_DISCOVER_CB, CBOR_NULL,
					CBOR_UINT32(0xdeadbeef), in_slot));
	mock_nrf_rpc_tr_expect_done();

	zassert_true(cb_called);
}

/* Test serialization of otDatasetSetActive() */
ZTEST(ot_rpc_commissioning_cli, test_otDatasetSetActive)
{
	otError error;

	const otOperationalDataset dataset = {
		{0x0123456789abcdefull, 0x1234, false},
		{0x0123456789abcdefull, 0x1234, false},
		{{INT_SEQUENCE(OT_NETWORK_KEY_SIZE)}},
		{{INT_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE)}},
		{{INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)}},
		{{INT_SEQUENCE(OT_IP6_PREFIX_SIZE)}},
		0x12345678ul,
		0xabcd,
		0xef67,
		{{INT_SEQUENCE(OT_PSKC_MAX_SIZE)}},
		{0x9876, 1, 0, 1, 0, 1, 0, 1, 0, 1, 7},
		0xfedcba98,
		{false, true, false, true, false, true, false, true, false, true, false, true}};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATASET_SET_ACTIVE, DATASET),
				   RPC_RSP(OT_ERROR_NONE));
	error = otDatasetSetActive(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test incoming parameters validation of otDatasetSetActive() */
ZTEST(ot_rpc_commissioning_cli, test_otDatasetSetActive_negative)
{
	otError error;

	error = otDatasetSetActive(NULL, NULL);
	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test serialization of otDatasetGetActive() */
ZTEST(ot_rpc_commissioning_cli, test_otDatasetGetActive)
{
	otError error;
	otOperationalDataset dataset;
	uint8_t net_key[] = {INT_SEQUENCE(OT_NETWORK_KEY_SIZE)};
	uint8_t nwk_name[] = {INT_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE), 0};
	uint8_t ext_pan_id[] = {INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)};
	uint8_t local_prefix[] = {INT_SEQUENCE(OT_IP6_PREFIX_SIZE)};
	uint8_t pskc[] = {INT_SEQUENCE(OT_PSKC_MAX_SIZE)};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATASET_GET_ACTIVE), RPC_RSP(DATASET));
	error = otDatasetGetActive(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
	zassert_equal(dataset.mActiveTimestamp.mSeconds, 0x0123456789abcdefull);
	zassert_equal(dataset.mActiveTimestamp.mTicks, 0x1234);
	zassert_false(dataset.mActiveTimestamp.mAuthoritative);

	zassert_equal(dataset.mPendingTimestamp.mSeconds, 0x0123456789abcdefull);
	zassert_equal(dataset.mPendingTimestamp.mTicks, 0x1234);
	zassert_false(dataset.mPendingTimestamp.mAuthoritative);

	zassert_mem_equal(dataset.mNetworkKey.m8, net_key, OT_NETWORK_KEY_SIZE);
	zassert_mem_equal(dataset.mNetworkName.m8, nwk_name, OT_NETWORK_NAME_MAX_SIZE + 1);
	zassert_mem_equal(dataset.mExtendedPanId.m8, ext_pan_id, OT_EXT_PAN_ID_SIZE);
	zassert_mem_equal(dataset.mMeshLocalPrefix.m8, local_prefix, OT_IP6_PREFIX_SIZE);
	zassert_equal(dataset.mDelay, 0x12345678ul);
	zassert_equal(dataset.mPanId, 0xabcd);
	zassert_equal(dataset.mChannel, 0xef67);
	zassert_mem_equal(dataset.mPskc.m8, pskc, OT_PSKC_MAX_SIZE);

	zassert_equal(dataset.mSecurityPolicy.mRotationTime, 0x9876);
	zassert_true(!!dataset.mSecurityPolicy.mObtainNetworkKeyEnabled);
	zassert_false(!!dataset.mSecurityPolicy.mNativeCommissioningEnabled);
	zassert_true(!!dataset.mSecurityPolicy.mRoutersEnabled);
	zassert_false(!!dataset.mSecurityPolicy.mExternalCommissioningEnabled);
	zassert_true(!!dataset.mSecurityPolicy.mCommercialCommissioningEnabled);
	zassert_false(!!dataset.mSecurityPolicy.mAutonomousEnrollmentEnabled);
	zassert_true(!!dataset.mSecurityPolicy.mNetworkKeyProvisioningEnabled);
	zassert_false(!!dataset.mSecurityPolicy.mTobleLinkEnabled);
	zassert_true(!!dataset.mSecurityPolicy.mNonCcmRoutersEnabled);
	zassert_equal(dataset.mSecurityPolicy.mVersionThresholdForRouting, 7);

	zassert_equal(dataset.mChannelMask, 0xfedcba98ul);

	zassert_false(dataset.mComponents.mIsActiveTimestampPresent);
	zassert_true(dataset.mComponents.mIsPendingTimestampPresent);
	zassert_false(dataset.mComponents.mIsNetworkKeyPresent);
	zassert_true(dataset.mComponents.mIsNetworkNamePresent);
	zassert_false(dataset.mComponents.mIsExtendedPanIdPresent);
	zassert_true(dataset.mComponents.mIsMeshLocalPrefixPresent);
	zassert_false(dataset.mComponents.mIsDelayPresent);
	zassert_true(dataset.mComponents.mIsPanIdPresent);
	zassert_false(dataset.mComponents.mIsChannelPresent);
	zassert_true(dataset.mComponents.mIsPskcPresent);
	zassert_false(dataset.mComponents.mIsSecurityPolicyPresent);
	zassert_true(dataset.mComponents.mIsChannelMaskPresent);
}

/* Test NULL replay on otDatasetGetActive() and parameter validation. */
ZTEST(ot_rpc_commissioning_cli, test_otDatasetGetActive_null)
{
	otError error;
	otOperationalDataset dataset;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATASET_GET_ACTIVE), RPC_RSP(CBOR_NULL));
	error = otDatasetGetActive(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NOT_FOUND);

	error = otDatasetGetActive(NULL, NULL);
	zassert_equal(error, OT_ERROR_NOT_FOUND);
}

ZTEST_SUITE(ot_rpc_commissioning_cli, NULL, NULL, tc_setup, NULL, NULL);
