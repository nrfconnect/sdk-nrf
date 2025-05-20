/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf.h>
#if defined(NRF_BPROT)
	#include <hal/nrf_bprot.h>
	#define PROTECT nrf_bprot_nvm_blocks_protection_enable
	#define ENABLE_PROTECTION_IN_DEBUG nrf_bprot_nvm_protection_in_debug_set
#elif defined(NRF_MPU)
	#include <hal/nrf_mpu.h>
	/* Rename nRF51's NRF_MPU to NRF_BPROT as they work the same. */
	#define NRF_BPROT NRF_MPU
	#define PROTECT nrf_mpu_nvm_blocks_protection_enable
	#define ENABLE_PROTECTION_IN_DEBUG nrf_mpu_nvm_protection_in_debug_set
#else
	#error Either NRF_BPROT or NRF_MPU must be available to use this file.
#endif
#include <zephyr/sys/util.h>
#include <errno.h>

 /* The number of CONFIG registers present in the chip. */
#define BPROT_CONFIGS_NUM DIV_ROUND_UP(BPROT_REGIONS_NUM, BITS_PER_LONG)
#if defined(CONFIG_SB_BPROT_IN_DEBUG)
#define ENABLE_IN_DEBUG true
#else
#define ENABLE_IN_DEBUG false
#endif


int fprotect_area(uint32_t start, size_t length)
{
	ENABLE_PROTECTION_IN_DEBUG(NRF_BPROT,
				   ENABLE_IN_DEBUG
				   );

	uint32_t block_start = start / BPROT_REGIONS_SIZE;
	uint32_t block_end   = (start + length) / BPROT_REGIONS_SIZE;
	uint32_t block_mask[BPROT_CONFIGS_NUM] = {0};

	BUILD_ASSERT(BPROT_REGIONS_SIZE == CONFIG_FPROTECT_BLOCK_SIZE);

	if ((start % BPROT_REGIONS_SIZE) ||
	    (length % BPROT_REGIONS_SIZE) ||
	    (block_end > BPROT_REGIONS_NUM) ||
	    (block_end < block_start)) {
		/*
		 * start or length isn't aligned with a BPROT region,
		 * or attempting to protect an area that is invalid or outside
		 * flash.
		 */
		return -EINVAL;
	}

	for (uint32_t i = block_start; i < block_end; i++) {
		block_mask[i / BITS_PER_LONG] |= BIT(i % BITS_PER_LONG);
	}

	for (uint32_t i = 0; i < BPROT_CONFIGS_NUM; i++) {
		PROTECT(NRF_BPROT, i, block_mask[i]);
	}

	return 0;
}
