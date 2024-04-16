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

struct sx_pk_acq_req sx_async_x25519_ptmult_go(const struct sx_x25519_op *k,
					       const struct sx_x25519_op *pt)
{
	struct sx_pk_acq_req pkreq;
	const struct sx_pk_ecurve *curve = &sx_curve_x25519;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_MONTGOMERY_PTMUL);
	if (pkreq.status) {
		return pkreq;
	}
	struct sx_pk_inops_montgomery_mult inputs;

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}

	sx_wrpkmem(inputs.p.addr, pt->bytes, SX_X25519_OP_SZ);
	sx_wrpkmem(inputs.k.addr, k->bytes, SX_X25519_OP_SZ);

	sx_pk_run(pkreq.req);

	return pkreq;
}

void sx_async_x25519_ptmult_end(sx_pk_req *req, struct sx_x25519_op *r)
{
	const char **outputs = sx_pk_get_output_ops(req);

	sx_rdpkmem(r->bytes, outputs[0], SX_X25519_OP_SZ);

	sx_pk_release_req(req);
}

int sx_x25519_ptmult(const struct sx_x25519_op *k, const struct sx_x25519_op *pt,
		     struct sx_x25519_op *r)
{
	struct sx_pk_acq_req pkreq;
	uint32_t status;

	pkreq = sx_async_x25519_ptmult_go(k, pt);
	if (pkreq.status) {
		return pkreq.status;
	}
	status = sx_pk_wait(pkreq.req);
	sx_async_x25519_ptmult_end(pkreq.req, r);

	return status;
}

struct sx_pk_acq_req sx_async_x448_ptmult_go(const struct sx_x448_op *k,
					     const struct sx_x448_op *pt)
{
	struct sx_pk_acq_req pkreq;
	const struct sx_pk_ecurve *curve = &sx_curve_x448;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_MONTGOMERY_PTMUL);
	if (pkreq.status) {
		return pkreq;
	}
	struct sx_pk_inops_montgomery_mult inputs;

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}

	sx_wrpkmem(inputs.p.addr, pt->bytes, SX_X448_OP_SZ);
	sx_wrpkmem(inputs.k.addr, k->bytes, SX_X448_OP_SZ);

	sx_pk_run(pkreq.req);

	return pkreq;
}

void sx_async_x448_ptmult_end(sx_pk_req *req, struct sx_x448_op *r)
{
	const char **outputs = sx_pk_get_output_ops(req);

	sx_rdpkmem(r->bytes, outputs[0], SX_X448_OP_SZ);

	sx_pk_release_req(req);
}

int sx_x448_ptmult(const struct sx_x448_op *k, const struct sx_x448_op *pt, struct sx_x448_op *r)
{
	struct sx_pk_acq_req pkreq;
	uint32_t status;

	pkreq = sx_async_x448_ptmult_go(k, pt);
	if (pkreq.status) {
		return pkreq.status;
	}
	status = sx_pk_wait(pkreq.req);
	sx_async_x448_ptmult_end(pkreq.req, r);

	return status;
}
