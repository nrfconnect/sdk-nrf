/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/core.h>
#include "op_slots.h"
#include "regs_commands.h"
#include "pkhardware_ba414e.h"

static const struct sx_pk_cmd_def CMD_MOD_ADD = {
	PK_OP_MOD_ADD, (1 << OP_SLOT_PTR_C), (1 << 0) | (1 << OP_SLOT_PTR_A) | (1 << OP_SLOT_PTR_B),
	OP_SLOT_PTR_A};
const struct sx_pk_cmd_def *const SX_PK_CMD_MOD_ADD = &CMD_MOD_ADD;

static const struct sx_pk_cmd_def CMD_MOD_SUB = {
	PK_OP_MOD_SUB, (1 << OP_SLOT_PTR_C), (1 << 0) | (1 << OP_SLOT_PTR_A) | (1 << OP_SLOT_PTR_B),
	OP_SLOT_PTR_A};
const struct sx_pk_cmd_def *const SX_PK_CMD_MOD_SUB = &CMD_MOD_SUB;

static const struct sx_pk_cmd_def CMD_ODD_MOD_MULT = {
	PK_OP_MOD_MULT, (1 << OP_SLOT_PTR_C),
	(1 << 0) | (1 << OP_SLOT_PTR_A) | (1 << OP_SLOT_PTR_B), OP_SLOT_PTR_A};
const struct sx_pk_cmd_def *const SX_PK_CMD_ODD_MOD_MULT = &CMD_ODD_MOD_MULT;

static const struct sx_pk_cmd_def CMD_EVEN_MOD_INV = {PK_OP_EVEN_MOD_INV, (1 << OP_SLOT_PTR_C),
						      (1 << 0) | (1 << OP_SLOT_PTR_B),
						      OP_SLOT_DEFAULT_PTRS};
const struct sx_pk_cmd_def *const SX_PK_CMD_EVEN_MOD_INV = &CMD_EVEN_MOD_INV;

static const struct sx_pk_cmd_def CMD_EVEN_MOD_REDUCE = {PK_OP_EVEN_MOD_RED, (1 << OP_SLOT_PTR_C),
							 (1 << 0) | (1 << OP_SLOT_PTR_B),
							 OP_SLOT_DEFAULT_PTRS};
const struct sx_pk_cmd_def *const SX_PK_CMD_EVEN_MOD_REDUCE = &CMD_EVEN_MOD_REDUCE;

static const struct sx_pk_cmd_def CMD_ODD_MOD_REDUCE = {PK_OP_MOD_REDUCE, (1 << OP_SLOT_PTR_C),
							(1 << 0) | (1 << OP_SLOT_PTR_B),
							OP_SLOT_DEFAULT_PTRS};
const struct sx_pk_cmd_def *const SX_PK_CMD_ODD_MOD_REDUCE = &CMD_ODD_MOD_REDUCE;

static const struct sx_pk_cmd_def CMD_ODD_MOD_DIV = {
	PK_OP_MOD_DIV, (1 << OP_SLOT_PTR_C), (1 << 0) | (1 << OP_SLOT_PTR_A) | (1 << OP_SLOT_PTR_B),
	OP_SLOT_PTR_A};
const struct sx_pk_cmd_def *const SX_PK_CMD_ODD_MOD_DIV = &CMD_ODD_MOD_DIV;

static const struct sx_pk_cmd_def CMD_ODD_MOD_INV = {
	PK_OP_MOD_INV, (1 << OP_SLOT_PTR_C), (1 << 0) | (1 << OP_SLOT_PTR_B), OP_SLOT_DEFAULT_PTRS};
const struct sx_pk_cmd_def *const SX_PK_CMD_ODD_MOD_INV = &CMD_ODD_MOD_INV;

static const struct sx_pk_cmd_def CMD_MOD_EXP = {
	PK_OP_MDEXP, (1 << OP_SLOT_MOD_EXP_RESULT),
	(1 << OP_SLOT_MOD_EXP_M) | (1 << OP_SLOT_MOD_EXP_INPUT) | (1 << OP_SLOT_MOD_EXP_EXP),
	OP_SLOT_PTR_A};
const struct sx_pk_cmd_def *const SX_PK_CMD_MOD_EXP = &CMD_MOD_EXP;

static const struct sx_pk_cmd_def CMD_MOD_EXP_FF = {
	PK_OP_MDEXP, (1 << OP_SLOT_MOD_EXP_RESULT),
	(1 << OP_SLOT_MOD_EXP_M) | (1 << OP_SLOT_MOD_EXP_INPUT) | (1 << OP_SLOT_MOD_EXP_EXP),
	OP_SLOT_PTR_A, SX_PK_OP_FLAGS_MOD_CM};
const struct sx_pk_cmd_def *const SX_PK_CMD_FF_MODEXP = &CMD_MOD_EXP_FF;

static const struct sx_pk_cmd_def CMD_MOD_SQRT = {PK_OP_MOD_SQRT, (1 << OP_SLOT_PTR_C),
						  (1 << OP_SLOT_PTR_A) | (1 << 0), OP_SLOT_PTR_A};
const struct sx_pk_cmd_def *const SX_PK_CMD_MOD_SQRT = &CMD_MOD_SQRT;

static const struct sx_pk_cmd_def CMD_MULT = {PK_OP_MULT, (1 << OP_SLOT_PTR_C),
					      (1 << OP_SLOT_PTR_A) | (1 << OP_SLOT_PTR_B),
					      OP_SLOT_PTR_A};
