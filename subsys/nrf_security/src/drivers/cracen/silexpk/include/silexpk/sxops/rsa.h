/** "sxops" interface for RSA and GF(p) cryptographic computations
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

#ifndef RSA_HEADER_FILE
#define RSA_HEADER_FILE

#include <silexpk/core.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "adapter.h"
#include "impl.h"
#include <silexpk/cmddefs/modmath.h>
#include <silexpk/cmddefs/rsa.h>
#include <silexpk/cmddefs/modexp.h>
#include <silexpk/version.h>

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_SRC_COMPATIBLE(2, 0, sxopsrsa);

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_SXOPS_MOD
 *
 * @{
 */

/** Asynchronous (non-blocking) Primitive modular operation with 1 operand
 *
 * Start a primitive modular operation with 1 operand on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_finish_single()
 *
 * @param[in] cmd Command definition. Should be a primitive modular
 * operation with 1 operands. For example:
 *    ::SX_PK_CMD_ODD_MOD_INV, ::SX_PK_CMD_ODD_MOD_REDUCE,
 *    ::SX_PK_CMD_EVEN_MOD_INV, ::SX_PK_CMD_EVEN_MOD_REDUCE
 * @param[in] modulo Modulus operand. Must be odd if using
 * ::SX_PK_CMD_ODD_MOD_INV or ::SX_PK_CMD_ODD_MOD_REDUCE command and even when
 * using ::SX_PK_CMD_EVEN_MOD_INV or ::SX_PK_CMD_EVEN_MOD_REDUCE command even
 * commands
 * @param[in] b Operand of modular operation
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req
sx_async_mod_single_op_cmd_go(const struct sx_pk_cmd_def *cmd, const sx_op *modulo, const sx_op *b)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_mod_single_op_cmd inputs;

	pkreq = sx_pk_acquire_req(cmd);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	int sizes[] = {
		sx_op_size(modulo),
		sx_op_size(b),
	};
	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	sx_pk_op2vmem(modulo, inputs.n.addr);
	sx_pk_op2vmem(b, inputs.b.addr);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Compute single operand modular operation
 *
 *  - result = b mod modulo for ::SX_PK_CMD_ODD_MOD_REDUCE,
 * ::SX_PK_CMD_EVEN_MOD_REDUCE
 *  - result = 1 / b mod modulo for ::SX_PK_CMD_ODD_MOD_INV,
 * ::SX_PK_CMD_EVEN_MOD_INV
 *
 * @param[in] cmd Command definition. Should be a primitive modular
 * operation with 1 operand. For example:
 *    ::SX_PK_CMD_ODD_MOD_REDUCE, ::SX_PK_CMD_ODD_MOD_INV,
 *    ::SX_PK_CMD_EVEN_MOD_REDUCE, ::SX_PK_CMD_EVEN_MOD_INV
 * @param[in] modulo Modulus operand. Must be odd if using
 * ::SX_PK_CMD_ODD_MOD_INV or ::SX_PK_CMD_ODD_MOD_REDUCE command and even when
 * using ::SX_PK_CMD_EVEN_MOD_INV or ::SX_PK_CMD_EVEN_MOD_REDUCE command
 * @param[in] b Operand
 * @param[out] result Result operand
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @remark It is up to the user to use the corresponding command w.r.t.
 * the parity of the modulus
 *
 * @see sx_async_mod_single_op_cmd_go() and sx_async_finish_single()
 * for an asynchronous version
 */
static inline int sx_mod_single_op_cmd(const struct sx_pk_cmd_def *cmd, const sx_op *modulo,
				       const sx_op *b, sx_op *result)
{
	struct sx_pk_acq_req pkreq;
	int status;

	pkreq = sx_async_mod_single_op_cmd_go(cmd, modulo, b);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);
	sx_async_finish_single(pkreq.req, result);

	return status;
}

