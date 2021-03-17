/*
 * Copyright (c) 2021 Nordic Semiconductor ASA. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <autoconf.h>

#ifdef CONFIG_TFM_ITS_ENCRYPTION

#include <string.h>
#include <Driver_Flash.h>
#include <flash_layout.h>
#include "flash_encryption.h"

#define MIN(a,b) ((a <= b) ? (a) : (b))

bool is_range_valid(uint32_t addr, uint32_t cnt);
ARM_DRIVER_VERSION ARM_Flash_GetVersion(void);
ARM_FLASH_CAPABILITIES ARM_Flash_GetCapabilities(void);
int32_t ARM_Flash_Initialize(ARM_Flash_SignalEvent_t cb_event);
int32_t ARM_Flash_Uninitialize(void);
int32_t ARM_Flash_PowerControl(ARM_POWER_STATE state);
int32_t ARM_Flash_ReadData(uint32_t addr, void *data, uint32_t cnt);
int32_t ARM_Flash_ProgramData(uint32_t addr, const void *data,
                                     uint32_t cnt);
int32_t ARM_Flash_EraseSector(uint32_t addr);
int32_t ARM_Flash_EraseChip(void);
ARM_FLASH_STATUS ARM_Flash_GetStatus(void);
ARM_FLASH_INFO * ARM_Flash_GetInfo(void);

static uint8_t enc_buf[TFM_ENC_PROGRAM_UNIT];

static ARM_FLASH_INFO FlashInfo_Encrypted = {
	.sector_info  = NULL, /* Uniform sector layout */
	.sector_count = FLASH_TOTAL_SIZE / FLASH_AREA_IMAGE_SECTOR_SIZE,
	.sector_size  = FLASH_AREA_IMAGE_SECTOR_SIZE,
	.page_size    = 4, /* 32-bit word = 4 bytes */
	.program_unit = TFM_ENC_PROGRAM_UNIT,
	.erased_value = 0xFF
};

static int32_t ARM_Flash_ReadData_Encrypted(uint32_t addr, void *data, uint32_t cnt)
{
	uint32_t read_addr = (addr / TFM_ENC_PROGRAM_UNIT) * TFM_ENC_PROGRAM_UNIT; /* Round down */
	fenc_ctx_t fenc_ctx;
	int32_t err;

	if (!is_range_valid(addr, cnt)) {
		return ARM_DRIVER_ERROR_PARAMETER;
	}

	err = fenc_prepare(&fenc_ctx);
	if (err != 0) {
		return ARM_DRIVER_ERROR;
	}

	while (read_addr < (addr + cnt)) {
		err = fenc_decrypt(&fenc_ctx, enc_buf, (const uint8_t *)read_addr, read_addr);
		if (err != 0) {
			return ARM_DRIVER_ERROR;
		}

		if (read_addr < addr) {
			uint32_t offs_in_buf = addr - read_addr;
			memcpy(data, &enc_buf[offs_in_buf],
				MIN(TFM_ENC_PROGRAM_UNIT - offs_in_buf, cnt));
		} else {
			memcpy(data + read_addr - addr, enc_buf,
				MIN(TFM_ENC_PROGRAM_UNIT, addr + cnt - read_addr));
		}

		read_addr += TFM_ENC_PROGRAM_UNIT;
	}

	err = fenc_finalize(&fenc_ctx);
	if (err != 0) {
		return ARM_DRIVER_ERROR;
	}

	return ARM_DRIVER_OK;
}

static int32_t ARM_Flash_ProgramData_Encrypted(uint32_t addr, const void *data, uint32_t cnt)
{
	uint32_t write_addr = addr;
	int32_t err;
	fenc_ctx_t fenc_ctx;

	if ((addr & (TFM_ENC_PROGRAM_UNIT - 1)) || (cnt & (TFM_ENC_PROGRAM_UNIT - 1))) {
		return ARM_DRIVER_ERROR_PARAMETER;
	}

	err = fenc_prepare(&fenc_ctx);
	if (err != 0) {
		return ARM_DRIVER_ERROR;
	}

	while (write_addr < (addr + cnt)) {
		err = fenc_encrypt(&fenc_ctx, enc_buf, data, write_addr);
		if (err != 0) {
			return ARM_DRIVER_ERROR;
		}

		if ((err = ARM_Flash_ProgramData(write_addr, enc_buf, TFM_ENC_PROGRAM_UNIT)) != ARM_DRIVER_OK) {
			return ARM_DRIVER_ERROR;
		}

		write_addr += TFM_ENC_PROGRAM_UNIT;
	}

	err = fenc_finalize(&fenc_ctx);
	if (err != 0) {
		return ARM_DRIVER_ERROR;
	}

	return ARM_DRIVER_OK;
}

static ARM_FLASH_INFO * ARM_Flash_GetInfo_Encrypted(void)
{
	return &FlashInfo_Encrypted;
}

const ARM_DRIVER_FLASH Driver_FLASH0_Encrypted = {
	.GetVersion	= ARM_Flash_GetVersion,
	.GetCapabilities = ARM_Flash_GetCapabilities,
	.Initialize	= ARM_Flash_Initialize,
	.Uninitialize	= ARM_Flash_Uninitialize,
	.PowerControl	= ARM_Flash_PowerControl,
	.ReadData	= ARM_Flash_ReadData_Encrypted,
	.ProgramData	= ARM_Flash_ProgramData_Encrypted,
	.EraseSector	= ARM_Flash_EraseSector,
	.EraseChip	= ARM_Flash_EraseChip,
	.GetStatus	= ARM_Flash_GetStatus,
	.GetInfo	= ARM_Flash_GetInfo_Encrypted,
};

#endif
