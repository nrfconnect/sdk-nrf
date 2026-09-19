/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SAP_CRYPTO_H__
#define SAP_CRYPTO_H__

/**
 * @file
 * @brief PSA Crypto helpers used by the SAP service.
 *
 * Internal to the SAP service, not part of the public API.
 *
 * All keys are volatile. A key ID returned through an output parameter belongs to the
 * caller, who must destroy it with psa_destroy_key(). Keys a helper creates for its own
 * use are destroyed before it returns.
 *
 * Functions return 0 on success or a negative errno. PSA status codes are not passed on.
 */

#include <zephyr/types.h>
#include <psa/crypto.h>

/** @brief Writable byte buffer. */
struct sap_crypto_buffer {
	/** Buffer. */
	uint8_t *data;
	/** Number of bytes to write. */
	size_t len;
};

/** @brief Read-only byte buffer. */
struct sap_crypto_const_buffer {
	/** Buffer. */
	const uint8_t *data;
	/** Number of valid bytes. */
	size_t len;
};

/** @brief Import a P-256 identity private key for ECDSA-SHA-256 signing.
 *
 * @param[in] key Private scalar, big-endian.
 * @param[in] key_len Length of @p key in bytes. 32 for P-256.
 * @param[out] key_id Imported key. Usage is PSA_KEY_USAGE_SIGN_HASH only.
 *
 * @retval 0 If the key was imported.
 * @retval -EIO If PSA rejected the key.
 */
int sap_crypto_import_identity_private(const uint8_t *key, size_t key_len, psa_key_id_t *key_id);

/** @brief Verify an ECDSA-SHA-256 signature with a P-256 identity public key.
 *
 * Hashes @p message with SHA-256 and verifies @p signature over the hash.
 *
 * @param[in] public_key Uncompressed SEC1 point, 65 bytes.
 * @param[in] public_key_len Length of @p public_key in bytes.
 * @param[in] message Signed message.
 * @param[in] message_len Length of @p message in bytes.
 * @param[in] signature Raw r||s signature, 64 bytes.
 * @param[in] signature_len Length of @p signature in bytes.
 *
 * @retval 0 If the signature is valid.
 * @retval -EKEYREJECTED If the signature does not match.
 * @retval -EIO If the key could not be imported or hashing failed.
 */
int sap_crypto_verify_identity(const uint8_t *public_key, size_t public_key_len,
			       const uint8_t *message, size_t message_len, const uint8_t *signature,
			       size_t signature_len);

/** @brief Sign a message with an identity private key.
 *
 * Hashes @p message with SHA-256 and signs the hash with ECDSA.
 *
 * @param[in] key_id Key from @ref sap_crypto_import_identity_private.
 * @param[in] message Message to sign.
 * @param[in] message_len Length of @p message in bytes.
 * @param[out] signature Raw r||s signature.
 * @param[in] signature_size Size of @p signature in bytes. At least 64 for P-256.
 * @param[out] signature_len Number of bytes written to @p signature.
 *
 * @retval 0 If the message was signed.
 * @retval -EIO If hashing or signing failed, including a too-small @p signature.
 */
int sap_crypto_sign_identity(psa_key_id_t key_id, const uint8_t *message, size_t message_len,
			     uint8_t *signature, size_t signature_size, size_t *signature_len);

/** @brief Generate an ephemeral P-256 ECDH key pair.
 *
 * Export the public point with psa_export_public_key(). It is 65 bytes, uncompressed SEC1.
 *
 * @param[out] key_id Generated key pair. Usage is PSA_KEY_USAGE_DERIVE with PSA_ALG_ECDH.
 *
 * @retval 0 If the key pair was generated.
 * @retval -EIO If PSA key generation failed.
 */
int sap_crypto_generate_ecdh_keypair(psa_key_id_t *key_id);

/** @brief Derive key material with HKDF-SHA-256.
 *
 * Writes exactly @c output.len bytes to @c output.data.
 *
 * @param[in] secret Input keying material. SAP passes the raw ECDH shared secret.
 * @param[in] salt HKDF salt.
 * @param[in] info HKDF info.
 * @param[out] output Derived key material.
 *
 * @retval 0 If the key material was derived.
 * @retval -EIO If importing @p secret or any derivation step failed.
 */
int sap_crypto_hkdf_sha256(struct sap_crypto_const_buffer secret,
			   struct sap_crypto_const_buffer salt, struct sap_crypto_const_buffer info,
			   struct sap_crypto_buffer output);

/** @brief Import an AES key for GCM encryption and decryption.
 *
 * @param[in] key Raw AES key.
 * @param[in] key_len Length of @p key in bytes. Must be 16, 24, or 32.
 * @param[out] key_id Imported key. Usage is PSA_KEY_USAGE_ENCRYPT and
 *                    PSA_KEY_USAGE_DECRYPT with PSA_ALG_GCM.
 *
 * @retval 0 If the key was imported.
 * @retval -EIO If PSA rejected the key.
 */
int sap_crypto_import_aes_gcm_key(const uint8_t *key, size_t key_len, psa_key_id_t *key_id);

#endif /* SAP_CRYPTO_H__ */
