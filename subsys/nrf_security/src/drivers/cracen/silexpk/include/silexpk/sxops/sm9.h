/** "sxops" interface for SM9 elliptic curve computations.
 *
 * Simpler functions to perform public key crypto operations. Included directly
 * in some interfaces (like sxbuf or OpenSSL engine). The functions
 * take input operands (large integers) and output operands
 * which will receive the computed results.
 *
 * Operands have the "sx_ecop" type. The specific interfaces (like sxbuf) define
 * the "sx_ecop" type.
 *
 * @file
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SM9_HEADER_FILE
#define SM9_HEADER_FILE

#include <silexpk/core.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <cracen/statuscodes.h>
#include <silexpk/cmddefs/sm9.h>
#include <silexpk/iomem.h>
#include "../ec_curves.h"
#include "adapter.h"
#include "impl.h"
#include <silexpk/version.h>

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_SRC_COMPATIBLE(2, 0, sxopssm9);

struct sx_pk_ecurve;

/**
 * @addtogroup SX_PK_SXOPS_SM9
 *
 * @{
 */

/** Affine point parameter group
 *
 * This structure is used for values in G1 which are stored in
 * two consecutive locations (x and y).
 */
struct sx_pk_point {
	sx_ecop *x; /**< x-coordinate */
	sx_ecop *y; /**< y-coordinate */
};

/** Extension field parameter group
 *
 * This structure is used for values in G2 which are stored in four
 * consecutive locations (two for x and two for y).
 */
struct sx_pk_ef_4 {
	sx_ecop *x0; /**< x-coordinate 0 */
	sx_ecop *x1; /**< x-coordinate 1 */
	sx_ecop *y0; /**< y-coordinate 0 */
	sx_ecop *y1; /**< y-coordinate 1 */
};

/** Extension field parameter group
 *
 * This structure is used for values in GT which are stored in
 * twelve consecutive locations.
 */
struct sx_pk_ef_12 {
	sx_ecop *coeffs[12]; /**< extension field coefficients */
};

/** SM9 Polynomial base */
static const uint8_t sm9_t[32] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				 "\x00\x00\x00\x00\x00\x00\x00\x00\x60\x00\x00\x00\x00\x58\xf9\x8A";

/** SM9 Frobenius constant */
static const uint8_t sm9_f[32] = "\x3f\x23\xea\x58\xe5\x72\x0b\xdb\x84\x3c\x6c\xfa\x9c\x08\x67\x49"
				 "\x47\xc5\xc8\x6e\x0d\xdd\x04\xed\xa9\x1d\x83\x54\x37\x7b\x69\x8b";

/** Perform an SM9 exponentiation synchronously
 *
 * The exponentiation has the following steps:
 *   1. z = g^h
 *
 * @param[in,out] req The acquired acceleration request for this operation
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] g Base
 * @param[in] h Exponent
 *
 * Truncation or padding should be done by user application
 * @param[out] z The result
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 */
static inline int sx_sm9_exp(sx_pk_req *req, struct sx_pk_cnx *cnx, const struct sx_pk_ef_12 *g,
			     const sx_const_ecop *h, struct sx_pk_ef_12 *z)
{
	struct sx_pk_inops_sm9_exp inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_SM9_EXP);

	const struct sx_pk_ecurve curve = sx_pk_get_curve_sm9(cnx);

	status = sx_pk_list_ecc_inslots(req, &curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	int opsz = sx_pk_get_opsize(req);

	sx_pk_ecop2mem(h, inputs.h.addr, opsz);
	sx_wrpkmem(inputs.t.addr, sm9_t, sizeof(sm9_t));
	for (int i = 0; i < 12; i++) {
		sx_pk_ecop2mem(g->coeffs[i], inputs.g[i].addr, opsz);
	}

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	for (int i = 0; i < count; i++) {
		sx_pk_mem2op(outputs[i], opsz, z->coeffs[i]);
	}

	return status;
}

