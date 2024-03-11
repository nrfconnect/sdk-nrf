/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <psa/crypto.h>
#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_crypto_defs.h>
#else /* CONFIG_BUILD_WITH_TFM */
#include <hw_unique_key.h>
#ifdef CONFIG_HW_CC3XX
#include <nrf_cc3xx_platform.h>
#endif
#endif /* CONFIG_BUILD_WITH_TFM */
#if !defined(HUK_HAS_KMU)
#include <zephyr/sys/reboot.h>
#endif /* !defined(HUK_HAS_KMU) */

#include "derive_key.h"

#define IV_LEN 12
#define MAC_LEN 16

#ifdef HUK_HAS_CC310
#define ENCRYPT_ALG PSA_ALG_CCM
#else
#define ENCRYPT_ALG PSA_ALG_GCM
#endif

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

#define PRINT_HEX(p_label, p_text, len)\
	({\
		LOG_INF("---- %s (len: %u): ----", p_label, len);\
		LOG_HEXDUMP_INF(p_text, len, "Content:");\
		LOG_INF("---- %s end  ----", p_label);\
	})

LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

static psa_key_id_t key_id;

int crypto_init(void)
{
	psa_status_t status;

#if !defined(CONFIG_BUILD_WITH_TFM)
	int result;

#if defined(HUK_HAS_CC310) || defined(HUK_HAS_CC312)
	result = nrf_cc3xx_platform_init();

	if (result != NRF_CC3XX_PLATFORM_SUCCESS) {
		LOG_INF("nrf_cc3xx_platform_init returned error: %d", result);
		return APP_ERROR;
	}
#endif

	if (!hw_unique_key_are_any_written()) {
		LOG_INF("Writing random keys to KMU");
		result = hw_unique_key_write_random();
		if (result != HW_UNIQUE_KEY_SUCCESS) {
			LOG_INF("hw_unique_key_write_random returned error: %d", result);
			return APP_ERROR;
		}
		LOG_INF("Success!");

#if !defined(HUK_HAS_KMU)
		/* Reboot to allow the bootloader to load the key into CryptoCell. */
		sys_reboot(0);
#endif /* !defined(HUK_HAS_KMU) */
	}
#endif /* !defined(CONFIG_BUILD_WITH_TFM) */

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_crypto_init returned error: %d", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int crypto_finish(void)
{
	psa_status_t status;

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key returned error: %d", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int generate_key(void)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t key_label[] = "HUK derivation sample label";
	psa_status_t status;

	LOG_INF("Deriving key");

	/* Set the key attributes for the storage key */
	psa_set_key_usage_flags(&attributes,
			(PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT));
	psa_set_key_algorithm(&attributes, ENCRYPT_ALG);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 128);

	status = derive_key(&attributes, key_label, sizeof(key_label) - 1, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("derive_key returned error: %d", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int encrypt_message(void)
{
	psa_status_t status;
	uint8_t plaintext[] = "Lorem ipsum dolor sit amet. This will be encrypted.";
	uint8_t ciphertext[sizeof(plaintext) + MAC_LEN];
	uint32_t ciphertext_out_len;
	uint8_t iv[IV_LEN];
	uint8_t additional_data[] = "This will be authenticated but not encrypted.";

	LOG_INF("Generating random IV");

	status = psa_generate_random(iv, IV_LEN);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_generate_random returned error: %d", status);
		return APP_ERROR;
	}

	PRINT_HEX("IV", iv, IV_LEN);

	LOG_INF("Key ID: 0x%x", key_id);
	LOG_INF("Encrypting");
	PRINT_HEX("Plaintext", plaintext, sizeof(plaintext) - 1);

	status = psa_aead_encrypt(key_id, ENCRYPT_ALG, iv, IV_LEN,
				additional_data, sizeof(additional_data) - 1,
				plaintext, sizeof(plaintext) - 1,
				ciphertext, sizeof(ciphertext), &ciphertext_out_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_aead_encrypt returned error: %d", status);
		return APP_ERROR;
	}
	if (ciphertext_out_len != (sizeof(plaintext) - 1 + MAC_LEN)) {
		LOG_INF("ciphertext has wrong length: %d", ciphertext_out_len);
		return APP_ERROR;
	}

	PRINT_HEX("Ciphertext (with authentication tag)", ciphertext, ciphertext_out_len);

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Derive a key, then use it to encrypt a message.");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = generate_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = encrypt_message();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
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
