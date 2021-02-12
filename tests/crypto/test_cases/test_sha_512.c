/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <logging/log.h>

#include "common_test.h"
#include <mbedtls/md.h>
#include <sha512.h>

/* Setting LOG_LEVEL_DBG might affect time measurements! */
LOG_MODULE_REGISTER(test_sha_512, LOG_LEVEL_INF);

extern test_vector_hash_t __start_test_vector_hash_512_data[];
extern test_vector_hash_t __stop_test_vector_hash_512_data[];

extern test_vector_hash_t __start_test_vector_hash_512_long_data[];
extern test_vector_hash_t __stop_test_vector_hash_512_long_data[];

#if defined(CONFIG_CRYPTO_TEST_LARGE_VECTORS)
#define INPUT_BUF_SIZE (4125)
#else
#define INPUT_BUF_SIZE (512)
#endif // CRYPTO_TEST_LARGE_VECTORS

#define OUTPUT_BUF_SIZE (64)

static mbedtls_sha512_context sha512_context;

static uint8_t m_sha_input_buf[INPUT_BUF_SIZE];
static uint8_t m_sha_output_buf[OUTPUT_BUF_SIZE];
static uint8_t m_sha_expected_output_buf[OUTPUT_BUF_SIZE];

static test_vector_hash_t *p_test_vector;
static uint32_t sha_vector_n;
static uint32_t sha_long_vector_n;

static size_t in_len;
static size_t out_len;
static size_t expected_out_len;

void sha_512_clear_buffers(void);
void unhexify_sha_512(void);
void unhexify_sha_512_long(void);

static void sha_512_setup(void)
{
	sha_512_clear_buffers();
	p_test_vector = ITEM_GET(test_vector_hash_512_data, test_vector_hash_t,
				 sha_vector_n);
	unhexify_sha_512();
}

static void sha_512_teardown(void)
{
	sha_vector_n++;
}

static void sha_512_long_setup(void)
{
	sha_512_clear_buffers();
	p_test_vector = ITEM_GET(test_vector_hash_512_long_data,
				 test_vector_hash_t, sha_long_vector_n);
	unhexify_sha_512_long();
}

static void sha_512_long_teardown(void)
{
	sha_long_vector_n++;
}

void sha_512_clear_buffers(void)
{
	memset(m_sha_input_buf, 0x00, sizeof(m_sha_input_buf));
	memset(m_sha_output_buf, 0x00, sizeof(m_sha_output_buf));
	memset(m_sha_expected_output_buf, 0x00,
	       sizeof(m_sha_expected_output_buf));
}

__attribute__((noinline)) void unhexify_sha_512(void)
{
	/* Fetch and unhexify test vectors. */
	in_len = hex2bin(p_test_vector->p_input, strlen(p_test_vector->p_input),
			 m_sha_input_buf, strlen(p_test_vector->p_input));
	expected_out_len = hex2bin(p_test_vector->p_expected_output,
				   strlen(p_test_vector->p_expected_output),
				   m_sha_expected_output_buf,
				   strlen(p_test_vector->p_expected_output));
	out_len = expected_out_len;
}

__attribute__((noinline)) void unhexify_sha_512_long(void)
{
	/* Fetch and unhexify test vectors. */
	in_len = p_test_vector->chunk_length;
	expected_out_len = hex2bin(p_test_vector->p_expected_output,
				   strlen(p_test_vector->p_expected_output),
				   m_sha_expected_output_buf,
				   strlen(p_test_vector->p_expected_output));
	out_len = expected_out_len;
	memcpy(m_sha_input_buf, p_test_vector->p_input, in_len);
}

/**@brief Function encapsulating sha512 execution steps.
 *
 */
static int exec_sha_512(test_vector_hash_t *p_test_vector, int in_len,
			bool is_long)
{
	mbedtls_sha512_init(&sha512_context);
	int err_code = mbedtls_sha512_starts_ret(&sha512_context, false);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Update the hash. */
	if (!is_long) {
		err_code = mbedtls_sha512_update_ret(&sha512_context,
						     m_sha_input_buf, in_len);
	} else {
		/* Update the hash until all input data is processed. */
		for (int j = 0; j < p_test_vector->update_iterations; j++) {
			/* Test mode for measuring the memcpy from the flash in SHA. */
			if (p_test_vector->mode == DO_MEMCPY) {
				memcpy(m_sha_input_buf, p_test_vector->p_input,
				       4096);
			}

			err_code = mbedtls_sha512_update_ret(
				&sha512_context, m_sha_input_buf, in_len);
			TEST_VECTOR_ASSERT_EQUAL(
				p_test_vector->expected_err_code, err_code);
		}
	}

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Finalize the hash. */
	return mbedtls_sha512_finish_ret(&sha512_context, m_sha_output_buf);
}

/**@brief Function for verifying the SHA digest of messages.
 */
void exec_test_case_sha_512(void)
{
	int err_code = -1;

	start_time_measurement();
	err_code = exec_sha_512(p_test_vector, in_len, false);
	stop_time_measurement();

	/* Verify the mbedtls_sha512_finish_ret err_code. */
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated digest. */
	TEST_VECTOR_ASSERT_EQUAL(expected_out_len, out_len);
	TEST_VECTOR_MEMCMP_ASSERT(m_sha_output_buf, m_sha_expected_output_buf,
				  expected_out_len,
				  p_test_vector->expected_result,
				  "Incorrect hash");

	/* Do the same in a single step */
	err_code = mbedtls_sha512_ret(m_sha_input_buf, in_len, m_sha_output_buf,
				      false);

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated digest. */
	TEST_VECTOR_ASSERT_EQUAL(expected_out_len, out_len);
	TEST_VECTOR_MEMCMP_ASSERT(m_sha_output_buf, m_sha_expected_output_buf,
				  expected_out_len,
				  p_test_vector->expected_result,
				  "Incorrect hash");

	mbedtls_sha512_free(&sha512_context);
}

/**@brief Function for verifying the SHA digest of long messages.
 */
void exec_test_case_sha_512_long(void)
{
	int err_code = -1;

	start_time_measurement();
	err_code = exec_sha_512(p_test_vector, in_len, true);
	stop_time_measurement();

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated digest. */
	TEST_VECTOR_ASSERT_EQUAL(expected_out_len, out_len);
	TEST_VECTOR_MEMCMP_ASSERT(m_sha_output_buf, m_sha_expected_output_buf,
				  expected_out_len,
				  p_test_vector->expected_result,
				  "Incorrect hash");

	mbedtls_sha512_free(&sha512_context);
}

ITEM_REGISTER(test_case_sha_512_data, test_case_t test_sha_512) = {
	.p_test_case_name = "SHA 512",
	.setup = sha_512_setup,
	.exec = exec_test_case_sha_512,
	.teardown = sha_512_teardown,
	.vector_type = TV_HASH,
	.vectors_start = __start_test_vector_hash_512_data,
	.vectors_stop = __stop_test_vector_hash_512_data,
};

ITEM_REGISTER(test_case_sha_512_data, test_case_t test_sha_512_long) = {
	.p_test_case_name = "SHA 512 long",
	.setup = sha_512_long_setup,
	.exec = exec_test_case_sha_512_long,
	.teardown = sha_512_long_teardown,
	.vector_type = TV_HASH,
	.vectors_start = __start_test_vector_hash_512_long_data,
	.vectors_stop = __stop_test_vector_hash_512_long_data,
};