/** Perform SM9 point multiplication in G1
 *
 * The exponentiation has the following steps
 *   1. Ppube = ke * p1
 *
 * In case ke is zero, ::SX_ERR_NOT_INVERTIBLE shall be
 * returned.
 *
 * @param[in,out] req The acquired acceleration request for this operation
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] p1 Point multiplicand
 * @param[in] ke Multiplier (Scalar)
 * @param[out] ppube Result
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *

 */
static inline int sx_sm9_pmulg1(sx_pk_req *req, struct sx_pk_cnx *cnx, const struct sx_pk_point *p1,
				const sx_const_ecop *ke, struct sx_pk_point *ppube)
{
	struct sx_pk_inops_sm9_pmulg1 inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_SM9_PMULG1);

	const struct sx_pk_ecurve curve = sx_pk_get_curve_sm9(cnx);

	status = sx_pk_list_ecc_inslots(req, &curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	int opsz = sx_pk_get_opsize(req);

	sx_pk_ecop2mem(p1->x, inputs.p1x0.addr, opsz);
	sx_pk_ecop2mem(p1->y, inputs.p1y0.addr, opsz);
	sx_pk_ecop2mem(ke, inputs.ke.addr, opsz);
	sx_wrpkmem(inputs.t.addr, sm9_t, sizeof(sm9_t));

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2ecop(outputs[0], opsz, ppube->x);
	sx_pk_mem2ecop(outputs[1], opsz, ppube->y);

	return status;
}

/** Perform an SM9 point multiplication in G2
 *
 * The exponentiation has the following steps
 *   1. Ppubs = ks * P2
 *
 * In case ks is zero, ::SX_ERR_NOT_INVERTIBLE shall be
 * returned.
 *
 * @param[in,out] req The acquired acceleration request for this operation
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] p2 Multiplicand
 * @param[in] ke Multiplier (Scalar)
 * @param[out] ppubs Result
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 */
static inline int sx_sm9_pmulg2(sx_pk_req *req, struct sx_pk_cnx *cnx, const struct sx_pk_ef_4 *p2,
				const sx_const_ecop *ke, struct sx_pk_ef_4 *ppubs)
{
	struct sx_pk_inops_sm9_pmulg2 inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_SM9_PMULG2);

	const struct sx_pk_ecurve curve = sx_pk_get_curve_sm9(cnx);

	status = sx_pk_list_ecc_inslots(req, &curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	int opsz = sx_pk_get_opsize(req);

	sx_pk_ecop2mem(p2->x0, inputs.p2x0.addr, opsz);
	sx_pk_ecop2mem(p2->x1, inputs.p2x1.addr, opsz);
	sx_pk_ecop2mem(p2->y0, inputs.p2y0.addr, opsz);
	sx_pk_ecop2mem(p2->y1, inputs.p2y1.addr, opsz);

	sx_pk_ecop2mem(ke, inputs.ke.addr, opsz);
	sx_wrpkmem(inputs.t.addr, sm9_t, sizeof(sm9_t));
	sx_pk_run(req);


	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);

	sx_pk_mem2op(outputs[0], opsz, ppubs->x0);
	sx_pk_mem2op(outputs[1], opsz, ppubs->x1);
	sx_pk_mem2op(outputs[2], opsz, ppubs->y0);
	sx_pk_mem2op(outputs[3], opsz, ppubs->y1);

	return status;
}

