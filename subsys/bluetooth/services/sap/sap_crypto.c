/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <bluetooth/services/sap_protocol.h>

#include "sap_crypto.h"

int sap_crypto_import_identity_private(const uint8_t *key, size_t key_len, psa_key_id_t *key_id)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attributes, 256);

	status = psa_import_key(&attributes, key, key_len, key_id);
	psa_reset_key_attributes(&attributes);

	return (status == PSA_SUCCESS) ? 0 : -EIO;
}

int sap_crypto_verify_identity(const uint8_t *public_key, size_t public_key_len,
			       const uint8_t *message, size_t message_len, const uint8_t *signature,
			       size_t signature_len)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = 0;
	uint8_t hash[32];
	psa_status_t status;
	size_t hash_len;
	int err = 0;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attributes, 256);

	status = psa_import_key(&attributes, public_key, public_key_len, &key_id);
	psa_reset_key_attributes(&attributes);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	status = psa_hash_compute(PSA_ALG_SHA_256, message, message_len, hash, sizeof(hash),
				  &hash_len);
	if (status != PSA_SUCCESS || hash_len != sizeof(hash)) {
		(void)psa_destroy_key(key_id);
		return -EIO;
	}

	status = psa_verify_hash(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), hash, sizeof(hash),
				 signature, signature_len);
	if (status != PSA_SUCCESS) {
		err = -EKEYREJECTED;
	}

	(void)psa_destroy_key(key_id);
	return err;
}

int sap_crypto_sign_identity(psa_key_id_t key_id, const uint8_t *message, size_t message_len,
			     uint8_t *signature, size_t signature_size, size_t *signature_len)
{
	uint8_t hash[32];
	psa_status_t status;
	size_t hash_len;

	status = psa_hash_compute(PSA_ALG_SHA_256, message, message_len, hash, sizeof(hash),
				  &hash_len);
	if (status != PSA_SUCCESS || hash_len != sizeof(hash)) {
		return -EIO;
	}

	status = psa_sign_hash(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), hash, sizeof(hash),
			       signature, signature_size, signature_len);
	return (status == PSA_SUCCESS) ? 0 : -EIO;
}

int sap_crypto_generate_ecdh_keypair(psa_key_id_t *key_id)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attributes, 256);

	status = psa_generate_key(&attributes, key_id);
	psa_reset_key_attributes(&attributes);

	return (status == PSA_SUCCESS) ? 0 : -EIO;
}

int sap_crypto_hkdf_sha256(struct sap_crypto_const_buffer secret,
			   struct sap_crypto_const_buffer salt, struct sap_crypto_const_buffer info,
			   struct sap_crypto_buffer output)
{
	psa_key_attributes_t input_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_key_id_t secret_id = 0;
	psa_status_t status;
	int err = 0;

	psa_set_key_usage_flags(&input_attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&input_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&input_attributes, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	psa_set_key_type(&input_attributes, PSA_KEY_TYPE_DERIVE);
	psa_set_key_bits(&input_attributes, secret.len * 8U);

	status = psa_import_key(&input_attributes, secret.data, secret.len, &secret_id);
	psa_reset_key_attributes(&input_attributes);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	status = psa_key_derivation_setup(&operation, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto out;
	}

	status = psa_key_derivation_input_bytes(&operation, PSA_KEY_DERIVATION_INPUT_SALT,
						salt.data, salt.len);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto out;
	}

	status = psa_key_derivation_input_key(&operation, PSA_KEY_DERIVATION_INPUT_SECRET,
					      secret_id);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto out;
	}

	status = psa_key_derivation_input_bytes(&operation, PSA_KEY_DERIVATION_INPUT_INFO,
						info.data, info.len);
	if (status != PSA_SUCCESS) {
		err = -EIO;
		goto out;
	}

	status = psa_key_derivation_output_bytes(&operation, output.data, output.len);
	if (status != PSA_SUCCESS) {
		err = -EIO;
	}

out:
	(void)psa_key_derivation_abort(&operation);
	(void)psa_destroy_key(secret_id);
	return err;
}

int sap_crypto_import_aes_gcm_key(const uint8_t *key, size_t key_len, psa_key_id_t *key_id)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, key_len * 8U);

	status = psa_import_key(&attributes, key, key_len, key_id);
	psa_reset_key_attributes(&attributes);

	return (status == PSA_SUCCESS) ? 0 : -EIO;
}
