/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <caf/events/sensor_data_aggregator_event.h>

#define MODULE workload_sim
#include <caf/events/module_state_event.h>

#define BUSY_TIME CONFIG_APP_WORKLOAD_TIME_PER_SAMPLE
#define WORKLOAD_SIMULATED_THREAD_STACK_SIZE 800
#define WORKLOAD_SIMULATED_THREAD_PRIORITY (K_LOWEST_THREAD_PRIO - 2)
#define NUMBER_OF_WORKLOADS DT_PROP(DT_NODELABEL(agg0), buf_count)

BUILD_ASSERT(WORKLOAD_SIMULATED_THREAD_PRIORITY >= 0);

LOG_MODULE_REGISTER(MODULE);

struct workload {
	struct sensor_value *samples;
	struct k_work work;
	uint8_t sample_cnt;
	uint8_t values_in_sample;
	atomic_t busy;
	const char *sensor_descr;
};

static K_THREAD_STACK_DEFINE(workload_simulated_thread_stack,
			     WORKLOAD_SIMULATED_THREAD_STACK_SIZE);
static struct k_work_q workload_simulated_work_q;

static struct workload workloads[NUMBER_OF_WORKLOADS];

static void simulated_work_handler(struct k_work *work)
{
	struct workload *workload = CONTAINER_OF(work, struct workload, work);

	/* Simulate processing of received data. */
	k_busy_wait(BUSY_TIME * workload->sample_cnt);
	struct sensor_data_aggregator_release_buffer_event *release_evt =
		new_sensor_data_aggregator_release_buffer_event();
	release_evt->samples = workload->samples;
	release_evt->sensor_descr = workload->sensor_descr;
	atomic_clear_bit(&workload->busy, 0);
	APP_EVENT_SUBMIT(release_evt);
}

static struct workload *get_free_workload(void)
{
	for (size_t i = 0; i < NUMBER_OF_WORKLOADS; i++) {
		if (!atomic_test_and_set_bit(&workloads[i].busy, 0)) {
			return &workloads[i];
		}
	}

	return NULL;
}

static void init(void)
{
	k_work_queue_init(&workload_simulated_work_q);
	k_work_queue_start(&workload_simulated_work_q,
			   workload_simulated_thread_stack,
			   K_THREAD_STACK_SIZEOF(workload_simulated_thread_stack),
			   WORKLOAD_SIMULATED_THREAD_PRIORITY,
			   NULL);

	for (size_t i = 0; i < NUMBER_OF_WORKLOADS; i++) {
		k_work_init(&workloads[i].work, simulated_work_handler);
	}
}

static bool event_handler(const struct app_event_header *aeh)
{
	if (is_sensor_data_aggregator_event(aeh)) {
		const struct sensor_data_aggregator_event *event =
			cast_sensor_data_aggregator_event(aeh);
		struct workload *workload = get_free_workload();

		__ASSERT_NO_MSG(workload);
		workload->samples = event->samples;
		workload->sample_cnt = event->sample_cnt;
		workload->values_in_sample = event->values_in_sample;
		workload->sensor_descr = event->sensor_descr;
		k_work_submit_to_queue(&workload_simulated_work_q, &workload->work);

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			init();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, sensor_data_aggregator_event);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
