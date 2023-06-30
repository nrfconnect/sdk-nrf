/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <errno.h>
#include <kernel.h>
#include <sys/printk.h>

#include <logging/log.h>
#include <nrfx_ipc.h>

#define IPC_MEM_IDX 0

LOG_MODULE_REGISTER(net_core_monitor, CONFIG_NET_CORE_MONITOR_LOG_LEVEL);

int ncm_net_status_check(void)
{
	uint32_t mem;
	static uint32_t prv_cnt;

	mem = NRF_IPC->GPMEM[0];
	LOG_DBG("mem: 0x%08X", mem);

	uint32_t diff = mem - prv_cnt;

	if (diff == 0 || diff > 3) {
		prv_cnt = 0;
		LOG_ERR("Resetting net");
		NRF_RESET->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Hold;
		k_busy_wait(10);
		NRF_RESET->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Release;
		return -EBUSY;
	}

	prv_cnt = mem;
	return 0;
}
