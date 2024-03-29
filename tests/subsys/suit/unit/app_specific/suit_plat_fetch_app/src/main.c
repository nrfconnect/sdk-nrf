/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <suit_plat_fetch_domain_specific.h>
#include <mocks.h>

#define TEST_COMPONENT_HANDLE ((suit_component_t)0x123)

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(suit_platform_app_fetch_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_platform_app_fetch_tests, test_fetch_type_supported)
{
	zassert_true(
		suit_plat_fetch_domain_specific_is_type_supported(SUIT_COMPONENT_TYPE_CAND_IMG),
		"suit_plat_fetch_domain_specific_is_type_supported returned false for supported "
		"type");
	zassert_true(
		suit_plat_fetch_domain_specific_is_type_supported(SUIT_COMPONENT_TYPE_CAND_MFST),
		"suit_plat_fetch_domain_specific_is_type_supported returned false for supported "
		"type");
	zassert_true(
		suit_plat_fetch_domain_specific_is_type_supported(SUIT_COMPONENT_TYPE_CACHE_POOL),
		"suit_plat_fetch_domain_specific_is_type_supported returned false for supported "
		"type");

	zassert_false(
		suit_plat_fetch_domain_specific_is_type_supported(SUIT_COMPONENT_TYPE_UNSUPPORTED),
		"suit_plat_fetch_domain_specific_is_type_supported returned true for unsupported "
		"type");
	zassert_false(suit_plat_fetch_domain_specific_is_type_supported(SUIT_COMPONENT_TYPE_MEM),
		      "suit_plat_fetch_domain_specific_is_type_supported returned true for "
		      "unsupported type");
	zassert_false(
		suit_plat_fetch_domain_specific_is_type_supported(SUIT_COMPONENT_TYPE_INSTLD_MFST),
		"suit_plat_fetch_domain_specific_is_type_supported returned true for unsupported "
		"type");
	zassert_false(
		suit_plat_fetch_domain_specific_is_type_supported(SUIT_COMPONENT_TYPE_SOC_SPEC),
		"suit_plat_fetch_domain_specific_is_type_supported returned true for unsupported "
		"type");
}

ZTEST(suit_platform_app_fetch_tests, test_fetch_integrated_type_supported)
{
	zassert_true(suit_plat_fetch_integrated_domain_specific_is_type_supported(
			     SUIT_COMPONENT_TYPE_CAND_IMG),
		     "suit_plat_fetch_integrated_domain_specific_is_type_supported returned false "
		     "for supported type");
	zassert_true(suit_plat_fetch_integrated_domain_specific_is_type_supported(
			     SUIT_COMPONENT_TYPE_CAND_MFST),
		     "suit_plat_fetch_integrated_domain_specific_is_type_supported returned false "
		     "for supported type");

	zassert_false(suit_plat_fetch_integrated_domain_specific_is_type_supported(
			      SUIT_COMPONENT_TYPE_CACHE_POOL),
		      "suit_plat_fetch_integrated_domain_specific_is_type_supported returned true "
		      "for unsupported type");
	zassert_false(suit_plat_fetch_integrated_domain_specific_is_type_supported(
			      SUIT_COMPONENT_TYPE_UNSUPPORTED),
		      "suit_plat_fetch_integrated_domain_specific_is_type_supported returned true "
		      "for unsupported type");
	zassert_false(suit_plat_fetch_integrated_domain_specific_is_type_supported(
			      SUIT_COMPONENT_TYPE_MEM),
		      "suit_plat_fetch_integrated_domain_specific_is_type_supported returned true "
		      "for unsupported type");
	zassert_false(suit_plat_fetch_integrated_domain_specific_is_type_supported(
			      SUIT_COMPONENT_TYPE_INSTLD_MFST),
		      "suit_plat_fetch_integrated_domain_specific_is_type_supported returned true "
		      "for unsupported type");
	zassert_false(suit_plat_fetch_integrated_domain_specific_is_type_supported(
			      SUIT_COMPONENT_TYPE_SOC_SPEC),
		      "suit_plat_fetch_integrated_domain_specific_is_type_supported returned true "
		      "for unsupported type");
}

ZTEST(suit_platform_app_fetch_tests, test_fetch_unsupported_component_type)
{
	struct stream_sink dummy_sink = {0};
	struct zcbor_string dummy_uri = {.value = (const uint8_t *)"test", .len = 4};

	zassert_equal(suit_plat_fetch_domain_specific(TEST_COMPONENT_HANDLE,
						      SUIT_COMPONENT_TYPE_UNSUPPORTED, &dummy_sink,
						      &dummy_uri),
		      SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "Unsupported component type returned incorrect error code");
}

