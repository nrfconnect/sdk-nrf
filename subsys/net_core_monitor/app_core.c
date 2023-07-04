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
#include <nrfx_ipc.h>

#define IPC_MEM_IDX 0

LOG_MODULE_REGISTER(net_core_monitor, CONFIG_NET_CORE_MONITOR_LOG_LEVEL);

static uint32_t prev_reset_cnt;

static void net_reset(void)
{
	LOG_WRN("Resetting net");
	NRF_RESET->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Hold;
	k_busy_wait(10);
	NRF_RESET->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Release;
}

int ncm_net_status_check(bool reset_on_failure)
{
	int ret;
	uint32_t mem;
	uint32_t diff;
	static uint32_t prev_cnt;

	mem = nrf_ipc_gpmem_get(NRF_IPC, IPC_MEM_IDX);

	uint32_t cnt = mem & 0xFFFF;
	uint32_t reset_cnt = mem >> 16;

	LOG_DBG("mem: 0x%08X", mem);

	if (reset_cnt != prev_reset_cnt) {
		prev_reset_cnt = reset_cnt;
		ret = -EFAULT;
	} else {
		diff = (cnt - prev_cnt) & BIT_MASK(16);
		if (diff == 0 || diff > 3) {
			if (reset_on_failure) {
				net_reset();
			}
			ret = -EBUSY;
		} else {
			ret = 0;
		}
	}

	prev_cnt = cnt;
	return ret;
}

static int ncm_init(const struct device *unused)
{
	nrf_ipc_gpmem_set(NRF_IPC, IPC_MEM_IDX, 0);
	/* Set to not report the first reset. */
	prev_reset_cnt = 1;
	return 0;
}

SYS_INIT(ncm_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
