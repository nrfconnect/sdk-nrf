/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <psa/crypto.h>

#include "ml_dsa_65_vectors.h"

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

/* ML-DSA signatures and keys are long, so printing is limited to just first bytes of these */
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

LOG_MODULE_REGISTER(ml_dsa, LOG_LEVEL_DBG);

/* ====================================================================== */
/*		Global variables/defines for the ML-DSA example		  */

static psa_key_id_t pub_key_id;
/* ====================================================================== */

int crypto_init(void)
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

int crypto_finish(void)
{
	psa_status_t status;

	/* Destroy the key handle */
	status = psa_destroy_key(pub_key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int import_ml_dsa_pub_key(void)
{
	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	LOG_INF("Importing an ML-DSA-65 public key...");

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ML_DSA);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ML_DSA_PUBLIC_KEY);

	status = psa_import_key(&key_attributes, ml_dsa_65_pub_key, sizeof(ml_dsa_65_pub_key),
				&pub_key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_import_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("ML-DSA-65 public key imported successfully!");
	PRINT_HEX("ML-DSA-65 public key", ml_dsa_65_pub_key, sizeof(ml_dsa_65_pub_key));

	/* Reset key attributes and free any allocated resources. */
	psa_reset_key_attributes(&key_attributes);

	return APP_SUCCESS;
}

int verify_message(void)
{
	psa_status_t status;

	LOG_INF("Verifying the ML-DSA signature...");

	/* Verify the signature of the message */
	status = psa_verify_message(pub_key_id, PSA_ALG_ML_DSA, ml_dsa_65_message,
				    sizeof(ml_dsa_65_message), ml_dsa_65_signature,
				    sizeof(ml_dsa_65_signature));
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_verify_message failed! (Error: %d)", status);
		return APP_ERROR;
	}

	PRINT_HEX("Message", ml_dsa_65_message, sizeof(ml_dsa_65_message));
	PRINT_HEX("Signature", ml_dsa_65_signature, sizeof(ml_dsa_65_signature));
	LOG_INF("Signature verification was successful!");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting ML-DSA example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = import_ml_dsa_pub_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = verify_message();
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
