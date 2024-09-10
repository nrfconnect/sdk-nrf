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

#include <openthread/instance.h>

/* Instance address used when testing serialization of a function that takes otInstance* */
#define INSTANCE_ADDR UINT32_MAX

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

/* Test serialization of otInstanceInitSingle() returning 0 */
ZTEST(ot_rpc_instance, test_otInstanceInitSingle_0)
{
	otInstance *instance;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_INIT_SINGLE), RPC_RSP(0));
	instance = otInstanceInitSingle();
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(instance, NULL);
}

/* Test serialization of otInstanceInitSingle() returning max allowed 0xffffffff */
ZTEST(ot_rpc_instance, test_otInstanceInitSingle_max)
{
	otInstance *instance;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_INIT_SINGLE),
				   RPC_RSP(CBOR_UINT32(UINT32_MAX)));
	instance = otInstanceInitSingle();
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(instance, (void *)UINT32_MAX);
}

/* Test serialization of otInstanceGetId() returning 0 */
ZTEST(ot_rpc_instance, test_otInstanceGetId_0)
{
	otInstance *instance = (otInstance *)INSTANCE_ADDR;
	uint32_t id;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_GET_ID, CBOR_UINT32(INSTANCE_ADDR)),
				   RPC_RSP(0));
	id = otInstanceGetId(instance);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(id, 0);
}

/* Test serialization of otInstanceGetId() returning max allowed UINT32_MAX */
ZTEST(ot_rpc_instance, test_otInstanceGetId_max)
{
	otInstance *instance = (otInstance *)INSTANCE_ADDR;
	uint32_t id;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_GET_ID, CBOR_UINT32(INSTANCE_ADDR)),
				   RPC_RSP(CBOR_UINT32(UINT32_MAX)));
	id = otInstanceGetId(instance);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(id, UINT32_MAX);
}

/* Test serialization of otInstanceIsInitialized() returning false */
ZTEST(ot_rpc_instance, test_otInstanceIsInitialized_false)
{
	otInstance *instance = (otInstance *)INSTANCE_ADDR;
	bool initialized;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_INSTANCE_IS_INITIALIZED, CBOR_UINT32(INSTANCE_ADDR)),
		RPC_RSP(CBOR_FALSE));
	initialized = otInstanceIsInitialized(instance);
	mock_nrf_rpc_tr_expect_done();

	zassert_false(initialized);
}

/* Test serialization of otInstanceIsInitialized() returning true */
ZTEST(ot_rpc_instance, test_otInstanceIsInitialized_true)
{
	otInstance *instance = (otInstance *)INSTANCE_ADDR;
	bool initialized;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_INSTANCE_IS_INITIALIZED, CBOR_UINT32(INSTANCE_ADDR)),
		RPC_RSP(CBOR_TRUE));
	initialized = otInstanceIsInitialized(instance);
	mock_nrf_rpc_tr_expect_done();

	zassert_true(initialized);
}

/* Test serialization of otInstanceFinalize() */
ZTEST(ot_rpc_instance, test_otInstanceFinalize)
{
	otInstance *instance = (otInstance *)INSTANCE_ADDR;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_INSTANCE_FINALIZE, CBOR_UINT32(INSTANCE_ADDR)), RPC_RSP());
	otInstanceFinalize(instance);
	mock_nrf_rpc_tr_expect_done();
}

/* Test serialization of otInstanceErasePersistentInfo() returning success */
ZTEST(ot_rpc_instance, test_otInstanceErasePersistentInfo_ok)
{
	otInstance *instance = (otInstance *)INSTANCE_ADDR;
	otError error;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_INSTANCE_ERASE_PERSISTENT_INFO, CBOR_UINT32(INSTANCE_ADDR)),
		RPC_RSP(OT_ERROR_NONE));
	error = otInstanceErasePersistentInfo(instance);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otInstanceErasePersistentInfo() returning error */
ZTEST(ot_rpc_instance, test_otInstanceErasePersistentInfo_error)
{
	otInstance *instance = (otInstance *)INSTANCE_ADDR;
	otError error;

	mock_nrf_rpc_tr_expect_add(
		RPC_CMD(OT_RPC_CMD_INSTANCE_ERASE_PERSISTENT_INFO, CBOR_UINT32(INSTANCE_ADDR)),
		RPC_RSP(OT_ERROR_INVALID_STATE));
	error = otInstanceErasePersistentInfo(instance);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_INVALID_STATE);
}

ZTEST_SUITE(ot_rpc_instance, NULL, NULL, tc_setup, NULL, NULL);