/** Perform an SM9 pairing
 *
 * The pairing has the following steps
 *   1. r = e(P, Q) where e is the bilinear mapping from G1xG2 to GT.
 *      e is also called the R-ate pairing
 *
 * @param[in,out] req The previously acquired acceleration
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] p P in G1
 * @param[in] q Q in G2
 * @param[out] r The 12th-degree extension field
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 */
static inline int sx_sm9_pair(sx_pk_req *req, struct sx_pk_cnx *cnx, const struct sx_pk_point *p,
			      const struct sx_pk_ef_4 *q, struct sx_pk_ef_12 *r)
{
	struct sx_pk_inops_sm9_pair inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_SM9_PAIR);

	const struct sx_pk_ecurve curve = sx_pk_get_curve_sm9(cnx);

	status = sx_pk_list_ecc_inslots(req, &curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	int opsz = sx_pk_get_opsize(req);

	sx_pk_ecop2mem(p->x, inputs.px0.addr, opsz);
	sx_pk_ecop2mem(p->y, inputs.py0.addr, opsz);
	sx_pk_ecop2mem(q->x0, inputs.qx0.addr, opsz);
	sx_pk_ecop2mem(q->x1, inputs.qx1.addr, opsz);
	sx_pk_ecop2mem(q->y0, inputs.qy0.addr, opsz);
	sx_pk_ecop2mem(q->y1, inputs.qy1.addr, opsz);
	sx_wrpkmem(inputs.t.addr, sm9_t, sizeof(sm9_t));
	sx_wrpkmem(inputs.f.addr, sm9_f, sizeof(sm9_f));

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);

	for (int i = 0; i < count; i++) {
		sx_pk_mem2op(outputs[i], opsz, r->coeffs[i]);
	}

	return status;
}

/** Perform SM9 signature private key generation
 *
 * The signature private key generation has
 * the following steps
 *   1. t1 = h + ks mod n
 *   2. t2 = ks * t1^-1 mod n
 *   3. ds = t2 * P1
 *
 * In case t1 is zero, ::SX_ERR_NOT_INVERTIBLE shall be
 * returned.
 *
 * @param[in,out] req The previously acquired acceleration
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] p1 p1 in G1
 * @param[in] h h in gf(q)
 * @param[in] ks ks in gf(q)
 * @param[out] ds ds in G1
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 */
static inline int sx_sm9_generate_signature_private_key(sx_pk_req req,
					struct sx_pk_cnx *cnx, const struct sx_pk_point *p1,
					const sx_const_ecop *h, const sx_const_ecop *ks,
					struct sx_pk_point *ds)
{
	struct sx_pk_inops_sm9_sigpkgen inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_SM9_PRIVSIGKEYGEN);

	const struct sx_pk_ecurve curve = sx_pk_get_curve_sm9(cnx);

	status = sx_pk_list_ecc_inslots(req, &curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	int opsz = sx_pk_get_opsize(req);

	sx_pk_ecop2mem(p1->x, inputs.p1x0.addr, opsz);
	sx_pk_ecop2mem(p1->y, inputs.p1y0.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);
	sx_pk_ecop2mem(ks, inputs.ks.addr, opsz);
	sx_wrpkmem(inputs.t.addr, sm9_t, sizeof(sm9_t));

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);

	sx_pk_mem2ecop(outputs[0], opsz, ds->x);
	sx_pk_mem2ecop(outputs[1], opsz, ds->y);

	return status;
}

/** Perform an SM9 signing operation
 *
 * The signing operation has the following steps
 *   1. l = r – h mod n
 *   2. S = l * ds
 *
 * In case l is zero, ::SX_ERR_NOT_INVERTIBLE shall be
 * returned.
 *
 * @param[in,out] req The previously acquired acceleration
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] ds ds in G1
 * @param[in] h h in gf(q)
 * @param[in] r r in gf(q)
 * @param[out] s signature in G1
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 */
static inline int sx_sm9_sign(sx_pk_req *req, struct sx_pk_cnx *cnx, const struct sx_pk_point *ds,
			      const sx_const_ecop *h, const sx_const_ecop *r,
			      struct sx_pk_point *s)
{
	struct sx_pk_inops_sm9_signaturegen inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_SM9_SIGNATUREGEN);

	const struct sx_pk_ecurve curve = sx_pk_get_curve_sm9(cnx);

	status = sx_pk_list_ecc_inslots(req, &curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	int opsz = sx_pk_get_opsize(req);

	sx_pk_ecop2mem(ds->x, inputs.dsx0.addr, opsz);
	sx_pk_ecop2mem(ds->y, inputs.dsy0.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);
	sx_pk_ecop2mem(r, inputs.r.addr, opsz);
	sx_wrpkmem(inputs.t.addr, sm9_t, sizeof(sm9_t));

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	sx_pk_mem2ecop(outputs[0], opsz, s->x);
	sx_pk_mem2ecop(outputs[1], opsz, s->y);

	return status;
}

