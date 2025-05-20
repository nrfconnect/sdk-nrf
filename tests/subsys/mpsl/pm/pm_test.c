/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdio.h>

#include <errno.h>
#include <zephyr/kernel.h>

#include "cmock_policy.h"
#include "cmock_mpsl_pm.h"
#include "cmock_mpsl_pm_config.h"
#include "cmock_mpsl_work.h"

#include "mpsl_pm_utils.h"
#include "nrf_errno.h"

#define NO_RADIO_EVENT_PERIOD_LATENCY_US CONFIG_MPSL_PM_NO_RADIO_EVENT_PERIOD_LATENCY_US
/* The unity_main is not declared in any header file. It is only defined in the generated test
 * runner because of ncs' unity configuration. It is therefore declared here to avoid a compiler
 * warning.
 */
extern int unity_main(void);

typedef enum {
	EVENT_FUNC_NONE,
	EVENT_FUNC_REGISTER,
	EVENT_FUNC_UPDATE,
	EVENT_FUNC_UNREGISTER,
} event_func_t;

typedef enum {
	LATENCY_FUNC_NONE,
	LATENCY_FUNC_UPDATE,
} latency_func_t;

typedef struct {
	bool pm_params_get_retval;
	mpsl_pm_params_t params;
	event_func_t event_func;
	uint64_t event_time_us;
	int64_t curr_time_ms;
} test_vector_event_t;

typedef struct {
	latency_func_t latency_func;
	mpsl_pm_low_latency_state_t low_latency_state_prev;
	bool low_latency_requested;
	mpsl_pm_low_latency_state_t low_latency_state_transition;
	mpsl_pm_low_latency_state_t low_latency_state_next;
} test_vector_latency_t;

/* Manual teardown tested module, there is no way to overload it for given test case. */
void uninit_tested_module(void)
{
	__cmock_mpsl_pm_uninit_Expect();

	__cmock_pm_policy_latency_request_remove_Expect(0);
	__cmock_pm_policy_latency_request_remove_IgnoreArg_req();
	__cmock_pm_policy_event_unregister_Expect(0);
	__cmock_pm_policy_event_unregister_IgnoreArg_evt();

	TEST_ASSERT_EQUAL(0, mpsl_pm_utils_uninit());
}

void init_expect_prepare(void)
{
	mpsl_pm_params_t pm_params_initial = {0};

	__cmock_pm_policy_latency_request_add_Expect(0, NO_RADIO_EVENT_PERIOD_LATENCY_US);
	__cmock_pm_policy_latency_request_add_IgnoreArg_req();

	__cmock_mpsl_pm_init_Expect();

	__cmock_mpsl_pm_params_get_ExpectAnyArgsAndReturn(true);
	__cmock_mpsl_pm_params_get_ReturnThruPtr_p_params(&pm_params_initial);
}

void test_init_uninit(void)
{
	init_expect_prepare();

	TEST_ASSERT_EQUAL(0, mpsl_pm_utils_init());

	uninit_tested_module();
}

void test_init_when_already_initialized(void)
{
	init_expect_prepare();

	TEST_ASSERT_EQUAL(0, mpsl_pm_utils_init());

	TEST_ASSERT_EQUAL(-NRF_EPERM, mpsl_pm_utils_init());

	uninit_tested_module();
}

void test_uninit_when_not_initialized(void)
{
	TEST_ASSERT_EQUAL(-NRF_EPERM, mpsl_pm_utils_uninit());
}

/* Latency and event handling tests */

void run_test(test_vector_event_t *p_test_vectors, int num_test_vctr)
{
	mpsl_pm_params_t pm_params_initial = {0};

	verifyTest(); /* Verify expectations until now. */
	__cmock_pm_policy_latency_request_add_Expect(0, NO_RADIO_EVENT_PERIOD_LATENCY_US);
	__cmock_pm_policy_latency_request_add_IgnoreArg_req();

	__cmock_mpsl_pm_init_Expect();

	__cmock_mpsl_pm_params_get_ExpectAnyArgsAndReturn(true);
	__cmock_mpsl_pm_params_get_ReturnThruPtr_p_params(&pm_params_initial);

	TEST_ASSERT_EQUAL(0, mpsl_pm_utils_init());

	for (int i = 0; i < num_test_vctr; i++) {
		test_vector_event_t v = p_test_vectors[i];

		__cmock_mpsl_pm_params_get_ExpectAnyArgsAndReturn(v.pm_params_get_retval);
		__cmock_mpsl_pm_params_get_ReturnThruPtr_p_params(&v.params);

		__cmock_mpsl_pm_low_latency_state_get_IgnoreAndReturn(
			MPSL_PM_LOW_LATENCY_STATE_OFF);
		__cmock_mpsl_pm_low_latency_requested_IgnoreAndReturn(false);

		switch (v.event_func) {
		case EVENT_FUNC_REGISTER:
			__cmock_pm_policy_event_register_Expect(0, v.event_time_us);
			__cmock_pm_policy_event_register_IgnoreArg_evt();
			break;
		case EVENT_FUNC_UPDATE:
			__cmock_pm_policy_event_update_Expect(0, v.event_time_us);
			__cmock_pm_policy_event_update_IgnoreArg_evt();
			break;
		case EVENT_FUNC_UNREGISTER:
			__cmock_pm_policy_event_unregister_ExpectAnyArgs();
			break;
		case EVENT_FUNC_NONE:
			break;
		}
		mpsl_pm_utils_work_handler();
	}
}

