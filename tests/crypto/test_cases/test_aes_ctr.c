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
#include <mbedtls/aes.h>

/* Setting LOG_LEVEL_DBG might affect time measurements! */
LOG_MODULE_REGISTER(test_aes_ctr, LOG_LEVEL_INF);
/* If LOG_LEVEL_DBG and this to true, some hexdumps will be displayed. */
static bool dbg_hexdump_on;

extern test_vector_aes_t __start_test_vector_aes_ctr_data[];
extern test_vector_aes_t __stop_test_vector_aes_ctr_data[];

extern test_vector_aes_t __start_test_vector_aes_ctr_func_data[];
extern test_vector_aes_t __stop_test_vector_aes_ctr_func_data[];

#define NUM_BUFFER_OVERFLOW_TEST_BYTES (2)
#define AES_IV_MAX_SIZE (16)
#define AES_MAC_INPUT_BLOCK_SIZE (16)
#define AES_PLAINTEXT_BUF_SIZE (256)
#define AES_PLAINTEXT_BUF_SIZE_PLUS                                            \
	(AES_PLAINTEXT_BUF_SIZE + NUM_BUFFER_OVERFLOW_TEST_BYTES)
#define AES_MAX_KEY_SIZE (256 / (8))
#define AES_MIN_KEY_SIZE (128 / (8))

static uint8_t m_aes_input_buf[AES_PLAINTEXT_BUF_SIZE_PLUS];
static uint8_t m_aes_output_buf[AES_PLAINTEXT_BUF_SIZE_PLUS];
static uint8_t m_aes_expected_output_buf[AES_PLAINTEXT_BUF_SIZE_PLUS];
/* Monte carlo tests use previous output iterations. */
static uint8_t m_prev_aes_output_buf[AES_PLAINTEXT_BUF_SIZE_PLUS];
static uint8_t m_aes_key_buf[AES_MAX_KEY_SIZE];
static uint8_t m_aes_iv_buf[AES_IV_MAX_SIZE + NUM_BUFFER_OVERFLOW_TEST_BYTES];
static uint8_t m_aes_temp_buf[AES_MAX_KEY_SIZE];

static test_vector_aes_t *p_test_vector;

static size_t input_len;
static size_t output_len;
static size_t key_len;
static size_t iv_len;
static size_t ad_len;

void aes_ctr_clear_buffers(void);
void unhexify_aes_ctr(void);

static int cipher_init(mbedtls_cipher_context_t *p_ctx, size_t key_len_bytes,
		       mbedtls_cipher_mode_t mode)
{
	mbedtls_cipher_init(p_ctx);

	const mbedtls_cipher_info_t *p_cipher_info =
		mbedtls_cipher_info_from_values(MBEDTLS_CIPHER_ID_AES,
						key_len_bytes * 8, mode);
	return mbedtls_cipher_setup(p_ctx, p_cipher_info);
}

static int cipher_set_key(mbedtls_cipher_context_t *p_ctx, size_t key_len_bytes,
			  mbedtls_operation_t operation)
{
	return mbedtls_cipher_setkey(p_ctx, m_aes_key_buf, key_len_bytes * 8,
				     operation);
}

static int cipher_set_padding(mbedtls_cipher_context_t *ctx,
			      mbedtls_cipher_padding_t padding)
{
	/* Only CBC mode supports padding at the moment. */
	return 0;
}

static void cipher_extra_checks_ctr(mbedtls_cipher_context_t *ctx,
				    test_vector_aes_t *p_test_vector,
				    size_t iv_len, size_t ad_len)
{
	/* Verify that the counter value has incremented as expected. */
	TEST_VECTOR_MEMCMP_ASSERT(ctx->iv, m_aes_temp_buf, ad_len, 0,
				  "Incorrect incremented counter value.");

	/* Verify that the next two bytes in buffers are not overwritten. */
	TEST_VECTOR_OVERFLOW_ASSERT(m_aes_iv_buf, iv_len, "IV buffer overflow");
}

static int cipher_crypt(mbedtls_cipher_context_t *p_ctx, size_t iv_len,
			size_t input_len)
{
	size_t crypt_len = 0; /* Unused */

	return mbedtls_cipher_crypt(p_ctx, m_aes_iv_buf, iv_len,
				    m_aes_input_buf, input_len,
				    m_aes_output_buf, &crypt_len);
}

static void aes_ctr_setup_functional(void)
{
	static int i;

	aes_ctr_clear_buffers();
	p_test_vector =
		ITEM_GET(test_vector_aes_ctr_func_data, test_vector_aes_t, i++);

	unhexify_aes_ctr();
}

static void aes_ctr_setup(void)
{
	static int i;

	aes_ctr_clear_buffers();

	p_test_vector =
		ITEM_GET(test_vector_aes_ctr_data, test_vector_aes_t, i++);

	unhexify_aes_ctr();
}