/** Asynchronous (non-blocking) Primitive modular operation with 2 operands
 *
 * Start a primitive modular operation with 2 operands on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_finish_single()
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] cmd Command definition. Should be a primitive modular
 * operation with 2 operands. For example:
 *    ::SX_PK_CMD_MOD_ADD, ::SX_PK_CMD_MOD_SUB,
 *    ::SX_PK_CMD_ODD_MOD_MULT, ::SX_PK_CMD_ODD_MOD_DIV
 * @param[in] modulo Modulus operand for the modular operantion
 * @param[in] a First operand of modular operation
 * @param[in] b Second operand of modular operation
 *
 * @return Acquired acceleration request for this operation
 *
 * @see sx_mod_primitive_cmd() for a synchronous version
 */
static inline struct sx_pk_acq_req sx_async_mod_cmd_go(struct sx_pk_cnx *cnx,
						       const struct sx_pk_cmd_def *cmd,
						       const sx_op *modulo, const sx_op *a,
						       const sx_op *b)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_mod_cmd inputs;

	pkreq = sx_pk_acquire_req(cmd);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	int sizes[] = {
		sx_op_size(modulo),
		sx_op_size(a),
		sx_op_size(b),
	};
	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	sx_pk_op2vmem(modulo, inputs.n.addr);
	sx_pk_op2vmem(a, inputs.a.addr);
	sx_pk_op2vmem(b, inputs.b.addr);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Primitive modular operation with 2 operands
 *
 * - result = a + b mod modulo for ::SX_PK_CMD_MOD_ADD
 * - result = a - b mod modulo for ::SX_PK_CMD_MOD_SUB
 * - result = a * b mod modulo for ::SX_PK_CMD_ODD_MOD_MULT with odd modulus
 * - result = a / b mod modulo for ::SX_PK_CMD_ODD_MOD_DIV with odd modulus
 *
 * Perform a modular addition or subtraction or
 * odd modular multiplication or odd modular division.
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] cmd Command definition. Should be a primitive modular
 * operation with 2 operands. See description
 * @param[in] modulo Modulus operand for the modular operation
 * @param[in] a First operand of modular operation
 * @param[in] b Second operand of modular operation
 * @param[out] result Result operand of the modular operation
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_mod_cmd_go() and sx_async_finish_single()
 * for an asynchronous version
 */
static inline int sx_mod_primitive_cmd(struct sx_pk_cnx *cnx, const struct sx_pk_cmd_def *cmd,
				       const sx_op *modulo, const sx_op *a, const sx_op *b,
				       sx_op *result)
{
	struct sx_pk_acq_req pkreq;
	int status;

	pkreq = sx_async_mod_cmd_go(cnx, cmd, modulo, a, b);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);
	sx_async_finish_single(pkreq.req, result);

	return status;
}

/** Asynchronous (non-blocking) mod inversion.
 *
 * Start a modular inversion with an odd modulus on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_finish_single()
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] modulo Odd modulus operand
 * @param[in] b Operand to inverse
 *
 * @return Acquired acceleration request for this operation
 *
 * @see sx_async_finish_single(), sx_mod_inv()
 */
static inline struct sx_pk_acq_req sx_async_mod_inv_go(struct sx_pk_cnx *cnx, const sx_op *modulo,
						       const sx_op *b)
{
	return sx_async_mod_single_op_cmd_go(SX_PK_CMD_ODD_MOD_INV, modulo, b);
}

/** Compute modular inversion with odd modulus
 *
 *    result = 1/b mod modulo
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] modulo Odd modulus operand
 * @param[in] b Operand to inverse
 * @param[out] result Result operand of the modular inversion
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_mod_inv_go() for an asynchronous verion
 */
static inline int sx_mod_inv(struct sx_pk_cnx *cnx, const sx_op *modulo, const sx_op *b,
			     sx_op *result)
{
	struct sx_pk_acq_req pkreq;
	int status;

	pkreq = sx_async_mod_inv_go(cnx, modulo, b);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_async_finish_single(pkreq.req, result);

	return status;
}

/** Asynchronous (non-blocking) modular exponentiation.
 *
 * Start a modular exponentiation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_mod_exp_end()
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] input Base operand for modular exponentiation
 * @param[in] e Exponent operand
 * @param[in] m Modulus operand
 *
 * @return Acquired acceleration request for this operation
 *
 * @see sx_mod_exp() for the synchronous version
 */
