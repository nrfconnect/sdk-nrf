/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/types.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <nrfx_ipc.h>
#include "common.h"
#include <helpers/nrfx_reset_reason.h>
#include "net_core_monitor.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(net_core_monitor, CONFIG_NET_CORE_MONITOR_LOG_LEVEL);

#define IPC_MEM_CNT_IDX  0
#define IPC_MEM_REAS_IDX 1

#define NET_CORE_CHECK_INTERVAL_MSEC CONFIG_NCM_FEEDING_INTERVAL_MSEC

static void ncm_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(ncm_work, ncm_work_handler);

static int ncm_net_status_check(uint32_t * const reset_reas)
{
	uint32_t gpmem;
	uint16_t cnt;
	static uint16_t prv_cnt;

	gpmem = nrfx_ipc_gpmem_get(IPC_MEM_CNT_IDX);
	cnt = (uint16_t)((gpmem & CNT_MSK) >> CNT_POS);

	if (gpmem & (FLAGS_RESET << FLAGS_POS)) {
		gpmem &= ~(FLAGS_RESET << FLAGS_POS);
		nrfx_ipc_gpmem_set(IPC_MEM_CNT_IDX, gpmem);

		/* Read the reason for the reset. */
		gpmem = nrfx_ipc_gpmem_get(IPC_MEM_REAS_IDX);
		nrfx_ipc_gpmem_set(IPC_MEM_REAS_IDX, 0);

		if (reset_reas) {
			*reset_reas = gpmem;
		}

		prv_cnt = 0;
		return -EFAULT;
	}

	if (prv_cnt == cnt) {
		return -EBUSY;
	}

	prv_cnt = cnt;
	return 0;
}

static void ncm_work_handler(struct k_work *work)
{
	int ret;
	uint32_t reset_reas;

	ret = ncm_net_status_check(&reset_reas);

	if (ret == -EBUSY) {
		ncm_net_core_event_handler(NCM_EVT_NET_CORE_FREEZE, 0);
	} else if (ret == -EFAULT) {
		ncm_net_core_event_handler(NCM_EVT_NET_CORE_RESET, reset_reas);
	} else {
		/* Nothing to do. */
	}

	k_work_reschedule(&ncm_work, K_MSEC(NET_CORE_CHECK_INTERVAL_MSEC));
}

__weak void ncm_net_core_event_handler(enum ncm_event_type event, uint32_t reset_reas)
{
	switch (event) {
	case NCM_EVT_NET_CORE_RESET:
		LOG_DBG("The network core reset.");
		if (reset_reas & NRF_RESET_RESETREAS_RESETPIN_MASK) {
			LOG_DBG("Reset by pin-reset.");
		} else if (reset_reas & NRF_RESET_RESETREAS_DOG0_MASK) {
			LOG_DBG("Reset by application watchdog timer 0.");
		} else if (reset_reas & NRF_RESET_RESETREAS_SREQ_MASK) {
			LOG_DBG("Reset by soft-reset");
		} else if (reset_reas) {
			LOG_DBG("Reset by a different source (0x%08X).", reset_reas);
		} else {
			/* Nothing to do. */
		}

		break;
	case NCM_EVT_NET_CORE_FREEZE:
		LOG_DBG("The network core is not responding.");
		break;
	}
}

static int app_init(void)
{
	LOG_DBG("Network Core Monitor Init");
	k_work_schedule(&ncm_work, K_MSEC(NET_CORE_CHECK_INTERVAL_MSEC));
	return 0;
}

SYS_INIT(app_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
