/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "uicr.h"

#include <stdint.h>
#include <errno.h>
#if defined(CONFIG_NRFX_NVMC)
#include <nrfx_nvmc.h>
#elif defined(CONFIG_NRFX_RRAMC)
#include <nrfx_rramc.h>
#endif

/* Memory address to store segger number of the board */
#define MEM_ADDR_UICR_SNR UICR_APP_BASE_ADDR
/* Memory address to store the location intended to be used for this board */
#define MEM_ADDR_UICR_LOC (MEM_ADDR_UICR_SNR + sizeof(uint32_t))

uint32_t uicr_location_get(void)
{
	return *(uint32_t *)MEM_ADDR_UICR_LOC;
}

int uicr_location_set(uint32_t location)
{
	if (location == *(uint32_t *)MEM_ADDR_UICR_LOC) {
		return 0;
	} else if (*(uint32_t *)MEM_ADDR_UICR_LOC != 0xFFFFFFFF) {
		return -EROFS;
	}

#if defined(CONFIG_NRFX_NVMC)
	nrfx_nvmc_word_write(MEM_ADDR_UICR_LOC, location);
#elif defined(CONFIG_NRFX_RRAMC)
	nrfx_rramc_word_write(MEM_ADDR_UICR_LOC, location);
#endif

	if (location == *(uint32_t *)MEM_ADDR_UICR_LOC) {
		return 0;
	} else {
		return -EIO;
	}

#if defined(CONFIG_NRFX_NVMC)
	while (!nrfx_nvmc_write_done_check()) {
	}
#endif
}

uint64_t uicr_snr_get(void)
{
	return *(uint64_t *)MEM_ADDR_UICR_SNR;
}
