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
#include <mbedtls/cmac.h>
#include <mbedtls/aes.h>

/* Setting LOG_LEVEL_DBG might affect time measurements! */
LOG_MODULE_REGISTER(test_aes_ecb_mac, LOG_LEVEL_INF);

extern test_vector_aes_t __start_test_vector_aes_ecb_mac_data[];
extern test_vector_aes_t __stop_test_vector_aes_ecb_mac_data[];

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
static uint8_t m_aes_key_buf[AES_MAX_KEY_SIZE];
static uint8_t m_aes_iv_buf[AES_IV_MAX_SIZE + NUM_BUFFER_OVERFLOW_TEST_BYTES];
static uint8_t m_aes_temp_buf[AES_MAX_KEY_SIZE];

static test_vector_aes_t *p_test_vector;

static size_t input_len;
static size_t output_len;
static size_t key_len;
static size_t iv_len;
static size_t ad_len;

void aes_ecb_mac_clear_buffers(void);
void unhexify_aes_ecb_mac(void);

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

static int cipher_update_iterative(mbedtls_cipher_context_t *p_ctx,
				   size_t input_len)
{
	int err_code;
	size_t round = 0;

	do {
		size_t process_len = MIN(16, input_len);
		err_code = mbedtls_cipher_cmac_update(
			p_ctx, &m_aes_input_buf[16 * round], process_len);
		input_len -= process_len;
		round++;
	} while ((err_code == 0) && (input_len > 0));

	return err_code;
}

static void aes_setup_ecb_mac(void)
{
	static int i;

	aes_ecb_mac_clear_buffers();

	p_test_vector =
		ITEM_GET(test_vector_aes_ecb_mac_data, test_vector_aes_t, i++);

	unhexify_aes_ecb_mac();
}

void aes_ecb_mac_clear_buffers(void)
{
	memset(m_aes_input_buf, 0xFF, sizeof(m_aes_input_buf));
	memset(m_aes_output_buf, 0xFF, sizeof(m_aes_output_buf));
	memset(m_aes_expected_output_buf, 0xFF,
	       sizeof(m_aes_expected_output_buf));
	memset(m_aes_key_buf, 0x00, sizeof(m_aes_key_buf));
	memset(m_aes_iv_buf, 0xFF, sizeof(m_aes_iv_buf));
	memset(m_aes_temp_buf, 0x00, sizeof(m_aes_temp_buf));
}

__attribute__((noinline)) void unhexify_aes_ecb_mac(void)
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

/**@brief Function for the AES MAC test execution.
 */
void exec_test_case_aes_ecb_mac(void)
{
	int err_code = -1;

	mbedtls_cipher_context_t ctx;

	err_code = cipher_init(&ctx, key_len, p_test_vector->mode);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_key(&ctx, key_len, MBEDTLS_ENCRYPT);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_padding(&ctx, p_test_vector->padding);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	LOG_DBG("Using ECB for MAC (CMAC).");
	start_time_measurement();
	err_code = mbedtls_cipher_cmac(ctx.cipher_info, m_aes_key_buf,
				       key_len * 8, m_aes_input_buf, input_len,
				       m_aes_output_buf);
	stop_time_measurement();

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify MAC based on underlying mode. */
	/* CMAC generates a tag only. */
	TEST_VECTOR_MEMCMP_ASSERT(m_aes_expected_output_buf, m_aes_output_buf,
				  16, p_test_vector->expected_result,
				  "Incorrect generated AES MAC");
	/* Redo all but now in iterations. */
	aes_ecb_mac_clear_buffers();
	unhexify_aes_ecb_mac();

	err_code = cipher_init(&ctx, key_len, p_test_vector->mode);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_set_key(&ctx, key_len, MBEDTLS_ENCRYPT);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_cipher_cmac_starts(&ctx, m_aes_key_buf, key_len * 8);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = cipher_update_iterative(&ctx, input_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_cipher_cmac_finish(&ctx, m_aes_output_buf);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	TEST_VECTOR_MEMCMP_ASSERT(m_aes_expected_output_buf, m_aes_output_buf,
				  16, p_test_vector->expected_result,
				  "Incorrect generated AES MAC");
	mbedtls_cipher_free(&ctx);
}

/** @brief  Macro for registering the AES test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_aes_data",
 *              which is initialized by main.
 */
ITEM_REGISTER(test_case_aes_ecb_mac_data, test_case_t test_aes_ecb_mac) = {
	.p_test_case_name = "AES ECB MAC",
	.setup = aes_setup_ecb_mac,
	.exec = exec_test_case_aes_ecb_mac,
	.teardown = teardown_pass,
	.vector_type = TV_AES,
	.vectors_start = __start_test_vector_aes_ecb_mac_data,
	.vectors_stop = __stop_test_vector_aes_ecb_mac_data,
};

ZTEST_SUITE(test_suite_aes_ecb_mac, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_suite_aes_ecb_mac, test_case_aes_ecb_mac)
{
	aes_setup_ecb_mac();
	exec_test_case_aes_ecb_mac();
}
