/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/core.h>
#include "op_slots.h"
#include "regs_commands.h"
#include "pkhardware_ba414e.h"

static const struct sx_pk_cmd_def CMD_ECJPAKE_GENERATE_ZKP = {
	PK_OP_ECJPAKE_GENERATE_ZKP, (1 << OP_SLOT_ECJPAKE_R),
	(1 << OP_SLOT_ECJPAKE_V) | (1 << OP_SLOT_ECJPAKE_X) | (1 << OP_SLOT_ECJPAKE_H), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECJPAKE_GENERATE_ZKP = &CMD_ECJPAKE_GENERATE_ZKP;

static const struct sx_pk_cmd_def CMD_ECJPAKE_VERIFY_ZKP = {
	PK_OP_ECJPAKE_VERIFY_ZKP, 0,
	(1 << OP_SLOT_ECJPAKE_XV) | (1 << OP_SLOT_ECJPAKE_YV) | (1 << OP_SLOT_ECJPAKE_XX) |
		(1 << OP_SLOT_ECJPAKE_YX) | (1 << OP_SLOT_ECJPAKE_R) | (1 << OP_SLOT_ECJPAKE_H) |
		(1 << OP_SLOT_ECJPAKE_XG_2) | (1 << OP_SLOT_ECJPAKE_YG_2),
	OP_SLOT_ECJPAKE_XG_2};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECJPAKE_VERIFY_ZKP = &CMD_ECJPAKE_VERIFY_ZKP;

static const struct sx_pk_cmd_def CMD_ECJPAKE_3PT_ADD = {
	PK_OP_ECJPAKE_3PT_ADD, (1 << OP_SLOT_ECJPAKE_GB_1) | (1 << OP_SLOT_ECJPAKE_GB_2),
	(1 << OP_SLOT_ECJPAKE_X2_1) | (1 << OP_SLOT_ECJPAKE_X2_2) | (1 << OP_SLOT_ECJPAKE_X3_1) |
		(1 << OP_SLOT_ECJPAKE_X3_2) | (1 << OP_SLOT_ECJPAKE_X1_1) |
		(1 << OP_SLOT_ECJPAKE_X1_2),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECJPAKE_3PT_ADD = &CMD_ECJPAKE_3PT_ADD;

static const struct sx_pk_cmd_def CMD_ECJPAKE_GEN_SESS_KEY = {
	PK_OP_ECJPAKE_GEN_SESS_KEY, (1 << OP_SLOT_ECJPAKE_T_1) | (1 << OP_SLOT_ECJPAKE_T_2),
	(1 << OP_SLOT_ECJPAKE_X4_1) | (1 << OP_SLOT_ECJPAKE_X4_2) | (1 << OP_SLOT_ECJPAKE_B_1) |
		(1 << OP_SLOT_ECJPAKE_B_2) | (1 << OP_SLOT_ECJPAKE_X2) | (1 << OP_SLOT_ECJPAKE_X2S),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECJPAKE_GEN_SESS_KEY = &CMD_ECJPAKE_GEN_SESS_KEY;

static const struct sx_pk_cmd_def CMD_ECJPAKE_GEN_STEP_2 = {
	PK_OP_ECJPAKE_GEN_STEP_2,
	(1 << OP_SLOT_ECJPAKE_AX) | (1 << OP_SLOT_ECJPAKE_AY) | (1 << OP_SLOT_ECJPAKE_X2S) |
		(1 << OP_SLOT_ECJPAKE_GAX) | (1 << OP_SLOT_ECJPAKE_GAY),
	(1 << OP_SLOT_ECJPAKE_X4_1) | (1 << OP_SLOT_ECJPAKE_X4_2) | (1 << OP_SLOT_ECJPAKE_X3_1) |
		(1 << OP_SLOT_ECJPAKE_X3_2) | (1 << OP_SLOT_ECJPAKE_X1_1) |
		(1 << OP_SLOT_ECJPAKE_X1_2) | (1 << OP_SLOT_ECJPAKE_S) |
		(1 << OP_SLOT_ECJPAKE_X2SS),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECJPAKE_GEN_STEP_2 = &CMD_ECJPAKE_GEN_STEP_2;
