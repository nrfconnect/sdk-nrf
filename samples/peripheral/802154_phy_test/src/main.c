/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include "rf_proc.h"
#include "timer_proc.h"
#include "comm_proc.h"
#include "periph_proc.h"
#if defined(CONFIG_APP_RPC)
#include "app_rpc.h"
#endif

#include <zephyr/logging/log.h>
	LOG_MODULE_REGISTER(phy_tt);

/* size of stack area used by each thread */
#define RF_THREAD_STACKSIZE (1024u)
#define COMM_THREAD_STACKSIZE (1024u)

/* scheduling priority used by each thread */
#define RF_THREAD_PRIORITY (7u)
#define COMM_THREAD_PRIORITY (7u)

void ptt_do_reset_ext(void)
{
#if defined(CONFIG_APP_RPC)
	app_system_reboot();
#else
	sys_reboot(SYS_REBOOT_COLD);
#endif
}

static int rf_setup(void)
{
	LOG_INF("RF setup started");


	rf_init();

	return 0;
}

static int setup(void)
{
	LOG_INF("Setup started");


	periph_init();

	comm_init();

	/* initialize ptt library */
	ptt_init(launch_ptt_process_timer, ptt_get_max_time());

	return 0;
}

SYS_INIT(rf_setup, POST_KERNEL, CONFIG_PTT_RF_INIT_PRIORITY);

SYS_INIT(setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

K_THREAD_DEFINE(rf_id, RF_THREAD_STACKSIZE, rf_thread, NULL, NULL, NULL, RF_THREAD_PRIORITY, 0, 0);
