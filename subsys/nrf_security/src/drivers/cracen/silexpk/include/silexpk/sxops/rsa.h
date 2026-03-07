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
#include <cracen/statuscodes.h>
#include <silexpk/cmddefs/modmath.h>
#include <silexpk/cmddefs/rsa.h>
#include <silexpk/cmddefs/modexp.h>
#include <silexpk/version.h>

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_SRC_COMPATIBLE(2, 0, sxopsrsa);

struct sx_pk_cmd_def;

/** Synchronous compute single operand modular operation
 *
 *  - result = b mod modulo for ::SX_PK_CMD_ODD_MOD_REDUCE,
 * ::SX_PK_CMD_EVEN_MOD_REDUCE
 *  - result = 1 / b mod modulo for ::SX_PK_CMD_ODD_MOD_INV,
 * ::SX_PK_CMD_EVEN_MOD_INV
 *
 * @param[in,out] req Acquired acceleration request for this operation
 * @param[in] cmd Command definition. Should be a primitive modular
 * operation with 1 operand. For example:
 *    ::y, ::SX_PK_CMD_ODD_MOD_INV,
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
 */
static inline int sx_mod_single_op_cmd(sx_pk_req *req,
					   const struct sx_pk_cmd_def *cmd,
					   const sx_const_op *modulo, const sx_const_op *b,
					   sx_op *result)
{
	int status;
	struct sx_pk_inops_mod_single_op_cmd inputs;

	sx_pk_set_cmd(req, cmd);

	/* convert and transfer operands */
	int sizes[] = {
		sx_const_op_size(modulo),
		sx_const_op_size(b),
	};

	status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}
	sx_pk_op2vmem(modulo, inputs.n.addr);
	sx_pk_op2vmem(b, inputs.b.addr);

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}
	const uint8_t **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2op(outputs[0], opsz, result);

	return status;
}

/** Synchronous primitive modular operation with 2 operands
 *
 * - result = a + b mod modulo for ::SX_PK_CMD_MOD_ADD
 * - result = a - b mod modulo for ::SX_PK_CMD_MOD_SUB
 * - result = a * b mod modulo for ::SX_PK_CMD_ODD_MOD_MULT with odd modulus
 * - result = a / b mod modulo for ::SX_PK_CMD_ODD_MOD_DIV with odd modulus
 *
 * Perform a modular addition or subtraction or
 * odd modular multiplication or odd modular division.
 *
 * @param[in,out] req Acquired acceleration request for this operation
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
 */
static inline int sx_mod_primitive_cmd(sx_pk_req *req, struct sx_pk_cnx *cnx,
						const struct sx_pk_cmd_def *cmd,
						const sx_const_op *modulo,
						const sx_const_op *a,
						const sx_const_op *b, sx_op *result)
{

	int status;
	struct sx_pk_inops_mod_cmd inputs;

	sx_pk_set_cmd(req, cmd);

	/* convert and transfer operands */
	int sizes[] = {
		sx_const_op_size(modulo),
		sx_const_op_size(a),
		sx_const_op_size(b),
	};
	status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}
	sx_pk_op2vmem(modulo, inputs.n.addr);
	sx_pk_op2vmem(a, inputs.a.addr);
	sx_pk_op2vmem(b, inputs.b.addr);

	sx_pk_run(req);

	status = sx_pk_wait(req);

	const uint8_t **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2op(outputs[0], opsz, result);

	return status;
}

/** Compute modular exponentiation.
 *
 *    result = input ^ e mod m
 *
 * @param[in,out] req Acquired acceleration request for this operation
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
 */
static inline int sx_mod_exp(sx_pk_req *req, struct sx_pk_cnx *cnx, const sx_const_op *input,
					const sx_const_op *e, const sx_const_op *m, sx_op *result)
{
	struct sx_pk_inops_mod_cmd inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_MOD_EXP);

	/* convert and transfer operands */
	int sizes[] = {
		sx_const_op_size(m),
		sx_const_op_size(input),
		sx_const_op_size(e),
	};
	status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}
	sx_pk_op2vmem(m, inputs.n.addr);
	sx_pk_op2vmem(input, inputs.a.addr);
	sx_pk_op2vmem(e, inputs.b.addr);

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2op(outputs[0], opsz, result);
	return status;
}

/** @} */

