/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdint.h>
#include <string.h>
#include <hal/nrf_spu.h>
#include <errno.h>

#define SPU_LOCK_REGION_SIZE (32 * 1024)

int fprotect_area(u32_t start, size_t length)
{
	if (start % SPU_LOCK_REGION_SIZE != 0 ||
		length % SPU_LOCK_REGION_SIZE != 0) {
		return -EINVAL;
	}

	for (u32_t i = 0; i < length / SPU_LOCK_REGION_SIZE; i++) {
		nrf_spu_flashregion_set(NRF_SPU_S,
				(start / SPU_LOCK_REGION_SIZE) + i,
				true,
				NRF_SPU_MEM_PERM_EXECUTE |
				NRF_SPU_MEM_PERM_READ,
				true);
	}

	return 0;
}
