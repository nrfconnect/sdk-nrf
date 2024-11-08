/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_cpu_run.h>
#include <zephyr/logging/log.h>

/*
 * Meant to be used in integration tests on posix.
 * May require changes to accommodate other cpu_ids
 */

#define NRF_PROCESSORPOSIX_1 0 /* Secure Domain Processor */
#define NRF_PROCESSORPOSIX_2 1 /* Application Core Processor */

LOG_MODULE_REGISTER(suit_plat_run, CONFIG_SUIT_LOG_LEVEL);

suit_plat_err_t suit_plat_cpu_halt(uint8_t cpu_id)
{
	switch (cpu_id) {
	case NRF_PROCESSORPOSIX_1:
	case NRF_PROCESSORPOSIX_2:
		LOG_INF("Mock AppCore/RadioCore halt");
		return SUIT_PLAT_SUCCESS;

	default:
		LOG_ERR("Unsupported CPU ID (%d)", cpu_id);
		return SUIT_PLAT_ERR_INVAL;
	}
}

suit_plat_err_t suit_plat_cpu_run(uint8_t cpu_id, uintptr_t run_address)
{
	switch (cpu_id) {
	case NRF_PROCESSORPOSIX_1:
	case NRF_PROCESSORPOSIX_2:
		LOG_INF("Mock AppCore/RadioCore run");
		return SUIT_PLAT_SUCCESS;

	default:
		LOG_ERR("Unsupported CPU ID (%d)", cpu_id);
		return SUIT_PLAT_ERR_INVAL;
	}
}
