/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <zephyr.h>
#include <stdio.h>
#include <net/nrf_cloud.h>
#include <mock_nrf_cloud_transport.h>
#include "nrf_cloud_fsm.h"

/* This is required since unity_main return int and
 * zephyr expects main not to return any value
 */
extern int unity_main(void);

/* Simple event handler needed for the init function to
 * have a success initialization in the test below
 */
void event_handler(const struct nrf_cloud_evt *const evt)
{
	switch (evt->type) {
	case NRF_CLOUD_EVT_READY:
		printk("Device is connected and ready");
		break;
	case NRF_CLOUD_EVT_ERROR:
		printk("Failed to connect to cloud");
		break;
	default:
		break;
	}
}

/* This function runs after each completed test */
void tearDown(void)
{
	/* Uninit to reset the internal state to idle */
	__wrap_nct_uninit_Expect();
	nrf_cloud_uninit();
}

/* The library can only be initialized when the internal state is
 * idle, so the test verifies that nrf_cloud_init function will
 * return with permission denied error (-EACCES) when not idle
 */
void test_init_not_idle(void)
{
	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	TEST_ASSERT_EQUAL_MESSAGE(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Call nrf_cloud_init twice with the first one
	 * being successful and check if the second one
	 * will fail since the state will not be idle
	 */
	__wrap_nct_init_IgnoreAndReturn(0);
	nrf_cloud_init(&params);

	int ret = nrf_cloud_init(&params);

	TEST_ASSERT_NOT_EQUAL_MESSAGE(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should not be in the idle state after initialization");

	TEST_ASSERT_EQUAL_MESSAGE(-EACCES, ret, "return should be -EACCES when not idle");
}

/* Verify that nrf_cloud_init function will throw an error
 * of invalid argument (-EINVAL) when given null params
 */
void test_init_null_params(void)
{
	TEST_ASSERT_EQUAL_MESSAGE(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(NULL);

	TEST_ASSERT_EQUAL_MESSAGE(-EINVAL, ret, "return should be -EINVAL with NULL params");
}

/* Verify that nrf_cloud_init function will throw an error
 * of invalid argument (-EINVAL) when a null event handler
 * is passed in as part of its parameters
 */
void test_init_null_handler(void)
{
	struct nrf_cloud_init_param params = {
		.event_handler = NULL,
		.client_id = NULL,
	};

	TEST_ASSERT_EQUAL_MESSAGE(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&params);

	TEST_ASSERT_EQUAL_MESSAGE(-EINVAL, ret,
		"return should be -EINVAL with NULL event handler");
}

/* Verify that nrf_cloud_init function will succeed
 * given valid internal state and event handler
 */
void test_init_success(void)
{
	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	TEST_ASSERT_EQUAL_MESSAGE(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	/* Expect nct_init to run successfully */
	__wrap_nct_init_ExpectAndReturn(params.client_id, 0);

	int ret = nrf_cloud_init(&params);

	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "return should be 0 when success");
}

/* Verify that nrf_cloud_init function will fail
 * when the transport routine fails to initialize
 */
void test_init_cloud_transport_fail(void)
{
	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
	};

	/* Ignore and set an error return value for nct_init */
	__wrap_nct_init_IgnoreAndReturn(-ENODEV);

	TEST_ASSERT_EQUAL_MESSAGE(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&params);

	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, ret,
		"return should be non-zero when transport routine init failed");
}

/* Verify that nrf_cloud_init function will fail
 * when full modem FOTA update fails to initialize.
 * Ignored when full modem FOTA update is disabled
 */
void test_init_fota_failed(void)
{
#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
	struct dfu_target_fmfu_fdev fmfu_dev_info = {
		.dev = NULL,
		.offset = 0L,
		.size = 0U,
	};

	struct nrf_cloud_init_param params = {
		.event_handler = event_handler,
		.client_id = NULL,
		.fmfu_dev_inf = &fmfu_dev_info,
	};

	TEST_ASSERT_EQUAL_MESSAGE(STATE_IDLE, nfsm_get_current_state(),
		"nrf_cloud lib should be in the idle state (uninitialized) at the start of test");

	int ret = nrf_cloud_init(&params);

	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, ret,
		"return should be non-zero when FOTA init failed");
#else
	TEST_IGNORE();
#endif
}

void main(void)
{
	(void)unity_main();
}