ZTEST(suit_platform_app_fetch_tests, test_fetch_dfu_cache_streamer_fail)
{
	struct stream_sink dummy_sink = {0};
	struct zcbor_string dummy_uri = {.value = (const uint8_t *)"test", .len = 4};

	suit_dfu_cache_streamer_stream_fake.return_val = SUIT_PLAT_ERR_IO;

	zassert_equal(suit_plat_fetch_domain_specific(TEST_COMPONENT_HANDLE,
						      SUIT_COMPONENT_TYPE_CAND_IMG, &dummy_sink,
						      &dummy_uri),
		      SUIT_ERR_CRASH, "Failed to return correct error code");
	zassert_equal(suit_dfu_cache_streamer_stream_fake.call_count, 1,
		      "Incorrect number of suit_dfu_cache_streamer_stream() calls");
	zassert_equal(suit_dfu_cache_streamer_stream_fake.arg0_val, dummy_uri.value,
		      "Incorrect value streamer argument");
	zassert_equal(suit_dfu_cache_streamer_stream_fake.arg1_val, dummy_uri.len,
		      "Incorrect value of streamer argument");
	zassert_equal(suit_dfu_cache_streamer_stream_fake.arg2_val, &dummy_sink,
		      "Incorrect value of streamer argument");

	zassert_equal(suit_dfu_cache_sink_commit_fake.call_count, 0,
		      "Incorrect number of suit_dfu_cache_sink_commit() calls");
	zassert_equal(suit_fetch_source_stream_fake.call_count, 0,
		      "Incorrect number of suit_fetch_source_stream() calls");
}

ZTEST(suit_platform_app_fetch_tests, test_fetch_dfu_cache_streamer_success)
{
	struct stream_sink dummy_sink = {0};
	struct zcbor_string dummy_uri = {.value = (const uint8_t *)"test", .len = 4};

	suit_component_type_t types[] = {SUIT_COMPONENT_TYPE_CAND_IMG,
					 SUIT_COMPONENT_TYPE_CAND_MFST};

	for (size_t i = 0; i < ARRAY_SIZE(types); i++) {
		mocks_reset();
		FFF_RESET_HISTORY();

		suit_dfu_cache_streamer_stream_fake.return_val = SUIT_PLAT_SUCCESS;

		zassert_equal(suit_plat_fetch_domain_specific(TEST_COMPONENT_HANDLE, types[i],
							      &dummy_sink, &dummy_uri),
			      SUIT_SUCCESS, "suit_plat_fetch_domain_specific() failed");
		zassert_equal(suit_dfu_cache_streamer_stream_fake.call_count, 1,
			      "Incorrect number of suit_dfu_cache_streamer_stream() calls");
		zassert_equal(suit_dfu_cache_streamer_stream_fake.arg0_val, dummy_uri.value,
			      "Incorrect value streamer argument");
		zassert_equal(suit_dfu_cache_streamer_stream_fake.arg1_val, dummy_uri.len,
			      "Incorrect value of streamer argument");
		zassert_equal(suit_dfu_cache_streamer_stream_fake.arg2_val, &dummy_sink,
			      "Incorrect value of streamer argument");

		zassert_equal(suit_dfu_cache_sink_commit_fake.call_count, 0,
			      "Incorrect number of suit_dfu_cache_sink_commit() calls");
		zassert_equal(suit_fetch_source_stream_fake.call_count, 0,
			      "Incorrect number of suit_fetch_source_stream() calls");
	}
}

ZTEST(suit_platform_app_fetch_tests, test_fetch_fetch_source_streamer_fail)
{
	struct stream_sink dummy_sink = {0};
	struct zcbor_string dummy_uri = {.value = (const uint8_t *)"test", .len = 4};

	suit_fetch_source_stream_fake.return_val = SUIT_PLAT_ERR_IO;

	zassert_equal(suit_plat_fetch_domain_specific(TEST_COMPONENT_HANDLE,
						      SUIT_COMPONENT_TYPE_CACHE_POOL, &dummy_sink,
						      &dummy_uri),
		      SUIT_ERR_CRASH, "Failed to return correct error code");
	zassert_equal(suit_fetch_source_stream_fake.call_count, 1,
		      "Incorrect number of suit_fetch_source_stream() calls");
	zassert_equal(suit_fetch_source_stream_fake.arg0_val, dummy_uri.value,
		      "Incorrect value streamer argument");
	zassert_equal(suit_fetch_source_stream_fake.arg1_val, dummy_uri.len,
		      "Incorrect value of streamer argument");
	zassert_equal(suit_fetch_source_stream_fake.arg2_val, &dummy_sink,
		      "Incorrect value of streamer argument");

	zassert_equal(suit_dfu_cache_sink_commit_fake.call_count, 0,
		      "Incorrect number of suit_dfu_cache_sink_commit() calls");
	zassert_equal(suit_dfu_cache_streamer_stream_fake.call_count, 0,
		      "Incorrect number of suit_fetch_source_stream() calls");
}

