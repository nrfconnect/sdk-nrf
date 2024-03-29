/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <suit_platform.h>

const static uint8_t valid_payload_value[] = {0xde, 0xad, 0xbe, 0xef};

static struct zcbor_string payload_valid = {
	.value = valid_payload_value,
	.len = sizeof(valid_payload_value),
};

static struct zcbor_string payload_invalid_null_value = {
	.value = NULL,
	.len = sizeof(valid_payload_value),
};

static struct zcbor_string payload_invalid_zero_length = {
	.value = valid_payload_value,
	.len = 0,
};

const static uint8_t valid_digest_value[] = {0x5f, 0x78, 0xc3, 0x32, 0x74, 0xe4, 0x3f, 0xa9,
					     0xde, 0x56, 0x59, 0x26, 0x5c, 0x1d, 0x91, 0x7e,
					     0x25, 0xc0, 0x37, 0x22, 0xdc, 0xb0, 0xb8, 0xd2,
					     0x7d, 0xb8, 0xd5, 0xfe, 0xaa, 0x81, 0x39, 0x53};

const static uint8_t invalid_digest_value[] = {0x5f, 0x78, 0xc3, 0x32, 0x74, 0xe4, 0x3f, 0xa9,
					       0xde, 0x56, 0x59, 0x26, 0x5c, 0x1d, 0x91, 0x7e,
					       0x25, 0xc0, 0x37, 0x22, 0xdc, 0xb0, 0xb8, 0xd2,
					       0x7d, 0xb8, 0xd5, 0xfe, 0xaa, 0x81, 0x39, 0x54};

static struct zcbor_string digest_valid = {
	.value = valid_digest_value,
	.len = sizeof(valid_digest_value),
};

static struct zcbor_string digest_invalid = {
	.value = invalid_digest_value,
	.len = sizeof(invalid_digest_value),
};

static struct zcbor_string digest_wrong_length = {
	.value = valid_digest_value,
	.len = sizeof(valid_digest_value) - 1,
};

static const enum suit_cose_alg valid_algorithm = suit_cose_sha256;
static const enum suit_cose_alg invalid_algorithm = (enum suit_cose_alg)0;

ZTEST_SUITE(digest_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(digest_tests, test_unsupported_algorithm)
{
	/* GIVEN unsupported digest algorithm... */
	const enum suit_cose_alg algorithm = invalid_algorithm;

	/* ...valid expected digest... */
	struct zcbor_string *expected_digest = &digest_valid;

	/* ...and valid payload */
	struct zcbor_string *payload = &payload_valid;

	/* WHEN digest is checked */
	int err = suit_plat_check_digest(algorithm, expected_digest, payload);

	/* THEN appropriate error code is returned */
	zassert_equal(err, SUIT_ERR_DECODING, "Unexpected error code");
}

ZTEST(digest_tests, test_digest_null)
{
	/* GIVEN supported digest algorithm... */
	enum suit_cose_alg algorithm = valid_algorithm;

	/* ...NULL expected digest */
	struct zcbor_string *expected_digest = NULL;

	/* ...and a valid payload */
	struct zcbor_string *payload = &payload_valid;

	/* WHEN a digest is checked */
	int err = suit_plat_check_digest(algorithm, expected_digest, payload);

	/* THEN appropriate error code is returned */
	zassert_equal(err, SUIT_FAIL_CONDITION, "Unexpected error code");
}

ZTEST(digest_tests, test_payload_null)
{
	/* GIVEN supported digest algorithm... */
	enum suit_cose_alg algorithm = valid_algorithm;

	/* ...valid expected digest */
	struct zcbor_string *expected_digest = &digest_valid;

	/* ...and a NULL payload */
	struct zcbor_string *payload = NULL;

	/* WHEN a digest is checked */
	int err = suit_plat_check_digest(algorithm, expected_digest, payload);

	/* THEN appropriate error code is returned */
	zassert_equal(err, SUIT_FAIL_CONDITION, "Unexpected error code");
}

ZTEST(digest_tests, test_payload_value_null)
{
	/* GIVEN supported digest algorithm... */
	enum suit_cose_alg algorithm = valid_algorithm;

	/* ...valid expected digest */
	struct zcbor_string *expected_digest = &digest_valid;

	/* ...and a payload which has a NULL value */
	struct zcbor_string *payload = &payload_invalid_null_value;

	/* WHEN a digest is checked */
	int err = suit_plat_check_digest(algorithm, expected_digest, payload);

	/* THEN appropriate error code is returned */
	zassert_equal(err, SUIT_ERR_DECODING, "Unexpected error code");
}

ZTEST(digest_tests, test_payload_length_zero)
{
	/* GIVEN supported digest algorithm... */
	enum suit_cose_alg algorithm = valid_algorithm;

	/* ...valid expected digest */
	struct zcbor_string *expected_digest = &digest_valid;

	/* ...and a payload which has a NULL value */
	struct zcbor_string *payload = &payload_invalid_zero_length;

	/* WHEN a digest is checked */
	int err = suit_plat_check_digest(algorithm, expected_digest, payload);

	/* THEN appropriate error code is returned */
	zassert_equal(err, SUIT_ERR_DECODING, "Unexpected error code");
}

ZTEST(digest_tests, test_digest_wrong_length)
{
	/* GIVEN supported digest algorithm... */
	enum suit_cose_alg algorithm = valid_algorithm;

	/* ...expected digest with wrong length */
	struct zcbor_string *expected_digest = &digest_wrong_length;

	/* ...and a valid payload */
	struct zcbor_string *payload = &payload_valid;

	/* WHEN a digest is checked */
	int err = suit_plat_check_digest(algorithm, expected_digest, payload);

	/* THEN appropriate error code is returned */
	zassert_equal(err, SUIT_ERR_DECODING, "Unexpected error code");
}

ZTEST(digest_tests, test_digest_match)
{
	/* GIVEN supported digest algorithm... */
	enum suit_cose_alg algorithm = valid_algorithm;

	/* ...a valid digest */
	struct zcbor_string *expected_digest = &digest_valid;

	/* ...and a valid payload */
	struct zcbor_string *payload = &payload_valid;

	/* WHEN a digest is checked */
	int err = suit_plat_check_digest(algorithm, expected_digest, payload);

	/* THEN operation ends with success */
	zassert_equal(err, SUIT_SUCCESS, "Unexpected error code");
}

ZTEST(digest_tests, test_digest_mismatch)
{
	/* GIVEN supported digest algorithm... */
	enum suit_cose_alg algorithm = valid_algorithm;

	/* ...an invalid digest */
	struct zcbor_string *expected_digest = &digest_invalid;

	/* ...and a valid payload */
	struct zcbor_string *payload = &payload_valid;

	/* WHEN a digest is checked */
	int err = suit_plat_check_digest(algorithm, expected_digest, payload);

	/* THEN operation ends with condition failure */
	zassert_equal(err, SUIT_FAIL_CONDITION, "Unexpected error code");
}
