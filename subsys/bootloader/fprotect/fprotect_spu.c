/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdint.h>
#include <string.h>
#include <nrf_spu.h>

void fprotect_area(u32_t start, size_t num_pages)
{
	/* Locks down 32kB of memory */
	nrf_spu_flashregion_set(NRF_SPU_S,
			0,
			false,
			NRF_SPU_MEM_PERM_EXECUTE | NRF_SPU_MEM_PERM_READ,
			true);
}
