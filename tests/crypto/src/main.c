/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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

static void test_state_reset(void)
{
	/* To avoid heap fragmentation, reallocate it.
	 * The first allocation is performed by the kernel.
	 * DRBG uses heap allocation, so it must be reset as well.
	 */
	_heap_free();
	_heap_init();

	TEST_VECTOR_ASSERT_EQUAL(0, init_drbg(NULL, 0));
}

void run_suites(test_case_t *cases, uint32_t case_count)
{
	for (uint32_t c = 0; c < case_count; c++) {
		uint32_t n_cases = get_vector_count(&cases[c]);

		if (n_cases == 0)
			continue;

		/* Inject an extra first test case for setting up state.
		 * If that fails, the rest will not run (fail fast).
		 * Ztest also requires an extra terminating NULL
		 * test case at the end.
		 */
		struct unit_test suites[n_cases + 2];

		uint32_t suite = 0;

		suites[suite].name = TV_NAME("state_reset");
		suites[suite].setup = unit_test_noop;
		suites[suite].teardown = unit_test_noop;
		suites[suite].test = test_state_reset;
		suites[suite].thread_options = 0;

		suite++;

		for (uint32_t cnt = 0; cnt < n_cases; suite++, cnt++) {
			suites[suite].name = get_vector_name(&cases[c], cnt);
			suites[suite].setup = cases[c].setup;
			suites[suite].teardown = cases[c].teardown;
			suites[suite].test = cases[c].exec;
			suites[suite].thread_options = 0;
		}
		/* Terminating test suite */
		suites[suite].test = NULL;

		z_ztest_run_test_suite(cases[c].p_test_case_name, suites);
	}
}

void test_main(void)
{
	LOG_INF("Crypto test app started.");

	if (init_leds() != 0)
		LOG_ERR("Bad leds init!");

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
		   ITEM_COUNT(test_case_ecdsa_data, test_case_t));
	run_suites(__start_test_case_ecdh_data,
		   ITEM_COUNT(test_case_ecdh_data, test_case_t));
	run_suites(__start_test_case_ecjpake_data,
		   ITEM_COUNT(test_case_ecjpake_data, test_case_t));

	#if defined(CONFIG_CRYPTO_TEST_HASH)
	run_suites(__start_test_case_sha_256_data,
		   ITEM_COUNT(test_case_sha_256_data, test_case_t));
	run_suites(__start_test_case_sha_512_data,
		   ITEM_COUNT(test_case_sha_512_data, test_case_t));
	run_suites(__start_test_case_hmac_data,
		   ITEM_COUNT(test_case_hmac_data, test_case_t));
	run_suites(__start_test_case_hkdf_data,
		   ITEM_COUNT(test_case_hkdf_data, test_case_t));
	#endif // CONFIG_CRYPTO_TEST_HASH
}
