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
#include <suit_ipuc_sdfw.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <suit_plat_mem_util.h>
#include <suit_memory_layout.h>
#include <mocks_sdfw.h>
#include <suit_manifest_variables.h>
#include <suit_storage.h>
#include <decrypt_test_utils.h>

#define DFU_PARTITION_OFFSET FIXED_PARTITION_OFFSET(dfu_partition)
#define FLASH_WRITE_ADDR     (suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET))
#define RAM_WRITE_ADDR	     ((uint8_t *)suit_memory_global_address_to_ram_address(0x2003EC00))

#define TEST_MFST_VAR_NVM_ID	    0
#define TEST_MFST_VAR_NVM_ID_MAX    7
#define TEST_MFST_VAR_RAM_PLAT_ID   128
#define TEST_MFST_VAR_RAM_MFST_ID   256
#define TEST_MFST_VAR_NVM_VALUE_MIN 0x00
#define TEST_MFST_VAR_NVM_VALUE	    0xAB
#define TEST_MFST_VAR_NVM_VALUE_MAX 0xFF

static uint8_t test_data[] = {0xDE, 0xAD, 0xBE, 0xEF};

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

ZTEST_SUITE(write_tests, NULL, test_suite_setup, test_before, NULL, NULL);

ZTEST(write_tests, test_write_to_flash_sink_OK)
{
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A000F0000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x0F, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ipc_client_id = 1234;
	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);
	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &valid_dst_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "in-place updateable component found");

	ret = suit_ipuc_sdfw_declare(dst_handle, SUIT_MANIFEST_UNKNOWN);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_declare failed - error %i", ret);

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &valid_dst_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_ipuc_sdfw_write_setup failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_write failed - error %i", ret);

	ret = suit_ipuc_sdfw_write_setup(ipc_client_id, &valid_dst_component_id, NULL, NULL);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "in-place updateable component found");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
	zassert_mem_equal(FLASH_WRITE_ADDR, test_data, sizeof(test_data),
			  "Data in destination is invalid");
}

ZTEST(write_tests, test_write_to_flash_sink_encrypted_OK)
{
	struct zcbor_string source = {
		.value = decrypt_test_ciphertext_direct,
		.len = sizeof(decrypt_test_ciphertext_direct)
	};

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A000F0000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x0F, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
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

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source, &valid_manifest_component_id, &enc_info);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_write failed - error %i", ret);

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
	zassert_mem_equal(FLASH_WRITE_ADDR, decrypt_test_plaintext, strlen(decrypt_test_plaintext),
			  "Data in destination is invalid");

	psa_destroy_key(cek_key_id);
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

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source, &valid_manifest_component_id, NULL);
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
	/* [h'MEM', h'02', h'1A000F0000', h'10'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02,
				     0x45, 0x1A, 0x00, 0x0F, 0x00, 0x00, 0x41, 0x10};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source, &valid_manifest_component_id, NULL);
	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_write should have failed on erase");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
}

ZTEST(write_tests, test_write_flash_sink_NOK_handle_released)
{
	struct zcbor_string source = {.value = test_data, .len = sizeof(test_data)};

	/* handle that will be used as destination */
	suit_component_t dst_handle = 0;

	int ret = suit_plat_write(dst_handle, &source, &valid_manifest_component_id, NULL);

	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_write should have failed - invalid handle");
}

ZTEST(write_tests, test_write_to_flash_sink_NOK_source_null)
{
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A000F0000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x0F, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, NULL, &valid_manifest_component_id, NULL);
	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_write should have failed - source null");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
}

ZTEST(write_tests, test_write_to_flash_sink_NOK_source_value_null)
{
	struct zcbor_string source = {.value = NULL, .len = sizeof(test_data)};

	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'02', h'1A000F0000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x0F, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source, &valid_manifest_component_id, NULL);
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
	/* [h'MEM', h'02', h'1A000F0000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x02, 0x45,
				     0x1A, 0x00, 0x0F, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_write(dst_handle, &source, &valid_manifest_component_id, NULL);
	zassert_not_equal(ret, SUIT_SUCCESS, "suit_plat_write should have failed - source len 0");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
}

ZTEST(write_tests, test_write_to_mfst_var_nvm_min_OK)
{
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MFST_VAR', h'00'] */
	uint8_t valid_dst_value[] = {0x82, 0x49, 0x68, 'M', 'F',  'S', 'T',
				     '_',  'V',	 'A',  'R', 0x41, 0x00};
	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};
	/* 0x00 */
	uint8_t test_value_0[] = {0x00};
	struct zcbor_string test_value_0_bstr = {
		.value = test_value_0,
		.len = sizeof(test_value_0),
	};
	uint32_t value = TEST_MFST_VAR_NVM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN that MFST_VAR/0 value is initialized with value greater than zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, TEST_MFST_VAR_NVM_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR value before the test");

	/* ... and the component handle for the MFST_VAR/0 is successfully created */
	zassert_equal(
		suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle),
		SUIT_SUCCESS, "Unable to create MFST_VAR component");

	/* WHEN write API is called */
	ret = suit_plat_write(dst_handle, &test_value_0_bstr, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "Unable to write value %d to the MFST_VAR component: %d",
		      TEST_MFST_VAR_NVM_VALUE, TEST_MFST_VAR_NVM_ID);

	/* THEN the value is updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_NVM_ID, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS, "Unable to get MFST_VAR value after the update");
	zassert_equal(value, TEST_MFST_VAR_NVM_VALUE_MIN, "Failed to update MFST_VAR/0 value");

	/* ... and the component can be safely released */
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Failed to release component handle after the test: %d",
		      ret);
}

