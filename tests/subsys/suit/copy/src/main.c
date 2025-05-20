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
#include <suit_ipuc_sdfw.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <mocks_sdfw.h>
#include <suit_manifest_variables.h>
#include <suit_storage.h>
#include <decrypt_test_utils.h>

#define WRITE_ADDR		  0x1A00080000
#define TEST_MFST_VAR_NVM_ID	  1
#define TEST_MFST_VAR_RAM_PLAT_ID 128
#define TEST_MFST_VAR_RAM_MFST_ID 256
#define TEST_MFST_VAR_INIT_VALUE  0x00
#define TEST_MFST_VAR_NVM_VALUE	  0xAB
#define TEST_MFST_VAR_RAM_VALUE	  0xABAB

static uint8_t test_data[] = {0, 1, 2, 3, 4, 5, 6, 7};

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

static void *test_suite_setup(void)
{
	suit_plat_err_t plat_ret = suit_storage_init();

	zassert_ok(plat_ret, "Failed to initialize SUIT storage");

	return NULL;
}

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_sdfw_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(copy_tests, NULL, test_suite_setup, test_before, NULL, NULL);

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

	int ipc_client_id = 1234;
	int ret = suit_plat_create_component_handle(&valid_src_component_id, false, &src_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(src_handle, &source, &valid_manifest_component_id, NULL);
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

	ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &valid_dst_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "in-place updateable component found");

	ret = suit_ipuc_sdfw_declare(dst_handle, SUIT_MANIFEST_UNKNOWN);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_declare failed - error %i", ret);

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &valid_dst_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write_setup failed - error %i", ret);

	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_copy failed - error %i", ret);

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &valid_dst_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "in-place updateable component found");

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

	int ret = suit_plat_create_component_handle(&valid_src_component_id, false, &src_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(src_handle, &source, &valid_manifest_component_id, NULL);
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

	ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);

	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, NULL);
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

	int ret = suit_plat_create_component_handle(&valid_src_component_id, false, &src_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(src_handle, &source, &valid_manifest_component_id, NULL);
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

	ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS, "src_handle release failed - error %i", ret);

	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, NULL);
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

	int ret = suit_plat_create_component_handle(&valid_src_component_id, false, &src_handle);

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

	ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, NULL);
	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_copy should have failed - memptr empty");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);

	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS, "src_handle release failed - error %i", ret);
}

ZTEST(copy_tests, test_mfst_plat_ram_var_to_nvm_var_OK)
{
	suit_component_t src_handle;
	/* [h'MFST_VAR', 128] */
	uint8_t src_component[] = {0x82, 0x49, 0x68, 'M', 'F',	'S',  'T',
				   '_',	 'V',  'A',  'R', 0x42, 0x18, 0x80};
	struct zcbor_string src_component_id = {
		.value = src_component,
		.len = sizeof(src_component),
	};
	suit_component_t dst_handle;
	/* [h'MFST_VAR', 1] */
	uint8_t dst_component[] = {0x82, 0x49, 0x68, 'M', 'F',	'S', 'T',
				   '_',	 'V',  'A',  'R', 0x41, 0x01};
	struct zcbor_string dst_component_id = {
		.value = dst_component,
		.len = sizeof(dst_component),
	};
	uint32_t value = TEST_MFST_VAR_NVM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN that MFST_VAR/128 value is initialized with value greater than zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_RAM_PLAT_ID, value), SUIT_PLAT_SUCCESS,
		      "Unable to set MFST_VAR/128 value before the test");

	/* ... and the component handle for the MFST_VAR/128 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&src_component_id, false, &src_handle),
		      SUIT_SUCCESS, "Unable to create MFST_VAR/128 component");

	/* ... and MFST_VAR/1 value is initialized with zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, TEST_MFST_VAR_INIT_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR/1 value before the test");

	/* ... and the component handle for the MFST_VAR/1 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&dst_component_id, false, &dst_handle),
		      SUIT_SUCCESS, "Unable to create MFST_VAR/1 component");

	/* WHEN copy API is called without encryption info */
	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Copying platform volatile to nonvolatile manifest variable failed: %d", ret);

	/* THEN the value is updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_NVM_ID, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS,
		      "Unable to get MFST_VAR/1 value after the update");
	zassert_equal(value, TEST_MFST_VAR_NVM_VALUE, "MFST_VAR/1 value not updated (%d != %d)",
		      value, TEST_MFST_VAR_NVM_VALUE);

	/* ... and the components can be safely released */
	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release source component handle after the test: %d", ret);
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release destination component handle after the test: %d", ret);
}

ZTEST(copy_tests, test_mfst_nvm_var_to_plat_ram_NOK_access)
{
	suit_component_t src_handle;
	/* [h'MFST_VAR', 1] */
	uint8_t src_component[] = {0x82, 0x49, 0x68, 'M', 'F',	'S', 'T',
				   '_',	 'V',  'A',  'R', 0x41, 0x01};
	struct zcbor_string src_component_id = {
		.value = src_component,
		.len = sizeof(src_component),
	};
	suit_component_t dst_handle;
	/* [h'MFST_VAR', 128] */
	uint8_t dst_component[] = {0x82, 0x49, 0x68, 'M', 'F',	'S',  'T',
				   '_',	 'V',  'A',  'R', 0x42, 0x18, 0x80};
	struct zcbor_string dst_component_id = {
		.value = dst_component,
		.len = sizeof(dst_component),
	};
	uint32_t value = TEST_MFST_VAR_NVM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN that MFST_VAR/1 value is initialized with value greater than zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, value), SUIT_PLAT_SUCCESS,
		      "Unable to set MFST_VAR/1 value before the test");

	/* ... and the component handle for the MFST_VAR/128 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&src_component_id, false, &src_handle),
		      SUIT_SUCCESS, "Unable to create MFST_VAR/1 component");

	/* ... and MFST_VAR/128 value is initialized with zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_RAM_PLAT_ID, TEST_MFST_VAR_INIT_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR/128 value before the test");

	/* ... and the component handle for the MFST_VAR/1 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&dst_component_id, false, &dst_handle),
		      SUIT_SUCCESS, "Unable to create MFST_VAR/128 component");

	/* WHEN copy API is called without encryption info */
	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_ERR_UNAUTHORIZED_COMPONENT,
		      "Writing any value to the MFST_VAR component: %d must fail",
		      TEST_MFST_VAR_RAM_PLAT_ID);

	/* THEN the value is not updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_RAM_PLAT_ID, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS,
		      "Unable to get MFST_VAR/128 value after the update");
	zassert_equal(value, TEST_MFST_VAR_INIT_VALUE, "MFST_VAR/128 value updated (%d != %d)",
		      value, TEST_MFST_VAR_INIT_VALUE);

	/* ... and the components can be safely released */
	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release source component handle after the test: %d", ret);
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release destination component handle after the test: %d", ret);
}

ZTEST(copy_tests, test_mfst_nvm_var_to_mfst_ram_var_OK)
{
	suit_component_t src_handle;
	/* [h'MFST_VAR', 1] */
	uint8_t src_component[] = {0x82, 0x49, 0x68, 'M', 'F',	'S', 'T',
				   '_',	 'V',  'A',  'R', 0x41, 0x01};
	struct zcbor_string src_component_id = {
		.value = src_component,
		.len = sizeof(src_component),
	};
	suit_component_t dst_handle;
	/* [h'MFST_VAR', 256] */
	uint8_t dst_component[] = {0x82, 0x49, 0x68, 'M',  'F',	 'S',  'T', '_',
				   'V',	 'A',  'R',  0x43, 0x19, 0x01, 0x00};
	struct zcbor_string dst_component_id = {
		.value = dst_component,
		.len = sizeof(dst_component),
	};
	uint32_t value = TEST_MFST_VAR_NVM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN that MFST_VAR/128 value is initialized with value greater than zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, value), SUIT_PLAT_SUCCESS,
		      "Unable to set MFST_VAR/1 value before the test");

	/* ... and the component handle for the MFST_VAR/128 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&src_component_id, false, &src_handle),
		      SUIT_SUCCESS, "Unable to create MFST_VAR/1 component");

	/* ... and MFST_VAR/1 value is initialized with zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_RAM_MFST_ID, TEST_MFST_VAR_INIT_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR/256 value before the test");

	/* ... and the component handle for the MFST_VAR/1 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&dst_component_id, false, &dst_handle),
		      SUIT_SUCCESS, "Unable to create MFST_VAR/256 component");

	/* WHEN copy API is called without encryption info */
	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Copying nonvolatile to manifest volatile variable failed: %d", ret);

	/* THEN the value is updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_RAM_MFST_ID, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS,
		      "Unable to get MFST_VAR/256 value after the update");
	zassert_equal(value, TEST_MFST_VAR_NVM_VALUE, "MFST_VAR/245 value not updated (%d != %d)",
		      value, TEST_MFST_VAR_NVM_VALUE);

	/* ... and the components can be safely released */
	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release source component handle after the test: %d", ret);
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release destination component handle after the test: %d", ret);
}

