/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>

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

LOG_MODULE_REGISTER(rng, LOG_LEVEL_DBG);

/* ========== Global variables/defines for the RNG example ========== */

#define NRF_CRYPTO_EXAMPLE_RNG_DATA_SIZE (100)
#define NRF_CRYPTO_EXAMPLE_RNG_ITERATIONS (5)

static uint8_t m_rng_data[NRF_CRYPTO_EXAMPLE_RNG_DATA_SIZE];

/* ========== End of global variables/defines for the RNG example ========== */

int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS)
		return APP_ERROR;

	return APP_SUCCESS;
}

int produce_rng_data(void)
{
	psa_status_t status;

	LOG_INF("Producing 5 random numbers...");

	for (int i = 0; i < NRF_CRYPTO_EXAMPLE_RNG_ITERATIONS; i++) {
		/* Generate the random number */
		status = psa_generate_random(m_rng_data, sizeof(m_rng_data));
		if (status != PSA_SUCCESS) {
			LOG_INF("psa_generate_random failed! (Error: %d)", status);
			return APP_ERROR;
		}

		PRINT_HEX("RNG data", m_rng_data, sizeof(m_rng_data));
	}

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting RNG example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = produce_rng_data();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	LOG_INF(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