/** Compute modular exponentiation with CRT
 *
 *  Compute (result = in ^ db mod m) with those steps:
 *
 *   vp = in ^ dp mod p
 *   vq = in ^ dq mod q
 *   u = (vp -vq) * qinv mod p
 *   result = vq + u * q
 *
 * @param[in,out] req Acquired acceleration request for this operation
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
 */
static inline int sx_crt_mod_exp(sx_pk_req *req, struct sx_pk_cnx *cnx,
					const sx_const_op *in, const sx_const_op *p,
					const sx_const_op *q, const sx_const_op *dp,
					const sx_const_op *dq, const sx_const_op *qinv,
					sx_op *result)
{
	struct sx_pk_inops_crt_mod_exp inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_MOD_EXP_CRT);

	/* convert and transfer operands */
	int sizes[] = {sx_const_op_size(p),  sx_const_op_size(q),  sx_const_op_size(in),
		       sx_const_op_size(dp), sx_const_op_size(dq), sx_const_op_size(qinv)};
	status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}
	sx_pk_op2vmem(in, inputs.in.addr);
	sx_pk_op2vmem(p, inputs.p.addr);
	sx_pk_op2vmem(q, inputs.q.addr);
	sx_pk_op2vmem(dp, inputs.dp.addr);
	sx_pk_op2vmem(dq, inputs.dq.addr);
	sx_pk_op2vmem(qinv, inputs.qinv.addr);

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2op(outputs[0], opsz, result);

	return status;
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
 * @param[out] req The acquired acceleration request for this operation
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
 */
static inline int sx_rsa_keygen(sx_pk_req *req, const sx_const_op *p, const sx_const_op *q,
				const sx_const_op *public_expo, sx_op *n, sx_op *lambda_n,
				sx_op *privkey)
{
	struct sx_pk_inops_rsa_keygen inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_RSA_KEYGEN);

	/* convert and transfer operands */
	int sizes[] = {sx_const_op_size(p), sx_const_op_size(q), sx_const_op_size(public_expo)};

	status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}

	sx_pk_op2vmem(p, inputs.p.addr);
	sx_pk_op2vmem(q, inputs.q.addr);
	sx_pk_op2vmem(public_expo, inputs.e.addr);

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2op(outputs[0], opsz, n);

	if (lambda_n != NULL) {
		sx_pk_mem2op(outputs[1], opsz, lambda_n);
	}
	sx_pk_mem2op(outputs[2], opsz, privkey);

	return status;
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
 * @param[out] req The acquired acceleration request for this operation
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
 */
static inline int sx_rsa_crt_keyparams(sx_pk_req *req, const sx_const_op *p, const sx_const_op *q,
				       const sx_const_op *privkey, sx_op *dp, sx_op *dq,
				       sx_op *qinv)
{
	struct sx_pk_inops_rsa_crt_keyparams inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_RSA_CRT_KEYPARAMS);

	/* convert and transfer operands */
	int sizes[] = {sx_const_op_size(p), sx_const_op_size(q), sx_const_op_size(privkey)};

	status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}
	sx_pk_op2vmem(p, inputs.p.addr);
	sx_pk_op2vmem(q, inputs.q.addr);
	sx_pk_op2vmem(privkey, inputs.privkey.addr);

	sx_pk_run(req);

	status = sx_pk_wait(req);
	if (status != SX_OK) {
		return status;
	}

	const uint8_t **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2op(outputs[0], opsz, dp);
	sx_pk_mem2op(outputs[1], opsz, dq);
	sx_pk_mem2op(outputs[2], opsz, qinv);

	return status;
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
 * @param[out] req The acquired acceleration request for this operation
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
 */

static inline int sx_miller_rabin(sx_pk_req *req, const sx_const_op *n, const sx_const_op *a)
{
	struct sx_pk_inops_miller_rabin inputs;
	int status;

	sx_pk_set_cmd(req, SX_PK_CMD_MILLER_RABIN);

	/* convert and transfer operands */
	int sizes[] = {sx_const_op_size(n), sx_const_op_size(a)};

	status = sx_pk_list_gfp_inslots(req, sizes, (struct sx_pk_slot *)&inputs);
	if (status != SX_OK) {
		return status;
	}
	sx_pk_op2vmem(n, inputs.n.addr);
	sx_pk_op2vmem(a, inputs.a.addr);

	sx_pk_run(req);

	status = sx_pk_wait(req);

	return status;
}

#ifdef __cplusplus
}
#endif

#endif
