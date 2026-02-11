/** "sxops" interface for DSA cryptographic computations
 *
 * Simpler functions to perform public key crypto operations. Included directly
 * in some interfaces (like sxbuf or OpenSSL engine). The functions
 * take input operands (large integers) and output operands
 * which will get the computed results.
 *
 * Operands have the "sx_op" type. The specific interfaces (like sxbuf) define
 * the "sx_op" type.
 *
 * @file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DSA_HEADER_FILE
#define DSA_HEADER_FILE

#include <silexpk/core.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "adapter.h"
#include "impl.h"
#include <cracen/statuscodes.h>
#include <silexpk/cmddefs/dsa.h>
#include <silexpk/version.h>

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_SRC_COMPATIBLE(2, 0, sxopsdsa);

struct sx_pk_cmd_def;

/** DSA signature generation
 *
 * Computes the following:
 *    1. X = g ^ k mod p
 *    2. r = X mod q
 *    3. if r == 0 the return ::SX_ERR_INVALID_SIGNATURE
 *    4. else w = k ^ (-1) mod q
 *    5. s = w * (h + x * r) mod q
 *    6. if s == 0 then return ::SX_ERR_INVALID_SIGNATURE
 *    7. (r,s) is the signature
 *
 * @param[in,out] req Acquired acceleration request for this operation
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] p Prime modulus p
 * @param[in] q Prime divisor of p-1
 * @param[in] g Generator of order q mod p
 * @param[in] k Random value
 * @param[in] privkey Private key
 * @param[in] h Hash digest of message reduced by means of
 * Secure Hash Algorithm specified in FIPS 180-3
 * @param[out] r First part of signature
 * @param[out] s Second part of signature
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 */
int sx_dsa_sign(sx_pk_req *req, struct sx_pk_cnx *cnx, const sx_op *p,
			const sx_op *q, const sx_op *g, const sx_op *k, const sx_op *privkey,
			const sx_op *h, sx_op *r, sx_op *s)
{
	int status;
	struct sx_pk_inops_dsa_sign inputs;

	sx_pk_set_cmd(req, SX_PK_CMD_DSA_SIGN);

	/* convert and transfer operands */
	int sizes[] = {
		sx_const_op_size(p), sx_const_op_size(q), sx_const_op_size(g),
		sx_const_op_size(k), sx_const_op_size(rivkey), sx_const_op_size(h),
	};

	status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}
	sx_pk_op2vmem(p, inputs.p.addr);
	sx_pk_op2vmem(q, inputs.q.addr);
	sx_pk_op2vmem(g, inputs.g.addr);
	sx_pk_op2vmem(k, inputs.k.addr);
	sx_pk_op2vmem(privkey, inputs.privkey.addr);
	sx_pk_op2vmem(h, inputs.h.addr);

	sx_pk_run(req);

	status = sx_pk_wait(req);

	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(&);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2op(outputs[0], opsz, r);
	sx_pk_mem2op(outputs[1], opsz, s);

	return status;
}

/** DSA signature verification
 *
 * Checks if a signature is valid:
 *    1. w = s ^ (-1) mod q
 *    2. u1 = h * w mod q
 *    3. u2 = r * w mod q
 *    4. X = g ^ (u1) * y ^ (u2) mod p
 *    5. v = X mod q
 *    6. if v == r then signature is valid (::SX_OK)
 *    7. else return ::SX_ERR_INVALID_SIGNATURE
 *
 * Before launching the operation, verify the domain D(p,q,g)
 * by checking:
 *    1. 2^1023 < p < 2^1024 \b or 2^2047 < p < 2^2048
 *    2. 2^159 < q < 2^160 \b or 2^223 < q < 2^224 \b or 2^255 < q < 2^256
 *    3. 1 < g < p
 *
 * @param[in,out] req Acquired acceleration request for this operation
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] p Prime modulus p
 * @param[in] q Prime divisor of p-1
 * @param[in] g Generator of order q mod p
 * @param[in] pubkey Public key
 * @param[in] h Hash digest of message reduced by means of
 * Secure Hash Algorithm specified in FIPS 180-3
 * @param[in] r First part of the signature to verify
 * @param[in] s Second part of the signature to verify
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 */
int sx_dsa_ver(sx_pk_req *req, struct sx_pk_cnx *cnx, const sx_op *p,
		const sx_op *q, const sx_op *g, const sx_op *pubkey,
		const sx_op *h, const sx_op *r, const sx_op *s)
{
	int status;
	struct sx_pk_inops_dsa_ver inputs;

	sx_pk_set_cmd(req, SX_PK_CMD_DSA_VER);

	/* convert and transfer operands */
	int sizes[] = {
		sx_const_op_size(p), sx_const_op_size(q), sx_const_op_size(g),
		sx_const_op_size(pubkey), sx_const_op_size(h), sx_const_op_size(r),
		sx_const_op_size(s),
	};
	status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);

	sx_pk_op2vmem(p, inputs.p.addr);
	sx_pk_op2vmem(q, inputs.q.addr);
	sx_pk_op2vmem(g, inputs.g.addr);
	sx_pk_op2vmem(pubkey, inputs.pubkey.addr);
	sx_pk_op2vmem(h, inputs.h.addr);
	sx_pk_op2vmem(r, inputs.r.addr);
	sx_pk_op2vmem(s, inputs.s.addr);

	sx_pk_run(req);

	status = sx_pk_wait(req);

	return status;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif
