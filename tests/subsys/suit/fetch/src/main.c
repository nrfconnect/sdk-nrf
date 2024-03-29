/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <suit_memptr_storage.h>
#include <suit_platform_internal.h>
#include <suit_dfu_cache.h>

#define TEST_DATA_SIZE 64
#define WRITE_ADDR     0x1A00080000

static uint8_t test_data[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
			      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
			      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
			      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

/*
 * {
 *    "http://source1.com": h'4235623423462346456234623487723572702975',
 *    "http://source2.com.no": h'25672519384710',
 *    "ftp://altsource.com": h'92384859284720'
 * }
 */
static const uint8_t cache[] = {
	0xBF, 0x72, 0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x73, 0x6F, 0x75, 0x72, 0x63, 0x65,
	0x31, 0x2E, 0x63, 0x6F, 0x6D, 0x54, 0x42, 0x35, 0x62, 0x34, 0x23, 0x46, 0x23, 0x46, 0x45,
	0x62, 0x34, 0x62, 0x34, 0x87, 0x72, 0x35, 0x72, 0x70, 0x29, 0x75, 0x75, 0x68, 0x74, 0x74,
	0x70, 0x3A, 0x2F, 0x2F, 0x73, 0x6F, 0x75, 0x72, 0x63, 0x65, 0x32, 0x2E, 0x63, 0x6F, 0x6D,
	0x2E, 0x6E, 0x6F, 0x47, 0x25, 0x67, 0x25, 0x19, 0x38, 0x47, 0x10, 0x73, 0x66, 0x74, 0x70,
	0x3A, 0x2F, 0x2F, 0x61, 0x6C, 0x74, 0x73, 0x6F, 0x75, 0x72, 0x63, 0x65, 0x2E, 0x63, 0x6F,
	0x6D, 0x47, 0x92, 0x38, 0x48, 0x59, 0x28, 0x47, 0x20, 0xFF};
static const size_t cache_len = sizeof(cache);

/*
 * {
 *    "http://databucket.com": h'4360021135853785764409444542662512368400086117',
 *    "http://storagehole.com": h'053714514299994946548928821768209760220451452304',
 *    "#file.bin": h'12358902317049812091623890476012378490701892365192830986701923'
 * }
 */
static const uint8_t cache2[] = {
	0xA3, 0x75, 0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x64, 0x61, 0x74, 0x61, 0x62,
	0x75, 0x63, 0x6B, 0x65, 0x74, 0x2E, 0x63, 0x6F, 0x6D, 0x57, 0x43, 0x60, 0x02, 0x11,
	0x35, 0x85, 0x37, 0x85, 0x76, 0x44, 0x09, 0x44, 0x45, 0x42, 0x66, 0x25, 0x12, 0x36,
	0x84, 0x00, 0x08, 0x61, 0x17, 0x76, 0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x73,
	0x74, 0x6F, 0x72, 0x61, 0x67, 0x65, 0x68, 0x6F, 0x6C, 0x65, 0x2E, 0x63, 0x6F, 0x6D,
	0x58, 0x18, 0x05, 0x37, 0x14, 0x51, 0x42, 0x99, 0x99, 0x49, 0x46, 0x54, 0x89, 0x28,
	0x82, 0x17, 0x68, 0x20, 0x97, 0x60, 0x22, 0x04, 0x51, 0x45, 0x23, 0x04, 0x69, 0x23,
	0x66, 0x69, 0x6C, 0x65, 0x2E, 0x62, 0x69, 0x6E, 0x58, 0x1F, 0x12, 0x35, 0x89, 0x02,
	0x31, 0x70, 0x49, 0x81, 0x20, 0x91, 0x62, 0x38, 0x90, 0x47, 0x60, 0x12, 0x37, 0x84,
	0x90, 0x70, 0x18, 0x92, 0x36, 0x51, 0x92, 0x83, 0x09, 0x86, 0x70, 0x19, 0x23};
static const size_t cache2_len = sizeof(cache2);

static void setup_cache_with_sample_entries(void)
{
	struct dfu_cache dfu_caches;

	zassert_between_inclusive(2, 1, CONFIG_SUIT_CACHE_MAX_CACHES,
				  "Failed to prepare test fixture: cache istoo small");

	dfu_caches.pools[0].address = (uint8_t *)cache;
	dfu_caches.pools[0].size = (size_t)cache_len;

	dfu_caches.pools[1].address = (uint8_t *)cache2;
	dfu_caches.pools[1].size = (size_t)cache2_len;
	dfu_caches.pools_count = 2;

	suit_plat_err_t rc = suit_dfu_cache_initialize(&dfu_caches);

	zassert_equal(rc, SUIT_PLAT_SUCCESS, "Failed to initialize cache: %i", rc);
}

ZTEST_SUITE(fetch_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(fetch_tests, test_integrated_fetch_to_memptr_OK)
{
	suit_component_t component_handle;
	memptr_storage_handle_t handle = NULL;
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				 '_',  'I',  'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, &component_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(component_handle, &source);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	ret = suit_plat_component_impl_data_get(component_handle, &handle);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_component_impl_data_get failed - error %i",
		      ret);

	const uint8_t *payload;
	size_t payload_size = 0;

	ret = suit_memptr_storage_ptr_get(handle, &payload, &payload_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "storage.get failed - error %i", ret);
	zassert_equal(test_data, payload, "Retrieved payload doesn't mach test_data");
	zassert_equal(sizeof(test_data), payload_size,
		      "Retrieved payload_size doesn't mach size of test_data");

	ret = suit_plat_release_component_handle(component_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Handle release failed - error %i", ret);
}

ZTEST(fetch_tests, test_fetch_to_memptr_OK)
{
	suit_component_t component_handle;
	struct zcbor_string uri = {.value = "http://databucket.com",
				   .len = sizeof("http://databucket.com")};
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				 '_',  'I',  'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, &component_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	setup_cache_with_sample_entries();

	ret = suit_plat_fetch(component_handle, &uri);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	ret = suit_plat_release_component_handle(component_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Handle release failed - error %i", ret);

	suit_dfu_cache_deinitialize();
}

ZTEST(fetch_tests, test_fetch_to_memptr_NOK_uri_not_in_cache)
{
	suit_component_t component_handle;
	struct zcbor_string uri = {.value = "http://databucket.no",
				   .len = sizeof("http://databucket.no")};
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				 '_',  'I',  'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, &component_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	setup_cache_with_sample_entries();

	ret = suit_plat_fetch(component_handle, &uri);
	zassert_not_equal(ret, SUIT_SUCCESS,
			  "suit_plat_fetch should fail - supplied uri is not in cache", ret);

	ret = suit_plat_release_component_handle(component_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Handle release failed - error %i", ret);

	suit_dfu_cache_deinitialize();
}

ZTEST(fetch_tests, test_fetch_to_memptr_NOK_invalid_component_id)
{
	suit_component_t component_handle;
	struct zcbor_string uri = {.value = "http://databucket.com",
				   .len = sizeof("http://databucket.com")};
	/* [h'MEM', h'01', h'1A00080000', h'08'] */
	uint8_t valid_value[] = {0x84, 0x44, 0x63, 'M',	 'E',  'M',  0x41, 0x01,
				 0x45, 0x1A, 0x00, 0x08, 0x00, 0x00, 0x41, 0x08};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, &component_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	setup_cache_with_sample_entries();

	ret = suit_plat_fetch(component_handle, &uri);
	zassert_not_equal(ret, SUIT_SUCCESS,
			  "suit_plat_fetch should have failed - invalid component_id");

	ret = suit_plat_release_component_handle(component_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Handle release failed - error %i", ret);

	suit_dfu_cache_deinitialize();
}

ZTEST(fetch_tests, test_integrated_fetch_to_memptr_NOK_data_ptr_NULL)
{
	suit_component_t component_handle;
	struct zcbor_string source = {.value = NULL, .len = sizeof(test_data)};
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				 '_',  'I',  'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, &component_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(component_handle, &source);
	zassert_not_equal(ret, SUIT_SUCCESS,
			  "suit_plat_fetch should have failed - NULL data pointer");

	ret = suit_plat_release_component_handle(component_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Handle release failed - error %i", ret);
}

ZTEST(fetch_tests, test_integrated_fetch_to_memptr_NOK_data_size_zero)
{
	suit_component_t component_handle;
	struct zcbor_string source = {.value = test_data, .len = 0};
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				 '_',  'I',  'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, &component_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(component_handle, &source);
	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_fetch should have failed - data size 0");

	ret = suit_plat_release_component_handle(component_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Handle release failed - error %i", ret);
}

ZTEST(fetch_tests, test_integrated_fetch_to_memptr_NOK_handle_NULL)
{
	suit_component_t component_handle;
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				 '_',  'I',  'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, &component_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(component_handle, &source);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	const uint8_t *payload;
	size_t payload_size = 0;

	ret = suit_memptr_storage_ptr_get(NULL, &payload, &payload_size);
	zassert_not_equal(ret, SUIT_PLAT_SUCCESS,
			  "suit_memptr_storage_get should failed - handle NULL");

	ret = suit_plat_release_component_handle(component_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Handle release failed - error %i", ret);
}
