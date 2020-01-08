/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdint.h>
#include <string.h>
#include <hal/nrf_spu.h>
#include <errno.h>

int fprotect_area(u32_t start, size_t length)
{
	if (start % CONFIG_FPROTECT_BLOCK_SIZE != 0 ||
		length % CONFIG_FPROTECT_BLOCK_SIZE != 0) {
		return -EINVAL;
	}

	for (u32_t i = 0; i < length / CONFIG_FPROTECT_BLOCK_SIZE; i++) {
		nrf_spu_flashregion_set(NRF_SPU_S,
				(start / CONFIG_FPROTECT_BLOCK_SIZE) + i,
				true,
				NRF_SPU_MEM_PERM_EXECUTE |
				NRF_SPU_MEM_PERM_READ,
				true);
	}

	return 0;
}
