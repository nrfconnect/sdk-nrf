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

#include <mbedtls/ecdsa.h>

/* Setting LOG_LEVEL_DBG might affect time measurements! */
LOG_MODULE_REGISTER(test_ecdsa, LOG_LEVEL_INF);

extern test_vector_ecdsa_verify_t __start_test_vector_ecdsa_verify_data[];
extern test_vector_ecdsa_verify_t __stop_test_vector_ecdsa_verify_data[];
extern test_vector_ecdsa_sign_t __start_test_vector_ecdsa_sign_data[];
extern test_vector_ecdsa_sign_t __stop_test_vector_ecdsa_sign_data[];
extern test_vector_ecdsa_random_t __start_test_vector_ecdsa_random_data[];
extern test_vector_ecdsa_random_t __stop_test_vector_ecdsa_random_data[];

/* Should be equal to SHA512 digest size */
#define ECDSA_MAX_INPUT_SIZE (64)

static uint8_t m_ecdsa_input_buf[ECDSA_MAX_INPUT_SIZE];

static test_vector_ecdsa_verify_t *p_test_vector_verify;
static test_vector_ecdsa_sign_t *p_test_vector_sign;
static test_vector_ecdsa_random_t *p_test_vector_random;

static size_t hash_len;

void ecdsa_clear_buffers(void);
void unhexify_ecdsa_verify(void);
void unhexify_ecdsa_sign(void);
void unhexify_ecdsa_random(void);

/**@brief Function for running the test setup.
 */
static void ecdsa_setup_verify(void)
{
	static int i;

	ecdsa_clear_buffers();
	p_test_vector_verify = ITEM_GET(test_vector_ecdsa_verify_data,
					test_vector_ecdsa_verify_t, i++);
	unhexify_ecdsa_verify();
}

static void ecdsa_setup_sign(void)
{
	static int i;

	ecdsa_clear_buffers();
	p_test_vector_sign = ITEM_GET(test_vector_ecdsa_sign_data,
				      test_vector_ecdsa_sign_t, i++);
	unhexify_ecdsa_sign();
}

static void ecdsa_setup_random(void)
{
	static int i;

	ecdsa_clear_buffers();
	p_test_vector_random = ITEM_GET(test_vector_ecdsa_random_data,
					test_vector_ecdsa_random_t, i++);
	unhexify_ecdsa_random();
}

__attribute__((noinline)) void unhexify_ecdsa_verify(void)
{
	hash_len = hex2bin_safe(p_test_vector_verify->p_input,
				m_ecdsa_input_buf,
				sizeof(m_ecdsa_input_buf));
}

__attribute__((noinline)) void unhexify_ecdsa_sign(void)
{
	hash_len = hex2bin_safe(p_test_vector_sign->p_input,
				m_ecdsa_input_buf,
				sizeof(m_ecdsa_input_buf));
}

__attribute__((noinline)) void unhexify_ecdsa_random(void)
{
	hash_len = hex2bin_safe(p_test_vector_random->p_input,
				m_ecdsa_input_buf,
				sizeof(m_ecdsa_input_buf));
}

void ecdsa_clear_buffers(void)
{
	memset(m_ecdsa_input_buf, 0x00, sizeof(m_ecdsa_input_buf));
}

/**@brief Function for the ECDSA sign test execution.
 */
