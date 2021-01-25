/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <crypto_common.h>

LOG_MODULE_REGISTER(rng, LOG_LEVEL_DBG);

/* ========== Global variables/defines for the RNG example ========== */

#define NRF_CRYPTO_EXAMPLE_RNG_DATA_SIZE (100)
#define NRF_CRYPTO_EXAMPLE_RNG_ITERATIONS (5)

static uint8_t m_rng_data[NRF_CRYPTO_EXAMPLE_RNG_DATA_SIZE];

/* ========== End of global variables/defines for the RNG example ========== */

int produce_rng_data(void)
{
	psa_status_t status;

	PRINT_MESSAGE("Producing 5 random numbers...");

	for (int i = 0; i < NRF_CRYPTO_EXAMPLE_RNG_ITERATIONS; i++) {
		/* Generate the random number */
		status = psa_generate_random(m_rng_data, sizeof(m_rng_data));
		if (status != PSA_SUCCESS) {
			PRINT_ERROR("psa_generate_random", status);
			return APP_ERROR;
		}

		PRINT_HEX("RNG data", m_rng_data, sizeof(m_rng_data));
	}

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	PRINT_MESSAGE("Starting RNG example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = produce_rng_data();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	PRINT_MESSAGE(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