/** Perform a SM9 signature verification synchronously
 *
 *  The signature verification operation has the following steps
 *   1. t = gh
 *   2. P = h1*P2 + Ppubs
 *   3. u = e(S, P)
 *   4. w = u*t
 *
 *  In case h = 0 or h >= q, ::SX_ERR_OUT_OF_RANGE shall be
 *  returned.
 *
 * @param[in,out] req The previously acquired acceleration
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] h1 h1 in gf(q)
 * @param[in] p2 p2 in G2
 * @param[in] ppubs ppubs in G2
 * @param[in] s s in G1
 * @param[in] h h in gf(q)
 * @param[in] g g in g(12)
 * @param[out] w w in g(12)
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * Truncation or padding should be done by user application
 *
 */
static inline int sx_sm9_signature_verify(sx_pk_req *req, struct sx_pk_cnx *cnx,
					  const sx_const_ecop *h1, const struct sx_pk_ef_4 *p2,
					  const struct sx_pk_ef_4 *ppubs,
					  const struct sx_pk_point *s, const sx_const_ecop *h,
					  const struct sx_pk_ef_12 *g, struct sx_pk_ef_12 *w)
{
	struct sx_pk_inops_sm9_signatureverify inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_SM9_SIGNATUREVERIFY);

	const struct sx_pk_ecurve curve = sx_pk_get_curve_sm9(cnx);

	status = sx_pk_list_ecc_inslots(req, &curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	int opsz = sx_pk_get_opsize(req);

	sx_pk_ecop2mem(h1, inputs.h1.addr, opsz);
	sx_pk_ecop2mem(p2->x0, inputs.p2x0.addr, opsz);
	sx_pk_ecop2mem(p2->x1, inputs.p2x1.addr, opsz);
	sx_pk_ecop2mem(p2->y0, inputs.p2y0.addr, opsz);
	sx_pk_ecop2mem(p2->y1, inputs.p2y1.addr, opsz);
	sx_pk_ecop2mem(ppubs->x0, inputs.ppubsx0.addr, opsz);
	sx_pk_ecop2mem(ppubs->x1, inputs.ppubsx1.addr, opsz);
	sx_pk_ecop2mem(ppubs->y0, inputs.ppubsy0.addr, opsz);
	sx_pk_ecop2mem(ppubs->y1, inputs.ppubsy1.addr, opsz);
	sx_pk_ecop2mem(s->x, inputs.sx0.addr, opsz);
	sx_pk_ecop2mem(s->y, inputs.sy0.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);
	sx_wrpkmem(inputs.f.addr, sm9_f, sizeof(sm9_f));
	sx_wrpkmem(inputs.t.addr, sm9_t, sizeof(sm9_t));
	for (int i = 0; i < 12; i++) {
		sx_pk_ecop2mem(g->coeffs[i], inputs.g[i].addr, opsz);
	}

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);

	for (int i = 0; i < count; i++) {
		sx_pk_mem2op(outputs[i], opsz, w->coeffs[i]);
	}

	return status;
}

