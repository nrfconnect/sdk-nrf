/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "key_operations.h"
#include "main.h"

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <cracen_psa_kmu.h>

LOG_MODULE_DECLARE(kmu_cracen_usage, LOG_LEVEL_DBG);

/* ====================================================================== */
/*			Global variables/defines for operations on the KMU keys		  */

#define PRINT_HEX(p_label, p_text, len)                                                            \
	({                                                                                         \
		LOG_INF("---- %s (len: %u): ----", p_label, len);                                  \
		LOG_HEXDUMP_INF(p_text, len, "Content:");                                          \
		LOG_INF("---- %s end  ----", p_label);                                             \
	})

/* Below text is used as plaintext for encryption/decryption */
static uint8_t m_plain_text[NRF_CRYPTO_EXAMPLE_KMU_USAGE_KEY_MAX_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of KMU."};

/* ====================================================================== */

/* AES key operations */

int key_operations_generate_aes_key(psa_key_id_t *key_id)
{
	psa_status_t status;
	psa_key_id_t tmp_key_id;

	if (key_id == NULL) {
		LOG_ERR("Provide a valid key ID to generate the key");
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	tmp_key_id = *key_id;

	LOG_INF("Generating random AES key...");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CTR);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 128);

	/* Persistent key specific settings */
	psa_set_key_lifetime(&key_attributes,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_PERSISTENCE_DEFAULT, PSA_KEY_LOCATION_CRACEN_KMU));
	psa_set_key_id(&key_attributes, *key_id);

	/* Generate a random AES key with persistent lifetime. The key can be used for
	 * encryption/decryption using the key_id.
	 * The key itself will be stored to the KMU peripheral.
	 */
	status = psa_generate_key(&key_attributes, key_id);

	if (status == PSA_ERROR_ALREADY_EXISTS) {
		/* Revering the key ID to the one provided.
		 * Reason: psa_generate_key() modifies it
		 */
		*key_id = tmp_key_id;
		psa_reset_key_attributes(&key_attributes);
		return status;
	} else if (status != PSA_SUCCESS) {
		LOG_INF("psa_generate_key failed! (Error: %d)", status);
		return status;
	}

	/* Make sure the key metadata is not in memory anymore,
	 * has the same affect as resetting the device
	 */
	status = psa_purge_key(*key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_purge_key failed! (Error: %d)", status);
		return status;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	LOG_INF("A key generated successfully!");

	return status;
}

