/** Simpler functions for base Montgomery elliptic curve operations
 *
 * @file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MONTGOMERY_HEADER_FILE
#define MONTGOMERY_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

/** Size in bytes of an operand on X25519 curve */
#define SX_X25519_OP_SZ 32

/** Size in bytes of an operand on X448 curve */
#define SX_X448_OP_SZ 56

/**
 * @addtogroup SX_PK_MONT
 *
 * @{
 */

/** An X25519 point */
struct sx_x25519_op {
	/** Bytes array representation of a X25519 point **/
	char bytes[SX_X25519_OP_SZ];
};

/** An X448 point */
struct sx_x448_op {
	/** Bytes array representation of a X448 point  **/
	char bytes[SX_X448_OP_SZ];
};

/** Montgomery point multiplication (X25519)
 *
 * Compute r = pt * k
 *
 * The operands must be decoded and clamped as defined in specifications
 * for X25519 and X448.
 *
 * @param[in] k Scalar
 * @param[in] pt Point on the X25519 curve
 * @param[out] r Multiplication result of k and pt
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
 * @see sx_async_x25519_ptmult_go() and sx_async_x25519_ptmult_end()
 * for an asynchronous version
 */
int sx_x25519_ptmult(const struct sx_x25519_op *k, const struct sx_x25519_op *pt,
		     struct sx_x25519_op *r);

/** Asynchronous Montgomery point multiplication (X25519)
 *
 * Start a montgomery point multiplication on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_x25519_ptmult_end()
 *
 * @param[in] k Scalar
 * @param[in] pt Point on the X25519 curve
 *
 * @return Acquired acceleration request for this operation
 *
 * @see sx_async_x25519_ptmult_end() and sx_x25519_ptmult()
 */
struct sx_pk_acq_req sx_async_x25519_ptmult_go(const struct sx_x25519_op *k,
					       const struct sx_x25519_op *pt);

/** Collect the result of asynchronous Montgomery point multiplication (X25519)
 *
 * Get the output operand of the Montgomery point multiplication
 * and release accelerator.
 *
 * @remark The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] r Multiplication result of k and pt
 *
 * @see sx_async_x25519_ptmult_go() and sx_async_x25519_ptmult()
 */
void sx_async_x25519_ptmult_end(sx_pk_req *req, struct sx_x25519_op *r);

/** Montgomery point multiplication (X448)
 *
 * Compute r = pt * k
 *
 * The operands must be decoded and clamped as defined in specifications
 * for X25519 and X448.
 *
 * @param[in] k Scalar
 * @param[in] pt Point on the X448 curve
 * @param[out] r Multiplication result of k and pt
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
 * @see sx_async_x448_ptmult_go() and sx_async_x448_ptmult_end()
 * for an asynchronous version
 */
int sx_x448_ptmult(const struct sx_x448_op *k, const struct sx_x448_op *pt, struct sx_x448_op *r);

/** Asynchronous Montgomery point multiplication (X448)
 *
 * Start a montgomery point multiplication on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_x448_ptmult_end()
 *
 * @param[in] k Scalar
 * @param[in] pt Point on the X448 curve
 *
 * @return Acquired acceleration request for this operation
 *
 * @see sx_async_x448_ptmult_end() and sx_x448_ptmult()
 */
struct sx_pk_acq_req sx_async_x448_ptmult_go(const struct sx_x448_op *k,
					     const struct sx_x448_op *pt);

/** Collect the result of asynchronous Montgomery point multiplication (X448)
 *
 * Get the output operand of the Montgomery point multiplication
 * and release the reserved resources.
 *
 * @remark The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in,out] req The previously acquired acceleration
 * request for this operation
 * @param[out] r Multiplication result of k and pt
 *
 * @see sx_async_x448_ptmult_go() and sx_async_x448_ptmult()
 */
void sx_async_x448_ptmult_end(sx_pk_req *req, struct sx_x448_op *r);

/**  @} */

#ifdef __cplusplus
}
#endif

#endif
