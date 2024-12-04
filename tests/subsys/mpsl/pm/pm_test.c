/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdio.h>


#include <errno.h>

#include "cmock_policy.h"
#include "cmock_mpsl_pm.h"
#include "cmock_mpsl_pm_config.h"
#include "cmock_mpsl_work.h"
#include "cmock_kernel_minimal_mock.h"

#include <mpsl/mpsl_pm_utils.h>

#define PM_MAX_LATENCY_HCI_COMMANDS_US 499999

#define TIME_TO_REGISTER_EVENT_IN_ZEPHYR_US 1000

/* Mock implementation for mpsl_work_q*/
struct k_work_q mpsl_work_q;

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
	EVENT_FUNC_DELAY_SCHEDULING,
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

void run_test(test_vector_event_t *p_test_vectors, int num_test_vctr)
{
	mpsl_pm_params_t pm_params_initial = {0};

	resetTest(); /* Verify expectations until now. */
	__cmock_pm_policy_latency_request_add_Expect(0, PM_MAX_LATENCY_HCI_COMMANDS_US);
	__cmock_pm_policy_latency_request_add_IgnoreArg_req();

	__cmock_mpsl_pm_init_Expect();

	__cmock_mpsl_pm_params_get_ExpectAnyArgsAndReturn(true);
	__cmock_mpsl_pm_params_get_ReturnThruPtr_p_params(&pm_params_initial);

	mpsl_pm_utils_init();

	for (int i = 0; i < num_test_vctr; i++) {
		test_vector_event_t v = p_test_vectors[i];

		__cmock_mpsl_pm_params_get_ExpectAnyArgsAndReturn(v.pm_params_get_retval);
		__cmock_mpsl_pm_params_get_ReturnThruPtr_p_params(&v.params);

		__cmock_mpsl_pm_low_latency_state_get_IgnoreAndReturn(
			MPSL_PM_LOW_LATENCY_STATE_OFF);
		__cmock_mpsl_pm_low_latency_requested_IgnoreAndReturn(false);

		switch (v.event_func) {
		case EVENT_FUNC_REGISTER:
			__cmock_k_uptime_get_ExpectAndReturn(v.curr_time_ms);
			__cmock_pm_policy_event_register_Expect(0, v.event_time_us);
			__cmock_pm_policy_event_register_IgnoreArg_evt();
			break;
		case EVENT_FUNC_UPDATE:
			__cmock_k_uptime_get_ExpectAndReturn(v.curr_time_ms);
			__cmock_pm_policy_event_update_Expect(0, v.event_time_us);
			__cmock_pm_policy_event_update_IgnoreArg_evt();
			break;
		case EVENT_FUNC_UNREGISTER:
			__cmock_pm_policy_event_unregister_ExpectAnyArgs();
			break;
		case EVENT_FUNC_DELAY_SCHEDULING:
			__cmock_k_uptime_get_ExpectAndReturn(v.curr_time_ms);
			__cmock_K_USEC_ExpectAndReturn(
				v.event_time_us, (k_timeout_t){v.event_time_us + 100});
			__cmock_mpsl_work_schedule_Expect(
				0, (k_timeout_t){v.event_time_us + 100});
			__cmock_mpsl_work_schedule_IgnoreArg_dwork();
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

	resetTest(); /* Verify expectations until now. */
	__cmock_pm_policy_latency_request_add_Expect(0, PM_MAX_LATENCY_HCI_COMMANDS_US);
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
				0, v.low_latency_requested ? 0 : PM_MAX_LATENCY_HCI_COMMANDS_US);
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

void test_init_only(void)
{
	run_test(NULL, 0);
}

void test_no_events(void)
{
	test_vector_event_t test_vectors[] = {
		/* Init then no events*/
		{false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_high_prio_changed_params(void)
{
	test_vector_event_t test_vectors[] = {
		/* Pretend high prio changed parameters while we read them. */
		{false, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1},
		 EVENT_FUNC_NONE, 0, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_latency_request(void)
{
	test_vector_latency_t test_vectors[] = {
		{LATENCY_FUNC_NONE,   MPSL_PM_LOW_LATENCY_STATE_OFF, false,
		 MPSL_PM_LOW_LATENCY_STATE_OFF, MPSL_PM_LOW_LATENCY_STATE_OFF},
		{LATENCY_FUNC_UPDATE, MPSL_PM_LOW_LATENCY_STATE_OFF, true,
		 MPSL_PM_LOW_LATENCY_STATE_REQUESTING, MPSL_PM_LOW_LATENCY_STATE_ON},
		{LATENCY_FUNC_NONE,   MPSL_PM_LOW_LATENCY_STATE_ON,  true,
		 MPSL_PM_LOW_LATENCY_STATE_ON, MPSL_PM_LOW_LATENCY_STATE_ON},
		{LATENCY_FUNC_UPDATE, MPSL_PM_LOW_LATENCY_STATE_ON,  false,
		 MPSL_PM_LOW_LATENCY_STATE_RELEASING, MPSL_PM_LOW_LATENCY_STATE_OFF},
	};
	run_test_latency(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_register_and_deregister_event(void)
{
	test_vector_event_t test_vectors[] = {
		/* Register event. */
		{true, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1},
		 EVENT_FUNC_REGISTER, 10000, 0},
		/* Deregister event. */
		{true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 2},
		 EVENT_FUNC_UNREGISTER, 0, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_register_update_and_deregister_event(void)
{
	test_vector_event_t test_vectors[] = {
		/* Register event. */
		{true, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1},
		 EVENT_FUNC_REGISTER, 10000, 0},
		/* Update event. */
		{true, {15000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 2},
		 EVENT_FUNC_UPDATE, 15000, 0},
		/* Deregister event. */
		{true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 3},
		 EVENT_FUNC_UNREGISTER, 0, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_event_delayed_work(void)
{
	uint64_t event_time_us = (uint64_t)UINT32_MAX + 50000;
	/* Make sure time until event will be more than UINT32_MAX cycles away. */
	TEST_ASSERT_GREATER_THAN_INT64(UINT32_MAX, k_us_to_cyc_floor64(event_time_us));

	test_vector_event_t test_vectors[] = {
		/* Event time after 32 bit cycles have wrapped, so schedule retry. */
		{true, {event_time_us, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1},
		 EVENT_FUNC_DELAY_SCHEDULING, event_time_us - 1000, 0},
		/* Time has progressed until cycles will no longer wrap,
		 * so register latency normally.
		 */
		{true, {event_time_us, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 2},
		 EVENT_FUNC_REGISTER, event_time_us, 100},
		/* Deregister event. */
		{true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 3},
		 EVENT_FUNC_UNREGISTER, 0, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

int main(void)
{
	(void)unity_main();

	return 0;
}
