/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <suit_manifest_variables.h>
#include <suit_storage.h>

static void test_suite_before(void *f)
{
	suit_plat_err_t ret = suit_storage_init();

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Failed to initialize SUIT storage module, error: %i",
		      ret);
}

ZTEST_SUITE(mfst_variables_tests, NULL, NULL, test_suite_before, NULL, NULL);

ZTEST(mfst_variables_tests, test_var_nvm_OK)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	for (size_t i = 0; i < CONFIG_SUIT_MANIFEST_VARIABLES_NVM_COUNT; i++) {
		uint32_t id = CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID + i;
		/* size of NVM based variable is limited to 8 bits
		 */
		uint32_t value = id & 0xFF;

		ret = suit_mfst_var_set(id, value);
		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "suit_mfst_var_set failed, id: %d, value: %d, error %i", id, value,
			      ret);
	}

	for (size_t i = 0; i < CONFIG_SUIT_MANIFEST_VARIABLES_NVM_COUNT; i++) {
		uint32_t id = CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID + i;
		uint32_t expected_value = id & 0xFF;
		uint32_t value = 0;
		uint32_t access_mask = 0;

		ret = suit_mfst_var_get_access_mask(id, &access_mask);
		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "suit_mfst_var_get_access_mask failed, id: %d, error %i", id, ret);
		zassert_equal(access_mask, CONFIG_SUIT_MANIFEST_VARIABLES_NVM_ACCESS_MASK, "id: %d",
			      id);

		ret = suit_mfst_var_get(id, &value);
		zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_mfst_var_get failed, id: %d, error %i",
			      id, ret);
		zassert_equal(value, expected_value, "id: %d, error %i", id, ret);
	}
}

ZTEST(mfst_variables_tests, test_var_plat_volatile_OK)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	for (size_t i = 0; i < CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_COUNT; i++) {
		uint32_t id = CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID + i;
		uint32_t value = id;

		ret = suit_mfst_var_set(id, value);
		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "suit_mfst_var_set failed, id: %d, value: %d, error %i", id, value,
			      ret);
	}

	for (size_t i = 0; i < CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_COUNT; i++) {
		uint32_t id = CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID + i;
		uint32_t expected_value = id;
		uint32_t value = 0;
		uint32_t access_mask = 0;

		ret = suit_mfst_var_get_access_mask(id, &access_mask);
		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "suit_mfst_var_get_access_mask failed, id: %d, error %i", id, ret);
		zassert_equal(access_mask, CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_ACCESS_MASK,
			      "id: %d", id);

		ret = suit_mfst_var_get(id, &value);
		zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_mfst_var_get failed, id: %d, error %i",
			      id, ret);
		zassert_equal(value, expected_value, "id: %d, error %i", id, ret);
	}
}

ZTEST(mfst_variables_tests, test_var_mfst_volatile_OK)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	for (size_t i = 0; i < CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_COUNT; i++) {
		uint32_t id = CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID + i;
		uint32_t value = id;

		ret = suit_mfst_var_set(id, value);
		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "suit_mfst_var_set failed, id: %d, value: %d, error %i", id, value,
			      ret);
	}

	for (size_t i = 0; i < CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_COUNT; i++) {
		uint32_t id = CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID + i;
		uint32_t expected_value = id;
		uint32_t value = 0;
		uint32_t access_mask = 0;

		ret = suit_mfst_var_get_access_mask(id, &access_mask);
		zassert_equal(ret, SUIT_PLAT_SUCCESS,
			      "suit_mfst_var_get_access_mask failed, id: %d, error %i", id, ret);
		zassert_equal(access_mask, CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_ACCESS_MASK,
			      "id: %d", id);

		ret = suit_mfst_var_get(id, &value);
		zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_mfst_var_get failed, id: %d, error %i",
			      id, ret);
		zassert_equal(value, expected_value, "id: %d, error %i", id, ret);
	}
}

ZTEST(mfst_variables_tests, test_var_nvm_overflow)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	uint32_t id = CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID;
	/* size of NVM based variable is limited to 8 bits
	 */
	uint32_t value = 0x1FF;

	ret = suit_mfst_var_set(id, value);

	zassert_equal(ret, SUIT_PLAT_ERR_SIZE, "id: %d, value: %d, error %i", id, value, ret);
}

ZTEST(mfst_variables_tests, test_var_not_found)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	uint32_t id = -1;
	uint32_t value = 0;
	uint32_t access_mask = 0;

	ret = suit_mfst_var_get_access_mask(id, &access_mask);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "id: %d, error %i", id, ret);

	ret = suit_mfst_var_get(id, &value);
	zassert_equal(ret, SUIT_PLAT_ERR_NOT_FOUND, "id: %d, error %i", id, ret);
}
