/** "sxops" interface for SRP computations
 *
 * Simpler functions to perform Secure Remote Protocol operations. Included
 * directly in some interfaces (like sxbuf or OpenSSL engine). The functions
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

#ifndef SRP_HEADER_FILE
#define SRP_HEADER_FILE

#include <silexpk/core.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "adapter.h"
#include "impl.h"
#include <silexpk/cmddefs/srp.h>
#include <silexpk/version.h>

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_SRC_COMPATIBLE(2, 0, sxopssrp);

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_SXOPS_SRP
 *
 * @{
 */

/** Asynchronous (non-blocking) SRP user key generation.
 *
 * Start a SRP user key generation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_srp_user_keygen_end()
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 startup
 * @param[in] n Safe prime number (N=2*q+1 with q prime)
 * @param[in] g Generator of the multiplicative group
 * @param[in] a Random value
 * @param[in] b k * g^x + g^t with t random value
 * @param[in] x Hash of (s, p) with s a random salt and p the user password
 * @param[in] k Hash value derived by both sides (for example k = H(n, g))
 * @param[in] u Hash of (g^a, b)

 * @return Acquired acceleration request for this operation
 *
 */
static inline struct sx_pk_acq_req
sx_async_srp_user_keygen_go(struct sx_pk_cnx *cnx, const sx_op *n, const sx_op *g, const sx_op *a,
			    const sx_op *b, const sx_op *x, const sx_op *k, const sx_op *u)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_srp_user_keyparams inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_SRP_USER_KEY_GEN);
	if (pkreq.status) {
		return pkreq;
	}

	/* convert and transfer operands */
	int sizes[] = {sx_op_size(n), sx_op_size(g), sx_op_size(a), sx_op_size(b),
		       sx_op_size(x), sx_op_size(k), sx_op_size(u)};
	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}
	sx_pk_op2vmem(n, inputs.n.addr);
	sx_pk_op2vmem(g, inputs.g.addr);
	sx_pk_op2vmem(a, inputs.a.addr);
	sx_pk_op2vmem(b, inputs.b.addr);
	sx_pk_op2vmem(x, inputs.x.addr);
	sx_pk_op2vmem(k, inputs.k.addr);
	sx_pk_op2vmem(u, inputs.u.addr);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) SRP user key generation.
 *
 * Get the output operans of the SRP user key generation and
 * release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] s Value to hash to get the strong password key K
 *
 * @see sx_async_srp_user_keygen_go()
 */
static inline void sx_async_srp_user_keygen_end(sx_pk_req *req, sx_op *s)
{
	const char **outputs = sx_pk_get_output_ops(req);
	const int opsz = sx_pk_get_opsize(req);

	sx_pk_mem2op(outputs[0], opsz, s);

	sx_pk_release_req(req);
}

/** Secure Remote Password (SRP): Generate key for user session
 *
 * Computes the following:
 *    s = (b - k * g^x) ^ (a + u * x)
 *
 * @param[in,out] cnx Connection structure obtained through sx_pk_open() at
 * startup
 * @param[in] n Safe prime number (N=2*q+1 with q prime)
 * @param[in] g Generator of the multiplicative group
 * @param[in] a Random value
 * @param[in] b k * g^x + g^t with t random value, k value derived by both sides
 * (for example k = H(n, g))
 * @param[in] x Hash of (s, p) with s a random salt and p the user password
 * @param[in] k Hash of (n, g)
 * @param[in] u Hash of (g^a, b)
 * @param[out] s Value to hash to get the strong password key K
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
 * @see sx_async_srp_user_keygen_go() and sx_async_srp_user_keygen_end()
 * for an asynchronous version
 */
static inline int sx_srp_user_keygen(struct sx_pk_cnx *cnx, const sx_op *n, const sx_op *g,
				     const sx_op *a, const sx_op *b, const sx_op *x, const sx_op *k,
				     const sx_op *u, sx_op *s)
{
	struct sx_pk_acq_req pkreq;
	int status;

	pkreq = sx_async_srp_user_keygen_go(cnx, n, g, a, b, x, k, u);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_async_srp_user_keygen_end(pkreq.req, s);

	return status;
}

