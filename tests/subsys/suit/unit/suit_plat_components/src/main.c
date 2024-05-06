/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <mocks.h>
#include <suit_platform.h>

uint8_t valid_mem_component_id_value[] = {0x84, 0x44, 0x63, 'M',  'E',	'M',
					  0x41, 0x02, 0x41, 0x00, 0x41, 0x00};

uint8_t valid_cand_img_id_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				     '_',  'I',	 'M',  'G', 0x41, 0x02};

uint8_t valid_cand_mfst_id_value[] = {0x82, 0x49, 0x68, 'C', 'A', 'N',	'D',
				      '_',  'M',  'F',	'S', 'T', 0x41, 0x02};

struct zcbor_string valid_mem_component_id = {
	.value = valid_mem_component_id_value,
	.len = sizeof(valid_mem_component_id_value),
};

struct zcbor_string valid_cand_img_id = {
	.value = valid_cand_img_id_value,
	.len = sizeof(valid_cand_img_id_value),
};

struct zcbor_string valid_cand_mfst_id = {
	.value = valid_cand_mfst_id_value,
	.len = sizeof(valid_cand_mfst_id_value),
};

static memptr_storage_handle_t valid_memptr_storage_handle;
static intptr_t memptr_address = (intptr_t)NULL;

static suit_component_type_t component_type;

static suit_component_t component_handles[SUIT_MAX_NUM_COMPONENT_PARAMS];

static void mocks_return_values_reset(void)
{
	suit_memptr_storage_get_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_memptr_storage_ptr_store_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_memptr_storage_release_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_plat_decode_component_type_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_plat_decode_address_size_fake.return_val = SUIT_PLAT_SUCCESS;
}

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();

	memptr_address = (intptr_t)NULL;
	component_type = 0;

	mocks_return_values_reset();

	/* release all component handles*/
	for (size_t i = 0; i < SUIT_MAX_NUM_COMPONENT_PARAMS; i++) {
		suit_plat_release_component_handle(component_handles[i]);
	}

	/* Reset mocks and history again - releasing component handles could
	 * have influenced them
	 */
	mocks_reset();
	FFF_RESET_HISTORY();
	mocks_return_values_reset();
}

static int suit_plat_decode_component_type_correct_fake_func(struct zcbor_string *component_id,
							     suit_component_type_t *type)
{
	zassert_not_null(type, "The API must provide a valid pointer");

	*type = component_type;
	return SUIT_PLAT_SUCCESS;
}

static int suit_memptr_storage_get_correct_fake_func(memptr_storage_handle_t *handle)
{
	zassert_not_null(handle, "The API must provide a valid pointer");

	*handle = &valid_memptr_storage_handle;

	return SUIT_PLAT_SUCCESS;
}

static int suit_plat_decode_address_size_correct_fake_func(struct zcbor_string *component_id,
							   intptr_t *run_address, size_t *size)
{

	zassert_not_null(component_id, "The API must provide valid pointers");
	zassert_not_null(run_address, "The API must provide valid pointers");
	zassert_not_null(size, "The API must provide valid pointers");

	*run_address = memptr_address;
	(void)size; /* Returned size isn't used and can be ignored */

	return SUIT_PLAT_SUCCESS;
}

static void create_valid_component(suit_component_type_t type)
{
	int ret;

	component_type = type;

	suit_memptr_storage_get_fake.custom_fake = suit_memptr_storage_get_correct_fake_func;
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;
	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);
	zassert_equal(SUIT_SUCCESS, ret, "Failed to create valid component handle");

	/* Reset mocks to only look at the results of the next call*/
	mocks_reset();
	FFF_RESET_HISTORY();
	mocks_return_values_reset();
}

ZTEST_SUITE(suit_plat_components_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_plat_components_tests, test_create_component_handle_null_component_handle)
{
	int ret = suit_plat_create_component_handle(&valid_mem_component_id, NULL);

	zassert_equal(SUIT_ERR_UNSUPPORTED_PARAMETER, ret, "Failed to catch null argument");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 0,
		      "incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 0,
		      "incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "incorrect number of suit_memptr_storage_ptr_store() calls");
}