static inline struct sx_pk_acq_req sx_async_mod_exp_go(struct sx_pk_cnx *cnx, const sx_op *input,
						       const sx_op *e, const sx_op *m)
{
	return sx_async_mod_cmd_go(cnx, SX_PK_CMD_MOD_EXP, m, input, e);
}

/** Finish asynchronous (non-blocking) modular exponentiation.
 *
 * Get the output operand of the modular exponentiation and release
 * the accelerator.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] result Result operand
 *
 * @see sx_async_mod_exp_go() and sx_mod_exp()
 */
static inline void sx_async_mod_exp_end(sx_pk_req *req, sx_op *result)
{
	sx_async_finish_single(req, result);
}

/** Compute modular exponentiation.
 *
 *    result = input ^ e mod m
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] input Base operand
 * @param[in] e Exponent operand
 * @param[in] m Modulus operand
 * @param[out] result Result operand
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_mod_exp_go(), sx_async_mod_exp_end()
 * for an asynchronous version
 */
static inline int sx_mod_exp(struct sx_pk_cnx *cnx, const sx_op *input, const sx_op *e,
			     const sx_op *m, sx_op *result)
{
	struct sx_pk_acq_req pkreq;
	int status;

	pkreq = sx_async_mod_exp_go(cnx, input, e, m);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);
	sx_async_mod_exp_end(pkreq.req, result);
	return status;
}

/** @} */

/**
 * @addtogroup SX_PK_SXOPS_RSA
 *
 * @{
 */

/** Asynchronous (non-blocking) modular exponentiation with CRT.
 *
 * Start a modular exponentiation with CRT on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_crt_mod_exp_end()
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 startup
 * @param[in] in Input
 * @param[in] p Prime number p
 * @param[in] q Prime number q
 * @param[in] dp d mod (p-1), with d the private key
 * @param[in] dq d mod (q-1), with d the private key
 * @param[in] qinv q^(-1) mod p

 * @return Acquired acceleration request for this operation
 *
 * @see sx_async_crt_mod_exp_end()
 */
static inline struct sx_pk_acq_req sx_async_crt_mod_exp_go(struct sx_pk_cnx *cnx, const sx_op *in,
							   const sx_op *p, const sx_op *q,
							   const sx_op *dp, const sx_op *dq,
							   const sx_op *qinv)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_crt_mod_exp inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_MOD_EXP_CRT);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	int sizes[] = {sx_op_size(p),  sx_op_size(q),  sx_op_size(in),
		       sx_op_size(dp), sx_op_size(dq), sx_op_size(qinv)};
	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	sx_pk_op2vmem(in, inputs.in.addr);
	sx_pk_op2vmem(p, inputs.p.addr);
	sx_pk_op2vmem(q, inputs.q.addr);
	sx_pk_op2vmem(dp, inputs.dp.addr);
	sx_pk_op2vmem(dq, inputs.dq.addr);
	sx_pk_op2vmem(qinv, inputs.qinv.addr);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) modular exponentiation with CRT.
 *
 * Get the output operand of the modular exponentiation with CRT
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] result
 *
 * @see sx_async_crt_mod_exp_go()
 */
static inline void sx_async_crt_mod_exp_end(sx_pk_req *req, sx_op *result)
{
	sx_async_finish_single(req, result);
}

/** Compute modular exponentiation with CRT
 *
 *  Compute (result = in ^ db mod m) with those steps:
 *
 *   vp = in ^ dp mod p
 *   vq = in ^ dq mod q
 *   u = (vp -vq) * qinv mod p
 *   result = vq + u * q
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] in Input
 * @param[in] p Prime number p
 * @param[in] q Prime number q
 * @param[in] dp d mod (p-1), with d the private key
 * @param[in] dq d mod (q-1), with d the private key
 * @param[in] qinv q^(-1) mod p
 * @param[out] result Result of modular exponentiation with CRT
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_crt_mod_exp_go(), sx_async_crt_mod_exp_end()
 * for an asynchronous version
 */
