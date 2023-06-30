/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/types.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>

#define APP_IPC_GPMEM_S  (*(volatile uint32_t *)(0x5002A610))
#define APP_IPC_GPMEM_NS (*(volatile uint32_t *)(0x4002A610))
#define APP_IPC_GPMEM    APP_IPC_GPMEM_S

LOG_MODULE_REGISTER(net_core_monitor, CONFIG_NET_CORE_MONITOR_LOG_LEVEL);

static void ncm_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(ncm_work, ncm_work_handler);

static uint32_t live_cnt;

static void ncm_work_handler(struct k_work *work)
{
	LOG_DBG("Work handler");
	volatile uint32_t tmp = APP_IPC_GPMEM;

	live_cnt++;
	printk("GPMEM: %d live_cnt %d\n", tmp, live_cnt);

	tmp &= ~0xffff;
	tmp |= live_cnt & 0xffff;
	APP_IPC_GPMEM = tmp;
	k_work_reschedule(&ncm_work, K_MSEC(CONFIG_NCM_APP_FEEDING_INTERVAL_MSEC));
}

static int reset(void)
{
	/* A notification that a core reset has occurred. */
	APP_IPC_GPMEM = ((1 << 16) | 0x00ff);

	return 0;
}

static int net_init(void)
{
	LOG_DBG("Net Init");
	k_work_schedule(&ncm_work, K_NO_WAIT);
	return 0;
}

SYS_INIT(reset, PRE_KERNEL_1, CONFIG_RESET_INIT_PRIORITY);
SYS_INIT(net_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
