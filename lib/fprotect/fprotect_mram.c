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
#include <hal/nrf_mramc.h>

#define NRF_MRAM_REGION_SIZE_UNIT         0x400
#define NRF_MRAM_REGION_ADDRESS_RESOLUTION 0x400

#if defined(CONFIG_SOC_NRF7120)
	#define MRAMC_REGION_FOR_FPROTECT               5
	#define MRAMC_REGION_FOR_COMBINED_FPROTECT_MID  4
	#define MRAMC_REGION_FOR_COMBINED_FPROTECT_BOT  3
	BUILD_ASSERT(CONFIG_NRF_MRAM_REGION_ADDRESS_RESOLUTION ==
		NRF_MRAM_REGION_ADDRESS_RESOLUTION,
		"NRF_MRAM_REGION_ADDRESS_RESOLUTION has wrong value");
	BUILD_ASSERT(CONFIG_NRF_MRAM_REGION_SIZE_UNIT == NRF_MRAM_REGION_SIZE_UNIT,
		"NRF_MRAM_REGION_SIZE_UNIT has wrong value");
#else
	#error Unsupported SOC
#endif

#define SINGLE_REGION_SIZE ((MRAMC_REGION_CONFIG_SIZE_Msk >> MRAMC_REGION_CONFIG_SIZE_Pos) * \
			NRF_MRAM_REGION_SIZE_UNIT)

#if defined(CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS)
#define REGION_SIZE_MAX (SINGLE_REGION_SIZE * 3)
#else
#define REGION_SIZE_MAX SINGLE_REGION_SIZE
#endif

static bool region_is_in_use(uint32_t region_idx)
{
	nrf_mramc_region_config_t cfg;

	nrf_mramc_region_config_get(NRF_MRAMC, region_idx, &cfg);
	return cfg.lock || cfg.size != 0;
}

static void configure_region(uint32_t region_idx, uint32_t addr, uint8_t size_kb)
{
	nrf_mramc_region_config_t config = {
		.read       = true,
		.write      = false,
		.execute    = true,
		.secure     = false,
		.write_once = true,
		.lock       = true,
		.size       = size_kb,
	};

	nrf_mramc_region_address_set(NRF_MRAMC, region_idx, addr);
	nrf_mramc_region_config_set(NRF_MRAMC, region_idx, &config);
}

int fprotect_area(uint32_t start, size_t size)
{
	if (start % NRF_MRAM_REGION_ADDRESS_RESOLUTION ||
	    size % NRF_MRAM_REGION_SIZE_UNIT) {
		return -EINVAL;
	}
	if (size > REGION_SIZE_MAX) {
		return -EINVAL;
	}
	if (region_is_in_use(MRAMC_REGION_FOR_FPROTECT)) {
		return -ENOSPC;
	}
#if defined(CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS)
	if (size > SINGLE_REGION_SIZE) {
		if (start != 0) {
			/*
			 * Region 3 has fixed 0x00000000 start address.
			 * Using multiple regions implies this constraint.
			 */
			return -EINVAL;
		}
		if (region_is_in_use(MRAMC_REGION_FOR_COMBINED_FPROTECT_BOT)) {
			return -ENOSPC;
		}
		if (size > 2 * SINGLE_REGION_SIZE) {
			if (region_is_in_use(MRAMC_REGION_FOR_COMBINED_FPROTECT_MID)) {
				return -ENOSPC;
			}
			configure_region(MRAMC_REGION_FOR_COMBINED_FPROTECT_MID,
					 SINGLE_REGION_SIZE,
					 SINGLE_REGION_SIZE / NRF_MRAM_REGION_SIZE_UNIT);
			configure_region(MRAMC_REGION_FOR_COMBINED_FPROTECT_BOT,
					 0,
					 SINGLE_REGION_SIZE / NRF_MRAM_REGION_SIZE_UNIT);
			start = 2 * SINGLE_REGION_SIZE;
			size -= 2 * SINGLE_REGION_SIZE;
		} else {
			configure_region(MRAMC_REGION_FOR_COMBINED_FPROTECT_BOT,
					 0,
					 SINGLE_REGION_SIZE / NRF_MRAM_REGION_SIZE_UNIT);
			start = SINGLE_REGION_SIZE;
			size -= SINGLE_REGION_SIZE;
		}
	}
#endif
	configure_region(MRAMC_REGION_FOR_FPROTECT,
			 start,
			 size / NRF_MRAM_REGION_SIZE_UNIT);

	return 0;
}