void aes_ctr_clear_buffers(void)
{
	memset(m_aes_input_buf, 0xFF, sizeof(m_aes_input_buf));
	memset(m_aes_output_buf, 0xFF, sizeof(m_aes_output_buf));
	memset(m_aes_expected_output_buf, 0xFF,
	       sizeof(m_aes_expected_output_buf));
	memset(m_prev_aes_output_buf, 0x00, sizeof(m_prev_aes_output_buf));
	memset(m_aes_key_buf, 0x00, sizeof(m_aes_key_buf));
	memset(m_aes_iv_buf, 0xFF, sizeof(m_aes_iv_buf));
	memset(m_aes_temp_buf, 0x00, sizeof(m_aes_temp_buf));
}

__attribute__((noinline)) void unhexify_aes_ctr(void)
{
	bool encrypt = p_test_vector->direction == MBEDTLS_ENCRYPT;

	key_len = hex2bin_safe(p_test_vector->p_key,
			       m_aes_key_buf,
			       sizeof(m_aes_key_buf));
	iv_len = hex2bin_safe(p_test_vector->p_iv,
			      m_aes_iv_buf,
			      sizeof(m_aes_iv_buf));
	ad_len = hex2bin_safe(p_test_vector->p_ad,
			      m_aes_temp_buf,
			      sizeof(m_aes_temp_buf));

	if (encrypt) {
		input_len = hex2bin_safe(p_test_vector->p_plaintext,
					 m_aes_input_buf,
					 sizeof(m_aes_input_buf));
		output_len = hex2bin_safe(p_test_vector->p_ciphertext,
					  m_aes_expected_output_buf,
					  sizeof(m_aes_expected_output_buf));
	} else {
		input_len = hex2bin_safe(p_test_vector->p_ciphertext,
					 m_aes_input_buf,
					 sizeof(m_aes_input_buf));
		output_len = hex2bin_safe(p_test_vector->p_plaintext,
					  m_aes_expected_output_buf,
					  sizeof(m_aes_expected_output_buf));
	}
}

/**@brief Function for the AES functional test execution.
 */
void exec_test_case_aes_ctr_functional(void)
{
	int err_code = -1;

	mbedtls_cipher_context_t ctx;

	err_code = cipher_init(&ctx, key_len, p_test_vector->mode);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_key(&ctx, key_len, MBEDTLS_ENCRYPT);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_padding(&ctx, p_test_vector->padding);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	start_time_measurement();
	err_code = cipher_crypt(&ctx, iv_len, input_len);
	stop_time_measurement();

	/* Verify the nrf_crypto_aes_finalize err_code. */
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated AES ciphertext. */
	TEST_VECTOR_MEMCMP_ASSERT(m_aes_expected_output_buf, m_aes_output_buf,
				  output_len, p_test_vector->expected_result,
				  "Incorrect generated AES ciphertext");

	/* Verify that the next two bytes in buffers are not overwritten. */
	TEST_VECTOR_OVERFLOW_ASSERT(m_aes_output_buf, output_len,
				    "output buffer overflow");
	TEST_VECTOR_OVERFLOW_ASSERT(m_aes_input_buf, input_len,
				    "input buffer overflow");

	/* Reset buffers and fetch test vectors. */
	aes_ctr_clear_buffers();
	unhexify_aes_ctr();

	err_code = cipher_init(&ctx, key_len, p_test_vector->mode);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_key(&ctx, key_len, MBEDTLS_DECRYPT);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_padding(&ctx, p_test_vector->padding);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_crypt(&ctx, iv_len, input_len);
	/* Verify the nrf_crypto_aes_finalize err_code. */
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated AES plaintext. */
	TEST_VECTOR_MEMCMP_ASSERT(m_aes_expected_output_buf, m_aes_output_buf,
				  output_len, p_test_vector->expected_result,
				  "Incorrect generated AES plaintext");

	/* Verify that the next two bytes in buffers are not overwritten. */
	TEST_VECTOR_OVERFLOW_ASSERT(m_aes_output_buf, output_len,
				    "output buffer overflow");
	TEST_VECTOR_OVERFLOW_ASSERT(m_aes_input_buf, input_len,
				    "input buffer overflow");

	/* Free resources */
	mbedtls_cipher_free(&ctx);
}

/**@brief Function for the AES test execution.
 */
void exec_test_case_aes_ctr(void)
{
	int err_code = -1;

	mbedtls_cipher_context_t ctx;

	err_code = cipher_init(&ctx, key_len, p_test_vector->mode);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_key(&ctx, key_len, p_test_vector->direction);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_padding(&ctx, p_test_vector->padding);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Encrypt or decrypt input. */
	start_time_measurement();
	err_code = cipher_crypt(&ctx, iv_len, input_len);
	stop_time_measurement();

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated AES ciphertext. */
	TEST_VECTOR_MEMCMP_ASSERT(m_aes_expected_output_buf, m_aes_output_buf,
				  input_len, p_test_vector->expected_result,
				  "Incorrect generated AES ciphertext");

	/* Extra mode-dependent checks */
	cipher_extra_checks_ctr(&ctx, p_test_vector, iv_len, ad_len);

	/* Free resources */
	mbedtls_cipher_free(&ctx);
}

