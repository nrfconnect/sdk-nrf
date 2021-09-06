/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <init.h>
#include <zephyr.h>

#include "rf_proc.h"
#include "timer_proc.h"
#include "comm_proc.h"
#include "periph_proc.h"

#include <logging/log.h>
	LOG_MODULE_REGISTER(phy_tt);

/* size of stack area used by each thread */
#define RF_THREAD_STACKSIZE (1024u)
#define COMM_THREAD_STACKSIZE (1024u)

/* scheduling priority used by each thread */
#define RF_THREAD_PRIORITY (7u)
#define COMM_THREAD_PRIORITY (7u)

void ptt_do_reset_ext(void)
{
	NVIC_SystemReset();
}

static int rf_setup(const struct device *dev)
{
	LOG_INF("RF setup started");

	ARG_UNUSED(dev);

	rf_init();

	return 0;
}

static int setup(const struct device *dev)
{
	LOG_INF("Setup started");

	ARG_UNUSED(dev);

	periph_init();

	comm_init();

	/* initialize ptt library */
	ptt_init(launch_ptt_process_timer, ptt_get_max_time());

	return 0;
}

SYS_INIT(rf_setup, POST_KERNEL, PTT_RF_INIT_PRIORITY);

SYS_INIT(setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

K_THREAD_DEFINE(rf_id, RF_THREAD_STACKSIZE, rf_thread, NULL, NULL, NULL, RF_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(comm_id, COMM_THREAD_STACKSIZE, comm_proc, NULL, NULL, NULL, COMM_THREAD_PRIORITY,
		0, 0);
