/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <suit_platform.h>
#include <suit_types.h>
#include <suit_memptr_storage.h>
#include <suit_platform_internal.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <suit_plat_mem_util.h>
#include <suit_memory_layout.h>

#define DFU_PARTITION_OFFSET FIXED_PARTITION_OFFSET(dfu_partition)
#define FLASH_WRITE_ADDR     (suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET))
#define RAM_WRITE_ADDR	     ((uint8_t *)suit_memory_global_address_to_ram_address(0x2003EC00))

static uint8_t test_data[] = {0xDE, 0xAD, 0xBE, 0xEF};

ZTEST_SUITE(write_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(write_tests, test_write_to_flash_sink_OK)
{
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A000FE000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x0F, 0xE0, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_write failed - error %i", ret);

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
	zassert_mem_equal(FLASH_WRITE_ADDR, test_data, sizeof(test_data),
			  "Data in destination is invalid");
}

ZTEST(write_tests, test_write_to_ram_sink_OK)
{
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A20000000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x20, 0x03, 0xEC, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_write failed - error %i", ret);

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
	zassert_mem_equal(RAM_WRITE_ADDR, test_data, sizeof(test_data),
			  "Data in destination is invalid");
}

ZTEST(write_tests, test_write_flash_sink_NOK_size_not_aligned)
{
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A000FE000', h'10'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02,
				     0x45, 0x1A, 0x00, 0x0F, 0xE0, 0x00, 0x41, 0x10};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source);
	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_write should have failed on erase");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
}

ZTEST(write_tests, test_write_flash_sink_NOK_handle_released)
{
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};

	/* handle that will be used as destination */
	suit_component_t dst_handle = 0;

	int ret = suit_plat_write(dst_handle, &source);

	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_write should have failed - invalid handle");
}

ZTEST(write_tests, test_write_to_flash_sink_NOK_source_null)
{
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A000FE000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x0F, 0xE0, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, NULL);
	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_write should have failed - source null");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
}

ZTEST(write_tests, test_write_to_flash_sink_NOK_source_value_null)
{
	struct zcbor_string source = {.value = NULL, .len = sizeof(test_data)};

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A000FE000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x0F, 0xE0, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source);
	zassert_not_equal(ret, SUIT_SUCCESS,
			  "suit_plat_write should have failed - source value null");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
}

ZTEST(write_tests, test_write_to_flash_sink_NOK_source_len_0)
{
	struct zcbor_string source = {.value = test_data, .len = 0};

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A000FE000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x0F, 0xE0, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source);
	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_write should have failed - source len 0");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
}
