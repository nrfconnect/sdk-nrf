/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrf_profiler.h>
#include <stdio.h>

#define MAX_VAL1 1000
#define MAX_VAL4 100
#define MAX_BUF_SIZE 256

static uint16_t data_event_id;
static uint16_t no_data_event_id;

static void profile_no_data_event(void)
{
	struct log_event_buf buf;

	nrf_profiler_log_start(&buf);
	nrf_profiler_log_send(&buf, no_data_event_id);
}

static void profile_data_event(uint32_t val1, int32_t val2, int16_t val3,
			       uint8_t val4, const char *string)
{
	struct log_event_buf buf;

	nrf_profiler_log_start(&buf);
	/* Use this function for 32-bit data type */
	nrf_profiler_log_encode_uint32(&buf, val1);
	nrf_profiler_log_encode_int32(&buf, val2);
	/* Use this function for 16-bit data type */
	nrf_profiler_log_encode_int16(&buf, val3);
	/* Use this function for 8-bit data type */
	nrf_profiler_log_encode_uint8(&buf, val4);
	/* Use this function for string data type */
	nrf_profiler_log_encode_string(&buf, string);

	nrf_profiler_log_send(&buf, data_event_id);
}

static void register_nrf_profiler_events(void)
{
	static const char * const data_names[] = {"value1", "value2", "value3", "value4", "string"};
	static const enum nrf_profiler_arg data_types[] = {NRF_PROFILER_ARG_U32,
						    NRF_PROFILER_ARG_S32, NRF_PROFILER_ARG_S16,
						    NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_STRING};

	no_data_event_id = nrf_profiler_register_event_type("no data event", NULL,
							NULL, 0);
	data_event_id = nrf_profiler_register_event_type("data event", data_names,
						     data_types, 5);
}

int main(void)
{
	if (nrf_profiler_init()) {
		printk("Profiler failed to initialize\n");
	};
	printk("Profiler initialized\n");
	register_nrf_profiler_events();
	printk("Events registered\n");
	uint32_t val1 = 50;
	int32_t val2 = -50;
	int16_t val3 = -10;
	uint8_t val4 = 7;

	/* Periodically changing values and profiling events */
	while (1) {
		val2 = -val2;
		val3 = -val3;
		val1++;
		val4++;

		if (val1 > MAX_VAL1) {
			val1 = 0;
		}

		if (val4 > MAX_VAL4) {
			val4 = 0;
		}

		char string[MAX_BUF_SIZE];
		int err = snprintf(string, sizeof(string),
				   "Sum of values is: %d.", val1 + val2 + val3 + val4);

		if ((err < 0) || (err >= sizeof(string))) {
			printk("snprintf error: %d\n", err);
			return 0;
		}

		profile_no_data_event();
		k_sleep(K_MSEC(10));
		profile_data_event(val1, val2, val3, val4, string);
		printk("Events sent\n");
		k_sleep(K_MSEC(500));
	}

}