const struct sx_pk_cmd_def *const SX_PK_CMD_MULT = &CMD_MULT;

static const struct sx_pk_cmd_def CMD_MOD_EXP_CM = {
	PK_OP_RSA_MDEXP_CM, (1 << OP_SLOT_MOD_EXP_CM_RESULT),
	(1 << OP_SLOT_MOD_EXP_CM_LAMBDA_N) | (1 << OP_SLOT_MOD_EXP_CM_M) |
		(1 << OP_SLOT_MOD_EXP_CM_INPUT) | (1 << OP_SLOT_MOD_EXP_CM_EXP),
	OP_SLOT_PTR_A, SX_PK_OP_FLAGS_MOD_CM};
const struct sx_pk_cmd_def *const SX_PK_CMD_RSA_MOD_EXP_CM = &CMD_MOD_EXP_CM;

static const struct sx_pk_cmd_def CMD_MOD_EXP_CRT = {
	PK_OP_RSA_CRT, (1 << OP_SLOT_RSA_CRT_RESULT),
	(1 << OP_SLOT_RSA_CRT_INPUT) | (1 << OP_SLOT_RSA_CRT_P) | (1 << OP_SLOT_RSA_CRT_Q) |
		(1 << OP_SLOT_RSA_CRT_DP) | (1 << OP_SLOT_RSA_CRT_DQ) | (1 << OP_SLOT_RSA_CRT_QINV),
	0, SX_PK_OP_FLAGS_MOD_CM};
const struct sx_pk_cmd_def *const SX_PK_CMD_MOD_EXP_CRT = &CMD_MOD_EXP_CRT;

static const struct sx_pk_cmd_def CMD_RSA_KEYGEN = {
	PK_OP_RSA_KEYGEN,
	(1 << OP_SLOT_RSA_KEYGEN_N) | (1 << OP_SLOT_RSA_KEYGEN_L) | (1 << OP_SLOT_RSA_KEYGEN_D),
	(1 << OP_SLOT_RSA_KEYGEN_P) | (1 << OP_SLOT_RSA_KEYGEN_Q) | (1 << OP_SLOT_RSA_KEYGEN_E), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_RSA_KEYGEN = &CMD_RSA_KEYGEN;

static const struct sx_pk_cmd_def CMD_RSA_CRT_KEYPARAMS = {
	PK_OP_CRT_KEYPARAMS,
	(1 << OP_SLOT_CRT_KEYPARAMS_DP) | (1 << OP_SLOT_CRT_KEYPARAMS_DQ) |
		(1 << OP_SLOT_CRT_KEYPARAMS_QINV),
	(1 << OP_SLOT_CRT_KEYPARAMS_P) | (1 << OP_SLOT_CRT_KEYPARAMS_Q) |
		(1 << OP_SLOT_CRT_KEYPARAMS_D),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_RSA_CRT_KEYPARAMS = &CMD_RSA_CRT_KEYPARAMS;

static const struct sx_pk_cmd_def CMD_MILLER_RABIN = {
	PK_OP_MILLER_RABIN, 0, (1 << OP_SLOT_MILLER_RABIN_N) | (1 << OP_SLOT_MILLER_RABIN_A), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_MILLER_RABIN = &CMD_MILLER_RABIN;

static const struct sx_pk_cmd_def CMD_DSA_SIGN = {
	PK_OP_DSA_SIGN, (1 << OP_SLOT_DSA_R) | (1 << OP_SLOT_DSA_S),
	(1 << OP_SLOT_DSA_P) | (1 << OP_SLOT_DSA_Q) | (1 << OP_SLOT_DSA_G) | (1 << OP_SLOT_DSA_K) |
		(1 << OP_SLOT_DSA_X) | (1 << OP_SLOT_DSA_H),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_DSA_SIGN = &CMD_DSA_SIGN;

static const struct sx_pk_cmd_def CMD_DSA_VER = {
	PK_OP_DSA_VER, 0,
	(1 << OP_SLOT_DSA_P) | (1 << OP_SLOT_DSA_Q) | (1 << OP_SLOT_DSA_G) | (1 << OP_SLOT_DSA_Y) |
		(1 << OP_SLOT_DSA_R) | (1 << OP_SLOT_DSA_S) | (1 << OP_SLOT_DSA_H),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_DSA_VER = &CMD_DSA_VER;

static const struct sx_pk_cmd_def CMD_CHECK_PARAM_AB = {
	PK_OP_CHECK_PARAM_AB, 0,
	(1 << OP_SLOT_ECC_PARAM_P) | (1 << OP_SLOT_ECC_PARAM_A) | (1 << OP_SLOT_ECC_PARAM_B), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_CHECK_PARAM_AB = &CMD_CHECK_PARAM_AB;

static const struct sx_pk_cmd_def CMD_CHECK_PARAM_N = {
	PK_OP_CHECK_PARAM_N, 0, (1 << OP_SLOT_ECC_PARAM_P) | (1 << OP_SLOT_ECC_PARAM_N), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_CHECK_PARAM_N = &CMD_CHECK_PARAM_N;

static const struct sx_pk_cmd_def CMD_CHECK_XY = {
	PK_OP_CHECK_XY, 0, (1 << OP_SLOT_ECC_PARAM_P) | (3 << OP_SLOT_PTR_A), OP_SLOT_PTR_A};
const struct sx_pk_cmd_def *const SX_PK_CMD_CHECK_XY = &CMD_CHECK_XY;