ZTEST(suit_plat_components_tests, test_create_component_handle_null_component_id)
{
	int ret = suit_plat_create_component_handle(NULL, &component_handles[0]);

	zassert_equal(SUIT_ERR_UNSUPPORTED_PARAMETER, ret, "Failed to catch null argument");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");
}

ZTEST(suit_plat_components_tests, test_create_component_handle_too_many_components)
{
	suit_component_t test_handle;
	int ret;

	for (size_t i = 0; i < SUIT_MAX_NUM_COMPONENT_PARAMS; i++) {
		ret = suit_plat_create_component_handle(&valid_mem_component_id,
							&component_handles[i]);
		zassert_equal(SUIT_SUCCESS, ret, "Failed to create valid component handle");
	}

	/* Reset mocks to only look at the results of the next call*/
	mocks_reset();
	FFF_RESET_HISTORY();
	mocks_return_values_reset();

	ret = suit_plat_create_component_handle(&valid_mem_component_id, &test_handle);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Failed to record overflow in "
		      "components count");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");
}

ZTEST(suit_plat_components_tests, test_create_component_handle_other_component_type)
{
	int ret;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;

	component_type = SUIT_COMPONENT_TYPE_SOC_SPEC;

	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);

	zassert_equal(SUIT_SUCCESS, ret, "Failed to create valid component handle");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");
}

ZTEST(suit_plat_components_tests, test_create_component_handle_mem_ok)
{
	struct zcbor_string *component_id;
	void *impl_data;
	int ret;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;
	suit_memptr_storage_get_fake.custom_fake = suit_memptr_storage_get_correct_fake_func;
	suit_plat_decode_address_size_fake.custom_fake =
		suit_plat_decode_address_size_correct_fake_func;

	component_type = SUIT_COMPONENT_TYPE_MEM;
	memptr_address = 0x12345000;

	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);

	zassert_equal(SUIT_SUCCESS, ret, "Failed to create valid component handle");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");

	suit_plat_component_id_get(component_handles[0], &component_id);
	zassert_true(suit_compare_zcbor_strings(suit_plat_decode_address_size_fake.arg0_val,
						component_id));

	zassert_equal_ptr(suit_memptr_storage_ptr_store_fake.arg0_val,
			  &valid_memptr_storage_handle);
	zassert_equal_ptr(suit_memptr_storage_ptr_store_fake.arg1_val, (uint8_t *)memptr_address);
	zassert_equal(suit_memptr_storage_ptr_store_fake.arg2_val, 0);

	suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal_ptr(impl_data, &valid_memptr_storage_handle);
}

ZTEST(suit_plat_components_tests, test_create_component_handle_mem_decode_component_type_fail)
{
	void *impl_data;
	int ret;

	suit_plat_decode_component_type_fake.return_val = SUIT_PLAT_ERR_CBOR_DECODING;

	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");

	/* Make sure the component is not allocated */
	ret = suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "Retrieving impl_data did not fail with appropriate error after failed "
		      "component creation");
}

ZTEST(suit_plat_components_tests, test_create_component_handle_mem_decode_address_size_fail)
{
	void *impl_data;
	int ret;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;
	suit_plat_decode_address_size_fake.return_val = SUIT_PLAT_ERR_CBOR_DECODING;

	component_type = SUIT_COMPONENT_TYPE_MEM;

	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");

	/* Make sure the component is not allocated */
	ret = suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "Retrieving impl_data did not fail with appropriate error after failed "
		      "component creation");
}

ZTEST(suit_plat_components_tests, test_create_component_handle_mem_memptr_storage_get_fail)
{
	void *impl_data;
	int ret;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;
	suit_plat_decode_address_size_fake.custom_fake =
		suit_plat_decode_address_size_correct_fake_func;
	suit_memptr_storage_get_fake.return_val = SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;

	component_type = SUIT_COMPONENT_TYPE_MEM;
	memptr_address = 0x12345000;

	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);

	zassert_equal(SUIT_ERR_CRASH, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");

	/* Make sure the component is not allocated */
	ret = suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "Retrieving impl_data did not fail with appropriate error after failed "
		      "component creation");
}

ZTEST(suit_plat_components_tests, test_create_component_handle_cand_img_memptr_storage_get_fail)
{
	void *impl_data;
	int ret;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;
	suit_memptr_storage_get_fake.return_val = SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;

	component_type = SUIT_COMPONENT_TYPE_CAND_IMG;

	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);

	zassert_equal(SUIT_ERR_CRASH, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");

	/* Make sure the component is not allocated */
	ret = suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "Retrieving impl_data did not fail with appropriate error after failed "
		      "component creation");
}

ZTEST(suit_plat_components_tests, test_create_component_handle_mem_memptr_storage_store_fail)
{
	void *impl_data;
	int ret;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;
	suit_plat_decode_address_size_fake.custom_fake =
		suit_plat_decode_address_size_correct_fake_func;
	suit_memptr_storage_get_fake.custom_fake = suit_memptr_storage_get_correct_fake_func;
	suit_memptr_storage_ptr_store_fake.return_val = SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;

	component_type = SUIT_COMPONENT_TYPE_MEM;
	memptr_address = 0x12345000;

	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);

	zassert_equal(SUIT_ERR_CRASH, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");

	/* Ensure the memptr_storage has been freed */
	zassert_equal(suit_memptr_storage_release_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_release() calls");
	zassert_equal_ptr(suit_memptr_storage_release_fake.arg0_val,
			  suit_memptr_storage_ptr_store_fake.arg0_val);

	/* Make sure the component is not allocated */
	ret = suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "Retrieving impl_data did not fail with appropriate error after failed "
		      "component creation");
}

ZTEST(suit_plat_components_tests, test_create_component_handle_cand_img_ok)
{
	void *impl_data;
	int ret;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;
	suit_memptr_storage_get_fake.custom_fake = suit_memptr_storage_get_correct_fake_func;

	component_type = SUIT_COMPONENT_TYPE_CAND_IMG;

	ret = suit_plat_create_component_handle(&valid_cand_img_id, &component_handles[0]);

	zassert_equal(SUIT_SUCCESS, ret, "Failed to create valid component handle");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");

	suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal_ptr(impl_data, &valid_memptr_storage_handle);
}

ZTEST(suit_plat_components_tests, test_create_component_handle_cand_mfst_ok)
{
	void *impl_data;
	int ret;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;
	suit_memptr_storage_get_fake.custom_fake = suit_memptr_storage_get_correct_fake_func;

	component_type = SUIT_COMPONENT_TYPE_CAND_MFST;

	ret = suit_plat_create_component_handle(&valid_cand_mfst_id, &component_handles[0]);

	zassert_equal(SUIT_SUCCESS, ret, "Failed to create valid component handle");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_get_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_get() calls");
	zassert_equal(suit_plat_decode_address_size_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_address_size() calls");
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_ptr_store() calls");

	suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal_ptr(impl_data, &valid_memptr_storage_handle);
}

ZTEST(suit_plat_components_tests, test_release_component_handle_invalid_handle)
{
	int ret;

	/* Handle address to low */
	ret = suit_plat_release_component_handle((suit_component_t)0);
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");

	/* Handle address to high */
	ret = suit_plat_release_component_handle((suit_component_t)0xFFFFFF00);
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_release_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_release() calls");

	/* Unaligned handle */
	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);
	zassert_equal(SUIT_SUCCESS, ret, "Failed to create valid component handle");
	/* Reset mocks to only look at the results of the next call*/
	mocks_reset();
	FFF_RESET_HISTORY();
	mocks_return_values_reset();

	ret = suit_plat_release_component_handle(
		(suit_component_t)((intptr_t)component_handles[0] + 1));
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_release_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_release() calls");
}

