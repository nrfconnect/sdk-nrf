/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <suit_platform.h>
#include <suit_types.h>
#include <suit_memory_layout.h>
#include <suit_plat_ipuc.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <mocks_sdfw.h>

static uint8_t test_data[] = {0, 1, 2, 3, 4, 5, 6, 7};

static suit_component_t ipuc_handle;
/* [h'MEM', h'02', h'1A00080000', h'191000'] */
static uint8_t ipuc_component_id_bstr[] = {0x84, 0x44, 0x63, 'M',  'E',	 'M',  0x41, 0x02, 0x45,
					   0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

static struct zcbor_string ipuc_component_id = {
	.value = ipuc_component_id_bstr,
	.len = sizeof(ipuc_component_id_bstr),
};

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_sdfw_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();

	int ret = suit_plat_create_component_handle(&ipuc_component_id, false, &ipuc_handle);

	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_create_component_handle failed - error %i",
		      ret);

	suit_plat_ipuc_revoke(ipuc_handle);
}

static void test_after(void *data)
{
	int ret = suit_plat_release_component_handle(ipuc_handle);

	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_release_component_handle failed - error %i",
		      ret);
}

ZTEST_SUITE(ipuc_tests, NULL, NULL, test_before, test_after, NULL);

ZTEST(ipuc_tests, test_ipuc_revoke_OK)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	ret = suit_plat_ipuc_declare(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_ipuc_declare failed - error %i", ret);

	ret = suit_plat_ipuc_revoke(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_ipuc_revoke failed - error %i", ret);

	ret = suit_plat_ipuc_revoke(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "suit_plat_ipuc_revoke failed - error %i", ret);
}

ZTEST(ipuc_tests, test_ipuc_copy_OK)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	ret = suit_plat_ipuc_declare(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_ipuc_declare failed - error %i", ret);

	size_t offset = 0;
	bool last_chunk = false;

	ret = suit_plat_ipuc_write(ipuc_handle, offset, (uintptr_t)test_data, sizeof(test_data),
				   last_chunk);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_ipuc_write failed - error %i", ret);

	offset += sizeof(test_data);
	last_chunk = true;
	ret = suit_plat_ipuc_write(ipuc_handle, offset, (uintptr_t)test_data, sizeof(test_data),
				   last_chunk);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_ipuc_write failed - error %i", ret);

	offset += sizeof(test_data);
	size_t stored_img_size = 0;

	ret = suit_plat_ipuc_get_stored_img_size(ipuc_handle, &stored_img_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS,
		      "suit_plat_ipuc_get_stored_img_size failed - error %i", ret);
	zassert_equal(stored_img_size, offset, "incorrect stored image size");

	/* Let's write again
	 */
	offset = 0;
	last_chunk = true;
	ret = suit_plat_ipuc_write(ipuc_handle, offset, (uintptr_t)test_data, sizeof(test_data),
				   last_chunk);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_ipuc_write failed - error %i", ret);

	offset += sizeof(test_data);
	stored_img_size = 0;
	ret = suit_plat_ipuc_get_stored_img_size(ipuc_handle, &stored_img_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS,
		      "suit_plat_ipuc_get_stored_img_size failed - error %i", ret);
	zassert_equal(stored_img_size, offset, "incorrect stored image size");
}

ZTEST(ipuc_tests, test_ipuc_copy_NOK_not_declared)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	size_t offset = 0;
	bool last_chunk = false;

	ret = suit_plat_ipuc_write(ipuc_handle, offset, (uintptr_t)test_data, sizeof(test_data),
				   last_chunk);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "suit_plat_ipuc_write failed - error %i", ret);
}

ZTEST(ipuc_tests, test_ipuc_copy_NOK_wrong_offset)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	ret = suit_plat_ipuc_declare(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_ipuc_declare failed - error %i", ret);

	size_t offset = sizeof(test_data);
	bool last_chunk = false;

	ret = suit_plat_ipuc_write(ipuc_handle, offset, (uintptr_t)test_data, sizeof(test_data),
				   last_chunk);
	zassert_equal(ret, SUIT_PLAT_ERR_INCORRECT_STATE, "suit_plat_ipuc_write failed - error %i",
		      ret);
}

ZTEST(ipuc_tests, test_ipuc_copy_NOK_wrong_state_write_in_progress)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	ret = suit_plat_ipuc_declare(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_ipuc_declare failed - error %i", ret);

	size_t stored_img_size = 0;

	ret = suit_plat_ipuc_get_stored_img_size(ipuc_handle, &stored_img_size);
	zassert_equal(ret, SUIT_PLAT_ERR_INCORRECT_STATE,
		      "suit_plat_ipuc_get_stored_img_size failed - error %i", ret);

	size_t offset = 0;
	bool last_chunk = true;

	ret = suit_plat_ipuc_write(ipuc_handle, offset, 0, 0, last_chunk);

	stored_img_size = 0;
	ret = suit_plat_ipuc_get_stored_img_size(ipuc_handle, &stored_img_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS,
		      "suit_plat_ipuc_get_stored_img_size failed - error %i", ret);
	zassert_equal(stored_img_size, offset, "incorrect stored image size");

	offset = sizeof(test_data);
	ret = suit_plat_ipuc_write(ipuc_handle, offset, (uintptr_t)test_data, sizeof(test_data),
				   last_chunk);

	zassert_equal(ret, SUIT_PLAT_ERR_INCORRECT_STATE, "suit_plat_ipuc_write failed - error %i",
		      ret);
}

ZTEST(ipuc_tests, test_ipuc_sdfw_mirror_OK)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	ret = suit_plat_ipuc_declare(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_ipuc_declare failed - error %i", ret);

	uintptr_t sdfw_update_area_addr = 0;
	size_t sdfw_update_area_size = 0;

	suit_memory_sdfw_update_area_info_get(&sdfw_update_area_addr, &sdfw_update_area_size);

	zassert_not_equal(sdfw_update_area_size, 0, "");

	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;
	arbiter_mem_access_check_fake.call_count = 0;

	uintptr_t mirror_addr = suit_plat_ipuc_sdfw_mirror_addr(16);

	zassert_between_inclusive(mirror_addr, sdfw_update_area_addr,
				  sdfw_update_area_addr + sdfw_update_area_size, "");

	zassert_equal(arbiter_mem_access_check_fake.call_count, 1,
		      "Incorrect number of arbiter_mem_access_check() calls");

	ret = suit_plat_ipuc_revoke(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_ipuc_revoke failed - error %i", ret);

	mirror_addr = suit_plat_ipuc_sdfw_mirror_addr(16);

	zassert_equal(mirror_addr, 0, "valid mirror_addr after suit_plat_ipuc_revoke() call");
}
