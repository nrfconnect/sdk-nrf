/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/core.h>
#include "op_slots.h"
#include "regs_commands.h"
#include "pkhardware_ba414e.h"

static const struct sx_pk_cmd_def CMD_SRP_USER_KEY_GEN = {
	SX_PK_OP_SRP_USER_KEY_GEN, (1 << OP_SLOT_SRP_KEYPARAMS_S),
	(1 << OP_SLOT_SRP_KEYPARAMS_N) | (1 << OP_SLOT_SRP_KEYPARAMS_G) |
		(1 << OP_SLOT_SRP_KEYPARAMS_A) | (1 << OP_SLOT_SRP_KEYPARAMS_B) |
		(1 << OP_SLOT_SRP_KEYPARAMS_X) | (1 << OP_SLOT_SRP_KEYPARAMS_K) |
		(1 << OP_SLOT_SRP_KEYPARAMS_U),
	0, SX_PK_OP_FLAGS_MOD_CM};
const struct sx_pk_cmd_def *const SX_PK_CMD_SRP_USER_KEY_GEN = &CMD_SRP_USER_KEY_GEN;

static const struct sx_pk_cmd_def CMD_SRP_SERVER_PUBLIC_KEY_GEN = {
	PK_OP_SRP_SERVER_PUBLIC_KEY_GEN, (1 << OP_SLOT_SRP_B),
	(1 << OP_SLOT_SRP_N) | (1 << OP_SLOT_SRP_G) | (1 << OP_SLOT_SRP_K) | (1 << OP_SLOT_SRP_V) |
		(1 << OP_SLOT_SRP_BRND),
	0, SX_PK_OP_FLAGS_MOD_CM};
const struct sx_pk_cmd_def *const SX_PK_CMD_SRP_SERVER_PUBLIC_KEY_GEN =
	&CMD_SRP_SERVER_PUBLIC_KEY_GEN;

static const struct sx_pk_cmd_def CMD_SRP_SERVER_SESSION_KEY_GEN = {
	PK_OP_SRP_SERVER_SESSION_KEY_GEN, (1 << OP_SLOT_SRP_S),
	(1 << OP_SLOT_SRP_N) | (1 << OP_SLOT_SRP_A) | (1 << OP_SLOT_SRP_U) | (1 << OP_SLOT_SRP_V) |
		(1 << OP_SLOT_SRP_BRND),
	0, SX_PK_OP_FLAGS_MOD_CM};
const struct sx_pk_cmd_def *const SX_PK_CMD_SRP_SERVER_SESSION_KEY_GEN =
	&CMD_SRP_SERVER_SESSION_KEY_GEN;
