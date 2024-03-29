/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_plat_memptr_size_update.h>
#include <mocks.h>

#define TEST_COMPONENT_HANDLE	    ((suit_component_t)0x123)
#define TEST_COMPONENT_ID	    ((struct zcbor_string *)0x456)
#define TEST_MEMPTR_COMP_ADDRESS    ((uint8_t *)0xabcd)
#define TEST_MEMPTR_COMP_SIZE	    ((size_t)0xef01)
#define TEST_MEMPTR_STORAGE_HANDLE  ((memptr_storage_handle_t)0x2345)
#define TEST_NEW_SIZE		    ((size_t)256)
#define TEST_RUN_ADDRESS	    ((intptr_t)0x12345)
#define TEST_COMPONENT_SIZE_SMALLER ((size_t)128)
#define TEST_COMPONENT_SIZE_BIGGER  ((size_t)512)

static size_t smaller_component_size = 128, bigger_component_size = 512;

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

static int suit_plat_component_id_get_invalid_fake_func(suit_component_t handle,
							struct zcbor_string **component_id)
{
	zassert_equal(TEST_COMPONENT_HANDLE, handle, "Invalid component handle value");
	zassert_not_null(component_id, "The API must provide a valid pointer");

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

static int suit_plat_component_id_get_correct_fake_func(suit_component_t handle,
							struct zcbor_string **component_id)
{
	zassert_equal(TEST_COMPONENT_HANDLE, handle, "Invalid component handle value");
	zassert_not_null(component_id, "The API must provide a valid pointer");

	*component_id = TEST_COMPONENT_ID;

	return SUIT_SUCCESS;
}

static int suit_plat_decode_address_size_invalid_fake_func(struct zcbor_string *component_id,
							   intptr_t *run_address, size_t *size)
{
	zassert_equal(TEST_COMPONENT_ID, component_id, "Invalid component_id");
	zassert_not_null(run_address, "The API must provide a valid pointer");
	zassert_not_null(size, "The API must provide a valid pointer");

	return SUIT_PLAT_ERR_CBOR_DECODING;
}

static int
suit_plat_decode_address_size_smaller_correct_fake_func(struct zcbor_string *component_id,
							intptr_t *run_address, size_t *size)
{
	zassert_equal(TEST_COMPONENT_ID, component_id, "Invalid component_id");
	zassert_not_null(run_address, "The API must provide a valid pointer");
	zassert_not_null(size, "The API must provide a valid pointer");

	*run_address = TEST_RUN_ADDRESS;
	*size = TEST_COMPONENT_SIZE_SMALLER;

	return SUIT_PLAT_SUCCESS;
}

static int suit_plat_decode_address_size_bigger_correct_fake_func(struct zcbor_string *component_id,
								  intptr_t *run_address,
								  size_t *size)
{
	zassert_equal(TEST_COMPONENT_ID, component_id, "Invalid component_id");
	zassert_not_null(run_address, "The API must provide a valid pointer");
	zassert_not_null(size, "The API must provide a valid pointer");

	*run_address = TEST_RUN_ADDRESS;
	*size = TEST_COMPONENT_SIZE_BIGGER;

	return SUIT_PLAT_SUCCESS;
}

static int suit_plat_component_impl_data_get_invalid_fake_func(suit_component_t handle,
							       void **impl_data)
{
	zassert_equal(TEST_COMPONENT_HANDLE, handle, "Unexpected component handle value");
	zassert_not_equal(impl_data, NULL,
			  "API must provide valid address to get imeplementation-specific data");

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

static int suit_plat_component_impl_data_get_correct_fake_func(suit_component_t handle,
							       void **impl_data)
{
	zassert_equal(TEST_COMPONENT_HANDLE, handle, "Unexpected component handle value");
	zassert_not_equal(impl_data, NULL,
			  "API must provide valid address to get imeplementation-specific data");

	*impl_data = TEST_MEMPTR_STORAGE_HANDLE;

	return SUIT_SUCCESS;
}

static suit_memptr_storage_err_t
suit_memptr_storage_ptr_get_invalid_fake_func(memptr_storage_handle_t handle,
					      const uint8_t **payload_ptr, size_t *payload_size)
{
	zassert_equal(handle, TEST_MEMPTR_STORAGE_HANDLE, "Unexpected memory storage handle value");
	zassert_not_equal(payload_ptr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(payload_size, NULL,
			  "API must provide valid address to get installed manifest size value");

	return SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;
}

static suit_memptr_storage_err_t
suit_memptr_storage_ptr_get_correct_fake_func(memptr_storage_handle_t handle,
					      const uint8_t **payload_ptr, size_t *payload_size)
{
	zassert_equal(handle, TEST_MEMPTR_STORAGE_HANDLE, "Unexpected memory storage handle value");
	zassert_not_equal(payload_ptr, NULL,
			  "API must provide valid address to get installed manifest address value");
	zassert_not_equal(payload_size, NULL,
			  "API must provide valid address to get installed manifest size value");

	*payload_ptr = TEST_MEMPTR_COMP_ADDRESS;
	*payload_size = TEST_MEMPTR_COMP_SIZE;

	return SUIT_PLAT_SUCCESS;
}

static suit_memptr_storage_err_t
suit_memptr_storage_ptr_store_invalid_fake_func(memptr_storage_handle_t handle,
						const uint8_t *payload_ptr, size_t payload_size)
{
	zassert_equal(handle, TEST_MEMPTR_STORAGE_HANDLE, "Unexpected memory storage handle value");
	zassert_not_equal(payload_ptr, NULL,
			  "API must provide valid address to get installed manifest address value");

	return SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;
}

static suit_memptr_storage_err_t
suit_memptr_storage_ptr_store_correct_fake_func(memptr_storage_handle_t handle,
						const uint8_t *payload_ptr, size_t payload_size)
{
	zassert_equal(handle, TEST_MEMPTR_STORAGE_HANDLE, "Unexpected memory storage handle value");
	zassert_not_equal(payload_ptr, NULL,
			  "API must provide valid address to get installed manifest address value");

	return SUIT_PLAT_SUCCESS;
}

ZTEST_SUITE(suit_plat_memptr_size_update_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_plat_memptr_size_update_tests, test_invalid_suit_plat_component_id_get)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_invalid_fake_func;

	int ret = suit_plat_memptr_size_update(TEST_COMPONENT_HANDLE, TEST_NEW_SIZE);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
}

ZTEST(suit_plat_memptr_size_update_tests, test_invalid_suit_plat_decode_address_size)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_address_size_fake.custom_fake =
		suit_plat_decode_address_size_invalid_fake_func;

	int ret = suit_plat_memptr_size_update(TEST_COMPONENT_HANDLE, TEST_NEW_SIZE);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
}

ZTEST(suit_plat_memptr_size_update_tests, test_valid_suit_plat_decode_address_size_smaller)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_address_size_fake.custom_fake =
		suit_plat_decode_address_size_smaller_correct_fake_func;

