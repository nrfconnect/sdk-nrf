/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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

#define WRITE_ADDR 0x1A00080000

static uint8_t test_data[] = {0, 1, 2, 3, 4, 5, 6, 7};

ZTEST_SUITE(copy_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(copy_tests, test_integrated_fetch_to_memptr_and_copy_to_msink_OK)
{
	memptr_storage_handle_t handle;
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};

	suit_component_t src_handle;
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_src_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				     '_',  'I',	 'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_src_component_id = {
		.value = valid_src_value,
		.len = sizeof(valid_src_value),
	};

	int ret = suit_plat_create_component_handle(&valid_src_component_id, &src_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(src_handle, &source);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	ret = suit_plat_component_impl_data_get(src_handle, &handle);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_component_impl_data_get failed - error %i",
		      ret);

	const uint8_t *payload;
	size_t payload_size = 0;

	ret = suit_memptr_storage_ptr_get(handle, &payload, &payload_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "storage.get failed - error %i", ret);
	zassert_equal_ptr(test_data, payload, "Retrieved payload doesn't mach test_data");
	zassert_equal(sizeof(test_data), payload_size,
		      "Retrieved payload_size doesn't mach size of test_data");
	zassert_not_null(payload, "Retrieved payload is NULL");

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A00080000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	ret = suit_plat_create_component_handle(&valid_dst_component_id, &dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_copy(dst_handle, src_handle);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_copy failed - error %i", ret);

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);

	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS, "src_handle release failed - error %i", ret);

	ret = suit_memptr_storage_release(handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "memptr_storage handle release failed - error %i",
		      ret);
}

/**************************************************************************************************/
ZTEST(copy_tests, test_integrated_fetch_to_memptr_and_copy_to_msink_NOK_dst_handle_released)
{
	memptr_storage_handle_t handle = NULL;
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};

	suit_component_t src_handle;
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_src_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				     '_',  'I',	 'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_src_component_id = {
		.value = valid_src_value,
		.len = sizeof(valid_src_value),
	};

	int ret = suit_plat_create_component_handle(&valid_src_component_id, &src_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(src_handle, &source);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	ret = suit_plat_component_impl_data_get(src_handle, &handle);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_component_impl_data_get failed - error %i",
		      ret);

	const uint8_t *payload;
	size_t payload_size = 0;

	ret = suit_memptr_storage_ptr_get(handle, &payload, &payload_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "storage.get failed - error %i", ret);
	zassert_equal_ptr(test_data, payload, "Retrieved payload doesn't mach test_data");
	zassert_equal(sizeof(test_data), payload_size,
		      "Retrieved payload_size doesn't mach size of test_data");
	zassert_not_null(payload, "Retrieved payload is NULL");

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A00080000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	ret = suit_plat_create_component_handle(&valid_dst_component_id, &dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);

	ret = suit_plat_copy(dst_handle, src_handle);
	zassert_not_equal(ret, SUIT_SUCCESS,
			  "suit_plat_copy should have failed - dst_handle released");

	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS, "src_handle release failed - error %i", ret);

	ret = suit_memptr_storage_release(handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "memptr_storage handle release failed - error %i",
		      ret);
}

ZTEST(copy_tests, test_integrated_fetch_to_memptr_and_copy_to_msink_NOK_src_handle_released)
{
	memptr_storage_handle_t handle = NULL;
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};

	suit_component_t src_handle;
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_src_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				     '_',  'I',	 'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_src_component_id = {
		.value = valid_src_value,
		.len = sizeof(valid_src_value),
	};

	int ret = suit_plat_create_component_handle(&valid_src_component_id, &src_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(src_handle, &source);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	ret = suit_plat_component_impl_data_get(src_handle, &handle);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_component_impl_data_get failed - error %i",
		      ret);

	const uint8_t *payload;
	size_t payload_size = 0;

	ret = suit_memptr_storage_ptr_get(handle, &payload, &payload_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "storage.get failed - error %i", ret);
	zassert_equal_ptr(test_data, payload, "Retrieved payload doesn't mach test_data");
	zassert_equal(sizeof(test_data), payload_size,
		      "Retrieved payload_size doesn't mach size of test_data");
	zassert_not_null(payload, "Retrieved payload is NULL");

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A00080000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	ret = suit_plat_create_component_handle(&valid_dst_component_id, &dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS, "src_handle release failed - error %i", ret);

	ret = suit_plat_copy(dst_handle, src_handle);
	zassert_not_equal(ret, SUIT_SUCCESS,
			  "suit_plat_copy should have failed - src_handle released");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "src_handle release failed - error %i", ret);

	ret = suit_memptr_storage_release(handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "memptr_storage handle release failed - error %i",
		      ret);
}

ZTEST(copy_tests, test_integrated_fetch_to_memptr_and_copy_to_msink_NOK_memptr_empty)
{
	suit_component_t src_handle;
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_src_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				     '_',  'I',	 'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_src_component_id = {
		.value = valid_src_value,
		.len = sizeof(valid_src_value),
	};

	int ret = suit_plat_create_component_handle(&valid_src_component_id, &src_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A00080000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	ret = suit_plat_create_component_handle(&valid_dst_component_id, &dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_copy(dst_handle, src_handle);
	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_copy should have failed - memptr empty");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);

	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS, "src_handle release failed - error %i", ret);
}