static inline int sx_crt_mod_exp(struct sx_pk_cnx *cnx, const sx_op *in, const sx_op *p,
				 const sx_op *q, const sx_op *dp, const sx_op *dq,
				 const sx_op *qinv, sx_op *result)
{
	struct sx_pk_acq_req pkreq;
	int status;

	pkreq = sx_async_crt_mod_exp_go(cnx, in, p, q, dp, dq, qinv);
	if (pkreq.status) {
		return pkreq.status;
	}
	status = sx_pk_wait(pkreq.req);
	sx_async_crt_mod_exp_end(pkreq.req, result);

	return status;
}

/** Asynchronous (non-blocking) RSA private key generation.
 *
 * Start an RSA private key generation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_rsa_keygen_end()
 *
 * @param[in] p Prime value p
 * @param[in] q Prime value q
 * @param[in] public_expo Public exponent operand

 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req sx_async_rsa_keygen_go(const sx_op *p, const sx_op *q,
							  const sx_op *public_expo)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_rsa_keygen inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_RSA_KEYGEN);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	int sizes[] = {sx_op_size(p), sx_op_size(q), sx_op_size(public_expo)};

	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	sx_pk_op2vmem(p, inputs.p.addr);
	sx_pk_op2vmem(q, inputs.q.addr);
	sx_pk_op2vmem(public_expo, inputs.e.addr);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) RSA private key generation.
 *
 * Get the output operands of the RSA private key generation
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] n Multiple of 2 primes p and q
 * @param[out] lambda_n Least common multiple of p-1 and q-1
 * @param[out] privkey Private key
 */
static inline void sx_async_rsa_keygen_end(sx_pk_req *req, sx_op *n, sx_op *lambda_n,
					   sx_op *privkey)
{
	const char **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2op(outputs[0], opsz, n);

	if (lambda_n != NULL) {
		sx_pk_mem2op(outputs[1], opsz, lambda_n);
	}
	sx_pk_mem2op(outputs[2], opsz, privkey);

	sx_pk_release_req(req);
}

/** Compute RSA private key and lambda_n from primes p and q.
 *
 * The private key is generated with the following steps:
 * 1. n = p * q
 * 2. lambda_n = lcm(p-1,q-1)
 * 3. d = e^-1 % lambda_n
 *
 * Where d is the private key (privkey).
 *
 * @param[in] p Prime value p
 * @param[in] q Prime value q
 * @param[in] public_expo Public exponent operand
 * @param[out] n Resulting n operand
 * @param[out] lambda_n Resulting lambda_n operand
 * @param[out] privkey Resulting private key operand
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_rsa_keygen_go(), sx_async_rsa_keygen_end()
 * for an asynchronous version
 */
static inline int sx_rsa_keygen(const sx_op *p, const sx_op *q, const sx_op *public_expo, sx_op *n,
				sx_op *lambda_n, sx_op *privkey)
{
	struct sx_pk_acq_req pkreq;
	int status;

	pkreq = sx_async_rsa_keygen_go(p, q, public_expo);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_async_rsa_keygen_end(pkreq.req, n, lambda_n, privkey);

	return status;
}

/** Asynchronous (non-blocking) RSA CRT private key parameters.
 *
 * Start a RSA CRT private key generation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_rsa_crt_keyparams_end()
 *
 * @param[in] p Prime operand p
 * @param[in] q Prime operand q
 * @param[in] privkey Private key
 *
 * @return Acquired acceleration request for this operation
 *
 * @see sx_async_rsa_crt_keyparams_end() and sx_async_rsa_crt_keyparams()
 */
