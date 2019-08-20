/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <logging/log.h>

#include "common_test.h"
#include <mbedtls/ccm.h>

/* Setting LOG_LEVEL_DBG might affect time measurements! */
LOG_MODULE_REGISTER(test_aead, LOG_LEVEL_INF);

extern test_vector_aead_t __start_test_vector_aead_ccm_data[];
extern test_vector_aead_t __stop_test_vector_aead_ccm_data[];
extern test_vector_aead_t __start_test_vector_aead_ccm_simple_data[];
extern test_vector_aead_t __stop_test_vector_aead_ccm_simple_data[];

extern test_vector_aead_t __start_test_vector_aead_gcm_data[];
extern test_vector_aead_t __stop_test_vector_aead_gcm_data[];
extern test_vector_aead_t __start_test_vector_aead_gcm_simple_data[];
extern test_vector_aead_t __stop_test_vector_aead_gcm_simple_data[];

extern test_vector_aead_t __start_test_vector_aead_chachapoly_data[];
extern test_vector_aead_t __stop_test_vector_aead_chachapoly_data[];
extern test_vector_aead_t __start_test_vector_aead_chachapoly_simple_data[];
extern test_vector_aead_t __stop_test_vector_aead_chachapoly_simple_data[];

/* This number of bytes are used as buffer end-markers in some tests
 * such that we can check for overflow via known values.
 */
#define NUM_BUFFER_OVERFLOW_TEST_BYTES (2)

#define AEAD_MAC_SIZE (16)
#define AEAD_MAX_TESTED_NONCE_SIZE (128)
#define AEAD_PLAINTEXT_BUF_SIZE (265)
#define AEAD_PLAINTEXT_BUF_SIZE_PLUS                                           \
	(AEAD_PLAINTEXT_BUF_SIZE + NUM_BUFFER_OVERFLOW_TEST_BYTES)
#define AEAD_MAX_MAC_SIZE_PLUS (AEAD_MAC_SIZE + NUM_BUFFER_OVERFLOW_TEST_BYTES)

#define AEAD_KEY_SIZE_BITS (256)
#define AEAD_KEY_SIZE (AEAD_KEY_SIZE_BITS / 8)

static uint8_t m_aead_input_buf[AEAD_PLAINTEXT_BUF_SIZE];
static uint8_t m_aead_output_buf[AEAD_PLAINTEXT_BUF_SIZE_PLUS];
static uint8_t m_aead_expected_output_buf[AEAD_PLAINTEXT_BUF_SIZE];
static uint8_t m_aead_output_mac_buf[AEAD_MAX_MAC_SIZE_PLUS];
static uint8_t m_aead_expected_mac_buf[AEAD_MAX_MAC_SIZE_PLUS];
static uint8_t m_aead_key_buf[AEAD_KEY_SIZE];
static uint8_t m_aead_ad_buf[AEAD_PLAINTEXT_BUF_SIZE];
static uint8_t m_aead_nonce_buf[AEAD_MAX_TESTED_NONCE_SIZE];

static test_vector_aead_t *p_test_vector;
/* Some tests require overriding the test vector crypt direction.
 * Use this additional flag to force decrypt (if set to false).
 */
static bool g_encrypt = true;
static size_t key_len;
static size_t key_bits;
static size_t ad_len;
static size_t mac_len;
static size_t nonce_len;
static size_t input_len;
static size_t output_len;

void aead_clear_buffers(void)
{
	memset(m_aead_input_buf, 0xFF, sizeof(m_aead_input_buf));
	memset(m_aead_output_buf, 0xFF, sizeof(m_aead_output_buf));
	memset(m_aead_expected_output_buf, 0xFF,
	       sizeof(m_aead_expected_output_buf));
	memset(m_aead_output_mac_buf, 0xFF, sizeof(m_aead_output_mac_buf));
	memset(m_aead_expected_mac_buf, 0xFF, sizeof(m_aead_expected_mac_buf));
	memset(m_aead_key_buf, 0x00, sizeof(m_aead_key_buf));
	memset(m_aead_ad_buf, 0x00, sizeof(m_aead_ad_buf));
	memset(m_aead_nonce_buf, 0x00, sizeof(m_aead_nonce_buf));
}

