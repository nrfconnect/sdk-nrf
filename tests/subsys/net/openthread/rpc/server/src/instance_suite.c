/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/net/openthread.h>
#include <zephyr/ztest.h>

#include <openthread/instance.h>

/* Fake functions */

FAKE_VALUE_FUNC(otInstance *, otInstanceInitSingle);
FAKE_VALUE_FUNC(uint32_t, otInstanceGetId, otInstance *);
FAKE_VALUE_FUNC(bool, otInstanceIsInitialized, otInstance *);
FAKE_VOID_FUNC(otInstanceFinalize, otInstance *);
FAKE_VALUE_FUNC(otError, otInstanceErasePersistentInfo, otInstance *);
FAKE_VALUE_FUNC(otError, otSetStateChangedCallback, otInstance *, otStateChangedCallback, void *);
FAKE_VOID_FUNC(otRemoveStateChangeCallback, otInstance *, otStateChangedCallback, void *);

#define FOREACH_FAKE(f)                                                                            \
	f(otInstanceInitSingle);                                                                   \
	f(otInstanceGetId);                                                                        \
	f(otInstanceIsInitialized);                                                                \
	f(otInstanceFinalize);                                                                     \
	f(otInstanceErasePersistentInfo);                                                          \
	f(otSetStateChangedCallback);                                                              \
	f(otRemoveStateChangeCallback);

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
 * Test reception of otInstanceInitSingle() command.
 * Test serialization of the result: NULL.
 */
ZTEST(ot_rpc_instance, test_otInstanceInitSingle_0)
{
	otInstanceInitSingle_fake.return_val = NULL;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_INSTANCE_INIT_SINGLE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otInstanceInitSingle_fake.call_count, 1);
}

/*
 * Test reception of otInstanceInitSingle() command.
 * Test serialization of the result: 0xffffffff.
 */
ZTEST(ot_rpc_instance, test_otInstanceInitSingle_max)
{
	otInstanceInitSingle_fake.return_val = (otInstance *)UINT32_MAX;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT32(UINT32_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_INSTANCE_INIT_SINGLE));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otInstanceInitSingle_fake.call_count, 1);
}

/*
 * Test reception of otInstanceGetId() command.
 * Test serialization of the result: 0.
 */
ZTEST(ot_rpc_instance, test_otInstanceGetId_0)
{
	uintptr_t instance = (uintptr_t)openthread_get_default_instance();

	otInstanceGetId_fake.return_val = 0;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(0), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_INSTANCE_GET_ID, CBOR_UINT32(instance)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otInstanceGetId_fake.call_count, 1);
	zassert_equal(otInstanceGetId_fake.arg0_val, (otInstance *)instance);
}

/*
 * Test reception of otInstanceGetId() command.
 * Test serialization of the result: 0xffffffff.
 */
ZTEST(ot_rpc_instance, test_otInstanceGetId_max)
{
	uintptr_t instance = (uintptr_t)openthread_get_default_instance();

	otInstanceGetId_fake.return_val = 0xffffffff;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_UINT32(UINT32_MAX)), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_INSTANCE_GET_ID, CBOR_UINT32(instance)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otInstanceGetId_fake.call_count, 1);
	zassert_equal(otInstanceGetId_fake.arg0_val, (otInstance *)instance);
}

/*
 * Test reception of otInstanceIsInitialized() command.
 * Test serialization of the result: false.
 */
ZTEST(ot_rpc_instance, test_otInstanceIsInitialized_false)
{
	uintptr_t instance = (uintptr_t)openthread_get_default_instance();

	otInstanceIsInitialized_fake.return_val = false;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_FALSE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_INSTANCE_IS_INITIALIZED, CBOR_UINT32(instance)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otInstanceIsInitialized_fake.call_count, 1);
	zassert_equal(otInstanceIsInitialized_fake.arg0_val, (otInstance *)instance);
}

/*
 * Test reception of otInstanceIsInitialized() command.
 * Test serialization of the result: true.
 */
ZTEST(ot_rpc_instance, test_otInstanceIsInitialized_true)
{
	uintptr_t instance = (uintptr_t)openthread_get_default_instance();

	otInstanceIsInitialized_fake.return_val = true;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(CBOR_TRUE), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_INSTANCE_IS_INITIALIZED, CBOR_UINT32(instance)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otInstanceIsInitialized_fake.call_count, 1);
	zassert_equal(otInstanceIsInitialized_fake.arg0_val, (otInstance *)instance);
}

/*
 * Test reception of otInstanceFinalize() command.
 */
ZTEST(ot_rpc_instance, test_otInstanceFinalize)
{
	uintptr_t instance = (uintptr_t)openthread_get_default_instance();

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_INSTANCE_FINALIZE, CBOR_UINT32(instance)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otInstanceFinalize_fake.call_count, 1);
}

/*
 * Test reception of otInstanceErasePersistentInfo() command.
 * Test serialization of the result: OT_ERROR_NONE.
 */
ZTEST(ot_rpc_instance, test_otInstanceErasePersistentInfo_ok)
{
	uintptr_t instance = (uintptr_t)openthread_get_default_instance();

	otInstanceErasePersistentInfo_fake.return_val = OT_ERROR_NONE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_NONE), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_INSTANCE_ERASE_PERSISTENT_INFO, CBOR_UINT32(instance)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otInstanceErasePersistentInfo_fake.call_count, 1);
	zassert_equal(otInstanceErasePersistentInfo_fake.arg0_val, (otInstance *)instance);
}

/*
 * Test reception of otInstanceErasePersistentInfo() command.
 * Test serialization of the result: OT_ERROR_INVALID_STATE.
 */
ZTEST(ot_rpc_instance, test_otInstanceErasePersistentInfo_error)
{
	uintptr_t instance = (uintptr_t)openthread_get_default_instance();

	otInstanceErasePersistentInfo_fake.return_val = OT_ERROR_INVALID_STATE;

	mock_nrf_rpc_tr_expect_add(RPC_RSP(OT_ERROR_INVALID_STATE), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_INSTANCE_ERASE_PERSISTENT_INFO, CBOR_UINT32(instance)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(otInstanceErasePersistentInfo_fake.call_count, 1);
	zassert_equal(otInstanceErasePersistentInfo_fake.arg0_val, (otInstance *)instance);
}

ZTEST_SUITE(ot_rpc_instance, NULL, NULL, tc_setup, NULL, NULL);
