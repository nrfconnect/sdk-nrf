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
 * @param[in] rsa_key       RSA key structure.
 * @param[out] signature    Signature structure to populate.
 * @param[in] hashalg       Hash algorithm.
 * @param[in] message       Message to sign.
 * @param[in] message_length Length of the message in bytes.
 * @param[in] saltsz        Salt size in bytes.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG The hash algorithm is not supported.
 * @retval ::SX_ERR_INSUFFICIENT_ENTROPY Insufficient entropy for salt generation.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pss_sign_message(struct cracen_rsa_key *rsa_key, struct cracen_signature *signature,
				const struct sxhashalg *hashalg, const uint8_t *message,
				size_t message_length, size_t saltsz);

/** @brief Sign a digest using RSA PSS.
 *
 * @param[in] rsa_key       RSA key structure.
 * @param[out] signature    Signature structure to populate.
 * @param[in] hashalg       Hash algorithm.
 * @param[in] input         Digest to sign.
 * @param[in] input_length  Length of the digest in bytes.
 * @param[in] saltsz        Salt size in bytes.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG The hash algorithm is not supported.
 * @retval ::SX_ERR_INSUFFICIENT_ENTROPY Insufficient entropy for salt generation.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pss_sign_digest(struct cracen_rsa_key *rsa_key, struct cracen_signature *signature,
			       const struct sxhashalg *hashalg, const uint8_t *input,
			       size_t input_length, size_t saltsz);

/** @brief Verify a message signature using RSA PSS.
 *
 * @param[in] rsa_key       RSA key structure.
 * @param[in] signature     Signature to verify.
 * @param[in] hashalg       Hash algorithm.
 * @param[in] message       Message that was signed.
 * @param[in] message_length Length of the message in bytes.
 * @param[in] saltsz        Salt size in bytes.
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG The hash algorithm is not supported.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pss_verify_message(struct cracen_rsa_key *rsa_key,
				  struct cracen_const_signature *signature,
				  const struct sxhashalg *hashalg, const uint8_t *message,
				  size_t message_length, size_t saltsz);

/** @brief Verify a digest signature using RSA PSS.
 *
 * @param[in] rsa_key       RSA key structure.
 * @param[in] signature     Signature to verify.
 * @param[in] hashalg       Hash algorithm.
 * @param[in] digest        Digest that was signed.
 * @param[in] digest_length Length of the digest in bytes.
 * @param[in] saltsz        Salt size in bytes.
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_UNSUPPORTED_HASH_ALG The hash algorithm is not supported.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pss_verify_digest(struct cracen_rsa_key *rsa_key,
				 struct cracen_const_signature *signature,
				 const struct sxhashalg *hashalg, const uint8_t *digest,
				 size_t digest_length, size_t saltsz);

/** @} */

#endif /* CRACEN_PSA_RSA_SIGNATURE_PSS_H */
