/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <psa/crypto.h>
#include <string.h>

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

/* ML-KEM keys and ciphertexts are long, so printing is limited to just first bytes of these. */
#define PRINT_HEX_BYTE_LIMIT	(16u)

#define PRINT_HEX(p_label, p_text, len)\
	({\
		size_t print_len = (size_t)(len);\
		size_t hex_bytes_to_print = MIN(print_len, (size_t)PRINT_HEX_BYTE_LIMIT); \
		LOG_INF("---- %s (total len: %zu, printing %zu bytes): ----",             \
			p_label, print_len, hex_bytes_to_print);                          \
		LOG_HEXDUMP_INF(p_text, hex_bytes_to_print, "Content:");                  \
		LOG_INF("---- %s end  ----", p_label);                                    \
	})

LOG_MODULE_REGISTER(ml_kem, LOG_LEVEL_DBG);

/* Parameter sets are defined in section 8, FIPS 203. */
#define SAMPLE_ML_KEM_768_KEY_BITS		768u
#define SAMPLE_ML_KEM_768_SEED_SIZE		64u
#define SAMPLE_ML_KEM_768_PUB_KEY_SIZE		1184u
#define SAMPLE_ML_KEM_768_CIPHERTEXT_SIZE	1088u
#define SAMPLE_ML_KEM_SHARED_SECRET_SIZE 	32u

static const uint8_t ml_kem_seed[SAMPLE_ML_KEM_768_SEED_SIZE] = {
	0x6D, 0xBB, 0xC4, 0x37, 0x51, 0x36, 0xDF, 0x3B,
	0x07, 0xF7, 0xC7, 0x0E, 0x63, 0x9E, 0x22, 0x3E,
	0x17, 0x7E, 0x7F, 0xD5, 0x3B, 0x16, 0x1B, 0x3F,
	0x4D, 0x57, 0x79, 0x17, 0x94, 0xF1, 0x26, 0x24,
	0xF6, 0x96, 0x48, 0x40, 0x48, 0xEC, 0x21, 0xF9,
	0x6C, 0xF5, 0x0A, 0x56, 0xD0, 0x75, 0x9C, 0x44,
	0x8F, 0x37, 0x79, 0x75, 0x2F, 0x03, 0x83, 0xD3,
	0x74, 0x49, 0x69, 0x06, 0x94, 0xCF, 0x7A, 0x68
};

static psa_key_id_t key_pair_id;
static psa_key_id_t public_key_id;
static psa_key_id_t encapsulated_secret_id;
static psa_key_id_t decapsulated_secret_id;
static uint8_t ciphertext[SAMPLE_ML_KEM_768_CIPHERTEXT_SIZE];
static size_t ciphertext_length;

static int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int crypto_finish(void)
{
	psa_key_id_t *key_ids[] = {
		&decapsulated_secret_id,
		&encapsulated_secret_id,
		&public_key_id,
		&key_pair_id,
	};

	for (size_t i = 0; i < ARRAY_SIZE(key_ids); i++) {
		if (*key_ids[i] != PSA_KEY_ID_NULL) {
			psa_status_t status = psa_destroy_key(*key_ids[i]);

			if (status != PSA_SUCCESS) {
				LOG_ERR("psa_destroy_key failed! (Error: %d)", status);
				return APP_ERROR;
			}

			*key_ids[i] = PSA_KEY_ID_NULL;
		}
	}

	return APP_SUCCESS;
}

static int import_ml_kem_key_pair(void)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	LOG_INF("Importing an ML-KEM-768 key pair...");

	psa_set_key_usage_flags(&key_attributes,
				PSA_KEY_USAGE_ENCRYPT |
				PSA_KEY_USAGE_DECRYPT |
				PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ML_KEM);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ML_KEM_KEY_PAIR);
	psa_set_key_bits(&key_attributes, SAMPLE_ML_KEM_768_KEY_BITS);

	status = psa_import_key(&key_attributes, ml_kem_seed, sizeof(ml_kem_seed), &key_pair_id);
	psa_reset_key_attributes(&key_attributes);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_import_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("ML-KEM-768 key pair imported successfully!");
	PRINT_HEX("ML-KEM-768 key pair seed", ml_kem_seed, sizeof(ml_kem_seed));

	return APP_SUCCESS;
}

