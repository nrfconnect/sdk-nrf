/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "uicr.h"

#include <stdint.h>
#include <errno.h>
#include <nrfx_nvmc.h>

/* Memory address to store segger number of the board */
#define MEM_ADDR_UICR_SNR UICR_APP_BASE_ADDR
/* Memory address to store the location intended to be used for this board */
#define MEM_ADDR_UICR_CH (MEM_ADDR_UICR_SNR + sizeof(uint32_t))

uint32_t uicr_location_get(void)
{
	return *(uint32_t *)MEM_ADDR_UICR_CH;
}

int uicr_location_set(uint32_t channel)
{
	if (channel == *(uint32_t *)MEM_ADDR_UICR_CH) {
		return 0;
	} else if (*(uint32_t *)MEM_ADDR_UICR_CH != 0xFFFFFFFF) {
		return -EROFS;
	}

	nrfx_nvmc_word_write(MEM_ADDR_UICR_CH, channel);

	if (channel == *(uint32_t *)MEM_ADDR_UICR_CH) {
		return 0;
	} else {
		return -EIO;
	}

	while (!nrfx_nvmc_write_done_check()) {
	}
}

uint64_t uicr_snr_get(void)
{
	return *(uint64_t *)MEM_ADDR_UICR_SNR;
}
