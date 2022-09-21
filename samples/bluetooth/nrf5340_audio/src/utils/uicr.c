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
/* Memory address to store the channel intended used for this board */
#define MEM_ADDR_UICR_CH (MEM_ADDR_UICR_SNR + sizeof(uint32_t))

uint8_t uicr_channel_get(void)
{
	return *(uint8_t *)MEM_ADDR_UICR_CH;
}

int uicr_channel_set(uint8_t channel)
{
	if (channel == *(uint8_t *)MEM_ADDR_UICR_CH) {
		return 0;
	} else if (*(uint32_t *)MEM_ADDR_UICR_CH != 0xFFFFFFFF) {
		return -EROFS;
	}

	nrfx_nvmc_byte_write(MEM_ADDR_UICR_CH, channel);

	if (channel == *(uint8_t *)MEM_ADDR_UICR_CH) {
		return 0;
	} else {
		return -EIO;
	}
}

uint64_t uicr_snr_get(void)
{
	return *(uint64_t *)MEM_ADDR_UICR_SNR;
}
