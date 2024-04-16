/** "sxops" interface for Weierstrass elliptic curve computations.
 *
 * Simpler functions to perform public key crypto operations. Included directly
 * in some interfaces (like sxbuf or OpenSSL engine). The functions
 * take input operands (large integers) and output operands
 * which will get the computed results.
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

#ifndef ECCWEIERSTRASS_HEADER_FILE
#define ECCWEIERSTRASS_HEADER_FILE

#include <silexpk/core.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <silexpk/cmddefs/ecc.h>
#include "../ec_curves.h"
#include "adapter.h"
#include "impl.h"
#include <silexpk/version.h>
#include <cracen/statuscodes.h>

#ifndef SX_MAX_ECC_ATTEMPTS
#define SX_MAX_ECC_ATTEMPTS 10
#endif

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_SRC_COMPATIBLE(2, 0, sxopseccweierstrass);

struct sx_pk_ecurve;

/**
 * @addtogroup SX_PK_SXOPS_ECC
 *
 * @{
 */

/** Curve generator point for sx_ecp_ptmult() or sx_async_ecp_mult_go() */
#define SX_PTMULT_CURVE_GENERATOR NULL

/**
 * @addtogroup SX_PK_SXOPS_ECDSA
 *
 * @{
 */

/** Asynchronous ECDSA signature generation
 *
 * Start an ECDSA signature generation on the accelerator
 * and return immediately.
 *
 * This operation can be protected with blinding counter measures.
 * When used with counter measures, the operation can randomly
 * fail with SX_ERR_NOT_INVERTIBLE. If it happens, the operation
 * should be retried with a new blinding factor. See the user guide
 * 'countermeasures' section for more information.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_ecdsa_generate_end()
 *
 *
 * @param[in] curve Elliptic curve on which to perform ECDSA signature
 * @param[in] d Private key
 * @param[in] k Non-zero random value smaller than the curve order
 * @param[in] h Formatted hash digest of message to be signed.
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_ecdsa_generate_go(const struct sx_pk_ecurve *curve,
							      const sx_ecop *d, const sx_ecop *k,
							      const sx_ecop *h)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecdsa_generate inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECDSA_GEN);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_ecop2mem(d, inputs.d.addr, opsz);
	sx_pk_ecop2mem(k, inputs.k.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) ECDSA generation.
 *
 * Get the output operands of the ECDSA signature generation
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] r First part of signature
 * @param[out] s Second part of signature
 */
static inline void sx_async_ecdsa_generate_end(sx_pk_req *req, sx_ecop *r, sx_ecop *s)
{
	sx_async_finish_ec_pair(req, r, s);
}

/** Generate an ECDSA signature on an elliptic curve
 *
 * The signature generation has the following steps :
 *   1. P(x1, y1) = k * G
 *   2. r = x1 mod n
 *   3. if r == 0 then report failure
 *   4. w = k ^ -1 mod n
 *   5. s = k ^ -1 * (h + d * r) mod n
 *   6. if s == 0 then report failure
 *   7. signature is the r and s
 *
 *
 * @param[in] curve Elliptic curve on which to perform ECDSA signature
 * @param[in] d Private key
 * @param[in] k Random value
 * @param[in] h Digest of message to be signed
 * Truncation or padding should be done by user application
 * @param[out] r First part of signature
 * @param[out] s Second part of signature
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_ecdsa_generate_go(), sx_async_ecdsa_generate_end() for
 * an asynchronous version
 */
static inline int sx_ecdsa_generate(const struct sx_pk_ecurve *curve, const sx_ecop *d,
				    const sx_ecop *k, const sx_ecop *h, sx_ecop *r, sx_ecop *s)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	for (int i = 0; i < SX_MAX_ECC_ATTEMPTS; i++) {
		pkreq = sx_async_ecdsa_generate_go(curve, d, k, h);
		if (pkreq.status) {
			return pkreq.status;
		}
		status = sx_pk_wait(pkreq.req);
		if (status == SX_ERR_NOT_INVERTIBLE) {
			sx_pk_release_req(pkreq.req);
		} else {
			break;
		}
	}
	if (status == SX_ERR_NOT_INVERTIBLE) {
		return status;
	}
	sx_async_ecdsa_generate_end(pkreq.req, r, s);

	return status;
}

