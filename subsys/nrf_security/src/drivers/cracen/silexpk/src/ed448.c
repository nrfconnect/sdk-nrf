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

/** Write a Ed448 digest into a pair of operand slots.
 *
 * A Ed448 digest has twice as many bytes as the normal operand size.
 *
 * \param op The digest bytes to write into the operand slot.
 * \param slots The pair of slots to write the operand into.
 */
static inline void write_ed448dgst(const struct sx_ed448_dgst *op, struct sx_pk_dblslot *slots)
{
	sx_wrpkmem(slots->a.addr, op->bytes, SX_ED448_SZ);
	sx_wrpkmem(slots->b.addr, op->bytes + SX_ED448_SZ, SX_ED448_SZ);
}

static inline void encode_eddsa_pt(const uint8_t *pxbuf, const uint8_t *pybuf,
				   struct sx_ed448_pt *pt)
{
	sx_rdpkmem(pt->encoded, pybuf, sizeof(pt->encoded));
	pt->encoded[SX_ED448_SZ - 1] |= (pxbuf[0] & 1) << 7;
}

static int ed448_ptmult_run(sx_pk_req *req, const struct sx_ed448_dgst *r)
{
	const struct sx_pk_ecurve *curve = &sx_curve_ed448;
	struct sx_pk_inops_eddsa_ptmult inputs;

	int status = sx_pk_list_ecc_inslots(req, curve, 0, (struct sx_pk_slot *)&inputs);

	if (status != SX_OK) {
		return status;
	}
	write_ed448dgst(r, &inputs.r);
	sx_pk_run(req);

	return SX_OK;
}

static void ed448_ptmult_read(sx_pk_req *req, struct sx_ed448_pt *pt)
{
	const uint8_t **outputs = sx_pk_get_output_ops(req);

	encode_eddsa_pt(outputs[0], outputs[1], pt);
}

int sx_ed448_ptmult(sx_pk_req *req, const struct sx_ed448_dgst *r, struct sx_ed448_pt *pt)
{
	sx_pk_set_cmd(req, SX_PK_CMD_EDDSA_PTMUL);

	int status = ed448_ptmult_run(req, r);

	if (status != SX_OK) {
		return status;
	}
	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}
	ed448_ptmult_read(req, pt);

	return SX_OK;
}

static int ed448_sign_run(sx_pk_req *req, const struct sx_ed448_dgst *k,
			  const struct sx_ed448_dgst *r, const struct sx_ed448_v *s)
{
	const struct sx_pk_ecurve *curve = &sx_curve_ed448;
	struct sx_pk_inops_eddsa_sign inputs;

	int status = sx_pk_list_ecc_inslots(req, curve, 0, (struct sx_pk_slot *)&inputs);

	if (status != SX_OK) {
		return status;
	}

	write_ed448dgst(k, &inputs.k);
	write_ed448dgst(r, &inputs.r);
	sx_wrpkmem(inputs.s.addr, &s->bytes, sizeof(s->bytes));

	sx_pk_run(req);

	return SX_OK;
}

static void ed448_sign_read(sx_pk_req *req, struct sx_ed448_v *sig_s)
{
	const uint8_t **outputs = sx_pk_get_output_ops(req);

	sx_rdpkmem(&sig_s->bytes, outputs[0], sizeof(sig_s->bytes));
}

int sx_ed448_sign(sx_pk_req *req, const struct sx_ed448_dgst *k, const struct sx_ed448_dgst *r,
		  const struct sx_ed448_v *s, struct sx_ed448_v *sig_s)
{
	sx_pk_set_cmd(req, SX_PK_CMD_EDDSA_SIGN);

	int status = ed448_sign_run(req, k, r, s);

	if (status != SX_OK) {
		return status;
	}
	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}
	ed448_sign_read(req, sig_s);

	return SX_OK;
}

/** Returns the least significant bit of the x coordinate of the encoded point*/
static inline int ed448_decode_pt_x(const struct sx_ed448_pt *pt)
{
	return (pt->encoded[SX_ED448_PT_SZ - 1] >> 7) & 1;
}

/** Write the y affine coordinate of an encoded Ed448 point into memory */
static inline void ed448_pt_write_y(const struct sx_ed448_pt *pt, uint8_t *ay)
{
	sx_wrpkmem(ay, pt->encoded, SX_ED448_PT_SZ - 1);
	sx_wrpkmem_byte(&ay[SX_ED448_PT_SZ - 1], pt->encoded[SX_ED448_PT_SZ - 1] & 0x7f);
}

static int ed448_verify_run(sx_pk_req *req, const struct sx_ed448_dgst *k,
			    const struct sx_ed448_pt *a, const struct sx_ed448_v *sig_s,
			    const struct sx_ed448_pt *r)
{
	const struct sx_pk_ecurve *curve = &sx_curve_ed448;
	uint32_t encodingflags = 0;
	struct sx_pk_inops_eddsa_ver inputs;

	if (ed448_decode_pt_x(a)) {
		encodingflags |= PK_OP_FLAGS_EDDSA_AX_LSB;
	}
	if (ed448_decode_pt_x(r)) {
		encodingflags |= PK_OP_FLAGS_EDDSA_RX_LSB;
	}

	int status = sx_pk_list_ecc_inslots(req, curve, encodingflags,
					    (struct sx_pk_slot *)&inputs);

	if (status != SX_OK) {
		return status;
	}
	write_ed448dgst(k, &inputs.k);
	ed448_pt_write_y(a, inputs.ay.addr);
	sx_wrpkmem(inputs.sig_s.addr, sig_s, sizeof(sig_s->bytes));
	ed448_pt_write_y(r, inputs.ry.addr);

	sx_pk_run(req);

	return SX_OK;
}

int sx_ed448_verify(sx_pk_req *req, const struct sx_ed448_dgst *k, const struct sx_ed448_pt *a,
		    const struct sx_ed448_v *sig_s, const struct sx_ed448_pt *r)
{
	sx_pk_set_cmd(req, SX_PK_CMD_EDDSA_VER);

	int status = ed448_verify_run(req, k, a, sig_s, r);

	if (status != SX_OK) {
		return status;
	}

	return sx_pk_wait(req);
}