	int ret = suit_plat_memptr_size_update(TEST_COMPONENT_HANDLE, TEST_NEW_SIZE);

	zassert_equal(SUIT_ERR_UNSUPPORTED_PARAMETER, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
}

ZTEST(suit_plat_memptr_size_update_tests, test_valid_suit_plat_decode_address_size_bigger)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_address_size_fake.custom_fake =
		suit_plat_decode_address_size_bigger_correct_fake_func;
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_invalid_fake_func;

	int ret = suit_plat_memptr_size_update(TEST_COMPONENT_HANDLE, TEST_NEW_SIZE);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
}

ZTEST(suit_plat_memptr_size_update_tests, test_invalid_suit_memptr_storage_ptr_get)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_address_size_fake.custom_fake =
		suit_plat_decode_address_size_bigger_correct_fake_func;
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_correct_fake_func;
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_invalid_fake_func;

	int ret = suit_plat_memptr_size_update(TEST_COMPONENT_HANDLE, TEST_NEW_SIZE);

	zassert_equal(SUIT_ERR_CRASH, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
}

ZTEST(suit_plat_memptr_size_update_tests, test_invalid_suit_memptr_storage_ptr_store)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_address_size_fake.custom_fake =
		suit_plat_decode_address_size_bigger_correct_fake_func;
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_correct_fake_func;
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_correct_fake_func;
	suit_memptr_storage_ptr_store_fake.custom_fake =
		suit_memptr_storage_ptr_store_invalid_fake_func;

	int ret = suit_plat_memptr_size_update(TEST_COMPONENT_HANDLE, TEST_NEW_SIZE);

	zassert_equal(SUIT_ERR_CRASH, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 1,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
}

ZTEST(suit_plat_memptr_size_update_tests, test_all_OK)
{
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_decode_address_size_fake.custom_fake =
		suit_plat_decode_address_size_bigger_correct_fake_func;
	suit_plat_component_impl_data_get_fake.custom_fake =
		suit_plat_component_impl_data_get_correct_fake_func;
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_correct_fake_func;
	suit_memptr_storage_ptr_store_fake.custom_fake =
		suit_memptr_storage_ptr_store_correct_fake_func;

	int ret = suit_plat_memptr_size_update(TEST_COMPONENT_HANDLE, TEST_NEW_SIZE);

	zassert_equal(SUIT_SUCCESS, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_class_id_validate() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_component_impl_data_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_id() calls");
	zassert_equal(suit_memptr_storage_ptr_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_processor_start_rights_validate() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 1,
		      "Incorrect number of suit_mci_memory_access_rights_validate() calls");
}
