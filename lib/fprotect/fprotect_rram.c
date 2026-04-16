/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <nrfx.h>
#include <hal/nrf_rramc.h>

#define RRAMC_REGION_FOR_FPROTECT_DEFAULT_VALUE NRF_RRAMC_REGION_CONFIG_PERM_MASK
#define NRF_RRAM_REGION_SIZE_UNIT 0x400
#define NRF_RRAM_REGION_ADDRESS_RESOLUTION 0x400

#if defined(CONFIG_SOC_NRF54L15) || defined(CONFIG_SOC_NRF54L10) || defined(CONFIG_SOC_NRF54L05)\
|| defined(CONFIG_SOC_NRF54LM20A) || defined(CONFIG_SOC_NRF54LM20B)				\
|| defined(CONFIG_SOC_NRF54LV10A)
	#define RRAMC_REGION_FOR_FPROTECT 4
	#define RRAMC_REGION_FOR_COMBINED_REGIONS_FPROTECT 3
	BUILD_ASSERT(CONFIG_NRF_RRAM_REGION_ADDRESS_RESOLUTION ==
		NRF_RRAM_REGION_ADDRESS_RESOLUTION,
		"NRF_RRAM_REGION_ADDRESS_RESOLUTION has wrong value");
	BUILD_ASSERT(CONFIG_NRF_RRAM_REGION_SIZE_UNIT == NRF_RRAM_REGION_SIZE_UNIT,
		"NRF_RRAM_REGION_SIZE_UNIT has wrong value");
#elif defined(CONFIG_SOC_NRF54LS05A) || defined(CONFIG_SOC_NRF54LS05B)
	#define RRAMC_REGION_FOR_FPROTECT 4
	#define RRAMC_REGION_FOR_COMBINED_REGIONS_FPROTECT 2
	BUILD_ASSERT(CONFIG_NRF_RRAM_REGION_ADDRESS_RESOLUTION ==
		NRF_RRAM_REGION_ADDRESS_RESOLUTION,
		"NRF_RRAM_REGION_ADDRESS_RESOLUTION has wrong value");
	BUILD_ASSERT(CONFIG_NRF_RRAM_REGION_SIZE_UNIT == NRF_RRAM_REGION_SIZE_UNIT,
		"NRF_RRAM_REGION_SIZE_UNIT has wrong value");
#else
	#error Unsupported SOC
#endif

#define SINGLE_REGION_SIZE ((RRAMC_REGION_CONFIG_SIZE_Msk >> RRAMC_REGION_CONFIG_SIZE_Pos) * \
			NRF_RRAM_REGION_SIZE_UNIT)

#if defined(CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS)
#define REGION_SIZE_MAX SINGLE_REGION_SIZE * 2
#else
#define REGION_SIZE_MAX SINGLE_REGION_SIZE
#endif

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
#if defined(CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS)

	if (size > SINGLE_REGION_SIZE) {
		if (start != 0) {
			/*
			 * Region[3] has fixed 0x00000000 start address.
			 * Using both regions implies this constraint.
			 */
			return -EINVAL;
		}
		if (nrf_rramc_region_config_raw_get(NRF_RRAMC,
			RRAMC_REGION_FOR_COMBINED_REGIONS_FPROTECT) !=
			RRAMC_REGION_FOR_FPROTECT_DEFAULT_VALUE) {
			return -ENOSPC;
		}
		nrf_rramc_region_config_t config = {
			.address = 0,
			.permissions =	NRF_RRAMC_REGION_PERM_READ_MASK |
					NRF_RRAMC_REGION_PERM_EXECUTE_MASK,
			.writeonce = true,
			.lock = true,
			.size_kb = (SINGLE_REGION_SIZE) / NRF_RRAM_REGION_SIZE_UNIT,
		};

		nrf_rramc_region_config_set(NRF_RRAMC, RRAMC_REGION_FOR_COMBINED_REGIONS_FPROTECT,
			&config);
		start = SINGLE_REGION_SIZE;
		size -= SINGLE_REGION_SIZE;
	}
#endif
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
