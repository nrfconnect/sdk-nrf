/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_cpu_run.h>
#include <zephyr/logging/log.h>
#include <drivers/nrfx_common.h>

#ifdef CONFIG_SDFW_VPRS
#include <sdfw/vprs.h>
#endif /* CONFIG_SDFW_VPRS */

#ifdef CONFIG_SDFW_RESET_HANDLING_ENABLED
#include <sdfw/reset_mgr.h>
#endif /* CONFIG_SDFW_RESET_HANDLING_ENABLED */

LOG_MODULE_REGISTER(suit_plat_run, CONFIG_SUIT_LOG_LEVEL);

suit_plat_err_t suit_plat_cpu_run(uint8_t cpu_id, uintptr_t run_address)
{
	int ret = 0;

	switch (cpu_id) {
	case NRF_PROCESSOR_APPLICATION: /* AppCore */
	case NRF_PROCESSOR_RADIOCORE:	/* RadioCore */
#ifdef CONFIG_SDFW_RESET_HANDLING_ENABLED
		LOG_INF("Starting Cortex core %d from address 0x%lx", cpu_id, run_address);
		/* Single run address implies no NSVTOR, so keep at reset value of 0x0. */
		ret = reset_mgr_init_and_boot_processor(cpu_id, run_address, 0);
#else  /* CONFIG_SDFW_RESET_HANDLING_ENABLED */
		LOG_WRN("Cortex core handling not supported - skip CPU %d start", cpu_id);
#endif /* CONFIG_SDFW_RESET_HANDLING_ENABLED */
		break;

	case NRF_PROCESSOR_SYSCTRL: /* SysCtrl */
#ifdef CONFIG_SDFW_VPRS
		LOG_INF("Starting SysCtrl from address 0x%lx", run_address);
		ret = vprs_sysctrl_start((uintptr_t)run_address);
#else  /* CONFIG_SDFW_VPRS */
		LOG_WRN("SysCtrl core handling not supported - skip VPR start");
#endif /* CONFIG_SDFW_VPRS */
		break;

	case NRF_PROCESSOR_PPR:	 /* PPR(VPR) */
	case NRF_PROCESSOR_FLPR: /* FLPR(VPR) */
		LOG_ERR("Application VPR %d start is not supported in SUIT", cpu_id);
		return SUIT_PLAT_ERR_UNSUPPORTED;

	default:
		LOG_ERR("Unsupported CPU ID: %d", cpu_id);
		return SUIT_PLAT_ERR_INVAL;
	}

	if (ret != 0) {
		LOG_ERR("Failed to start CPU %d from address 0x%lx (err: %d)", cpu_id, run_address,
			ret);
		return SUIT_PLAT_ERR_CRASH;
	}

	return SUIT_PLAT_SUCCESS;
}
