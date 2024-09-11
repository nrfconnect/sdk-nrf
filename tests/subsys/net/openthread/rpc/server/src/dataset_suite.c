/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/dataset.h>

/* Fake functions */
FAKE_VALUE_FUNC(bool, otDatasetIsCommissioned, otInstance *);
FAKE_VALUE_FUNC(otError, otDatasetSetActiveTlvs, otInstance *, const otOperationalDatasetTlvs *);
FAKE_VALUE_FUNC(otError, otDatasetGetActiveTlvs, otInstance *, otOperationalDatasetTlvs *);
FAKE_VALUE_FUNC(otError, otDatasetSetActive, otInstance *, const otOperationalDataset *);
FAKE_VALUE_FUNC(otError, otDatasetGetActive, otInstance *, otOperationalDataset *);

#define FOREACH_FAKE(f)                                                                            \
	f(otDatasetIsCommissioned);                                                                \
	f(otDatasetSetActiveTlvs);                                                                 \
	f(otDatasetGetActiveTlvs);                                                                 \
	f(otDatasetSetActive);                                                                     \
	f(otDatasetGetActive);

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
 * Test reception of otDatasetIsCommissioned command.
 * Test serialization of the result: true or false.
 */
ZTEST(ot_rpc_dataset, test_otDatasetIsCommissioned)
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
ZTEST(ot_rpc_dataset, test_otDatasetSetActiveTlvs)
{
	const otOperationalDatasetTlvs expected_dataset = {
		{INT_SEQUENCE(OT_OPERATIONAL_DATASET_MAX_LENGTH)},
		OT_OPERATIONAL_DATASET_MAX_LENGTH};

	otDatasetSetActiveTlvs_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATASET_SET_ACTIVE_TLVS, TLVS));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetSetActiveTlvs_fake.call_count, 1);
	zassert_mem_equal(otDatasetSetActiveTlvs_fake.arg1_val, &expected_dataset,
			  sizeof(otOperationalDatasetTlvs));
}

/*
 * Test reception of otDatasetSetActiveTlvs command with NULL pointer.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_dataset, test_otDatasetSetActiveTlvs_negative)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_INVALID_ARGS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATASET_SET_ACTIVE_TLVS, CBOR_NULL));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetSetActiveTlvs_fake.call_count, 0);
}

static otError dataset_get_tlvs_fake(otInstance *instance, otOperationalDatasetTlvs *dataset)
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
ZTEST(ot_rpc_dataset, test_otDatasetGetActiveTlvs)
{
	otDatasetGetActiveTlvs_fake.custom_fake = dataset_get_tlvs_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(TLVS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetGetActiveTlvs_fake.call_count, 1);
}

/*
 * Test reception of otDatasetGetActiveTlvs command.
 * Test serialization of the result: NULL as tlvs data has not been found.
 */
ZTEST(ot_rpc_dataset, test_otDatasetGetActiveTlvs_null)
{
	otDatasetGetActiveTlvs_fake.return_val = OT_ERROR_NOT_FOUND;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_NULL), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetGetActiveTlvs_fake.call_count, 1);
}

