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

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATA_SET_ACTIVE_TLVS, TLVS),
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

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATA_GET_ACTIVE_TLVS), RPC_RSP(TLVS));
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

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_DATA_GET_ACTIVE_TLVS), RPC_RSP(CBOR_NULL));
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

ZTEST_SUITE(ot_rpc_commissioning_cli, NULL, NULL, tc_setup, NULL, NULL);
