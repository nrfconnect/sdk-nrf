/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cpu_load_monitor.h"
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>

#define CPU_LOAD_MONITOR_THREAD_STACK_SIZE 4096
#define CPU_LOAD_MONITOR_PERIOD_MS	   25
#define MAX_CPU_LOAD_VALUES_HELD	   32

typedef enum {
	WAIT_FOR_TRIGGER = 0,
	MEASURE_CPU_LOAD = 1,
	CHECK_TERM_SIGNAL = 2
} monitor_state;

static uint32_t cpu_loads[MAX_CPU_LOAD_VALUES_HELD];
static uint32_t average_cpu_load;
static uint32_t peak_cpu_load;

static K_SEM_DEFINE(cpu_load_start_sem, 0, 1);
static K_SEM_DEFINE(cpu_load_stop_sem, 0, 1);
static K_SEM_DEFINE(cpu_load_calc_done_sem, 0, 1);
static K_SEM_DEFINE(cpu_load_thread_terminate_sem, 0, 1);

static void calculate_peak_and_average_cpu_load(uint32_t loads_counter, uint32_t *peak_load,
						uint32_t *average_load)
{
	uint64_t average_buffer = 0;

	*peak_load = 0;

	if (loads_counter == 0U) {
		*average_load = 0U;
		return;
	}

	for (uint32_t i = 0; i < loads_counter; i++) {
		average_buffer += cpu_loads[i];
		if (cpu_loads[i] > *peak_load) {
			*peak_load = cpu_loads[i];
		}
	}

	*average_load = (uint32_t)(average_buffer / loads_counter);
}

static void cpu_load_monitor(void *param1, void *param2, void *param3)
{
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);
	ARG_UNUSED(param3);

	int32_t cpu_load;
	static uint32_t cpu_loads_counter;

	static monitor_state cpu_load_monitor_state = WAIT_FOR_TRIGGER;

	while (1) {
		switch (cpu_load_monitor_state) {
		case WAIT_FOR_TRIGGER:
			if (k_sem_take(&cpu_load_start_sem, K_NO_WAIT) == 0) {
				cpu_load_monitor_state = MEASURE_CPU_LOAD;
				peak_cpu_load = 0;
				average_cpu_load = 0;
				cpu_loads_counter = 0;
			}
			if (k_sem_take(&cpu_load_thread_terminate_sem, K_NO_WAIT) == 0) {
				k_sleep(K_FOREVER);
			}
			k_msleep(1);
			break;

		case MEASURE_CPU_LOAD:
			cpu_load = cpu_load_get(true);
			if (cpu_load < 0) {
				cpu_load = 0;
			}
			if (cpu_loads_counter < MAX_CPU_LOAD_VALUES_HELD) {
				cpu_loads[cpu_loads_counter] = (uint32_t)cpu_load;
				cpu_loads_counter++;
			}
			k_msleep(CPU_LOAD_MONITOR_PERIOD_MS);
			cpu_load_monitor_state = CHECK_TERM_SIGNAL;
			break;

		case CHECK_TERM_SIGNAL:
			if (k_sem_take(&cpu_load_stop_sem, K_NO_WAIT) == 0) {
				cpu_load_monitor_state = WAIT_FOR_TRIGGER;
				calculate_peak_and_average_cpu_load(
					cpu_loads_counter, &peak_cpu_load, &average_cpu_load);
				k_sem_give(&cpu_load_calc_done_sem);
			} else {
				cpu_load_monitor_state = MEASURE_CPU_LOAD;
			}
			break;

		default:
			break;
		}
	}
}

K_THREAD_DEFINE(cpu_load_monitor_thread, CPU_LOAD_MONITOR_THREAD_STACK_SIZE, cpu_load_monitor, NULL,
		NULL, NULL, 3, 0, 0);

void cpu_load_monitor_init(void)
{
	cpu_load_get(false);
}

void cpu_load_monitor_start(void)
{
	k_sem_give(&cpu_load_start_sem);
}

void cpu_load_monitor_stop(void)
{
	k_sem_give(&cpu_load_stop_sem);
}

void cpu_load_monitor_show(void)
{
	k_sem_take(&cpu_load_calc_done_sem, K_FOREVER);
	printk("Measured CPU load:\n");
	printk("Peak CPU load: %u [m%%]\n", peak_cpu_load);
	printk("Average CPU load: %u [m%%]\n", average_cpu_load);
}

void cpu_load_monitor_terminate(void)
{
	k_sem_give(&cpu_load_thread_terminate_sem);
}
