/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <suit_mci.h>
#include <suit_execution_mode.h>
#ifdef CONFIG_SUIT_STORAGE
#include <suit_storage.h>
#endif /* CONFIG_SUIT_STORAGE */

#define OUTPUT_MAX_SIZE 32
static const suit_uuid_t *result_uuid[OUTPUT_MAX_SIZE];
static suit_manifest_class_info_t result_class_info[OUTPUT_MAX_SIZE];

static void *test_suit_setup(void)
{
	int ret = 0;

#ifdef CONFIG_SUIT_STORAGE
	ret = suit_storage_init();
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unable to initialize SUIT storage");
#endif /* CONFIG_SUIT_STORAGE */

	ret = suit_mci_init();
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unable to initialize MCI module");

	return NULL;
}

ZTEST_SUITE(mci_snity_tests, NULL, test_suit_setup, NULL, NULL, NULL);

ZTEST(mci_snity_tests, test_null_pointers)
{
	const suit_manifest_class_id_t unsupported_manifest_class_id = {
		{'u', 'n', 's', 'u', 'p', 'p', 'o', 'r', 't', 'e', 'd', '!', '!', '!', ' ', ' '}};
	size_t output_size = OUTPUT_MAX_SIZE;
	uint32_t key_id = 0;
	int processor_id = 0;
	void *mem_address = &mem_address;
	size_t mem_size = sizeof(mem_address);
	int platform_specific_component_number = 0;
	suit_downgrade_prevention_policy_t policy;
	suit_independent_updateability_policy_t update_policy;
	int rc = SUIT_PLAT_SUCCESS;

	rc = suit_mci_nordic_vendor_id_get(NULL);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL, "suit_mci_nordic_vendor_id_get returned (%d)", rc);

	output_size = OUTPUT_MAX_SIZE;
	rc = suit_mci_supported_manifest_class_ids_get(NULL, &output_size);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, NULL);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	output_size = OUTPUT_MAX_SIZE;
	rc = suit_mci_invoke_order_get(NULL, &output_size);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL, "suit_mci_invoke_order_get returned (%d)", rc);

	rc = suit_mci_invoke_order_get(result_uuid, NULL);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL, "suit_mci_invoke_order_get returned (%d)", rc);

	rc = suit_mci_downgrade_prevention_policy_get(NULL, &policy);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_downgrade_prevention_policy_get returned (%d)", rc);

	rc = suit_mci_downgrade_prevention_policy_get(&unsupported_manifest_class_id, NULL);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_downgrade_prevention_policy_get returned (%d)", rc);

	rc = suit_mci_independent_update_policy_get(NULL, &update_policy);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_independent_update_policy_get returned (%d)", rc);

	rc = suit_mci_independent_update_policy_get(&unsupported_manifest_class_id, NULL);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_independent_update_policy_get returned (%d)", rc);

	rc = suit_mci_signing_key_id_validate(NULL, key_id);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL, "suit_mci_signing_key_id_validate returned (%d)",
		      rc);

	rc = suit_mci_manifest_class_id_validate(NULL);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL, "suit_mci_manifest_class_id_validate returned (%d)",
		      rc);

	rc = suit_mci_processor_start_rights_validate(NULL, processor_id);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_processor_start_rights_validate returned (%d)", rc);

	rc = suit_mci_memory_access_rights_validate(NULL, mem_address, mem_size);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_memory_access_rights_validate returned (%d)", rc);

	rc = suit_mci_memory_access_rights_validate(&unsupported_manifest_class_id, NULL, mem_size);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_memory_access_rights_validate returned (%d)", rc);

	rc = suit_mci_platform_specific_component_rights_validate(
		NULL, platform_specific_component_number);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_platform_specific_component_rights_validate returned (%d)", rc);

	rc = suit_mci_manifest_parent_child_declaration_validate(&unsupported_manifest_class_id,
								 NULL);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_manifest_parent_child_validate returned (%d)", rc);

	rc = suit_mci_manifest_parent_child_declaration_validate(NULL,
								 &unsupported_manifest_class_id);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_manifest_parent_child_validate returned (%d)", rc);

	rc = suit_mci_manifest_process_dependency_validate(&unsupported_manifest_class_id, NULL);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_manifest_parent_child_validate returned (%d)", rc);

	rc = suit_mci_manifest_process_dependency_validate(NULL, &unsupported_manifest_class_id);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_manifest_parent_child_validate returned (%d)", rc);

	rc = suit_mci_vendor_id_for_manifest_class_id_get(NULL, result_uuid);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_vendor_id_for_manifest_class_id_get returned (%d)", rc);

	rc = suit_mci_vendor_id_for_manifest_class_id_get(&unsupported_manifest_class_id, NULL);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_vendor_id_for_manifest_class_id_get returned (%d)", rc);
}

ZTEST(mci_snity_tests, test_invalid_params)
{
	void *mem_address = &mem_address;
	size_t mem_size = sizeof(mem_address);
	int rc = SUIT_PLAT_SUCCESS;

	rc = suit_mci_memory_access_rights_validate(NULL, mem_address, mem_size);
	zassert_equal(rc, SUIT_PLAT_ERR_INVAL,
		      "suit_mci_memory_access_rights_validate returned (%d)", rc);
}

ZTEST(mci_snity_tests, test_output_buffer_too_small)
{
	size_t output_size = OUTPUT_MAX_SIZE;
	size_t supported_manifest_count = 0;
	size_t invokable_manifest_count = 0;
	int rc = SUIT_PLAT_SUCCESS;

	output_size = OUTPUT_MAX_SIZE;
	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d), isn't %d manifests "
		      "enough?!",
		      rc, OUTPUT_MAX_SIZE);
	supported_manifest_count = output_size;

	output_size = supported_manifest_count;
	rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);

	if (supported_manifest_count > 1) {
		output_size = supported_manifest_count - 1;
		rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
		zassert_equal(rc, SUIT_PLAT_ERR_SIZE,
			      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);
	}

	if (supported_manifest_count > 0) {
		output_size = 0;
		rc = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
		zassert_equal(rc, SUIT_PLAT_ERR_SIZE,
			      "suit_mci_supported_manifest_class_ids_get returned (%d)", rc);
	}

	suit_execution_mode_set(EXECUTION_MODE_INVOKE);
	output_size = OUTPUT_MAX_SIZE;
	rc = suit_mci_invoke_order_get(result_uuid, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS,
		      "suit_mci_invoke_order_get returned (%d), isn't %d manifests enough?!", rc,
		      OUTPUT_MAX_SIZE);
	invokable_manifest_count = output_size;

	output_size = invokable_manifest_count;
	rc = suit_mci_invoke_order_get(result_uuid, &output_size);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "suit_mci_invoke_order_get returned (%d)", rc);

	if (invokable_manifest_count > 1) {
		output_size = invokable_manifest_count - 1;
		rc = suit_mci_invoke_order_get(result_uuid, &output_size);
		zassert_equal(rc, SUIT_PLAT_ERR_SIZE, "suit_mci_invoke_order_get returned (%d)",
			      rc);
	}

	if (invokable_manifest_count > 0) {
		output_size = 0;
		rc = suit_mci_invoke_order_get(result_uuid, &output_size);
		zassert_equal(rc, SUIT_PLAT_ERR_SIZE, "suit_mci_invoke_order_get returned (%d)",
			      rc);
	}
}
