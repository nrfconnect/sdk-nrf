/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_rsa_signatures
 * @{
 * @brief RSA PSS signature operations for CRACEN PSA driver (internal use only).
 *
 * @warning These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_PSA_RSA_SIGNATURE_PSS_H
#define CRACEN_PSA_RSA_SIGNATURE_PSS_H

#include <cracen_psa_primitives.h>

/** @brief Sign a message using RSA PSS.
 *
 * @param[in] rsa_key        RSA key structure. Must not be NULL.
 * @param[out] signature     Signature structure. Must not be NULL.
 *                           On input, sz must contain the buffer size (must be
 *                           at least the RSA modulus size). The r field must point
 *                           to a pre-allocated buffer of at least sz bytes.
 *                           On output, sz is set to the actual signature length,
 *                           and r contains the signature.
 * @param[in] hashalg        Hash algorithm descriptor for both message hashing and
 *                           MGF1. Must not be NULL.
 * @param[in] message        Message to sign. Must not be NULL (unless @p message_length is 0).
 * @param[in] message_length Length of the message in bytes. Can be 0.
 * @param[in] saltsz         Salt size in bytes. Must satisfy:
 *                           saltsz <= emLen - digestsz - 2, where emLen is the
 *                           encoded message length (typically equals modulus size).
 *
 * @retval 0 (::SX_OK)                       Operation completed successfully.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG The hash algorithm is not supported.
 * @retval ::SX_ERR_INVALID_ARG              RSA modulus has zero in most significant byte.
 * @retval ::SX_ERR_INSUFFICIENT_ENTROPY     Insufficient entropy for salt generation.
 * @retval ::SX_ERR_INPUT_BUFFER_TOO_SMALL   Signature buffer too small, digest too short,
 *                                           or encoded message too small for salt + hash.
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient.
 * @retval ::SX_ERR_PK_RETRY                 Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pss_sign_message(struct cracen_rsa_key *rsa_key, struct cracen_signature *signature,
				const struct sxhashalg *hashalg, const uint8_t *message,
				size_t message_length, size_t saltsz);

/** @brief Sign a digest using RSA PSS.
 *
 * @param[in] rsa_key       RSA private key structure. Must not be NULL.
 *                          The most significant byte of the modulus must not be zero.
 * @param[out] signature    Signature structure to populate.  Must not be NULL.
 *                          On input, sz must contain the buffer size (must be
 *                          at least the RSA modulus size).
 *                          On output, sz is set to the actual signature length.
 * @param[in] hashalg       Hash algorithm descriptor used for MGF1.
 *                          Must match the algorithm used to compute the digest.
 *                          Must not be NULL.
 * @param[in] input         Digest to sign. Must not be NULL.
 *                          Parameter name in signature is input for historical reasons.
 * @param[in] input_length  Length of the digest in bytes. Must be at least the
 *                          output size of the specified hash algorithm.
 *                          Parameter name in signature is input_length.
 * @param[in] saltsz        Salt size in bytes. Must satisfy:
 *                          saltsz <= emLen - digestsz - 2, where emLen is the
 *                          encoded message length (typically equals modulus size).
 *
 * @retval 0 (::SX_OK)                       Operation completed successfully.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG     The hash algorithm is not supported.
 * @retval ::SX_ERR_INVALID_ARG              RSA modulus has zero in most significant byte.
 * @retval ::SX_ERR_INPUT_BUFFER_TOO_SMALL   Signature buffer too small (signature->sz < modulussz),
 *                                           digest too short, or salt too large for key/hash combo.
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient.
 * @retval ::SX_ERR_INSUFFICIENT_ENTROPY     Insufficient entropy for salt generation.
 * @retval ::SX_ERR_PK_RETRY                 Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pss_sign_digest(struct cracen_rsa_key *rsa_key, struct cracen_signature *signature,
			       const struct sxhashalg *hashalg, const uint8_t *input,
			       size_t input_length, size_t saltsz);

/** @brief Verify a message signature using RSA PSS.
 *
 * @param[in] rsa_key        RSA key structure. Must not be NULL.
 *                           The most significant byte of the modulus must not be zero.
 * @param[in] signature      Signature to verify. Must not be NULL. The r field must
 *                           point to the signature data, and sz must contain its length
 *                           (must be at least the RSA modulus size).
 * @param[in] hashalg        Hash algorithm. Must match the algorithm used
 *                           during signing. Must not be NULL.
 * @param[in] message        Original message that was signed. Must not be NULL
 *                           (unless message_length is 0).
 * @param[in] message_length Length of the message in bytes. Can be 0.
 * @param[in] saltsz         Salt size in bytes. Must match the salt size
 *                           used during signing.
 *
 * @retval 0 (::SX_OK)                       The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE        The signature verification failed.
 * @retval ::SX_ERR_INVALID_ARG              RSA modulus has zero in most significant byte.
 * @retval ::SX_ERR_INPUT_BUFFER_TOO_SMALL   Signature too short, or salt/hash don't fit
 *                                           in encoded message.
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG     The hash algorithm is not supported.
 * @retval ::SX_ERR_PK_RETRY                 Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pss_verify_message(struct cracen_rsa_key *rsa_key,
				  struct cracen_const_signature *signature,
				  const struct sxhashalg *hashalg, const uint8_t *message,
				  size_t message_length, size_t saltsz);

/** @brief Verify a digest signature using RSA PSS.
 *
 * @param[in] rsa_key       RSA key structure. Must not be NULL.
 *                          The most significant byte of the modulus must not be zero.
 * @param[in] signature     Signature to verify. Must not be NULL. The sz field
 *                          must be at least the RSA modulus size.
 * @param[in] hashalg       Hash algorithm. Must match the algorithm used
 *                          during signing. Must not be NULL.
 * @param[in] digest        Digest that was signed. Must not be NULL.
 * @param[in] digest_length Length of the digest in bytes.  Must be at least the
 *                          output size of the specified hash algorithm.
 * @param[in] saltsz        Expected salt size in bytes. Must match the salt size
 *                          used during signing.
 *
 * @retval 0 (::SX_OK)                       The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE        The signature verification failed.
 * @retval ::SX_ERR_INVALID_ARG              RSA modulus has zero in most significant byte.
 * @retval ::SX_ERR_INPUT_BUFFER_TOO_SMALL   Signature too short, digest too short, or
 *                                           salt/hash don't fit in encoded message.
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG     The hash algorithm is not supported.
 * @retval ::SX_ERR_PK_RETRY                 Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pss_verify_digest(struct cracen_rsa_key *rsa_key,
				 struct cracen_const_signature *signature,
				 const struct sxhashalg *hashalg, const uint8_t *digest,
				 size_t digest_length, size_t saltsz);

/** @} */

#endif /* CRACEN_PSA_RSA_SIGNATURE_PSS_H */
