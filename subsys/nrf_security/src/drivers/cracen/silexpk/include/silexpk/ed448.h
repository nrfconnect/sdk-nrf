/** Simpler functions for base Ed448 operations
 *
 * @file
 */
/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ED448_HEADER_FILE
#define ED448_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "core.h"

/** Size in bytes of a reduced value in Ed448 operations */
#define SX_ED448_SZ 57

/** Size in bytes of an encoded Ed448 point */
#define SX_ED448_PT_SZ 57

/** Size in bytes of a digest in Ed448 operations */
#define SX_ED448_DGST_SZ (57 * 2)

/**
 * @addtogroup SX_PK_ED448
 *
 * @{
 */

/** An encoded Ed448 point */
struct sx_ed448_pt {
	/** Bytes array representing encoded point for Ed448 **/
	uint8_t encoded[SX_ED448_PT_SZ];
};

/** A Ed448 scalar value */
struct sx_ed448_v {
	/** Bytes array representing scalar for Ed448 **/
	uint8_t bytes[SX_ED448_SZ];
};

/** A hash digest used in the Ed448 protocol */
struct sx_ed448_dgst {
	/** Bytes array of hash digest **/
	uint8_t bytes[SX_ED448_DGST_SZ];
};

/** EdDSA point multiplication (Ed448)
 *
 * Compute R = r * G, where r is a scalar which can be up to twice the
 * size of the other operands. G is the generator point for the curve.
 * The point R is encoded in pt.
 *
 * When computing the public key, the scalar 'r' is the secret scalar based on
 * the clamped hash digest of the private key.
 * For signature step, the scalar 'r' is a digest based on the message,
 * prefix, context and flag (see rfc8032).
 *
 * @param[in,out] req The acquired acceleration request
 * @param[in] r Scalar
 * @param[out] pt Encoded resulting R point
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
 */
int sx_ed448_ptmult(sx_pk_req *req, const struct sx_ed448_dgst *r, struct sx_ed448_pt *pt);

/** Compute signature scalar s for pure EdDSA (Ed448).
 *
 * This represents the second step in computing an EdDSA signature.
 *
 * This step computes sig_s :
 *   sig_s = (r + k * s) % l
 *
 * @param[in,out] req The acquired acceleration request
 * @param[in] k Hash of the encoded point R, the public key and the message.
 * It is interpreted as a scalar with a size double of other operands
 * @param[in] r Secret nonce already used in the first signature step
 * @param[in] s Secret scalar derived from the private key
 * @param[out] sig_s Second part of the EdDSA signature
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
 */
int sx_ed448_sign(sx_pk_req *req, const struct sx_ed448_dgst *k, const struct sx_ed448_dgst *r,
		  const struct sx_ed448_v *s, struct sx_ed448_v *sig_s);

/** Verify an EdDSA signature (Ed448)
 *
 * It checks if sig_s * G - k * A matches R.
 *
 * sig_s and the encoded point R form the signature. The points A and R are
 * passed in their encoded form via 'a' and 'r'.
 *
 * @param[in,out] req The acquired acceleration request
 * @param[in] k Hash of the encoded point R, the public key and the message.
 * It is interpreted as a scalar with a size double of other operands
 * @param[in] a Encoded public key
 * @param[in] sig_s Second part of the signature
 * @param[in] r Encoded first part of the signature
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 * @return ::SX_ERR_PK_RETRY
 */
int sx_ed448_verify(sx_pk_req *req, const struct sx_ed448_dgst *k, const struct sx_ed448_pt *a,
		    const struct sx_ed448_v *sig_s, const struct sx_ed448_pt *r);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
