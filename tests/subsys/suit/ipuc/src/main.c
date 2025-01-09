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
#include <suit_ipuc_sdfw.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <mocks_sdfw.h>

static uint8_t test_data[] = {0xde, 0xad, 0xbe, 0xef};

static uint8_t test_digest_bstr[] = {0x5f, 0x78, 0xc3, 0x32, 0x74, 0xe4, 0x3f, 0xa9,
				     0xde, 0x56, 0x59, 0x26, 0x5c, 0x1d, 0x91, 0x7e,
				     0x25, 0xc0, 0x37, 0x22, 0xdc, 0xb0, 0xb8, 0xd2,
				     0x7d, 0xb8, 0xd5, 0xfe, 0xaa, 0x81, 0x39, 0x53};

static struct zcbor_string test_digest = {
	.value = test_digest_bstr,
	.len = sizeof(test_digest_bstr),
};

static const enum suit_cose_alg digest_algorithm = suit_cose_sha256;

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

	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_create_component_handle failed - ret: %i", ret);

	suit_ipuc_sdfw_revoke(ipuc_handle);
}

static void test_after(void *data)
{
	int ret = suit_plat_release_component_handle(ipuc_handle);

	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_release_component_handle failed - ret: %i",
		      ret);
}

ZTEST_SUITE(ipuc_tests, NULL, NULL, test_before, test_after, NULL);

ZTEST(ipuc_tests, test_ipuc_revoke_OK)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	suit_manifest_role_t role = SUIT_MANIFEST_APP_LOCAL_1;

	ret = suit_ipuc_sdfw_declare(ipuc_handle, role);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_declare failed - ret: %i", ret);

	ret = suit_ipuc_sdfw_revoke(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_revoke failed - ret: %i", ret);

	ret = suit_ipuc_sdfw_revoke(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "suit_ipuc_sdfw_revoke failed - ret: %i", ret);
}

ZTEST(ipuc_tests, test_ipuc_write_OK)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	suit_manifest_role_t role = SUIT_MANIFEST_APP_LOCAL_1;
	int ipc_client_id = 1234;

	ret = suit_ipuc_sdfw_declare(ipuc_handle, role);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_declare failed - ret: %i", ret);

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &ipuc_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write_setup failed - ret: %i", ret);

	size_t offset = 2;
	size_t chunk_size = 2;
	bool last_chunk = false;

	ret = suit_ipuc_sdfw_write(ipc_client_id, &ipuc_component_id, offset,
				   (uintptr_t)test_data + offset, chunk_size, last_chunk);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write failed - ret: %i", ret);

	offset = 0;
	chunk_size = 2;
	last_chunk = true;

	ret = suit_ipuc_sdfw_write(ipc_client_id, &ipuc_component_id, offset,
				   (uintptr_t)test_data + offset, chunk_size, last_chunk);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write failed - ret: %i", ret);

	ret = suit_ipuc_sdfw_digest_compare(&ipuc_component_id, digest_algorithm, &test_digest);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_digest_compare failed - ret: %i",
		      ret);

	/* Let's check if suit_ipuc_sdfw_write_setup erases IPUC
	 */
	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &ipuc_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write_setup failed - ret: %i", ret);

	offset = 2;
	chunk_size = 1;
	last_chunk = true;

	ret = suit_ipuc_sdfw_write(ipc_client_id, &ipuc_component_id, offset,
				   (uintptr_t)test_data + offset, chunk_size, last_chunk);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write failed - ret: %i", ret);

	ret = suit_ipuc_sdfw_digest_compare(&ipuc_component_id, digest_algorithm, &test_digest);
	zassert_equal(ret, SUIT_PLAT_ERR_INVAL,
		      "suit_ipuc_sdfw_digest_compare - wrong return code - ret: %i", ret);

	/* Let's check 2nd method of last_chunk signallization
	 */
	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &ipuc_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write_setup failed - ret: %i", ret);

	offset = 2;
	chunk_size = 2;
	last_chunk = false;

	ret = suit_ipuc_sdfw_write(ipc_client_id, &ipuc_component_id, offset,
				   (uintptr_t)test_data + offset, chunk_size, last_chunk);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write failed - ret: %i", ret);

	offset = 0;
	chunk_size = 2;
	last_chunk = false;

	ret = suit_ipuc_sdfw_write(ipc_client_id, &ipuc_component_id, offset,
				   (uintptr_t)test_data + offset, chunk_size, last_chunk);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write failed - ret: %i", ret);

	offset = 0;
	chunk_size = 0;
	last_chunk = true;

	ret = suit_ipuc_sdfw_write(ipc_client_id, &ipuc_component_id, offset, 0, chunk_size,
				   last_chunk);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write failed - ret: %i", ret);

	ret = suit_ipuc_sdfw_digest_compare(&ipuc_component_id, digest_algorithm, &test_digest);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_digest_compare failed - ret: %i",
		      ret);
}

ZTEST(ipuc_tests, test_ipuc_write_setup_NOK_not_declared)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	int ipc_client_id = 1234;

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &ipuc_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "suit_ipuc_sdfw_write_setup failed - ret: %i",
		      ret);
}