ZTEST(write_tests, test_write_to_mfst_var_ram_mfst_OK)
{
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MFST_VAR', 256] */
	uint8_t valid_dst_value[] = {0x82, 0x49, 0x68, 'M',  'F',  'S',	 'T', '_',
				     'V',  'A',	 'R',  0x43, 0x19, 0x01, 0x00};
	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};
	/* 0x00 */
	uint8_t test_value_0[] = {0x00};
	struct zcbor_string test_value_0_bstr = {
		.value = test_value_0,
		.len = sizeof(test_value_0),
	};
	uint32_t value = TEST_MFST_VAR_NVM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN that MFST_VAR/0 value is initialized with value greater than zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_RAM_MFST_ID, TEST_MFST_VAR_NVM_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR value before the test");

	/* ... and the component handle for the MFST_VAR/0 is successfully created */
	zassert_equal(
		suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle),
		SUIT_SUCCESS, "Unable to create MFST_VAR component");

	/* WHEN write API is called */
	ret = suit_plat_write(dst_handle, &test_value_0_bstr, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "Unable to write value %d to the MFST_VAR component: %d",
		      TEST_MFST_VAR_NVM_VALUE, TEST_MFST_VAR_RAM_MFST_ID);

	/* THEN the value is updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_RAM_MFST_ID, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS, "Unable to get MFST_VAR value after the update");
	zassert_equal(value, TEST_MFST_VAR_NVM_VALUE_MIN, "Failed to update MFST_VAR/256 value");

	/* ... and the component can be safely released */
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Failed to release component handle after the test: %d",
		      ret);
}

ZTEST(write_tests, test_write_to_mfst_var_nvm_max_OK)
{
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MFST_VAR', h'07'] */
	uint8_t valid_dst_value[] = {0x82, 0x49, 0x68, 'M', 'F',  'S', 'T',
				     '_',  'V',	 'A',  'R', 0x41, 0x07};
	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};
	/* 0xFF */
	uint8_t test_value_FF[] = {0x18, 0xFF};
	struct zcbor_string test_value_FF_bstr = {
		.value = test_value_FF,
		.len = sizeof(test_value_FF),
	};
	uint32_t value = TEST_MFST_VAR_NVM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN that MFST_VAR/7 value is initialized with value greater than zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID_MAX, value), SUIT_PLAT_SUCCESS,
		      "Unable to set MFST_VAR value before the test");

	/* ... and the component handle for the MFST_VAR/0 is successfully created */
	zassert_equal(
		suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle),
		SUIT_SUCCESS, "Unable to create MFST_VAR component");

	/* WHEN write API is called */
	ret = suit_plat_write(dst_handle, &test_value_FF_bstr, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "Unable to write value %d to the MFST_VAR component: %d",
		      TEST_MFST_VAR_NVM_VALUE_MAX, TEST_MFST_VAR_NVM_ID_MAX);

	/* THEN the value is updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_NVM_ID_MAX, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS, "Unable to get MFST_VAR value after the update");
	zassert_equal(value, TEST_MFST_VAR_NVM_VALUE_MAX, "Failed to update MFST_VAR/0 value");

	/* ... and the component can be safely released */
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Failed to release component handle after the test: %d",
		      ret);
}