static inline struct sx_pk_acq_req sx_async_rsa_crt_keyparams_go(const sx_op *p, const sx_op *q,
								 const sx_op *privkey)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_rsa_crt_keyparams inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_RSA_CRT_KEYPARAMS);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	int sizes[] = {sx_op_size(p), sx_op_size(q), sx_op_size(privkey)};

	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	sx_pk_op2vmem(p, inputs.p.addr);
	sx_pk_op2vmem(q, inputs.q.addr);
	sx_pk_op2vmem(privkey, inputs.privkey.addr);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) RSA CRT private key parameters.
 *
 * Get the output operands of the RSA CRT private key parameters
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] dp d mod (p - 1)
 * @param[out] dq d mod (q - 1)
 * @param[out] qinv q ^ -1 mod p
 *
 * @see sx_async_rsa_crt_keyparams_go() and sx_rsa_crt_keyparams()
 */
static inline void sx_async_rsa_crt_keyparams_end(sx_pk_req *req, sx_op *dp, sx_op *dq, sx_op *qinv)
{
	const char **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2op(outputs[0], opsz, dp);
	sx_pk_mem2op(outputs[1], opsz, dq);
	sx_pk_mem2op(outputs[2], opsz, qinv);

	sx_pk_release_req(req);
}

/** Compute RSA CRT private key parameters.
 *
 * Computes the following parameters:
 *  dp = d mod (p - 1)
 *  dq = d mod (q - 1)
 *  qinv = q ^ -1 mod p
 * where d is the private key and the pair p and q are the secret primes used
 * to create the RSA private key.
 *
 * @param[in] p Prime value p
 * @param[in] q Prime value q
 * @param[in] privkey Private key operand
 * @param[out] dp d mod (p - 1)
 * @param[out] dq d mod (q - 1)
 * @param[out] qinv q ^ -1 mod p
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
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
 * @see sx_async_rsa_crt_keyparams_go(), sx_async_rsa_crt_keyparams_end()
 * for an asynchronous version
 */
static inline int sx_rsa_crt_keyparams(const sx_op *p, const sx_op *q, const sx_op *privkey,
				       sx_op *dp, sx_op *dq, sx_op *qinv)
{
	struct sx_pk_acq_req pkreq;
	int status;

	pkreq = sx_async_rsa_crt_keyparams_go(p, q, privkey);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_async_rsa_crt_keyparams_end(pkreq.req, dp, dq, qinv);

	return status;
}

/** @} */

/**
 * @addtogroup SX_PK_SXOPS_MILLER_RABIN
 *
 * @{
 */

/** Asynchronous (non-blocking) Miller Rabin.
 *
 * Start one round of the Miller Rabin test on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_pk_release_req()
 *
 * @param[in] n Number to test as prime value. Must be larger than 2
 * @param[in] a Random value in interval [2, n-2]
 *
 * @return Acquired acceleration request for this operation
 *
 * @see sx_miller_rabin() for a synchronous version
 */
static inline struct sx_pk_acq_req sx_async_miller_rabin_go(const sx_op *n, const sx_op *a)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_miller_rabin inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_MILLER_RABIN);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	int sizes[] = {sx_op_size(n), sx_op_size(a)};

	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	sx_pk_op2vmem(n, inputs.n.addr);
	sx_pk_op2vmem(a, inputs.a.addr);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Run one round of the Miller Rabin primality test
 *
 * The input operand 'n' is the number to test. It must be larger than 2.
 * The input operand 'a' is a random value in interval [2, n-2] which is used
 * to test the primality of 'n'. To check if a given 'n' is probably prime,
 * this test should be executed multiple times with different random
 * values 'a'. If the number to test is composite, the error
 * ::SX_ERR_COMPOSITE_VALUE is returned.
 *
 * @param[in] n Number to test as prime value. Must be larger than 2
 * @param[in] a Random value in interval [2, n-2]
 *
 * @return ::SX_OK
 * @return ::SX_ERR_COMPOSITE_VALUE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 *
 * @see sx_async_miller_rabin_go() for an asynchronous version
 */

static inline int sx_miller_rabin(const sx_op *n, const sx_op *a)
{
	struct sx_pk_acq_req pkreq;
	uint32_t status;

	pkreq = sx_async_miller_rabin_go(n, a);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);
	sx_pk_release_req(pkreq.req);

	return status;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif
