/** "sxops" interface for EC J-PAKE computations.
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

#ifndef ECJPAKE_HEADER_FILE
#define ECJPAKE_HEADER_FILE

#include <silexpk/core.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <silexpk/cmddefs/ecjpake.h>
#include <silexpk/iomem.h>
#include "../ec_curves.h"
#include "adapter.h"
#include "impl.h"
#include <silexpk/version.h>

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_SRC_COMPATIBLE(2, 0, sxopsecjpake);

struct sx_pk_ecurve;

/**
 * @addtogroup SX_PK_SXOPS_ECJPAKE
 *
 * @{
 */

/** Curve generator point for sx_ecjpake_verify_zkp() or
 * sx_ecjpake_verify_zkp_go()
 */
#ifndef SX_PT_CURVE_GENERATOR
#define SX_PT_CURVE_GENERATOR NULL
#endif

/** Asynchronous EC J-PAKE proof generation
 *
 * Start a EC J-PAKE proof generation operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_generate_zkp_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] v Random input (< n)
 * @param[in] x Exponent
 * @param[in] h Hash digest
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_ecjpake_generate_zkp_go(const struct sx_pk_ecurve *curve,
							      const sx_ecop *v, const sx_ecop *x,
							      const sx_ecop *h)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecjpake_generate_zkp inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECJPAKE_GENERATE_ZKP);
	if (pkreq.status) {
		return pkreq;
	}

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}

	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_ecop2mem(v, inputs.v.addr, opsz);
	sx_pk_ecop2mem(x, inputs.x.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) EC J-PAKE proof generation.
 *
 * Get the output operands of the EC J-PAKE proof generation
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] r The resulting value
 */
static inline void sx_ecjpake_generate_zkp_end(sx_pk_req *req, sx_ecop *r)
{
	sx_async_finish_single_ec(req, r);
}

/** Perform an EC J-PAKE proof generation
 *
 * The proof generation has the following steps:
 *   1. r = (v - (x * h)) % n [where n is the modulus, a curve parameter]
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] v Random input (< n)
 * @param[in] x Exponent
 * @param[in] h Hash digest
 *
 * Truncation or padding should be done by user application
 * @param[out] r The result
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_ecjpake_generate_zkp_go(), sx_async_ecjpake_generate_zkp_end()
 * for an asynchronous version
 */
static inline int sx_ecjpake_generate_zkp(const struct sx_pk_ecurve *curve, const sx_ecop *v,
					  const sx_ecop *x, const sx_ecop *h, sx_ecop *r)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_ecjpake_generate_zkp_go(curve, v, x, h);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_ecjpake_generate_zkp_end(pkreq.req, r);

	return status;
}

/** Asynchronous EC J-PAKE proof verification
 *
 * Start an EC J-PAKE proof verification operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_verify_zkp_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] v Point on the curve
 * @param[in] x Point on the curve
 * @param[in] r Proof to be verified
 * @param[in] h Hash digest
 * @param[in] g Point on the curve (Optional, pass SX_PT_CURVE_GENERATOR to use
 *              the curve generator point)
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_ecjpake_verify_zkp_go(const struct sx_pk_ecurve *curve,
							    const sx_pk_affine_point *v,
							    const sx_pk_affine_point *x,
							    const sx_ecop *r, const sx_ecop *h,
							    const sx_pk_affine_point *g)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecjpake_verify_zkp inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECJPAKE_VERIFY_ZKP);
	if (pkreq.status) {
		return pkreq;
	}

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}

	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_affpt2mem(v, inputs.xv.addr, inputs.yv.addr, opsz);
	sx_pk_affpt2mem(x, inputs.xx.addr, inputs.yx.addr, opsz);
	sx_pk_ecop2mem(r, inputs.r.addr, opsz);
	sx_pk_ecop2mem(h, inputs.h.addr, opsz);

	if (g != SX_PT_CURVE_GENERATOR) {
		sx_pk_affpt2mem(g, inputs.xg2.addr, inputs.yg2.addr, opsz);
	} else {
		sx_pk_write_curve_gen(pkreq.req, curve, inputs.xg2, inputs.yg2);
	}

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) EC J-PAKE proof verification.
 *
 * Finishes the EC J-PAKE proof verification
 * and releases the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 */
static inline void sx_ecjpake_verify_zkp_end(sx_pk_req *req)
{
	sx_pk_release_req(req);
}

