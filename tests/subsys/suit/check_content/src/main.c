/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <suit_platform.h>
#include <suit_plat_decode_util.h>
#include <suit_memptr_storage.h>
#include <suit_sink.h>
#include <suit_manifest_variables.h>

#define TEST_MFST_VAR_NVM_ID	0
#define TEST_MFST_VAR_NVM_VALUE 0xAB

/* Data used as component content for a MEM component */
static uint8_t data_mem[] = {0xde, 0xad, 0xbe, 0xef};

/* Matching value of the content parameter for a MEM component.
 * Note it has the same value as @ref data, however it is kept
 * as a separate value to ensure comparison is done byte by
 * byte, not via pointer comparison.
 */
static uint8_t matching_content_value_mem[] = {0xde, 0xad, 0xbe, 0xef};

/* Matching content parameter for a MEM component. */
static struct zcbor_string matching_content_mem = {
	.value = matching_content_value_mem,
	.len = sizeof(matching_content_value_mem),
};

/* Not matching value of the content parameter for a MEM component. */
static uint8_t not_matching_content_value_mem[] = {0xde, 0xad, 0xbe, 0xed};

/* Not matching content parameter for a MEM component. */
static struct zcbor_string not_matching_content_mem = {
	.value = not_matching_content_value_mem,
	.len = sizeof(not_matching_content_value_mem),
};