/** Asynchronous (non-blocking) ECDSA verification.
 *
 * Start an ECDSA signature verification on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_pk_release_req()
 *
 *
 * @param[in] curve Elliptic curve on which to perform ECDSA verification
 * @param[in] q The public key Q
 * @param[in] r First part of signature to verify
 * @param[in] s Second part of signature to verify
 * @param[in] h Digest of message to be signed
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_ecdsa_verify_go(const struct sx_pk_ecurve *curve,
							    const sx_pk_affine_point *q,
							    const sx_ecop *r, const sx_ecop *s,
							    const sx_ecop *h)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecdsa_verify inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECDSA_VER);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_affpt2mem(q, inputs.qx.addr, inputs.qy.addr, opsz);
	sx_pk_ecop2mem(r, inputs.r.addr, opsz);
	sx_pk_ecop2mem(s, inputs.s.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Verify ECDSA signature on an elliptic curve
 *
 *  The verification has the following steps:
 *   1. check qx and qy are smaller than q from the domain
 *   2. Check that Q lies on the elliptic curve from the domain
 *   3. Check that r and s are smaller than n
 *   4.  w = s ^ -1 mod n
 *   5.  u1 = h * w mod n
 *   6.  u2 = r * w mod n
 *   7.  X(x1, y1) = u1 * G + u2 * Q
 *   8. If X is invalid, then the signature is invalid
 *   9.  v = x1 mod n
 *   10. Accept signature if and only if v == r
 *
 *
 * @param[in] curve Elliptic curve on which to perform ECDSA verification
 * @param[in] q The public key Q
 * @param[in] r First part of signature to verify
 * @param[in] s Second part of signature to verify
 * @param[in] h Digest of message to be signed
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_ecdsa_verify_go() for an asynchronous version
 */
static inline int sx_ecdsa_verify(const struct sx_pk_ecurve *curve, const sx_pk_affine_point *q,
				  const sx_ecop *r, const sx_ecop *s, const sx_ecop *h)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_ecdsa_verify_go(curve, q, r, s, h);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);
	sx_pk_release_req(pkreq.req);

	return status;
}

/** @} */

/**
 * @addtogroup SX_PK_SXOPS_ECOPS
 *
 * @{
 */

/** Asynchronous EC point multiplication.
 *
 * Starts an EC point multiplication on the accelerator
 * and return immediately.
 *
 * This operation can be protected with blinding counter measures.
 * When used with counter measures, the operation can randomly
 * fail with SX_ERR_NOT_INVERTIBLE. If it happens, the operation
 * should be retried with a new blinding factor. See the user guide
 * 'countermeasures' section for more information.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_ecp_mult_end()
 *
 *
 * @param[in] curve Elliptic curve used to perform point multiplication
 * @param[in] k Scalar that multiplies point P. It must be non zero and
 *             smaller than the curve order.
 * @param[in] p Point p
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_ecp_mult_go(const struct sx_pk_ecurve *curve,
							const sx_ecop *k,
							const sx_pk_affine_point *p)
{
	struct sx_pk_acq_req pkreq;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECC_PTMUL);
	if (pkreq.status) {
		return pkreq;
	}

	struct sx_pk_inops_ecp_mult inputs;

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_ecop2mem(k, inputs.k.addr, opsz);
	if (p == SX_PTMULT_CURVE_GENERATOR) {
		sx_pk_write_curve_gen(pkreq.req, curve, inputs.px, inputs.py);
	} else {
		sx_pk_affpt2mem(p, inputs.px.addr, inputs.py.addr, opsz);
	}

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous EC point multiplication.
 *
 * Get the output operands of the EC point multiplication
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] r Affine point R
 */
static inline void sx_async_ecp_mult_end(sx_pk_req *req, sx_pk_affine_point *r)
{
	sx_async_finish_affine_point(req, r);
}

/** Compute point multiplication on an elliptic curve
 *
 *  (Rx, Ry) = k * (Px, Py)
 *
 * @param[in] curve Elliptic curve used to perform point multiplication
 * @param[in] k Scalar that multiplies point P
 * @param[in] p Affine point P
 * @param[out] r Affine point R
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_ecp_mult_go(), sx_async_ecp_mult_end()
 * for an asynchronous versions
 */
