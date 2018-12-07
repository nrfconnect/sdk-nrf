/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stddef.h>
#include <nrf.h>
#include <nrf_peripherals.h>
#include <misc/util.h>
#include <misc/__assert.h>
#include <errno.h>

#if !defined(NRF_BPROT) && defined(NRF_MPU)
/* Rename nRF51's NRF_MPU to NRF_BPROT as they work the same. */
#define NRF_BPROT NRF_MPU
#define CONFIG0 PROTENSET0
#define CONFIG1 PROTENSET1
#endif

/* The number of CONFIG registers present in the chip. */
#define BPROT_CONFIGS_NUM ceiling_fraction(BPROT_REGIONS_NUM, BITS_PER_LONG)

int fprotect_area(u32_t start, size_t length)
{
	/*  TODO - use BPROT HAL when available. */

	u32_t pagenum_start = start / BPROT_REGIONS_SIZE;
	u32_t pagenum_end   = (start + length) / BPROT_REGIONS_SIZE;

	if ((start % BPROT_REGIONS_SIZE) ||
	    (length % BPROT_REGIONS_SIZE) ||
	    (pagenum_end >= BPROT_REGIONS_NUM) ||
	    (pagenum_end < pagenum_start)) {
		/* start or length isn't aligned with a BPROT region,
		 * or attempting to protect an area that is invalid or outside
		 * flash. */
		return -EINVAL;
	}

	u32_t config_masks[BPROT_CONFIGS_NUM];

	for (u32_t i = 0; i < BPROT_CONFIGS_NUM; i++) {
		config_masks[i] = 0;
	}

	for (u32_t i = pagenum_start; i <= pagenum_end; i++)	{
		config_masks[i / BITS_PER_LONG] |= BIT(i % BITS_PER_LONG);
	}

	/* The bits in the BPROT CONFIG registers cannot be cleared to 0,
	 * so these = are effectively |= */
	NRF_BPROT->CONFIG0 = config_masks[0];
#if BPROT_CONFIGS_NUM > 1
	NRF_BPROT->CONFIG1 = config_masks[1];
#endif
#if BPROT_CONFIGS_NUM > 2
	NRF_BPROT->CONFIG2 = config_masks[2];
#endif
#if BPROT_CONFIGS_NUM > 3
	NRF_BPROT->CONFIG3 = config_masks[3];
#endif
#if BPROT_CONFIGS_NUM > 4
	#warning Flash protection above page 127 not implemented.
#endif
	return 0;
}
