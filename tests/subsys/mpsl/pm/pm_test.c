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

#include <mpsl/mpsl_pm_utils.h>
#include <zephyr/kernel.h>


#define PM_MAX_LATENCY_HCI_COMMANDS_US 4999999
#define MAX_DELAY_SINCE_READING_PARAMS_US 50

#define TIME_TO_REGISTER_EVENT_IN_ZEPHYR_US 1000
#define RETRY_TIME_MAX_US (UINT32_MAX - TIME_TO_REGISTER_EVENT_IN_ZEPHYR_US)

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
	LATENCY_FUNC_REGISTER,
	LATENCY_FUNC_UPDATE,
} latency_func_t;

typedef struct {
	bool new_test;
	bool pm_params_get_retval;
	mpsl_pm_params_t params;
	event_func_t event_func;
	uint64_t event_time_us;
	latency_func_t latency_func;
	uint64_t latency_us;
} test_vector_t;

void run_test(test_vector_t *p_test_vectors, int num_test_vctr)
{
	for (int i = 0; i < num_test_vctr; i++) {
		test_vector_t v = p_test_vectors[i];

		if (v.new_test) {
			resetTest(); /* Verify expectations until now. */
			__cmock_pm_policy_latency_request_add_Expect(0, v.latency_us);
			__cmock_pm_policy_latency_request_add_IgnoreArg_req();

			__cmock_mpsl_pm_init_Expect();

			__cmock_mpsl_pm_params_get_ExpectAnyArgsAndReturn(true);
			__cmock_mpsl_pm_params_get_ReturnThruPtr_p_params(&v.params);

			mpsl_pm_utils_init();
		} else {
			__cmock_mpsl_pm_params_get_ExpectAnyArgsAndReturn(v.pm_params_get_retval);
			__cmock_mpsl_pm_params_get_ReturnThruPtr_p_params(&v.params);

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
			case EVENT_FUNC_DELAY_SCHEDULING:
				__cmock_mpsl_work_schedule_Expect(0, K_USEC(v.event_time_us));
				__cmock_mpsl_work_schedule_IgnoreArg_dwork();
				break;
			case EVENT_FUNC_NONE:
				break;
			}
			switch (v.latency_func) {
			case LATENCY_FUNC_REGISTER:
				__cmock_pm_policy_latency_request_add_Expect(0, v.latency_us);
				__cmock_pm_policy_latency_request_add_IgnoreArg_req();
				break;
			case LATENCY_FUNC_UPDATE:
				__cmock_pm_policy_latency_request_update_Expect(0, v.latency_us);
				__cmock_pm_policy_latency_request_update_IgnoreArg_req();
				break;
			case LATENCY_FUNC_NONE:
				break;
			}
			mpsl_pm_utils_work_handler();
		}
	}
}

