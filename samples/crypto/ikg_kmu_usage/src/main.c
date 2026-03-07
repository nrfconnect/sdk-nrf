/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <zephyr/logging/log.h>
#include <cracen_psa_key_ids.h>
#include <cracen/mem_helpers.h>
#include <cracen_psa_kmu.h>
#include <cracen_psa_key_management.h>
#include <cracen/lib_kmu.h>

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#endif

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

#define PRINT_HEX(p_label, key_id, p_text, len)\
	({\
		LOG_INF("---- %s (key_id: 0x%08x, len: %u): ----", p_label, key_id, len);\
		LOG_HEXDUMP_INF(p_text, len, "Content:");\
		LOG_INF("---- %s end  ----", p_label);\
	})

LOG_MODULE_REGISTER(ikg_kmu_usage, LOG_LEVEL_DBG);

static psa_key_id_t identity_key_id = CRACEN_BUILTIN_IDENTITY_KEY_ID;
static psa_key_id_t mkek_key_id = CRACEN_BUILTIN_MKEK_ID;
static psa_key_id_t mext_key_id = CRACEN_BUILTIN_MEXT_ID;

#define NRF_CRYPTO_EXAMPLE_AES_GCM_TAG_LENGTH (16)
#define NRF_CRYPTO_EXAMPLE_AES_IV_SIZE (12)
#define NRF_CRYPTO_EXAMPLE_MAX_TEXT_SIZE (100)
#define NRF_CRYPTO_EXAMPLE_MAX_ENC_SIZE (NRF_CRYPTO_EXAMPLE_MAX_TEXT_SIZE + NRF_CRYPTO_EXAMPLE_AES_GCM_TAG_LENGTH)

static uint8_t m_plain_text[] = {
	"Example string to demonstrate basic usage of IKG and protected KMU keys."
};

/* Function to initialize a volatile encryption key attributes structure  */
static void init_key_attributes(psa_key_attributes_t *attributes,
				psa_key_usage_t usage,
				psa_key_lifetime_t lifetime,
				psa_algorithm_t alg)
{
	psa_set_key_usage_flags(attributes, usage);
	psa_set_key_lifetime(attributes, lifetime);
	psa_set_key_algorithm(attributes, alg);
	psa_set_key_type(attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(attributes, 256);
}


static void init_volatile_gcm_attributes(psa_key_attributes_t *attributes) {
	init_key_attributes(attributes,
			    PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT,
			    PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				PSA_KEY_PERSISTENCE_VOLATILE, PSA_KEY_LOCATION_LOCAL_STORAGE),
			    PSA_ALG_GCM);
}


int crypto_init(void)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to init crypto");
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

/** @brief Function making use of IKG identity key
 *
 * The identity key is a private key using secp256r1 that is not
 * available to the CPU. It can be used to sign, and it is possible
 * to export
 */
static int use_ikg_identity_key(void) {
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t signature[64] = {0};
	size_t signature_len;

	/* Check that identity key exists (e.g. by populating key attributes)*/
	status = psa_get_key_attributes(identity_key_id, &attr);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to resolve the IKG key (Error: %d)", status);
		return APP_ERROR;
	}
	(void)attr;

	LOG_INF("Signing message with identity key");

	/* Sign the example text */
	status = psa_sign_message(identity_key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
				  m_plain_text, sizeof(m_plain_text),
				  signature, sizeof(signature), &signature_len);

	if (status != PSA_SUCCESS) {
		LOG_ERR("Signing failed using IKG (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Verifying message with identity key");

	status = psa_verify_message(identity_key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
				    m_plain_text, sizeof(m_plain_text),
				    signature, sizeof(signature));
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to verify message (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

/** @brief Function derive key using PSA_ALG_SP800_108_COUNTER_CMAC
 */
static int derive_key(psa_key_id_t key_id, psa_key_attributes_t *derive_attr, psa_key_id_t* key_id_out)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t derived_key[PSA_BITS_TO_BYTES(256)] = {0};
	uint8_t label[] = "Some label";
	uint8_t context[] = "Some context (salt), optional";

	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	status = psa_key_derivation_setup(&op, PSA_ALG_SP800_108_COUNTER_CMAC);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed key derivation: Failed setup (Error: %d)", status);
		return status;
	}

	status = psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed key derivation: Input key (Error: %d)", status);
		return status;
	}

	status = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_LABEL, label,
						sizeof(label));
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed key derivation: Label (Error: %d)", status);
		return status;
	}

	/* Context is optional to use, so it is set to NULL with zero length */
	status = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_CONTEXT, context,
						sizeof(context));
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed key derivation: Context (Error: %d)", status);
		return status;
	}

	status = psa_key_derivation_output_bytes(&op, derived_key, sizeof(derived_key));
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to derive key. (Error: %d)", status);
		return status;
	}

	status = psa_import_key(derive_attr, derived_key, sizeof(derived_key), &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import derived key. (Error: %d)", status);
		return status;
	}

	safe_memzero(derived_key, sizeof(derived_key));

	*key_id_out = key_id;
	return APP_SUCCESS;
}

/** @brief Function to encrypt and decrypt the example text with AES GCM
 */
