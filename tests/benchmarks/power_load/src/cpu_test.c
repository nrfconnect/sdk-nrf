/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cpu_test.h"

LOG_MODULE_REGISTER(cpu_test, LOG_LEVEL_INF);

extern atomic_t started_threads;

static void cpu_load_thread_worker(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	atomic_inc(&started_threads);

	while (1) {
		k_busy_wait(CPU_ACTIVE_TIME_US);
		k_msleep(CPU_THREAD_SLEEP);
	}
}

K_THREAD_DEFINE(cpu_load_thread, CPU_THREAD_STACKSIZE, cpu_load_thread_worker, NULL, NULL, NULL,
		K_PRIO_PREEMPT(CPU_THREAD_PRIORITY), 0, 0);
