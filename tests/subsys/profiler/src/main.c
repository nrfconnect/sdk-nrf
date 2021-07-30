/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <profiler.h>

#define PROFILED_EVENTS_NB 100
#define U_VALUE_START 0
#define S_VALUE_START -50
#define EXAMPLE_STRING "example string"

static uint16_t no_data_event_id;
static uint16_t data_event_id;
static uint16_t big_event_id;

static void profile_data_event(struct log_event_buf *buf)
{
	static uint32_t val1;

	profiler_log_encode_uint32(buf, val1++);
}

static void profile_big_event(struct log_event_buf *buf)
{
	static uint32_t val1 = U_VALUE_START;
	static int32_t val2 = S_VALUE_START;
	static uint16_t val3 = U_VALUE_START;
	static int16_t val4 = S_VALUE_START;
	static uint8_t val5 = U_VALUE_START;
	static int8_t val6 = S_VALUE_START;

	profiler_log_encode_uint32(buf, val1++);
	profiler_log_encode_int32(buf, val2++);
	profiler_log_encode_uint16(buf, val3++);
	profiler_log_encode_int16(buf, val4++);
	profiler_log_encode_uint8(buf, val5++);
	profiler_log_encode_int8(buf, val6++);
	profiler_log_encode_string(buf, EXAMPLE_STRING);
}

static void register_profiler_events(void)
{
	static const char * const data_names[] = {"value1"};
	static const enum profiler_arg data_types[] = {PROFILER_ARG_U32};
	static const char * const big_event_data_names[] = {"value1", "value2", "value3",
							  "value4", "value5", "value6",
							  "string"};
	static const enum profiler_arg big_event_data_types[] = {PROFILER_ARG_U32,
								 PROFILER_ARG_S32,
								 PROFILER_ARG_U16,
								 PROFILER_ARG_S16,
								 PROFILER_ARG_U8,
								 PROFILER_ARG_S8,
								 PROFILER_ARG_STRING};
	no_data_event_id = profiler_register_event_type("no data event", NULL,
							NULL, 0);
	data_event_id = profiler_register_event_type("data event", data_names,
						     data_types, 1);
	big_event_id = profiler_register_event_type("big event", big_event_data_names,
						    big_event_data_types, 7);
}

static uint32_t test_performance_core(void (*profiler_func)(struct log_event_buf *buf),
				      uint16_t event_id)
{
	uint32_t start_time;
	uint32_t elapsed_ticks;
	uint32_t elapsed_time_us;

	/* Profiling no data event */
	start_time = k_cycle_get_32();
	for (size_t i = 0; i < PROFILED_EVENTS_NB; i++) {
		struct log_event_buf buf;

		profiler_log_start(&buf);
		if (profiler_func) {
			(*profiler_func)(&buf);
		}
		profiler_log_send(&buf, event_id);
	}
	elapsed_ticks = k_cycle_get_32() - start_time;
	elapsed_time_us = k_cyc_to_us_near32(elapsed_ticks);
	return elapsed_time_us;
}

static void test_init(void)
{
	zassert_ok(profiler_init(), "Error when initializing");
	register_profiler_events();
}

static void test_performance1(void)
{
	/* Profiling events with no data */
	uint32_t elapsed_time_us = test_performance_core(NULL, no_data_event_id);

	printk("Logged %d events with no data.\nElapsed time [us]: %d\n",
	       PROFILED_EVENTS_NB, elapsed_time_us);
}

static void test_performance2(void)
{
	uint32_t elapsed_time_us = test_performance_core(profile_data_event, data_event_id);

	printk("Logged %d events with 4-byte data.\nElapsed time [us]: %d\n",
	       PROFILED_EVENTS_NB, elapsed_time_us);
}

static void test_performance3(void)
{
	uint32_t elapsed_time_us = test_performance_core(profile_big_event, big_event_id);

	printk("Logged %d events with 14-byte data and 14-character string.\nElapsed time [us]: %d\n",
	       PROFILED_EVENTS_NB, elapsed_time_us);
}

void test_main(void)
{
	ztest_test_suite(profiler_tests,
			 ztest_unit_test(test_init),
			 ztest_unit_test(test_performance1),
			 ztest_unit_test(test_performance2),
			 ztest_unit_test(test_performance3)
			 );

	ztest_run_test_suite(profiler_tests);
}
