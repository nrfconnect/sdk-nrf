/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <suit_sink.h>
#include <suit_dfu_cache_sink.h>
#include <suit_dfu_cache_rw.h>
#include <suit_plat_mem_util.h>

#ifndef CONFIG_BOARD_NATIVE_POSIX
static const uint8_t corrupted_cache_header_ok_size_nok[] = {
	0xBF, /* map(*) */
	0x75, /* text(21) */
	0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x64, 0x61, 0x74, 0x61, 0x62,
	0x75, 0x63, 0x6B, 0x65, 0x74, 0x2E, 0x63, 0x6F, 0x6D, /* "http://databucket.com" */
	0x5A, 0x00, 0x00, 0x00, 0x17,			      /* bytes(23) */
	0x43, 0x60, 0x02, 0x11, 0x35, 0x85, 0x37, 0x85, 0x76, 0x44, 0x09, 0x44,
	0x45, 0x42, 0x66, 0x25, 0x12, 0x36, 0x84, 0x00, 0x08, 0x61, 0x17, /* payload(23) */
	0x60,								  /* Empty padding uri */
	0x4B,								  /* bytes(11) */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* padding bytes */
	0x69,							    /* text(9) */
	0x23, 0x66, 0x69, 0x6C, 0x65, 0x2E, 0x62, 0x69, 0x6E,	    /* "#file.bin" */
	0x5A, 0xFF, 0xFF, 0xFF, 0xFF, /* not updated, corrupted value */
	0x12, 0x35, 0x89, 0x02, 0x31, 0x70, 0x49, 0x81, 0x20, 0x91, 0x62, 0x38,
	0x90, 0x47, 0x60, 0x12, 0x37, 0x84, 0x90, 0x70, 0x18, 0x92, 0x36, 0x51,
	0x92, 0x83, 0x09, 0x86, 0x70, 0x19, 0x23, /* payload(31) */
	0xFF,					  /* primitive(*) - end marker */
	0xFF};					  /* Alignment to erase block size (16 bytes) */

static const uint8_t corrupted_cache_malformed_zcbor[] = {
	0xBF, /* map(*) */
	0x75, /* text(21) */
	0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x64, 0x61, 0x74, 0x61, 0x62,
	0x75, 0x63, 0x6B, 0x65, 0x74, 0x2E, 0x63, 0x6F, 0x6D, /* "http://databucket.com" */
	0x5A, 0x00, 0x00, 0x00, 0x17,			      /* bytes(23) */
	0x43, 0x60, 0x02, 0x11, 0x35, 0x85, 0x37, 0x85, 0x76, 0x44, 0x09, 0x44,
	0x45, 0x42, 0x66, 0x25, 0x12, 0x36, 0x84, 0x00, 0x08, 0x61, 0x17, /* payload(23) */
	0x60,								  /* Empty padding uri */
	0x4B,								  /* bytes(11) */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* padding bytes */
	0x69,							    /* text(9) */
	0x23, 0x66, 0x69, 0x6C, 0x65, 0x2E, 0x62, 0x69, 0x6E,	    /* "#file.bin" */
	0x5A, 0x00, 0x00, 0x00, 0x1E, /* bytes(30) - mismatched with real size (31) */
	0x12, 0x35, 0x89, 0x02, 0x31, 0x70, 0x49, 0x81, 0x20, 0x91, 0x62, 0x38,
	0x90, 0x47, 0x60, 0x12, 0x37, 0x84, 0x90, 0x70, 0x18, 0x92, 0x36, 0x51,
	0x92, 0x83, 0x09, 0x86, 0x70, 0x19, 0x23, /* payload(31) */
	0xFF,					  /* primitive(*) - end marker */
	0xFF};					  /* Alignment to erase block size (16 bytes) */
#endif

static uint8_t uri[] = "http://databucket.com";
static uint8_t data[] = {0x43, 0x60, 0x02, 0x11, 0x35, 0x85, 0x37, 0x85, 0x76,
			 0x44, 0x09, 0xDE, 0xAD, 0x44, 0x45, 0x42, 0x66, 0x25,
			 0x12, 0x36, 0x84, 0x00, 0x08, 0x61, 0x17};

static uint8_t uri2[] = "http://storagehole.com";
static uint8_t data2[] = {0x05, 0x37, 0x14, 0x51, 0x42, 0x99, 0x99, 0x49, 0x46, 0x54, 0x89, 0x28,
			  0x82, 0x17, 0x68, 0x20, 0x97, 0x60, 0x22, 0x04, 0x51, 0x45, 0x23, 0x04};

