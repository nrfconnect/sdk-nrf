/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <zephyr/logging/log.h>

#include "common_test.h"
#include <mbedtls/md.h>

/* Setting LOG_LEVEL_DBG might affect time measurements! */
LOG_MODULE_REGISTER(test_hmac, LOG_LEVEL_INF);

extern test_vector_hmac_t __start_test_vector_hmac_data[];
extern test_vector_hmac_t __stop_test_vector_hmac_data[];

#define INPUT_BUF_SIZE (256)
#define OUTPUT_BUF_SIZE (64)
#define HMAC_SHA512_RESULT_SIZE (64)

static mbedtls_md_context_t md_context;

static uint8_t m_hmac_key_buf[INPUT_BUF_SIZE];
static uint8_t m_hmac_input_buf[INPUT_BUF_SIZE];
static uint8_t m_hmac_output_buf[OUTPUT_BUF_SIZE];
static uint8_t m_hmac_expected_output_buf[OUTPUT_BUF_SIZE];

static test_vector_hmac_t *p_test_vector;
static uint32_t hmac_vector_n;
static uint32_t hmac_combined_vector_n;

static size_t key_len;
static size_t in_len;
static size_t expected_hmac_len;
static size_t hmac_len;

void hmac_clear_buffers(void);
void unhexify_hmac(void);

static void hmac_setup(void)
{
	hmac_clear_buffers();
	p_test_vector = ITEM_GET(test_vector_hmac_data, test_vector_hmac_t,
				 hmac_vector_n);
	unhexify_hmac();
}

static void hmac_teardown(void)
{
	hmac_vector_n++;
}

static void hmac_combined_teardown(void)
{
	hmac_combined_vector_n++;
}

void hmac_clear_buffers(void)
{
	memset(m_hmac_key_buf, 0x00, sizeof(m_hmac_key_buf));
	memset(m_hmac_input_buf, 0x00, sizeof(m_hmac_input_buf));
	memset(m_hmac_output_buf, 0x00, sizeof(m_hmac_output_buf));
	memset(m_hmac_expected_output_buf, 0x00,
	       sizeof(m_hmac_expected_output_buf));
}

__attribute__((noinline)) void unhexify_hmac(void)
{
	/* Fetch and unhexify test vectors. */
	key_len = hex2bin_safe(p_test_vector->p_key,
			       m_hmac_key_buf,
			       sizeof(m_hmac_key_buf));
	in_len = hex2bin_safe(p_test_vector->p_input,
			      m_hmac_input_buf,
			      sizeof(m_hmac_input_buf));
	expected_hmac_len = hex2bin_safe(p_test_vector->p_expected_output,
					 m_hmac_expected_output_buf,
					 sizeof(m_hmac_expected_output_buf));
	hmac_len = expected_hmac_len;
}

/**@brief Function for the test execution.
 */
void exec_test_case_hmac(void)
{
	int err_code = -1;

	/* Initialize the HMAC module. */
	mbedtls_md_init(&md_context);

	const mbedtls_md_info_t *p_md_info =
		mbedtls_md_info_from_type(p_test_vector->digest_type);
	err_code = mbedtls_md_setup(&md_context, p_md_info, 1);
	if (err_code != 0) {
		LOG_WRN("mb setup ec: -0x%02X", -err_code);
	}
	TEST_VECTOR_ASSERT_EQUAL(err_code, 0);

	start_time_measurement();
	err_code = mbedtls_md_hmac_starts(&md_context, m_hmac_key_buf, key_len);
	TEST_VECTOR_ASSERT_EQUAL(err_code, 0);

	err_code =
		mbedtls_md_hmac_update(&md_context, m_hmac_input_buf, in_len);
	TEST_VECTOR_ASSERT_EQUAL(err_code, 0);

	/* Finalize the HMAC computation. */
	err_code = mbedtls_md_hmac_finish(&md_context, m_hmac_output_buf);
	stop_time_measurement();

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated HMAC. */
	TEST_VECTOR_ASSERT_EQUAL(expected_hmac_len, hmac_len);
	TEST_VECTOR_MEMCMP_ASSERT(m_hmac_output_buf, m_hmac_expected_output_buf,
				  expected_hmac_len,
				  p_test_vector->expected_result,
				  "Incorrect hmac");

	mbedtls_md_free(&md_context);
}

/**@brief Function for the test execution.
 */
void exec_test_case_hmac_combined(void)
{
	int err_code = -1;

	/* Generate the HMAC using the combined method. */
	const mbedtls_md_info_t *p_md_info =
		mbedtls_md_info_from_type(p_test_vector->digest_type);
	start_time_measurement();
	err_code = mbedtls_md_hmac(p_md_info, m_hmac_key_buf, key_len,
				   m_hmac_input_buf, in_len, m_hmac_output_buf);
	stop_time_measurement();

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	TEST_VECTOR_ASSERT_EQUAL(expected_hmac_len, hmac_len);
	TEST_VECTOR_MEMCMP_ASSERT(m_hmac_output_buf, m_hmac_expected_output_buf,
				  expected_hmac_len,
				  p_test_vector->expected_result,
				  "Incorrect hmac");
}

ITEM_REGISTER(test_case_hmac_data, test_case_t test_hmac) = {
	.p_test_case_name = "HMAC",
	.setup = hmac_setup,
	.exec = exec_test_case_hmac,
	.teardown = hmac_teardown,
	.vector_type = TV_HMAC,
	.vectors_start = __start_test_vector_hmac_data,
	.vectors_stop = __stop_test_vector_hmac_data,
};

ITEM_REGISTER(test_case_hmac_data, test_case_t test_hmac_combined) = {
	.p_test_case_name = "HMAC combined",
	.setup = hmac_setup,
	.exec = exec_test_case_hmac_combined,
	.teardown = hmac_combined_teardown,
	.vector_type = TV_HMAC,
	.vectors_start = __start_test_vector_hmac_data,
	.vectors_stop = __stop_test_vector_hmac_data,
};

#if defined(CONFIG_CRYPTO_TEST_HASH)
ZTEST_SUITE(test_suite_hmac, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_suite_hmac, test_case_hmac)
{
	hmac_setup();
	exec_test_case_hmac();
	hmac_teardown();
}

ZTEST(test_suite_hmac, test_case_hmac_combined)
{
	hmac_setup();
	exec_test_case_hmac_combined();
	hmac_combined_teardown();
}
#endif
