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
#include <mbedtls/hkdf.h>

/* Setting LOG_LEVEL_DBG might affect time measurements! */
LOG_MODULE_REGISTER(test_hkdf, LOG_LEVEL_INF);

extern test_vector_hkdf_t __start_test_vector_hkdf_data[];
extern test_vector_hkdf_t __stop_test_vector_hkdf_data[];

/* TODO: Possibly tune buffers which have lower size requirements */
#define HKDF_BUF_SIZE (258)

/* Input key material */
static uint8_t m_hkdf_ikm_buf[HKDF_BUF_SIZE];
/* Output key material */
static uint8_t m_hkdf_okm_buf[HKDF_BUF_SIZE];
/* Pseudo-random key (material) */
static uint8_t m_hkdf_prk_buf[HKDF_BUF_SIZE];
static uint8_t m_hkdf_salt_buf[HKDF_BUF_SIZE];
static uint8_t m_hkdf_info_buf[HKDF_BUF_SIZE];
static uint8_t m_hkdf_expected_okm_buf[HKDF_BUF_SIZE];

static uint8_t *p_hkdp_salt;
static uint8_t *p_hkdp_info;

static test_vector_hkdf_t *p_test_vector;

static size_t ikm_len;
static size_t okm_len;
static size_t prk_len;
static size_t salt_len;
static size_t info_len;
static size_t expected_okm_len;

void hkdf_clear_buffers(void)
{
	memset(m_hkdf_ikm_buf, 0x00, sizeof(m_hkdf_ikm_buf));
	memset(m_hkdf_okm_buf, 0xFF, sizeof(m_hkdf_okm_buf));
	memset(m_hkdf_prk_buf, 0x00, sizeof(m_hkdf_prk_buf));
	memset(m_hkdf_salt_buf, 0x00, sizeof(m_hkdf_salt_buf));
	memset(m_hkdf_info_buf, 0x00, sizeof(m_hkdf_info_buf));
	memset(m_hkdf_expected_okm_buf, 0x00, sizeof(m_hkdf_expected_okm_buf));
}

__attribute__((noinline)) void unhexify_hkdf(void)
{
	/* Fetch and unhexify test vectors. */
	ikm_len = hex2bin_safe(p_test_vector->p_ikm,
			       m_hkdf_ikm_buf,
			       sizeof(m_hkdf_ikm_buf));
	prk_len = hex2bin_safe(p_test_vector->p_prk,
			       m_hkdf_prk_buf,
			       sizeof(m_hkdf_prk_buf));
	salt_len = hex2bin_safe(p_test_vector->p_salt,
				m_hkdf_salt_buf,
				sizeof(m_hkdf_salt_buf));
	info_len = hex2bin_safe(p_test_vector->p_info,
				m_hkdf_info_buf,
				sizeof(m_hkdf_info_buf));
	expected_okm_len = hex2bin_safe(p_test_vector->p_okm,
					m_hkdf_expected_okm_buf,
					sizeof(m_hkdf_expected_okm_buf));
	okm_len = expected_okm_len;

	p_hkdp_salt = (salt_len == 0) ? NULL : m_hkdf_salt_buf;
	p_hkdp_info = (info_len == 0) ? NULL : m_hkdf_info_buf;
}

void hkdf_setup(void)
{
	static uint32_t i;

	hkdf_clear_buffers();
	p_test_vector =
		ITEM_GET(test_vector_hkdf_data, test_vector_hkdf_t, i++);
	unhexify_hkdf();
}

/**@brief Function for the test execution.
 */
void exec_test_case_hkdf(void)
{
	int err_code = -1;

	/* Calculation of the HKDF extract and expand. */
	start_time_measurement();

	const mbedtls_md_info_t *p_md_info =
		mbedtls_md_info_from_type(p_test_vector->digest_type);
	err_code = mbedtls_hkdf(p_md_info, p_hkdp_salt, salt_len,
				m_hkdf_ikm_buf, ikm_len, m_hkdf_info_buf,
				info_len, m_hkdf_okm_buf, okm_len);
	stop_time_measurement();

	LOG_DBG("Error code extract and expand: %d, expected: %d", err_code,
		p_test_vector->expected_err_code);
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	/* Verify the generated HKDF output key material. */
	TEST_VECTOR_ASSERT_EQUAL(expected_okm_len, okm_len);
	TEST_VECTOR_MEMCMP_ASSERT(m_hkdf_okm_buf, m_hkdf_expected_okm_buf,
				  expected_okm_len,
				  p_test_vector->expected_result,
				  "Incorrect hkdf on extract and expand");

	/* Verify that the next two bytes in buffer are not overwritten. */
	TEST_VECTOR_OVERFLOW_ASSERT(m_hkdf_okm_buf, okm_len,
				    "OKM buffer overflow");

	memset(m_hkdf_okm_buf, 0xFF, sizeof(m_hkdf_okm_buf));

	/* Calculation of the HKDF expand only. */
	err_code = mbedtls_hkdf_expand(p_md_info, m_hkdf_prk_buf, prk_len,
				       m_hkdf_info_buf, info_len,
				       m_hkdf_okm_buf, okm_len);

	LOG_DBG("Error code expand: -0x%04X", -err_code);
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code_expand,
				 err_code);

	/* Verify the generated HKDF output key material. */
	TEST_VECTOR_ASSERT_EQUAL(expected_okm_len, okm_len);
	TEST_VECTOR_MEMCMP_ASSERT(m_hkdf_okm_buf, m_hkdf_expected_okm_buf,
				  expected_okm_len,
				  p_test_vector->expected_result_expand,
				  "Incorrect hkdf on expand");

	/* Verify that the next two bytes in buffer are not overwritten. */
	TEST_VECTOR_OVERFLOW_ASSERT(m_hkdf_okm_buf, okm_len,
				    "OKM buffer overflow on expand");
}

ITEM_REGISTER(test_case_hkdf_data, test_case_t test_hkdf) = {
	.p_test_case_name = "HKDF",
	.setup = hkdf_setup,
	.exec = exec_test_case_hkdf,
	.teardown = teardown_pass,
	.vector_type = TV_HKDF,
	.vectors_start = __start_test_vector_hkdf_data,
	.vectors_stop = __stop_test_vector_hkdf_data,
};

ZTEST_SUITE(test_suite_hkdf, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_suite_hkdf, test_case_hkdf)
{
	hkdf_setup();
	exec_test_case_hkdf();
}