void monte_carlo_update_key(size_t key_len, size_t ciphertext_len)
{
	if (dbg_hexdump_on) {
		LOG_HEXDUMP_DBG(m_aes_key_buf, key_len,
				"m_aes_key_buf before update");
	}
	uint8_t divider;

	divider = key_len - ciphertext_len;

	/* Xor previous cipher with key if key_len > cipher_len. */
	for (uint8_t xor_start = 0; xor_start < divider; xor_start++) {
		m_aes_key_buf[xor_start] ^=
			m_prev_aes_output_buf[ciphertext_len - divider +
					      xor_start];
	}

	/* Xor cipher with last 16 bytes of key. */
	for (uint8_t xor_start = 0; xor_start < ciphertext_len; xor_start++) {
		m_aes_key_buf[divider + xor_start] ^=
			m_aes_output_buf[xor_start];
	}

	if (dbg_hexdump_on) {
		LOG_HEXDUMP_DBG(m_aes_key_buf, key_len,
				"m_aes_key_buf after update");
	}
}

int monte_carlo(test_vector_aes_t *p_test_vector,
		mbedtls_cipher_context_t *p_ctx, size_t key_len, size_t iv_len,
		size_t input_len, size_t output_len)
{
	uint16_t j;
	int err_code;

	/* Execution of encryption or decryption 1000 times with same AES key. */
	for (j = 0; j < 1000; j++) {
		if (j < 5 && dbg_hexdump_on) {
			LOG_DBG("MC inner #%d", j);
			LOG_HEXDUMP_DBG(m_aes_input_buf, input_len,
					"m_aes_input_buf");
		}
		memcpy(m_prev_aes_output_buf, m_aes_output_buf,
		       sizeof(m_prev_aes_output_buf));
		err_code =
			mbedtls_cipher_update(p_ctx, m_aes_input_buf, input_len,
					      m_aes_output_buf, &output_len);
		TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code,
					 err_code);

		if (j < 5 && dbg_hexdump_on) {
			LOG_HEXDUMP_DBG(m_aes_output_buf, input_len,
					"m_aes_output_buf");
		}
	}

	/* Update the AES key. */
	monte_carlo_update_key(key_len, output_len);

	err_code = cipher_set_key(p_ctx, key_len, p_test_vector->direction);
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	return err_code;
}

/**@brief Function for the AES Monte Carlo test execution.
 */
void exec_test_case_aes_monte_carlo(void)
{
	int err_code;

	TEST_VECTOR_ASSERT_EQUAL(input_len, output_len);

	mbedtls_cipher_context_t ctx;
	err_code = cipher_init(&ctx, key_len, p_test_vector->mode);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_key(&ctx, key_len, p_test_vector->direction);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_padding(&ctx, p_test_vector->padding);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Start the Monte Carlo test. */
	uint32_t k = 0;
	start_time_measurement();
	do {
		if (k < 3 && dbg_hexdump_on) {
			LOG_DBG("MC outer #%d", k);
			LOG_HEXDUMP_DBG(m_aes_key_buf, key_len,
					"m_aes_key_buf outer");
		} else {
			/* Turn off hexdump after a few iterations. */
			dbg_hexdump_on = false;
		}
		err_code = monte_carlo(p_test_vector, &ctx, key_len, iv_len,
				       input_len, output_len);
	} while ((err_code == p_test_vector->expected_err_code) && (++k < 100));
	stop_time_measurement();

	LOG_HEXDUMP_DBG(m_aes_output_buf, output_len, "m_aes_output_buf final");
	LOG_HEXDUMP_DBG(m_aes_expected_output_buf, output_len,
			"m_aes_expected_output_buf");

	/* Verify generated AES plaintext or ciphertext. */
	TEST_VECTOR_MEMCMP_ASSERT(m_aes_expected_output_buf, m_aes_output_buf,
				  output_len, p_test_vector->expected_result,
				  "Incorrect generated AES ciphertext");

	/* Un-initialize resources. */
	mbedtls_cipher_free(&ctx);
}

/** @brief  Macro for registering the AES funtional test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_aes_data",
 *              which is initialized by main.
 */
ITEM_REGISTER(test_case_aes_ctr_data,
	      test_case_t test_aes_encrypt_functional) = {
	.p_test_case_name = "AES CTR Functional",
	.setup = aes_ctr_setup_functional,
	.exec = exec_test_case_aes_ctr_functional,
	.teardown = teardown_pass,
	.vector_type = TV_AES,
	.vectors_start = __start_test_vector_aes_ctr_func_data,
	.vectors_stop = __stop_test_vector_aes_ctr_func_data,
};

/** @brief  Macro for registering the AES test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_aes_data",
 *              which is initialized by main.
 */
ITEM_REGISTER(test_case_aes_ctr_data, test_case_t test_aes_encrypt) = {
	.p_test_case_name = "AES CTR Encrypt",
	.setup = aes_ctr_setup,
	.exec = exec_test_case_aes_ctr,
	.teardown = teardown_pass,
	.vector_type = TV_AES,
	.vectors_start = __start_test_vector_aes_ctr_data,
	.vectors_stop = __stop_test_vector_aes_ctr_data,
};