/** Synchronous EC J-PAKE proof verification
 *
 * Start an EC J-PAKE proof verification operation on the accelerator
 * and return immediately.
 *
 * The proof verification has the following steps:
 *   1. ( (G * r) + (X * h) ) ?= V
 *
 * In case of a comparison failure SX_ERR_INVALID_SIGNATURE shall
 * be returned.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_verify_zkp_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] v Point on the curve
 * @param[in] x Point on the curve
 * @param[in] r Proof to be verified
 * @param[in] h Hash digest
 * @param[in] g Point on the curve (Optional, pass SX_PT_CURVE_GENERATOR to use
 *              the curve generator point)
 *
 * Truncation or padding should be done by user application
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_ecjpake_verify_zkp_go(), sx_async_ecjpake_verify_zkp_end() for
 * an asynchronous version
 */
static inline int sx_ecjpake_verify_zkp(const struct sx_pk_ecurve *curve,
					const sx_pk_affine_point *v, const sx_pk_affine_point *x,
					const sx_ecop *r, const sx_ecop *h,
					const sx_pk_affine_point *g)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_ecjpake_verify_zkp_go(curve, v, x, r, h, g);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_ecjpake_verify_zkp_end(pkreq.req);

	return status;
}

/** Asynchronous EC J-PAKE 3 point addition
 *
 * Start a EC J-PAKE 3 point addition operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_3pt_add_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] a Point on the curve
 * @param[in] b Point on the curve
 * @param[in] c Point on the curve
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 *
 */
static inline struct sx_pk_acq_req sx_ecjpake_3pt_add_go(const struct sx_pk_ecurve *curve,
							 const sx_pk_affine_point *a,
							 const sx_pk_affine_point *b,
							 const sx_pk_affine_point *c)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecjpake_3pt_add inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECJPAKE_3PT_ADD);
	if (pkreq.status) {
		return pkreq;
	}

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}

	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_affpt2mem(b, inputs.x2_1.addr, inputs.x2_2.addr, opsz);
	sx_pk_affpt2mem(c, inputs.x3_1.addr, inputs.x3_2.addr, opsz);
	sx_pk_affpt2mem(a, inputs.x1_1.addr, inputs.x1_2.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) EC J-PAKE 3 point addition.
 *
 * Finishes the EC J-PAKE 3 point addition
 * and releases the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] gb The addition result
 */
static inline void sx_ecjpake_3pt_add_end(sx_pk_req *req, sx_pk_affine_point *gb)
{
	sx_async_finish_affine_point(req, gb);
}

/** Synchronous EC J-PAKE 3 point addition
 *
 * Start a EC J-PAKE 3 point addition operation on the accelerator
 * and return immediately.
 *
 * The 3 point addition operation has the following steps:
 *   1. gb = a + b + c
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_3pt_add_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] a Point on the curve
 * @param[in] b Point on the curve
 * @param[in] c Point on the curve
 * @param[out] gb The addition result
 *
 * Truncation or padding should be done by user application
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_ecjpake_verify_zkp_go(), sx_async_ecjpake_verify_zkp_end() for
 * an asynchronous version
 */
static inline int sx_ecjpake_3pt_add(const struct sx_pk_ecurve *curve, const sx_pk_affine_point *a,
				     const sx_pk_affine_point *b, const sx_pk_affine_point *c,
				     sx_pk_affine_point *gb)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_ecjpake_3pt_add_go(curve, a, b, c);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_ecjpake_3pt_add_end(pkreq.req, gb);

	return status;
}

/** Asynchronous EC J-PAKE session key generation
 *
 * Start a EC J-PAKE session key generation operation on
 * the accelerator and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_gen_sess_key_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] x4 Point on the curve
 * @param[in] b Point on the curve
 * @param[in] x2 Generated random number
 * @param[in] x2s (x2 * s) % n [s = password, n = modulus]
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_ecjpake_gen_sess_key_go(const struct sx_pk_ecurve *curve,
							      const sx_pk_affine_point *x4,
							      const sx_pk_affine_point *b,
							      const sx_ecop *x2, const sx_ecop *x2s)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecjpake_gen_sess_key inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECJPAKE_GEN_SESS_KEY);
	if (pkreq.status) {
		return pkreq;
	}

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}

	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_affpt2mem(x4, inputs.x4_1.addr, inputs.x4_2.addr, opsz);
	sx_pk_affpt2mem(b, inputs.b_1.addr, inputs.b_2.addr, opsz);
	sx_pk_ecop2mem(x2, inputs.x2.addr, opsz);
	sx_pk_ecop2mem(x2s, inputs.x2s.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) EC J-PAKE session key generation
 *
 * Finishes the EC J-PAKE session key generation operation
 * and releases the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] t The result
 */
static inline void sx_ecjpake_gen_sess_key_end(sx_pk_req *req, sx_pk_affine_point *t)
{
	sx_async_finish_affine_point(req, t);
}

