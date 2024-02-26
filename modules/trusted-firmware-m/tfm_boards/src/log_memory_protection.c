/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cmsis.h>
#include <tfm_spm_log.h>
#include <mpu_armv8m_drv.h>
#include <nrf.h>
#include <nrf_peripherals.h>
#include <array.h>
#include <nrfx_nvmc.h>

void log_memory_protection_of_mpu(void)
{
	SPMLOG_DBGMSG("\nARM MPU configuration\r\n");
	SPMLOG_DBGMSGVAL("CTRL", MPU->CTRL);

	uint32_t num_regions = ((MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos);

	for (uint32_t i = 0; i < num_regions; i++) {
		MPU->RNR = i;
		SPMLOG_DBGMSGVAL("RNR", i);
		uint32_t rbar = MPU->RBAR;
		uint32_t rlar = MPU->RLAR;
		uint32_t en = (rlar & MPU_RLAR_EN_Msk) >> MPU_RLAR_EN_Pos;

		SPMLOG_DBGMSGVAL("  EN", en);
		if (en) {
			SPMLOG_DBGMSGVAL("  BASE", rbar & MPU_RBAR_BASE_Msk);

			/* The last address that is protected is limit + 31 */
			SPMLOG_DBGMSGVAL("  LIMIT*", (rlar & MPU_RLAR_LIMIT_Msk) + 31);

			SPMLOG_DBGMSGVAL("  XN", (rbar & MPU_RBAR_XN_Msk) >> MPU_RBAR_XN_Pos);
			SPMLOG_DBGMSGVAL("  AP", (rbar & MPU_RBAR_AP_Msk) >> MPU_RBAR_AP_Pos);
			SPMLOG_DBGMSGVAL("  SH", (rbar & MPU_RBAR_SH_Msk) >> MPU_RBAR_SH_Pos);

			SPMLOG_DBGMSGVAL("  ATTRINDEX",
					 (rlar & MPU_RLAR_AttrIndx_Msk) >> MPU_RLAR_AttrIndx_Pos);
		}
	}
}

void log_memory_protection_of_spu_nsc(void)
{
	SPMLOG_DBGMSGVAL("NRF_SPU_S->FLASHNSC[0].REGION", NRF_SPU_S->FLASHNSC[0].REGION);
	SPMLOG_DBGMSGVAL("NRF_SPU_S->FLASHNSC[1].REGION", NRF_SPU_S->FLASHNSC[1].REGION);

	SPMLOG_DBGMSGVAL("NRF_SPU_S->FLASHNSC[0].SIZE", NRF_SPU_S->FLASHNSC[0].SIZE);
	SPMLOG_DBGMSGVAL("NRF_SPU_S->FLASHNSC[1].SIZE", NRF_SPU_S->FLASHNSC[1].SIZE);

	SPMLOG_DBGMSGVAL("NRF_SPU_S->RAMNSC[0].REGION", NRF_SPU_S->RAMNSC[0].REGION);
	SPMLOG_DBGMSGVAL("NRF_SPU_S->RAMNSC[1].REGION", NRF_SPU_S->RAMNSC[1].REGION);

	SPMLOG_DBGMSGVAL("NRF_SPU_S->RAMNSC[0].SIZE", NRF_SPU_S->RAMNSC[0].SIZE);
	SPMLOG_DBGMSGVAL("NRF_SPU_S->RAMNSC[1].SIZE", NRF_SPU_S->RAMNSC[1].SIZE);
}

void log_memory_protection_of_spu_flash(void)
{
	SPMLOG_DBGMSG("SPU FLASH region permissions\n");
	uint32_t num_flash_regions = ARRAY_SIZE(NRF_SPU_S->FLASHREGION);
	uint32_t prev_perm = 0x0BADF00D;

	for (int i = 0; i < num_flash_regions; i++) {
		uint32_t perm = NRF_SPU_S->FLASHREGION[i].PERM;

		if (perm != prev_perm) {
			prev_perm = perm;
			SPMLOG_DBGMSGVAL("i", i * nrfx_nvmc_flash_page_size_get());
			SPMLOG_DBGMSGVAL("PERM", perm);
		}
	}
}

void log_memory_protection_of_spu_ram(void)
{
	SPMLOG_DBGMSG("SPU RAM region permissions\n");
	uint32_t num_ram_regions = ARRAY_SIZE(NRF_SPU_S->RAMREGION);
	uint32_t prev_perm = 0x0BADF00D;

	for (int i = 0; i < num_ram_regions; i++) {
		uint32_t perm = NRF_SPU_S->RAMREGION[i].PERM;

		if (perm != prev_perm) {
			prev_perm = perm;
			SPMLOG_DBGMSGVAL("i", i * SPU_RAMREGION_SIZE);
			SPMLOG_DBGMSGVAL("PERM", perm);
		}
	}
}

void log_memory_protection_of_spu(void)
{
	log_memory_protection_of_spu_nsc();
	log_memory_protection_of_spu_flash();
	log_memory_protection_of_spu_ram();
}

void log_memory_protection(void)
{
	log_memory_protection_of_mpu();
	log_memory_protection_of_spu();
}
