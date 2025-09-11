/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <health_monitor.h>

LOG_MODULE_REGISTER(nrf_ps_server_main, CONFIG_NRF_PS_SERVER_LOG_LEVEL);


#define WORK_STACK_SIZE 1024
#define WORK_PRIORITY   K_LOWEST_APPLICATION_THREAD_PRIO

K_THREAD_STACK_DEFINE(hotpot_workqueue_stack, WORK_STACK_SIZE);

struct k_work_q hotpork_workq;

static int init_hotpot_workqueue(void)
{
	k_work_queue_init(&hotpork_workq);

	k_work_queue_start(&hotpork_workq,
			    hotpot_workqueue_stack,
			    K_THREAD_STACK_SIZEOF(hotpot_workqueue_stack),
			    WORK_PRIORITY,
			    NULL);

	health_monitor_init(&hotpork_workq);

	LOG_INF("Initialized application workqueue.");

	return 0;
}

SYS_INIT(init_hotpot_workqueue, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