static uint8_t uri3[] = "#file.bin";
static uint8_t uri4[] = "#file2.bin";
static uint8_t data3[] = {
	0x3b, 0xd1, 0x84, 0x20, 0x54, 0xae, 0x42, 0x71, 0xe2, 0x9e, 0xef, 0x1c, 0xe1, 0x63, 0x96,
	0x75, 0x2d, 0x45, 0xb3, 0x38, 0xd6, 0x2f, 0x31, 0xa9, 0xe9, 0x55, 0xc8, 0x43, 0x61, 0xb7,
	0xbc, 0xc4, 0xa2, 0xf6, 0x5c, 0xb1, 0x8c, 0xc8, 0xe6, 0xf1, 0xf4, 0x2d, 0xce, 0xd3, 0xc9,
	0x8a, 0x30, 0x32, 0x5f, 0x8d, 0xb1, 0x28, 0x4a, 0x3a, 0x7f, 0x2c, 0x9b, 0x6f, 0x67, 0x1e,
	0xbd, 0xb1, 0x71, 0x09, 0xaa, 0x01, 0x89, 0x51, 0x7d, 0xf0, 0xbf, 0xd8, 0xb5, 0x4c, 0x37,
	0xcb, 0xc8, 0xd6, 0x2b, 0xa4, 0xfa, 0x67, 0xa8, 0x3e, 0x21, 0x40, 0xa0, 0xe0, 0x95, 0xfe,
	0x75, 0xc9, 0xb1, 0xa5, 0x26, 0x46, 0x24, 0x7d, 0x6d, 0x8b, 0x24, 0x7e, 0xfb, 0xaa, 0xcf,
	0x8d, 0x7f, 0xcd, 0x9d, 0xef, 0x08, 0x40, 0x7e, 0x9c, 0x47, 0x06, 0xbe, 0x16, 0x50, 0x7c,
	0xfb, 0x7d, 0x91, 0x58, 0x57, 0xf9, 0x37, 0xbe, 0xf1, 0x09, 0x9d, 0x53, 0x64, 0x7e, 0x49,
	0x74, 0x70, 0xe5, 0xc6, 0x2d, 0xac, 0x96, 0x0e, 0xc1, 0x4a, 0x9a, 0xcb, 0x29, 0x92, 0x29,
	0xb5, 0xd2, 0xaf, 0x3f, 0xb7, 0xa5, 0x32, 0x5f, 0xe0, 0x54, 0x6c, 0x3d, 0x05, 0xed, 0x17,
	0xb1, 0xa6, 0xfc, 0xd9, 0xc4, 0x54, 0xc9, 0x4d, 0x6b, 0x28, 0x26, 0xe2, 0x7f, 0xa0, 0x85,
	0x79, 0x4b, 0x1c, 0x72, 0x2e, 0xe7, 0x6f, 0x0e, 0x2b, 0x31, 0x58, 0x92, 0xa5, 0x8a, 0xcf,
	0x44, 0x88, 0x5e, 0x2f, 0xf4, 0x3e, 0xce, 0x7a, 0xf5, 0x93, 0x22, 0x39, 0x38, 0x14, 0xc5,
	0x65, 0xba, 0xb0, 0x8c, 0x3c, 0x06, 0x44, 0x0b, 0x67, 0x47, 0x72, 0x54, 0xf4, 0x8a, 0x06,
	0x58, 0x8a, 0x43, 0x8d, 0xe6, 0xf9, 0xbd, 0xae, 0x43, 0xaa, 0xf2, 0x09, 0xc6, 0x0d, 0x68,
	0x26, 0xa4, 0xef, 0xfd, 0xee, 0xf9, 0x94, 0x3b, 0x4f, 0x8b, 0x2e, 0x73, 0x97, 0x6b, 0x5f,
	0xd5, 0x27, 0x22, 0x94, 0x03, 0x1a, 0x39, 0xbf, 0xb3, 0x77, 0x68, 0x99, 0x54, 0x75, 0xc6,
	0x5b, 0xbd, 0x03, 0x18, 0xb1, 0x85, 0xad, 0xeb, 0x43, 0x04, 0x0d, 0xb1, 0x80, 0xba, 0x51,
	0xa1, 0xda, 0xbb, 0x94, 0x09, 0x80, 0xf9, 0x49, 0x23, 0xec, 0x3d, 0x13, 0x0d, 0x81, 0xeb,
	0x49, 0x00, 0x5d, 0xad, 0xf1, 0xad, 0xb9, 0x7b, 0xa8, 0xfe, 0x65, 0x78, 0xd4, 0x93, 0xa5,
	0x5f, 0x2e, 0x11, 0x9c, 0x0c, 0xbe, 0x37, 0xa0, 0xd5, 0x87, 0xc0, 0x1d, 0xc6, 0xbf, 0x80,
	0x78, 0xc2, 0x75, 0x91, 0x6f, 0xd0, 0x99, 0xb1, 0x84, 0x40, 0x14, 0xb6, 0x78, 0x8c, 0x07,
	0x0b, 0x5d, 0x6c, 0xc0, 0x6e, 0xab, 0x68, 0xb7, 0x39, 0x8e, 0x4c, 0xef, 0xdb, 0xac, 0xfc,
	0x31, 0x3d, 0x46, 0x12, 0x4a, 0x4c, 0xe2, 0x54, 0x83, 0x7c, 0xc6, 0xb3, 0x77, 0xfc, 0x35,
	0xba, 0xc6, 0x57, 0x4d, 0x68, 0xe5, 0x44, 0xa4, 0x8e, 0xb4, 0x70, 0xc4, 0xf9, 0xb5, 0xd4,
	0xad, 0xbe, 0xfa, 0x58, 0x76, 0xb3, 0xb2, 0x21, 0x14, 0x4f, 0xf9, 0xfc, 0x48, 0x2c, 0xba,
	0x1a, 0x20, 0xf4, 0x13, 0x23, 0x53, 0x38, 0x9e, 0x3a, 0xd0, 0x7e, 0x38, 0x68, 0x21, 0x9d,
	0x98, 0x52, 0x3b, 0x60, 0xb3, 0xcc, 0x70, 0x9c, 0xf9, 0x08, 0xa1, 0x33, 0xaa, 0x69, 0xe1,
	0x0f, 0x7a, 0x73, 0xf4, 0x1a, 0xef, 0x55, 0xb7, 0x90, 0xf9, 0x7d, 0x00, 0xec, 0xb4, 0x8c,
	0x29, 0x1d, 0x50, 0xa9, 0x3e, 0x15, 0x24, 0x26, 0xe9, 0x1c, 0xde, 0x41, 0xed, 0xd4, 0x55,
	0x96, 0xe7, 0x15, 0x55, 0x5e, 0xd8, 0x0c, 0xfd, 0xe0, 0x4c, 0xa7, 0xa1, 0xb4, 0xd6, 0xee,
	0x6d, 0x0e, 0x0d, 0x42, 0x79, 0x93, 0xe2, 0x87, 0x2d, 0x0f, 0xee, 0x73, 0xaf, 0x6b, 0x4e,
	0x2e, 0x09, 0x7d, 0xf0, 0x5c, 0x0c, 0xa9, 0xa5, 0x9a, 0x8a, 0xe0, 0xa3, 0xa8, 0x23, 0xc1,
	0x41, 0x4e, 0x5e, 0x3f, 0xf7, 0x39, 0xa4, 0xc5, 0x5b, 0xec};

