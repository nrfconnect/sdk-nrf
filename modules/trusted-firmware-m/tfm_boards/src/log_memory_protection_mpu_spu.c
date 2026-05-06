/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <log_memory_protection.h>
#include <cmsis.h>
#include <tfm_log.h>
#include <mpu_armv8m_drv.h>
#include <nrfx.h>
#include <array.h>
#include <nrfx_nvmc.h>

void log_memory_protection_of_mpu(void)
{
	VERBOSE("\nARM MPU configuration\r\n");
	VERBOSE("CTRL: 0x%08x\r\n", (unsigned int)MPU->CTRL);

	uint32_t num_regions = ((MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos);

	for (uint32_t i = 0; i < num_regions; i++) {
		MPU->RNR = i;
		VERBOSE("RNR: 0x%08x\r\n", (unsigned int)i);
		uint32_t rbar = MPU->RBAR;
		uint32_t rlar = MPU->RLAR;
		uint32_t en = (rlar & MPU_RLAR_EN_Msk) >> MPU_RLAR_EN_Pos;

		VERBOSE("  EN: 0x%08x\r\n", (unsigned int)en);
		if (en) {
			VERBOSE("  BASE: 0x%08x\r\n", (unsigned int)(rbar & MPU_RBAR_BASE_Msk));

			/* The last address that is protected is limit + 31 */
			VERBOSE("  LIMIT*: 0x%08x\r\n",
				(unsigned int)((rlar & MPU_RLAR_LIMIT_Msk) + 31));

			VERBOSE("  XN: 0x%08x\r\n",
				(unsigned int)((rbar & MPU_RBAR_XN_Msk) >> MPU_RBAR_XN_Pos));
			VERBOSE("  AP: 0x%08x\r\n",
				(unsigned int)((rbar & MPU_RBAR_AP_Msk) >> MPU_RBAR_AP_Pos));
			VERBOSE("  SH: 0x%08x\r\n",
				(unsigned int)((rbar & MPU_RBAR_SH_Msk) >> MPU_RBAR_SH_Pos));

			VERBOSE("  ATTRINDEX: 0x%08x\r\n",
				(unsigned int)((rlar & MPU_RLAR_AttrIndx_Msk) >>
					       MPU_RLAR_AttrIndx_Pos));
		}
	}
}

void log_memory_protection_of_spu_nsc(void)
{
	VERBOSE("NRF_SPU_S->FLASHNSC[0].REGION: 0x%08x\r\n",
		(unsigned int)NRF_SPU_S->FLASHNSC[0].REGION);
	VERBOSE("NRF_SPU_S->FLASHNSC[1].REGION: 0x%08x\r\n",
		(unsigned int)NRF_SPU_S->FLASHNSC[1].REGION);

	VERBOSE("NRF_SPU_S->FLASHNSC[0].SIZE: 0x%08x\r\n",
		(unsigned int)NRF_SPU_S->FLASHNSC[0].SIZE);
	VERBOSE("NRF_SPU_S->FLASHNSC[1].SIZE: 0x%08x\r\n",
		(unsigned int)NRF_SPU_S->FLASHNSC[1].SIZE);

	VERBOSE("NRF_SPU_S->RAMNSC[0].REGION: 0x%08x\r\n",
		(unsigned int)NRF_SPU_S->RAMNSC[0].REGION);
	VERBOSE("NRF_SPU_S->RAMNSC[1].REGION: 0x%08x\r\n",
		(unsigned int)NRF_SPU_S->RAMNSC[1].REGION);

	VERBOSE("NRF_SPU_S->RAMNSC[0].SIZE: 0x%08x\r\n",
		(unsigned int)NRF_SPU_S->RAMNSC[0].SIZE);
	VERBOSE("NRF_SPU_S->RAMNSC[1].SIZE: 0x%08x\r\n",
		(unsigned int)NRF_SPU_S->RAMNSC[1].SIZE);
}

void log_memory_protection_of_spu_flash(void)
{
	VERBOSE("SPU FLASH region permissions\n");
	uint32_t num_flash_regions = ARRAY_SIZE(NRF_SPU_S->FLASHREGION);
	uint32_t prev_perm = 0x0BADF00D;

	for (int i = 0; i < num_flash_regions; i++) {
		uint32_t perm = NRF_SPU_S->FLASHREGION[i].PERM;

		if (perm != prev_perm) {
			prev_perm = perm;
			VERBOSE("i: 0x%08x\r\n",
				(unsigned int)(i * nrfx_nvmc_flash_page_size_get()));
			VERBOSE("PERM: 0x%08x\r\n", (unsigned int)perm);
		}
	}
}

void log_memory_protection_of_spu_ram(void)
{
	VERBOSE("SPU RAM region permissions\n");
	uint32_t num_ram_regions = ARRAY_SIZE(NRF_SPU_S->RAMREGION);
	uint32_t prev_perm = 0x0BADF00D;

	for (int i = 0; i < num_ram_regions; i++) {
		uint32_t perm = NRF_SPU_S->RAMREGION[i].PERM;

		if (perm != prev_perm) {
			prev_perm = perm;
			VERBOSE("i: 0x%08x\r\n", (unsigned int)(i * SPU_RAMREGION_SIZE));
			VERBOSE("PERM: 0x%08x\r\n", (unsigned int)perm);
		}
	}
}

void log_memory_protection_of_spu(void)
{
	log_memory_protection_of_spu_nsc();
	log_memory_protection_of_spu_flash();
	log_memory_protection_of_spu_ram();
}

void log_memory_protection_mpu_spu(void)
{
	log_memory_protection_of_mpu();
	log_memory_protection_of_spu();
}
