/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util_macro.h>
#include <psa/crypto.h>
#include <string.h>

static const uint8_t test_context[] = "sample context";

/*
 * Known-Answer Test (KAT) vectors for SHAKE128 and SHAKE256.
 *
 * Empty-input vectors
 *   Source: NIST FIPS 202, Appendix B, August 2015.
 *           https://doi.org/10.6028/NIST.FIPS.202
 *
 * Non-empty input vectors (Len=24, 3-byte message)
 *   Source: NIST CAVP, "SHA-3 XOF Test Vectors for Byte-Oriented Output",
 *           file SHAKE128ShortMsg.rsp / SHAKE256ShortMsg.rsp.
 *           https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/documents/sha3/shakebytetestvectors.zip
 */

/* SHAKE128(empty, Outputlen=256 bits) — FIPS 202 Appendix B.1 */
static const uint8_t shake128_empty_out[32] = {
	0x7f, 0x9c, 0x2b, 0xa4, 0xe8, 0x8f, 0x82, 0x7d,
	0x61, 0x60, 0x45, 0x50, 0x76, 0x05, 0x85, 0x3e,
	0xd7, 0x3b, 0x80, 0x93, 0xf6, 0xef, 0xbc, 0x88,
	0xeb, 0x1a, 0x6e, 0xac, 0xfa, 0x66, 0xef, 0x26
};

/*
 * SHAKE128(Msg=0x1b3b6e, Outputlen=128 bits) — CAVP SHAKE128ShortMsg.rsp,
 * Len=24 (3 bytes), Outputlen=128.
 */
static const uint8_t shake128_msg_in[3] = {
	0x1b, 0x3b, 0x6e
};

static const uint8_t shake128_msg_out[16] = {
	0xd7, 0x33, 0x54, 0x97, 0xe4, 0xcd, 0x36, 0x66,
	0x88, 0x5e, 0xdb, 0xb0, 0x82, 0x4d, 0x7a, 0x75
};

/* SHAKE256(empty, Outputlen=512 bits) — FIPS 202 Appendix B.2 */
static const uint8_t shake256_empty_out[64] = {
	0x46, 0xb9, 0xdd, 0x2b, 0x0b, 0xa8, 0x8d, 0x13,
	0x23, 0x3b, 0x3f, 0xeb, 0x74, 0x3e, 0xeb, 0x24,
	0x3f, 0xcd, 0x52, 0xea, 0x62, 0xb8, 0x1b, 0x82,
	0xb5, 0x0c, 0x27, 0x64, 0x6e, 0xd5, 0x76, 0x2f,
	0xd7, 0x5d, 0xc4, 0xdd, 0xd8, 0xc0, 0xf2, 0x00,
	0xcb, 0x05, 0x01, 0x9d, 0x67, 0xb5, 0x92, 0xf6,
	0xfc, 0x82, 0x1c, 0x49, 0x47, 0x9a, 0xb4, 0x86,
	0x40, 0x29, 0x2e, 0xac, 0xb3, 0xb7, 0xc4, 0xbe
};

/*
 * SHAKE256(Msg=0x21eda6, Outputlen=256 bits) — CAVP SHAKE256ShortMsg.rsp,
 * Len=24 (3 bytes), Outputlen=256.
 */
static const uint8_t shake256_msg_in[3] = {
	0x21, 0xed, 0xa6
};

static const uint8_t shake256_msg_out[32] = {
	0xf7, 0xd0, 0x2b, 0x45, 0x12, 0xbe, 0x5d, 0xdc,
	0xc2, 0x5d, 0x14, 0x8c, 0x71, 0x66, 0x4d, 0xfd,
	0x34, 0xe1, 0x6a, 0xbe, 0xa2, 0x6d, 0x6e, 0x72,
	0x87, 0xf4, 0x5e, 0x08, 0xed, 0x6f, 0xcd, 0x87
};

/**
 * @brief Common test suite setup function
 */
static void *setup_crypto(void)
{
	psa_status_t status = psa_crypto_init();

	zassert_equal(status, PSA_SUCCESS, "PSA Crypto initialization failed");
	return NULL;
}

/**
 * @brief Test XOF setup with valid algorithms
 */
