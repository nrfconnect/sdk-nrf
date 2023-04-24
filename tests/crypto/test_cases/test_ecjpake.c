/*
 * Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * Methodology and test vectors for ECJPAKE taken from mbed TLS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <zephyr/logging/log.h>

#include "common_test.h"
#include <mbedtls/md.h>
#include <mbedtls/ecjpake.h>

/* Setting LOG_LEVEL_DBG might affect time measurements! */
LOG_MODULE_REGISTER(test_ecjpake, LOG_LEVEL_INF);

#define ECJPAKE_SECRET_BUF_SIZE (32)
#define ECJPAKE_PASSWORD_BUF_SIZE (32)
#define ECJPAKE_PRIVKEY_BUF_SIZE (32)
#define ECJPAKE_MSG_BUF_SIZE (332)

extern test_vector_ecjpake_t __start_test_vector_ecjpake_given_data[];
extern test_vector_ecjpake_t __stop_test_vector_ecjpake_given_data[];
extern test_vector_ecjpake_t __start_test_vector_ecjpake_random_data[];
extern test_vector_ecjpake_t __stop_test_vector_ecjpake_random_data[];

static uint8_t m_secret_cli[ECJPAKE_SECRET_BUF_SIZE];
static uint8_t m_secret_srv[ECJPAKE_SECRET_BUF_SIZE];
static uint8_t m_expected_secret[ECJPAKE_SECRET_BUF_SIZE];
static uint8_t m_password[ECJPAKE_PASSWORD_BUF_SIZE];

/* These correspond to ecp point components, see:
 * https://tls.mbed.org/api/structmbedtls__ecjpake__context.html
 */
static uint8_t m_priv_key_cli_1[ECJPAKE_PRIVKEY_BUF_SIZE];
static uint8_t m_priv_key_cli_2[ECJPAKE_PRIVKEY_BUF_SIZE];
static uint8_t m_priv_key_srv_1[ECJPAKE_PRIVKEY_BUF_SIZE];
static uint8_t m_priv_key_srv_2[ECJPAKE_PRIVKEY_BUF_SIZE];

/* These correspond to the different write/read rounds of ecjpake, see:
 * https://tls.mbed.org/api/ecjpake_8h_source.html
 */
static uint8_t m_msg_cli_1[ECJPAKE_MSG_BUF_SIZE];
static uint8_t m_msg_cli_2[ECJPAKE_MSG_BUF_SIZE];
static uint8_t m_msg_srv_1[ECJPAKE_MSG_BUF_SIZE];
static uint8_t m_msg_srv_2[ECJPAKE_MSG_BUF_SIZE];

static size_t secret_len;
static size_t password_len;
static size_t priv_key_cli_1_len;
static size_t priv_key_cli_2_len;
static size_t priv_key_srv_1_len;
static size_t priv_key_srv_2_len;
static size_t msg_cli_1_len;
static size_t msg_cli_2_len;
static size_t msg_srv_1_len;
static size_t msg_srv_2_len;

static test_vector_ecjpake_t *p_test_vector;

void ecjpake_clear_buffers(void);
void unhexify_ecjpake(void);

void ecjpake_clear_buffers(void)
{
	memset(m_secret_cli, 0x00, sizeof(m_secret_cli));
	memset(m_secret_srv, 0x00, sizeof(m_secret_srv));
	memset(m_expected_secret, 0x00, sizeof(m_expected_secret));
	memset(m_password, 0x00, sizeof(m_password));

	memset(m_priv_key_cli_1, 0x00, sizeof(m_priv_key_cli_1));
	memset(m_priv_key_cli_2, 0x00, sizeof(m_priv_key_cli_2));
	memset(m_priv_key_srv_1, 0x00, sizeof(m_priv_key_srv_1));
	memset(m_priv_key_srv_2, 0x00, sizeof(m_priv_key_srv_2));

	memset(m_msg_cli_1, 0x00, sizeof(m_priv_key_cli_1));
	memset(m_msg_cli_2, 0x00, sizeof(m_priv_key_cli_2));
	memset(m_msg_srv_1, 0x00, sizeof(m_priv_key_srv_1));
	memset(m_msg_srv_2, 0x00, sizeof(m_priv_key_srv_2));
}

