/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <suit_mci.h>
#ifdef CONFIG_SUIT_STORAGE
#include <suit_storage.h>
#endif /* CONFIG_SUIT_STORAGE */
#ifdef CONFIG_SUIT_EXECUTION_MODE
#include <suit_execution_mode.h>
#endif /* CONFIG_SUIT_EXECUTION_MODE */

#define OUTPUT_MAX_SIZE 32
static const suit_uuid_t *result_uuid[OUTPUT_MAX_SIZE];
static suit_manifest_class_info_t result_class_info[OUTPUT_MAX_SIZE];

static void *test_suit_setup(void)
{
	suit_plat_err_t ret = 0;

#ifdef CONFIG_SUIT_STORAGE
	ret = suit_storage_init();
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unable to initialize SUIT storage");
#endif /* CONFIG_SUIT_STORAGE */

#ifdef CONFIG_SUIT_EXECUTION_MODE
	ret = suit_execution_mode_set(EXECUTION_MODE_INVOKE);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unable to set execution mode");
#endif /* CONFIG_SUIT_EXECUTION_MODE */

	ret = suit_mci_init();
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unable to initialize MCI module");

	return NULL;
}

ZTEST_SUITE(mci_api_positive_scenarios_tests, NULL, test_suit_setup, NULL, NULL, NULL);

ZTEST(mci_api_positive_scenarios_tests, test_supported_manifest_class_ids_get)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);
}

ZTEST(mci_api_positive_scenarios_tests, test_invoke_order_get)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_invoke_order_get(result_uuid, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "suit_mci_invoke_order_get returned (%d)", rc);
}

ZTEST(mci_api_positive_scenarios_tests, test_downgrade_prevention_policy_get)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		suit_downgrade_prevention_policy_t policy;

		rc = suit_mci_downgrade_prevention_policy_get(result_class_info[i].class_id,
							      &policy);
		zassert_equal(rc, SUIT_PLAT_SUCCESS,
			      "suit_mci_downgrade_prevention_policy_get returned (%d)", rc);
	}
}

ZTEST(mci_api_positive_scenarios_tests, test_independent_update_policy_get)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		suit_independent_updateability_policy_t policy;

		rc = suit_mci_independent_update_policy_get(result_class_info[i].class_id, &policy);
		zassert_equal(rc, SUIT_PLAT_SUCCESS,
			      "suit_mci_independent_update_policy_get returned (%d)", rc);
	}
}

ZTEST(mci_api_positive_scenarios_tests, test_manifest_class_id_validate)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		rc = suit_mci_manifest_class_id_validate(result_class_info[i].class_id);
		zassert_equal(rc, SUIT_PLAT_SUCCESS,
			      "suit_mci_manifest_class_id_validate returned (%d)", rc);
	}
}

ZTEST(mci_api_positive_scenarios_tests, test_signing_key_id_validate)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		uint32_t key_id = 0;

		rc = suit_mci_signing_key_id_validate(result_class_info[i].class_id, key_id);
		zassert_true((rc == MCI_ERR_NOACCESS || rc == SUIT_PLAT_SUCCESS ||
			      rc == MCI_ERR_WRONGKEYID),
			     "suit_mci_signing_key_id_validate returned (%d)", rc);
	}
}

ZTEST(mci_api_positive_scenarios_tests, test_processor_start_rights_validate)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		int processor_id = 0;

		rc = suit_mci_processor_start_rights_validate(result_class_info[i].class_id,
							      processor_id);
		zassert_true((rc == MCI_ERR_NOACCESS || rc == SUIT_PLAT_SUCCESS),
			     "suit_mci_processor_start_rights_validate returned (%d)", rc);
	}
}

ZTEST(mci_api_positive_scenarios_tests, test_memory_access_rights_validate)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		void *address = &address;
		size_t mem_size = sizeof(void *);

		rc = suit_mci_memory_access_rights_validate(result_class_info[i].class_id, address,
							    mem_size);
		zassert_true((rc == MCI_ERR_NOACCESS || rc == SUIT_PLAT_SUCCESS),
			     "suit_mci_memory_access_rights_validate returned (%d)", rc);
	}
}

ZTEST(mci_api_positive_scenarios_tests, test_platform_specific_component_rights_validate)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		int platform_specific_component_number = 0;

		rc = suit_mci_platform_specific_component_rights_validate(
			result_class_info[i].class_id, platform_specific_component_number);
		zassert_true((rc == MCI_ERR_NOACCESS || rc == SUIT_PLAT_SUCCESS),
			     "suit_mci_platform_specific_component_rights_validate returned (%d)",
			     rc);
	}
}

ZTEST(mci_api_positive_scenarios_tests, test_manifest_parent_child_declaration_validate)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		rc = suit_mci_manifest_parent_child_declaration_validate(
			result_class_info[0].class_id, result_class_info[i].class_id);
		zassert_true((rc == MCI_ERR_NOACCESS || rc == SUIT_PLAT_SUCCESS),
			     "suit_mci_manifest_parent_child_validate returned (%d)", rc);
	}
}

ZTEST(mci_api_positive_scenarios_tests, test_manifest_process_dependency_validate)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		rc = suit_mci_manifest_process_dependency_validate(result_class_info[0].class_id,
								   result_class_info[i].class_id);
		zassert_true((rc == MCI_ERR_NOACCESS || rc == SUIT_PLAT_SUCCESS),
			     "suit_mci_manifest_parent_child_validate returned (%d)", rc);
	}
}

ZTEST(mci_api_positive_scenarios_tests, test_vendor_id_for_manifest_class_id_get)
{
	int rc = 0;
	size_t output_size = OUTPUT_MAX_SIZE;

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	for (int i = 0; i < output_size; ++i) {
		const suit_uuid_t *vendor_id = NULL;

		rc = suit_mci_vendor_id_for_manifest_class_id_get(result_class_info[i].class_id,
								  &vendor_id);
		zassert_equal(rc, SUIT_PLAT_SUCCESS,
			      "suit_mci_vendor_id_for_manifest_class_id_get returned (%d)", rc);
	}
}
