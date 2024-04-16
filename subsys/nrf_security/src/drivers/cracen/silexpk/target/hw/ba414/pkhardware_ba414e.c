/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/membarriers.h>

#include <stdint.h>
#include <assert.h>

#include <zephyr/sys/util.h>

#include <silexpk/core.h>
#include "regs_addr.h"
#include <internal.h>
#include "op_slots.h"
#include "regs_commands.h"
#include "pkhardware_ba414e.h"
#include "ba414_status.h"
#include <silexpk/iomem.h>
#include <cracen/statuscodes.h>
#include "../../include/sx_regs.h"
#include <silexpk/cmddefs/rsa.h>
#include <silexpk/cmddefs/modmath.h>
#include <silexpk/blinding.h>

/** Select which operand slots to use in crypto-RAM
 *
 * For some operations, the slots in crypto-RAM can be configured. Force the
 * slot to be used as:
 *   + operand slot A: 6
 *   + operand slot B:  8
 *   + operand slot C (result):  10
 *
 * \param req The acceleration request.
 */
void sx_pk_select_ops(sx_pk_req *req)
{
	int slotA = req->cmd->selected_ptrA;

	if (slotA == 0) {
		return;
	}
	sx_pk_wrreg(&req->regs, PK_REG_CONFIG,
		    PK_RB_CONFIG_PTRS(slotA, OP_SLOT_PTR_B, OP_SLOT_PTR_C));
}

int sx_pk_get_opsize(sx_pk_req *req)
{
	return req->op_size;
}

void sx_pk_write_curve(sx_pk_req *req, const struct sx_pk_ecurve *curve)
{
	int i;
	char *dst = req->cryptoram;
	int slot_size = req->slot_sz;
	const char *src = curve->params;

	if (curve->curveflags & PK_OP_FLAGS_SELCUR_MASK) {
		/* Predefined curves have curve parameters hardcoded in hardware
		 */
		return;
	}
	if (req->cmd->cmdcode & SX_PK_OP_FLAGS_BIGENDIAN) {
		/* In big endian mode, the operands should be put at the end
		 * of the slot.
		 */
		dst += slot_size - curve->sz;
	}
	int curve_param_count = sx_pk_count_curve_params(curve);

	for (i = 0; i < curve_param_count; i++) {
		sx_wrpkmem(dst, src, curve->sz);
		src += curve->sz;
		dst += slot_size;
	}
}

void sx_pk_write_curve_gen(sx_pk_req *pk, const struct sx_pk_ecurve *curve, struct sx_pk_slot px,
			   struct sx_pk_slot py)
{
	(void)pk;
	const char *src = curve->params + curve->sz * 2;

	sx_wrpkmem(px.addr, src, curve->sz);
	src += curve->sz;
	sx_wrpkmem(py.addr, src, curve->sz);
}

const char **sx_pk_get_output_ops(sx_pk_req *req)
{
	int slots = req->cmd->outslots;
	char *cryptoram = req->cryptoram;
	int slot_size = req->slot_sz;
	unsigned int i = 0;

	if (req->cmd->cmdcode & SX_PK_OP_FLAGS_BIGENDIAN) {
		/* In big endian mode, the operands should be put at the end
		 * of the slot.
		 */
		cryptoram += slot_size - req->op_size;
	}
	while (slots) {
		if (slots & 1) {
			assert(i < ARRAY_SIZE(req->outputops));
			req->outputops[i++] = cryptoram;
		}
		cryptoram += slot_size;
		slots >>= 1;
	}

	return req->outputops;
}

/** Test if instance is in IK mode or not */
static int sx_pk_ik_mode(sx_pk_req *pk)
{
	return pk->ik_mode;
}

static void write_command(sx_pk_req *req, int op_size, uint32_t flags)
{
	uint32_t command = req->cmd->cmdcode | ((op_size - 1) << 8) | flags;

	sx_pk_wrreg(&req->regs, PK_REG_COMMAND, command);
}

static void sx_pk_blind(sx_pk_req *req, sx_pk_blind_factor factor)
{
	/* Casting the factor into char*. This way the LSB
	 * can be changed regardless of the CPU endianness
	 */
	char *p = (char *)(&factor);
	int slot_sz = req->slot_sz;
	char *cryptoram = req->cryptoram + slot_sz * 16;
	const int blind_sz = sizeof(factor);

	if (req->cmd->cmdcode & SX_PK_OP_FLAGS_BIGENDIAN) {
		/* Force lsb bit to guarantee odd random value */
		p[7] |= 1;
		/* In big endian mode, the operands should be put at the end
		 * of the slot.
		 */
		sx_clrpkmem(cryptoram - req->op_size, req->op_size - blind_sz);
		cryptoram -= blind_sz;
	} else {
		/* Force lsb bit to guarantee odd random value */
		p[0] |= 1;
		cryptoram -= slot_sz;
		sx_clrpkmem(cryptoram + blind_sz, req->op_size - blind_sz);
	}
	sx_wrpkmem(cryptoram, &factor, blind_sz);
}

