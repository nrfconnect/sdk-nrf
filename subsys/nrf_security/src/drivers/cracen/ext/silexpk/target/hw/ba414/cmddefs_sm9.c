/*
 * Copyright (c) 2018-2020 Silex Insight sa
 * Copyright (c) 2018 Beerten Engineering scs
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <silexpk/core.h>
#include "op_slots.h"
#include "regs_commands.h"
#include "pkhardware_ba414e.h"

static const struct sx_pk_cmd_def CMD_SM9_EXP = {
	PK_OP_SM9_EXP, (0xfff << OP_SLOT_SM9_R_12),
	(1 << OP_SLOT_SM9_H) | (1 << OP_SLOT_SM9_T) | (0xfff << OP_SLOT_SM9_R_12), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_EXP = &CMD_SM9_EXP;

static const struct sx_pk_cmd_def CMD_SM9_PMULG1 = {
	PK_OP_SM9_PMULG1, (1 << OP_SLOT_SM9_PPUBE_X0) | (1 << OP_SLOT_SM9_PPUBE_Y0),
	(1 << OP_SLOT_SM9_P1_X0) | (1 << OP_SLOT_SM9_P1_Y0) | (1 << OP_SLOT_SM9_KE) |
		(1 << OP_SLOT_SM9_T),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_PMULG1 = &CMD_SM9_PMULG1;

static const struct sx_pk_cmd_def CMD_SM9_PMULG2 = {
	PK_OP_SM9_PMULG2,
	(1 << OP_SLOT_SM9_PPUBS_X0) | (1 << OP_SLOT_SM9_PPUBS_Y0) | (1 << OP_SLOT_SM9_PPUBS_X1) |
		(1 << OP_SLOT_SM9_PPUBS_Y1),
	(1 << OP_SLOT_SM9_P2_X0) | (1 << OP_SLOT_SM9_P2_Y0) | (1 << OP_SLOT_SM9_P2_X1) |
		(1 << OP_SLOT_SM9_P2_Y1) | (1 << OP_SLOT_SM9_KE) | (1 << OP_SLOT_SM9_T),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_PMULG2 = &CMD_SM9_PMULG2;

static const struct sx_pk_cmd_def CMD_SM9_PAIR = {
	PK_OP_SM9_PAIR, (0xfff << OP_SLOT_SM9_R_12),
	(1 << OP_SLOT_SM9_PX0) | (1 << OP_SLOT_SM9_PY0) | (1 << OP_SLOT_SM9_QX0) |
		(1 << OP_SLOT_SM9_QX1) | (1 << OP_SLOT_SM9_QY0) | (1 << OP_SLOT_SM9_QY1) |
		(1 << OP_SLOT_SM9_F) | (1 << OP_SLOT_SM9_T),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_PAIR = &CMD_SM9_PAIR;

static const struct sx_pk_cmd_def CMD_SM9_PRIVSIGKEYGEN = {
	PK_OP_SM9_PRIVSIGKEYGEN, (1 << OP_SLOT_SM9_DS_X0) | (1 << OP_SLOT_SM9_DS_Y0),
	(1 << OP_SLOT_SM9_P1_X0) | (1 << OP_SLOT_SM9_P1_Y0) | (1 << OP_SLOT_SM9_H) |
		(1 << OP_SLOT_SM9_KS) | (1 << OP_SLOT_SM9_T),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_PRIVSIGKEYGEN = &CMD_SM9_PRIVSIGKEYGEN;

static const struct sx_pk_cmd_def CMD_SM9_SIGNATUREGEN = {
	PK_OP_SM9_SIGNATUREGEN, (1 << OP_SLOT_SM9_S_X0) | (1 << OP_SLOT_SM9_S_Y0),
	(1 << OP_SLOT_SM9_DS_X0) | (1 << OP_SLOT_SM9_DS_Y0) | (1 << OP_SLOT_SM9_H) |
		(1 << OP_SLOT_SM9_R) | (1 << OP_SLOT_SM9_T),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_SIGNATUREGEN = &CMD_SM9_SIGNATUREGEN;

static const struct sx_pk_cmd_def CMD_SM9_SIGNATUREVERIFY = {
	PK_OP_SM9_SIGNATUREVERIFY, (0xfff << OP_SLOT_SM9_R_12),
	(1 << OP_SLOT_SM9_H1) | (1 << OP_SLOT_SM9_P2_X0) | (1 << OP_SLOT_SM9_P2_X1) |
		(1 << OP_SLOT_SM9_P2_Y0) | (1 << OP_SLOT_SM9_P2_Y1) | (1 << OP_SLOT_SM9_PPUBS_X0) |
		(1 << OP_SLOT_SM9_PPUBS_X1) | (1 << OP_SLOT_SM9_PPUBS_Y0) |
		(1 << OP_SLOT_SM9_PPUBS_Y1) | (1 << OP_SLOT_SM9_SX0) | (1 << OP_SLOT_SM9_SY0) |
		(1 << OP_SLOT_SM9_H) | (1 << OP_SLOT_SM9_F) | (1 << OP_SLOT_SM9_T) |
		(0xfff << OP_SLOT_SM9_R_12),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_SIGNATUREVERIFY = &CMD_SM9_SIGNATUREVERIFY;

static const struct sx_pk_cmd_def CMD_SM9_PRIVENCRKEYGEN = {
	PK_OP_SM9_PRIVENCRKEYGEN,
	(1 << OP_SLOT_SM9_OUTDE_X0) | (1 << OP_SLOT_SM9_OUTDE_X1) | (1 << OP_SLOT_SM9_OUTDE_Y0) |
		(1 << OP_SLOT_SM9_OUTDE_Y1),
	(1 << OP_SLOT_SM9_P2_X0) | (1 << OP_SLOT_SM9_P2_X1) | (1 << OP_SLOT_SM9_P2_Y0) |
		(1 << OP_SLOT_SM9_P2_Y1) | (1 << OP_SLOT_SM9_H) | (1 << OP_SLOT_SM9_KE) |
		(1 << OP_SLOT_SM9_T),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_PRIVENCRKEYGEN = &CMD_SM9_PRIVENCRKEYGEN;

static const struct sx_pk_cmd_def CMD_SM9_SENDKEY = {
	PK_OP_SM9_SENDKEY, (1 << OP_SLOT_SM9_PPUBE_RX0) | (1 << OP_SLOT_SM9_PPUBE_RY0),
	(1 << OP_SLOT_SM9_P1_X0) | (1 << OP_SLOT_SM9_P1_Y0) | (1 << OP_SLOT_SM9_PPUBE_X0) |
		(1 << OP_SLOT_SM9_PPUBE_Y0) | (1 << OP_SLOT_SM9_H) | (1 << OP_SLOT_SM9_R) |
		(1 << OP_SLOT_SM9_T),
	0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_SENDKEY = &CMD_SM9_SENDKEY;

static const struct sx_pk_cmd_def CMD_SM9_REDUCEH = {
	PK_OP_SM9_REDUCEH, (1 << OP_SLOT_SM9_H), (1 << OP_SLOT_SM9_T) | (1 << OP_SLOT_SM9_H), 0};
const struct sx_pk_cmd_def *const SX_PK_CMD_SM9_REDUCEH = &CMD_SM9_REDUCEH;
