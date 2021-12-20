/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/light_ctrl_reg_spec.h>

#define REG_INT CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC_INTERVAL

static void reg_step(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_light_ctrl_reg_spec *spec_reg = CONTAINER_OF(
		dwork, struct bt_mesh_light_ctrl_reg_spec, timer);

	if (!spec_reg->enabled) {
		/* The regulator might be disabled asynchronously. */
		return;
	}

	k_work_reschedule(&spec_reg->timer, K_MSEC(REG_INT));

	float target = bt_mesh_light_ctrl_reg_target_get(&spec_reg->reg);
	float error = target - spec_reg->reg.measured;
	/* Accuracy should be in percent and both up and down: */
	float accuracy = (spec_reg->reg.cfg.accuracy * target) / (2 * 100.0f);
	float input;

	if (error > accuracy) {
		input = error - accuracy;
	} else if (error < -accuracy) {
		input = error + accuracy;
	} else {
		input = 0.0f;
	}

	float kp, ki;

	if (input >= 0) {
		kp = spec_reg->reg.cfg.kp.up;
		ki = spec_reg->reg.cfg.ki.up;
	} else {
		kp = spec_reg->reg.cfg.kp.down;
		ki = spec_reg->reg.cfg.ki.down;
	}

	spec_reg->i += (input * ki) * ((float)REG_INT / (float)MSEC_PER_SEC);
	spec_reg->i = CLAMP(spec_reg->i, 0, UINT16_MAX);

	float p = input * kp;
	float output = spec_reg->i + p;

	spec_reg->reg.updated(&spec_reg->reg, output);
}

void bt_mesh_light_ctrl_reg_spec_start(struct bt_mesh_light_ctrl_reg *reg)
{
	struct bt_mesh_light_ctrl_reg_spec *spec_reg = CONTAINER_OF(
		reg, struct bt_mesh_light_ctrl_reg_spec, reg);
	spec_reg->enabled = true;
	k_work_schedule(&spec_reg->timer, K_MSEC(REG_INT));
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
