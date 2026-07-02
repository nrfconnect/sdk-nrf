/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SAP_CRYPTO_H__
#define SAP_CRYPTO_H__

#include <zephyr/types.h>
#include <psa/crypto.h>

struct sap_crypto_buffer {
	uint8_t *data;
	size_t len;
};

struct sap_crypto_const_buffer {
	const uint8_t *data;
	size_t len;
};

int sap_crypto_import_identity_private(const uint8_t *key, size_t key_len, psa_key_id_t *key_id);
int sap_crypto_verify_identity(const uint8_t *public_key, size_t public_key_len,
			       const uint8_t *message, size_t message_len, const uint8_t *signature,
			       size_t signature_len);
int sap_crypto_sign_identity(psa_key_id_t key_id, const uint8_t *message, size_t message_len,
			     uint8_t *signature, size_t signature_size, size_t *signature_len);

int sap_crypto_generate_ecdh_keypair(psa_key_id_t *key_id);
int sap_crypto_hkdf_sha256(struct sap_crypto_const_buffer secret,
			   struct sap_crypto_const_buffer salt, struct sap_crypto_const_buffer info,
			   struct sap_crypto_buffer output);
int sap_crypto_import_aes_gcm_key(const uint8_t *key, size_t key_len, psa_key_id_t *key_id);

#endif /* SAP_CRYPTO_H__ */
