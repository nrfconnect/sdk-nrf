/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <nrfx_ipc.h>
#include "common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(net_core_monitor, CONFIG_NET_CORE_MONITOR_LOG_LEVEL);

#define IPC_MEM_CNT_IDX  0
#define IPC_MEM_REAS_IDX 1

int ncm_net_status_check(uint32_t * const reset_reas)
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