static int get_ml_kem_public_key(void)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t public_key[SAMPLE_ML_KEM_768_PUB_KEY_SIZE];
	size_t public_key_length;
	psa_status_t status;

	status = psa_export_public_key(key_pair_id, public_key, sizeof(public_key),
				       &public_key_length);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_export_public_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ML_KEM);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ML_KEM_PUBLIC_KEY);
	psa_set_key_bits(&key_attributes, SAMPLE_ML_KEM_768_KEY_BITS);

	status = psa_import_key(&key_attributes, public_key, public_key_length, &public_key_id);
	psa_reset_key_attributes(&key_attributes);
	if (status != PSA_SUCCESS) {
		LOG_ERR("public key import failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("ML-KEM-768 public key extracted successfully!");
	PRINT_HEX("ML-KEM-768 public key", public_key, public_key_length);

	return APP_SUCCESS;
}

static int encapsulate_key(void)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CCM);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, PSA_BYTES_TO_BITS(SAMPLE_ML_KEM_SHARED_SECRET_SIZE));

	status = psa_encapsulate(public_key_id, PSA_ALG_ML_KEM, &key_attributes,
				 &encapsulated_secret_id, ciphertext, sizeof(ciphertext),
				 &ciphertext_length);
	psa_reset_key_attributes(&key_attributes);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_encapsulate failed! (Error: %d)", status);
		return APP_ERROR;
	}

	PRINT_HEX("Ciphertext", ciphertext, ciphertext_length);
	LOG_INF("ML-KEM encapsulation was successful!");

	return APP_SUCCESS;
}

static int decapsulate_key(void)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CCM);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, PSA_BYTES_TO_BITS(SAMPLE_ML_KEM_SHARED_SECRET_SIZE));

	status = psa_decapsulate(key_pair_id, PSA_ALG_ML_KEM, ciphertext, ciphertext_length,
				 &key_attributes, &decapsulated_secret_id);
	psa_reset_key_attributes(&key_attributes);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_decapsulate failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int compare_shared_secrets(void)
{
	uint8_t encapsulated_secret[SAMPLE_ML_KEM_SHARED_SECRET_SIZE];
	uint8_t decapsulated_secret[SAMPLE_ML_KEM_SHARED_SECRET_SIZE];
	size_t encapsulated_secret_length;
	size_t decapsulated_secret_length;
	psa_status_t status;

	status = psa_export_key(encapsulated_secret_id, encapsulated_secret,
				sizeof(encapsulated_secret), &encapsulated_secret_length);
	if (status != PSA_SUCCESS) {
		LOG_ERR("encapsulated secret export failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_export_key(decapsulated_secret_id, decapsulated_secret,
				sizeof(decapsulated_secret), &decapsulated_secret_length);
	if (status != PSA_SUCCESS) {
		LOG_ERR("decapsulated secret export failed! (Error: %d)", status);
		return APP_ERROR;
	}

	if (encapsulated_secret_length != decapsulated_secret_length) {
		LOG_ERR("sizes of shared secrets differ");
		return APP_ERROR;
	}

	if (memcmp(encapsulated_secret, decapsulated_secret, encapsulated_secret_length) != 0) {
		LOG_ERR("shared secrets do not match");
		return APP_ERROR;
	}

	PRINT_HEX("Shared secret", encapsulated_secret, encapsulated_secret_length);
	LOG_INF("Shared secrets match!");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting ML-KEM example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = import_ml_kem_key_pair();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = get_ml_kem_public_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		(void)crypto_finish();
		return APP_ERROR;
	}

	status = encapsulate_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		(void)crypto_finish();
		return APP_ERROR;
	}

	status = decapsulate_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		(void)crypto_finish();
		return APP_ERROR;
	}

	status = compare_shared_secrets();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		(void)crypto_finish();
		return APP_ERROR;
	}

	status = crypto_finish();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	LOG_INF(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
