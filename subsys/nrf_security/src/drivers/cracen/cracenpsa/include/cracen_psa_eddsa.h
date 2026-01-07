/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_eddsa
 * @{
 * @brief EdDSA operations for CRACEN PSA driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications should use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_PSA_EDDSA_H
#define CRACEN_PSA_EDDSA_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/** @brief Sign a message using Ed25519.
 *
 * @param[in] priv_key      Private key.
 * @param[out] signature    Buffer to store the signature.
 * @param[in] message       Message to sign.
 * @param[in] message_length Length of the message in bytes.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available, retry later.
 * @retval Other SX status codes from @ref status_codes on internal errors.
 */
int cracen_ed25519_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			size_t message_length);

/** @brief Verify an Ed25519 signature.
 *
 * @param[in] pub_key       Public key.
 * @param[in] message       Message that was signed.
 * @param[in] message_length Length of the message in bytes.
 * @param[in] signature     Signature to verify.
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_PK_RETRY Resources not available, retry later.
 * @retval Other SX status codes from @ref status_codes on internal errors.
 */
int cracen_ed25519_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			  const uint8_t *signature);

/** @brief Sign a message using Ed25519ph (pre-hashed).
 *
 * @param[in] priv_key      Private key.
 * @param[out] signature    Buffer to store the signature.
 * @param[in] message       Message or hash to sign.
 * @param[in] message_length Length of the message in bytes.
 * @param[in] is_message    True if @p message is a message, false if it is a hash.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available, retry later.
 * @retval Other SX status codes from @ref status_codes on internal errors.
 */
int cracen_ed25519ph_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			  size_t message_length, bool is_message);

/** @brief Verify an Ed25519ph signature.
 *
 * @param[in] pub_key       Public key.
 * @param[in] message       Message or hash that was signed.
 * @param[in] message_length Length of the message in bytes.
 * @param[in] signature     Signature to verify.
 * @param[in] is_message    True if @p message is a message, false if it is a hash.
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_PK_RETRY Resources not available, retry later.
 * @retval Other SX status codes from @ref status_codes on internal errors.
 */
int cracen_ed25519ph_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			    const uint8_t *signature, bool is_message);

/** @brief Create an Ed25519 public key from a private key.
 *
 * @param[in] priv_key Private key.
 * @param[out] pub_key  Buffer to store the public key.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available, retry later.
 * @retval Other SX status codes from @ref status_codes on internal errors.
 */
int cracen_ed25519_create_pubkey(const uint8_t *priv_key, uint8_t *pub_key);

/** @brief Sign a message using Ed448.
 *
 * @param[in] priv_key      Private key.
 * @param[out] signature    Buffer to store the signature.
 * @param[in] message       Message to sign.
 * @param[in] message_length Length of the message in bytes.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available, retry later.
 * @retval Other SX status codes from @ref status_codes on internal errors.
 */
int cracen_ed448_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			size_t message_length);

/** @brief Verify an Ed448 signature.
 *
 * @param[in] pub_key       Public key.
 * @param[in] message       Message that was signed.
 * @param[in] message_length Length of the message in bytes.
 * @param[in] signature     Signature to verify.
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_PK_RETRY Resources not available, retry later.
 * @retval Other SX status codes from @ref status_codes on internal errors.
 */
int cracen_ed448_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			  const uint8_t *signature);

/** @brief Sign a message using Ed448ph (pre-hashed).
 *
 * @param[in] priv_key      Private key.
 * @param[out] signature    Buffer to store the signature.
 * @param[in] message       Message or hash to sign.
 * @param[in] message_length Length of the message in bytes.
 * @param[in] is_message    True if @p message is a message, false if it is a hash.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available, retry later.
 * @retval Other SX status codes from @ref status_codes on internal errors.
 */
int cracen_ed448ph_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			  size_t message_length, bool is_message);

/** @brief Verify an Ed448ph signature.
 *
 * @param[in] pub_key       Public key.
 * @param[in] message       Message or hash that was signed.
 * @param[in] message_length Length of the message in bytes.
 * @param[in] signature     Signature to verify.
 * @param[in] is_message    True if @p message is a message, false if it is a hash.
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_PK_RETRY Resources not available, retry later.
 * @retval Other SX status codes from @ref status_codes on internal errors.
 */
int cracen_ed448ph_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			    const uint8_t *signature, bool is_message);

/** @brief Create an Ed448 public key from a private key.
 *
 * @param[in] priv_key Private key.
 * @param[out] pub_key  Buffer to store the public key.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available, retry later.
 * @retval Other SX status codes from @ref status_codes on internal errors.
 */
int cracen_ed448_create_pubkey(const uint8_t *priv_key, uint8_t *pub_key);

/** @} */

#endif /* CRACEN_PSA_EDDSA_H */