ZTEST(ipuc_tests, test_ipuc_write_setup_NOK_busy_other_client)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	suit_manifest_role_t role = SUIT_MANIFEST_APP_LOCAL_1;
	int ipc_client_id = 1234;

	ret = suit_ipuc_sdfw_declare(ipuc_handle, role);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_declare failed - ret: %i", ret);

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &ipuc_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write_setup failed - ret: %i", ret);

	ipc_client_id++;

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &ipuc_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_ERR_INCORRECT_STATE,
		      "suit_ipuc_sdfw_write_setup wrong return code - ret: %i", ret);
}

ZTEST(ipuc_tests, test_ipuc_write_setup_NOK_busy_sdfw_mirror)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	suit_manifest_role_t role = SUIT_MANIFEST_APP_LOCAL_1;
	int ipc_client_id = 1234;

	ret = suit_ipuc_sdfw_declare(ipuc_handle, role);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_declare failed - ret: %i", ret);

	uintptr_t sdfw_update_area_addr = 0;
	size_t sdfw_update_area_size = 0;

	suit_memory_sdfw_update_area_info_get(&sdfw_update_area_addr, &sdfw_update_area_size);

	zassert_not_equal(sdfw_update_area_size, 0, "");

	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;
	arbiter_mem_access_check_fake.call_count = 0;

	uintptr_t mirror_addr = suit_ipuc_sdfw_mirror_addr(16);

	zassert_between_inclusive(mirror_addr, sdfw_update_area_addr,
				  sdfw_update_area_addr + sdfw_update_area_size, "");

	zassert_equal(arbiter_mem_access_check_fake.call_count, 1,
		      "Incorrect number of arbiter_mem_access_check() calls");

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &ipuc_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_ERR_INCORRECT_STATE,
		      "suit_ipuc_sdfw_write_setup wrong return code - ret: %i", ret);
}

ZTEST(ipuc_tests, test_ipuc_write_NOK_not_setup)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	suit_manifest_role_t role = SUIT_MANIFEST_APP_LOCAL_1;
	int ipc_client_id = 1234;

	ret = suit_ipuc_sdfw_declare(ipuc_handle, role);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_declare failed - ret: %i", ret);

	size_t offset = 2;
	size_t chunk_size = 2;
	bool last_chunk = false;

	ret = suit_ipuc_sdfw_write(ipc_client_id, &ipuc_component_id, offset,
				   (uintptr_t)test_data + offset, chunk_size, last_chunk);
	zassert_equal(ret, SUIT_PLAT_ERR_INCORRECT_STATE,
		      "suit_ipuc_sdfw_write wrong return code - ret: %i", ret);
}

ZTEST(ipuc_tests, test_ipuc_sdfw_mirror_OK)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	suit_manifest_role_t role = SUIT_MANIFEST_APP_LOCAL_1;

	ret = suit_ipuc_sdfw_declare(ipuc_handle, role);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_declare failed - ret: %i", ret);

	uintptr_t sdfw_update_area_addr = 0;
	size_t sdfw_update_area_size = 0;

	suit_memory_sdfw_update_area_info_get(&sdfw_update_area_addr, &sdfw_update_area_size);

	zassert_not_equal(sdfw_update_area_size, 0, "");

	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;
	arbiter_mem_access_check_fake.call_count = 0;

	uintptr_t mirror_addr = suit_ipuc_sdfw_mirror_addr(16);

	zassert_between_inclusive(mirror_addr, sdfw_update_area_addr,
				  sdfw_update_area_addr + sdfw_update_area_size, "");

	zassert_equal(arbiter_mem_access_check_fake.call_count, 1,
		      "Incorrect number of arbiter_mem_access_check() calls");

	ret = suit_ipuc_sdfw_revoke(ipuc_handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_revoke failed - ret: %i", ret);

	mirror_addr = suit_ipuc_sdfw_mirror_addr(16);

	zassert_equal(mirror_addr, 0, "valid mirror_addr after suit_ipuc_sdfw_revoke() call");
}

ZTEST(ipuc_tests, test_ipuc_sdfw_mirror_NOK_not_declared)
{
	uintptr_t sdfw_update_area_addr = 0;
	size_t sdfw_update_area_size = 0;

	suit_memory_sdfw_update_area_info_get(&sdfw_update_area_addr, &sdfw_update_area_size);

	zassert_not_equal(sdfw_update_area_size, 0, "");

	uintptr_t mirror_addr = suit_ipuc_sdfw_mirror_addr(16);

	zassert_equal(mirror_addr, 0, "valid mirror_addr when IPUC not declared");
}

ZTEST(ipuc_tests, test_ipuc_sdfw_mirror_NOK_busy_other_client)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	suit_manifest_role_t role = SUIT_MANIFEST_APP_LOCAL_1;
	int ipc_client_id = 1234;

	ret = suit_ipuc_sdfw_declare(ipuc_handle, role);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_declare failed - ret: %i", ret);

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &ipuc_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write_setup failed - ret: %i", ret);

	uintptr_t sdfw_update_area_addr = 0;
	size_t sdfw_update_area_size = 0;

	suit_memory_sdfw_update_area_info_get(&sdfw_update_area_addr, &sdfw_update_area_size);

	zassert_not_equal(sdfw_update_area_size, 0, "");

	arbiter_mem_access_check_fake.return_val = ARBITER_STATUS_OK;
	arbiter_mem_access_check_fake.call_count = 0;

	uintptr_t mirror_addr = suit_ipuc_sdfw_mirror_addr(16);

	zassert_equal(mirror_addr, 0, "valid mirror_addr when IPUC not declared");
}