__attribute__((noinline)) void unhexify_aead(void)
{
	bool encrypt =
		(p_test_vector->direction == MBEDTLS_ENCRYPT) && g_encrypt;

	key_len = hex2bin(p_test_vector->p_key, strlen(p_test_vector->p_key),
			  m_aead_key_buf, strlen(p_test_vector->p_key));
	mac_len =
		hex2bin(p_test_vector->p_mac, strlen(p_test_vector->p_mac),
			m_aead_expected_mac_buf, strlen(p_test_vector->p_mac));
	ad_len = hex2bin(p_test_vector->p_ad, strlen(p_test_vector->p_ad),
			 m_aead_ad_buf, strlen(p_test_vector->p_ad));
	nonce_len =
		hex2bin(p_test_vector->p_nonce, strlen(p_test_vector->p_nonce),
			m_aead_nonce_buf, strlen(p_test_vector->p_nonce));

	/* Fetch and unhexify plaintext and ciphertext for encryption. */
	if (encrypt) {
		input_len = hex2bin(p_test_vector->p_plaintext,
				    strlen(p_test_vector->p_plaintext),
				    m_aead_input_buf,
				    strlen(p_test_vector->p_plaintext));
		output_len = hex2bin(p_test_vector->p_ciphertext,
				     strlen(p_test_vector->p_ciphertext),
				     m_aead_expected_output_buf,
				     strlen(p_test_vector->p_ciphertext));
		mac_len = hex2bin(p_test_vector->p_mac,
				  strlen(p_test_vector->p_mac),
				  m_aead_expected_mac_buf,
				  strlen(p_test_vector->p_mac));
	} else {
		input_len = hex2bin(p_test_vector->p_ciphertext,
				    strlen(p_test_vector->p_ciphertext),
				    m_aead_input_buf,
				    strlen(p_test_vector->p_ciphertext));
		output_len = hex2bin(p_test_vector->p_plaintext,
				     strlen(p_test_vector->p_plaintext),
				     m_aead_expected_output_buf,
				     strlen(p_test_vector->p_plaintext));
		mac_len = hex2bin(p_test_vector->p_mac,
				  strlen(p_test_vector->p_mac),
				  m_aead_output_mac_buf,
				  strlen(p_test_vector->p_mac));
	}
}

void aead_ccm_setup(void)
{
	aead_clear_buffers();

	static int i;
	p_test_vector =
		ITEM_GET(test_vector_aead_ccm_data, test_vector_aead_t, i++);

	unhexify_aead();
}

void aead_gcm_setup(void)
{
	aead_clear_buffers();

	static int i;
	p_test_vector =
		ITEM_GET(test_vector_aead_gcm_data, test_vector_aead_t, i++);

	unhexify_aead();
}

void aead_chachapoly_setup(void)
{
	aead_clear_buffers();

	static int i;
	p_test_vector = ITEM_GET(test_vector_aead_chachapoly_data,
				 test_vector_aead_t, i++);

	unhexify_aead();
}

void aead_ccm_setup_simple(void)
{
	aead_clear_buffers();

	static int i;
	p_test_vector = ITEM_GET(test_vector_aead_ccm_simple_data,
				 test_vector_aead_t, i++);

	unhexify_aead();
}

void aead_gcm_setup_simple(void)
{
	aead_clear_buffers();

	static int i;
	p_test_vector = ITEM_GET(test_vector_aead_gcm_simple_data,
				 test_vector_aead_t, i++);

	unhexify_aead();
}

