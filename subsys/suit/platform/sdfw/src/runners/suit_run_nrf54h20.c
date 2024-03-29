/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_cpu_run.h>
#include <zephyr/logging/log.h>
#include <suit_platform_internal.h>
#include <drivers/nrfx_common.h>
#include <sdfw/vprs.h>

#ifdef CONFIG_SDFW_RESET_HANDLING_ENABLED
#include <sdfw/reset_mgr.h>
#endif /* CONFIG_SDFW_RESET_HANDLING_ENABLED */

LOG_MODULE_REGISTER(suit_plat_run, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_cpu_run(uint8_t cpu_id, intptr_t run_address)
{
	switch (cpu_id) {
	case NRF_PROCESSOR_APPLICATION: /* AppCore */
	case NRF_PROCESSOR_RADIOCORE: { /* RadioCore */
#ifdef CONFIG_SDFW_RESET_HANDLING_ENABLED
		/* Single run address implies no NSVTOR, so keep at reset value of 0x0. */
		return reset_mgr_init_and_boot_processor(cpu_id, run_address, 0);
#else
		return SUIT_SUCCESS;
#endif /* CONFIG_SDFW_RESET_HANDLING_ENABLED */
	} break;

	case NRF_PROCESSOR_BBPR: { /* BBPR - Baseband Processor */
		LOG_ERR("No implementation for BBPR invoke");
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	} break;

	case NRF_PROCESSOR_SYSCTRL: { /* SysCtrl */
		LOG_INF("Starting SysCtrl from address 0x%lx", run_address);
		return vprs_sysctrl_start((uintptr_t)run_address);
	} break;

	case NRF_PROCESSOR_PPR:	   /* PPR(VPR) */
	case NRF_PROCESSOR_FLPR: { /* FLPR(VPR) */
		LOG_ERR("No implementation for VPR invoke");
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	} break;

	default: {
		LOG_ERR("Unsupported CPU ID");
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}
	}
}
