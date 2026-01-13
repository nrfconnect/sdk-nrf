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

int sx_async_x25519_ptmult_go(sx_pk_req *req, const struct sx_x25519_op *k,
			      const struct sx_x25519_op *pt)
{
	const struct sx_pk_ecurve *curve = &sx_curve_x25519;
	struct sx_pk_inops_montgomery_mult inputs;
	int status;

	sx_pk_acquire_hw(req);
	sx_pk_set_cmd(req, SX_PK_CMD_MONTGOMERY_PTMUL);

	status = sx_pk_list_ecc_inslots(req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (status) {
		sx_pk_release_req(req);
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

	return SX_OK;
}

void sx_async_x25519_ptmult_end(sx_pk_req *req, struct sx_x25519_op *r)
{
	const uint8_t **outputs = sx_pk_get_output_ops(req);

	sx_rdpkmem(r->bytes, outputs[0], SX_X25519_OP_SZ);

	sx_pk_release_req(req);
}

int sx_x25519_ptmult(const struct sx_x25519_op *k, const struct sx_x25519_op *pt,
		     struct sx_x25519_op *r)
{
	sx_pk_req req;
	int status;

	status = sx_async_x25519_ptmult_go(&req, k, pt);
	if (status) {
		return status;
	}
	status = sx_pk_wait(&req);
	sx_async_x25519_ptmult_end(&req, r);

	return status;
}

int sx_async_x448_ptmult_go(sx_pk_req *req, const struct sx_x448_op *k,
			    const struct sx_x448_op *pt)
{
	const struct sx_pk_ecurve *curve = &sx_curve_x448;
	struct sx_pk_inops_montgomery_mult inputs;
	int status;

	sx_pk_acquire_hw(req);
	sx_pk_set_cmd(req, SX_PK_CMD_MONTGOMERY_PTMUL);

	status = sx_pk_list_ecc_inslots(req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (status) {
		sx_pk_release_req(req);
		return status;
	}

	sx_wrpkmem(inputs.p.addr, pt->bytes, SX_X448_OP_SZ);
	sx_wrpkmem(inputs.k.addr, k->bytes, SX_X448_OP_SZ);
	/* clamp the scalar */
	sx_wrpkmem_byte(&inputs.k.addr[55], inputs.k.addr[55] | 0x80);
	sx_wrpkmem_byte(&inputs.k.addr[0], inputs.k.addr[0] & 0xFC);

	sx_pk_run(req);

	return SX_OK;
}

void sx_async_x448_ptmult_end(sx_pk_req *req, struct sx_x448_op *r)
{
	const uint8_t **outputs = sx_pk_get_output_ops(req);

	sx_rdpkmem(r->bytes, outputs[0], SX_X448_OP_SZ);

	sx_pk_release_req(req);
}

int sx_x448_ptmult(const struct sx_x448_op *k, const struct sx_x448_op *pt, struct sx_x448_op *r)
{
	sx_pk_req req;
	int status;

	status = sx_async_x448_ptmult_go(&req, k, pt);
	if (status) {
		return status;
	}
	status = sx_pk_wait(&req);
	sx_async_x448_ptmult_end(&req, r);

	return status;
}