void exec_test_case_ecdsa_sign(void)
{
	int err_code = -1;

	/* Prepare signer context. */
	mbedtls_ecdsa_context ctx_sign;
	mbedtls_ecdsa_init(&ctx_sign);

	err_code = mbedtls_ecp_group_load(&ctx_sign.grp,
					  p_test_vector_sign->curve_type);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Get public key. */
	err_code = mbedtls_ecp_point_read_string(&ctx_sign.Q, 16,
						 p_test_vector_sign->p_qx,
						 p_test_vector_sign->p_qy);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Get private key. */
	err_code = mbedtls_mpi_read_string(&ctx_sign.d, 16,
					   p_test_vector_sign->p_x);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Verify keys. */
	err_code = mbedtls_ecp_check_pubkey(&ctx_sign.grp, &ctx_sign.Q);
	LOG_DBG("Error code pubkey check: 0x%04X", err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecp_check_privkey(&ctx_sign.grp, &ctx_sign.d);
	LOG_DBG("Error code privkey check: 0x%04X", err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Prepare and generate the ECDSA signature. */
	/* Note: The contexts do not contain these (as is the case for e.g. Q), so simply share them here. */
	mbedtls_mpi r;
	mbedtls_mpi s;
	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	start_time_measurement();

	err_code = mbedtls_ecdsa_sign(&ctx_sign.grp, &r, &s, &ctx_sign.d,
				      m_ecdsa_input_buf, hash_len,
				      drbg_random, &drbg_ctx);

	stop_time_measurement();

	/* Verify sign. */
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector_sign->expected_sign_err_code,
				 err_code);

	/* Prepare verification context. */
	mbedtls_ecdsa_context ctx_verify;
	mbedtls_ecdsa_init(&ctx_verify);

	/* Transfer public EC information. */
	err_code = mbedtls_ecp_group_copy(&ctx_verify.grp, &ctx_sign.grp);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Transfer public key. */
	err_code = mbedtls_ecp_copy(&ctx_verify.Q, &ctx_sign.Q);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Verify the generated ECDSA signature by running the ECDSA verify. */
	err_code = mbedtls_ecdsa_verify(&ctx_verify.grp, m_ecdsa_input_buf,
					hash_len, &ctx_verify.Q, &r, &s);

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector_sign->expected_verify_err_code,
				 err_code);

	/* Free resources. */
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&s);
	mbedtls_ecdsa_free(&ctx_sign);
	mbedtls_ecdsa_free(&ctx_verify);
}

/**@brief Function for the ECDSA verify test execution.
 */
void exec_test_case_ecdsa_verify(void)
{
	int err_code = -1;

	mbedtls_ecdsa_context ctx_verify;
	mbedtls_ecdsa_init(&ctx_verify);

	err_code = mbedtls_ecp_group_load(&ctx_verify.grp,
					  p_test_vector_verify->curve_type);
	if (err_code != 0) {
		LOG_WRN("ecp group load error code: -0x%02X, curve type: %d",
			-err_code, p_test_vector_verify->curve_type);
	}

	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Get public key. */
	err_code = mbedtls_ecp_point_read_string(&ctx_verify.Q, 16,
						 p_test_vector_verify->p_qx,
						 p_test_vector_verify->p_qy);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* If not expected to succeed, keys might be bad on purpose. */
	if (p_test_vector_verify->expected_err_code == 0) {
		/* Verify key. */
		err_code = mbedtls_ecp_check_pubkey(&ctx_verify.grp,
						    &ctx_verify.Q);
		LOG_DBG("Error code pubkey check: 0x%04X", err_code);
		TEST_VECTOR_ASSERT_EQUAL(0, err_code);
	}

	/* Prepare and generate the ECDSA signature. */
	mbedtls_mpi r;
	mbedtls_mpi s;
	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	err_code = mbedtls_mpi_read_string(&r, 16, p_test_vector_verify->p_r);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_mpi_read_string(&s, 16, p_test_vector_verify->p_s);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Verify the ECDSA signature by running the ECDSA verify. */
	start_time_measurement();
	err_code = mbedtls_ecdsa_verify(&ctx_verify.grp, m_ecdsa_input_buf,
					hash_len, &ctx_verify.Q, &r, &s);
	stop_time_measurement();

	/* Verify the verification. */
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector_verify->expected_err_code,
				 err_code);

	/* Free the generated key. */
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&s);
	mbedtls_ecdsa_free(&ctx_verify);
}

/**@brief Function for the ECDSA random test execution.
 */
