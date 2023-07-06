/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/light_ctrl_reg_spec.h>

#define REG_INT CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC_INTERVAL

struct reg_terms {
	float i;
	float p;
};

static struct reg_terms reg_terms_calc(struct bt_mesh_light_ctrl_reg_spec *spec_reg)
{
	float target = bt_mesh_light_ctrl_reg_target_get(&spec_reg->reg);
	float error = target - spec_reg->reg.measured;
	/* Accuracy should be in percent and both up and down: */
	float accuracy = (spec_reg->reg.cfg.accuracy * target) / (2 * 100.0f);
	float input;
	float kp, ki;

	if (error > accuracy) {
		input = error - accuracy;
	} else if (error < -accuracy) {
		input = error + accuracy;
	} else {
		input = 0.0f;
	}

	if (input >= 0) {
		kp = spec_reg->reg.cfg.kp.up;
		ki = spec_reg->reg.cfg.ki.up;
	} else {
		kp = spec_reg->reg.cfg.kp.down;
		ki = spec_reg->reg.cfg.ki.down;
	}

	return (struct reg_terms){
		.i = ((input) * (ki) * ((float)REG_INT / (float)MSEC_PER_SEC)),
		.p = input * kp,
	};
}

static void reg_step(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_light_ctrl_reg_spec *spec_reg = CONTAINER_OF(
		dwork, struct bt_mesh_light_ctrl_reg_spec, timer);
	struct reg_terms reg_terms;

	if (!spec_reg->enabled) {
		/* The regulator might be disabled asynchronously. */
		return;
	}

	k_work_reschedule(&spec_reg->timer, K_MSEC(REG_INT));

	reg_terms = reg_terms_calc(spec_reg);
	spec_reg->i += reg_terms.i;

	if (spec_reg->i >= 0) {
		/* Drop the negative flag as soon as the internal sum becomes positive. */
		spec_reg->neg = false;
	}

	if (!spec_reg->neg) {
		spec_reg->i = CLAMP(spec_reg->i, 0, UINT16_MAX);
	}

	float output = spec_reg->i + reg_terms.p;

	spec_reg->reg.updated(&spec_reg->reg, output);
}

static void internal_sum_recover(struct bt_mesh_light_ctrl_reg_spec *spec_reg, uint16_t lightness)
{
	struct reg_terms reg_terms;

	reg_terms = reg_terms_calc(spec_reg);

	/* Recalculate the internal sum so that it is equal to the passed lightness level at the
	 * next regulator step.
	 */
	spec_reg->i = lightness - reg_terms.i;
	/* Allow the internal sum to be negative until it becomes positive. */
	spec_reg->neg = true;
}

void bt_mesh_light_ctrl_reg_spec_start(struct bt_mesh_light_ctrl_reg *reg, uint16_t lightness)
{
	struct bt_mesh_light_ctrl_reg_spec *spec_reg = CONTAINER_OF(
		reg, struct bt_mesh_light_ctrl_reg_spec, reg);
	spec_reg->enabled = true;
	k_work_schedule(&spec_reg->timer, K_MSEC(REG_INT));
	internal_sum_recover(spec_reg, lightness);
}

void bt_mesh_light_ctrl_reg_spec_stop(struct bt_mesh_light_ctrl_reg *reg)
{
	struct bt_mesh_light_ctrl_reg_spec *spec_reg = CONTAINER_OF(
		reg, struct bt_mesh_light_ctrl_reg_spec, reg);
	spec_reg->i = 0;
	spec_reg->enabled = false;
	k_work_cancel_delayable(&spec_reg->timer);
}

void bt_mesh_light_ctrl_reg_spec_init(struct bt_mesh_light_ctrl_reg *reg)
{
	struct bt_mesh_light_ctrl_reg_spec *spec_reg = CONTAINER_OF(
		reg, struct bt_mesh_light_ctrl_reg_spec, reg);
	k_work_init_delayable(&spec_reg->timer, reg_step);
}
