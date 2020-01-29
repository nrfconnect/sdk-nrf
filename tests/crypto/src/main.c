/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <common_test.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(test_main);

static int init_leds(void)
{
	return dk_leds_init();
}

void run_suites(test_case_t *test_cases, u32_t test_case_count)
{
	for (u32_t i = 0; i < test_case_count; i++) {
		uint32_t n_cases = get_vector_count(&test_cases[i]);
		if (n_cases == 0) {
			continue;
		}

		struct unit_test test_suite[n_cases + 1];

		for (uint32_t v = 0; v < n_cases; v++) {
			test_suite[v].name = get_vector_name(&test_cases[i], v);
			test_suite[v].setup = test_cases[i].setup;
			test_suite[v].teardown = test_cases[i].teardown;
			test_suite[v].test = test_cases[i].exec;
			test_suite[v].thread_options = 0;
		}
		test_suite[n_cases].test = NULL;

		z_ztest_run_test_suite(test_cases[i].p_test_case_name,
				       test_suite);
	}
}

void test_main(void)
{
	LOG_INF("Crypto test app started.");

	if (init_leds() != 0) {
		LOG_ERR("Bad leds init!");
	}
	if (init_ctr_drbg(NULL, 0) != 0) {
		LOG_ERR("Bad ctr drbg init!");
	}

	run_suites(__start_test_case_aead_ccm_data,
		   ITEM_COUNT(test_case_aead_ccm_data, test_case_t));
	run_suites(__start_test_case_aead_ccm_simple_data,
		   ITEM_COUNT(test_case_aead_ccm_simple_data, test_case_t));
	run_suites(__start_test_case_aead_gcm_data,
		   ITEM_COUNT(test_case_aead_gcm_data, test_case_t));
	run_suites(__start_test_case_aead_gcm_simple_data,
		   ITEM_COUNT(test_case_aead_gcm_simple_data, test_case_t));
	run_suites(__start_test_case_aead_chachapoly_data,
		   ITEM_COUNT(test_case_aead_chachapoly_data, test_case_t));
	run_suites(__start_test_case_aead_chachapoly_simple_data,
		   ITEM_COUNT(test_case_aead_chachapoly_simple_data,
			      test_case_t));
	run_suites(__start_test_case_aes_ecb_mac_data,
		   ITEM_COUNT(test_case_aes_ecb_mac_data, test_case_t));
	run_suites(__start_test_case_aes_cbc_mac_data,
		   ITEM_COUNT(test_case_aes_cbc_mac_data, test_case_t));
	run_suites(__start_test_case_aes_ctr_data,
		   ITEM_COUNT(test_case_aes_ctr_data, test_case_t));
	run_suites(__start_test_case_aes_ecb_data,
		   ITEM_COUNT(test_case_aes_ecb_data, test_case_t));
	run_suites(__start_test_case_aes_cbc_data,
		   ITEM_COUNT(test_case_aes_cbc_data, test_case_t));
	run_suites(__start_test_case_ecdsa_data,
		   ITEM_COUNT(test_case_ecdh_data, test_case_t));
	run_suites(__start_test_case_ecdh_data,
		   ITEM_COUNT(test_case_ecdh_data, test_case_t));
	run_suites(__start_test_case_sha_256_data,
		   ITEM_COUNT(test_case_sha_256_data, test_case_t));
	run_suites(__start_test_case_sha_512_data,
		   ITEM_COUNT(test_case_sha_512_data, test_case_t));
	run_suites(__start_test_case_hmac_data,
		   ITEM_COUNT(test_case_hmac_data, test_case_t));
	run_suites(__start_test_case_hkdf_data,
		   ITEM_COUNT(test_case_hkdf_data, test_case_t));
	run_suites(__start_test_case_ecjpake_data,
		   ITEM_COUNT(test_case_ecjpake_data, test_case_t));
}