void exec_test_case_ecdsa_random(void)
{
	int err_code = -1;


	/* Prepare signer context. */
	mbedtls_ecdsa_context ctx_sign;
	mbedtls_ecdsa_init(&ctx_sign);

	/* Create a ECDSA key pair */
	err_code = mbedtls_ecdsa_genkey(&ctx_sign,
					p_test_vector_random->curve_type,
					drbg_random, &drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Verify keys. */
	err_code = mbedtls_ecp_check_pubkey(&ctx_sign.grp, &ctx_sign.Q);
	LOG_DBG("Error code pubkey check: 0x%04X", err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecp_check_privkey(&ctx_sign.grp, &ctx_sign.d);
	LOG_DBG("Error code privkey check: 0x%04X", err_code);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Prepare and generate the ECDSA signature. */
	/* Note: The contexts do not contain these (as is the case for e.g. Q), so simply share them here. */
	mbedtls_mpi r;
	mbedtls_mpi s;
	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	start_time_measurement();
	err_code = mbedtls_ecdsa_sign(&ctx_sign.grp, &r, &s, &ctx_sign.d,
				      m_ecdsa_input_buf, hash_len,
				      drbg_random, &drbg_ctx);
	stop_time_measurement();

	/* Prepare verification context. */
	mbedtls_ecdsa_context ctx_verify;
	mbedtls_ecdsa_init(&ctx_verify);

	/* Transfer public EC information. */
	err_code = mbedtls_ecp_group_copy(&ctx_verify.grp, &ctx_sign.grp);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Transfer public key. */
	err_code = mbedtls_ecp_copy(&ctx_verify.Q, &ctx_sign.Q);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Verify the generated ECDSA signature by running the ECDSA verify. */
	err_code = mbedtls_ecdsa_verify(&ctx_verify.grp, m_ecdsa_input_buf,
					hash_len, &ctx_verify.Q, &r, &s);

	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Modify the signature value to induce a verification failure. */
	err_code = mbedtls_mpi_set_bit(&r, 0, !mbedtls_mpi_get_bit(&r, 0));
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecdsa_verify(&ctx_verify.grp, m_ecdsa_input_buf,
					hash_len, &ctx_verify.Q, &r, &s);

	/* Verify failure. */
	TEST_VECTOR_ASSERT_NOT_EQUAL(0, err_code);


	/* Free resources. */
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&s);
	mbedtls_ecdsa_free(&ctx_sign);
	mbedtls_ecdsa_free(&ctx_verify);
}

/** @brief  Macro for registering the ECDSA sign test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_data",
 *              which is initialized by main.
 */
ITEM_REGISTER(test_case_ecdsa_data, test_case_t test_ecdsa_sign) = {
	.p_test_case_name = "ECDSA Sign",
	.setup = ecdsa_setup_sign,
	.exec = exec_test_case_ecdsa_sign,
	.teardown = teardown_pass,
	.vector_type = TV_ECDSA_SIGN,
	.vectors_start = __start_test_vector_ecdsa_sign_data,
	.vectors_stop = __stop_test_vector_ecdsa_sign_data,
};

/** @brief  Macro for registering the ECDSA verify test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_data",
 *              which is initialized by main.
 */
ITEM_REGISTER(test_case_ecdsa_data, test_case_t test_ecdsa_verify) = {
	.p_test_case_name = "ECDSA Verify",
	.setup = ecdsa_setup_verify,
	.exec = exec_test_case_ecdsa_verify,
	.teardown = teardown_pass,
	.vector_type = TV_ECDSA_VERIFY,
	.vectors_start = __start_test_vector_ecdsa_verify_data,
	.vectors_stop = __stop_test_vector_ecdsa_verify_data,
};

/** @brief  Macro for registering the ECDSA random test case by using section variables.
 *
 * @details     This macro places a variable in a section named "test_case_data",
 *              which is initialized by main.
 */
ITEM_REGISTER(test_case_ecdsa_data, test_case_t test_ecdsa_random) = {
	.p_test_case_name = "ECDSA Random",
	.setup = ecdsa_setup_random,
	.exec = exec_test_case_ecdsa_random,
	.teardown = teardown_pass,
	.vector_type = TV_ECDSA_RANDOM,
	.vectors_start = __start_test_vector_ecdsa_random_data,
	.vectors_stop = __stop_test_vector_ecdsa_random_data,
};
