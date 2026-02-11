/** Simpler functions for base Montgomery elliptic curve operations
 *
 * @file
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MONTGOMERY_HEADER_FILE
#define MONTGOMERY_HEADER_FILE

#include <stdint.h>

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
	uint8_t bytes[SX_X25519_OP_SZ];
};

/** An X448 point */
struct sx_x448_op {
	/** Bytes array representation of a X448 point  **/
	uint8_t bytes[SX_X448_OP_SZ];
};

/** Montgomery point multiplication (X25519)
 *
 * Compute r = pt * k
 *
 * The operands are decoded and clamped as defined in specifications
 * for X25519 and X448.
 *
 * @param[in,out] req The previously acquired acceleration
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
 */
int sx_x25519_ptmult(sx_pk_req *req, const struct sx_x25519_op *k,
		const struct sx_x25519_op *pt, struct sx_x25519_op *r);

/** Montgomery point multiplication (X448)
 *
 * Compute r = pt * k
 *
 * The operands are decoded and clamped as defined in specifications
 * for X25519 and X448.
 *
 * @param[in,out] req The previously acquired acceleration
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
 */
int sx_x448_ptmult(sx_pk_req *req, const struct sx_x448_op *k,
		const struct sx_x448_op *pt, struct sx_x448_op *r);

#ifdef __cplusplus
}
#endif

#endif