static inline int sx_ecp_ptmult(const struct sx_pk_ecurve *curve, const sx_ecop *k,
				const sx_pk_affine_point *p, sx_pk_affine_point *r)
{
	int status;
	struct sx_pk_acq_req pkreq;

	for (int i = 0; i < SX_MAX_ECC_ATTEMPTS; i++) {
		pkreq = sx_async_ecp_mult_go(curve, k, p);
		if (pkreq.status) {
			return pkreq.status;
		}
		status = sx_pk_wait(pkreq.req);
		if (status == SX_ERR_NOT_INVERTIBLE) {
			sx_pk_release_req(pkreq.req);
		} else {
			break;
		}
	}
	if (status == SX_ERR_NOT_INVERTIBLE) {
		return status;
	}
	sx_async_ecp_mult_end(pkreq.req, r);

	return status;
}

/** Asynchronous EC point doubling.
 *
 * Starts an EC point doubling on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_ecp_double_end()
 *
 *
 * @param[in] curve Elliptic curve used to perform point doubling
 * @param[in] p Affine point P
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_ecp_double_go(const struct sx_pk_ecurve *curve,
							  const sx_pk_affine_point *p)
{
	struct sx_pk_acq_req pkreq;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECC_PT_DOUBLE);
	if (pkreq.status) {
		return pkreq;
	}

	struct sx_pk_inops_ecp_double inputs;

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_affpt2mem(p, inputs.px.addr, inputs.py.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous EC point doubling.
 *
 * Get the output operands of the EC point doubling
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] r Affine resulting point R
 */
static inline void sx_async_ecp_double_end(sx_pk_req *req, sx_pk_affine_point *r)
{
	sx_async_finish_affine_point(req, r);
}

/** Compute point doubling on an elliptic curve
 *
 *  (Rx, Ry) = 2 * (Px, Py)
 *
 *
 * @param[in] curve Elliptic curve used to perform point doubling
 * @param[in] p Affine point P
 * @param[out] r Affine resulting point R
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_ecp_double_go(), sx_async_ecp_double_end()
 * for an asynchronous verion
 */
static inline int sx_ecp_double(const struct sx_pk_ecurve *curve, const sx_pk_affine_point *p,
				sx_pk_affine_point *r)
{
	int status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_ecp_double_go(curve, p);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);
	sx_async_ecp_double_end(pkreq.req, r);

	return status;
}

/** Asynchronous (non-blocking) EC point on curve check.
 *
 * Starts an EC point on curve check on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_pk_release_req()
 *
 *
 * @param[in] curve Elliptic curve used to validate point
 * @param[in] p Point P
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_ec_ptoncurve_go(const struct sx_pk_ecurve *curve,
							    const sx_pk_affine_point *p)
{
	struct sx_pk_acq_req pkreq;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECC_PTONCURVE);
	if (pkreq.status) {
		return pkreq;
	}

	struct sx_pk_inops_ec_ptoncurve inputs;

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_affpt2mem(p, inputs.px.addr, inputs.py.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Check if the given point is on the given elliptic curve
 *
 * It succeeds if the following checks pass:
 * For GF(p):
 *  1. px < p
 *  2. py < p
 *  3. py^2 == px^3 + a * px + b mod p
 * For GF(2^m), where q = 2^m
 *  1. px < q
 *  2. py < q
 *  3. py^2 + px * py ==  px^3 + a * px^2 + b mod q
 *
 *
 * @param[in] curve Elliptic curve used to validate point
 * @param[in] p Affine point P
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 */
static inline int sx_ec_ptoncurve(const struct sx_pk_ecurve *curve, const sx_pk_affine_point *p)
{
	int status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_ec_ptoncurve_go(curve, p);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);
	sx_pk_release_req(pkreq.req);

	return status;
}

