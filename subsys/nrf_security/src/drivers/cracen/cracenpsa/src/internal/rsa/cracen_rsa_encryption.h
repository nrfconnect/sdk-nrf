/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_rsa_encryption
 * @{
 * @brief RSA encryption operations for CRACEN PSA driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_RSA_ENCRYPTION_H
#define CRACEN_RSA_ENCRYPTION_H

#include <cracen_psa_primitives.h>

/** @brief Encrypt data using RSA OAEP.
 *
 * @param[in] hashalg		    Hash algorithm.
 * @param[in] rsa_key		    RSA key structure.
 * @param[in] text		        Text to encrypt. Must not be NULL.
 * @param[in] label		        Optional label. Use NULL for an empty label.
 * @param[out] output		    Buffer to store the ciphertext. Must be at least
 *				                the size of the RSA modulus.
 * @param[in,out] output_length On input, size of the output buffer in bytes.
 *                              On output, length of the generated ciphertext.
 *
 * @retval 0 (::SX_OK)                       Operation completed successfully.
 * @retval ::SX_ERR_OUTPUT_BUFFER_TOO_SMALL  Output buffer is too small.
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient for key size.
 * @retval ::SX_ERR_PK_RETRY			     Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_oaep_encrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *rsa_key,
			    struct cracen_crypt_text *text, struct sx_const_buf *label,
			    uint8_t *output, size_t *output_length);

/** @brief Decrypt data using RSA OAEP.
 *
 * @param[in] hashalg		    Hash algorithm. Must match
 *				                the algorithm used during encryption.
 * @param[in] rsa_key		    RSA key structure.
 * @param[in] text		        Ciphertext to decrypt. Size must not exceed the
 *				                RSA modulus size.
 * @param[in] label		        Optional label. Use NULL for an empty label.
 * @param[out] output		    Buffer to store the plaintext.
 * @param[in,out] output_length On input, size of the output buffer in bytes.
 *                              On output, length of the decrypted plaintext.
 *
 * @retval 0 (::SX_OK)                       Operation completed successfully.
 * @retval ::SX_ERR_INVALID_ARG              RSA modulus too small for the hash algorithm.
 * @retval ::SX_ERR_TOO_BIG                  Ciphertext larger than the RSA modulus.
 * @retval ::SX_ERR_INVALID_CIPHERTEXT       Decryption failed: invalid ciphertext, wrong key,
 *                                           or label mismatch.
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient for key size.
 * @retval ::SX_ERR_PK_RETRY                 Hardware resources unavailable; retry later.
 */
int cracen_rsa_oaep_decrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *rsa_key,
			    struct cracen_crypt_text *text, struct sx_const_buf *label,
			    uint8_t *output, size_t *output_length);

/** @brief Decrypt data using RSA PKCS#1 v1.5.
 *
 * @param[in] rsa_key		 RSA key structure.
 * @param[in] text		     Ciphertext to decrypt. Size must not exceed the
 *				             RSA modulus size. Ciphertext smaller than the modulus
 *				             is implicitly zero-padded.
 * @param[out] output		 Buffer to store the plaintext.
 * @param[out] output_length On input, size of the output buffer in bytes.
 *                           On output, length of the decrypted plaintext.
 *
 * @retval 0 (::SX_OK)                       Operation completed successfully.
 * @retval ::SX_ERR_INVALID_ARG              RSA modulus too small (must be > 11 bytes).
 * @retval ::SX_ERR_TOO_BIG                  Ciphertext larger than the RSA modulus.
 * @retval ::SX_ERR_INVALID_CIPHERTEXT	     Ciphertext is invalid or decryption failed.
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient for key size.
 * @retval ::SX_ERR_PK_RETRY			     Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pkcs1v15_decrypt(struct cracen_rsa_key *rsa_key, struct cracen_crypt_text *text,
				uint8_t *output, size_t *output_length);

/** @brief Encrypt data using RSA PKCS#1 v1.5.
 *
 * @param[in] rsa_key		    RSA key structure.
 * @param[in] text		        Plaintext to encrypt. Must not be NULL.
 * @param[out] output		    Buffer to store the ciphertext. Must be at least
 *				                the size of the RSA modulus.
 * @param[out] output_length	On input, size of the output buffer in bytes.
 *                              On output, length of the generated ciphertext.
 *
 * @retval 0 (::SX_OK)                       Operation completed successfully.
 * @retval ::SX_ERR_INVALID_ARG              RSA modulus too small (must be > 11 bytes).
 * @retval ::SX_ERR_TOO_BIG                  Plaintext too large (max: modulussz - 11 bytes).
 * @retval ::SX_ERR_TOO_MANY_ATTEMPTS        Random padding generation failed after maximum retries.
 * @retval ::SX_ERR_UNKNOWN_ERROR            Random number generation failed.
 * @retval ::SX_ERR_WORKMEM_BUFFER_TOO_SMALL Internal work memory insufficient for key size.
 */
int cracen_rsa_pkcs1v15_encrypt(struct cracen_rsa_key *rsa_key, struct cracen_crypt_text *text,
				uint8_t *output, size_t *output_length);

/** @} */

#endif /* CRACEN_RSA_ENCRYPTION_H */
