/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_mpc.h>

#include "bl_mpc_hw.h"

#define HW_PERMMASK_ALL                                                                            \
	(NRF_MPC_PERM_READ_MASK | NRF_MPC_PERM_WRITE_MASK | NRF_MPC_PERM_EXECUTE_MASK |            \
	 NRF_MPC_PERM_SECURE_MASK)

#define MPC_INSTANCE (NRF_MPC00)

static uint32_t permissions_to_hw(uint32_t permissions)
{
	uint32_t hw_permissions = NRF_MPC_PERM_SECURE_MASK;

	if ((permissions & BL_MPC_PERM_READ) != 0U) {
		hw_permissions |= NRF_MPC_PERM_READ_MASK;
	}

	if ((permissions & BL_MPC_PERM_WRITE) != 0U) {
		hw_permissions |= NRF_MPC_PERM_WRITE_MASK;
	}

	if ((permissions & BL_MPC_PERM_EXECUTE) != 0U) {
		hw_permissions |= NRF_MPC_PERM_EXECUTE_MASK;
	}

	return hw_permissions;
}

void bl_mpc_hw_override_config_get(uint8_t index, bool *enabled, bool *locked)
{
	nrf_mpc_override_config_t config = nrf_mpc_override_config_get(MPC_INSTANCE, index);

	*enabled = config.enable;
	*locked = config.lock;
}

bool bl_mpc_hw_override_matches(uint8_t index, const struct bl_mpc_region *region)
{
	return nrf_mpc_override_startaddr_get(MPC_INSTANCE, index) == region->start &&
	       nrf_mpc_override_endaddr_get(MPC_INSTANCE, index) == region->end &&
	       nrf_mpc_override_perm_get(MPC_INSTANCE, index) ==
		       permissions_to_hw(region->permissions) &&
	       nrf_mpc_override_permmask_get(MPC_INSTANCE, index) == HW_PERMMASK_ALL;
}

void bl_mpc_hw_override_write(uint8_t index, const struct bl_mpc_region *region)
{
	nrf_mpc_override_config_t config = {
		.lock = true,
		.enable = true,
	};

	nrf_mpc_override_startaddr_set(MPC_INSTANCE, index, region->start);
	nrf_mpc_override_endaddr_set(MPC_INSTANCE, index, region->end);
	nrf_mpc_override_perm_set(MPC_INSTANCE, index, permissions_to_hw(region->permissions));
	nrf_mpc_override_permmask_set(MPC_INSTANCE, index, HW_PERMMASK_ALL);
	nrf_mpc_override_config_set(MPC_INSTANCE, index, &config);
}

void bl_mpc_hw_override_disable(uint8_t index, bool lock)
{
	nrf_mpc_override_config_t config = {
		.lock = lock,
		.enable = false,
	};

	nrf_mpc_override_config_set(MPC_INSTANCE, index, &config);
}

void bl_mpc_hw_data_sync(void)
{
	__DSB();
}

void bl_mpc_hw_instruction_sync(void)
{
	__ISB();
}