/** Asynchronous (non-blocking) EC point decompression.
 *
 * Starts an EC point decompression on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_ec_pt_decompression_end()
 *
 *
 * @param[in] curve Elliptic curve used to validate point
 * @param[in] x x-coordinate of point to decompress
 * @param[in] y_lsb Least Significant Bit of y-coordinate
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req
sx_async_ec_pt_decompression_go(const struct sx_pk_ecurve *curve, const sx_ecop *x, const int y_lsb)
{
	struct sx_pk_acq_req pkreq;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECC_PT_DECOMP);
	if (pkreq.status) {
		return pkreq;
	}

	struct sx_pk_inops_ec_pt_decompression inputs;

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, ((y_lsb & 1) << 29),
					      (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_ecop2mem(x, inputs.x.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) EC point decompression.
 *
 * Get the output operand of the EC point decompression
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] y y-coordinate of decompressed point
 */
static inline void sx_async_ec_pt_decompression_end(sx_pk_req *req, sx_ecop *y)
{
	sx_async_finish_single_ec(req, y);
}

/** ECC point decompression
 *
 *  Recover the y coordinate of a point using
 *  x value and LSB of y:
 *    1. y = sqrt(x^3 + a * x + b)
 *    2. if (y & 1) != y_lsb then y = p - y
 *    with a and p the curve parameters
 *    3. else return ::SX_ERR_NOT_QUADRATIC_RESIDUE
 *
 * @remark: Point decompression is supported for GF(p) only
 *
 * @param[in] curve Elliptic curve used to validate point
 * @param[in] x x-coordinate of point to decompress
 * @param[in] y_lsb Least Significant Bit of y-coordinate
 * @param[out] y y-coordinate of decompressed point
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_QUADRATIC_RESIDUE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 */
static inline int sx_ec_pt_decompression(const struct sx_pk_ecurve *curve, const sx_ecop *x,
					 const int y_lsb, sx_ecop *y)
{
	int status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_ec_pt_decompression_go(curve, x, y_lsb);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);
	sx_async_ec_pt_decompression_end(pkreq.req, y);

	return status;
}

/** @} */

/**
 * @addtogroup SX_PK_SXOPS_ECKCDSA
 *
 * @{
 */

/** Asynchronous (non-blocking) EC-KCDSA pubkey generation.
 *
 * Start an EC-KCDSA pubkey generation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_eckcdsa_pubkey_generate_end()
 *
 * This operation can be protected with blinding counter measures.
 * When used with counter measures, the operation can randomly
 * fail with SX_ERR_NOT_INVERTIBLE. If it happens, the operation
 * should be retried with a new blinding factor. See the user guide
 * 'countermeasures' section for more information.
 *
 * @param[in] curve Elliptic curve used to perform EC-KCDSA public
 * key generation
 * @param[in] d Private key (non zero and smaller than the curve order)
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req
sx_async_eckcdsa_pubkey_generate_go(const struct sx_pk_ecurve *curve, const sx_ecop *d)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_eckcdsa_generate inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECKCDSA_PUBKEY_GEN);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_ecop2mem(d, inputs.d.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) EC-KCDSA pubkey generation.
 *
 * Get the output operands of the EC-KCDSA pubkey generation
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] q The Public key Q
 */
static inline void sx_async_eckcdsa_pubkey_generate_end(sx_pk_req *req, sx_pk_affine_point *q)
{
	sx_async_finish_affine_point(req, q);
}

/** Generate an EC-KCDSA public key
 *
 *    Compute: Q = (1 / d) * G
 * If d & n are not coprime then SX_ERR_NOT_INVERTIBLE failure
 *
 *
 * @param[in] curve Elliptic curve used to perform EC-KCDSA public
 * key generation
 * @param[in] d Private key
 * @param[out] q The public key Q
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_eckcdsa_pubkey_generate_go(),
 * sx_async_eckcdsa_pubkey_generate_end() for an asynchronous verion
 */
static inline int sx_eckcdsa_pubkey_generate(const struct sx_pk_ecurve *curve, const sx_op *d,
					     sx_pk_affine_point *q)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	for (int i = 0; i < SX_MAX_ECC_ATTEMPTS; i++) {
		pkreq = sx_async_eckcdsa_pubkey_generate_go(curve, d);
		if (pkreq.status) {
			return pkreq.status;
		}
		status = sx_pk_wait(pkreq.req);
		if (status == SX_ERR_NOT_INVERTIBLE) {
			sx_pk_release_req(pkreq.req);
		} else {
			break;
		}
	}
	if (status == SX_ERR_NOT_INVERTIBLE) {
		return status;
	}
	sx_async_eckcdsa_pubkey_generate_end(pkreq.req, q);

	return status;
}