void aead_chachapoly_setup_simple(void)
{
	aead_clear_buffers();

	static int i;
	p_test_vector = ITEM_GET(test_vector_aead_chachapoly_simple_data,
				 test_vector_aead_t, i++);

	unhexify_aead();
}

#if defined(CONFIG_MBEDTLS_CCM_C)
void exec_test_case_aead_ccm_star_simple(void)
{
	int err_code = -1;

	/* Initialize AEAD. */
	key_bits = key_len * 8;

	mbedtls_ccm_context ctx;
	mbedtls_ccm_init(&ctx);

	/* Set secret key. */
	err_code = mbedtls_ccm_setkey(&ctx, p_test_vector->id, m_aead_key_buf,
				      key_bits);
	LOG_DBG("Err code setkey: %d", err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	size_t operation_len = output_len;
	start_time_measurement();
	if (p_test_vector->direction == MBEDTLS_ENCRYPT) {
		err_code = mbedtls_ccm_star_encrypt_and_tag(
			&ctx, input_len, m_aead_nonce_buf, nonce_len,
			m_aead_ad_buf, ad_len, m_aead_input_buf,
			m_aead_output_buf, m_aead_output_mac_buf, mac_len);
	} else {
		err_code = mbedtls_ccm_star_auth_decrypt(
			&ctx, input_len, m_aead_nonce_buf, nonce_len,
			m_aead_ad_buf, ad_len, m_aead_input_buf,
			m_aead_output_buf, m_aead_output_mac_buf, mac_len);
	}

	stop_time_measurement();

	LOG_DBG("Err code %s: -0x%04X, operation len: %d",
		p_test_vector->direction ? "encrypt" : "decrypt", -err_code,
		operation_len);
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	if (input_len != 0) {
		TEST_VECTOR_MEMCMP_ASSERT(m_aead_expected_output_buf,
					  m_aead_output_buf, output_len,
					  p_test_vector->crypt_expected_result,
					  "Output buf check");
	}
	if (p_test_vector->direction == MBEDTLS_ENCRYPT) {
		TEST_VECTOR_MEMCMP_ASSERT(m_aead_expected_mac_buf,
					  m_aead_output_mac_buf, mac_len,
					  p_test_vector->mac_expected_result,
					  "MAC buf check");
	}

	/* Free resources. */
	mbedtls_ccm_free(&ctx);
}

void exec_test_case_aead_ccm_star(void)
{
	int err_code = -1;

	/* Initialize AEAD. */
	key_bits = key_len * 8;

	mbedtls_ccm_context ctx;
	mbedtls_ccm_init(&ctx);

	/* Set secret key. */
	err_code = mbedtls_ccm_setkey(&ctx, p_test_vector->id, m_aead_key_buf,
				      key_bits);
	LOG_DBG("Err code setkey: %d", err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	size_t operation_len = output_len;
	start_time_measurement();
	err_code = mbedtls_ccm_star_encrypt_and_tag(
		&ctx, input_len, m_aead_nonce_buf, nonce_len, m_aead_ad_buf,
		ad_len, m_aead_input_buf, m_aead_output_buf,
		m_aead_output_mac_buf, mac_len);
	stop_time_measurement();

	LOG_DBG("Err code %s: -0x%04X, operation len: %d", "encrypt", -err_code,
		operation_len);
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	TEST_VECTOR_MEMCMP_ASSERT(m_aead_expected_output_buf, m_aead_output_buf,
				  output_len,
				  p_test_vector->crypt_expected_result,
				  "Output buf check");
	TEST_VECTOR_MEMCMP_ASSERT(m_aead_expected_mac_buf,
				  m_aead_output_mac_buf, mac_len,
				  p_test_vector->mac_expected_result,
				  "MAC buf check");

	/* Overflow checks */
	TEST_VECTOR_OVERFLOW_ASSERT(m_aead_output_buf, output_len,
				    "output buffer overflow");
	TEST_VECTOR_OVERFLOW_ASSERT(m_aead_output_mac_buf, mac_len,
				    "MAC buffer overflow");

	/* Decrypt part. */
	aead_clear_buffers();
	g_encrypt = false;
	unhexify_aead();

	/* Set secret key. */
	err_code = mbedtls_ccm_setkey(&ctx, p_test_vector->id, m_aead_key_buf,
				      key_bits);
	LOG_DBG("Err code setkey: %d", err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ccm_star_auth_decrypt(
		&ctx, input_len, m_aead_nonce_buf, nonce_len, m_aead_ad_buf,
		ad_len, m_aead_input_buf, m_aead_output_buf,
		m_aead_output_mac_buf, mac_len);

	LOG_DBG("Err code %s: -0x%04X, operation len: %d", "decrypt", -err_code,
		operation_len);
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	TEST_VECTOR_MEMCMP_ASSERT(m_aead_expected_output_buf, m_aead_output_buf,
				  output_len,
				  p_test_vector->crypt_expected_result,
				  "Output buf check");

	/* Overflow checks */
	TEST_VECTOR_OVERFLOW_ASSERT(m_aead_output_buf, output_len,
				    "output buffer overflow");
	TEST_VECTOR_OVERFLOW_ASSERT(m_aead_output_mac_buf, mac_len,
				    "MAC buffer overflow");

	/* Free resources. */
	mbedtls_ccm_free(&ctx);
	g_encrypt = true;
}
#endif /* MBEDTLS_CCM_C */

/**@brief Function for the AEAD test execution.
 */
void exec_test_case_aead(void)
{
	int err_code = -1;

#if defined(CONFIG_MBEDTLS_CCM_C)
	if (p_test_vector->mode == MBEDTLS_MODE_CCM &&
	    p_test_vector->ccm_star) {
		exec_test_case_aead_ccm_star();
		return;
	}
#endif /* MBEDTLS_CCM_C */

	/* Initialize AEAD. */
	key_bits = key_len * 8;

	mbedtls_cipher_context_t ctx;
	mbedtls_cipher_init(&ctx);

	/* Setup cipher. */
	const mbedtls_cipher_info_t *p_info = mbedtls_cipher_info_from_values(
		p_test_vector->id, key_bits, p_test_vector->mode);
	if (p_info == NULL) {
		LOG_DBG("Err code info from values: id: %d, key bits: %d, mode: %d",
			p_test_vector->id, key_bits, p_test_vector->mode);
	}

	err_code = mbedtls_cipher_setup(&ctx, p_info);
	if (err_code != 0) {
		LOG_DBG("Err code setup: %d", err_code);
	}
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Set secret key. */
	err_code = mbedtls_cipher_setkey(&ctx, m_aead_key_buf, key_bits,
					 MBEDTLS_ENCRYPT);
	LOG_DBG("Err code setkey: %d", err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	size_t operation_len = output_len;
	start_time_measurement();
	err_code = mbedtls_cipher_auth_encrypt(
		&ctx, m_aead_nonce_buf, nonce_len, m_aead_ad_buf, ad_len,
		m_aead_input_buf, input_len, m_aead_output_buf, &operation_len,
		m_aead_output_mac_buf, mac_len);
	stop_time_measurement();

	LOG_DBG("Err code %s: -0x%04X, operation len: %d",
		p_test_vector->direction ? "encrypt" : "decrypt", -err_code,
		operation_len);
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	TEST_VECTOR_MEMCMP_ASSERT(m_aead_expected_output_buf, m_aead_output_buf,
				  output_len,
				  p_test_vector->crypt_expected_result,
				  "Output buf check");
	TEST_VECTOR_MEMCMP_ASSERT(m_aead_expected_mac_buf,
				  m_aead_output_mac_buf, mac_len,
				  p_test_vector->mac_expected_result,
				  "MAC buf check");

	/* Overflow checks */
	TEST_VECTOR_OVERFLOW_ASSERT(m_aead_output_buf, output_len,
				    "output buffer overflow");
	TEST_VECTOR_OVERFLOW_ASSERT(m_aead_output_mac_buf, mac_len,
				    "MAC buffer overflow");

	/* Decrypt part. */
	aead_clear_buffers();
	g_encrypt = false;
	unhexify_aead();

	/* Set secret key. */
	err_code = mbedtls_cipher_setkey(&ctx, m_aead_key_buf, key_bits,
					 MBEDTLS_DECRYPT);
	LOG_DBG("Err code setkey: %d", err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_cipher_auth_decrypt(
		&ctx, m_aead_nonce_buf, nonce_len, m_aead_ad_buf, ad_len,
		m_aead_input_buf, input_len, m_aead_output_buf, &operation_len,
		m_aead_output_mac_buf, mac_len);

	LOG_DBG("Err code %s: -0x%04X, operation len: %d", "decrypt", -err_code,
		operation_len);
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	TEST_VECTOR_MEMCMP_ASSERT(m_aead_expected_output_buf, m_aead_output_buf,
				  output_len,
				  p_test_vector->crypt_expected_result,
				  "Output buf check");

	/* Overflow checks */
	TEST_VECTOR_OVERFLOW_ASSERT(m_aead_output_buf, output_len,
				    "output buffer overflow");
	TEST_VECTOR_OVERFLOW_ASSERT(m_aead_output_mac_buf, mac_len,
				    "MAC buffer overflow");

	/* Free resources. */
	mbedtls_cipher_free(&ctx);
	g_encrypt = true;
}

/**@brief Function for the AEAD test execution.
 */
void exec_test_case_aead_simple(void)
{
	int err_code = -1;

#if defined(CONFIG_MBEDTLS_CCM_C)
	if (p_test_vector->mode == MBEDTLS_MODE_CCM &&
	    p_test_vector->ccm_star) {
		exec_test_case_aead_ccm_star_simple();
		return;
	}
#endif

	/* Initialize AEAD. */
	key_bits = key_len * 8;

	mbedtls_cipher_context_t ctx;
	mbedtls_cipher_init(&ctx);

	/* Setup cipher. */
	const mbedtls_cipher_info_t *p_info = mbedtls_cipher_info_from_values(
		p_test_vector->id, key_bits, p_test_vector->mode);

	err_code = mbedtls_cipher_setup(&ctx, p_info);
	LOG_DBG("Err code setup: -0x%04X", -err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Set secret key. */
	err_code = mbedtls_cipher_setkey(&ctx, m_aead_key_buf, key_bits,
					 p_test_vector->direction);
	LOG_DBG("Err code setkey: -0x%04X", -err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	size_t operation_len = output_len;
	start_time_measurement();
	if (p_test_vector->direction == MBEDTLS_ENCRYPT) {
		err_code = mbedtls_cipher_auth_encrypt(
			&ctx, m_aead_nonce_buf, nonce_len, m_aead_ad_buf,
			ad_len, m_aead_input_buf, input_len, m_aead_output_buf,
			&operation_len, m_aead_output_mac_buf, mac_len);
	} else {
		err_code = mbedtls_cipher_auth_decrypt(
			&ctx, m_aead_nonce_buf, nonce_len, m_aead_ad_buf,
			ad_len, m_aead_input_buf, input_len, m_aead_output_buf,
			&operation_len, m_aead_output_mac_buf, mac_len);
	}

	stop_time_measurement();

	LOG_DBG("Err code %s: -0x%04X, operation len: %d",
		p_test_vector->direction ? "encrypt" : "decrypt", -err_code,
		operation_len);
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	if (input_len != 0) {
		TEST_VECTOR_MEMCMP_ASSERT(m_aead_expected_output_buf,
					  m_aead_output_buf, output_len,
					  p_test_vector->crypt_expected_result,
					  "Output buf check");
	}
	if (p_test_vector->direction == MBEDTLS_ENCRYPT) {
		TEST_VECTOR_MEMCMP_ASSERT(m_aead_expected_mac_buf,
					  m_aead_output_mac_buf, mac_len,
					  p_test_vector->mac_expected_result,
					  "MAC buf check");
	}

	/* Free resources. */
	mbedtls_cipher_free(&ctx);
}

/** @brief  Macro for registering the AEAD test case by using section variables.
 *
 */
ITEM_REGISTER(test_case_aead_ccm_data, test_case_t test_aead_ccm) = {
	.p_test_case_name = "AEAD CCM",
	.setup = aead_ccm_setup,
	.exec = exec_test_case_aead,
	.teardown = teardown_pass,
	.vector_type = TV_AEAD,
	.vectors_start = __start_test_vector_aead_ccm_data,
	.vectors_stop = __stop_test_vector_aead_ccm_data,
};

/** @brief  Macro for registering the AEAD test case by using section variables.
 *
 */
ITEM_REGISTER(test_case_aead_ccm_simple_data,
	      test_case_t test_aead_ccm_simple) = {
	.p_test_case_name = "AEAD CCM simple",
	.setup = aead_ccm_setup_simple,
	.exec = exec_test_case_aead_simple,
	.teardown = teardown_pass,
	.vector_type = TV_AEAD,
	.vectors_start = __start_test_vector_aead_ccm_simple_data,
	.vectors_stop = __stop_test_vector_aead_ccm_simple_data,
};

/** @brief  Macro for registering the AEAD test case by using section variables.
 *
 */
ITEM_REGISTER(test_case_aead_gcm_data, test_case_t test_aead_gcm) = {
	.p_test_case_name = "AEAD GCM",
	.setup = aead_gcm_setup,
	.exec = exec_test_case_aead,
	.teardown = teardown_pass,
	.vector_type = TV_AEAD,
	.vectors_start = __start_test_vector_aead_gcm_data,
	.vectors_stop = __stop_test_vector_aead_gcm_data,
};

/** @brief  Macro for registering the AEAD test case by using section variables.
 *
 */
ITEM_REGISTER(test_case_aead_gcm_simple_data,
	      test_case_t test_aead_gcm_simple) = {
	.p_test_case_name = "AEAD GCM simple",
	.setup = aead_gcm_setup_simple,
	.exec = exec_test_case_aead_simple,
	.teardown = teardown_pass,
	.vector_type = TV_AEAD,
	.vectors_start = __start_test_vector_aead_gcm_simple_data,
	.vectors_stop = __stop_test_vector_aead_gcm_simple_data,
};

/** @brief  Macro for registering the AEAD test case by using section variables.
 *
 */
ITEM_REGISTER(test_case_aead_chachapoly_data,
	      test_case_t test_aead_chachapoly) = {
	.p_test_case_name = "AEAD CHACHAPOLY",
	.setup = aead_chachapoly_setup,
	.exec = exec_test_case_aead,
	.teardown = teardown_pass,
	.vector_type = TV_AEAD,
	.vectors_start = __start_test_vector_aead_chachapoly_data,
	.vectors_stop = __stop_test_vector_aead_chachapoly_data,
};

/** @brief  Macro for registering the AEAD test case by using section variables.
 *
 */
ITEM_REGISTER(test_case_aead_chachapoly_simple_data,
	      test_case_t test_aead_chachapoly_simple) = {
	.p_test_case_name = "AEAD CHACHAPOLY simple",
	.setup = aead_chachapoly_setup_simple,
	.exec = exec_test_case_aead_simple,
	.teardown = teardown_pass,
	.vector_type = TV_AEAD,
	.vectors_start = __start_test_vector_aead_chachapoly_simple_data,
	.vectors_stop = __stop_test_vector_aead_chachapoly_simple_data,
};
