/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <bl_mpc.h>

#include "bl_mpc_hw.h"

static inline bool is_boundary_aligned(uint32_t address)
{
	return (address % BL_MPC_GRANULARITY_BYTES) == 0U;
}

/**
 * @brief Validate the requested region list.
 *
 * Checks the region count, boundary alignment, range ordering, and permission
 * bits without accessing MPC.
 */
static int validate_requested_regions(const struct bl_mpc_region *regions, size_t region_count)
{
	if (regions == NULL || region_count == 0U) {
		return -EINVAL;
	}

	if (region_count > BL_MPC_REGION_COUNT_MAX) {
		return -ENOSPC;
	}

	for (size_t i = 0; i < region_count; i++) {
		if (!is_boundary_aligned(regions[i].start) ||
		    !is_boundary_aligned(regions[i].end) ||
		    regions[i].end <= regions[i].start ||
		    (regions[i].permissions & ~BL_MPC_PERM_ALL) != 0U) {
			return -EINVAL;
		}
	}

	return 0;
}

/** @brief Check whether any software-configurable override is locked. */
static bool any_override_locked(void)
{
	for (uint8_t i = 0U; i <= BL_MPC_REGION_COUNT_MAX; i++) {
		bool enabled;
		bool locked;

		bl_mpc_hw_override_config_get(i, &enabled, &locked);
		if (locked) {
			return true;
		}
	}

	return false;
}

/**
 * @brief Verify that all software-configurable overrides match the target state.
 *
 * Assigned overrides must be enabled, locked, and match their requested regions.
 * Unassigned and access-blocking overrides must be locked disabled.
 */
static int verify_target_state(const struct bl_mpc_region *regions, uint8_t region_count)
{
	for (uint8_t i = 0U; i < region_count; i++) {
		bool enabled;
		bool locked;

		bl_mpc_hw_override_config_get(i, &enabled, &locked);
		if (!locked || !enabled || !bl_mpc_hw_override_matches(i, &regions[i])) {
			return -EIO;
		}
	}

	for (uint8_t i = region_count; i <= BL_MPC_REGION_COUNT_MAX; i++) {
		bool enabled;
		bool locked;

		bl_mpc_hw_override_config_get(i, &enabled, &locked);
		if (!locked || enabled) {
			return -EIO;
		}
	}

	return 0;
}

int bl_mpc_protect_and_lock(const struct bl_mpc_region *regions, size_t region_count)
{
	int err = 0;
	uint8_t requested_count;

	err = validate_requested_regions(regions, region_count);
	if (err != 0) {
		return err;
	}

	requested_count = (uint8_t)region_count;

	if (any_override_locked()) {
		err = verify_target_state(regions, requested_count);
		return err == 0 ? 0 : -EACCES;
	}

	for (uint8_t i = 0U; i < requested_count; i++) {
		bool enabled;
		bool locked;

		bl_mpc_hw_override_config_get(i, &enabled, &locked);

		/* If the override is unlocked and enabled, disable it, so it can be programmed
		 * to the requested state.
		 */
		if (enabled) {
			bl_mpc_hw_override_disable(i, false);
			bl_mpc_hw_data_sync();
		}

		bl_mpc_hw_override_write(i, &regions[i]);
	}

	for (uint8_t i = requested_count; i <= BL_MPC_REGION_COUNT_MAX; i++) {
		bool enabled;
		bool locked;

		bl_mpc_hw_override_config_get(i, &enabled, &locked);

		if (enabled) {
			bl_mpc_hw_override_disable(i, false);
			bl_mpc_hw_data_sync();
		}

		bl_mpc_hw_override_disable(i, true);
	}

	bl_mpc_hw_data_sync();
	bl_mpc_hw_instruction_sync();

	return verify_target_state(regions, requested_count);
}
