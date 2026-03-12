/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_lcs/nrf_lcs_mem.h>
#include <bl_storage.h>

int nrf_lcs_mem_provisioning_read(uint32_t *value)
{
	enum bl_storage_lcs bl_lcs = BL_STORAGE_LCS_UNKNOWN;
	int ret = read_life_cycle_state(&bl_lcs);

	*value = (uint32_t)BL_STORAGE_LCS_PROVISIONING;

	if ((ret == 0) && (bl_lcs < BL_STORAGE_LCS_PROVISIONING)) {
		*value = NRF_LCS_MEM_VALUE_NOT_ENTERED;
	}

	return ret;
}

int nrf_lcs_mem_provisioning_write(uint32_t value)
{
	return update_life_cycle_state(BL_STORAGE_LCS_PROVISIONING);
}

int nrf_lcs_mem_secured_read(uint32_t *value)
{
	enum bl_storage_lcs bl_lcs = BL_STORAGE_LCS_UNKNOWN;
	int ret = read_life_cycle_state(&bl_lcs);

	*value = (uint32_t)BL_STORAGE_LCS_SECURED;

	if ((ret == 0) && (bl_lcs < BL_STORAGE_LCS_SECURED)) {
		*value = NRF_LCS_MEM_VALUE_NOT_ENTERED;
	}

	return ret;
}

int nrf_lcs_mem_secured_write(uint32_t value)
{
	return update_life_cycle_state(BL_STORAGE_LCS_SECURED);
}

int nrf_lcs_mem_decommissioned_read(uint32_t *value)
{
	enum bl_storage_lcs bl_lcs = BL_STORAGE_LCS_UNKNOWN;
	int ret = read_life_cycle_state(&bl_lcs);

	*value = (uint32_t)BL_STORAGE_LCS_DECOMMISSIONED;

	if ((ret == 0) && (bl_lcs < BL_STORAGE_LCS_DECOMMISSIONED)) {
		*value = NRF_LCS_MEM_VALUE_NOT_ENTERED;
	}

	return ret;
}

int nrf_lcs_mem_decommissioned_write(uint32_t value)
{
	return update_life_cycle_state(BL_STORAGE_LCS_DECOMMISSIONED);
}
