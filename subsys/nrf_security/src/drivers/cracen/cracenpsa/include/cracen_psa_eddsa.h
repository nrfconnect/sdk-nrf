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
 * @details
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * Supported variants of EdDSA:
 * - Ed25519: Pure EdDSA using Curve25519 with SHA-512
 * - Ed25519ph: Pre-hashed EdDSA using Curve25519 with SHA-512
 * - Ed448: Pure EdDSA using Curve448 with SHAKE256
 * - Ed448ph: Pre-hashed EdDSA using Curve448 with SHAKE256
 *
 * Key and signature sizes:
 * | Curve     | Private Key | Public Key | Signature |
 * |-----------|-------------|------------|-----------|
 * | Ed25519   | 32 bytes    | 32 bytes   | 64 bytes  |
 * | Ed448     | 57 bytes    | 57 bytes   | 114 bytes |
 *
 * Key formats:
 * - Private key: Raw 32-byte (Ed25519) or 57-byte (Ed448) secret seed.
 *   The internal scalar is derived by hashing and clamping, as specified in RFC 8032.
 * - Public key: Compressed Edwards point encoding (y-coordinate with sign bit).
 * - Signature: Concatenated R || S where R is an encoded point and S is a scalar.
 *
 * Pre-hashed variants (ph):
 * The _ph functions support two modes via the is_message parameter:
 * - is_message = true: The function hashes the message internally before signing/verifying.
 * - is_message = false: The caller provides a pre-computed hash. For Ed25519ph, this must
 *   be a 64-byte SHA-512 digest. For Ed448ph, this must be a 64-byte SHAKE256 digest.
 *
 * These functions perform minimal input validation:
 * - NULL pointers are not checked and cause undefined behavior.
 * - Private key format is not validated so invalid keys produce invalid signatures.
 * - Output buffer sizes are not validated so undersized buffers cause overflow.
 */

#ifndef CRACEN_PSA_EDDSA_H
#define CRACEN_PSA_EDDSA_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/** @brief Sign a message using Ed25519.
 *
 * @param[in] priv_key      Private key (32 bytes).
 * @param[out] signature    Buffer to store the signature. Must be at least 64 bytes.
 * @param[in] message       Message to sign.
 * @param[in] message_length Length of the message in bytes. Can be zero.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ed25519_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			size_t message_length);

