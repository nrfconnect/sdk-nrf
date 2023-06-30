/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <init.h>
#include <errno.h>
#include <kernel.h>
#include <sys/printk.h>

#include <logging/log.h>

#define APP_IPC_GPMEM_S  (*(volatile uint32_t *)(0x5002A610))
#define APP_IPC_GPMEM_NS (*(volatile uint32_t *)(0x4002A610))
#define APP_IPC_GPMEM    APP_IPC_GPMEM_S

LOG_MODULE_REGISTER(net_core_monitor, CONFIG_NET_CORE_MONITOR_LOG_LEVEL);

static void ncm_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(ncm_work, ncm_work_handler);

static uint32_t live_cnt;

static void ncm_work_handler(struct k_work *work)
{
	live_cnt++;

	APP_IPC_GPMEM = live_cnt;
	k_work_reschedule(&ncm_work, K_MSEC(CONFIG_NCM_APP_FEEDING_INTERVAL_MSEC));
}

static int net_init(const struct device *unused)
{
	k_work_schedule(&ncm_work, K_NO_WAIT);
	return 0;
}
SYS_INIT(net_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