/** Asynchronous (non-blocking) EC-KCDSA 2nd part signature generation.
 *
 * Start an EC-KCDSA signature generation operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_eckcdsa_sign_end()
 *
 *
 * @param[in] curve Elliptic curve used to perform EC-KCDSA signature generation
 * @param[in] d Private key
 * @param[in] h Digest of z concatenated to M (z || M). z is the hash of the
 * certification data. M is the message.
 * @param[in] k Random value. Value should not be bigger than n-parameter of
 * curve.
 * @param[in] r First part of the signature
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_eckcdsa_sign_go(const struct sx_pk_ecurve *curve,
							    const sx_ecop *d, const sx_ecop *k,
							    const sx_ecop *h, const sx_ecop *r)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_eckcdsa_sign inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECKCDSA_SIGN);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_ecop2mem(d, inputs.d.addr, opsz);
	sx_pk_ecop2mem(k, inputs.k.addr, opsz);
	sx_pk_ecop2mem(r, inputs.r.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) EC-KCDSA signature generation.
 *
 * Get the output operands of the EC-KCDSA signature generation
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] s Second part of the signature
 */
static inline void sx_async_eckcdsa_sign_end(sx_pk_req *req, sx_ecop *s)
{
	sx_async_finish_single_ec(req, s);
}

/** Generate the 2nd part of an EC-KCDSA signature on an elliptic curve
 *
 * The 2nd part of signature generation has the following steps :
 *   1. e = (r xor h) mod n
 *   2. s = d*(k-e) mod n
 *   3. if s == 0 then report SX_ERR_INVALID_SIGNATURE failure
 *
 *
 * @param[in] curve Elliptic curve used to perform EC-KCDSA signature generation
 * @param[in] d Private key
 * @param[in] h Digest of z concatenated to M (z || M). z is the hash of the
 * certification data. M is the message.
 * @param[in] k Random value. Value should not be bigger than n-parameter of
 * curve.
 * @param[in] r First part of the signature
 * @param[out] s Second part of the signature
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_eckcdsa_sign_go(), sx_async_eckcdsa_sign_end()
 * for an asynchronous verion
 */
static inline int sx_eckcdsa_sign(const struct sx_pk_ecurve *curve, const sx_ecop *d,
				  const sx_ecop *k, const sx_ecop *h, const sx_ecop *r, sx_ecop *s)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_eckcdsa_sign_go(curve, d, k, h, r);
	if (pkreq.status) {
		return pkreq.status;
	}
	status = sx_pk_wait(pkreq.req);
	sx_async_eckcdsa_sign_end(pkreq.req, s);

	return status;
}

/** Asynchronous EC-KCDSA verification.
 *
 * Start an EC-KCDSA verification on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_eckcdsa_verify_end()
 *
 *
 * @param[in] curve Elliptic curve to perform EC-KCDSA verification
 * @param[in] q The public key Q
 * @param[in] r First part of signature
 * @param[in] s Second part of signature
 * @param[in] h Digest of z concatenated to M (z || M). z is the hash of the
 * certification data. M is the message.
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_eckcdsa_verify_go(const struct sx_pk_ecurve *curve,
							      const sx_pk_affine_point *q,
							      const sx_ecop *r, const sx_ecop *s,
							      const sx_ecop *h)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecdsa_verify inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECKCDSA_VER);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_affpt2mem(q, inputs.qx.addr, inputs.qy.addr, opsz);
	sx_pk_ecop2mem(r, inputs.r.addr, opsz);
	sx_pk_ecop2mem(s, inputs.s.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) EC-KCDSA verification.
 *
 * Get the output operands of the EC-KCDSA verification
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] w Resulting Point W
 */
static inline void sx_async_eckcdsa_verify_end(sx_pk_req *req, sx_pk_affine_point *w)
{
	sx_async_finish_affine_point(req, w);
}