static void test_xof_setup_valid(void)
{
	psa_xof_operation_t operation = psa_xof_operation_init();
	psa_status_t status;

	/* Test SHAKE128 setup */
	if (IS_ENABLED(CONFIG_PSA_WANT_ALG_SHAKE128)) {
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE128);
		zassert_equal(status, PSA_SUCCESS,
			      "SHAKE128 setup failed with status %d", status);
		psa_xof_abort(&operation);
	}

	/* Test SHAKE256 setup */
	if (IS_ENABLED(CONFIG_PSA_WANT_ALG_SHAKE256)) {
		operation = psa_xof_operation_init();
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS,
			      "SHAKE256 setup failed with status %d", status);
		psa_xof_abort(&operation);
	}
}

/**
 * @brief Test XOF basic output without input
 */
static void test_xof_output_no_input(void)
{
	psa_xof_operation_t operation = psa_xof_operation_init();
	psa_status_t status;
	uint8_t output1[32];
	uint8_t output2[32];

	if (IS_ENABLED(CONFIG_PSA_WANT_ALG_SHAKE256)) {
		/* Setup and output without input */
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "SHAKE256 setup failed");

		status = psa_xof_output(&operation, output1, sizeof(output1));
		zassert_equal(status, PSA_SUCCESS,
			      "XOF output failed with status %d", status);

		status = psa_xof_abort(&operation);
		zassert_equal(status, PSA_SUCCESS, "XOF abort failed");

		/* Verify output is consistent */
		operation = psa_xof_operation_init();
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "SHAKE256 setup failed (2nd)");

		status = psa_xof_output(&operation, output2, sizeof(output2));
		zassert_equal(status, PSA_SUCCESS, "XOF output failed (2nd)");

		status = psa_xof_abort(&operation);
		zassert_equal(status, PSA_SUCCESS, "XOF abort failed (2nd)");

		/* Both outputs should be identical */
		zassert_equal(memcmp(output1, output2, sizeof(output1)), 0,
			      "XOF outputs differ");
	}
}

/**
 * @brief Test XOF with context
 */
static void test_xof_with_context(void)
{
	psa_xof_operation_t operation = psa_xof_operation_init();
	psa_status_t status;

	if (IS_ENABLED(CONFIG_PSA_WANT_ALG_SHAKE256)) {
		/* XOF with context */
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "SHAKE256 setup failed");

		status = psa_xof_set_context(&operation, test_context,
					     sizeof(test_context));
		zassert_equal(status, PSA_ERROR_INVALID_ARGUMENT,
			      "XOF set_context succeeded (must fail)");
	}
}

/**
 * @brief Test XOF state transitions
 */
static void test_xof_state_transitions(void)
{
	psa_xof_operation_t operation = psa_xof_operation_init();
	psa_status_t status;
	uint8_t output[32];
	uint8_t test_data[] = "test";

	if (IS_ENABLED(CONFIG_PSA_WANT_ALG_SHAKE256)) {
		/* Test: operations fail in inactive state */
		status = psa_xof_update(&operation, test_data,
				       sizeof(test_data));
		zassert_equal(status, PSA_ERROR_BAD_STATE,
			      "Update should fail in inactive state");

		status = psa_xof_output(&operation, output, sizeof(output));
		zassert_equal(status, PSA_ERROR_BAD_STATE,
			      "Output should fail in inactive state");

		/* Abort is always OK */
		status = psa_xof_abort(&operation);
		zassert_equal(status, PSA_SUCCESS, "Abort failed");

		/* Test: can't set context after update */
		operation = psa_xof_operation_init();
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "Setup failed");

		status = psa_xof_update(&operation, test_data,
				       sizeof(test_data));
		zassert_equal(status, PSA_SUCCESS, "Update failed");

		status = psa_xof_set_context(&operation, test_context,
					     sizeof(test_context));
		zassert_equal(status, PSA_ERROR_BAD_STATE,
			      "set_context should fail after update");

		psa_xof_abort(&operation);

		/* Test: multiple outputs are OK */
		operation = psa_xof_operation_init();
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "Setup failed (2nd)");

		status = psa_xof_update(&operation, test_data,
				       sizeof(test_data));
		zassert_equal(status, PSA_SUCCESS, "Update failed (2nd)");

		uint8_t out1[16];
		uint8_t out2[16];

		status = psa_xof_output(&operation, out1, sizeof(out1));
		zassert_equal(status, PSA_SUCCESS, "First output failed");

		status = psa_xof_output(&operation, out2, sizeof(out2));
		zassert_equal(status, PSA_SUCCESS, "Second output failed");

		/* First part of output should match when calling again */
		uint8_t out_combined[32];

		psa_xof_abort(&operation);

		operation = psa_xof_operation_init();
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "Setup failed (3rd)");

		status = psa_xof_update(&operation, test_data,
				       sizeof(test_data));
		zassert_equal(status, PSA_SUCCESS, "Update failed (3rd)");

		status = psa_xof_output(&operation, out_combined, 32);
		zassert_equal(status, PSA_SUCCESS, "Combined output failed");

		zassert_equal(memcmp(out1, out_combined, 16), 0,
			      "First output part mismatch");
		zassert_equal(memcmp(out2, out_combined + 16, 16), 0,
			      "Second output part mismatch");

		psa_xof_abort(&operation);
	}
}

/**
 * @brief Test XOF with multiple updates
 */
static void test_xof_multiple_updates(void)
{
	psa_xof_operation_t operation = psa_xof_operation_init();
	psa_status_t status;
	uint8_t output1[32];
	uint8_t output2[32];
	uint8_t part1[] = { 0x61, 0x62 };  /* "ab" */
	uint8_t part2[] = { 0x63 };        /* "c" */
	uint8_t combined[] = { 0x61, 0x62, 0x63 };  /* "abc" */

	if (IS_ENABLED(CONFIG_PSA_WANT_ALG_SHAKE256)) {
		/* Test: split input into multiple updates */
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "Setup failed");

		status = psa_xof_update(&operation, part1, sizeof(part1));
		zassert_equal(status, PSA_SUCCESS, "First update failed");

		status = psa_xof_update(&operation, part2, sizeof(part2));
		zassert_equal(status, PSA_SUCCESS, "Second update failed");

		status = psa_xof_output(&operation, output1, sizeof(output1));
		zassert_equal(status, PSA_SUCCESS, "Output failed");

		psa_xof_abort(&operation);

		/* Test: single update with combined input */
		operation = psa_xof_operation_init();
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "Setup failed (2nd)");

		status = psa_xof_update(&operation, combined, sizeof(combined));
		zassert_equal(status, PSA_SUCCESS, "Update failed (2nd)");

		status = psa_xof_output(&operation, output2, sizeof(output2));
		zassert_equal(status, PSA_SUCCESS, "Output failed (2nd)");

		psa_xof_abort(&operation);

		/* Outputs should be identical */
		zassert_equal(memcmp(output1, output2, sizeof(output1)), 0,
			      "Split vs single update outputs differ");
	}
}

/**
 * @brief Test XOF empty update
 */
static void test_xof_empty_update(void)
{
	psa_xof_operation_t operation = psa_xof_operation_init();
	psa_status_t status;
	uint8_t output1[32];
	uint8_t output2[32];

	if (IS_ENABLED(CONFIG_PSA_WANT_ALG_SHAKE256)) {
		/* Test: empty update(NULL, 0) */
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "Setup failed");

		status = psa_xof_update(&operation, NULL, 0);
		zassert_equal(status, PSA_SUCCESS,
			      "Empty update(NULL, 0) failed");

		status = psa_xof_output(&operation, output1, sizeof(output1));
		zassert_equal(status, PSA_SUCCESS, "Output failed");

		psa_xof_abort(&operation);

		/* Test: no update at all */
		operation = psa_xof_operation_init();
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "Setup failed (2nd)");

		status = psa_xof_output(&operation, output2, sizeof(output2));
		zassert_equal(status, PSA_SUCCESS, "Output failed (2nd)");

		psa_xof_abort(&operation);

		/* Both should produce the same output */
		zassert_equal(memcmp(output1, output2, sizeof(output1)), 0,
			      "Empty update vs no update outputs differ");
	}
}

/**
 * @brief Test XOF abort idempotency
 */
static void test_xof_abort_idempotent(void)
{
	psa_xof_operation_t operation = psa_xof_operation_init();
	psa_status_t status;

	if (IS_ENABLED(CONFIG_PSA_WANT_ALG_SHAKE256)) {
		/* Multiple aborts should be OK */
		status = psa_xof_abort(&operation);
		zassert_equal(status, PSA_SUCCESS, "First abort failed");

		status = psa_xof_abort(&operation);
		zassert_equal(status, PSA_SUCCESS, "Second abort failed");

		status = psa_xof_abort(&operation);
		zassert_equal(status, PSA_SUCCESS, "Third abort failed");

		/* Setup and abort multiple times */
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "Setup failed");

		status = psa_xof_abort(&operation);
		zassert_equal(status, PSA_SUCCESS, "Abort after setup failed");

		status = psa_xof_abort(&operation);
		zassert_equal(status, PSA_SUCCESS,
			      "Second abort after setup failed");
	}
}

/**
 * @brief Known-Answer Test: verify SHAKE128 and SHAKE256 against
 *	  NIST FIPS 202 test vectors (see vector declarations above
 *	  for the source reference).
 */