ZTEST_SUITE(suit_check_content_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(suit_check_content_tests, test_mem_matching)
{
	/* GIVEN a MEM component pointing to the data */
	uint8_t component_id_value[] = {0x84, 0x44, 0x63, 'M',	'E',  'M',
					0x41, 0x02, 0x41, 0x00, 0x41, 0x00};

	struct zcbor_string matching_src_component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};

	suit_component_t component;
	int err = suit_plat_create_component_handle(&matching_src_component_id, false, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	void *impl_data = NULL;

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err, "test error - suit_plat_component_impl_data_get: %d", err);

	err = suit_memptr_storage_ptr_store((memptr_storage_handle_t)impl_data, data_mem,
					    sizeof(data_mem));
	zassert_equal(SUIT_PLAT_SUCCESS, err, "test error - suit_memptr_storage_ptr_store: %d",
		      err);

	/* WHEN a check content function is called */
	err = suit_plat_check_content(component, &matching_content_mem);

	/* THEN the component contents match its content parameter */
	zassert_equal(SUIT_SUCCESS, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

ZTEST(suit_check_content_tests, test_mem_different_size)
{
	/* GIVEN a MEM component pointing to the data... */
	uint8_t component_id_value[] = {0x84, 0x44, 0x63, 'M',	'E',  'M',
					0x41, 0x02, 0x41, 0x00, 0x41, 0x00};

	struct zcbor_string valid_src_component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};

	suit_component_t component;
	int err = suit_plat_create_component_handle(&valid_src_component_id, false, &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	void *impl_data = NULL;

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err, "test error - suit_plat_component_impl_data_get: %d", err);

	/* ... but indicating wrong data size */
	err = suit_memptr_storage_ptr_store((memptr_storage_handle_t)impl_data, data_mem,
					    sizeof(data_mem) - 1);
	zassert_equal(SUIT_PLAT_SUCCESS, err, "test error - suit_memptr_storage_ptr_store: %d",
		      err);

	/* WHEN a check content function is called */
	err = suit_plat_check_content(component, &matching_content_mem);

	/* THEN appropriate error code is returned */
	zassert_equal(SUIT_FAIL_CONDITION, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

ZTEST(suit_check_content_tests, test_mem_not_matching)
{
	/* GIVEN a MEM component pointing to the data */
	uint8_t component_id_value[] = {0x84, 0x44, 0x63, 'M',	'E',  'M',
					0x41, 0x02, 0x41, 0x00, 0x41, 0x00};

	struct zcbor_string not_matching_src_component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};

	suit_component_t component;
	int err = suit_plat_create_component_handle(&not_matching_src_component_id, false,
						    &component);

	zassert_equal(SUIT_SUCCESS, err, "test error - create_component_handle failed: %d", err);

	void *impl_data = NULL;

	err = suit_plat_component_impl_data_get(component, &impl_data);
	zassert_equal(SUIT_SUCCESS, err, "test error - suit_plat_component_impl_data_get: %d", err);

	err = suit_memptr_storage_ptr_store((memptr_storage_handle_t)impl_data, data_mem,
					    sizeof(data_mem));
	zassert_equal(SUIT_PLAT_SUCCESS, err, "test error - suit_memptr_storage_ptr_store: %d",
		      err);

	/* WHEN a check content function is called */
	err = suit_plat_check_content(component, &not_matching_content_mem);

	/* THEN the component contents match its content parameter */
	zassert_equal(SUIT_FAIL_CONDITION, err, "unexpected error code: %d", err);

	/* Cleanup */
	err = suit_plat_release_component_handle(component);
	zassert_equal(SUIT_SUCCESS, err, "test error - failed to cleanup component handle: %d",
		      err);
}

ZTEST(suit_check_content_tests, test_mfst_var_nvm_matching)
{
	/* [h'MFST_VAR', h'00'] */
	uint8_t component_id_value[] = {0x82, 0x49, 0x68, 'M', 'F',  'S', 'T',
					'_',  'V',  'A',  'R', 0x41, 0x00};
	struct zcbor_string src_component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	/* 0xAB */
	uint8_t test_value[] = {0x18, TEST_MFST_VAR_NVM_VALUE};
	struct zcbor_string test_value_zcbor = {
		.value = test_value,
		.len = sizeof(test_value),
	};
	suit_component_t component;
	int ret;

	/* GIVEN that MFST_VAR/0 value is initialized with sample value... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, TEST_MFST_VAR_NVM_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR value before the test");

	/* ... and the component handle for the MFST_VAR/0 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&src_component_id, false, &component),
		      SUIT_SUCCESS, "Unable to create MFST_VAR component");

	/* WHEN a check content function is called */
	ret = suit_plat_check_content(component, &test_value_zcbor);

	/* THEN the component contents match its content parameter */
	zassert_equal(ret, SUIT_SUCCESS, "Unexpected error code: %d", ret);

	/* ... and the component can be safely released */
	ret = suit_plat_release_component_handle(component);
	zassert_equal(ret, SUIT_SUCCESS, "Failed to release component handle after the test: %d",
		      ret);
}

ZTEST(suit_check_content_tests, test_mfst_var_nvm_not_matching)
{
	/* [h'MFST_VAR', h'00'] */
	uint8_t component_id_value[] = {0x82, 0x49, 0x68, 'M', 'F',  'S', 'T',
					'_',  'V',  'A',  'R', 0x41, 0x00};
	struct zcbor_string src_component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	/* 0xBA */
	uint8_t test_value[] = {0x18, 0xBA};
	struct zcbor_string test_value_zcbor = {
		.value = test_value,
		.len = sizeof(test_value),
	};
	suit_component_t component;
	int ret;

	/* GIVEN that MFST_VAR/0 value is initialized with sample value... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, TEST_MFST_VAR_NVM_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR value before the test");

	/* ... and the component handle for the MFST_VAR/0 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&src_component_id, false, &component),
		      SUIT_SUCCESS, "Unable to create MFST_VAR component");

	/* WHEN a check content function is called */
	ret = suit_plat_check_content(component, &test_value_zcbor);

	/* THEN the component contents does not match its content parameter */
	zassert_equal(ret, SUIT_FAIL_CONDITION, "Unexpected error code: %d", ret);

	/* ... and the component can be safely released */
	ret = suit_plat_release_component_handle(component);
	zassert_equal(ret, SUIT_SUCCESS, "Failed to release component handle after the test: %d",
		      ret);
}

ZTEST(suit_check_content_tests, test_mfst_var_invalid_content)
{
	/* [h'MFST_VAR', h'00'] */
	uint8_t component_id_value[] = {0x82, 0x49, 0x68, 'M', 'F',  'S', 'T',
					'_',  'V',  'A',  'R', 0x41, 0x00};
	struct zcbor_string src_component_id = {
		.value = component_id_value,
		.len = sizeof(component_id_value),
	};
	/* [0x00] */
	uint8_t test_value[] = {0x81, 0x00};
	struct zcbor_string test_value_zcbor = {
		.value = test_value,
		.len = sizeof(test_value),
	};
	suit_component_t component;
	int ret;

	/* GIVEN that MFST_VAR/0 value is initialized with sample value... */
	zassert_equal(suit_mfst_var_set(TEST_MFST_VAR_NVM_ID, TEST_MFST_VAR_NVM_VALUE),
		      SUIT_PLAT_SUCCESS, "Unable to set MFST_VAR value before the test");

	/* ... and the component handle for the MFST_VAR/0 is successfully created */
	zassert_equal(suit_plat_create_component_handle(&src_component_id, false, &component),
		      SUIT_SUCCESS, "Unable to create MFST_VAR component");

	/* WHEN a check content function is called */
	ret = suit_plat_check_content(component, &test_value_zcbor);

	/* THEN the component contents does not match its content parameter */
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER, "Unexpected error code: %d", ret);

	/* ... and the component can be safely released */
	ret = suit_plat_release_component_handle(component);
	zassert_equal(ret, SUIT_SUCCESS, "Failed to release component handle after the test: %d",
		      ret);
}
