/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <stdio.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <logging/log.h>

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
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

LOG_MODULE_REGISTER(sha256, LOG_LEVEL_DBG);

/* ====================================================================== */
/*				Global variables/defines for the SHA256 example			  */

#define NRF_CRYPTO_EXAMPLE_SHA256_TEXT_SIZE (100)
#define NRF_CRYPTO_EXAMPLE_SHA256_SIZE (32)

/* Below text is used as plaintext for computing/verifying the hash. */
static uint8_t m_plain_text[NRF_CRYPTO_EXAMPLE_SHA256_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of SHA256."
};

static uint8_t m_hash[NRF_CRYPTO_EXAMPLE_SHA256_SIZE];

/* ====================================================================== */

int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS)
		return APP_ERROR;

	return APP_SUCCESS;
}

int hash_sha256(void)
{
	uint32_t olen;
	psa_status_t status;

	LOG_INF("Hashing using SHA256...");

	/* Calculate the SHA256 hash */
	status = psa_hash_compute(
		PSA_ALG_SHA_256, m_plain_text, sizeof(m_plain_text), m_hash, sizeof(m_hash), &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_hash_compute failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Hashing successful!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("SHA256 hash", m_hash, sizeof(m_hash));

	return APP_SUCCESS;
}

int verify_sha256(void)
{
	psa_status_t status;

	LOG_INF("Verifying the SHA256 hash...");

	/* Verify the hash */
	status = psa_hash_compare(
		PSA_ALG_SHA_256, m_plain_text, sizeof(m_plain_text), m_hash, sizeof(m_hash));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_hash_compare failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("SHA256 verification successful!");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting SHA256 example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = hash_sha256();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = verify_sha256();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	LOG_INF(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