/** Verify EC-KCDSA signature on an elliptic curve
 *
 *  EC-KCDSA verification goes as follow:
 *    1. if s < n then SX_ERR_OUT_OF_RANGE failure
 *    2. e = (r xor h) mod n
 *    3. Check Q.x < p & Q.y < p where p is the field size
 *    4. If failure: SX_ERR_OUT_OF_RANGE
 *    5. Check Q lies on EC
 *    6. If Failure: SX_ERR_POINT_NOT_ON_CURVE
 *    7. W(x, y) = e * G + s * Q
 *  Accept the signature only if:
 *     hash(W) == r
 *
 *
 * @param[in] curve Elliptic curve to perform EC-KCDSA verification
 * @param[in] q The public key Q
 * @param[in] r First part of signature
 * @param[in] s Second part of signature
 * @param[in] h Digest of z concatenated to M (z || M). z is the hash of the
 * certification data. M is the message.
 * @param[out] w Resulting point W
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_eckcdsa_verify_go(), sx_async_eckcdsa_verify_end()
 * for an asynchronous version
 */
static inline int sx_eckcdsa_verify(const struct sx_pk_ecurve *curve, const sx_pk_affine_point *q,
				    const sx_ecop *r, const sx_ecop *s, const sx_ecop *h,
				    sx_pk_affine_point *w)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_eckcdsa_verify_go(curve, q, r, s, h);
	if (pkreq.status) {
		return pkreq.status;
	}
	status = sx_pk_wait(pkreq.req);
	sx_async_eckcdsa_verify_end(pkreq.req, w);

	return status;
}

/** @} */

/**  @addtogroup SX_PK_SXOPS_ECOPS
 *
 * @{
 */

/** Asynchronous (non-blocking) EC point addition.
 *
 * Start an EC point addition on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_ecp_add_end()
 *
 *
 * @param[in] curve Elliptic curve to perform EC point addition
 * @param[in] p1 First point
 * @param[in] p2 Second point
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_ecp_add_go(const struct sx_pk_ecurve *curve,
						       const sx_pk_affine_point *p1,
						       const sx_pk_affine_point *p2)
{
	struct sx_pk_acq_req pkreq;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECC_PT_ADD);
	if (pkreq.status) {
		return pkreq;
	}

	struct sx_pk_inops_ecp_add inputs;

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_affpt2mem(p1, inputs.p1x.addr, inputs.p1y.addr, opsz);
	sx_pk_affpt2mem(p2, inputs.p2x.addr, inputs.p2y.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) EC point addition.
 *
 * Get the output operands of the EC point addition
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] r The resulting point R
 */
static inline void sx_async_ecp_add_end(sx_pk_req *req, sx_pk_affine_point *r)
{
	sx_async_finish_affine_point(req, r);
}

/** Compute point addition on an elliptic curve
 *
 *  (Rx, Ry) = P1 + P2
 *
 * If P1 == P2 returns an SX_ERR_NOT_INVERTIBLE error
 *
 * @remark Use point doubling operation for the addition of equal points
 *
 *
 * @param[in] curve Elliptic curve to do point addition
 * @param[in] p1 The first point
 * @param[in] p2 The second point
 * @param[out] r The resulting point R
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @remark Use point doubling operation for the addition of equal points
 * @see sx_ecp_double()
 */
static inline int sx_ecp_ptadd(const struct sx_pk_ecurve *curve, const sx_pk_affine_point *p1,
			       const sx_pk_affine_point *p2, sx_pk_affine_point *r)
{
	int status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_ecp_add_go(curve, p1, p2);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);
	sx_async_ecp_add_end(pkreq.req, r);

	return status;
}

/** @} */

/**
 * @addtogroup SX_PK_SXOPS_SM2
 *
 * @{
 */

/** Asynchronous (non-blocking) SM2 signature generation.
 *
 * Start a SM2 signature generation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_sm2_generate_end()
 *
 * This operation can be protected with blinding counter measures.
 * When used with counter measures, the operation can randomly
 * fail with SX_ERR_NOT_INVERTIBLE. If it happens, the operation
 * should be retried with a new blinding factor. See the user guide
 * 'countermeasures' section for more information.
 *
 * @param[in] curve Elliptic curve to perform SM2 signature
 * generation with
 * @param[in] d Private key
 * @param[in] k Random value. The value should be non-zero and smaller
 * than the n-parameter of the curve
 * @param[in] h 256-bit Hash digest of message M using SM3 hash functions
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_sm2_generate_go(const struct sx_pk_ecurve *curve,
							    const sx_ecop *d, const sx_ecop *k,
							    const sx_ecop *h)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecdsa_generate inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_SM2_GEN);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_ecop2mem(d, inputs.d.addr, opsz);
	sx_pk_ecop2mem(k, inputs.k.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) SM2 signature generation.
 *
 * Get the output operands of the SM2 signature generation and release
 * the accelerator.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] r First part of the signature
 * @param[out] s Second part of the signature
 */
