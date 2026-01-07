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

#ifndef CRACEN_PSA_RSA_ENCRYPTION_H
#define CRACEN_PSA_RSA_ENCRYPTION_H

#include "cracen_psa_primitives.h"

/** @brief Encrypt data using RSA OAEP.
 *
 * @param[in] hashalg      Hash algorithm.
 * @param[in] rsa_key      RSA key structure.
 * @param[in] text          Text to encrypt.
 * @param[in] label         Optional label.
 * @param[out] output       Buffer to store the ciphertext.
 * @param[in,out] output_length On input, size of the output buffer in bytes.
 *                              On output, length of the generated ciphertext.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_OUTPUT_BUFFER_TOO_SMALL The output buffer is too small.
 * @retval ::SX_ERR_INVALID_CIPHERTEXT The input plaintext is too large for the key size.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_oaep_encrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *rsa_key,
			    struct cracen_crypt_text *text, struct sx_const_buf *label,
			    uint8_t *output, size_t *output_length);

/** @brief Decrypt data using RSA OAEP.
 *
 * @param[in] hashalg      Hash algorithm.
 * @param[in] rsa_key      RSA key structure.
 * @param[in] text          Ciphertext to decrypt.
 * @param[in] label         Optional label.
 * @param[out] output       Buffer to store the plaintext.
 * @param[in,out] output_length On input, size of the output buffer in bytes.
 *                              On output, length of the decrypted plaintext.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_OUTPUT_BUFFER_TOO_SMALL The output buffer is too small.
 * @retval ::SX_ERR_INVALID_CIPHERTEXT The ciphertext is invalid or decryption failed.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_oaep_decrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *rsa_key,
			    struct cracen_crypt_text *text, struct sx_const_buf *label,
			    uint8_t *output, size_t *output_length);

/** @brief Decrypt data using RSA PKCS#1 v1.5.
 *
 * @param[in] rsa_key      RSA key structure.
 * @param[in] text          Ciphertext to decrypt.
 * @param[out] output       Buffer to store the plaintext.
 * @param[in,out] output_length On input, size of the output buffer in bytes.
 *                              On output, length of the decrypted plaintext.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_OUTPUT_BUFFER_TOO_SMALL The output buffer is too small.
 * @retval ::SX_ERR_INVALID_CIPHERTEXT The ciphertext is invalid or decryption failed.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pkcs1v15_decrypt(struct cracen_rsa_key *rsa_key, struct cracen_crypt_text *text,
				uint8_t *output, size_t *output_length);

/** @brief Encrypt data using RSA PKCS#1 v1.5.
 *
 * @param[in] rsa_key      RSA key structure.
 * @param[in] text          Plaintext to encrypt.
 * @param[out] output       Buffer to store the ciphertext.
 * @param[in,out] output_length On input, size of the output buffer in bytes.
 *                              On output, length of the generated ciphertext.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_OUTPUT_BUFFER_TOO_SMALL The output buffer is too small.
 * @retval ::SX_ERR_INVALID_CIPHERTEXT The input plaintext is too large for the key size.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_pkcs1v15_encrypt(struct cracen_rsa_key *rsa_key, struct cracen_crypt_text *text,
				uint8_t *output, size_t *output_length);

/** @} */

#endif /* CRACEN_PSA_RSA_ENCRYPTION_H */