/** Perform SM9 encryption private key generation
 *
 * The encryption private key generation has the
 * following steps:
 *
 *   1. t1 = h + ke mod n
 *   2. t2 = ke * t1^-1 mod n
 *   3. de = t2 * P2
 *
 * In case t1 is zero, ::SX_ERR_NOT_INVERTIBLE shall
 * be returned.
 *
 * @param[in,out] req The previously acquired acceleration
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] p2 p2 in G2
 * @param[in] h h in gf(q)
 * @param[in] ke ke in gf(q)
 * @param[out] de de in G2
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 */
static inline int sx_sm9_generate_encryption_private_key(sx_pk_req *req,
					struct sx_pk_cnx *cnx, const struct sx_pk_ef_4 *p2,
					const sx_const_ecop *h, const sx_const_ecop *ke,
					struct sx_pk_ef_4 *de)
{
	struct sx_pk_inops_sm9_privencrkeygen inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_SM9_PRIVENCRKEYGEN);

	const struct sx_pk_ecurve curve = sx_pk_get_curve_sm9(cnx);

	status = sx_pk_list_ecc_inslots(req, &curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	int opsz = sx_pk_get_opsize(req);

	sx_pk_ecop2mem(p2->x0, inputs.p2x0.addr, opsz);
	sx_pk_ecop2mem(p2->x1, inputs.p2x1.addr, opsz);
	sx_pk_ecop2mem(p2->y0, inputs.p2y0.addr, opsz);
	sx_pk_ecop2mem(p2->y1, inputs.p2y1.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);
	sx_pk_ecop2mem(ke, inputs.ks.addr, opsz);
	sx_wrpkmem(inputs.t.addr, sm9_t, sizeof(sm9_t));

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);

	sx_pk_mem2op(outputs[0], opsz, de->x0);
	sx_pk_mem2op(outputs[1], opsz, de->x1);
	sx_pk_mem2op(outputs[2], opsz, de->y0);
	sx_pk_mem2op(outputs[3], opsz, de->y1);

	return status;
}

/** Perform an SM9 send key operation
 *
 * The send key operation has the following steps
 *   1. QB = h * P1 + Ppube
 *   2. R (rx) = r * QB
 *
 * In case r is zero, ::SX_ERR_NOT_INVERTIBLE shall be
 * returned.
 *
 * @param[in,out] req The previously acquired acceleration
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] p1 p1 in G1
 * @param[in] ppube ppube in G1
 * @param[in] h h in gf(q)
 * @param[in] r r in gf(q)
 * @param[out] rx R in G1
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 */
static inline int sx_sm9_send_key(sx_pk_req *req, struct sx_pk_cnx *cnx,
				  const struct sx_pk_point *p1, const struct sx_pk_point *ppube,
				  const sx_const_ecop *h, const sx_const_ecop *r,
				  struct sx_pk_point *rx)
{
	struct sx_pk_inops_sm9_sendkey inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_SM9_SENDKEY);

	const struct sx_pk_ecurve curve = sx_pk_get_curve_sm9(cnx);

	status = sx_pk_list_ecc_inslots(req, &curve, 0, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	int opsz = sx_pk_get_opsize(req);

	sx_pk_ecop2mem(p1->x, inputs.p1x0.addr, opsz);
	sx_pk_ecop2mem(p1->y, inputs.p1y0.addr, opsz);
	sx_pk_ecop2mem(ppube->x, inputs.ppubex0.addr, opsz);
	sx_pk_ecop2mem(ppube->y, inputs.ppubey0.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);
	sx_pk_ecop2mem(r, inputs.r.addr, opsz);
	sx_wrpkmem(inputs.t.addr, sm9_t, sizeof(sm9_t));

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);

	sx_pk_mem2ecop(outputs[0], opsz, r->x);
	sx_pk_mem2ecop(outputs[1], opsz, r->y);

	return status;
}

/** Perform an SM9 reduce h operation
 *
 * The reduce h operation has the following steps
 *   1. h = (h mod (n-1)) + 1
 *
 * @param[in,out] req The previously acquired acceleration
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] h integer
 * @param[out] rh h in gf(q)
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 */
static inline int sx_sm9_reduce_h(sx_pk_req *req, struct sx_pk_cnx *cnx,
					const sx_const_ecop *h, sx_ecop *rh)
{
	struct sx_pk_inops_sm9_reduceh inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_SM9_REDUCEH);

	int sizes[] = {sx_const_op_size(h), sizeof(sm9_t)};

	status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	sx_pk_op2vmem(h, inputs.h.addr);
	sx_wrpkmem(inputs.t.addr, sm9_t, sizeof(sm9_t));

	sx_pk_run(req);


	status = sx_pk_wait(req);

	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2ecop(outputs[0], opsz, rh);

	return status;
}

/** @} */
#ifdef __cplusplus
}
#endif

#endif
