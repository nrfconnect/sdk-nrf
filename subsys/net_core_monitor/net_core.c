/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/types.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <hal/nrf_reset.h>
#include "common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(net_core_monitor, CONFIG_NET_CORE_MONITOR_LOG_LEVEL);

/* General purpose memory IPC of the application core.
 * Access possible only through the memory address.
 */
#define APP_IPC_GPMEM_0_S  ((volatile uint32_t *)(0x5002A610))
#define APP_IPC_GPMEM_0_NS ((volatile uint32_t *)(0x4002A610))
#define APP_IPC_GPMEM      APP_IPC_GPMEM_0_S

static void ncm_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(ncm_work, ncm_work_handler);

static void ncm_work_handler(struct k_work *work)
{
	static uint16_t live_cnt = CNT_INIT_VAL;
	uint32_t gpmem = APP_IPC_GPMEM[IPC_MEM_CNT_IDX];

	live_cnt++;
	gpmem = (gpmem & (~CNT_MSK)) | (live_cnt << CNT_POS);
	APP_IPC_GPMEM[IPC_MEM_CNT_IDX] = gpmem;

	k_work_reschedule(&ncm_work, K_MSEC(CONFIG_NCM_FEEDING_INTERVAL_MSEC));
}

static int reset(void)
{
	uint32_t reas;

	reas = nrf_reset_resetreas_get(NRF_RESET);
	nrf_reset_resetreas_clear(NRF_RESET, reas);

	/* A notification that a core reset has occurred.
	 * And set the non-zero value of the counter.
	 */
	APP_IPC_GPMEM[IPC_MEM_CNT_IDX] = (APP_IPC_GPMEM[IPC_MEM_CNT_IDX] & FLAGS_MSK)
				       | (FLAGS_RESET << FLAGS_POS)
				       | (CNT_INIT_VAL << CNT_POS);

	/* Save the reason for the reset. */
	APP_IPC_GPMEM[IPC_MEM_REAS_IDX] = reas;

	return 0;
}

static int net_init(void)
{
	LOG_DBG("Network Core Monitor Init");
	k_work_schedule(&ncm_work, K_NO_WAIT);
	return 0;
}

SYS_INIT(reset, PRE_KERNEL_1, CONFIG_NCM_RESET_INIT_PRIORITY);
SYS_INIT(net_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