void run_test_latency(test_vector_latency_t *p_test_vectors, int num_test_vctr)
{
	mpsl_pm_params_t pm_params = {0};

	verifyTest(); /* Verify expectations until now. */
	__cmock_pm_policy_latency_request_add_Expect(0, NO_RADIO_EVENT_PERIOD_LATENCY_US);
	__cmock_pm_policy_latency_request_add_IgnoreArg_req();

	__cmock_mpsl_pm_init_Expect();

	__cmock_mpsl_pm_params_get_ExpectAnyArgsAndReturn(true);
	__cmock_mpsl_pm_params_get_ReturnThruPtr_p_params(&pm_params);

	mpsl_pm_utils_init();

	for (int i = 0; i < num_test_vctr; i++) {
		test_vector_latency_t v = p_test_vectors[i];

		__cmock_mpsl_pm_params_get_ExpectAnyArgsAndReturn(true);
		__cmock_mpsl_pm_params_get_ReturnThruPtr_p_params(&pm_params);

		__cmock_mpsl_pm_low_latency_state_get_ExpectAndReturn(v.low_latency_state_prev);

		switch (v.latency_func) {
		case LATENCY_FUNC_UPDATE:
			__cmock_mpsl_pm_low_latency_requested_ExpectAndReturn(
				v.low_latency_requested);
			__cmock_pm_policy_latency_request_update_Expect(
				0, v.low_latency_requested ? 0 : NO_RADIO_EVENT_PERIOD_LATENCY_US);
			__cmock_pm_policy_latency_request_update_IgnoreArg_req();
			__cmock_mpsl_pm_low_latency_state_set_Expect(
				v.low_latency_state_transition);
			__cmock_mpsl_pm_low_latency_state_set_Expect(v.low_latency_state_next);
			break;
		case LATENCY_FUNC_NONE:
			__cmock_mpsl_pm_low_latency_requested_ExpectAndReturn(
				v.low_latency_requested);
			break;
		}
		mpsl_pm_utils_work_handler();
	}
}

void test_no_events(void)
{
	test_vector_event_t test_vectors[] = {
		/* Init then no events*/
		{false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0}, EVENT_FUNC_NONE, 0, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));

	uninit_tested_module();
}

void test_high_prio_changed_params(void)
{
	test_vector_event_t test_vectors[] = {
		/* Pretend high prio changed parameters while we read them. */
		{false, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1}, EVENT_FUNC_NONE, 0, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));

	uninit_tested_module();
}

void test_latency_request(void)
{
	test_vector_latency_t test_vectors[] = {
		{LATENCY_FUNC_NONE, MPSL_PM_LOW_LATENCY_STATE_OFF, false,
		 MPSL_PM_LOW_LATENCY_STATE_OFF, MPSL_PM_LOW_LATENCY_STATE_OFF},
		{LATENCY_FUNC_UPDATE, MPSL_PM_LOW_LATENCY_STATE_OFF, true,
		 MPSL_PM_LOW_LATENCY_STATE_REQUESTING, MPSL_PM_LOW_LATENCY_STATE_ON},
		{LATENCY_FUNC_NONE, MPSL_PM_LOW_LATENCY_STATE_ON, true,
		 MPSL_PM_LOW_LATENCY_STATE_ON, MPSL_PM_LOW_LATENCY_STATE_ON},
		{LATENCY_FUNC_UPDATE, MPSL_PM_LOW_LATENCY_STATE_ON, false,
		 MPSL_PM_LOW_LATENCY_STATE_RELEASING, MPSL_PM_LOW_LATENCY_STATE_OFF},
	};
	run_test_latency(&test_vectors[0], ARRAY_SIZE(test_vectors));

	uninit_tested_module();
}

void test_register_and_deregister_event(void)
{
	test_vector_event_t test_vectors[] = {
		/* Register event. */
		{true, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1}, EVENT_FUNC_REGISTER, 10000, 0},
		/* Deregister event. */
		{true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 2}, EVENT_FUNC_UNREGISTER, 0, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));

	uninit_tested_module();
}

void test_register_update_and_deregister_event(void)
{
	test_vector_event_t test_vectors[] = {
		/* Register event. */
		{true, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1}, EVENT_FUNC_REGISTER, 10000, 0},
		/* Update event. */
		{true, {15000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 2}, EVENT_FUNC_UPDATE, 15000, 0},
		/* Deregister event. */
		{true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 3}, EVENT_FUNC_UNREGISTER, 0, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));

	uninit_tested_module();
}

int main(void)
{
	(void)unity_main();

	return 0;
}
