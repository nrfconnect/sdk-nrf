/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <nrf_rpc/nrf_rpc_cbkproxy.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/dataset.h>

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

/* Test serialization of otDatasetIsCommissioned() */
ZTEST(ot_rpc_dataset, test_otDatasetIsCommissioned)
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

static void test_set_tlvs(uint8_t cmd,
			  otError (*set)(otInstance *, const otOperationalDatasetTlvs *))
{
	otError error = OT_ERROR_NONE;

	const otOperationalDatasetTlvs dataset = {{INT_SEQUENCE(OT_OPERATIONAL_DATASET_MAX_LENGTH)},
						  OT_OPERATIONAL_DATASET_MAX_LENGTH};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(cmd, TLVS), RPC_RSP(OT_ERROR_NONE));
	error = set(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otDatasetSetActiveTlvs() */
ZTEST(ot_rpc_dataset, test_otDatasetSetActiveTlvs)
{
	test_set_tlvs(OT_RPC_CMD_DATASET_SET_ACTIVE_TLVS, otDatasetSetActiveTlvs);
}

/* Test serialization of otDatasetSetPendingTlvs() */
ZTEST(ot_rpc_dataset, test_otDatasetSetPendingTlvs)
{
	test_set_tlvs(OT_RPC_CMD_DATASET_SET_PENDING_TLVS, otDatasetSetPendingTlvs);
}

static void test_set_tlvs_negative(otError (*set)(otInstance *, const otOperationalDatasetTlvs *))
{
	otError error;
	const otOperationalDatasetTlvs dataset = {{0}, OT_OPERATIONAL_DATASET_MAX_LENGTH + 1};

	error = set(NULL, &dataset);
	zassert_equal(error, OT_ERROR_INVALID_ARGS);
}

/* Test incoming parameters validation of otDatasetSetActiveTlvs() */
ZTEST(ot_rpc_dataset, test_otDatasetSetActiveTlvs_negative)
{
	test_set_tlvs_negative(otDatasetSetActiveTlvs);
}

/* Test incoming parameters validation of otDatasetSetPendingTlvs() */
ZTEST(ot_rpc_dataset, test_otDatasetSetPendingTlvs_negative)
{
	test_set_tlvs_negative(otDatasetSetPendingTlvs);
}

static void test_get_tlvs(uint8_t cmd, otError (*get)(otInstance *, otOperationalDatasetTlvs *))
{
	otError error;
	otOperationalDatasetTlvs dataset;
	const otOperationalDatasetTlvs expected_dataset = {
		{INT_SEQUENCE(OT_OPERATIONAL_DATASET_MAX_LENGTH)},
		OT_OPERATIONAL_DATASET_MAX_LENGTH};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(cmd), RPC_RSP(TLVS));
	error = get(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
	zassert_mem_equal(&dataset, &expected_dataset, sizeof(otOperationalDatasetTlvs));
}

/* Test serialization of otDatasetGetActiveTlvs() */
ZTEST(ot_rpc_dataset, test_otDatasetGetActiveTlvs)
{
	test_get_tlvs(OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS, otDatasetGetActiveTlvs);
}

/* Test serialization of otDatasetGetPendingTlvs() */
ZTEST(ot_rpc_dataset, test_otDatasetGetPendingTlvs)
{
	test_get_tlvs(OT_RPC_CMD_DATASET_GET_PENDING_TLVS, otDatasetGetPendingTlvs);
}

static void test_get_tlvs_negative(uint8_t cmd,
				   otError (*get)(otInstance *, otOperationalDatasetTlvs *))
{
	otError error;
	otOperationalDatasetTlvs dataset;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(cmd), RPC_RSP(CBOR_NULL));
	error = get(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NOT_FOUND);
}

/* Test NULL replay on otDatasetGetActiveTlvs() and parameter validation. */
ZTEST(ot_rpc_dataset, test_otDatasetGetActiveTlvs_negative)
{
	test_get_tlvs_negative(OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS, otDatasetGetActiveTlvs);
}

/* Test NULL replay on otDatasetGetPendingTlvs() and parameter validation. */
ZTEST(ot_rpc_dataset, test_otDatasetGetPendingTlvs_negative)
{
	test_get_tlvs_negative(OT_RPC_CMD_DATASET_GET_PENDING_TLVS, otDatasetGetPendingTlvs);
}

static void test_set(uint8_t cmd, otError (*set)(otInstance *, const otOperationalDataset *))
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

	mock_nrf_rpc_tr_expect_add(RPC_CMD(cmd, DATASET), RPC_RSP(OT_ERROR_NONE));
	error = set(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otDatasetSetActive() */
ZTEST(ot_rpc_dataset, test_otDatasetSetActive)
{
	test_set(OT_RPC_CMD_DATASET_SET_ACTIVE, otDatasetSetActive);
}

/* Test serialization of otDatasetSetPending() */
ZTEST(ot_rpc_dataset, test_otDatasetSetPending)
{
	test_set(OT_RPC_CMD_DATASET_SET_PENDING, otDatasetSetPending);
}

static void test_get(uint8_t cmd, otError (*get)(otInstance *, otOperationalDataset *))
{
	otError error;
	otOperationalDataset dataset;
	uint8_t net_key[] = {INT_SEQUENCE(OT_NETWORK_KEY_SIZE)};
	uint8_t nwk_name[] = {INT_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE), 0};
	uint8_t ext_pan_id[] = {INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)};
	uint8_t local_prefix[] = {INT_SEQUENCE(OT_IP6_PREFIX_SIZE)};
	uint8_t pskc[] = {INT_SEQUENCE(OT_PSKC_MAX_SIZE)};

	mock_nrf_rpc_tr_expect_add(RPC_CMD(cmd), RPC_RSP(DATASET));
	error = get(NULL, &dataset);
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

/* Test serialization of otDatasetGetActive() */
ZTEST(ot_rpc_dataset, test_otDatasetGetActive)
{
	test_get(OT_RPC_CMD_DATASET_GET_ACTIVE, otDatasetGetActive);
}

/* Test serialization of otDatasetGetPending() */
ZTEST(ot_rpc_dataset, test_otDatasetGetPending)
{
	test_get(OT_RPC_CMD_DATASET_GET_PENDING, otDatasetGetPending);
}

static void test_get_negative(uint8_t cmd, otError (*get)(otInstance *, otOperationalDataset *))
{
	otError error;
	otOperationalDataset dataset;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(cmd), RPC_RSP(CBOR_NULL));
	error = get(NULL, &dataset);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NOT_FOUND);
}

/* Test NULL replay on otDatasetGetActive() and parameter validation. */
ZTEST(ot_rpc_dataset, test_otDatasetGetActive_negative)
{
	test_get_negative(OT_RPC_CMD_DATASET_GET_ACTIVE, otDatasetGetActive);
}

/* Test NULL replay on otDatasetGetPending() and parameter validation. */
ZTEST(ot_rpc_dataset, test_otDatasetGetPending_negative)
{
	test_get_negative(OT_RPC_CMD_DATASET_GET_PENDING, otDatasetGetPending);
}

ZTEST_SUITE(ot_rpc_dataset, NULL, NULL, tc_setup, NULL, NULL);