__attribute__((noinline)) void unhexify_ecjpake(void)
{
	secret_len = hex2bin_safe(p_test_vector->p_expected_shared_secret,
				  m_expected_secret,
				  sizeof(m_expected_secret));
	password_len = hex2bin_safe(p_test_vector->p_password,
				    m_password,
				    sizeof(m_password));

	priv_key_cli_1_len = hex2bin_safe(p_test_vector->p_priv_key_client_1,
					  m_priv_key_cli_1,
					  sizeof(m_priv_key_cli_1));
	priv_key_cli_2_len = hex2bin_safe(p_test_vector->p_priv_key_client_2,
					  m_priv_key_cli_2,
					  sizeof(m_priv_key_cli_2));
	priv_key_srv_1_len = hex2bin_safe(p_test_vector->p_priv_key_server_1,
					  m_priv_key_srv_1,
					  sizeof(m_priv_key_srv_1));
	priv_key_srv_2_len = hex2bin_safe(p_test_vector->p_priv_key_server_2,
					  m_priv_key_srv_2,
					  sizeof(m_priv_key_srv_2));

	msg_cli_1_len = hex2bin_safe(p_test_vector->p_round_message_client_1,
				     m_msg_cli_1,
				     sizeof(m_msg_cli_1));
	msg_cli_2_len = hex2bin_safe(p_test_vector->p_round_message_client_2,
				     m_msg_cli_2,
				     sizeof(m_msg_cli_2));
	msg_srv_1_len = hex2bin_safe(p_test_vector->p_round_message_server_1,
				     m_msg_srv_1,
				     sizeof(m_msg_srv_1));
	msg_srv_2_len = hex2bin_safe(p_test_vector->p_round_message_server_2,
				     m_msg_srv_2,
				     sizeof(m_msg_srv_2));
}

/*
 * Key loading the same way mbedtls ECJPAKE test code does it.
 * Private keys are loaded raw, public keys are then generated via
 * EC multiplication.
 */
#if !defined(MBEDTLS_ECJPAKE_ALT)
static int ecjpake_test_load(mbedtls_ecjpake_context *ctx,
					const unsigned char *xm1, size_t len1,
					const unsigned char *xm2, size_t len2)
{
	int err_code;

	err_code = mbedtls_mpi_read_binary(&ctx->xm1, xm1, len1);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_mpi_read_binary(&ctx->xm2, xm2, len2);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	mbedtls_ecp_mul(&ctx->grp, &ctx->Xm1, &ctx->xm1, &ctx->grp.G, NULL,
			NULL);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	mbedtls_ecp_mul(&ctx->grp, &ctx->Xm2, &ctx->xm2, &ctx->grp.G, NULL,
			NULL);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	return err_code;
}
#else
extern int ecjpake_test_load(mbedtls_ecjpake_context *ctx,
					const unsigned char *xm1, size_t len1,
					const unsigned char *xm2, size_t len2);
#endif

static void ecjpake_given_setup(void)
{
	static int i;

	ecjpake_clear_buffers();
	p_test_vector = ITEM_GET(test_vector_ecjpake_given_data,
				 test_vector_ecjpake_t, i++);
	unhexify_ecjpake();
}

static void ecjpake_random_setup(void)
{
	static int i;

	ecjpake_clear_buffers();
	p_test_vector = ITEM_GET(test_vector_ecjpake_random_data,
				 test_vector_ecjpake_t, i++);
	unhexify_ecjpake();
}

