/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/light_ctrl_reg.h>

void bt_mesh_light_ctrl_reg_target_set(struct bt_mesh_light_ctrl_reg *reg,
				       float value,
				       int32_t transition_time)
{
	reg->prev_target = reg->target;
	reg->target = value;
	if (reg->target == reg->prev_target) {
		reg->transition_time = 0;
		return;
	}
	reg->transition_start = k_uptime_get();
	reg->transition_time = transition_time;
}

float bt_mesh_light_ctrl_reg_target_get(struct bt_mesh_light_ctrl_reg *reg)
{
	if (reg->transition_time == 0) {
		return reg->target;
	}

	int32_t elapsed = k_uptime_get() - reg->transition_start;

	if (elapsed >= reg->transition_time) {
		reg->transition_time = 0;
		return reg->target;
	}
	return reg->prev_target +
		(elapsed * (reg->target - reg->prev_target)) / reg->transition_time;
}
