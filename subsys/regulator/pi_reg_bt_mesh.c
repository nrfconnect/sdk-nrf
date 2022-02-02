/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <kernel.h>

#include <regulator/pi_reg_bt_mesh.h>

#define REG_INT CONFIG_REGULATOR_PI_REG_BT_MESH_INTERVAL

static void reg_step(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct regulator_pi_reg_bt_mesh *mesh_reg = CONTAINER_OF(
		dwork, struct regulator_pi_reg_bt_mesh, timer);

	if (!mesh_reg->enabled) {
		/* The regulator might be disabled asynchronously. */
		return;
	}

	k_work_reschedule(&mesh_reg->timer, K_MSEC(REG_INT));

	float target = regulator_pi_reg_target_get(&mesh_reg->reg);
	float error = target - mesh_reg->reg.measured;
	/* Accuracy should be in percent and both up and down: */
	float accuracy = (mesh_reg->reg.cfg.accuracy * target) / (2 * 100.0f);
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
		kp = mesh_reg->reg.cfg.kp.up;
		ki = mesh_reg->reg.cfg.ki.up;
	} else {
		kp = mesh_reg->reg.cfg.kp.down;
		ki = mesh_reg->reg.cfg.ki.down;
	}

	mesh_reg->i += (input * ki) * ((float)REG_INT / (float)MSEC_PER_SEC);
	mesh_reg->i = CLAMP(mesh_reg->i, 0, UINT16_MAX);

	float p = input * kp;
	float output = mesh_reg->i + p;

	mesh_reg->reg.updated(&mesh_reg->reg, output);
}

void regulator_pi_reg_bt_mesh_start(struct regulator_pi_reg *reg)
{
	struct regulator_pi_reg_bt_mesh *mesh_reg = CONTAINER_OF(
		reg, struct regulator_pi_reg_bt_mesh, reg);
	mesh_reg->enabled = true;
	k_work_schedule(&mesh_reg->timer, K_MSEC(REG_INT));
}

void regulator_pi_reg_bt_mesh_stop(struct regulator_pi_reg *reg)
{
	struct regulator_pi_reg_bt_mesh *mesh_reg = CONTAINER_OF(
		reg, struct regulator_pi_reg_bt_mesh, reg);
	mesh_reg->i = 0;
	mesh_reg->enabled = false;
	k_work_cancel_delayable(&mesh_reg->timer);
}

void regulator_pi_reg_bt_mesh_init(struct regulator_pi_reg *reg)
{
	struct regulator_pi_reg_bt_mesh *mesh_reg = CONTAINER_OF(
		reg, struct regulator_pi_reg_bt_mesh, reg);
	k_work_init_delayable(&mesh_reg->timer, reg_step);
}