ZTEST(suit_platform_app_fetch_tests, test_fetch_cache_pool_type)
{
	uint8_t dummy_ctx = 0;
	struct stream_sink dummy_sink = {.ctx = &dummy_ctx};
	struct zcbor_string dummy_uri = {.value = (const uint8_t *)"test", .len = 4};

	suit_fetch_source_stream_fake.return_val = SUIT_PLAT_SUCCESS;

	zassert_equal(suit_plat_fetch_domain_specific(TEST_COMPONENT_HANDLE,
						      SUIT_COMPONENT_TYPE_CACHE_POOL, &dummy_sink,
						      &dummy_uri),
		      SUIT_SUCCESS, "Failed to return correct error code");
	zassert_equal(suit_fetch_source_stream_fake.call_count, 1,
		      "Incorrect number of suit_fetch_source_stream() calls");
	zassert_equal(suit_fetch_source_stream_fake.arg0_val, dummy_uri.value,
		      "Incorrect value streamer argument");
	zassert_equal(suit_fetch_source_stream_fake.arg1_val, dummy_uri.len,
		      "Incorrect value of streamer argument");
	zassert_equal(suit_fetch_source_stream_fake.arg2_val, &dummy_sink,
		      "Incorrect value of streamer argument");

	zassert_equal(suit_dfu_cache_sink_commit_fake.call_count, 1,
		      "Incorrect number of suit_dfu_cache_sink_commit() calls");
	zassert_equal(suit_dfu_cache_sink_commit_fake.arg0_val, &dummy_ctx,
		      "Incorrect value suit_dfu_cache_sink_commit argument");

	zassert_equal(suit_dfu_cache_streamer_stream_fake.call_count, 0,
		      "Incorrect number of suit_fetch_source_stream() calls");
}

ZTEST(suit_platform_app_fetch_tests, test_fetch_mem_type)
{
	uint8_t dummy_ctx = 0;
	struct stream_sink dummy_sink = {.ctx = &dummy_ctx};
	struct zcbor_string dummy_uri = {.value = (const uint8_t *)"test", .len = 4};

	suit_fetch_source_stream_fake.return_val = SUIT_PLAT_SUCCESS;

	zassert_equal(suit_plat_fetch_domain_specific(TEST_COMPONENT_HANDLE,
						      SUIT_COMPONENT_TYPE_MEM, &dummy_sink,
						      &dummy_uri),
		      SUIT_SUCCESS, "Failed to return correct error code");
	zassert_equal(suit_fetch_source_stream_fake.call_count, 1,
		      "Incorrect number of suit_fetch_source_stream() calls");
	zassert_equal(suit_fetch_source_stream_fake.arg0_val, dummy_uri.value,
		      "Incorrect value streamer argument");
	zassert_equal(suit_fetch_source_stream_fake.arg1_val, dummy_uri.len,
		      "Incorrect value of streamer argument");
	zassert_equal(suit_fetch_source_stream_fake.arg2_val, &dummy_sink,
		      "Incorrect value of streamer argument");

	zassert_equal(suit_dfu_cache_sink_commit_fake.call_count, 0,
		      "Incorrect number of suit_dfu_cache_sink_commit() calls");

	zassert_equal(suit_dfu_cache_streamer_stream_fake.call_count, 0,
		      "Incorrect number of suit_fetch_source_stream() calls");
}

ZTEST(suit_platform_app_fetch_tests, test_fetch_integrated_domain_specific)
{
	struct stream_sink dummy_sink = {0};

	/* Simply return success unconditionally */
	zassert_equal(suit_plat_fetch_integrated_domain_specific(
			      TEST_COMPONENT_HANDLE, SUIT_COMPONENT_TYPE_CACHE_POOL, &dummy_sink),
		      SUIT_SUCCESS, "suit_plat_fetch_integrated_domain_specific failed");
}