static void ecjpake_ctx_init(mbedtls_ecjpake_context *ctx,
				 mbedtls_ecjpake_role role)
{
	int err_code;

	mbedtls_ecjpake_init(ctx);
	err_code = mbedtls_ecjpake_setup(ctx, role, MBEDTLS_MD_SHA256,
					 MBEDTLS_ECP_DP_SECP256R1, m_password,
					 password_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecjpake_check(ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);
}

void exec_test_case_ecjpake_given(void)
{
	int err_code;
	size_t len_ss_cli;
	size_t len_ss_srv;

	mbedtls_ecjpake_context ctx_client;
	mbedtls_ecjpake_context ctx_server;

	ecjpake_ctx_init(&ctx_client, MBEDTLS_ECJPAKE_CLIENT);
	ecjpake_ctx_init(&ctx_server, MBEDTLS_ECJPAKE_SERVER);

	err_code = ecjpake_test_load(&ctx_client, m_priv_key_cli_1,
				     priv_key_cli_1_len, m_priv_key_cli_2,
				     priv_key_cli_2_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = ecjpake_test_load(&ctx_server, m_priv_key_srv_1,
				     priv_key_srv_1_len, m_priv_key_srv_2,
				     priv_key_srv_2_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	start_time_measurement();
	/* Round one. */
	err_code = mbedtls_ecjpake_read_round_one(&ctx_server, m_msg_cli_1,
						  msg_cli_1_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecjpake_read_round_one(&ctx_client, m_msg_srv_1,
						  msg_srv_1_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/*
	The keys are already given; go straight to reading next round.
	*/

	/* Round two. */
	err_code = mbedtls_ecjpake_read_round_two(&ctx_client, m_msg_srv_2,
						  msg_srv_2_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecjpake_read_round_two(&ctx_server, m_msg_cli_2,
						  msg_cli_2_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Derive secrets. */
	err_code = mbedtls_ecjpake_derive_secret(
		&ctx_client, m_secret_cli, sizeof(m_secret_cli), &len_ss_cli,
		drbg_random, &drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecjpake_derive_secret(
		&ctx_server, m_secret_srv, sizeof(m_secret_srv), &len_ss_srv,
		drbg_random, &drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);
	stop_time_measurement();

	TEST_VECTOR_ASSERT_EQUAL(secret_len, len_ss_cli);
	TEST_VECTOR_ASSERT_EQUAL(secret_len, len_ss_srv);

	TEST_VECTOR_MEMCMP_ASSERT(
		m_secret_cli, m_expected_secret, secret_len, EXPECTED_TO_PASS,
		"client secret should be equal to expected secret");

	TEST_VECTOR_MEMCMP_ASSERT(
		m_secret_srv, m_expected_secret, secret_len, EXPECTED_TO_PASS,
		"server secret should be equal to expected secret");

	mbedtls_ecjpake_free(&ctx_client);
	mbedtls_ecjpake_free(&ctx_server);
}

void exec_test_case_ecjpake_random(void)
{
	int err_code;
	size_t len;
	size_t len_ss_cli;
	size_t len_ss_srv;

	mbedtls_ecjpake_context ctx_client;
	mbedtls_ecjpake_context ctx_server;

	ecjpake_ctx_init(&ctx_client, MBEDTLS_ECJPAKE_CLIENT);
	ecjpake_ctx_init(&ctx_server, MBEDTLS_ECJPAKE_SERVER);

	start_time_measurement();
	/* Round one. */
	err_code = mbedtls_ecjpake_write_round_one(&ctx_client, m_msg_cli_1,
						   sizeof(m_msg_cli_1), &len,
						   drbg_random,
						   &drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code =
		mbedtls_ecjpake_read_round_one(&ctx_server, m_msg_cli_1, len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecjpake_write_round_one(&ctx_server, m_msg_srv_1,
						   sizeof(m_msg_srv_1), &len,
						   drbg_random,
						   &drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code =
		mbedtls_ecjpake_read_round_one(&ctx_client, m_msg_srv_1, len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Round two. */
	err_code = mbedtls_ecjpake_write_round_two(&ctx_server, m_msg_srv_2,
						   sizeof(m_msg_srv_2), &len,
						   drbg_random,
						   &drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code =
		mbedtls_ecjpake_read_round_two(&ctx_client, m_msg_srv_2, len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecjpake_write_round_two(&ctx_client, m_msg_cli_2,
						   sizeof(m_msg_cli_2), &len,
						   drbg_random,
						   &drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code =
		mbedtls_ecjpake_read_round_two(&ctx_server, m_msg_cli_2, len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Derive secrets. */
	err_code = mbedtls_ecjpake_derive_secret(
		&ctx_client, m_secret_cli, sizeof(m_secret_cli), &len_ss_cli,
		drbg_random, &drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecjpake_derive_secret(
		&ctx_server, m_secret_srv, sizeof(m_secret_srv), &len_ss_srv,
		drbg_random, &drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);
	stop_time_measurement();

	TEST_VECTOR_ASSERT_EQUAL(len_ss_cli, len_ss_srv);

	TEST_VECTOR_MEMCMP_ASSERT(m_secret_cli, m_secret_srv, secret_len,
				  EXPECTED_TO_PASS, "secrets should be equal");

	mbedtls_ecjpake_free(&ctx_client);
	mbedtls_ecjpake_free(&ctx_server);
}

ITEM_REGISTER(test_case_ecjpake_data, test_case_t test_ecjpake_given) = {
	.p_test_case_name = "ECJPAKE given",
	.setup = ecjpake_given_setup,
	.exec = exec_test_case_ecjpake_given,
	.teardown = teardown_pass,
	.vector_type = TV_ECJPAKE,
	.vectors_start = __start_test_vector_ecjpake_given_data,
	.vectors_stop = __stop_test_vector_ecjpake_given_data,
};

ITEM_REGISTER(test_case_ecjpake_data, test_case_t test_ecjpake_random) = {
	.p_test_case_name = "ECJPAKE random",
	.setup = ecjpake_random_setup,
	.exec = exec_test_case_ecjpake_random,
	.teardown = teardown_pass,
	.vector_type = TV_ECJPAKE,
	.vectors_start = __start_test_vector_ecjpake_random_data,
	.vectors_stop = __stop_test_vector_ecjpake_random_data,
};

ZTEST_SUITE(test_suite_ecjpake, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_suite_ecjpake, test_case_ecjpake_given)
{
	ecjpake_given_setup();
	exec_test_case_ecjpake_given();
}

ZTEST(test_suite_ecjpake, test_case_ecjpake_random)
{
	ecjpake_random_setup();
	exec_test_case_ecjpake_random();
}
