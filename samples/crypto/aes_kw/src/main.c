/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE	"Example finished successfully!"
#define APP_ERROR_MESSAGE	"Example exited with error!"

#if defined(CONFIG_SAMPLE_AES_KW_SHOW_KWP)
#define APP_ALGORITHM_USED	"AES-KWP"
#else
#define APP_ALGORITHM_USED	"AES-KW"
#endif /* CONFIG_SAMPLE_AES_KW_SHOW_KWP */

#define PRINT_HEX(p_label, p_text, len)\
	({\
		LOG_INF("---- %s (len: %u): ----", p_label, len);\
		LOG_HEXDUMP_INF(p_text, len, "Content:");\
		LOG_INF("---- %s end  ----", p_label);\
	})

#define SAMPLE_KEY_BUFFER_SIZE	(40u)

LOG_MODULE_REGISTER(aes_kw, LOG_LEVEL_DBG);

/* ====================================================================== */
/* Global variables/defines for the AES-KW example */

static uint8_t key_buf[SAMPLE_KEY_BUFFER_SIZE];
static size_t key_len;

#if defined(CONFIG_SAMPLE_AES_KW_KMU_DEMO)
static psa_key_id_t enc_key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
				 CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED, PSA_KEY_ID_USER_MIN);
#else
static psa_key_id_t enc_key_id;
#endif /* CONFIG_SAMPLE_AES_KW_KMU_DEMO */

static psa_key_id_t key_id;

#if !defined(CONFIG_SAMPLE_AES_KW_SHOW_KWP)

