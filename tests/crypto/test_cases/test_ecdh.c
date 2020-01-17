/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <sys/byteorder.h>
#include <logging/log.h>

#include "common_test.h"
#include <mbedtls/ecdh.h>

/* Setting LOG_LEVEL_DBG might affect time measurements! */
LOG_MODULE_REGISTER(test_ecdh, LOG_LEVEL_INF);

extern test_vector_ecdh_t __start_test_vector_ecdh_data_random[];
extern test_vector_ecdh_t __stop_test_vector_ecdh_data_random[];

extern test_vector_ecdh_t __start_test_vector_ecdh_data_deterministic_simple[];
extern test_vector_ecdh_t __stop_test_vector_ecdh_data_deterministic_simple[];

extern test_vector_ecdh_t __start_test_vector_ecdh_data_deterministic_full[];
extern test_vector_ecdh_t __stop_test_vector_ecdh_data_deterministic_full[];

/* TODO: Possibly tune buffers which have lower size requirements. */
#define ECDH_BUF_SIZE (512)

static uint8_t m_ecdh_initiater_priv_key_buf[ECDH_BUF_SIZE];
static uint8_t m_ecdh_responder_priv_key_buf[ECDH_BUF_SIZE];
static uint8_t m_ecdh_initiater_publ_key_buf[ECDH_BUF_SIZE];
static uint8_t m_ecdh_responder_publ_key_buf[ECDH_BUF_SIZE];
static uint8_t m_ecdh_initiator_ss_buf[ECDH_BUF_SIZE];
static uint8_t m_ecdh_responder_ss_buf[ECDH_BUF_SIZE];
static uint8_t m_ecdh_expected_ss_buf[ECDH_BUF_SIZE];

static test_vector_ecdh_t *p_test_vector;
static size_t expected_ss_len;

void ecdh_clear_buffers(void)
{
	memset(m_ecdh_initiater_priv_key_buf, 0x00,
	       sizeof(m_ecdh_initiater_priv_key_buf));
	memset(m_ecdh_responder_priv_key_buf, 0x00,
	       sizeof(m_ecdh_responder_priv_key_buf));
	memset(m_ecdh_initiater_publ_key_buf, 0x00,
	       sizeof(m_ecdh_initiater_publ_key_buf));
	memset(m_ecdh_responder_publ_key_buf, 0x00,
	       sizeof(m_ecdh_responder_publ_key_buf));
	memset(m_ecdh_initiator_ss_buf, 0x00, sizeof(m_ecdh_initiator_ss_buf));
	memset(m_ecdh_responder_ss_buf, 0x00, sizeof(m_ecdh_responder_ss_buf));
	memset(m_ecdh_expected_ss_buf, 0x00, sizeof(m_ecdh_expected_ss_buf));
}

__attribute__((noinline)) static void unhexify_ecdh(void)
{
	expected_ss_len =
		hex2bin(p_test_vector->p_expected_shared_secret,
			strlen(p_test_vector->p_expected_shared_secret),
			m_ecdh_expected_ss_buf,
			strlen(p_test_vector->p_expected_shared_secret));
}

void ecdh_setup_random(void)
{
	ecdh_clear_buffers();
	static int i;
	p_test_vector =
		ITEM_GET(test_vector_ecdh_data_random, test_vector_ecdh_t, i++);
	unhexify_ecdh();
}

void ecdh_setup_deterministic_simple(void)
{
	ecdh_clear_buffers();
	static int i;
	p_test_vector = ITEM_GET(test_vector_ecdh_data_deterministic_simple,
				 test_vector_ecdh_t, i++);
	unhexify_ecdh();
}

void ecdh_setup_deterministic_full(void)
{
	ecdh_clear_buffers();
	static int i;
	p_test_vector = ITEM_GET(test_vector_ecdh_data_deterministic_full,
				 test_vector_ecdh_t, i++);
	unhexify_ecdh();
}

/**@brief Function for executing ECDH for initiator and repsonder by
 * using random generated keys.
 */
void exec_test_case_ecdh_random(void)
{
	int err_code_initiator = -1;
	int err_code_responder = -1;

	mbedtls_ecdh_context initiator_ctx;
	mbedtls_ecdh_context responder_ctx;

	mbedtls_ecdh_init(&initiator_ctx);
	mbedtls_ecdh_init(&responder_ctx);

	err_code_initiator = mbedtls_ecp_group_load(&initiator_ctx.grp,
						    p_test_vector->curve_type);
	TEST_VECTOR_ASSERT_EQUAL(err_code_initiator, 0);

	err_code_responder = mbedtls_ecp_group_load(&responder_ctx.grp,
						    p_test_vector->curve_type);
	TEST_VECTOR_ASSERT_EQUAL(err_code_responder, 0);

	err_code_initiator =
		mbedtls_ecdh_gen_public(&initiator_ctx.grp, &initiator_ctx.d,
					&initiator_ctx.Q,
					mbedtls_ctr_drbg_random, &ctr_drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code_initiator);

	err_code_responder =
		mbedtls_ecdh_gen_public(&responder_ctx.grp, &responder_ctx.d,
					&responder_ctx.Q,
					mbedtls_ctr_drbg_random, &ctr_drbg_ctx);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code_responder);

	start_time_measurement();

	err_code_initiator = mbedtls_ecdh_compute_shared(
		&initiator_ctx.grp, &initiator_ctx.z, &responder_ctx.Q,
		&initiator_ctx.d, mbedtls_ctr_drbg_random, &ctr_drbg_ctx);
	err_code_responder = mbedtls_ecdh_compute_shared(
		&responder_ctx.grp, &responder_ctx.z, &initiator_ctx.Q,
		&responder_ctx.d, mbedtls_ctr_drbg_random, &ctr_drbg_ctx);

	stop_time_measurement();

	TEST_VECTOR_ASSERT_EQUAL(0, err_code_initiator);

	TEST_VECTOR_ASSERT_EQUAL(0, err_code_responder);

	size_t len_ss_initiator = mbedtls_mpi_size(&initiator_ctx.z);
	size_t len_ss_responder = mbedtls_mpi_size(&responder_ctx.z);
	TEST_VECTOR_ASSERT_EQUAL(len_ss_initiator, len_ss_responder);

	TEST_VECTOR_MEMCMP_ASSERT(initiator_ctx.z.p, responder_ctx.z.p,
				  len_ss_initiator,
				  p_test_vector->expected_result,
				  "Shared secret comparison unexpected result");

	mbedtls_ecdh_free(&initiator_ctx);
	mbedtls_ecdh_free(&responder_ctx);
}

static int curve25519_ctx_fixup(mbedtls_ecdh_context *ctx)
{
	/* Set certain bits to predefined values */
	int err_code = mbedtls_mpi_set_bit(&ctx->d, 0, 0);

	TEST_VECTOR_ASSERT_EQUAL(0, err_code);
	err_code |= mbedtls_mpi_set_bit(&ctx->d, 1, 0);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);
	err_code |= mbedtls_mpi_set_bit(&ctx->d, 2, 0);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);
	err_code |= mbedtls_mpi_set_bit(&ctx->d, 254, 1);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);
	err_code |= mbedtls_mpi_set_bit(&ctx->d, 255, 0);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	mbedtls_mpi_lset(&ctx->Q.Z, 1);

	return err_code;
}

/**@brief Function for executing deterministic ECDH for initiator and responder.
 */
void exec_test_case_ecdh_deterministic_full(void)
{
	int err_code = -1;

	size_t initiator_ss_len;
	size_t responder_ss_len;

	LOG_DBG("Test vector pointer: %p", (void *)p_test_vector);

	mbedtls_ecdh_context initiator_ctx;
	mbedtls_ecdh_context responder_ctx;

	mbedtls_ecdh_init(&initiator_ctx);
	mbedtls_ecdh_init(&responder_ctx);

	err_code = mbedtls_ecp_group_load(&initiator_ctx.grp,
					  p_test_vector->curve_type);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecp_group_load(&responder_ctx.grp,
					  p_test_vector->curve_type);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	const char *initiator_publ_y;
	const char *responder_publ_y;
	if (p_test_vector->curve_type == MBEDTLS_ECP_DP_CURVE25519) {
		/* This curve wants Q.X and Q.Y concatenated into Q.X. */
		/* This is done in the test vector source. */
		/* As the Y component is now within Q.X, Q.Y should be empty. */
		initiator_publ_y = "";
		responder_publ_y = "";
	} else {
		initiator_publ_y = p_test_vector->p_initiator_publ_y;
		responder_publ_y = p_test_vector->p_responder_publ_y;
	}

	/* Prepare initator public and private datatypes. */
	err_code =
		mbedtls_ecp_point_read_string(&initiator_ctx.Q, 16,
					      p_test_vector->p_initiator_publ_x,
					      initiator_publ_y);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_mpi_read_string(&initiator_ctx.d, 16,
					   p_test_vector->p_initiator_priv);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Prepare responder public and private datatypes. */
	err_code =
		mbedtls_ecp_point_read_string(&responder_ctx.Q, 16,
					      p_test_vector->p_responder_publ_x,
					      responder_publ_y);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_mpi_read_string(&responder_ctx.d, 16,
					   p_test_vector->p_responder_priv);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	if (p_test_vector->curve_type == MBEDTLS_ECP_DP_CURVE25519) {
		err_code = curve25519_ctx_fixup(&initiator_ctx);
		TEST_VECTOR_ASSERT_EQUAL(0, err_code);
		err_code = curve25519_ctx_fixup(&responder_ctx);
		TEST_VECTOR_ASSERT_EQUAL(0, err_code);
	}

	/* Validate keys if applicable. */
	if (p_test_vector->expected_result == EXPECTED_TO_PASS) {
		err_code = mbedtls_ecp_check_pubkey(&initiator_ctx.grp,
						    &initiator_ctx.Q);
		LOG_DBG("Error code pubkey check: -0x%04X", -err_code);
		TEST_VECTOR_ASSERT_EQUAL(0, err_code);

		err_code = mbedtls_ecp_check_privkey(&initiator_ctx.grp,
						     &initiator_ctx.d);
		LOG_DBG("Error code privkey check: -0x%04X", -err_code);
		TEST_VECTOR_ASSERT_EQUAL(0, err_code);

		err_code = mbedtls_ecp_check_pubkey(&responder_ctx.grp,
						    &responder_ctx.Q);
		LOG_DBG("Error code pubkey check: -0x%04X", -err_code);
		TEST_VECTOR_ASSERT_EQUAL(0, err_code);

		err_code = mbedtls_ecp_check_privkey(&responder_ctx.grp,
						     &responder_ctx.d);
		LOG_DBG("Error code privkey check: -0x%04X", -err_code);
		TEST_VECTOR_ASSERT_EQUAL(0, err_code);
	}

	expected_ss_len =
		hex2bin(p_test_vector->p_expected_shared_secret,
			strlen(p_test_vector->p_expected_shared_secret),
			m_ecdh_expected_ss_buf,
			strlen(p_test_vector->p_expected_shared_secret));

	/* Execute ECDH on initiator side. */
	start_time_measurement();
	err_code =
		mbedtls_ecdh_compute_shared(&initiator_ctx.grp,
					    &initiator_ctx.z, &responder_ctx.Q,
					    &initiator_ctx.d, NULL, NULL);
	stop_time_measurement();

	LOG_DBG("Error code compute shared: -0x%04X", -err_code);
	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	initiator_ss_len = mbedtls_mpi_size(&initiator_ctx.z);

	err_code = mbedtls_mpi_write_binary(
		&initiator_ctx.z, m_ecdh_initiator_ss_buf, expected_ss_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Execute ECDH on responder side. */
	err_code =
		mbedtls_ecdh_compute_shared(&responder_ctx.grp,
					    &responder_ctx.z, &initiator_ctx.Q,
					    &responder_ctx.d, NULL, NULL);

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	responder_ss_len = mbedtls_mpi_size(&responder_ctx.z);
	err_code = mbedtls_mpi_write_binary(
		&responder_ctx.z, m_ecdh_responder_ss_buf, expected_ss_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Verify length of generated shared secrets. */
	TEST_VECTOR_ASSERT_EQUAL(expected_ss_len, initiator_ss_len);
	TEST_VECTOR_ASSERT_EQUAL(expected_ss_len, responder_ss_len);

	/* Compare generated initiator shared secret to responder shared secret. */
	TEST_VECTOR_MEMCMP_ASSERT(
		m_ecdh_initiator_ss_buf, m_ecdh_responder_ss_buf,
		initiator_ss_len, p_test_vector->expected_result,
		"Shared secret mismatch between responder and initiator");

	/* Compare generated shared secret to expected shared secret. */
	TEST_VECTOR_MEMCMP_ASSERT(
		m_ecdh_responder_ss_buf, m_ecdh_expected_ss_buf,
		expected_ss_len, p_test_vector->expected_result,
		"Shared secret mismatch between responder and expected");

	/* Free the generated resources. */
	mbedtls_ecdh_free(&initiator_ctx);
	mbedtls_ecdh_free(&responder_ctx);
}

/**@brief Function for executing deterministic ECDH for responder.
 */
void exec_test_case_ecdh_deterministic(void)
{
	int err_code = -1;

	mbedtls_ecdh_context initiator_ctx;
	mbedtls_ecdh_context responder_ctx;

	mbedtls_ecdh_init(&initiator_ctx);
	mbedtls_ecdh_init(&responder_ctx);

	err_code = mbedtls_ecp_group_load(&initiator_ctx.grp,
					  p_test_vector->curve_type);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	err_code = mbedtls_ecp_group_load(&responder_ctx.grp,
					  p_test_vector->curve_type);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Prepare initiator public key. */
	err_code = mbedtls_ecp_point_read_string(
		&initiator_ctx.Q, 16, p_test_vector->p_initiator_publ_x,
		p_test_vector->p_initiator_publ_y);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Prepare responder private key. */
	err_code = mbedtls_mpi_read_string(&responder_ctx.d, 16,
					   p_test_vector->p_responder_priv);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	/* Validate keys if applicable. */
	if (p_test_vector->expected_result == EXPECTED_TO_PASS) {
		err_code = mbedtls_ecp_check_pubkey(&initiator_ctx.grp,
						    &initiator_ctx.Q);
		LOG_DBG("Error code pubkey check: -0x%04X", -err_code);
		TEST_VECTOR_ASSERT_EQUAL(0, err_code);

		err_code = mbedtls_ecp_check_privkey(&responder_ctx.grp,
						     &responder_ctx.d);
		LOG_DBG("Error code privkey check: -0x%04X", -err_code);
		TEST_VECTOR_ASSERT_EQUAL(0, err_code);
	}

	start_time_measurement();
	err_code =
		mbedtls_ecdh_compute_shared(&responder_ctx.grp,
					    &responder_ctx.z, &initiator_ctx.Q,
					    &responder_ctx.d, NULL, NULL);
	stop_time_measurement();

	LOG_DBG("Error code ss computation: -0x%04X", -err_code);
	LOG_DBG("Ss size expected: %d, actual: %d", expected_ss_len,
		mbedtls_mpi_size(&responder_ctx.z));

	TEST_VECTOR_ASSERT_EQUAL(p_test_vector->expected_err_code, err_code);

	err_code = mbedtls_mpi_write_binary(
		&responder_ctx.z, m_ecdh_responder_ss_buf, expected_ss_len);
	TEST_VECTOR_ASSERT_EQUAL(0, err_code);

	TEST_VECTOR_MEMCMP_ASSERT(m_ecdh_responder_ss_buf,
				  m_ecdh_expected_ss_buf, expected_ss_len,
				  p_test_vector->expected_result,
				  "Shared secret not as expected");

	/* Free the generated resources. */
	mbedtls_ecdh_free(&initiator_ctx);
	mbedtls_ecdh_free(&responder_ctx);
}

/** @brief  Macro for registering the ECDH test case by using section variables.
 */
ITEM_REGISTER(test_case_ecdh_data, test_case_t test_ecdh) = {
	.p_test_case_name = "ECDH random",
	.setup = ecdh_setup_random,
	.exec = exec_test_case_ecdh_random,
	.teardown = teardown_pass,
	.vector_type = TV_ECDH,
	.vectors_start = __start_test_vector_ecdh_data_random,
	.vectors_stop = __stop_test_vector_ecdh_data_random,
};

/** @brief  Macro for registering the ECDH test case by using section variables.
 */
ITEM_REGISTER(test_case_ecdh_data, test_case_t test_ecdh_det) = {
	.p_test_case_name = "ECDH deterministic",
	.setup = ecdh_setup_deterministic_simple,
	.exec = exec_test_case_ecdh_deterministic,
	.teardown = teardown_pass,
	.vector_type = TV_ECDH,
	.vectors_start = __start_test_vector_ecdh_data_deterministic_simple,
	.vectors_stop = __stop_test_vector_ecdh_data_deterministic_simple,
};

/** @brief  Macro for registering the ECDH test case by using section variables.
 */
ITEM_REGISTER(test_case_ecdh_data, test_case_t test_ecdh_det_full) = {
	.p_test_case_name = "ECDH deterministic full",
	.setup = ecdh_setup_deterministic_full,
	.exec = exec_test_case_ecdh_deterministic_full,
	.teardown = teardown_pass,
	.vector_type = TV_ECDH,
	.vectors_start = __start_test_vector_ecdh_data_deterministic_full,
	.vectors_stop = __stop_test_vector_ecdh_data_deterministic_full,
};