/** Asynchronous SRP server public key generation
 *
 * Start an SRP server public key generation operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_srp_server_public_key_gen_end()
 *
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] n Safe prime number (N=2*q+1 with q prime)
 * @param[in] g Generator
 * @param[in] k Hash value derived by both sides (for example k = H(n, g))
 * @param[in] v Exponentiated hash digest
 * @param[in] b k * g^x + g^t with t random value
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req
sx_async_srp_server_public_key_gen_go(struct sx_pk_cnx *cnx, const sx_op *n, const sx_op *g,
				      const sx_op *k, const sx_op *v, const sx_op *b)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_srp_server_public_key_gen inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_SRP_SERVER_PUBLIC_KEY_GEN);
	if (pkreq.status) {
		return pkreq;
	}

	int sizes[] = {sx_op_size(n), sx_op_size(g), sx_op_size(k), sx_op_size(v), sx_op_size(b)};

	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}

	sx_pk_op2vmem(n, inputs.n.addr);
	sx_pk_op2vmem(g, inputs.g.addr);
	sx_pk_op2vmem(k, inputs.k.addr);
	sx_pk_op2vmem(v, inputs.v.addr);
	sx_pk_op2vmem(b, inputs.b.addr);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) SRP server public key generation.
 *
 * Get the output operands of the SRP server public key generation
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] rb The resulting value
 */
static inline void sx_async_srp_server_public_key_gen_end(sx_pk_req *req, sx_op *rb)
{
	sx_async_finish_single(req, rb);
}

/** Perform an SRP server public key generation.
 *
 * The SRP server public key generation has the following steps:
 *   1. B = k*v + g^b
 *
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] n Safe prime number (N=2*q+1 with q prime)
 * @param[in] g Generator
 * @param[in] k Hash value derived by both sides (for example k = H(n, g))
 * @param[in] v Exponentiated hash digest
 * @param[in] b k * g^x + g^t with t random value
 *
 * Truncation or padding should be done by user application
 * @param[out] rb The result
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
 * @see sx_async_srp_server_public_key_gen_go(),
 * sx_async_srp_server_public_key_gen_end() for an asynchronous version
 */
static inline int sx_srp_server_public_key_gen(struct sx_pk_cnx *cnx, const sx_op *n,
					       const sx_op *g, const sx_op *k, const sx_op *v,
					       const sx_op *b, sx_op *rb)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_srp_server_public_key_gen_go(cnx, n, g, k, v, b);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_async_srp_server_public_key_gen_end(pkreq.req, rb);

	return status;
}

/** Asynchronous SRP server session key generation
 *
 * Start an SRP server session key generation operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_srp_server_public_key_gen_end()
 *
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] n Safe prime number (N=2*q+1 with q prime)
 * @param[in] a Random
 * @param[in] u Hash of (g^a, b)
 * @param[in] v Exponentiated hash digest
 * @param[in] b k * g^x + g^t with t random value
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline struct sx_pk_acq_req
sx_async_srp_server_session_key_gen_go(struct sx_pk_cnx *cnx, const sx_op *n, const sx_op *a,
				       const sx_op *u, const sx_op *v, const sx_op *b)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_srp_server_session_key_gen inputs;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_SRP_SERVER_SESSION_KEY_GEN);
	if (pkreq.status) {
		return pkreq;
	}

	int sizes[] = {sx_op_size(n), sx_op_size(a), sx_op_size(u), sx_op_size(v), sx_op_size(b)};

	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return pkreq;
	}

	sx_pk_op2vmem(n, inputs.n.addr);
	sx_pk_op2vmem(a, inputs.a.addr);
	sx_pk_op2vmem(u, inputs.u.addr);
	sx_pk_op2vmem(v, inputs.v.addr);
	sx_pk_op2vmem(b, inputs.b.addr);

	sx_pk_run(pkreq.req);

	return pkreq;
}

/** Finish asynchronous (non-blocking) SRP server session key generation.
 *
 * Get the output operands of the SRP server session key generation
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] rs The resulting value
 */
static inline void sx_async_srp_server_session_key_gen_end(sx_pk_req *req, sx_op *rs)
{
	sx_async_finish_single(req, rs);
}

/** Perform an SRP server session key generation.
 *
 * The SRP server session key generation has the following steps:
 *   1. (a * v ^ u) ^ b
 *
 * @param[in] cnx Connection structure obtained through sx_pk_open() at startup
 * @param[in] n Safe prime number (N=2*q+1 with q prime)
 * @param[in] a Random
 * @param[in] u Hash of (g^a, b)
 * @param[in] v Exponentiated hash digest
 * @param[in] b k * g^x + g^t with t random value
 *
 * Truncation or padding should be done by user application
 * @param[out] rs The result
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
 * @see sx_async_srp_server_session_key_gen_go(),
 * sx_async_srp_server_session_key_gen_end() for an asynchronous version
 */
static inline int sx_srp_server_session_key_gen(struct sx_pk_cnx *cnx, const sx_op *n,
						const sx_op *a, const sx_op *u, const sx_op *v,
						const sx_op *b, sx_op *rs)
{
	uint32_t status;
	struct sx_pk_acq_req pkreq;

	pkreq = sx_async_srp_server_session_key_gen_go(cnx, n, a, u, v, b);
	if (pkreq.status) {
		return pkreq.status;
	}

	status = sx_pk_wait(pkreq.req);

	sx_async_srp_server_session_key_gen_end(pkreq.req, rs);

	return status;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif
