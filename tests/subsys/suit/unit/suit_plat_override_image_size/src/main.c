/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <mocks.h>

#define TEST_FAKE_ADDRESS ((intptr_t)0xDEADBEEF)
#define TEST_FAKE_SIZE	  ((size_t)42)

static uint8_t component_id_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',  0x41, 0x02,
				       0x45, 0x1A, 0xC0, 0xFE, 0x00, 0x00, 0x41, 0x04};

static struct zcbor_string component_id = {
	.value = component_id_value,
	.len = sizeof(component_id_value),
};

static suit_component_t component;

static suit_plat_err_t
suit_plat_decode_component_type_fake_mem_ok(struct zcbor_string *component_id,
					    suit_component_type_t *type)
{
	*type = SUIT_COMPONENT_TYPE_MEM;
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_plat_decode_component_type_fake_false(struct zcbor_string *component_id,
								  suit_component_type_t *type)
{
	return SUIT_PLAT_ERR_CRASH;
}

static suit_plat_err_t
suit_plat_decode_component_type_fake_cand_img_ok(struct zcbor_string *component_id,
						 suit_component_type_t *type)
{
	*type = SUIT_COMPONENT_TYPE_CAND_IMG;
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_plat_decode_address_size_fake_ok(struct zcbor_string *component_id,
							     intptr_t *run_address, size_t *size)
{
	*run_address = TEST_FAKE_ADDRESS;
	*size = TEST_FAKE_SIZE;
	return SUIT_PLAT_SUCCESS;
}

static int suit_memptr_storage_ptr_get_fake_invalid_record(memptr_storage_handle_t handle,
							   const uint8_t **payload_ptr,
							   size_t *payload_size)
{
	return SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;
}

static int suit_memptr_storage_ptr_store_fake_unallocated_record(memptr_storage_handle_t handle,
								 const uint8_t *payload_ptr,
								 size_t payload_size)
{
	return SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD;
}

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();

	/* Create MEM component handle */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_fake_mem_ok;
	suit_plat_decode_address_size_fake.custom_fake = suit_plat_decode_address_size_fake_ok;

	int err = suit_plat_create_component_handle(&component_id, &component);

	zassert_equal(SUIT_SUCCESS, err, "test setup error - create_component_handle failed: %d",
		      err);
}

static void test_after(void *data)
{
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_fake_mem_ok;

	int err = suit_plat_release_component_handle(component);

	zassert_equal(SUIT_SUCCESS, err,
		      "test teardown error - suit_plat_release_component failed: %d", err);
}

ZTEST_SUITE(suit_platform_override_image_size_tests, NULL, NULL, test_before, test_after, NULL);

ZTEST(suit_platform_override_image_size_tests, test_suit_plat_override_image_size_init_and_override)
{
	/* GIVEN a MEM-type component */
	/* WHEN a component handle is created (in test setup step) */
	/* THEN memptr_storage size should be set to 0 */
	int expected_call_count = 1;
	size_t expected_size = 0;

	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, expected_call_count,
		      "Wrong initial size store call count",
		      suit_memptr_storage_ptr_store_fake.call_count, expected_call_count);
	zassert_equal(suit_memptr_storage_ptr_store_fake.arg2_history[0], expected_size,
		      "Wrong initial size: %d instead of %d",
		      suit_memptr_storage_ptr_store_fake.arg2_history[0], expected_size);

	/* WHEN size override is called */
	int err = suit_plat_override_image_size(component, TEST_FAKE_SIZE);

	zassert_equal(SUIT_SUCCESS, err, "Failed to override image size: %d", err);

	/* THEN memptr_storage size should be updated to given value */
	expected_call_count = 2;
	expected_size = TEST_FAKE_SIZE;
	zassert_equal(suit_memptr_storage_ptr_store_fake.call_count, expected_call_count,
		      "Wrong size store call count: %d instead of %d",
		      suit_memptr_storage_ptr_store_fake.call_count, expected_call_count);
	zassert_equal(suit_memptr_storage_ptr_store_fake.arg2_history[1], expected_size,
		      "Wrong size: %d instead of %d",
		      suit_memptr_storage_ptr_store_fake.arg2_history[1], expected_size);
}

ZTEST(suit_platform_override_image_size_tests,
	test_suit_plat_override_image_size_wrong_component_type)
{
	/* GIVEN a MEM-type component (created in test setup step)*/
	/* WHEN internal function call is going to fail */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_fake_false;

	/* WHEN size override is called */
	int err = suit_plat_override_image_size(component, TEST_FAKE_SIZE);

	/* THEN appropriate error code is returned */
	int expected_error = SUIT_ERR_DECODING;

	zassert_equal(expected_error, err, "Unexpected error code: %d instead of %d", err,
		      expected_error);
}

ZTEST(suit_platform_override_image_size_tests, test_suit_plat_override_image_size_fail_get)
{
	/* GIVEN a MEM-type component (created in test setup step)*/
	/* WHEN internal function is going to fail */
	suit_memptr_storage_ptr_get_fake.custom_fake =
		suit_memptr_storage_ptr_get_fake_invalid_record;

	/* WHEN size override is called */
	int err = suit_plat_override_image_size(component, TEST_FAKE_SIZE);
	/* THEN appropriate error code is returned */
	int expected_error = SUIT_ERR_CRASH;

	zassert_equal(expected_error, err, "Unexpected error code: %d instead of %d", err,
		      expected_error);
}

ZTEST(suit_platform_override_image_size_tests, test_suit_plat_override_image_size_fail_store)
{
	/* GIVEN a MEM-type component (created in test setup step)*/
	/* WHEN internal function is going to fail */
	suit_memptr_storage_ptr_store_fake.custom_fake =
		suit_memptr_storage_ptr_store_fake_unallocated_record;
	/* WHEN size override is called */
	int err = suit_plat_override_image_size(component, TEST_FAKE_SIZE);
	/* THEN appropriate error code is returned */
	int expected_error = SUIT_ERR_CRASH;

	zassert_equal(expected_error, err, "Unexpected error code: %d instead of %d", err,
		      expected_error);
}

ZTEST(suit_platform_override_image_size_tests,
	test_suit_plat_override_image_size_component_released)
{
	/* GIVEN a released MEM-type component */
	suit_memptr_storage_release_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_plat_release_component_handle(component);

	/* WHEN size override is called */
	int err = suit_plat_override_image_size(component, TEST_FAKE_SIZE);
	/* THEN appropriate error code is returned */
	int expected_error = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;

	zassert_equal(expected_error, err, "Unexpected error code: %d instead of %d", err,
		      expected_error);

	/* Cleanup: create component to satisfy test_after */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_fake_mem_ok;
	suit_plat_decode_address_size_fake.custom_fake = suit_plat_decode_address_size_fake_ok;
	err = suit_plat_create_component_handle(&component_id, &component);
	zassert_equal(SUIT_SUCCESS, err, "test setup error - create_component_handle failed: %d",
		      err);
}

ZTEST(suit_platform_override_image_size_tests,
	test_suit_plat_override_image_size_unsupported_component_type)
{
	/* GIVEN a MEM-type component (created in test setup step)*/
	/* WHEN an unsupported component type is used */
	suit_plat_decode_component_type_fake.custom_fake =
		suit_plat_decode_component_type_fake_cand_img_ok;

	/* WHEN size override is called */
	int err = suit_plat_override_image_size(component, TEST_FAKE_SIZE);
	/* THEN appropriate error code is returned */
	int expected_error = SUIT_ERR_UNSUPPORTED_COMMAND;

	zassert_equal(expected_error, err, "Unexpected error code: %d instead of %d", err,
		      expected_error);
}

ZTEST(suit_platform_override_image_size_tests,
	test_suit_plat_override_image_size_fail_decode_address_size)
{
	/* GIVEN a MEM-type component (created in test setup step)*/
	/* WHEN internal function is going to fail */
	suit_plat_decode_address_size_fake.custom_fake = NULL;
	suit_plat_decode_address_size_fake.return_val = SUIT_PLAT_ERR_CBOR_DECODING;
	/* WHEN size override is called */
	int err = suit_plat_override_image_size(component, TEST_FAKE_SIZE);
	/* THEN appropriate error code is returned */
	int expected_error = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;

	zassert_equal(expected_error, err, "Unexpected error code: %d instead of %d", err,
		      expected_error);
}

ZTEST(suit_platform_override_image_size_tests, test_suit_plat_override_image_size_exceeding_size)
{
	/* GIVEN a MEM-type component (created in test setup step)*/
	/* WHEN size override is called with size exceeding the boundaries */
	int err = suit_plat_override_image_size(component, TEST_FAKE_SIZE + 1);
	/* THEN appropriate error code is returned */
	int expected_error = SUIT_ERR_UNSUPPORTED_PARAMETER;

	zassert_equal(expected_error, err, "Unexpected error code: %d instead of %d", err,
		      expected_error);
}
