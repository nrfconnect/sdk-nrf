/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stddef.h>
#include <silexpk/core.h>
#include <silexpk/montgomery.h>
#include <silexpk/ec_curves.h>
#include <silexpk/iomem.h>
#include <cracen/statuscodes.h>
#include <silexpk/cmddefs/ecc.h>
#include <string.h>

int sx_x25519_ptmult(sx_pk_req *req, const struct sx_x25519_op *k,
		const struct sx_x25519_op *pt, struct sx_x25519_op *r)
{
	int status;

	const struct sx_pk_ecurve *curve = &sx_curve_x25519;
	struct sx_pk_inops_montgomery_mult inputs;

	sx_pk_set_cmd(req, SX_PK_CMD_MONTGOMERY_PTMUL);

	status = sx_pk_list_ecc_inslots(req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	sx_wrpkmem(inputs.p.addr, pt->bytes, SX_X25519_OP_SZ);
	sx_wrpkmem(inputs.k.addr, k->bytes, SX_X25519_OP_SZ);
	/* clamp the scalar */
	sx_wrpkmem_byte(&inputs.k.addr[31], (inputs.k.addr[31] | 1 << 6) & 0x7f);
	sx_wrpkmem_byte(&inputs.k.addr[0], inputs.k.addr[0] & 0xF8);

	/* clamp the pt */
	sx_wrpkmem_byte(&inputs.p.addr[31], inputs.p.addr[31] & 0x7f);

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);

	sx_rdpkmem(r->bytes, outputs[0], SX_X25519_OP_SZ);

	return status;
}

int sx_x448_ptmult(sx_pk_req *req, const struct sx_x448_op *k,
		const struct sx_x448_op *pt, struct sx_x448_op *r)
{
	int status;

	const struct sx_pk_ecurve *curve = &sx_curve_x448;
	struct sx_pk_inops_montgomery_mult inputs;

	sx_pk_set_cmd(req, SX_PK_CMD_MONTGOMERY_PTMUL);

	status = sx_pk_list_ecc_inslots(req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	sx_wrpkmem(inputs.p.addr, pt->bytes, SX_X448_OP_SZ);
	sx_wrpkmem(inputs.k.addr, k->bytes, SX_X448_OP_SZ);
	/* clamp the scalar */
	sx_wrpkmem_byte(&inputs.k.addr[55], inputs.k.addr[55] | 0x80);
	sx_wrpkmem_byte(&inputs.k.addr[0], inputs.k.addr[0] & 0xFC);

	sx_pk_run(req);

	status = sx_pk_wait(req);

	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);

	sx_rdpkmem(r->bytes, outputs[0], SX_X448_OP_SZ);

	return status;
}
