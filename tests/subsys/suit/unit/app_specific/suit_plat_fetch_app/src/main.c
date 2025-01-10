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

/* clang-format off */
static const uint8_t valid_manifest_component[] = {
	0x82, /* array: 2 elements */
		0x4c, /* byte string: 12 bytes */
			0x6b, /* string: 11 characters */
				'I', 'N', 'S', 'T', 'L', 'D', '_', 'M', 'F', 'S', 'T',
		0x50, /* byte string: 16 bytes */
			/* RFC4122 uuid5(nordic_vid, 'test_sample_root') */
			0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1,
			0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b, 0x0a,

};
/* clang-format on */

static struct zcbor_string valid_manifest_component_id = {
	.value = valid_manifest_component,
	.len = sizeof(valid_manifest_component),
};

static suit_plat_err_t suit_dfu_cache_sink_get_dummy(struct stream_sink *sink,
						     uint8_t cache_partition_id, const uint8_t *uri,
						     size_t uri_size, bool write_enabled)
{
	struct stream_sink dummy_sink = {0};

	zassert_not_equal(sink, NULL,
			  "The API must provide a valid pointer, to fill sink structure");
	zassert_equal(write_enabled, true, "Dry-run flag enabled in regular scenario");
	*sink = dummy_sink;

	return SUIT_PLAT_SUCCESS;
}

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
	zassert_false(
		suit_plat_fetch_domain_specific_is_type_supported(SUIT_COMPONENT_TYPE_CAND_IMG),
		"suit_plat_fetch_domain_specific_is_type_supported returned false for supported "
		"type");
	zassert_false(
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
	zassert_false(suit_plat_fetch_integrated_domain_specific_is_type_supported(
			      SUIT_COMPONENT_TYPE_CAND_IMG),
		      "suit_plat_fetch_integrated_domain_specific_is_type_supported returned false "
		      "for supported type");
	zassert_false(suit_plat_fetch_integrated_domain_specific_is_type_supported(
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
	struct zcbor_string dummy_uri = {.value = (const uint8_t *)"test", .len = 4};

	zassert_equal(suit_plat_fetch_domain_specific(TEST_COMPONENT_HANDLE,
						      SUIT_COMPONENT_TYPE_UNSUPPORTED, &dummy_uri,
						      &valid_manifest_component_id, NULL),
		      SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "Unsupported component type returned incorrect error code");
}

ZTEST(suit_platform_app_fetch_tests, test_fetch_fetch_source_streamer_fail)
{
	struct zcbor_string dummy_uri = {.value = (const uint8_t *)"test", .len = 4};

	/* Pretend that component handle is valid. */
	suit_plat_component_id_get_fake.return_val = SUIT_SUCCESS;
	suit_plat_decode_component_number_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_dfu_cache_sink_get_fake.custom_fake = suit_dfu_cache_sink_get_dummy;
	suit_fetch_source_stream_fake.return_val = SUIT_PLAT_ERR_IO;

	zassert_equal(suit_plat_fetch_domain_specific(TEST_COMPONENT_HANDLE,
						      SUIT_COMPONENT_TYPE_CACHE_POOL, &dummy_uri,
						      &valid_manifest_component_id, NULL),
		      SUIT_SUCCESS, "Failed when URI not found in cache (should succeed)");
	zassert_equal(suit_fetch_source_stream_fake.call_count, 1,
		      "Incorrect number of suit_fetch_source_stream() calls");
	zassert_equal(suit_fetch_source_stream_fake.arg0_val, dummy_uri.value,
		      "Incorrect value streamer argument");
	zassert_equal(suit_fetch_source_stream_fake.arg1_val, dummy_uri.len,
		      "Incorrect value of streamer argument");

	zassert_equal(suit_dfu_cache_sink_commit_fake.call_count, 0,
		      "Incorrect number of suit_dfu_cache_sink_commit() calls");
	zassert_equal(suit_dfu_cache_streamer_stream_fake.call_count, 0,
		      "Incorrect number of suit_dfu_cache_streamer_stream() calls");
}

ZTEST(suit_platform_app_fetch_tests, test_fetch_fetch_source_streamer_success)
{
	struct zcbor_string dummy_uri = {.value = (const uint8_t *)"test", .len = 4};

	suit_dfu_cache_streamer_stream_fake.return_val = SUIT_PLAT_SUCCESS;

	/* Pretend that component handle is valid. */
	suit_plat_component_id_get_fake.return_val = SUIT_SUCCESS;
	suit_plat_decode_component_number_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_dfu_cache_sink_get_fake.custom_fake = suit_dfu_cache_sink_get_dummy;
	suit_fetch_source_stream_fake.return_val = SUIT_PLAT_SUCCESS;

	zassert_equal(suit_plat_fetch_domain_specific(TEST_COMPONENT_HANDLE,
						      SUIT_COMPONENT_TYPE_CACHE_POOL, &dummy_uri,
						      &valid_manifest_component_id, NULL),
		      SUIT_SUCCESS, "suit_plat_fetch_domain_specific() failed");
	zassert_equal(suit_fetch_source_stream_fake.call_count, 1,
		      "Incorrect number of suit_fetch_source_stream() calls");
	zassert_equal(suit_fetch_source_stream_fake.arg0_val, dummy_uri.value,
		      "Incorrect value streamer argument");
	zassert_equal(suit_fetch_source_stream_fake.arg1_val, dummy_uri.len,
		      "Incorrect value of streamer argument");
	zassert_equal(suit_dfu_cache_sink_commit_fake.call_count, 1,
		      "Incorrect number of suit_dfu_cache_sink_commit() calls");

	zassert_equal(suit_dfu_cache_streamer_stream_fake.call_count, 0,
		      "Incorrect number of suit_dfu_cache_streamer_stream() calls");
}
