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

#ifdef CONFIG_NRF_MODEM_LIB_ON_FAULT_RESET_MODEM
static K_SEM_DEFINE(fault_sem, 0, 1);
#endif

#if CONFIG_NRF_MODEM_LIB_FAULT_STRERROR
const char *nrf_modem_lib_fault_strerror(int reason)
{
	const static struct {
		const char *str;
		uint32_t reason;
	} fault[] = {
	#define FAULT(F) {.str = #F, .reason = F}
		FAULT(NRF_MODEM_FAULT_UNDEFINED),
		FAULT(NRF_MODEM_FAULT_HW_WD_RESET),
		FAULT(NRF_MODEM_FAULT_HARDFAULT),
		FAULT(NRF_MODEM_FAULT_MEM_MANAGE),
		FAULT(NRF_MODEM_FAULT_BUS),
		FAULT(NRF_MODEM_FAULT_USAGE),
		FAULT(NRF_MODEM_FAULT_SECURE_RESET),
		FAULT(NRF_MODEM_FAULT_PANIC_DOUBLE),
		FAULT(NRF_MODEM_FAULT_PANIC_RESET_LOOP),
		FAULT(NRF_MODEM_FAULT_ASSERT),
		FAULT(NRF_MODEM_FAULT_PANIC),
		FAULT(NRF_MODEM_FAULT_FLASH_ERASE),
		FAULT(NRF_MODEM_FAULT_FLASH_WRITE),
		FAULT(NRF_MODEM_FAULT_POFWARN),
		FAULT(NRF_MODEM_FAULT_THWARN),
	};
	for (size_t i = 0; i < ARRAY_SIZE(fault); i++) {
		if (fault[i].reason == reason) {
			return fault[i].str;
		}
	}
	return "<unknown>";
}
#endif /* CONFIG_NRF_MODEM_LIB_FAULT_STRERROR */

#ifndef CONFIG_NRF_MODEM_LIB_ON_FAULT_APPLICATION_SPECIFIC
void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault)
{
#if CONFIG_NRF_MODEM_LIB_FAULT_STRERROR
	LOG_ERR("Modem has crashed, reason 0x%x %s, PC: 0x%x",
		fault->reason,
		nrf_modem_lib_fault_strerror(fault->reason),
		fault->program_counter);
#else
	LOG_ERR("Modem has crashed, reason 0x%x, PC: 0x%x",
		fault->reason, fault->program_counter);
#endif
#if CONFIG_NRF_MODEM_LIB_ON_FAULT_RESET_MODEM
	k_sem_give(&fault_sem);
#endif
}
#endif /* not CONFIG_NRF_MODEM_LIB_ON_FAULT_APPLICATION_SPECIFIC */

#ifdef CONFIG_NRF_MODEM_LIB_ON_FAULT_RESET_MODEM
static void restart_on_fault(void *p1, void *p2, void *p3)
{
	while (true) {
		k_sem_take(&fault_sem, K_FOREVER);
		LOG_INF("Modem has crashed, re-initializing");

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