static void test_xof_known_answer(void)
{
	psa_xof_operation_t operation = psa_xof_operation_init();
	psa_status_t status;

	if (IS_ENABLED(CONFIG_PSA_WANT_ALG_SHAKE128)) {
		uint8_t output[32];

		/* SHAKE128(empty, Outputlen=256 bits) — FIPS 202 Appendix B.1 */
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE128);
		zassert_equal(status, PSA_SUCCESS, "SHAKE128 setup failed");
		status = psa_xof_output(&operation, output, sizeof(output));
		zassert_equal(status, PSA_SUCCESS, "SHAKE128 output failed");
		zassert_equal(psa_xof_abort(&operation), PSA_SUCCESS, "abort failed");
		zassert_mem_equal(output, shake128_empty_out, sizeof(output),
				  "SHAKE128(empty) KAT mismatch");

		/** CAVP SHAKE128ShortMsg.rsp, Len=24
		 *  SHAKE128(Msg=0x1b3b6e, Outputlen=128 bits)
		 */
		uint8_t output128[16];

		operation = psa_xof_operation_init();
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE128);
		zassert_equal(status, PSA_SUCCESS, "SHAKE128 setup failed");
		status = psa_xof_update(&operation, shake128_msg_in,
					sizeof(shake128_msg_in));
		zassert_equal(status, PSA_SUCCESS, "SHAKE128 update failed");
		status = psa_xof_output(&operation, output128, sizeof(output128));
		zassert_equal(status, PSA_SUCCESS, "SHAKE128 output failed");
		zassert_equal(psa_xof_abort(&operation), PSA_SUCCESS, "abort failed");
		zassert_mem_equal(output128, shake128_msg_out, sizeof(output128),
				  "SHAKE128(0x1b3b6e) KAT mismatch");
	}

	if (IS_ENABLED(CONFIG_PSA_WANT_ALG_SHAKE256)) {
		uint8_t output[64];

		/* SHAKE256(empty, Outputlen=512 bits) — FIPS 202 Appendix B.2 */
		operation = psa_xof_operation_init();
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "SHAKE256 setup failed");
		status = psa_xof_output(&operation, output, sizeof(output));
		zassert_equal(status, PSA_SUCCESS, "SHAKE256 output failed");
		zassert_equal(psa_xof_abort(&operation), PSA_SUCCESS, "abort failed");
		zassert_mem_equal(output, shake256_empty_out, sizeof(output),
				  "SHAKE256(empty) KAT mismatch");

		/** CAVP SHAKE256ShortMsg.rsp, Len=24
		 *  SHAKE256(Msg=0x21eda6, Outputlen=256 bits)
		 */
		uint8_t output256[32];

		operation = psa_xof_operation_init();
		status = psa_xof_setup(&operation, PSA_ALG_SHAKE256);
		zassert_equal(status, PSA_SUCCESS, "SHAKE256 setup failed");
		status = psa_xof_update(&operation, shake256_msg_in,
					sizeof(shake256_msg_in));
		zassert_equal(status, PSA_SUCCESS, "SHAKE256 update failed");
		status = psa_xof_output(&operation, output256, sizeof(output256));
		zassert_equal(status, PSA_SUCCESS, "SHAKE256 output failed");
		zassert_equal(psa_xof_abort(&operation), PSA_SUCCESS, "abort failed");
		zassert_mem_equal(output256, shake256_msg_out, sizeof(output256),
				  "SHAKE256(0x21eda6) KAT mismatch");
	}
}

/* ZTEST suite definition */
ZTEST_SUITE(psa_xof_tests, NULL, setup_crypto, NULL, NULL, NULL);

ZTEST(psa_xof_tests, test_xof_setup_valid)
{
	test_xof_setup_valid();
}

ZTEST(psa_xof_tests, test_xof_output_no_input)
{
	test_xof_output_no_input();
}

ZTEST(psa_xof_tests, test_xof_with_context)
{
	test_xof_with_context();
}

ZTEST(psa_xof_tests, test_xof_state_transitions)
{
	test_xof_state_transitions();
}

ZTEST(psa_xof_tests, test_xof_multiple_updates)
{
	test_xof_multiple_updates();
}

ZTEST(psa_xof_tests, test_xof_empty_update)
{
	test_xof_empty_update();
}

ZTEST(psa_xof_tests, test_xof_abort_idempotent)
{
	test_xof_abort_idempotent();
}

ZTEST(psa_xof_tests, test_xof_known_answer)
{
	test_xof_known_answer();
}
