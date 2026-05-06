/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <log_memory_protection.h>
#include <cmsis.h>
#include <hal/nrf_mpc.h>
#include <tfm_log.h>

static void log_memory_protection_sau(void)
{
	uint32_t sau_regions_count = SAU->TYPE & SAU_TYPE_SREGION_Msk;
	uint32_t limit_address;

	INFO("SAU config:\r\n");
	for (uint32_t i = 0; i < sau_regions_count; i++) {
		SAU->RNR = i & SAU_RNR_REGION_Msk;

		limit_address = SAU->RLAR;
		if (limit_address & SAU_RLAR_ENABLE_Msk) {
			if (limit_address & SAU_RLAR_NSC_Msk) {
				INFO("NCS region\r\n");
			} else {
				INFO("NS region\r\n");
			}
			limit_address &= ~(SAU_RLAR_ENABLE_Msk | SAU_RLAR_NSC_Msk);
			limit_address &= SAU_RLAR_LADDR_Msk;

			INFO("   Base addr : 0x%08X\r\n", SAU->RBAR & SAU_RBAR_BADDR_Msk);
			INFO("   Limit addr: 0x%08X\r\n", limit_address);
		}
	}
}

static void log_memory_protection_mpc(void)
{
	/* On 54L, the override regions (NRF_MPC00->OVERRIDE[]) are fixed in HW and the
	 * OVERRIDE indexes (that are useful to us) start at 0 and end at 4 (inclusive).
	 */
	const uint32_t max_index = 4;
	uint32_t address;
	uint32_t perm_settings;
	nrf_mpc_override_config_t config;

	INFO("MPC config:\r\n");
	for (uint32_t i = 0; i <= max_index; i++) {
		config = nrf_mpc_override_config_get(NRF_MPC00, i);
		if (!config.enable) {
			continue;
		}

		perm_settings = nrf_mpc_override_perm_get(NRF_MPC00, i);
		perm_settings &= nrf_mpc_override_permmask_get(NRF_MPC00, i);

		if (perm_settings & MPC_OVERRIDE_PERM_SECATTR_Msk) {
			INFO("S region\r\n");
		} else {
			INFO("NS region\r\n");
		}

		address = nrf_mpc_override_startaddr_get(NRF_MPC00, i);
		INFO("   Base addr: 0x%08X\r\n", address);

		address = nrf_mpc_override_endaddr_get(NRF_MPC00, i);
		INFO("   End addr : 0x%08X\r\n", address);
	}
}

void log_memory_protection_sau_mpc(void)
{
	INFO("** NS memory layout config start **\r\n");
	log_memory_protection_sau();
	log_memory_protection_mpc();
	INFO("** NS memory layout config end **\r\n");
}
