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
#include <crypto_common.h>

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

int hash_sha256(void)
{
	uint32_t olen;
	psa_status_t status;

	PRINT_MESSAGE("Hashing using SHA256...");

	/* Calculate the SHA256 hash */
	status = psa_hash_compute(
		PSA_ALG_SHA_256, m_plain_text, sizeof(m_plain_text), m_hash, sizeof(m_hash), &olen);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_hash_compute", status);
		return APP_ERROR;
	}

	PRINT_MESSAGE("Hashing successful!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("SHA256 hash", m_hash, sizeof(m_hash));

	return APP_SUCCESS;
}

int verify_sha256(void)
{
	psa_status_t status;

	PRINT_MESSAGE("Verifying the SHA256 hash...");

	/* Verify the hash */
	status = psa_hash_compare(
		PSA_ALG_SHA_256, m_plain_text, sizeof(m_plain_text), m_hash, sizeof(m_hash));
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_hash_compare", status);
		return APP_ERROR;
	}

	PRINT_MESSAGE("SHA256 verification successful!");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	PRINT_MESSAGE("Starting SHA256 example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = hash_sha256();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = verify_sha256();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	PRINT_MESSAGE(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