static inline void sx_async_sm2_generate_end(sx_pk_req *req, sx_ecop *r, sx_ecop *s)
{
	sx_async_finish_ec_pair(req, r, s);
}

/** Generate an SM2 signature
 *
 *  The signature generation has the following steps:
 *   1. Compute (x1, y1) = k * G
 *   2. Compute r = (h + x1) mod n
 *   3. If r == 0 then SX_ERR_INVALID_SIGNATURE failure
 *   4. Compute s = ((1 + d)^(-1) * (k - r * d)) mod n
 *   5. If s == 0 then SX_ERR_INVALID_SIGNATURE failure
 *   6. Signature is (r, s)
 *
 *
 * @param[in] curve Elliptic curve to perform SM2 signature
 * generation with
 * @param[in] d Private key
 * @param[in] k Random value. The value should not be bigger
 * than the n-parameter of the curve
 * @param[in] h 256-bit Hash digest of message M using SM3 hash functions
 * @param[out] r First part of signature
 * @param[out] s Second part of signature
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 */
static inline int sx_sm2_generate(const struct sx_pk_ecurve *curve, const sx_ecop *d,
				  const sx_ecop *k, const sx_ecop *h, sx_ecop *r, sx_ecop *s)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	for (int i = 0; i < SX_MAX_ECC_ATTEMPTS; i++) {
		pkreq = sx_async_sm2_generate_go(curve, d, k, h);
		if (pkreq.status) {
			return pkreq.status;
		}
		status = sx_pk_wait(pkreq.req);
		if (status == SX_ERR_NOT_INVERTIBLE) {
			sx_pk_release_req(pkreq.req);
		} else {
			break;
		}
	}
	if (status == SX_ERR_NOT_INVERTIBLE) {
		return status;
	}
	sx_async_sm2_generate_end(pkreq.req, r, s);

	return status;
}

/** Asynchronous (non-blocking) SM2 verification.
 *
 * Start a SM2 verification operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_pk_release_req()
 *
 *
 * @param[in] curve Elliptic curve to perform SM2 signature verification with
 * @param[in] q The public key Q
 * @param[in] r First part of signature
 * @param[in] s Second part of signature
 * @param[in] h 256-bit Hash digest of a message M using SM3 hash functions
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_sm2_verify_go(const struct sx_pk_ecurve *curve,
							  const sx_pk_affine_point *q,
							  const sx_ecop *r, const sx_ecop *s,
							  const sx_ecop *h)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecdsa_verify inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_SM2_VER);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_affpt2mem(q, inputs.qx.addr, inputs.qy.addr, opsz);
	sx_pk_ecop2mem(r, inputs.r.addr, opsz);
	sx_pk_ecop2mem(s, inputs.s.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);
	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Verify SM2 signature on an elliptic curve
 *
 *  The verification has the following steps:
 *   1. check that r,s < n
 *   2. t = (r + s) mod n
 *   3. if t = 0 then SX_ERR_INVALID_SIGNATURE failure
 *   4. Compute (x1’, y1’) = s.G + t.Q
 *   5. Compute R = (h + x1’) mod n
 *   6. if R != r then SX_ERR_INVALID_SIGNATURE failure
 *   7. else signature is valid
 *
 *
 * @param[in] curve Elliptic curve to perform SM2 signature verification with
 * @param[in] q The public key Q
 * @param[in] r First part of signature
 * @param[in] s Second part of signature
 * @param[in] h 256-bit Hash digest of a message M using SM3 hash functions
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 */
static inline int sx_sm2_verify(const struct sx_pk_ecurve *curve, const sx_pk_affine_point *q,
				const sx_ecop *r, const sx_ecop *s, const sx_ecop *h)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_sm2_verify_go(curve, q, r, s, h);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);
	sx_pk_release_req(pkreq.req);

	return status;
}

