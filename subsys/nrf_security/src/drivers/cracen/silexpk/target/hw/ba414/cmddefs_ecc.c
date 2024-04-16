/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/core.h>
#include "op_slots.h"
#include "regs_commands.h"
#include "pkhardware_ba414e.h"

static const struct sx_pk_cmd_def CMD_MG_PTMUL = {PK_OP_MG_PTMUL, (1 << OP_SLOT_PTR_C),
						  (1 << OP_SLOT_PTR_A) | (1 << OP_SLOT_PTR_B),
						  OP_SLOT_PTR_A};
const struct sx_pk_cmd_def *const SX_PK_CMD_MONTGOMERY_PTMUL = &CMD_MG_PTMUL;

static const struct sx_pk_cmd_def CMD_ECDSA_VER = {
	PK_OP_ECDSA_VER, 0,
	(1 << OP_SLOT_ECDSA_VER_QX) | (1 << OP_SLOT_ECDSA_VER_QY) | (1 << OP_SLOT_ECDSA_VER_R) |
		(1 << OP_SLOT_ECDSA_VER_S) | (1 << OP_SLOT_ECDSA_VER_H),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECDSA_VER = &CMD_ECDSA_VER;

static const struct sx_pk_cmd_def CMD_ECDSA_GEN = {
	PK_OP_ECDSA_GEN, (1 << OP_SLOT_ECDSA_SGN_R) | (1 << OP_SLOT_ECDSA_SGN_S),
	(1 << OP_SLOT_ECDSA_SGN_D) | (1 << OP_SLOT_ECDSA_SGN_K) | (1 << OP_SLOT_ECDSA_SGN_H), 0,
	SX_PK_OP_FLAGS_ECC_CM};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECDSA_GEN = &CMD_ECDSA_GEN;

static const struct sx_pk_cmd_def CMD_ECKCDSA_PUBKEY_GEN = {
	PK_OP_ECKCDSA_PUBKEY_GEN, (1 << OP_SLOT_ECKCDSA_QX) | (1 << OP_SLOT_ECKCDSA_QY),
	(1 << OP_SLOT_ECKCDSA_D), 0, SX_PK_OP_FLAGS_ECC_CM};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECKCDSA_PUBKEY_GEN = &CMD_ECKCDSA_PUBKEY_GEN;

static const struct sx_pk_cmd_def CMD_ECKCDSA_SIGN = {
	PK_OP_ECKCDSA_SIGN, (1 << OP_SLOT_ECKCDSA_S),
	(1 << OP_SLOT_ECKCDSA_D) | (1 << OP_SLOT_ECKCDSA_K) | (1 << OP_SLOT_ECKCDSA_R) |
		(1 << OP_SLOT_ECKCDSA_H),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECKCDSA_SIGN = &CMD_ECKCDSA_SIGN;

static const struct sx_pk_cmd_def CMD_ECKCDSA_VER = {
	PK_OP_ECKCDSA_VER, (1 << OP_SLOT_ECKCDSA_WX) | (1 << OP_SLOT_ECKCDSA_WY),
	(1 << OP_SLOT_ECKCDSA_QX) | (1 << OP_SLOT_ECKCDSA_QY) | (1 << OP_SLOT_ECKCDSA_R) |
		(1 << OP_SLOT_ECKCDSA_S) | (1 << OP_SLOT_ECKCDSA_H),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECKCDSA_VER = &CMD_ECKCDSA_VER;

static const struct sx_pk_cmd_def CMD_ECC_PT_ADD = {
	PK_OP_ECC_PT_ADD,
	(3 << OP_SLOT_ECC_PT_ADD_R),
	(3 << OP_SLOT_ECC_PT_ADD_P1) | (3 << OP_SLOT_ECC_PT_ADD_P2),
	OP_SLOT_ECC_PT_ADD_P1,
};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECC_PT_ADD = &CMD_ECC_PT_ADD;

static const struct sx_pk_cmd_def CMD_SM2_VER = {
	PK_OP_SM2_VER, 0,
	(3 << OP_SLOT_SM2_Q) | (1 << OP_SLOT_SM2_R) | (1 << OP_SLOT_SM2_S) | (1 << OP_SLOT_SM2_H),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM2_VER = &CMD_SM2_VER;

static const struct sx_pk_cmd_def CMD_ECC_PTMUL = {PK_OP_ECC_PTMUL, (3 << OP_SLOT_ECC_PTMUL_R),
						   (1 << OP_SLOT_ECC_PTMUL_K) |
							   (3 << OP_SLOT_ECC_PTMUL_P),
						   OP_SLOT_ECC_PTMUL_P, SX_PK_OP_FLAGS_ECC_CM};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECC_PTMUL = &CMD_ECC_PTMUL;

static const struct sx_pk_cmd_def CMD_SM2_EXCH = {
	PK_OP_SM2_EXCH, (3 << OP_SLOT_SM2_U),
	(1 << OP_SLOT_SM2_D) | (1 << OP_SLOT_SM2_K) | (3 << OP_SLOT_SM2_Q) | (3 << OP_SLOT_SM2_RB) |
		(1 << OP_SLOT_SM2_COF) | (1 << OP_SLOT_SM2_RA) | (1 << OP_SLOT_SM2_W),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM2_EXCH = &CMD_SM2_EXCH;

static const struct sx_pk_cmd_def CMD_ECC_PT_DOUBLE = {
	PK_OP_ECC_PT_DOUBLE, (3 << OP_SLOT_ECC_PT_DOUBLE_R), (3 << OP_SLOT_ECC_PT_DOUBLE_P),
	OP_SLOT_ECC_PT_DOUBLE_P};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECC_PT_DOUBLE = &CMD_ECC_PT_DOUBLE;

static const struct sx_pk_cmd_def CMD_ECC_PTONCURVE = {
	PK_OP_ECC_PTONCURVE, 0, (3 << OP_SLOT_ECC_PTONCURVE_P), OP_SLOT_ECC_PTONCURVE_P};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECC_PTONCURVE = &CMD_ECC_PTONCURVE;

static const struct sx_pk_cmd_def CMD_EDDSA_PTMUL = {
	PK_OP_EDDSA_PTMUL, (1 << OP_SLOT_EDDSA_PTMUL_RX) | (1 << OP_SLOT_EDDSA_PTMUL_RY),
	(3 << OP_SLOT_EDDSA_PTMUL_R), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_EDDSA_PTMUL = &CMD_EDDSA_PTMUL;

static const struct sx_pk_cmd_def CMD_EDWARDS_PTMUL = {
	PK_OP_EDWARDS_PTMUL, (1 << OP_SLOT_EDDSA_PTMUL_RX) | (1 << OP_SLOT_EDDSA_PTMUL_RY),
	(3 << OP_SLOT_EDDSA_PTMUL_R) | (3 << OP_SLOT_EDDSA_PTMUL_P), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_EDWARDS_PTMUL = &CMD_EDWARDS_PTMUL;

static const struct sx_pk_cmd_def CMD_EDDSA_SIGN = {
	PK_OP_EDDSA_SIGN, (1 << OP_SLOT_EDDSA_SIGN_SIG_S),
	(3 << OP_SLOT_EDDSA_SIGN_K) | (3 << OP_SLOT_EDDSA_SIGN_R) | (1 << OP_SLOT_EDDSA_SIGN_S), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_EDDSA_SIGN = &CMD_EDDSA_SIGN;

static const struct sx_pk_cmd_def CMD_SM2_GEN = {
	PK_OP_SM2_GEN, (1 << OP_SLOT_SM2_R) | (1 << OP_SLOT_SM2_S),
	(1 << OP_SLOT_SM2_D) | (1 << OP_SLOT_SM2_K) | (1 << OP_SLOT_SM2_H), 0,
	SX_PK_OP_FLAGS_ECC_CM};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM2_GEN = &CMD_SM2_GEN;

static const struct sx_pk_cmd_def CMD_EDDSA_VER = {
	PK_OP_EDDSA_VERIFY, 0,
	(3 << OP_SLOT_EDDSA_VER_K) | (1 << OP_SLOT_EDDSA_VER_AY) | (1 << OP_SLOT_EDDSA_VER_S) |
		(1 << OP_SLOT_EDDSA_VER_RY),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_EDDSA_VER = &CMD_EDDSA_VER;

static const struct sx_pk_cmd_def CMD_ECC_PT_DECOMP = {PK_OP_ECC_PT_DECOMP, (1 << OP_SLOT_PTR_C),
						       (1 << OP_SLOT_PTR_A), OP_SLOT_PTR_A};
const struct sx_pk_cmd_def *const SX_PK_CMD_ECC_PT_DECOMP = &CMD_ECC_PT_DECOMP;