/** Synchronous EC J-PAKE session key generation
 *
 * Start a EC J-PAKE session key generation operation on the accelerator
 * and return immediately.
 *
 * The session key generation has the following steps:
 *   1. T = (b - (x4 * x2s)) * x2
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_gen_sess_key_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] x4 Point on the curve
 * @param[in] b Point on the curve
 * @param[in] x2 Generated random number
 * @param[in] x2s x2 * password
 * @param[out] t result
 *
 * Truncation or padding should be done by user application
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_ecjpake_gen_sess_key_go(), sx_async_ecjpake_gen_sess_key_end()
 * for an asynchronous version
 */
static inline int sx_ecjpake_gen_sess_key(const struct sx_pk_ecurve *curve,
					  const sx_pk_affine_point *x4, const sx_pk_affine_point *b,
					  const sx_ecop *x2, const sx_ecop *x2s,
					  sx_pk_affine_point *t)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_ecjpake_gen_sess_key_go(curve, x4, b, x2, x2s);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_ecjpake_gen_sess_key_end(pkreq.req, t);

	return status;
}

/** Asynchronous EC J-PAKE step 2 calculation
 *
 * Start an EC J-PAKE step 2 calculation operation on
 * the accelerator and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_gen_step_2_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] x4 Point on the curve
 * @param[in] x3 Point on the curve
 * @param[in] x1 Point on the curve
 * @param[in] x2s (x2 * s) % n [x2 = random, s = password, n = modulus]
 * @param[in] s password
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_ecjpake_gen_step_2_go(const struct sx_pk_ecurve *curve,
							    const sx_pk_affine_point *x4,
							    const sx_pk_affine_point *x3,
							    const sx_pk_affine_point *x1,
							    const sx_ecop *x2s, const sx_ecop *s)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ecjpake_gen_step_2 inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_ECJPAKE_GEN_STEP_2);
	if (pkreq.status) {
		return pkreq;
	}

	pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}

	int opsz = sx_pk_get_opsize(pkreq.req);

	sx_pk_affpt2mem(x4, inputs.x4_1.addr, inputs.x4_2.addr, opsz);
	sx_pk_affpt2mem(x3, inputs.x3_1.addr, inputs.x3_2.addr, opsz);
	sx_pk_affpt2mem(x1, inputs.x1_1.addr, inputs.x1_2.addr, opsz);
	sx_pk_ecop2mem(x2s, inputs.x2s.addr, opsz);
	sx_pk_ecop2mem(s, inputs.s.addr, opsz);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish an asynchronous (non-blocking) EC J-PAKE step 2 calculation
 *
 * Finishes the EC J-PAKE step 2 calculation operation
 * and releases the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] a Point on the curve
 * @param[out] x2s Generated random * password
 * @param[out] ga Point on the curve
 */
static inline void sx_ecjpake_gen_step_2_end(sx_pk_req *req, sx_pk_affine_point *a, sx_ecop *x2s,
					     sx_pk_affine_point *ga)
{
	const char **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2affpt(outputs[0], outputs[1], opsz, a);
	sx_pk_mem2op(outputs[2], opsz, x2s);
	sx_pk_mem2affpt(outputs[3], outputs[4], opsz, ga);
	sx_pk_release_req(req);
}

/** Synchronous EC J-PAKE step 2 calculation
 *
 * Start an EC J-PAKE step 2 calculation operation on the accelerator
 * and return immediately.
 *
 * The step 2 calculation has the following steps:
 *   1. ga = x1 + x3 + x4
 *   2. rx2s = (x2s * s) % curve.n
 *   3. a = ga * rx2s
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] x4 Point on the curve
 * @param[in] x3 Point on the curve
 * @param[in] x1 Point on the curve
 * @param[in] x2s Generated random * password
 * @param[in] s Password
 * @param[out] a Point on the curve
 * @param[out] rx2s Generated random * password
 * @param[out] ga Point on the curve
 *
 * Truncation or padding should be done by user application
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_ecjpake_gen_step_2_go(), sx_async_ecjpake_gen_step_2_end() for
 * an asynchronous version
 */
static inline int sx_ecjpake_gen_step_2(const struct sx_pk_ecurve *curve,
					const sx_pk_affine_point *x4, const sx_pk_affine_point *x3,
					const sx_pk_affine_point *x1, const sx_ecop *x2s,
					const sx_ecop *s, sx_pk_affine_point *a, sx_ecop *rx2s,
					sx_pk_affine_point *ga)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_ecjpake_gen_step_2_go(curve, x4, x3, x1, x2s, s);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_ecjpake_gen_step_2_end(pkreq.req, a, rx2s, ga);

	return status;
}

/** @} */
#ifdef __cplusplus
}
#endif

#endif
