/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_rsa_signatures
 * @{
 * @brief RSA PKCS#1 v1.5 signature operations for CRACEN PSA driver (internal use only).
 *
 * @warning These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_PSA_RSA_SIGNATURE_PKCS1V15_H
#define CRACEN_PSA_RSA_SIGNATURE_PKCS1V15_H

#include <cracen_psa_primitives.h>

/** @brief Sign a message using RSA PKCS#1 v1.5.
 *
 * @param[in] rsa_key        RSA key structure. Must not be NULL.
 * @param[out] signature     Signature structure to populate.  Must not be NULL.
 *                           The r field must point to a pre-allocated buffer of at
 *                           least the RSA modulus size in bytes. The sz field will
 *                           be set to the signature length on success.
 * @param[in] hashalg        Hash algorithm. Must not be NULL.
 * @param[in] message        Message to sign. Must not be NULL (unless @p message_length is 0).
 * @param[in] message_length Length of the message in bytes. Can be 0.
 *
 * @retval 0 (::SX_OK)                       Operation completed successfully.
 * @retval ::SX_ERR_INVALID_ARG              Hash algorithm not supported.
 * @retval ::SX_ERR_INPUT_BUFFER_TOO_SMALL   RSA modulus too small for the hash algorithm
 *                                           (must be at least
 *                                           DER_size + digest_size + 11 bytes).
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient.
 * @retval ::SX_ERR_PK_RETRY                 Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pkcs1v15_sign_message(struct cracen_rsa_key *rsa_key,
				     struct cracen_signature *signature,
				     const struct sxhashalg *hashalg, const uint8_t *message,
				     size_t message_length);

/** @brief Sign a digest using RSA PKCS#1 v1.5.
 *
 * @param[in] rsa_key       RSA key structure. Must not be NULL.
 * @param[out] signature    Signature structure to populate. Must not be NULL.
 *                          The r field must point to a pre-allocated buffer of at
 *                          least the RSA modulus size in bytes.
 * @param[in] hashalg       Hash algorithm. Must not be NULL.
 * @param[in] digest        Pre-computed message digest. Must not be NULL.
 * @param[in] digest_length Length of the digest in bytes. Must be at least the output
 *                          size of the specified hash algorithm.
 *
 * @retval 0 (::SX_OK)                       Operation completed successfully.
 * @retval ::SX_ERR_INVALID_ARG              Hash algorithm not supported.
 *
 * @retval ::SX_ERR_INPUT_BUFFER_TOO_SMALL   @p digest too short for the hash algorithm, or
 *                                           RSA modulus too small for the hash algorithm.
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient.
 * @retval ::SX_ERR_PK_RETRY                 Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pkcs1v15_sign_digest(struct cracen_rsa_key *rsa_key,
				    struct cracen_signature *signature,
				    const struct sxhashalg *hashalg, const uint8_t *digest,
				    size_t digest_length);

/** @brief Verify a message signature using RSA PKCS#1 v1.5.
 *
 * @param[in] rsa_key        RSA key structure. Must not be NULL.
 * @param[in] signature      Signature to verify. Must not be NULL. The r field must
 *                           point to the signature data, and sz must contain its length.
 *                           Signature size must not exceed the RSA modulus size.
 * @param[in] hashalg        Hash algorithm. Must match the algorithm used
 *                           during signing. Must not be NULL.
 * @param[in] message        Original message that was signed. Must not be NULL
 *                           (unless @p message_length is 0).
 * @param[in] message_length Length of the message in bytes. Can be 0.
 *
 * @retval 0 (::SX_OK)                       The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE        The signature verification failed.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG     The hash algorithm is not supported.
 * @retval ::SX_ERR_OUT_OF_RANGE             Signature value exceeds RSA modulus.
 * @retval ::SX_ERR_PK_RETRY                 Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pkcs1v15_verify_message(struct cracen_rsa_key *rsa_key,
				       struct cracen_const_signature *signature,
				       const struct sxhashalg *hashalg, const uint8_t *message,
				       size_t message_length);

/** @brief Verify a digest signature using RSA PKCS#1 v1.5.
 *
 * @param[in] rsa_key       RSA key structure. Must not be NULL.
 * @param[in] signature     Signature to verify. Must not be NULL. The r field must
 *                          point to the signature data, and sz must contain its length.
 * @param[in] hashalg       Hash algorithm that was used to compute the digest.
 *                          Must not be NULL.
 * @param[in] digest        Pre-computed message digest. Must not be NULL.
 * @param[in] digest_length Length of the digest in bytes. Must be at least the output
 *                          size of the specified hash algorithm.
 *
 * @retval 0 (::SX_OK)                       The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE        The signature verification failed.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG     The hash algorithm is not supported.
 * @retval ::SX_ERR_INPUT_BUFFER_TOO_SMALL   Digest too short for the hash algorithm.
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient.
 * @retval ::SX_ERR_OUT_OF_RANGE             Signature value exceeds RSA modulus
 * @retval ::SX_ERR_PK_RETRY                 Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pkcs1v15_verify_digest(struct cracen_rsa_key *rsa_key,
				      struct cracen_const_signature *signature,
				      const struct sxhashalg *hashalg, const uint8_t *digest,
				      size_t digest_length);

/** @} */

#endif /* CRACEN_PSA_RSA_SIGNATURE_PKCS1V15_H */