ZTEST(suit_plat_components_tests, test_release_component_handle_already_released)
{
	int ret;

	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);
	zassert_equal(SUIT_SUCCESS, ret, "Failed to create valid component handle");

	ret = suit_plat_release_component_handle(component_handles[0]);
	zassert_equal(SUIT_SUCCESS, ret, "Failed to release component handle");

	/* Reset mocks to only look at the results of the next call*/
	mocks_reset();
	FFF_RESET_HISTORY();
	mocks_return_values_reset();

	ret = suit_plat_release_component_handle(component_handles[0]);
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_release_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_release() calls");
}

ZTEST(suit_plat_components_tests, test_release_component_handle_decoding_type_failed)
{
	int ret;

	ret = suit_plat_create_component_handle(&valid_mem_component_id, &component_handles[0]);
	zassert_equal(SUIT_SUCCESS, ret, "Failed to create valid component handle");

	/* Reset mocks to only look at the results of the next call*/
	mocks_reset();
	FFF_RESET_HISTORY();
	mocks_return_values_reset();

	suit_plat_decode_component_type_fake.return_val = SUIT_PLAT_ERR_CBOR_DECODING;

	ret = suit_plat_release_component_handle(component_handles[0]);
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_release_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_release() calls");
}

ZTEST(suit_plat_components_tests, test_release_component_handle_storage_release_failed)
{
	void *impl_data;
	int ret;

	create_valid_component(SUIT_COMPONENT_TYPE_MEM);
	component_type = SUIT_COMPONENT_TYPE_MEM;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;
	suit_memptr_storage_release_fake.return_val = SUIT_PLAT_ERR_INVAL;

	ret = suit_plat_release_component_handle(component_handles[0]);
	zassert_equal(SUIT_ERR_CRASH, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_release_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_release() calls");

	/* Verify that component data was not cleared */
	suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal_ptr(impl_data, &valid_memptr_storage_handle);
}

ZTEST(suit_plat_components_tests, test_release_component_handle_mem_mapped_ok)
{
	void *impl_data;
	int ret;

	/* TYPE MEM */
	create_valid_component(SUIT_COMPONENT_TYPE_MEM);
	component_type = SUIT_COMPONENT_TYPE_MEM;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;

	ret = suit_plat_release_component_handle(component_handles[0]);
	zassert_equal(SUIT_SUCCESS, ret, "Failed to release component handle");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_release_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_release() calls");

	ret = suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal(
		ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		"Retrieving impl_data did not fail with appropriate error after releasing handle");

	/* TYPE CAND_IMG */
	create_valid_component(SUIT_COMPONENT_TYPE_CAND_IMG);
	component_type = SUIT_COMPONENT_TYPE_CAND_IMG;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;

	ret = suit_plat_release_component_handle(component_handles[0]);
	zassert_equal(SUIT_SUCCESS, ret, "Failed to release component handle");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_release_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_release() calls");

	ret = suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal(
		ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		"Retrieving impl_data did not fail with appropriate error after releasing handle");

	/* TYPE CAND_MFST */
	create_valid_component(SUIT_COMPONENT_TYPE_CAND_MFST);
	component_type = SUIT_COMPONENT_TYPE_CAND_MFST;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;

	ret = suit_plat_release_component_handle(component_handles[0]);
	zassert_equal(SUIT_SUCCESS, ret, "Failed to release component handle");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_release_fake.call_count, 1,
		      "Incorrect number of suit_memptr_storage_release() calls");

	ret = suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal(
		ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		"Retrieving impl_data did not fail with appropriate error after releasing handle");
}