ZTEST(copy_tests, test_mfst_ram_var_to_nvm_var_NOK_too_large)
{
	suit_component_t src_handle;
	/* [h'MFST_VAR', 128] */
	uint8_t src_component[] = {0x82, 0x49, 0x68, 'M', 'F',	'S',  'T',
				   '_',	 'V',  'A',  'R', 0x42, 0x18, 0x80};
	struct zcbor_string src_component_id = {
		.value = src_component,
		.len = sizeof(src_component),
	};
	suit_component_t dst_handle;
	/* [h'MFST_VAR', 1] */
	uint8_t dst_component[] = {0x82, 0x49, 0x68, 'M', 'F',	'S', 'T',
				   '_',	 'V',  'A',  'R', 0x41, 0x01};
	struct zcbor_string dst_component_id = {
		.value = dst_component,
		.len = sizeof(dst_component),
	};
	uint32_t value = TEST_MFST_VAR_RAM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN that MFST_VAR/128 value is initialized with value greater than zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_RAM_PLAT_ID, value), SUIT_PLAT_SUCCESS,
		      "Unable to set MFST_VAR/128 value before the test");

	/* ... and the component handle for the MFST_VAR/128 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&src_component_id, false, &src_handle),
		      SUIT_SUCCESS, "Unable to create MFST_VAR/128 component");

	/* ... and MFST_VAR/1 value is initialized with zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, TEST_MFST_VAR_INIT_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR/1 value before the test");

	/* ... and the component handle for the MFST_VAR/1 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&dst_component_id, false, &dst_handle),
		      SUIT_SUCCESS, "Unable to create MFST_VAR/1 component");

	/* WHEN copy API is called without encryption info */
	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER,
		      "Copying platform volatile with large value to nonvolatile manifest variable "
		      "did not fail: %d",
		      ret);

	/* THEN the value is not updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_NVM_ID, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS,
		      "Unable to get MFST_VAR/1 value after the update");
	zassert_equal(value, TEST_MFST_VAR_INIT_VALUE, "MFST_VAR/1 value updated (%d != %d)", value,
		      TEST_MFST_VAR_INIT_VALUE);

	/* ... and the components can be safely released */
	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release source component handle after the test: %d", ret);
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release destination component handle after the test: %d", ret);
}

ZTEST(copy_tests, test_mram_to_mfst_nvm_var_NOK)
{
	suit_component_t src_handle;
	/* [h'MEM', h'02', h'1A00080000', h'191000'] */
	uint8_t src_component[] = {0x84, 0x44, 0x63, 'M',  'E',	 'M',  0x41, 0x02, 0x45,
				   0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string src_component_id = {
		.value = src_component,
		.len = sizeof(src_component),
	};
	suit_component_t dst_handle;
	/* [h'MFST_VAR', 1] */
	uint8_t dst_component[] = {0x82, 0x49, 0x68, 'M', 'F',	'S', 'T',
				   '_',	 'V',  'A',  'R', 0x41, 0x01};
	struct zcbor_string dst_component_id = {
		.value = dst_component,
		.len = sizeof(dst_component),
	};
	uint32_t value = TEST_MFST_VAR_RAM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN MFST_VAR/1 value is initialized with zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, TEST_MFST_VAR_INIT_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR/1 value before the test");

	/* ... and the component handle for the MFST_VAR/1 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&dst_component_id, false, &dst_handle),
		      SUIT_SUCCESS, "Unable to create MFST_VAR/1 component");

	/* ... and the component handle for the MEM is successfully created */
	zassert_equal(suit_plat_create_component_handle(&src_component_id, false, &src_handle),
		      SUIT_SUCCESS, "Unable to create MEM component");

	/* WHEN copy API is called without encryption info */
	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER,
		      "Copying MEM value to nonvolatile manifest variable did not fail: %d", ret);

	/* THEN the value is not updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_NVM_ID, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS,
		      "Unable to get MFST_VAR/1 value after the update");
	zassert_equal(value, TEST_MFST_VAR_INIT_VALUE, "MFST_VAR/1 value updated (%d != %d)", value,
		      TEST_MFST_VAR_INIT_VALUE);

	/* ... and the components can be safely released */
	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release source component handle after the test: %d", ret);
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release destination component handle after the test: %d", ret);
}

