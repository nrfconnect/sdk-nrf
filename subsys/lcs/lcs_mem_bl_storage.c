/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <lcs_mem.h>
#include <bl_storage.h>

int lcs_mem_provisioning_read(uint16_t *value)
{
	enum bl_storage_lcs bl_lcs = BL_STORAGE_LCS_UNKNOWN;
	int ret = read_life_cycle_state(&bl_lcs);

	*value = (uint16_t)BL_STORAGE_LCS_PROVISIONING;

	if ((ret == 0) && (bl_lcs < BL_STORAGE_LCS_PROVISIONING)) {
		*value = LCS_MEM_VALUE_NOT_ENTERED;
	}

	return ret;
}

int lcs_mem_provisioning_write(uint16_t value)
{
	return update_life_cycle_state(BL_STORAGE_LCS_PROVISIONING);
}

int lcs_mem_secured_read(uint16_t *value)
{
	enum bl_storage_lcs bl_lcs = BL_STORAGE_LCS_UNKNOWN;
	int ret = read_life_cycle_state(&bl_lcs);

	*value = (uint16_t)BL_STORAGE_LCS_SECURED;

	if ((ret == 0) && (bl_lcs < BL_STORAGE_LCS_SECURED)) {
		*value = LCS_MEM_VALUE_NOT_ENTERED;
	}

	return ret;
}

int lcs_mem_secured_write(uint16_t value)
{
	return update_life_cycle_state(BL_STORAGE_LCS_SECURED);
}

int lcs_mem_decommissioned_read(uint16_t *value)
{
	enum bl_storage_lcs bl_lcs = BL_STORAGE_LCS_UNKNOWN;
	int ret = read_life_cycle_state(&bl_lcs);

	*value = (uint16_t)BL_STORAGE_LCS_DECOMMISSIONED;

	if ((ret == 0) && (bl_lcs < BL_STORAGE_LCS_DECOMMISSIONED)) {
		*value = LCS_MEM_VALUE_NOT_ENTERED;
	}

	return ret;
}

int lcs_mem_decommissioned_write(uint16_t value)
{
	return update_life_cycle_state(BL_STORAGE_LCS_DECOMMISSIONED);
}
