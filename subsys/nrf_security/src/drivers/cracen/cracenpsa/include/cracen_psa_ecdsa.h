/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_ecdsa
 * @{
 * @brief ECDSA operations for CRACEN PSA driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * @details
 * These ECDSA implementation functions expect the following key and signature formats:
 * - Public key: Uncompressed format as Qx || Qy
 *   (2 × curve_op_size bytes, no 0x04 prefix).
 * - Private key: Scalar d in big-endian format (curve_op_size bytes),
 *   wrapped in struct @ref cracen_ecc_priv_key.
 * - Signature: Concatenated r || s values in big-endian format
 *   (2 × curve_op_size bytes total).
 * - The curve operation size can be obtained through sx_pk_curve_opsize().
 * - If the provided digest is larger than the curve order size, it is
 *   truncated, as per FIPS 186-4 DSS requirements. Digests exceeding
 *   64 bytes (SHA-512) are rejected.
 *
 * These ECDSA implementation functions perform minimal input validation for
 * performance, with the following limitations:
 * - NULL pointers are not checked and cause undefined behavior.
 * - Private key range (1 ≤ d < n) is not validated.
 * - Curve (sx_pk_ecurve) and hash algorithm (sxhashalg) pointers are not validated.
 * - Output buffer sizes are not validated, so undersized buffers cause an overflow.
 */

#ifndef CRACEN_PSA_ECDSA_H
#define CRACEN_PSA_ECDSA_H

#include <psa/crypto.h>
#include <stdint.h>

/** @brief Verify an ECDSA message signature.
 *
 * @param[in] pubkey        Public key.
 * @param[in] hashalg       Hash algorithm to use for message digest,
 *                          given as a sxhashalg struct.
 * @param[in] message       Message that was signed.
 * @param[in] message_length Length of the message in bytes. Can be zero.
 * @param[in] curve         Elliptic curve parameters.
 * @param[in] signature     Signature to verify.
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_POINT_NOT_ON_CURVE The public key point is not on the curve.
 * @retval ::SX_ERR_OUT_OF_RANGE Signature values r or s are out of valid range.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG The hash algorithm is not supported.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ecdsa_verify_message(const uint8_t *pubkey, const struct sxhashalg *hashalg,
				const uint8_t *message, size_t message_length,
				const struct sx_pk_ecurve *curve, const uint8_t *signature);

/** @brief Verify an ECDSA signature over a pre-computed digest.
 *
 * @param[in] pubkey   Public key.
 * @param[in] digest   Digest that was signed.
 * @param[in] digestsz Length of the digest in bytes.
 * @param[in] curve    Elliptic curve parameters.
 * @param[in] signature Signature to verify.
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_POINT_NOT_ON_CURVE The public key point is not on the curve.
 * @retval ::SX_ERR_OUT_OF_RANGE Signature values r or s are out of valid range.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ecdsa_verify_digest(const uint8_t *pubkey, const uint8_t *digest, size_t digestsz,
			       const struct sx_pk_ecurve *curve, const uint8_t *signature);

/** @brief Sign a message using ECDSA.
 *
 * @param[in] privkey       Private key structure.
 * @param[in] hashalg       Hash algorithm to use for message digest.
 * @param[in] curve         Elliptic curve parameters.
 * @param[in] message       Message to sign.
 * @param[in] message_length Length of the message in bytes. Can be zero.
 * @param[out] signature    Buffer to store the signature.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_TOO_MANY_ATTEMPTS Nonce generation failed after maximum
 *         retry attempts (255). This is extremely rare and may indicate
 *         issues with the random number generator.
 * @retval ::SX_ERR_INSUFFICIENT_ENTROPY Insufficient entropy for random number generation.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG The hash algorithm is not supported.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ecdsa_sign_message(const struct cracen_ecc_priv_key *privkey,
			      const struct sxhashalg *hashalg, const struct sx_pk_ecurve *curve,
			      const uint8_t *message, size_t message_length, uint8_t *signature);

/** @brief Sign a digest using ECDSA.
 *
 * @param[in] privkey       Private key structure.
 * @param[in] hashalg       Hash algorithm to use for message digest.
 * @param[in] curve         Elliptic curve parameters.
 * @param[in] digest        Digest to sign.
 * @param[in] digest_length Length of the digest in bytes.
 *                          Maximum 64 bytes due to the SHA-512 limit.
 * @param[out] signature    Buffer to store the signature.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_TOO_BIG The digest length exceeds the maximum supported size (64 bytes).
 * @retval ::SX_ERR_TOO_MANY_ATTEMPTS Nonce generation failed after maximum
 *         retry attempts (255). This is extremely rare and may indicate
 *         issues with the random number generator.
 * @retval ::SX_ERR_INSUFFICIENT_ENTROPY Insufficient entropy for random number generation.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ecdsa_sign_digest(const struct cracen_ecc_priv_key *privkey,
			     const struct sxhashalg *hashalg, const struct sx_pk_ecurve *curve,
			     const uint8_t *digest, size_t digest_length, uint8_t *signature);

/** @brief Sign a message using deterministic ECDSA (RFC 6979).
 *
 * @param[in] privkey       Private key structure.
 * @param[in] hashalg       Hash algorithm to use for signing.
 * @param[in] curve         Elliptic curve parameters.
 * @param[in] message       Message to sign.
 * @param[in] message_length Length of the message in bytes. Can be zero.
 * @param[out] signature    Buffer to store the signature.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_TOO_MANY_ATTEMPTS Deterministic nonce generation failed after maximum
 *         retry attempts (255). This should not occur with valid inputs
 *         and may indicate corrupted key material.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG The hash algorithm is not supported.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ecdsa_sign_message_deterministic(const struct cracen_ecc_priv_key *privkey,
					    const struct sxhashalg *hashalg,
					    const struct sx_pk_ecurve *curve,
					    const uint8_t *message, size_t message_length,
					    uint8_t *signature);

/** @brief Sign a digest using deterministic ECDSA (RFC 6979).
 *
 * @param[in] privkey   Private key structure.
 * @param[in] hashalg   Hash algorithm to use for message digest.
 * @param[in] curve     Elliptic curve parameters.
 * @param[in] digest    Digest to sign.
 * @param[in] digestsz  Length of the digest in bytes.
 * @param[out] signature Buffer to store the signature.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_TOO_MANY_ATTEMPTS Deterministic nonce generation failed after maximum
 *         retry attempts (255). This should not occur with valid inputs
 *         and may indicate corrupted key material.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ecdsa_sign_digest_deterministic(const struct cracen_ecc_priv_key *privkey,
					   const struct sxhashalg *hashalg,
					   const struct sx_pk_ecurve *curve, const uint8_t *digest,
					   size_t digestsz, uint8_t *signature);

/** @} */

#endif /* CRACEN_PSA_ECDSA_H */
