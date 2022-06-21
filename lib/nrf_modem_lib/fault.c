/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/nrf_modem_lib_trace.h>

LOG_MODULE_DECLARE(nrf_modem, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

#if CONFIG_NRF_MODEM_LIB_ON_FAULT_DO_NOTHING
void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	LOG_ERR("Modem error: 0x%x, PC: 0x%x", fault_info->reason, fault_info->program_counter);
}
#endif

#if CONFIG_NRF_MODEM_LIB_ON_FAULT_RESET_MODEM

static K_SEM_DEFINE(fault_sem, 0, 1);

void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	LOG_ERR("Modem error: 0x%x, PC: 0x%x", fault_info->reason, fault_info->program_counter);
	k_sem_give(&fault_sem);
}

static void restart_on_fault(void *p1, void *p2, void *p3)
{
	while (true) {
		k_sem_take(&fault_sem, K_FOREVER);
		LOG_INF("Modem has faulted, re-initializing");

		(void)nrf_modem_lib_shutdown();

		/* Be nice, yield to application threads
		 * so that can see -NRF_ESHUTDOWN before
		 * the library is re-initialized.
		 */
		k_yield();

		(void)nrf_modem_lib_init(NORMAL_MODE);
	}
}

#if CONFIG_SIZE_OPTIMIZATIONS
#define STACK_SIZE 768
#else
#define STACK_SIZE 1024
#endif

/* The thread that re-initializes the Modem library should have a lower priority
 * than any other application thread that wants to see the NRF_ESHUTDOWN error
 * when the Modem library is shutdown or the modem has faulted.
 */
K_THREAD_DEFINE(nrf_modem_lib_fault, STACK_SIZE, restart_on_fault, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

#endif /* CONFIG_NRF_MODEM_LIB_ON_FAULT_RESET_MODEM */