void setup_dfu_test_cache(void *f)
{
	int ret = suit_dfu_cache_rw_initialize(NULL, 0);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to initialize cache: %i", ret);
}

#ifndef CONFIG_BOARD_NATIVE_POSIX
void setup_dfu_test_corrupted_cache(const uint8_t *corrupted_cache, size_t corrupted_cache_size)
{
	/* Erase the area, to met the preconditions in the next test. */
	const struct device *fdev = SUIT_PLAT_INTERNAL_NVM_DEV;

	zassert_not_null(fdev, "Unable to find a driver to erase area");

	int rc = flash_write(fdev, FIXED_PARTITION_OFFSET(dfu_cache_partition_1), corrupted_cache,
			     corrupted_cache_size);
	zassert_equal(rc, 0, "Unable to write corrupted_cache before test execution: %i", rc);

	printk("dfu_cache_partition_1 address: %p\n",
	       suit_plat_mem_nvm_ptr_get(FIXED_PARTITION_OFFSET(dfu_cache_partition_1)));

	int res = memcmp(corrupted_cache,
			 suit_plat_mem_nvm_ptr_get(FIXED_PARTITION_OFFSET(dfu_cache_partition_1)),
			 corrupted_cache_size);
	zassert_equal(res, 0, "Mem compare after write failed");

	setup_dfu_test_cache(NULL);
}
#endif

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

