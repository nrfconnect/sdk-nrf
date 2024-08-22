/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <nrf_erratas.h>
#include <hal/nrf_rramc.h>

#define RRAMC_REGION_FOR_FPROTECT_DEFAULT_VALUE 0x0000000f
#define RRAMC_REGION_FOR_FPROTECT 4
/* symbols temporarily defined here, since PM can't handle different
 * erase-block-size and fprotect-block-size
 */
#define NRF_RRAM_REGION_SIZE_UNIT 0x400
#define NRF_RRAM_REGION_ADDRESS_RESOLUTION 0x400

#define REGION_SIZE_MAX ((RRAMC_REGION_CONFIG_SIZE_Msk >> RRAMC_REGION_CONFIG_SIZE_Pos) * \
			NRF_RRAM_REGION_SIZE_UNIT)

int fprotect_area(uint32_t start, size_t size)
{
	if (start % NRF_RRAM_REGION_ADDRESS_RESOLUTION ||
	    size % NRF_RRAM_REGION_SIZE_UNIT) {
		/*
		 * We avoid setting up locks from address lower than requested
		 * which occurs due to difference in resolution.
		 * Also we don't want to lock more than requested.
		 */
		return -EINVAL;
	}
	if (size > REGION_SIZE_MAX) {
		return -EINVAL;
	}
	if (nrf_rramc_region_config_raw_get(NRF_RRAMC, RRAMC_REGION_FOR_FPROTECT) !=
	    RRAMC_REGION_FOR_FPROTECT_DEFAULT_VALUE) {
		return -ENOSPC;
	}
	nrf_rramc_region_config_t config = {
		.address = start,
		.permissions =	NRF_RRAMC_REGION_PERM_READ_MASK |
				NRF_RRAMC_REGION_PERM_EXECUTE_MASK,
		.writeonce = true,
		.lock = true,
		.size_kb = size / NRF_RRAM_REGION_SIZE_UNIT,
	};

	nrf_rramc_region_config_set(NRF_RRAMC, RRAMC_REGION_FOR_FPROTECT, &config);

	return 0;
}