int sx_pk_list_ecc_inslots(sx_pk_req *req, const struct sx_pk_ecurve *curve, int flags,
			   struct sx_pk_slot *inputs)
{
	int slots = req->cmd->inslots;
	char *cryptoram = req->cryptoram;
	int slot_size = req->slot_sz;
	int i = 0;
	const struct sx_pk_capabilities *caps;
	struct sx_pk_blinder *bldr = *sx_pk_get_blinder(req->cnx);

	if (sx_pk_ik_mode(req)) {
		sx_pk_release_req(req);
		return SX_ERR_IK_MODE;
	}

	caps = sx_pk_fetch_capabilities();
	int max_opsz = 0;

	if (curve->curveflags & 0x80) {
		max_opsz = caps->max_gfb_opsz;
	} else {
		max_opsz = caps->max_gfp_opsz;
	}
	if (curve->sz > max_opsz) {
		sx_pk_release_req(req);
		return SX_ERR_OPERAND_TOO_LARGE;
	}
	req->op_size = curve->sz;

	if (bldr && req->cmd->blind_flags) {
		flags |= req->cmd->blind_flags;
	}
	write_command(req, curve->sz, curve->curveflags | flags);
	if (req->cmd->cmdcode & SX_PK_OP_FLAGS_BIGENDIAN) {
		/* In big endian mode, the operands should be put at the end
		 * of the slot.
		 */
		cryptoram += slot_size - req->op_size;
	}
	while (slots) {
		if (slots & 1) {
			inputs[i++].addr = cryptoram;
		}
		cryptoram += slot_size;
		slots >>= 1;
	}
	sx_pk_write_curve(req, curve);

	if (bldr && req->cmd->blind_flags) {
		sx_pk_blind(req, bldr->generate(bldr));
	}
	return 0;
}

static int sx_pk_gfcmd_opsize(const struct sx_pk_cmd_def *cmd, const int *opsizes)
{
	if (cmd == SX_PK_CMD_MOD_EXP_CRT || cmd == SX_PK_CMD_RSA_KEYGEN ||
	    cmd == SX_PK_CMD_RSA_CRT_KEYPARAMS || cmd == SX_PK_CMD_MULT) {
		return opsizes[0] + opsizes[1];
	}
	if (cmd == SX_PK_CMD_ODD_MOD_REDUCE) {
		return MAX(opsizes[0], opsizes[1]);
	}

	return opsizes[0];
}

int sx_pk_list_gfp_inslots(sx_pk_req *req, const int *opsizes, struct sx_pk_slot *inputs)
{
	int slots = req->cmd->inslots;
	char *cryptoram = req->cryptoram;
	int slot_size = req->slot_sz;
	int i = 0;
	const struct sx_pk_capabilities *caps;
	struct sx_pk_blinder *bldr = *sx_pk_get_blinder(req->cnx);
	uint32_t flags = 0;

	if (sx_pk_ik_mode(req)) {
		sx_pk_release_req(req);
		return SX_ERR_IK_MODE;
	}

	req->op_size = sx_pk_gfcmd_opsize(req->cmd, opsizes);
	caps = sx_pk_fetch_capabilities();
	if (req->op_size > caps->max_gfp_opsz) {
		sx_pk_release_req(req);
		return SX_ERR_OPERAND_TOO_LARGE;
	}

	if (bldr && req->cmd->blind_flags) {
		flags |= req->cmd->blind_flags;
	}
	write_command(req, req->op_size, flags);
	if (req->cmd->cmdcode & SX_PK_OP_FLAGS_BIGENDIAN) {
		/* In big endian mode, the operands should be put at the end
		 * of the slot.
		 */
		cryptoram += slot_size - req->op_size;
		for (; slots; slots >>= 1, cryptoram += slot_size) {
			if (slots & 1) {
				if (opsizes[i] > req->op_size) {
					sx_pk_release_req(req);
					return SX_ERR_OPERAND_TOO_LARGE;
				}
				sx_clrpkmem(cryptoram, req->op_size - opsizes[i]);
				inputs[i].addr = cryptoram + req->op_size - opsizes[i];
				i++;
			}
		}
	}
	for (; slots; slots >>= 1, cryptoram += slot_size) {
		if (slots & 1) {
			if (opsizes[i] > req->op_size) {
				sx_pk_release_req(req);
				return SX_ERR_OPERAND_TOO_LARGE;
			}
			inputs[i].addr = cryptoram;
			sx_clrpkmem(cryptoram + opsizes[i], req->op_size - opsizes[i]);
			i++;
		}
	}
	if (flags) {
		sx_pk_blind(req, bldr->generate(bldr));
	}

	return 0;
}