/*
 * Test reception of otDatasetSetActive command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_dataset, test_otDatasetSetActive)
{
	const otOperationalDataset *expected_dataset;
	uint8_t net_key[] = {INT_SEQUENCE(OT_NETWORK_KEY_SIZE)};
	uint8_t nwk_name[] = {INT_SEQUENCE(OT_NETWORK_NAME_MAX_SIZE), 0};
	uint8_t ext_pan_id[] = {INT_SEQUENCE(OT_EXT_PAN_ID_SIZE)};
	uint8_t local_prefix[] = {INT_SEQUENCE(OT_IP6_PREFIX_SIZE)};
	uint8_t pskc[] = {INT_SEQUENCE(OT_PSKC_MAX_SIZE)};

	otDatasetSetActive_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATASET_SET_ACTIVE, DATASET));
	mock_nrf_rpc_tr_expect_done();

	expected_dataset = otDatasetSetActive_fake.arg1_val;

	zassert_equal(otDatasetSetActive_fake.call_count, 1);
	zassert_equal(expected_dataset->mActiveTimestamp.mSeconds, 0x0123456789abcdefull);
	zassert_equal(expected_dataset->mActiveTimestamp.mTicks, 0x1234);
	zassert_false(expected_dataset->mActiveTimestamp.mAuthoritative);

	zassert_equal(expected_dataset->mPendingTimestamp.mSeconds, 0x0123456789abcdefull);
	zassert_equal(expected_dataset->mPendingTimestamp.mTicks, 0x1234);
	zassert_false(expected_dataset->mPendingTimestamp.mAuthoritative);

	zassert_mem_equal(expected_dataset->mNetworkKey.m8, net_key, OT_NETWORK_KEY_SIZE);
	zassert_mem_equal(expected_dataset->mNetworkName.m8, nwk_name,
			  OT_NETWORK_NAME_MAX_SIZE + 1);
	zassert_mem_equal(expected_dataset->mExtendedPanId.m8, ext_pan_id, OT_EXT_PAN_ID_SIZE);
	zassert_mem_equal(expected_dataset->mMeshLocalPrefix.m8, local_prefix, OT_IP6_PREFIX_SIZE);
	zassert_equal(expected_dataset->mDelay, 0x12345678ul);
	zassert_equal(expected_dataset->mPanId, 0xabcd);
	zassert_equal(expected_dataset->mChannel, 0xef67);
	zassert_mem_equal(expected_dataset->mPskc.m8, pskc, OT_PSKC_MAX_SIZE);

	zassert_equal(expected_dataset->mSecurityPolicy.mRotationTime, 0x9876);
	zassert_true(!!expected_dataset->mSecurityPolicy.mObtainNetworkKeyEnabled);
	zassert_false(!!expected_dataset->mSecurityPolicy.mNativeCommissioningEnabled);
	zassert_true(!!expected_dataset->mSecurityPolicy.mRoutersEnabled);
	zassert_false(!!expected_dataset->mSecurityPolicy.mExternalCommissioningEnabled);
	zassert_true(!!expected_dataset->mSecurityPolicy.mCommercialCommissioningEnabled);
	zassert_false(!!expected_dataset->mSecurityPolicy.mAutonomousEnrollmentEnabled);
	zassert_true(!!expected_dataset->mSecurityPolicy.mNetworkKeyProvisioningEnabled);
	zassert_false(!!expected_dataset->mSecurityPolicy.mTobleLinkEnabled);
	zassert_true(!!expected_dataset->mSecurityPolicy.mNonCcmRoutersEnabled);
	zassert_equal(expected_dataset->mSecurityPolicy.mVersionThresholdForRouting, 7);

	zassert_equal(expected_dataset->mChannelMask, 0xfedcba98ul);

	zassert_false(expected_dataset->mComponents.mIsActiveTimestampPresent);
	zassert_true(expected_dataset->mComponents.mIsPendingTimestampPresent);
	zassert_false(expected_dataset->mComponents.mIsNetworkKeyPresent);
	zassert_true(expected_dataset->mComponents.mIsNetworkNamePresent);
	zassert_false(expected_dataset->mComponents.mIsExtendedPanIdPresent);
	zassert_true(expected_dataset->mComponents.mIsMeshLocalPrefixPresent);
	zassert_false(expected_dataset->mComponents.mIsDelayPresent);
	zassert_true(expected_dataset->mComponents.mIsPanIdPresent);
	zassert_false(expected_dataset->mComponents.mIsChannelPresent);
	zassert_true(expected_dataset->mComponents.mIsPskcPresent);
	zassert_false(expected_dataset->mComponents.mIsSecurityPolicyPresent);
	zassert_true(expected_dataset->mComponents.mIsChannelMaskPresent);
}

/*
 * Test reception of otDatasetSetActive command with NULL pointer.
 * Test serialization of the result: OT_ERROR_INVALID_ARGS.
 */
ZTEST(ot_rpc_dataset, test_otDatasetSetActive_negative)
{
	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_INVALID_ARGS), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATASET_SET_ACTIVE, CBOR_NULL));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetSetActive_fake.call_count, 0);
}

static otError dataset_get_fake(otInstance *instance, otOperationalDataset *dataset)
{
	otOperationalDataset expected_dataset = {
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

	memcpy(dataset, &expected_dataset, sizeof(otOperationalDataset));

	return OT_ERROR_NONE;
}

/*
 * Test reception of otDatasetGetActive command.
 * Test serialization of the result: dataset.
 */
ZTEST(ot_rpc_dataset, test_otDatasetGetActive)
{
	otDatasetGetActive_fake.custom_fake = dataset_get_fake;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(DATASET), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATASET_GET_ACTIVE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetGetActive_fake.call_count, 1);
}

/*
 * Test reception of otDatasetGetActive command.
 * Test serialization of the result: NULL as dataset has not been found.
 */
ZTEST(ot_rpc_dataset, test_otDatasetGetActive_null)
{
	otDatasetGetActive_fake.return_val = OT_ERROR_NOT_FOUND;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_NULL), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_DATASET_GET_ACTIVE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otDatasetGetActive_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_dataset, NULL, NULL, tc_setup, NULL, NULL);
