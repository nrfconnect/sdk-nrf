/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stddef.h>
#include <silexpk/core.h>
#include <silexpk/ed448.h>
#include <silexpk/ec_curves.h>
#include <cracen/statuscodes.h>
#include <silexpk/iomem.h>
#include <silexpk/cmddefs/edwards.h>

/** Write a ED448 digest into a pair of operand slots.
 *
 * A ED448 digest has twice as many bytes as the normal operand size.
 *
 * \param op The digest bytes to write into the operand slot.
 * \param slots The pair of slots to write the operand into.
 */
static inline void write_ed448dgst(const struct sx_ed448_dgst *op, struct sx_pk_dblslot *slots)
{
	sx_wrpkmem(slots->a.addr, op->bytes, SX_ED448_SZ);
	sx_wrpkmem(slots->b.addr, op->bytes + SX_ED448_SZ, SX_ED448_SZ);
}

static inline void encode_eddsa_pt(const char *pxbuf, const char *pybuf, struct sx_ed448_pt *pt)
{
	sx_rdpkmem(pt->encoded, pybuf, sizeof(pt->encoded));
	pt->encoded[SX_ED448_SZ - 1] |= (pxbuf[0] & 1) << 7;
}

struct sx_pk_acq_req sx_async_ed448_ptmult_go(const struct sx_ed448_dgst *r)
{
	struct sx_pk_acq_req pkreq;
	const struct sx_pk_ecurve *curve = &sx_curve_ed448;
	struct sx_pk_inops_eddsa_ptmult inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_EDDSA_PTMUL);
	if (pkreq.status) {
		return pkreq;
	}
	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	write_ed448dgst(r, &inputs.r);
	sx_pk_run(pkreq.req);

	return pkreq;
}

void sx_async_ed448_ptmult_end(sx_pk_req *req, struct sx_ed448_pt *pt)
{
	const char **outputs = sx_pk_get_output_ops(req);

	encode_eddsa_pt(outputs[0], outputs[1], pt);

	sx_pk_release_req(req);
}

int sx_ed448_ptmult(const struct sx_ed448_dgst *r, struct sx_ed448_pt *pt)
{
	struct sx_pk_acq_req pkreq;
	uint32_t status;

	pkreq = sx_async_ed448_ptmult_go(r);
	if (pkreq.status) {
		return pkreq.status;
	}
	status = sx_pk_wait(pkreq.req);
	sx_async_ed448_ptmult_end(pkreq.req, pt);

	return status;
}

struct sx_pk_acq_req sx_pk_async_ed448_sign_go(const struct sx_ed448_dgst *k,
					       const struct sx_ed448_dgst *r,
					       const struct sx_ed448_v *s)
{
	struct sx_pk_acq_req pkreq;
	const struct sx_pk_ecurve *curve = &sx_curve_ed448;
	struct sx_pk_inops_eddsa_sign inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_EDDSA_SIGN);
	if (pkreq.status) {
		return pkreq;
	}
	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}

	write_ed448dgst(k, &inputs.k);
	write_ed448dgst(r, &inputs.r);
	sx_wrpkmem(inputs.s.addr, &s->bytes, sizeof(s->bytes));

	sx_pk_run(pkreq.req);

	return pkreq;
}

void sx_async_ed448_sign_end(sx_pk_req *req, struct sx_ed448_v *sig_s)
{
	const char **outputs = sx_pk_get_output_ops(req);

	sx_rdpkmem(&sig_s->bytes, outputs[0], sizeof(sig_s->bytes));

	sx_pk_release_req(req);
}

int sx_ed448_sign(const struct sx_ed448_dgst *k, const struct sx_ed448_dgst *r,
		  const struct sx_ed448_v *s, struct sx_ed448_v *sig_s)
{
	struct sx_pk_acq_req pkreq;
	uint32_t status;

	pkreq = sx_pk_async_ed448_sign_go(k, r, s);
	if (pkreq.status) {
		return pkreq.status;
	}
	status = sx_pk_wait(pkreq.req);
	sx_async_ed448_sign_end(pkreq.req, sig_s);

	return status;
}

/** Returns the least significant bit of the x coordinate of the encoded point*/
static inline int ed448_decode_pt_x(const struct sx_ed448_pt *pt)
{
	return (pt->encoded[SX_ED448_PT_SZ - 1] >> 7) & 1;
}

/** Write the y affine coordinate of an encoded ED448 point into memory */
static inline void ed448_pt_write_y(const struct sx_ed448_pt *pt, char *ay)
{
	sx_wrpkmem(ay, pt->encoded, SX_ED448_PT_SZ - 1);
	sx_wrpkmem_byte(&ay[SX_ED448_PT_SZ - 1], pt->encoded[SX_ED448_PT_SZ - 1] & 0x7f);
}

struct sx_pk_acq_req sx_async_ed448_verify_go(const struct sx_ed448_dgst *k,
					      const struct sx_ed448_pt *a,
					      const struct sx_ed448_v *sig_s,
					      const struct sx_ed448_pt *r)
{
	struct sx_pk_acq_req pkreq;
	const struct sx_pk_ecurve *curve = &sx_curve_ed448;
	uint32_t encodingflags = 0;
	struct sx_pk_inops_eddsa_ver inputs;

	if (ed448_decode_pt_x(a)) {
		encodingflags |= PK_OP_FLAGS_EDDSA_AX_LSB;
	}
	if (ed448_decode_pt_x(r)) {
		encodingflags |= PK_OP_FLAGS_EDDSA_RX_LSB;
	}
	pkreq = sx_pk_acquire_req(SX_PK_CMD_EDDSA_VER);
	if (pkreq.status) {
		return pkreq;
	}

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, encodingflags,
					      (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	write_ed448dgst(k, &inputs.k);
	ed448_pt_write_y(a, inputs.ay.addr);
	sx_wrpkmem(inputs.sig_s.addr, sig_s, sizeof(sig_s->bytes));
	ed448_pt_write_y(r, inputs.ry.addr);

	sx_pk_run(pkreq.req);

	return pkreq;
}

int sx_ed448_verify(const struct sx_ed448_dgst *k, const struct sx_ed448_pt *a,
		    const struct sx_ed448_v *sig_s, const struct sx_ed448_pt *r)
{
	struct sx_pk_acq_req pkreq;
	uint32_t status;

	pkreq = sx_async_ed448_verify_go(k, a, sig_s, r);
	if (pkreq.status) {
		return pkreq.status;
	}
	status = sx_pk_wait(pkreq.req);
	sx_pk_release_req(pkreq.req);

	return status;
}
