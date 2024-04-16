/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/core.h>
#include <silexpk/ik.h>
#include "regs_commands.h"
#include "../ba414/op_slots.h"
#include "../ba414/pkhardware_ba414e.h"

static const struct sx_pk_cmd_def CMD_IK_ECDSA_SIGN = {
	PK_OP_IK_ECDSA_SIGN, (1 << OP_SLOT_ECDSA_SGN_R) | (1 << OP_SLOT_ECDSA_SGN_S),
	(1 << OP_SLOT_ECDSA_SGN_H), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_IK_ECDSA_SIGN = &CMD_IK_ECDSA_SIGN;

static const struct sx_pk_cmd_def CMD_IK_PTMULT = {PK_OP_IK_ECC_PTMUL, (3 << OP_SLOT_IK_PUBKEY_R),
						   (3 << OP_SLOT_IK_PUBKEY_P), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_IK_PTMULT = &CMD_IK_PTMULT;

static const struct sx_pk_cmd_def CMD_IK_PUBKEY_GEN = {PK_OP_IK_PUB_GEN, (3 << OP_SLOT_IK_PUBKEY_Q),
						       0, 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_IK_PUBKEY_GEN = &CMD_IK_PUBKEY_GEN;

static const struct sx_pk_cmd_def CMD_EXIT_IK = {PK_OP_IK_EXIT, 0, 0, 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_IK_EXIT = &CMD_EXIT_IK;
