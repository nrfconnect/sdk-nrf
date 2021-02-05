/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <profiler.h>

#define MAX_VAL1 1000

static uint16_t data_event_id;
static uint16_t no_data_event_id;

static void profile_no_data_event(void)
{
	struct log_event_buf buf;

	profiler_log_start(&buf);
	profiler_log_send(&buf, no_data_event_id);
}

static void profile_data_event(uint32_t val1, int32_t val2)
{
	struct log_event_buf buf;

	profiler_log_start(&buf);
	/* Use this function for every data type */
	profiler_log_encode_u32(&buf, val1);
	profiler_log_encode_u32(&buf, val2);
	profiler_log_send(&buf, data_event_id);
}

static void register_profiler_events(void)
{
	static const char *data_names[] = {"value1", "value2"};
	static const enum profiler_arg data_types[] = {PROFILER_ARG_U32,
						       PROFILER_ARG_S32};

	no_data_event_id = profiler_register_event_type("no data event", NULL,
							NULL, 0);
	data_event_id = profiler_register_event_type("data event", data_names,
						     data_types, 2);
}

void main(void)
{
	if (profiler_init()) {
		printk("Profiler failed to initialize\n");
	};
	printk("Profiler initialized\n");
	register_profiler_events();
	printk("Events registered\n");
	uint32_t val1 = 50;
	int32_t val2 = -50;

	/* Periodically changing values and profiling events */
	while (1) {
		val2 = -val2;
		val1++;
		if (val1 > MAX_VAL1) {
			val1 = 0;
		}

		profile_no_data_event();
		k_sleep(K_MSEC(10));
		profile_data_event(val1, val2);
		printk("Events sent\n");
		k_sleep(K_MSEC(500));
	}

}