ZTEST(copy_tests, test_mfst_nvm_var_to_mram_NOK)
{
	suit_component_t src_handle;
	/* [h'MFST_VAR', 1] */
	uint8_t src_component[] = {0x82, 0x49, 0x68, 'M', 'F',	'S', 'T',
				   '_',	 'V',  'A',  'R', 0x41, 0x01};
	struct zcbor_string src_component_id = {
		.value = src_component,
		.len = sizeof(src_component),
	};
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A00080000', h'191000'] */
	uint8_t dst_component[] = {0x84, 0x44, 0x63, 'M',  'E',	 'M',  0x41, 0x02, 0x45,
				   0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string dst_component_id = {
		.value = dst_component,
		.len = sizeof(dst_component),
	};
	int ret;

	/* GIVEN MFST_VAR/1 value is initialized with zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, TEST_MFST_VAR_NVM_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR/1 value before the test");

	/* ... and the component handle for the MFST_VAR/1 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&src_component_id, false, &src_handle),
		      SUIT_SUCCESS, "Unable to create MFST_VAR/1 component");

	/* ... and the component handle for the MEM is successfully created */
	zassert_equal(suit_plat_create_component_handle(&dst_component_id, false, &dst_handle),
		      SUIT_SUCCESS, "Unable to create MEM component");

	/* WHEN copy API is called without encryption info */
	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "Copying nonvolatile manifest variable content to MEM did not fail: %d", ret);

	/* THEN the MEM value is not updated ... */
	/* ... and the components can be safely released */
	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release source component handle after the test: %d", ret);
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Failed to release destination component handle after the test: %d", ret);
}

ZTEST(copy_tests, test_integrated_fetch_and_copy_to_msink_encrypted_OK)
{
	memptr_storage_handle_t handle;
	struct zcbor_string source = {
		.value = decrypt_test_ciphertext_direct,
		.len = sizeof(decrypt_test_ciphertext_direct)
	};

	suit_component_t src_handle;
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_src_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
					 '_',  'I',	 'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_src_component_id = {
		.value = valid_src_value,
		.len = sizeof(valid_src_value),
	};

	psa_key_id_t cek_key_id;
	uint8_t cek_key_id_cbor[] = {
		0x1A, 0x00, 0x00, 0x00, 0x00,
	};

	psa_status_t status = psa_crypto_init();

	zassert_equal(status, PSA_SUCCESS, "Failed to init psa crypto");

	status = decrypt_test_init_encryption_key(decrypt_test_key_data,
				sizeof(decrypt_test_key_data), &cek_key_id,
				PSA_ALG_GCM, cek_key_id_cbor);
	zassert_equal(status, PSA_SUCCESS, "Failed to import key");

	struct suit_encryption_info enc_info =
				DECRYPT_TEST_ENC_INFO_DEFAULT_INIT(cek_key_id_cbor);

	int ret = suit_plat_create_component_handle(&valid_src_component_id, false, &src_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch_integrated(src_handle, &source, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	ret = suit_plat_component_impl_data_get(src_handle, &handle);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_component_impl_data_get failed - error %i",
			  ret);

	const uint8_t *payload;
	size_t payload_size = 0;

	ret = suit_memptr_storage_ptr_get(handle, &payload, &payload_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "storage.get failed - error %i", ret);
	zassert_equal_ptr(decrypt_test_ciphertext_direct, payload,
			"Retrieved payload doesn't mach ciphertext_direct");
	zassert_equal(sizeof(decrypt_test_ciphertext_direct), payload_size,
			"Retrieved payload_size doesn't mach size of ciphertext_direct");
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

	ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_copy(dst_handle, src_handle, &valid_manifest_component_id, &enc_info);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_copy failed - error %i", ret);

	ret = suit_plat_component_impl_data_get(dst_handle, &handle);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_component_impl_data_get failed - error %i",
			  ret);

	ret = suit_memptr_storage_ptr_get(handle, &payload, &payload_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "storage.get failed - error %i", ret);
	zassert_equal(memcmp(decrypt_test_plaintext, payload, strlen(decrypt_test_plaintext)), 0,
			"Retrieved decrypted payload doesn't mach decrypt_test_plaintext");
	zassert_equal(sizeof(decrypt_test_plaintext), payload_size,
			"Retrieved payload_size doesn't mach size of decrypt_test_plaintext");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);

	ret = suit_plat_release_component_handle(src_handle);
	zassert_equal(ret, SUIT_SUCCESS, "src_handle release failed - error %i", ret);

	ret = suit_memptr_storage_release(handle);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "memptr_storage handle release failed - error %i",
		  ret);

	psa_destroy_key(cek_key_id);
}
