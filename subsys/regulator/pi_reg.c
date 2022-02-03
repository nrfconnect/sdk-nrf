/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <kernel.h>

#include <regulator/pi_reg.h>

void regulator_pi_reg_target_set(struct regulator_pi_reg *reg,
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

float regulator_pi_reg_target_get(struct regulator_pi_reg *reg)
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