bool is_cache_partition_1_empty(void)
{
	const uint8_t *partition_address =
		suit_plat_mem_nvm_ptr_get(FIXED_PARTITION_OFFSET(dfu_cache_partition_1));
	const size_t partition_size = FIXED_PARTITION_SIZE(dfu_cache_partition_1);

	for (size_t i = 0; i < partition_size; i++) {
		if (partition_address[i] != 0xFF) {
			return false;
		}
	}

	return true;
}

ZTEST_SUITE(cache_rw_initialization_tests, NULL, NULL, NULL, clear_dfu_test_partitions, NULL);

ZTEST(cache_rw_initialization_tests, test_cache_initialization_size_nok)
{
	uint8_t *envelope_address =
		suit_plat_mem_nvm_ptr_get(FIXED_PARTITION_OFFSET(dfu_partition)) + 256;
	size_t envelope_size = FIXED_PARTITION_SIZE(dfu_partition);

	int ret = suit_dfu_cache_rw_initialize(envelope_address, envelope_size);

	zassert_not_equal(ret, SUIT_PLAT_SUCCESS,
			  "Initialization should have failed: size out of bounds");
}

ZTEST(cache_rw_initialization_tests, test_cache_initialization_address_nok)
{
	uint8_t *envelope_address =
		suit_plat_mem_nvm_ptr_get(FIXED_PARTITION_OFFSET(dfu_partition)) - 256;
	size_t envelope_size = FIXED_PARTITION_SIZE(dfu_partition);

	int ret = suit_dfu_cache_rw_initialize(envelope_address, envelope_size);

	zassert_not_equal(ret, SUIT_PLAT_SUCCESS,
			  "Initialization should have failed: address out of bounds");
}

ZTEST(cache_rw_initialization_tests, test_cache_initialization_ok)
{
	uint8_t *envelope_address =
		suit_plat_mem_nvm_ptr_get(FIXED_PARTITION_OFFSET(dfu_partition)) + 256;
	size_t envelope_size = FIXED_PARTITION_SIZE(dfu_partition) - 1024;

	int ret = suit_dfu_cache_rw_initialize(envelope_address, envelope_size);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Initialization failed: %i", ret);
}

#ifndef CONFIG_BOARD_NATIVE_POSIX
ZTEST_SUITE(cache_sink_recovery_tests, NULL, NULL, NULL,
	    clear_dfu_test_partitions, NULL);