/** Asynchronous (non-blocking) SM2 key exchange.
 *
 * Start a SM2 key exchange operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_sm2_exchange_end()
 *
 *
 * @param[in] curve Elliptic curve to perform SM2 key exchange with
 * @param[in] d Private key
 * @param[in] k Random value
 * @param[in] q The public key Q
 * @param[in] rb Random point of user b
 * @param[in] cof Cofactor \#E(Fp)/n, where n is the degree of a base point G
 * and \#E(Fp) the number of rational points of the curve
 * @param[in] rax x-coordinate of random point of user A
 * @param[in] w Value equal to (log2(n)/2)-1. n is the degree of
 * the base point of the selected curve.
 *
 * @return Acquired acceleration request for this operation
 *
 * @see sx_async_sm2_exchange_end()
 */
static inline struct sx_pk_acq_req
sx_async_sm2_exchange_go(const struct sx_pk_ecurve *curve, const sx_ecop *d, const sx_ecop *k,
			 const sx_pk_affine_point *q, const sx_pk_affine_point *rb,
			 const sx_ecop *cof, const sx_ecop *rax, const sx_ecop *w)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_sm2_exchange inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_SM2_EXCH);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_ecop2mem(d, inputs.d.addr, opsz);
	sx_pk_ecop2mem(k, inputs.k.addr, opsz);
	sx_pk_affpt2mem(q, inputs.qx.addr, inputs.qy.addr, opsz);
	sx_pk_affpt2mem(rb, inputs.rbx.addr, inputs.rby.addr, opsz);
	sx_pk_ecop2mem(cof, inputs.cof.addr, opsz);
	sx_pk_ecop2mem(rax, inputs.rax.addr, opsz);
	sx_pk_ecop2mem(w, inputs.w.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) SM2 key exchange.
 *
 * Get the output operands of the SM2 key exchange and release
 * the accelerator.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] u The shared secret U
 *
 * @see sx_async_sm2_exchange_go()
 */
static inline void sx_async_sm2_exchange_end(sx_pk_req *req, sx_pk_affine_point *u)
{
	sx_async_finish_affine_point(req, u);
}

/** SM2 key exchange
 *
 *  SM2 key exchange goes as follow:
 *    1. if Q or RB not on curve: SX_ERR_POINT_NOT_ON_CURVE failure
 *    2. x1 = w + (RA.x mod w) with w = 2^([log2(n) / 2] - 1)
 *      n is given as domain param
 *    3. tA = (d + x1 * k) mod n
 *    4. x2 = w + (RB.x mod w) with w = 2^([log2(n) / 2] - 1)
 *    5. U = (cof * tA) * (Q + x2 * RB)
 *    6. if U == 0 (point at infinity) then SX_ERR_NOT_INVERTIBLE failure
 *
 *
 * @param[in] curve Elliptic curve to perform SM2 key exchange with
 * @param[in] d Private key
 * @param[in] k Random value
 * @param[in] q The publix key Q
 * @param[in] rb Random point of user b
 * @param[in] cof Cofactor \#E(Fp)/n, where n is the degree of a base point G
 * and \#E(Fp) the number of rational points of the curve
 * @param[in] rax x-coordinate of random point of user A
 * @param[in] w Value equal to (log2(n)/2)-1. n is the degree of
 * the base point of the selected curve.
 * @param[out] u The shared secret U
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 */
static inline int sx_sm2_exchange(const struct sx_pk_ecurve *curve, const sx_ecop *d,
				  const sx_ecop *k, const sx_pk_affine_point *q,
				  const sx_pk_affine_point *rb, const sx_ecop *cof,
				  const sx_ecop *rax, const sx_ecop *w, sx_pk_affine_point *u)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_sm2_exchange_go(curve, d, k, q, rb, cof, rax, w);
	if (pkreq.status) {
		return pkreq.status;
	}
	status = sx_pk_wait(pkreq.req);
	sx_async_sm2_exchange_end(pkreq.req, u);

	return status;
}

/** @} */

/** @} */
#ifdef __cplusplus
}
#endif

#endif