int key_operations_use_aes_key(psa_key_id_t *key_id)
{
	uint32_t olen;
	psa_status_t status;

	if (key_id == NULL) {
		LOG_ERR("Provide a valid key ID to generate the key");
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	uint8_t encrypted_text[PSA_CIPHER_ENCRYPT_OUTPUT_SIZE(
		PSA_KEY_TYPE_AES, PSA_ALG_CTR, NRF_CRYPTO_EXAMPLE_KMU_USAGE_KEY_MAX_TEXT_SIZE)];
	uint8_t decrypted_text[NRF_CRYPTO_EXAMPLE_KMU_USAGE_KEY_MAX_TEXT_SIZE];

	status = psa_cipher_encrypt(*key_id, PSA_ALG_CTR, m_plain_text, sizeof(m_plain_text),
				    encrypted_text, sizeof(encrypted_text), &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_encrypt failed! (Error: %d)", status);
		return status;
	}

	LOG_INF("Encryption successful!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("Encrypted text", encrypted_text, sizeof(encrypted_text));

	status = psa_cipher_decrypt(*key_id, PSA_ALG_CTR, encrypted_text, sizeof(encrypted_text),
				    decrypted_text, sizeof(decrypted_text), &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_decrypt failed! (Error: %d)", status);
		return status;
	}

	PRINT_HEX("Decrypted text", decrypted_text, sizeof(decrypted_text));

	/* Check the validity of the decryption */
	if (memcmp(decrypted_text, m_plain_text, NRF_CRYPTO_EXAMPLE_KMU_USAGE_KEY_MAX_TEXT_SIZE) !=
	    0) {
		LOG_INF("Error: Decrypted text doesn't match the plaintext");
		return status;
	}

	LOG_INF("Decryption successful!");

	return status;
}

/* ECC key pair operations */

/* EdDSA */

static int key_operations_import_eddsa_pub_key(const uint8_t *p_key, size_t key_size,
					       psa_key_id_t *key_id)
{
	assert(p_key != NULL);
	assert(key_id != NULL);

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_PURE_EDDSA);
	psa_set_key_type(&key_attributes,
			 PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
	psa_set_key_bits(&key_attributes, 255);

	status = psa_import_key(&key_attributes, p_key, key_size, key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed! (Error: %d)", status);
		return status;
	}

	/* Reset key attributes and free any allocated resources. */
	psa_reset_key_attributes(&key_attributes);

	return status;
}

int key_operations_use_eddsa_key_pair(psa_key_id_t *key_pair_id)
{
	psa_status_t status;
	psa_key_id_t pub_key_id;

	uint8_t pub_key[KEY_OPERATIONS_EDDSA_PUBLIC_KEY_SIZE];
	size_t pub_key_len;
	uint8_t signature[KEY_OPERATIONS_EDDSA_SIGNATURE_SIZE];
	size_t signature_len;

	if (key_pair_id == NULL) {
		LOG_ERR("Provide a valid key ID to generate the key");
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	LOG_INF("Using EdDSA key pair...");

	/* Export the public key */
	status = psa_export_public_key(*key_pair_id, pub_key, sizeof(pub_key), &pub_key_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_export_public_key failed! (Error: %d)", status);
		return status;
	}

	PRINT_HEX("Public-key", pub_key, pub_key_len);

	status = key_operations_import_eddsa_pub_key(pub_key, pub_key_len, &pub_key_id);
	memset(pub_key, 0, sizeof(pub_key));
	if (status != PSA_SUCCESS) {
		LOG_INF("key_operations_import_eddsa_pub_key failed! (Error: %d)", status);
		return status;
	}

	LOG_INF("Signing a message using EDDSA...");

	/* Sign the message */
	status = psa_sign_message(*key_pair_id, PSA_ALG_PURE_EDDSA, m_plain_text,
				  sizeof(m_plain_text), signature, sizeof(signature),
				  &signature_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_sign_message failed! (Error: %d)", status);
		goto exit;
	}

	LOG_INF("Message signed successfully!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("Signature", signature, sizeof(signature));

	LOG_INF("Verifying EDDSA signature...");

	/* Verify the signature of the message */
	status = psa_verify_message(pub_key_id, PSA_ALG_PURE_EDDSA, m_plain_text,
				    sizeof(m_plain_text), signature, signature_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_verify_message failed! (Error: %d)", status);
		goto exit;
	}

	LOG_INF("EdDSA signature verification was successful!");

exit:
	psa_destroy_key(pub_key_id);
	return status;
}

/* ECDSA */

static int key_operations_import_ecdsa_pub_key(const uint8_t *p_key, size_t key_size,
					       psa_key_id_t *key_id)
{
	assert(p_key != NULL);
	assert(key_id != NULL);

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	status = psa_import_key(&key_attributes, p_key, key_size, key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed! (Error: %d)", status);
		return status;
	}

	/* Reset key attributes and free any allocated resources. */
	psa_reset_key_attributes(&key_attributes);

	return status;
}

int key_operations_generate_ecdsa_key_pair(psa_key_id_t *key_pair_id)
{
	psa_status_t status;
	psa_key_id_t tmp_key_id;

	if (key_pair_id == NULL) {
		LOG_ERR("Provide a valid key ID to generate the key");
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	tmp_key_id = *key_pair_id;

	LOG_INF("Generating random ECDSA keypair...");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	/* Persistent key specific settings */
	psa_set_key_lifetime(&key_attributes,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_PERSISTENCE_DEFAULT, PSA_KEY_LOCATION_CRACEN_KMU));
	psa_set_key_id(&key_attributes, *key_pair_id);

	/* Generate a random keypair. The keypair is not exposed to the application,
	 * we can use it to sign hashes.
	 */
	status = psa_generate_key(&key_attributes, key_pair_id);

	if (status == PSA_ERROR_ALREADY_EXISTS) {
		/* Using key_pair_id without any modifications. */
		*key_pair_id = tmp_key_id;
		psa_reset_key_attributes(&key_attributes);
		return status;
	} else if (status != PSA_SUCCESS) {
		LOG_INF("psa_generate_key failed! (Error: %d)", status);
		return status;
	}

	/* Make sure the key metadata is not in memory anymore,
	 * has the same affect as resetting the device
	 */
	status = psa_purge_key(*key_pair_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_purge_key failed! (Error: %d)", status);
		return status;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	LOG_INF("ECDSA key pair generated successfully!");

	return status;
}

int key_operations_use_ecdsa_key_pair(psa_key_id_t *key_pair_id)
{
	psa_status_t status;
	psa_key_id_t pub_key_id;
	uint32_t output_len;

	uint8_t pub_key[KEY_OPERATIONS_ECDSA_PUBLIC_KEY_SIZE];
	size_t pub_key_len;
	uint8_t signature[KEY_OPERATIONS_ECDSA_SIGNATURE_SIZE];
	uint8_t hash[KEY_OPERATIONS_ECDSA_HASH_SIZE];

	if (key_pair_id == NULL) {
		LOG_ERR("Provide a valid key ID to generate the key");
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	LOG_INF("Using ECDSA key pair...");

	/* Export the public key */
	status = psa_export_public_key(*key_pair_id, pub_key, sizeof(pub_key), &pub_key_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_export_public_key failed! (Error: %d)", status);
		return status;
	}

	PRINT_HEX("Public-key", pub_key, pub_key_len);

	status = key_operations_import_ecdsa_pub_key(pub_key, pub_key_len, &pub_key_id);
	memset(pub_key, 0, sizeof(pub_key));
	if (status != PSA_SUCCESS) {
		LOG_INF("key_operations_import_ecdsa_pub_key failed! (Error: %d)", status);
		return status;
	}

	LOG_INF("Signing a message using ECDSA...");

	/* Compute the SHA256 hash*/
	status = psa_hash_compute(PSA_ALG_SHA_256, m_plain_text, sizeof(m_plain_text), hash,
				  sizeof(hash), &output_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_hash_compute failed! (Error: %d)", status);
		goto exit;
	}

	/* Sign the hash */
	status = psa_sign_hash(*key_pair_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), hash, sizeof(hash),
			       signature, sizeof(signature), &output_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_sign_hash failed! (Error: %d)", status);
		goto exit;
	}

	LOG_INF("Message signed successfully!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("SHA256 hash", hash, sizeof(hash));
	PRINT_HEX("Signature", signature, sizeof(signature));

	LOG_INF("Verifying ECDSA signature...");

	/* Verify the signature of the hash */
	status = psa_verify_hash(pub_key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), hash, sizeof(hash),
				 signature, sizeof(signature));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_verify_hash failed! (Error: %d)", status);
		goto exit;
	}

	LOG_INF("ECDSA signature verification was successful!");

exit:
	psa_destroy_key(pub_key_id);
	return status;
}