ZTEST(write_tests, test_write_to_mfst_var_NOK_encrypted)
{
	struct suit_encryption_info enc_info = {0};
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MFST_VAR', h'00'] */
	uint8_t valid_dst_value[] = {0x82, 0x49, 0x68, 'M', 'F',  'S', 'T',
				     '_',  'V',	 'A',  'R', 0x41, 0x00};
	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};
	/* 0x00 */
	uint8_t test_value_0[] = {0x00};
	struct zcbor_string test_value_0_bstr = {
		.value = test_value_0,
		.len = sizeof(test_value_0),
	};
	uint32_t value = TEST_MFST_VAR_NVM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN that MFST_VAR/0 value is initialized with value greater than zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, value), SUIT_PLAT_SUCCESS,
		      "Unable to set MFST_VAR value before the test");

	/* ... and the component handle for the MFST_VAR/0 is successfully created */
	zassert_equal(
		suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle),
		SUIT_SUCCESS, "Unable to create MFST_VAR component");

	/* WHEN write API is called with encryption info set */
	ret = suit_plat_write(dst_handle, &test_value_0_bstr, &valid_manifest_component_id,
			      &enc_info);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER, "Encrypted MFST_VAR value shall fail");

	/* THEN the value is not updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_NVM_ID, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS, "Unable to get MFST_VAR value after the update");
	zassert_equal(value, TEST_MFST_VAR_NVM_VALUE, "MFST_VAR/0 value updated");

	/* ... and the component can be safely released */
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Failed to release component handle after the test: %d",
		      ret);
}

ZTEST(write_tests, test_write_to_mfst_var_NOK_invalid_content)
{
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MFST_VAR', h'00'] */
	uint8_t valid_dst_value[] = {0x82, 0x49, 0x68, 'M', 'F',  'S', 'T',
				     '_',  'V',	 'A',  'R', 0x41, 0x00};
	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};
	/* h'00' */
	uint8_t test_value_0[] = {0x41, 0x00};
	struct zcbor_string test_value_0_bstr = {
		.value = test_value_0,
		.len = sizeof(test_value_0),
	};
	uint32_t value = TEST_MFST_VAR_NVM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN that MFST_VAR/0 value is initialized with value greater than zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, value), SUIT_PLAT_SUCCESS,
		      "Unable to set MFST_VAR value before the test");

	/* ... and the component handle for the MFST_VAR/0 is successfully created */
	zassert_equal(
		suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle),
		SUIT_SUCCESS, "Unable to create MFST_VAR component");

	/* WHEN write API is called with encryption info set */
	ret = suit_plat_write(dst_handle, &test_value_0_bstr, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER,
		      "Setting bstr as MFST_VAR value shall fail");

	/* THEN the value is not updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_NVM_ID, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS, "Unable to get MFST_VAR value after the update");
	zassert_equal(value, TEST_MFST_VAR_NVM_VALUE, "MFST_VAR/0 value updated");

	/* ... and the component can be safely released */
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Failed to release component handle after the test: %d",
		      ret);
}

ZTEST(write_tests, test_write_to_mfst_var_ram_plat_NOK_access)
{
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MFST_VAR', 128] */
	uint8_t valid_dst_value[] = {0x82, 0x49, 0x68, 'M', 'F',  'S',	'T',
				     '_',  'V',	 'A',  'R', 0x42, 0x18, 0x80};
	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};
	/* 0x00 */
	uint8_t test_value_0[] = {0x00};
	struct zcbor_string test_value_0_bstr = {
		.value = test_value_0,
		.len = sizeof(test_value_0),
	};
	uint32_t value = TEST_MFST_VAR_NVM_VALUE;
	suit_plat_err_t plat_ret;
	int ret;

	/* GIVEN that MFST_VAR/0 value is initialized with value greater than zero... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_RAM_PLAT_ID, TEST_MFST_VAR_NVM_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR value before the test");

	/* ... and the component handle for the MFST_VAR/128 is successfully created */
	zassert_equal(
		suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle),
		SUIT_SUCCESS, "Unable to create MFST_VAR component");

	/* WHEN write API is called */
	ret = suit_plat_write(dst_handle, &test_value_0_bstr, &valid_manifest_component_id, NULL);
	zassert_equal(ret, SUIT_ERR_UNAUTHORIZED_COMPONENT,
		      "Writing any value to the MFST_VAR component: %d must fail",
		      TEST_MFST_VAR_RAM_PLAT_ID);

	/* THEN the value is not updated ... */
	plat_ret = suit_mfst_var_get(TEST_MFST_VAR_RAM_PLAT_ID, &value);
	zassert_equal(plat_ret, SUIT_PLAT_SUCCESS, "Unable to get MFST_VAR value after the update");
	zassert_equal(value, TEST_MFST_VAR_NVM_VALUE, "MFST_VAR/128 value updated");

	/* ... and the component can be safely released */
	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "Failed to release component handle after the test: %d",
		      ret);
}
