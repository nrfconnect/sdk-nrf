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
static K_WORK_DELAYABLE_DEFINE(ncm_work, ncm_work_handler);

static bool once;

static void ncm_work_handler(struct k_work *work)
{
	uint32_t mem = APP_IPC_GPMEM;
	uint32_t reset_cnt = mem >> 16;
	uint32_t cnt = mem & 0xFFFF;

	if (once == false) {
		reset_cnt++;
		reset_cnt &= 0xFFFF;
		once = true;
	}
	cnt++;
	cnt &= 0xFFFF;

	mem = (reset_cnt << 16) | cnt;

	APP_IPC_GPMEM = mem;
	k_work_reschedule(&ncm_work, K_MSEC(CONFIG_NCM_APP_FEEDING_INTERVAL_MSEC));
}

static int net_init(const struct device *unused)
{
	k_work_schedule(&ncm_work, K_NO_WAIT);
	return 0;
}

SYS_INIT(net_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
