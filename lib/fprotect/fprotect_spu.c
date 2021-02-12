/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <hal/nrf_spu.h>
#include <errno.h>
#include <soc.h>
#include <nrf_erratas.h>


#if defined(CONFIG_SOC_NRF5340_CPUAPP) \
	&& defined(CONFIG_NRF5340_CPUAPP_ERRATUM19)
#define SPU_BLOCK_SIZE (nrf53_errata_19() ? 32*1024 : 16*1024)
#else
#define SPU_BLOCK_SIZE CONFIG_FPROTECT_BLOCK_SIZE
#endif

int fprotect_area(uint32_t start, size_t length)
{
	if (start % SPU_BLOCK_SIZE != 0 ||
		length % SPU_BLOCK_SIZE != 0) {
		return -EINVAL;
	}

	for (uint32_t i = 0; i < length / SPU_BLOCK_SIZE; i++) {
		nrf_spu_flashregion_set(NRF_SPU_S,
				(start / SPU_BLOCK_SIZE) + i,
				true,
				NRF_SPU_MEM_PERM_EXECUTE |
				NRF_SPU_MEM_PERM_READ,
				true);
	}

	return 0;
}