static int encrypt_decrypt(psa_key_id_t key_id)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t iv[NRF_CRYPTO_EXAMPLE_AES_IV_SIZE] = {0};
	uint8_t encrypted[NRF_CRYPTO_EXAMPLE_MAX_ENC_SIZE] = {0};
	uint8_t decrypted[NRF_CRYPTO_EXAMPLE_MAX_TEXT_SIZE] = {0};
	size_t encrypted_len = 0;
	size_t output_len = 0;

	/* create IV */
	status = psa_generate_random(iv, NRF_CRYPTO_EXAMPLE_AES_IV_SIZE);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_generate_random failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Encrypt the plaintext and create the authentication tag */
	status = psa_aead_encrypt(key_id,
				  PSA_ALG_GCM,
				  iv,
				  sizeof(iv),
				  NULL, /* AAD, skipped here */
				  0, /*AAD size, skipped here */
				  m_plain_text,
				  sizeof(m_plain_text),
				  encrypted,
				  sizeof(encrypted),
				  &encrypted_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_aead_encrypt failed! (Error: %d)", status);
		return APP_ERROR;
	}

	PRINT_HEX("Encrypted text", key_id, encrypted, encrypted_len);

	status = psa_aead_decrypt(key_id,
				  PSA_ALG_GCM,
				  iv,
				  sizeof(iv),
				  NULL, /* AAD, skipped here */
				  0, /*AAD size, skipped here */
				  encrypted,
				  encrypted_len,
				  decrypted,
				  sizeof(decrypted),
				  &output_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_aead_decrypt failed! (Error: %d)", status);
		return APP_ERROR;
	}

	PRINT_HEX("Decrypted text", key_id, decrypted, sizeof(decrypted));
	return APP_SUCCESS;
}

/** @brief Function to derive a key from MKEK and use it for encryption
 */
int use_mkek_key(void) {
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	init_volatile_gcm_attributes(&attributes);

	LOG_INF("Deriving key from MKEK");

	status = derive_key(mkek_key_id, &attributes, &key_id);
	if (status != APP_SUCCESS) {
		LOG_ERR("Failed to derive using MKEK key");
		return APP_ERROR;
	}

	LOG_INF("New volatile key was derived from MKEK (key_id: 0x%08x).", key_id);

	status = encrypt_decrypt(key_id);
	if (status != APP_SUCCESS) {
		LOG_ERR("Failed to encrypt with MKEK derived key");
		return APP_ERROR;
	}

	LOG_INF("Destroying volatile key derived from MKEK (key_id: 0x%08x).", key_id);

	status = psa_destroy_key(key_id);
	if (status != APP_SUCCESS) {
		LOG_ERR("Failed to destroy MKEK derived key");
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

/** @brief Function to derive a key from MEXT and use it for encryption
 */
int use_mext_key(void) {
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	init_volatile_gcm_attributes(&attributes);

	LOG_INF("Deriving key from MEXT");

	status = derive_key(mext_key_id, &attributes, &key_id);
	if (status != APP_SUCCESS) {
		LOG_ERR("Failed to derive using MEXT key");
	}

	LOG_INF("New volatile key was derived from MKEK (key_id: 0x%08x).", key_id);

	status = encrypt_decrypt(key_id);
	if (status != APP_SUCCESS) {
		LOG_ERR("Failed to encrypt with MEXT derived key");
		return APP_ERROR;
	}

	LOG_INF("Destroying volatile key derived from MEXT (key_id: 0x%08x).", key_id);

	status = psa_destroy_key(key_id);
	if (status != APP_SUCCESS) {
		LOG_ERR("Failed to destroy MEXT derived key");
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

#define KMU_KEY_ID_DERIVATION \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED, 4)

#define KMU_KEY_ID_DIRECT_USAGE \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED, 6)

/** @brief Function to generate a KMU key
*/
int generate_kmu_key(void) {
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	LOG_INF("Destroying KMU key (if present)");

	status = psa_destroy_key(KMU_KEY_ID_DIRECT_USAGE);
	if (status != PSA_SUCCESS && status != PSA_ERROR_INVALID_HANDLE) {
		LOG_ERR("Failed to destroy direct usage key in KMU: %d", status);
		return APP_ERROR;
	}

	LOG_INF("Creating KMU key");

	/* Create a key for direct usage. This key is CPU inaccessible when stored */
	init_key_attributes(&attributes,
			    PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT,
			    PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			    PSA_KEY_PERSISTENCE_DEFAULT, PSA_KEY_LOCATION_CRACEN_KMU),
			    PSA_ALG_GCM);
	psa_set_key_id(&attributes, KMU_KEY_ID_DIRECT_USAGE);

	status = psa_generate_key(&attributes, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to create key for direct usage in KMU: %d", status);
		return APP_ERROR;
	}

	LOG_INF("KMU key created (key_id: 0x%08x).", key_id);

	safe_memzero(&attributes, sizeof(attributes));

	return APP_SUCCESS;
}

/** @brief Function to directly use a KMU key
 */
int use_kmu_key(void)
{
	int status;

	LOG_INF("Using KMU key directly (key_id: 0x%08x).", KMU_KEY_ID_DIRECT_USAGE);

	status = encrypt_decrypt(KMU_KEY_ID_DIRECT_USAGE);
	if (status != PSA_SUCCESS) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting IKG/KMU example...");

	LOG_INF("crypto init");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = use_ikg_identity_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = use_mkek_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = use_mext_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = generate_kmu_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = use_kmu_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	LOG_INF(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
