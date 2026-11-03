/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "mocks.h"

#include <mock_nrf_rpc_transport.h>
#include <ot_rpc_ids.h>
#include <test_rpc_env.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <openthread/instance.h>

FAKE_VOID_FUNC(state_changed_cb, otChangedFlags, void *);

static void verify_state_changed_cb_locked(otChangedFlags flags, void *ctx)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(ctx);

	zassert_true(ot_rpc_is_mutex_locked());
}

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

/* Test serialization of otInstanceInitSingle() */
ZTEST(ot_rpc_instance, test_otInstanceInitSingle_0)
{
	otInstance *instance;
	otInstance *instance2;

	/* Verify a non-null instance is returned from the function. */
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_INIT_SINGLE), RPC_RSP());
	instance = otInstanceInitSingle();
	mock_nrf_rpc_tr_expect_done();

	zassert_not_null(instance, NULL);

	/* Verify that the same instance is returned for subsequent calls. */
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_INIT_SINGLE), RPC_RSP());
	instance2 = otInstanceInitSingle();
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(instance, instance2);
}

/* Test serialization of otInstanceGetId() returning 0 */
ZTEST(ot_rpc_instance, test_otInstanceGetId_0)
{
	uint32_t id;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_GET_ID), RPC_RSP(0));
	id = otInstanceGetId(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(id, 0);
}

/* Test serialization of otInstanceGetId() returning max allowed UINT32_MAX */
ZTEST(ot_rpc_instance, test_otInstanceGetId_max)
{
	uint32_t id;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_GET_ID),
				   RPC_RSP(CBOR_UINT32(UINT32_MAX)));
	id = otInstanceGetId(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(id, UINT32_MAX);
}

/* Test serialization of otInstanceIsInitialized() returning false */
ZTEST(ot_rpc_instance, test_otInstanceIsInitialized_false)
{
	bool initialized;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_IS_INITIALIZED),
				   RPC_RSP(CBOR_FALSE));
	initialized = otInstanceIsInitialized(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_false(initialized);
}

/* Test serialization of otInstanceIsInitialized() returning true */
ZTEST(ot_rpc_instance, test_otInstanceIsInitialized_true)
{
	bool initialized;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_IS_INITIALIZED), RPC_RSP(CBOR_TRUE));
	initialized = otInstanceIsInitialized(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_true(initialized);
}

/* Test serialization of otInstanceFinalize() */
ZTEST(ot_rpc_instance, test_otInstanceFinalize)
{
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_FINALIZE), RPC_RSP());
	otInstanceFinalize(NULL);
	mock_nrf_rpc_tr_expect_done();
}

/* Test serialization of otInstanceErasePersistentInfo() returning success */
ZTEST(ot_rpc_instance, test_otInstanceErasePersistentInfo_ok)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_ERASE_PERSISTENT_INFO),
				   RPC_RSP(OT_ERROR_NONE));
	error = otInstanceErasePersistentInfo(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_NONE);
}

/* Test serialization of otInstanceErasePersistentInfo() returning error */
ZTEST(ot_rpc_instance, test_otInstanceErasePersistentInfo_error)
{
	otError error;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_ERASE_PERSISTENT_INFO),
				   RPC_RSP(OT_ERROR_INVALID_STATE));
	error = otInstanceErasePersistentInfo(NULL);
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(error, OT_ERROR_INVALID_STATE);
}

/* Test serialization of otInstanceFactoryReset() */
ZTEST(ot_rpc_instance, test_otInstanceFactoryReset)
{
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_INSTANCE_FACTORY_RESET), RPC_RSP());
	otInstanceFactoryReset(NULL);
	mock_nrf_rpc_tr_expect_done();
}

/* Test serialization of otGetVersionString() */
ZTEST(ot_rpc_instance, test_otGetVersionString)
{
	const char exp_version[] = {VERSION_STR, '\0'};
	const char *version;

	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_GET_VERSION_STRING),
				   RPC_RSP(0x58, 64, VERSION_STR));
	version = otGetVersionString();
	mock_nrf_rpc_tr_expect_done();

	zassert_str_equal(version, exp_version);
}

/* Test state changed callback registration, invocation, and removal */
ZTEST(ot_rpc_instance, test_state_changed_callback)
{
	otError error;
	void *ctx = (void *)0xdeadbeef;
	const otChangedFlags flags = OT_CHANGED_THREAD_ROLE | OT_CHANGED_THREAD_RLOC_ADDED;

	RESET_FAKE(state_changed_cb);
	state_changed_cb_fake.custom_fake = verify_state_changed_cb_locked;

	/* NULL callback is rejected locally without RPC traffic. */
	error = otSetStateChangedCallback(NULL, NULL, ctx);
	zassert_equal(error, OT_ERROR_INVALID_ARGS);

	/* Registering a callback allocates callback ID 1 and sends only the ID. */
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_SET_STATE_CHANGED_CALLBACK, 1),
				   RPC_RSP(OT_ERROR_NONE));
	error = otSetStateChangedCallback(NULL, state_changed_cb, ctx);
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(error, OT_ERROR_NONE);

	/* Receiving OT_RPC_CMD_STATE_CHANGED for callback ID 1 invokes the callback. */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_STATE_CHANGED, 1, CBOR_UINT8(flags)));
	mock_nrf_rpc_tr_expect_done();

	zassert_equal(state_changed_cb_fake.call_count, 1);
	zassert_equal(state_changed_cb_fake.arg0_val, flags);
	zassert_equal(state_changed_cb_fake.arg1_val, ctx);

	/* Receiving OT_RPC_CMD_STATE_CHANGED with an unregistered or out-of-range ID is ignored. */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_STATE_CHANGED, 2, CBOR_UINT8(flags)));
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(state_changed_cb_fake.call_count, 1);

	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(
		RPC_CMD(OT_RPC_CMD_STATE_CHANGED, CBOR_UINT32(0x24040004), CBOR_UINT8(flags)));
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(state_changed_cb_fake.call_count, 1);

	/* Removing an unregistered callback/context pair does not send an RPC command. */
	otRemoveStateChangeCallback(NULL, state_changed_cb, NULL);

	/* Removing the registered callback sends callback ID 1 and frees the entry. */
	mock_nrf_rpc_tr_expect_add(RPC_CMD(OT_RPC_CMD_REMOVE_STATE_CHANGED_CALLBACK, 1),
				   RPC_RSP());
	otRemoveStateChangeCallback(NULL, state_changed_cb, ctx);
	mock_nrf_rpc_tr_expect_done();

	/* Subsequent OT_RPC_CMD_STATE_CHANGED for freed ID 1 does not invoke the callback. */
	mock_nrf_rpc_tr_expect_add(RPC_RSP(), NO_RSP);
	mock_nrf_rpc_tr_receive(RPC_CMD(OT_RPC_CMD_STATE_CHANGED, 1, CBOR_UINT8(flags)));
	mock_nrf_rpc_tr_expect_done();
	zassert_equal(state_changed_cb_fake.call_count, 1);
}

ZTEST_SUITE(ot_rpc_instance, NULL, NULL, tc_setup, NULL, NULL);
