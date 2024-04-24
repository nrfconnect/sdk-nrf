/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <suit_sink.h>
#include <suit_platform.h>
#include <suit_dfu_cache_sink.h>
#include <suit_dfu_cache_rw.h>
#include <suit_plat_mem_util.h>
#include <suit_types.h>
#include <suit_metadata.h>
#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

static uint8_t uri[] = "http://databucket.com";
static uint8_t data[] = {0x43, 0x60, 0x02, 0x11, 0x35, 0x85, 0x37, 0x85, 0x76,
			 0x44, 0x09, 0xDE, 0xAD, 0x44, 0x45, 0x42, 0x66, 0x25,
			 0x12, 0x36, 0x84, 0x00, 0x08, 0x61, 0x17};
static uint8_t valid_digest[] = {0x0c, 0xfd, 0xd2, 0x46, 0xb1, 0x84, 0x77, 0x9e, 0xc0, 0xb5, 0x0b,
				 0xca, 0xf1, 0xea, 0x0d, 0xaf, 0x2b, 0x18, 0xa5, 0xbe, 0x5b, 0x61,
				 0x24, 0xc2, 0x65, 0x1c, 0xa9, 0xd7, 0x29, 0x96, 0x37, 0xa8};
static struct zcbor_string exp_digest = {
	.value = valid_digest,
	.len = sizeof(valid_digest),
};

static uint8_t invalid_digest[] = {
	0x0c, 0xfd, 0xd2, 0x46, 0xb1, 0x84, 0x77, 0x9e, 0xc0, 0xb5, 0x0b,
	0xca, 0xf1, 0xea, 0x0d, 0xaf, 0x2b, 0x18, 0xa5, 0xbe, 0x5b, 0x61,
	0x24, 0xc2, 0x65, 0x1c, 0xa9, 0xd7, 0x29, 0x96, 0x37, 0xa7}; /* Last byte changed */
static struct zcbor_string unexp_digest = {
	.value = invalid_digest,
	.len = sizeof(invalid_digest),
};

/* Mocks of functions provided via SSF services */
FAKE_VALUE_FUNC(int, suit_plat_component_compatibility_check, suit_manifest_class_id_t *,
		struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_authorize_sequence_num, enum suit_command_sequence,
		struct zcbor_string *, unsigned int);
FAKE_VALUE_FUNC(int, suit_plat_authorize_unsigned_manifest, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_authenticate_manifest, struct zcbor_string *, enum suit_cose_alg,
		struct zcbor_string *, struct zcbor_string *, struct zcbor_string *);

void setup_dfu_test_cache(void *f)
{
	int ret = suit_dfu_cache_rw_initialize(NULL, 0);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to initialize cache: %i", ret);
}

void clear_dfu_test_partitions(void *f)
{
	/* Erase the area, to meet the preconditions in the next test. */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to erase area");

	int rc = flash_erase(fdev, FIXED_PARTITION_OFFSET(dfu_cache_partition_1),
			     FIXED_PARTITION_SIZE(dfu_cache_partition_1));

	zassert_equal(rc, 0, "Unable to erase dfu__cache_partition_1 before test execution: %i",
		      rc);

	rc = flash_erase(fdev, FIXED_PARTITION_OFFSET(dfu_cache_partition_3),
			 FIXED_PARTITION_SIZE(dfu_cache_partition_3));
	zassert_equal(rc, 0, "Unable to erase dfu_cache_partition_3 before test execution: %i", rc);

	suit_dfu_cache_rw_deinitialize();
}

ZTEST_SUITE(cache_pool_digest_tests, NULL, NULL, setup_dfu_test_cache, clear_dfu_test_partitions,
	    NULL);

ZTEST(cache_pool_digest_tests, test_cache_get_slot_ok)
{
	struct stream_sink sink;

	int ret = suit_dfu_cache_sink_get(&sink, 1, uri, sizeof(uri), true);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to get sink: %i", ret);

	ret = sink.write(sink.ctx, data, sizeof(data));
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to write to sink: %i", ret);

	ret = suit_dfu_cache_sink_commit(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to commit to cache: %i", ret);

	ret = sink.release(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to release sink: %i", ret);

	const uint8_t *payload = NULL;
	size_t payload_size = 0;
	const uint8_t ok_uri[] = "http://databucket.com";
	size_t uri_size = sizeof("http://databucket.com");

	ret = suit_dfu_cache_search(ok_uri, uri_size, &payload, &payload_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "\nGet from cache failed: %i", ret);
	zassert_equal(payload_size, sizeof(data),
		      "Invalid data size for retrieved payload. payload(%u) data(%u)", payload_size,
		      sizeof(data));

	/* Valid id value for cand_img component */
	static uint8_t valid_cand_img_component_id_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
							      '_',  'I',  'M',	'G', 0x41, 0x00};
	/* Valid cand_img component id */
	struct zcbor_string valid_cand_img_component_id = {
		.value = valid_cand_img_component_id_value,
		.len = sizeof(valid_cand_img_component_id_value),
	};

	suit_component_t dst_component;

	ret = suit_plat_create_component_handle(&valid_cand_img_component_id, &dst_component);
	zassert_equal(SUIT_SUCCESS, ret, "test error - create_component_handle failed: %d", ret);

	struct zcbor_string src_uri = {
		.value = uri,
		.len = sizeof(uri),
	};

	ret = suit_plat_fetch(dst_component, &src_uri);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	ret = suit_plat_check_image_match(dst_component, suit_cose_sha256, &exp_digest);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_check_image_match failed - error %i", ret);

	ret = suit_plat_check_image_match(dst_component, suit_cose_sha256, &unexp_digest);
	zassert_not_equal(ret, SUIT_SUCCESS,
			  "suit_plat_check_image_match should have failed - digest invalid");

	ret = suit_plat_release_component_handle(dst_component);
	zassert_equal(SUIT_SUCCESS, ret, "test error - failed to cleanup component handle: %d",
		      ret);
};