/** @brief Verify an Ed25519 signature.
 *
 * @param[in] pub_key       Public key (32 bytes, compressed Edwards point).
 * @param[in] message       Message that was signed.
 * @param[in] message_length Length of the message in bytes.
 * @param[in] signature     Signature to verify (64 bytes, in the R || S format).
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_POINT_NOT_ON_CURVE The public key or signature point R
 *         is not a valid curve point.
 * @retval ::SX_ERR_OUT_OF_RANGE Signature scalar S is out of valid range.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ed25519_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			  const uint8_t *signature);

/** @brief Sign a message using Ed25519ph (pre-hashed).
 *
 * @param[in] priv_key      Private key (32 bytes).
 * @param[out] signature    Buffer to store the signature (minimum 64 bytes).
 * @param[in] message       Message to sign, or pre-computed SHA-512 hash
 *                          if @p is_message is false.
 * @param[in] message_length Length of the @p message in bytes. If @p is_message
 *                           is false, this should be 64 (SHA-512 output size).
 * @param[in] is_message    If true, @p message is hashed internally with
 *                            SHA-512 before signing. If false, @p message is
 *                            treated as a pre-computed 64-byte SHA-512 hash.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ed25519ph_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			  size_t message_length, bool is_message);

/** @brief Verify an Ed25519ph signature.
 *
 * @param[in] pub_key       Public key (32 bytes, compressed Edwards point).
 * @param[in] message       Message that was signed, or a pre-computed SHA-512
 *                          hash if @p is_message is false.
 * @param[in] message_length Length of @p message in bytes. If @p is_message
 *                           is false, this should be 64 (SHA-512 output size).
 * @param[in] signature     Signature to verify (64 bytes).
 * @param[in] is_message    If true, @p message is hashed internally with
 *                          SHA-512 before verification. If false, @p message
 *                          is treated as a pre-computed 64-byte SHA-512 hash.
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_POINT_NOT_ON_CURVE The public key or signature point R
 *         is not a valid curve point.
 * @retval ::SX_ERR_OUT_OF_RANGE Signature scalar S is out of valid range.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ed25519ph_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			    const uint8_t *signature, bool is_message);

/** @brief Create an Ed25519 public key from a private key.
 *
 * @param[in] priv_key Private key (32 bytes).
 * @param[out] pub_key  Buffer to store the public key (minimum 32 bytes).
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ed25519_create_pubkey(const uint8_t *priv_key, uint8_t *pub_key);

/** @brief Sign a message using Ed448.
 *
 * @param[in] priv_key      Private key (57 bytes).
 * @param[out] signature    Buffer to store the signature (minimum 114 bytes).
 * @param[in] message       Message to sign. Can be any length.
 * @param[in] message_length Length of the @p message in bytes. Can be zero.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ed448_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			size_t message_length);

/** @brief Verify an Ed448 signature.
 *
 * @param[in] pub_key       Public key (57 bytes, compressed Edwards point).
 * @param[in] message       Message that was signed.
 * @param[in] message_length Length of the message in bytes.
 * @param[in] signature     Signature to verify (114 bytes, in the R || S format).
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_POINT_NOT_ON_CURVE The public key or signature point R
 *         is not a valid curve point.
 * @retval ::SX_ERR_OUT_OF_RANGE Signature scalar S is out of valid range.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ed448_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			  const uint8_t *signature);

/** @brief Sign a message using Ed448ph (pre-hashed).
 *
 * @param[in] priv_key      Private key (57 bytes).
 * @param[out] signature    Buffer to store the signature (minimum 114 bytes).
 * @param[in] message       Message or hash to sign or a pre-computed SHAKE256 hash
 *                          if @p is_message is false.
 * @param[in] message_length Length of @p message in bytes. If @p is_message
 *                           is false, this should be 64 bytes (SHAKE256-64 output).
 * @param[in] is_message    If true, @p message is hashed internally with
 *                          SHAKE256 (64-byte output) before signing.
 *                          If false, @p message is treated as a pre-computed
 *                          64-byte SHAKE256 hash.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ed448ph_sign(const uint8_t *priv_key, uint8_t *signature, const uint8_t *message,
			  size_t message_length, bool is_message);

/** @brief Verify an Ed448ph signature.
 *
 * @param[in] pub_key       Public key (57 bytes, compressed Edwards point).
 * @param[in] message       Message that was signed, or a pre-computed SHAKE256
 *                          hash if @p is_message is false.
 * @param[in] message_length Length of @p message in bytes. If @p is_message
 *                           is false, this should be 64 bytes (SHAKE256-64 output).
 * @param[in] signature     Signature to verify.
 * @param[in] is_message    If true, @p message is hashed internally with
 *                          SHAKE256 (64-byte output) before signing.
 *                          If false, @p message is treated as a pre-computed
 *                          64-byte SHAKE256 hash.
 *
 * @retval 0 (::SX_OK) The signature is valid.
 * @retval ::SX_ERR_INVALID_SIGNATURE The signature verification failed.
 * @retval ::SX_ERR_POINT_NOT_ON_CURVE The public key or signature point R
 *         is not a valid curve point.
 * @retval ::SX_ERR_OUT_OF_RANGE Signature scalar S is out of valid range.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ed448ph_verify(const uint8_t *pub_key, const uint8_t *message, size_t message_length,
			    const uint8_t *signature, bool is_message);

/** @brief Create an Ed448 public key from a private key.
 *
 * @param[in]  priv_key Private key (57 bytes).
 * @param[out] pub_key  Buffer to store the public key (minimum 57 bytes).
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_ed448_create_pubkey(const uint8_t *priv_key, uint8_t *pub_key);

/** @} */

#endif /* CRACEN_PSA_EDDSA_H */
