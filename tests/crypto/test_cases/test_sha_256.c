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
#include <sha256.h>
#include <mbedtls/md.h>

/* Setting LOG_LEVEL_DBG might affect time measurements! */
LOG_MODULE_REGISTER(test_sha_256, LOG_LEVEL_INF);

extern test_vector_hash_t __start_test_vector_hash_256_data[];
extern test_vector_hash_t __stop_test_vector_hash_256_data[];

extern test_vector_hash_t __start_test_vector_hash_256_long_data[];
extern test_vector_hash_t __stop_test_vector_hash_256_long_data[];

#if defined(CONFIG_CRYPTO_TEST_LARGE_VECTORS)
#define INPUT_BUF_SIZE (4125)
#else
#define INPUT_BUF_SIZE (512)
#endif /* CRYPTO_TEST_LARGE_VECTORS */

#define OUTPUT_BUF_SIZE (64)

static mbedtls_sha256_context sha256_context;

static uint8_t m_sha_input_buf[INPUT_BUF_SIZE];
static uint8_t m_sha_output_buf[OUTPUT_BUF_SIZE];
static uint8_t m_sha_expected_output_buf[OUTPUT_BUF_SIZE];

static test_vector_hash_t *p_test_vector;
static uint32_t sha_vector_n;
static uint32_t sha_long_vector_n;

static size_t in_len;
static size_t out_len;
static size_t expected_out_len;

void sha_256_clear_buffers(void);
void unhexify_sha_256(void);
void unhexify_sha_256_long(void);

void sha_256_clear_buffers(void)
{
	memset(m_sha_input_buf, 0x00, sizeof(m_sha_input_buf));
	memset(m_sha_output_buf, 0x00, sizeof(m_sha_output_buf));
	memset(m_sha_expected_output_buf, 0x00,
	       sizeof(m_sha_expected_output_buf));
}

static void sha_256_setup(void)
{
	sha_256_clear_buffers();
	p_test_vector = ITEM_GET(test_vector_hash_256_data, test_vector_hash_t,
				 sha_vector_n);
	unhexify_sha_256();
}

static void sha_256_teardown(void)
{
	sha_vector_n++;
}

static void sha_256_long_setup(void)
{
	sha_256_clear_buffers();
	p_test_vector = ITEM_GET(test_vector_hash_256_long_data,
				 test_vector_hash_t, sha_long_vector_n);
	unhexify_sha_256_long();
}

static void sha_256_long_teardown(void)
{
	sha_long_vector_n++;
}

__attribute__((noinline)) void unhexify_sha_256(void)
{
	/* Fetch and unhexify test vectors. */
	in_len = hex2bin_safe(p_test_vector->p_input,
			      m_sha_input_buf,
			      sizeof(m_sha_input_buf));
	expected_out_len = hex2bin_safe(p_test_vector->p_expected_output,
					m_sha_expected_output_buf,
					sizeof(m_sha_expected_output_buf));
	out_len = expected_out_len;
}

__attribute__((noinline)) void unhexify_sha_256_long(void)
{
	/* Fetch and unhexify test vectors. */
	in_len = p_test_vector->chunk_length;
	expected_out_len = hex2bin_safe(p_test_vector->p_expected_output,
					m_sha_expected_output_buf,
					sizeof(m_sha_expected_output_buf));
	out_len = expected_out_len;
	memcpy(m_sha_input_buf, p_test_vector->p_input, in_len);
}

/**@brief Function encapsulating sha256 execution steps.
 *
 */
static int exec_sha256(test_vector_hash_t *p_test_vector, int in_len,
		       bool is_long)
{
	mbedtls_sha256_init(&sha256_context);
	int err_code = mbedtls_sha256_starts(&sha256_context, false);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Update the hash. */
	if (!is_long) {
		err_code = mbedtls_sha256_update(&sha256_context,
						     m_sha_input_buf, in_len);
	} else {
		/* Update the hash until all input data is processed. */
		for (int j = 0; j < p_test_vector->update_iterations; j++) {
			/* Test mode for measuring the memcpy from the flash in SHA. */
			if (p_test_vector->mode == DO_MEMCPY) {
				memcpy(m_sha_input_buf, p_test_vector->p_input,
				       4096);
			}

			err_code = mbedtls_sha256_update(
				&sha256_context, m_sha_input_buf, in_len);
			TEST_VECTOR_ASSERT_EQUAL(
				p_test_vector->expected_err_code, err_code);
		}
	}

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Finalize the hash. */
	return mbedtls_sha256_finish(&sha256_context, m_sha_output_buf);
}

/**@brief Function for verifying the SHA digest of messages.
 */
void exec_test_case_sha_256(void)
{
	int err_code = -1;

	start_time_measurement();
	err_code = exec_sha256(p_test_vector, in_len, false);
	stop_time_measurement();

	/* Verify the mbedtls_sha256_finish_ret err_code. */
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated digest. */
	TEST_VECTOR_ASSERT_EQUAL(expected_out_len, out_len);
	TEST_VECTOR_MEMCMP_ASSERT(m_sha_output_buf, m_sha_expected_output_buf,
				  expected_out_len,
				  p_test_vector->expected_result,
				  "Incorrect hash");

	/* Do the same in a single step */
	err_code = mbedtls_sha256(m_sha_input_buf, in_len, m_sha_output_buf,
				      false);

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated digest. */
	TEST_VECTOR_ASSERT_EQUAL(expected_out_len, out_len);
	TEST_VECTOR_MEMCMP_ASSERT(m_sha_output_buf, m_sha_expected_output_buf,
				  expected_out_len,
				  p_test_vector->expected_result,
				  "Incorrect hash");

	mbedtls_sha256_free(&sha256_context);
}

/**@brief Function for verifying the SHA digest of long messages.
 */
void exec_test_case_sha_256_long(void)
{
	int err_code = -1;

	start_time_measurement();
	err_code = exec_sha256(p_test_vector, in_len, true);
	stop_time_measurement();

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated digest. */
	TEST_VECTOR_ASSERT_EQUAL(expected_out_len, out_len);
	TEST_VECTOR_MEMCMP_ASSERT(m_sha_output_buf, m_sha_expected_output_buf,
				  expected_out_len,
				  p_test_vector->expected_result,
				  "Incorrect hash");

	mbedtls_sha256_free(&sha256_context);
}

ITEM_REGISTER(test_case_sha_256_data, test_case_t test_sha_256) = {
	.p_test_case_name = "SHA 256",
	.setup = sha_256_setup,
	.exec = exec_test_case_sha_256,
	.teardown = sha_256_teardown,
	.vector_type = TV_HASH,
	.vectors_start = __start_test_vector_hash_256_data,
	.vectors_stop = __stop_test_vector_hash_256_data,
};

ITEM_REGISTER(test_case_sha_256_data, test_case_t test_sha_256_long) = {
	.p_test_case_name = "SHA 256 long",
	.setup = sha_256_long_setup,
	.exec = exec_test_case_sha_256_long,
	.teardown = sha_256_long_teardown,
	.vector_type = TV_HASH,
	.vectors_start = __start_test_vector_hash_256_long_data,
	.vectors_stop = __stop_test_vector_hash_256_long_data,
};

ZTEST_SUITE(test_suite_sha_256, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_suite_sha_256, test_case_sha_256)
{
	sha_256_setup();
	exec_test_case_sha_256();
	sha_256_teardown();
}

ZTEST(test_suite_sha_256, test_case_sha_256_long)
{
	sha_256_long_setup();
	exec_test_case_sha_256_long();
	sha_256_long_teardown();
}
