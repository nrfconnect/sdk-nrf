/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BL_MPC_HW_H__
#define BL_MPC_HW_H__

#include <stdbool.h>
#include <stdint.h>

#include <bl_mpc.h>

void bl_mpc_hw_override_config_get(uint8_t index, bool *enabled, bool *locked);

bool bl_mpc_hw_override_matches(uint8_t index, const struct bl_mpc_region *region);

void bl_mpc_hw_override_write(uint8_t index, const struct bl_mpc_region *region);

void bl_mpc_hw_override_disable(uint8_t index, bool lock);

void bl_mpc_hw_data_sync(void);

void bl_mpc_hw_instruction_sync(void);

#endif /* BL_MPC_HW_H__ */