ZTEST(cache_sink_recovery_tests, test_cache_recovery_header_ok_size_nok)
{
	struct stream_sink sink;

	setup_dfu_test_corrupted_cache(corrupted_cache_header_ok_size_nok,
				       sizeof(corrupted_cache_header_ok_size_nok));

	zassert_true(is_cache_partition_1_empty(), "Corrupted cache partition was not recovered");

	int ret = suit_dfu_cache_sink_get(&sink, 1, uri3, sizeof(uri3), true);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to get sink: %i", ret);

	ret = sink.write(sink.ctx, data, sizeof(data));
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to write to sink: %i", ret);

	ret = suit_dfu_cache_sink_commit(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to commit to cache: %i", ret);

	ret = sink.release(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to release sink: %i", ret);
}

ZTEST(cache_sink_recovery_tests, test_cache_recovery_malformed_zcbor)
{
	setup_dfu_test_corrupted_cache(corrupted_cache_malformed_zcbor,
				       sizeof(corrupted_cache_malformed_zcbor));

	zassert_true(is_cache_partition_1_empty(), "Corrupted cache partition was not recovered");
}

#endif

ZTEST_SUITE(cache_sink_tests, NULL, NULL, setup_dfu_test_cache, clear_dfu_test_partitions, NULL);

ZTEST(cache_sink_tests, test_cache_drop_slot_ok)
{
	struct stream_sink sink;

	int ret = suit_dfu_cache_sink_get(&sink, 1, uri, sizeof(uri), true);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to get sink: %i", ret);

	ret = sink.write(sink.ctx, data, sizeof(data));
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to write to sink: %i", ret);

	ret = suit_dfu_cache_sink_drop(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to drop cache: %i", ret);

	ret = sink.release(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to release sink: %i", ret);

	ret = suit_dfu_cache_sink_get(&sink, 1, uri2, sizeof(uri2), true);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to get sink: %i", ret);

	ret = sink.write(sink.ctx, data2, sizeof(data2));
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to write to sink: %i", ret);

	ret = suit_dfu_cache_sink_commit(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to commit to cache: %i", ret);

	ret = sink.release(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to release sink: %i", ret);

	const uint8_t *payload = NULL;
	size_t payload_size = 0;
	const uint8_t ok_uri[] = "http://storagehole.com";
	size_t uri_size = sizeof("http://storagehole.com");

	ret = suit_dfu_cache_search(ok_uri, uri_size, &payload, &payload_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "\nGet from cache failed: %i", ret);
	zassert_equal(payload_size, sizeof(data2),
		      "Invalid data size for retrieved payload. payload(%u) data(%u)", payload_size,
		      sizeof(data));
}

ZTEST(cache_sink_tests, test_cache_get_slot_ok)
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

	ret = suit_dfu_cache_sink_get(&sink, 3, uri2, sizeof(uri2), true);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to get sink: %i", ret);

	ret = sink.write(sink.ctx, data2, sizeof(data2));
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to write to sink: %i", ret);

	ret = suit_dfu_cache_sink_commit(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to commit to cache: %i", ret);

	ret = sink.release(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to release sink: %i", ret);

	const uint8_t *ok_uri2 = "http://storagehole.com";
	size_t uri_size2 = sizeof("http://storagehole.com");

	ret = suit_dfu_cache_search(ok_uri2, uri_size2, &payload, &payload_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "\nGet from cache failed: %i", ret);
	zassert_equal(payload_size, sizeof(data2),
		      "Invalid data size for retrieved payload. payload(%u) data(%u)", payload_size,
		      sizeof(data));
}

ZTEST(cache_sink_tests, test_cache_get_slot_nok_uri_exists)
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

	/* Request sink for dfu_cache_partition_3 but it should fail because dfu_cache_partition_1
	 *	already has same uri written
	 */
	ret = suit_dfu_cache_sink_get(&sink, 3, uri, sizeof(uri), true);
	zassert_not_equal(ret, SUIT_PLAT_SUCCESS,
			  "Get cache sink should have failed, uri already should be in cache: %i",
			  ret);
}

ZTEST(cache_sink_tests, test_cache_get_slot_nok_no_requested_cache)
{
	struct stream_sink sink;

	/* Request sink for suit_cache_4, which should fail because suit_cache_4 was not defined */
	int ret = suit_dfu_cache_sink_get(&sink, 4, uri, sizeof(uri), false);

	zassert_not_equal(ret, SUIT_PLAT_SUCCESS,
			  "Get cache sink should have failed, suit_cache_4 not defined: %i", ret);

	ret = suit_dfu_cache_sink_get(&sink, 4, uri, sizeof(uri), true);
	zassert_not_equal(ret, SUIT_PLAT_SUCCESS,
			  "Get cache sink should have failed, suit_cache_4 not defined: %i", ret);
}

ZTEST(cache_sink_tests, test_cache_get_slot_nok_not_enough_space)
{
	struct stream_sink sink;

	int ret = suit_dfu_cache_sink_get(&sink, 1, uri3, sizeof(uri3), true);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to get sink: %i", ret);

	for (size_t i = 0x10 + sizeof(data3); i < FIXED_PARTITION_SIZE(dfu_cache_partition_1);
	     i += sizeof(data3)) {
		ret = sink.write(sink.ctx, data3, sizeof(data3));
		zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to write to sink: %i", ret);
	}

	ret = suit_dfu_cache_sink_commit(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to commit to cache: %i", ret);

	ret = sink.release(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to release sink: %i", ret);

#ifdef CONFIG_BOARD_NATIVE_POSIX
	ret = suit_dfu_cache_sink_get(&sink, 1, uri4, sizeof(uri4), true);
	zassert_not_equal(ret, SUIT_PLAT_SUCCESS,
			  "Get sink should have failed due to lack of free space: %i", ret);
#else
	ret = suit_dfu_cache_sink_get(&sink, 1, uri4, sizeof(uri4), true);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to get sink: %i", ret);

	ret = sink.write(sink.ctx, data3, sizeof(data3));
	zassert_not_equal(ret, SUIT_PLAT_SUCCESS,
			  "Write to sink should fail due to lack of space: %i", ret);
#endif /* CONFIG_BOARD_NATIVE_POSIX */

	printk("\nReleasing sink\n");
	ret = sink.release(sink.ctx);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to release sink: %i", ret);
}