/* AES-KW test vector from RFC3394 */
static const uint8_t enc_key[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static const uint8_t key[] = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

static const uint8_t ct[] = {
	0x1F, 0xA6, 0x8B, 0x0A, 0x81, 0x12, 0xB4, 0x47,
	0xAE, 0xF3, 0x4B, 0xD8, 0xFB, 0x5A, 0x7B, 0x82,
	0x9D, 0x3E, 0x86, 0x23, 0x71, 0xD2, 0xCF, 0xE5
};

#else

/* AES-KWP test vector from RFC5649 */
static const uint8_t enc_key[] = {
	0x58, 0x40, 0xdf, 0x6e, 0x29, 0xb0, 0x2a, 0xf1,
	0xab, 0x49, 0x3b, 0x70, 0x5b, 0xf1, 0x6e, 0xa1,
	0xae, 0x83, 0x38, 0xf4, 0xdc, 0xc1, 0x76, 0xa8
};

static const uint8_t key[] = {
	0xc3, 0x7b, 0x7e, 0x64, 0x92, 0x58, 0x43, 0x40,
	0xbe, 0xd1, 0x22, 0x07, 0x80, 0x89, 0x41, 0x15,
	0x50, 0x68, 0xf7, 0x38
};

static const uint8_t ct[] = {
	0x13, 0x8b, 0xde, 0xaa, 0x9b, 0x8f, 0xa7, 0xfc,
	0x61, 0xf9, 0x77, 0x42, 0xe7, 0x22, 0x48, 0xee,
	0x5a, 0xe6, 0xae, 0x53, 0x60, 0xd1, 0xae, 0x6a,
	0x5f, 0x54, 0xf3, 0x73, 0xfa, 0x54, 0x3b, 0x6a
};
#endif /* CONFIG_SAMPLE_AES_KW_SHOW_KWP */
/* ====================================================================== */

static int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int crypto_finish(bool enc_key_blocked)
{
	psa_status_t status;

	/* Destroy the unwrapped key handle */
	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key for unwrapped key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/** Destroy the Key-Encryption Key handle.
	 *  This is only possible if the key has not been blocked.
	 */
	status = psa_destroy_key(enc_key_id);
	if ((!enc_key_blocked && status != PSA_SUCCESS) ||
	    (enc_key_blocked && status != PSA_ERROR_NOT_PERMITTED)) {
		LOG_INF("psa_destroy_key for Key-Encryption Key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static void preset_enc_key_attributes(psa_key_attributes_t *attributes)
{
	psa_set_key_usage_flags(attributes, PSA_KEY_USAGE_WRAP | PSA_KEY_USAGE_UNWRAP);
	psa_set_key_type(attributes, PSA_KEY_TYPE_AES);

	if (IS_ENABLED(CONFIG_SAMPLE_AES_KW_SHOW_KWP)) {
		psa_set_key_algorithm(attributes, PSA_ALG_KWP);
		psa_set_key_bits(attributes, 192);
	} else {
		psa_set_key_algorithm(attributes, PSA_ALG_KW);
		psa_set_key_bits(attributes, 128);
	}

	if (IS_ENABLED(CONFIG_SAMPLE_AES_KW_KMU_DEMO)) {
		psa_set_key_lifetime(attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
						 CRACEN_KEY_PERSISTENCE_REVOKABLE,
						 PSA_KEY_LOCATION_CRACEN_KMU));

		psa_set_key_id(attributes, enc_key_id);
	} else {
		psa_set_key_lifetime(attributes, PSA_KEY_LIFETIME_VOLATILE);
	}
}

static int import_enc_key(void)
{
	psa_status_t status;

	LOG_INF("Importing Key-Encryption Key...");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	preset_enc_key_attributes(&key_attributes);
	status = psa_import_key(&key_attributes, enc_key, sizeof(enc_key), &enc_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed (Key-Encryption Key)! (Error: %d)", status);
		return APP_ERROR;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	LOG_INF("Key-Encryption Key imported successfully!");

	return APP_SUCCESS;
}

static int generate_enc_key(void)
{
	psa_status_t status;

	LOG_INF("Generating Key-Encryption Key...");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	preset_enc_key_attributes(&key_attributes);

	status = psa_generate_key(&key_attributes, &enc_key_id);
	if (status != PSA_SUCCESS) {
		if (status == PSA_ERROR_ALREADY_EXISTS) {
			LOG_WRN("Key already exists in KMU. "
				"Please re-flash the sample with \"ERASEALL\" option.");
		}

		LOG_INF("psa_import_key failed (Key-Encryption Key)! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Make sure the key metadata is not in memory anymore,
	 * has the same affect as resetting the device
	 */
	status = psa_purge_key(enc_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_purge_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	LOG_INF("Key-Encryption Key generated successfully!");

	return APP_SUCCESS;
}

static int import_key(void)
{
	psa_status_t status;

	LOG_INF("Importing key (to wrap)...");
	PRINT_HEX("Source key", key, sizeof(key));

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_EXPORT);

	if (IS_ENABLED(CONFIG_SAMPLE_AES_KW_SHOW_KWP)) {
		psa_set_key_algorithm(&key_attributes, PSA_ALG_KWP);
		psa_set_key_type(&key_attributes, PSA_KEY_TYPE_RAW_DATA);
	} else {
		psa_set_key_algorithm(&key_attributes, PSA_ALG_KW);
		psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	}

	status = psa_import_key(&key_attributes, key, sizeof(key), &key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed (KEY)! (Error: %d)", status);
		return APP_ERROR;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	LOG_INF("KEY key imported successfully!");

	return APP_SUCCESS;
}

static int wrap_key(bool compare_expected_result)
{
	bool buffers_equal;
	bool buffer_len_equal;
	psa_status_t status;
	psa_algorithm_t wrap_alg;

	if (IS_ENABLED(CONFIG_SAMPLE_AES_KW_SHOW_KWP)) {
		wrap_alg = PSA_ALG_KWP;
	} else {
		wrap_alg = PSA_ALG_KW;
	}

	LOG_INF("Wrapping key...");

	status = psa_wrap_key(enc_key_id, wrap_alg, key_id,
			      key_buf, sizeof(key_buf), &key_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_wrap_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	PRINT_HEX("Wrapped key", key_buf, key_len);

	if (compare_expected_result) {
		buffer_len_equal = key_len == sizeof(ct);
		LOG_INF("Wrapped key size is %sexpected: %d bytes",
			buffer_len_equal ? "" : "NOT ", key_len);

		buffers_equal = memcmp(key_buf, ct, sizeof(ct)) == 0;
		LOG_INF("Wrapped key data is %scorrect!", buffers_equal ? "" : "NOT ");
	} else {
		LOG_INF("Wrapped key verification skipped.");
		buffer_len_equal = true;
		buffers_equal = true;
	}

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return (buffer_len_equal && buffers_equal) ? APP_SUCCESS : APP_ERROR;
}

static int unwrap_key(void)
{
	bool buffers_equal;
	bool buffer_len_equal;
	psa_status_t status;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t unwrap_alg;

	LOG_INF("Unwrapping key...");

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CTR);

	if (IS_ENABLED(CONFIG_SAMPLE_AES_KW_SHOW_KWP)) {
		unwrap_alg = PSA_ALG_KWP;
		psa_set_key_type(&key_attributes, PSA_KEY_TYPE_RAW_DATA);
	} else {
		unwrap_alg = PSA_ALG_KW;
		psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	}

	status = psa_unwrap_key(&key_attributes, enc_key_id, unwrap_alg,
			      key_buf, key_len, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_unwrap_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_export_key(key_id, key_buf, sizeof(key_buf), &key_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_export_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	PRINT_HEX("Unwrapped key", key_buf, key_len);
	buffer_len_equal = key_len == sizeof(key);
	LOG_INF("Unwrapped key size is %sexpected: %d bytes",
		buffer_len_equal ? "" : "NOT ", key_len);

	buffers_equal = memcmp(key_buf, key, sizeof(key)) == 0;
	LOG_INF("Unwrapped key data is %scorrect!", buffers_equal ? "" : "NOT ");

	return (buffer_len_equal && buffers_equal) ? APP_SUCCESS : APP_ERROR;
}

static int block_enc_key(void)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status = psa_get_key_attributes(enc_key_id, &attributes);

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_get_key_attributes failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* PSA API standard does not support key blocking yet */
	cracen_kmu_block(&attributes);
	if (status != PSA_SUCCESS) {
		LOG_ERR("cracen_kmu_block failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int main(void)
{
	int status;
	bool enc_key_blocked = false;

	LOG_INF("Starting AES-KW example...");
	LOG_INF("Using %s algorithm", APP_ALGORITHM_USED);

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_AES_KW_KMU_DEMO)) {
		status = generate_enc_key();
	} else {
		status = import_enc_key();
	}

	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = import_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = wrap_key(!IS_ENABLED(CONFIG_SAMPLE_AES_KW_KMU_DEMO));
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = unwrap_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_AES_KW_KMU_DEMO)) {
		status = block_enc_key();
		if (status != APP_SUCCESS) {
			LOG_INF(APP_ERROR_MESSAGE);
			return APP_ERROR;
		}

		enc_key_blocked = true;

		/** Key-encryption key usage is not possible after it is blocked (until the next
		 *  device reset), so the following operation must fail (as well as the attempt
		 *  to destroy the blocked key).
		 *
		 *  This would be a normal use-case for the bootloader, which blocks
		 *  Key-Encryption Key after being used before jumping to the application.
		 */
		status = wrap_key(!IS_ENABLED(CONFIG_SAMPLE_AES_KW_KMU_DEMO));
		if (status == APP_SUCCESS) {
			LOG_INF(APP_ERROR_MESSAGE);
			return APP_ERROR;
		}
	}

	status = crypto_finish(enc_key_blocked);
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	LOG_INF(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