void test_init_only(void)
{
	test_vector_t test_vectors[] = {
		{true, false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_REGISTER, PM_MAX_LATENCY_HCI_COMMANDS_US},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_no_events(void)
{
	test_vector_t test_vectors[] = {
		/* Init then no events*/
		{true, false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_REGISTER, PM_MAX_LATENCY_HCI_COMMANDS_US},
		{false, false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_NONE, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_high_prio_changed_params(void)
{
	test_vector_t test_vectors[] = {
		/* Init. */
		{true, false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_REGISTER, PM_MAX_LATENCY_HCI_COMMANDS_US},
		/* Pretend high prio changed parameters while we read them. */
		{false, false, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_NONE, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_latency_request(void)
{
	test_vector_t test_vectors[] = {
		/* Init. */
		{true, false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_REGISTER, PM_MAX_LATENCY_HCI_COMMANDS_US},
		/* Check low-latency is set. */
		{false, true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 1},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_NONE, 0},
		/* Set zero-latency. */
		{false, true, {0, MPSL_PM_EVENT_STATE_IN_EVENT, 2},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_UPDATE, 0},
		/* Set low-latency. */
		{false, true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 3},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_UPDATE, PM_MAX_LATENCY_HCI_COMMANDS_US},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_register_and_derigster_event(void)
{
	test_vector_t test_vectors[] = {
		/* Init. */
		{true, false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_REGISTER, PM_MAX_LATENCY_HCI_COMMANDS_US},
		/* Register event. */
		{false, true, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1},
		 EVENT_FUNC_REGISTER, 10000 - MAX_DELAY_SINCE_READING_PARAMS_US,
		 LATENCY_FUNC_NONE, 0},
		/* Deregister event. */
		{false, true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 2},
		 EVENT_FUNC_UNREGISTER, 0,
		 LATENCY_FUNC_NONE, 0},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_register_enter_and_derigster_event(void)
{
	test_vector_t test_vectors[] = {
		/* Init. */
		{true, false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_REGISTER, PM_MAX_LATENCY_HCI_COMMANDS_US},
		 /* Register event. */
		{false, true, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1},
		 EVENT_FUNC_REGISTER, 10000 - MAX_DELAY_SINCE_READING_PARAMS_US,
		 LATENCY_FUNC_NONE, 0},
		/* Pretend to be in event */
		{false, true, {0, MPSL_PM_EVENT_STATE_IN_EVENT, 2},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_UPDATE, 0},
		/* Deregister event. */
		{false, true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 3},
		 EVENT_FUNC_UNREGISTER, 0,
		 LATENCY_FUNC_UPDATE, PM_MAX_LATENCY_HCI_COMMANDS_US},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_register_update_enter_and_deregister_event(void)
{
	test_vector_t test_vectors[] = {
		/* Init. */
		{true, false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_REGISTER, PM_MAX_LATENCY_HCI_COMMANDS_US},
		/* Register event. */
		{false, true, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1},
		 EVENT_FUNC_REGISTER, 10000 - MAX_DELAY_SINCE_READING_PARAMS_US,
		 LATENCY_FUNC_NONE, 0},
		/* Update event. */
		{false, true, {15000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 2},
		 EVENT_FUNC_UPDATE, 15000 - MAX_DELAY_SINCE_READING_PARAMS_US,
		 LATENCY_FUNC_NONE, 0},
		/* Pretend to be in event */
		{false, true, {0, MPSL_PM_EVENT_STATE_IN_EVENT, 3},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_UPDATE, 0},
		/* Deregister event. */
		{false, true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 4},
		 EVENT_FUNC_UNREGISTER, 0,
		 LATENCY_FUNC_UPDATE, PM_MAX_LATENCY_HCI_COMMANDS_US},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_register_enter_and_update_event(void)
{
	test_vector_t test_vectors[] = {
		/* Init. */
		{true, false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_REGISTER, PM_MAX_LATENCY_HCI_COMMANDS_US},
		/* Register event. */
		{false, true, {10000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1},
		 EVENT_FUNC_REGISTER, 10000 - MAX_DELAY_SINCE_READING_PARAMS_US,
		 LATENCY_FUNC_NONE, 0},
		/* Pretend to be in event */
		{false, true, {0, MPSL_PM_EVENT_STATE_IN_EVENT, 2},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_UPDATE, 0},
		/* Update event (before we get the state no events left). */
		{false, true, {15000, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 3},
		 EVENT_FUNC_UPDATE, 15000 - MAX_DELAY_SINCE_READING_PARAMS_US,
		 LATENCY_FUNC_UPDATE, PM_MAX_LATENCY_HCI_COMMANDS_US},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

void test_event_delayed_work(void)
{
	uint64_t retry_evt_time = (uint64_t)UINT32_MAX + (uint64_t)UINT32_MAX + 5000;
	test_vector_t test_vectors[] = {
		/* Init. */
		{true, false, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 0},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_REGISTER, PM_MAX_LATENCY_HCI_COMMANDS_US},
		/* Pretend we call workqueue delayed. */
		{false, true, {retry_evt_time, MPSL_PM_EVENT_STATE_BEFORE_EVENT, 1},
		 EVENT_FUNC_DELAY_SCHEDULING, RETRY_TIME_MAX_US,
		 LATENCY_FUNC_NONE, 0},
		/* Pretend we call workqueue delayed. */
		{false, true, {retry_evt_time - RETRY_TIME_MAX_US,
				MPSL_PM_EVENT_STATE_BEFORE_EVENT, 2},
		 EVENT_FUNC_DELAY_SCHEDULING, RETRY_TIME_MAX_US,
		 LATENCY_FUNC_NONE, 0},
		/* Register event. */
		{false, true, {retry_evt_time - RETRY_TIME_MAX_US - RETRY_TIME_MAX_US,
				MPSL_PM_EVENT_STATE_BEFORE_EVENT, 3},
		 EVENT_FUNC_REGISTER, retry_evt_time - RETRY_TIME_MAX_US -
				      RETRY_TIME_MAX_US - MAX_DELAY_SINCE_READING_PARAMS_US,
		 LATENCY_FUNC_NONE, 0},
		/* Pretend to be in event */
		{false, true, {0, MPSL_PM_EVENT_STATE_IN_EVENT, 4},
		 EVENT_FUNC_NONE, 0,
		 LATENCY_FUNC_UPDATE, 0},
		/* Deregister event. */
		{false, true, {0, MPSL_PM_EVENT_STATE_NO_EVENTS_LEFT, 5},
		 EVENT_FUNC_UNREGISTER, 0,
		 LATENCY_FUNC_UPDATE, PM_MAX_LATENCY_HCI_COMMANDS_US},
	};
	run_test(&test_vectors[0], ARRAY_SIZE(test_vectors));
}

int main(void)
{
	(void)unity_main();

	return 0;
}
