/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <suit_digest_sink.h>

static const uint8_t valid_payload[] = {0xde, 0xad, 0xbe, 0xef};

const static uint8_t valid_digest[] = {0x5f, 0x78, 0xc3, 0x32, 0x74, 0xe4, 0x3f, 0xa9,
				       0xde, 0x56, 0x59, 0x26, 0x5c, 0x1d, 0x91, 0x7e,
				       0x25, 0xc0, 0x37, 0x22, 0xdc, 0xb0, 0xb8, 0xd2,
				       0x7d, 0xb8, 0xd5, 0xfe, 0xaa, 0x81, 0x39, 0x53};

const static uint8_t invalid_digest[] = {0x5f, 0x78, 0xc3, 0x32, 0x74, 0xe4, 0x3f, 0xa9,
					 0xde, 0x56, 0x59, 0x26, 0x5c, 0x1d, 0x91, 0x7e,
					 0x25, 0xc0, 0x37, 0x22, 0xdc, 0xb0, 0xb8, 0xd2,
					 0x7d, 0xb8, 0xd5, 0xfe, 0xaa, 0x81, 0x39, 0x54};

static const psa_algorithm_t valid_algorithm = PSA_ALG_SHA_256;
static const psa_algorithm_t invalid_algorithm = (psa_algorithm_t)0;

ZTEST_SUITE(digest_sink_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(digest_sink_tests, test_digest_sink_get_OK)
{
	struct stream_sink digest_sink;

	suit_plat_err_t err = suit_digest_sink_get(&digest_sink, valid_algorithm, valid_digest);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unexpected error code");

	err = digest_sink.release(digest_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "digest_sink.release failed - error %i", err);
}

ZTEST(digest_sink_tests, test_digest_sink_get_NOK)
{
	struct stream_sink digest_sink;

	suit_plat_err_t err = suit_digest_sink_get(NULL, valid_algorithm, valid_digest);

	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_digest_sink_get should have failed - sink == null");

	err = suit_digest_sink_get(&digest_sink, valid_algorithm, NULL);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_digest_sink_get should have failed - digest == null");

	err = suit_digest_sink_get(&digest_sink, invalid_algorithm, valid_digest);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_digest_sink_get should have failed - invalid algorithm");
}

ZTEST(digest_sink_tests, test_digest_sink_get_out_of_contexts)
{
	struct stream_sink digest_sinks[CONFIG_SUIT_STREAM_SINK_DIGEST_CONTEXT_COUNT + 1];
	suit_plat_err_t err;

	for (size_t i = 0; i < CONFIG_SUIT_STREAM_SINK_DIGEST_CONTEXT_COUNT; i++) {
		err = suit_digest_sink_get(&digest_sinks[i], valid_algorithm, valid_digest);
		zassert_equal(err, SUIT_PLAT_SUCCESS, "Unexpected error code");
	}

	err = suit_digest_sink_get(&digest_sinks[CONFIG_SUIT_STREAM_SINK_DIGEST_CONTEXT_COUNT],
				   valid_algorithm, valid_digest);

	zassert_equal(err, SUIT_PLAT_ERR_NO_RESOURCES, "Unexpected error code");

	for (size_t i = 0; i < CONFIG_SUIT_STREAM_SINK_DIGEST_CONTEXT_COUNT; i++) {
		err = digest_sinks[i].release(digest_sinks[i].ctx);
		zassert_equal(err, SUIT_PLAT_SUCCESS, "digest_sink.release failed - error %i", err);
	}
}

ZTEST(digest_sink_tests, test_digest_sink_write_NOK)
{
	struct stream_sink digest_sink;

	suit_plat_err_t err = suit_digest_sink_get(&digest_sink, valid_algorithm, valid_digest);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unexpected error code");

	err = digest_sink.write(NULL, valid_payload, sizeof(valid_payload));
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "digest_sink.write should have failed - ctx == NULL");

	err = digest_sink.write(digest_sink.ctx, NULL, sizeof(valid_payload));
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "digest_sink.write should have failed - payload == NULL");

	err = digest_sink.write(digest_sink.ctx, valid_payload, 0);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "digest_sink.write should have failed - size == 0");

	err = digest_sink.release(digest_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "digest_sink.release failed - error %i", err);

	err = digest_sink.write(digest_sink.ctx, valid_payload, sizeof(valid_payload));
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "digest_sink.write should have failed - ctx released");
}

ZTEST(digest_sink_tests, test_digest_sink_match_OK)
{
	struct stream_sink digest_sink;

	suit_plat_err_t err = suit_digest_sink_get(&digest_sink, valid_algorithm, valid_digest);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unexpected error code");

	err = digest_sink.write(digest_sink.ctx, valid_payload, sizeof(valid_payload));
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unexpected error code");

	err = suit_digest_sink_digest_match(digest_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unexpected error code");

	err = digest_sink.release(digest_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "digest_sink.release failed - error %i", err);
}

ZTEST(digest_sink_tests, test_digest_sink_match_NOK)
{
	struct stream_sink digest_sink;

	suit_plat_err_t err = suit_digest_sink_get(&digest_sink, valid_algorithm, valid_digest);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unexpected error code");

	err = digest_sink.write(digest_sink.ctx, valid_payload, sizeof(valid_payload));
	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unexpected error code");

	err = suit_digest_sink_digest_match(NULL);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_digest_sink_digest_match should have failed - ctx == NULL");

	err = digest_sink.release(digest_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "digest_sink.release failed - error %i", err);

	err = suit_digest_sink_digest_match(digest_sink.ctx);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_digest_sink_digest_match should have failed - ctx released");
}

ZTEST(digest_sink_tests, test_digest_sink_release_NOK)
{
	struct stream_sink digest_sink;

	suit_plat_err_t err = suit_digest_sink_get(&digest_sink, valid_algorithm, invalid_digest);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "Unexpected error code");

	err = digest_sink.release(NULL);
	zassert_equal(err, SUIT_PLAT_ERR_INVAL, "Unexpected error code");

	err = digest_sink.release(digest_sink.ctx);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "digest_sink.release failed - error %i", err);
}