ZTEST(suit_plat_components_tests, test_release_component_handle_other_type_ok)
{
	void *impl_data;
	int ret;

	/* Currently releasing component of unsupported type will succeed but will not
	 * do anything apart marking the component as not used.
	 */
	create_valid_component(SUIT_COMPONENT_TYPE_SOC_SPEC);

	component_type = SUIT_COMPONENT_TYPE_SOC_SPEC;

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;

	ret = suit_plat_release_component_handle(component_handles[0]);
	zassert_equal(SUIT_SUCCESS, ret, "Failed to release component handle");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
	zassert_equal(suit_memptr_storage_release_fake.call_count, 0,
		      "Incorrect number of suit_memptr_storage_release() calls");

	ret = suit_plat_component_impl_data_get(component_handles[0], &impl_data);
	zassert_equal(
		ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		"Retrieving impl_data did not fail with appropriate error after releasing handle");
}

ZTEST(suit_plat_components_tests, test_impl_data_set_component_not_created)
{
	int ret;
	uint8_t data;

	ret = suit_plat_component_impl_data_set(component_handles[0], &data);
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");
}

ZTEST(suit_plat_components_tests, test_impl_data_get_component_not_created)
{
	int ret;
	uint8_t *data_out;

	ret = suit_plat_component_impl_data_get(component_handles[0], (void **)&data_out);
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");
}

ZTEST(suit_plat_components_tests, test_impl_data_set_get_ok)
{
	int ret;
	uint8_t data;
	uint8_t *data_out;

	create_valid_component(SUIT_COMPONENT_TYPE_MEM);

	ret = suit_plat_component_impl_data_set(component_handles[0], &data);
	zassert_equal(SUIT_SUCCESS, ret, "Setting implementation data failed");

	ret = suit_plat_component_impl_data_get(component_handles[0], (void **)&data_out);
	zassert_equal(SUIT_SUCCESS, ret, "Getting implementation data failed");

	zassert_equal_ptr(&data, data_out);
}

ZTEST(suit_plat_components_tests, test_component_id_get_component_not_created)
{
	int ret;
	struct zcbor_string *component_id_out;

	ret = suit_plat_component_id_get(component_handles[0], &component_id_out);
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");
}

ZTEST(suit_plat_components_tests, test_component_id_get_component_ok)
{
	int ret;
	struct zcbor_string *component_id_out;

	create_valid_component(SUIT_COMPONENT_TYPE_MEM);

	ret = suit_plat_component_id_get(component_handles[0], &component_id_out);
	zassert_equal(SUIT_SUCCESS, ret, "Getting component id failed");
	zassert_true(suit_compare_zcbor_strings(component_id_out, &valid_mem_component_id));
}

ZTEST(suit_plat_components_tests, test_type_get_component_not_created)
{
	int ret;
	suit_component_type_t type_out;

	ret = suit_plat_component_type_get(component_handles[0], &type_out);
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 0,
		      "Incorrect number of suit_plat_decode_component_type() calls");
}

ZTEST(suit_plat_components_tests, test_type_get_decoding_failed)
{
	int ret;
	suit_component_type_t type_out;

	create_valid_component(SUIT_COMPONENT_TYPE_MEM);

	suit_plat_decode_component_type_fake.return_val = SUIT_PLAT_ERR_CBOR_DECODING;

	ret = suit_plat_component_type_get(component_handles[0], &type_out);
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Incorrect returned error code");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
}

ZTEST(suit_plat_components_tests, test_type_get_mem_ok)
{
	int ret;
	suit_component_type_t type_out;

	create_valid_component(SUIT_COMPONENT_TYPE_MEM);

	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_correct_fake_func;
	component_type = SUIT_COMPONENT_TYPE_MEM;

	ret = suit_plat_component_type_get(component_handles[0], &type_out);
	zassert_equal(SUIT_SUCCESS, ret, "Getting component type failed");

	zassert_equal(type_out, SUIT_COMPONENT_TYPE_MEM, "Incorrect returned component type");

	zassert_equal(suit_plat_decode_component_type_fake.call_count, 1,
		      "Incorrect number of suit_plat_decode_component_type() calls");
}
