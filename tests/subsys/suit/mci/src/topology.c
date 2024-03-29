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

static int compare_manifest_class_id(const suit_manifest_class_id_t *manifest_class_id1,
				     const suit_manifest_class_id_t *manifest_class_id2)
{
	return memcmp(manifest_class_id1->raw, manifest_class_id2->raw,
		      sizeof(((suit_manifest_class_id_t *)0)->raw));
}

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

ZTEST_SUITE(mci_topology_tests, NULL, test_suit_setup, NULL, NULL, NULL);

ZTEST(mci_topology_tests, test_duplicate_ids_in_supported_manifest)
{
	int rc = 0;
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	size_t output_size = OUTPUT_MAX_SIZE;

	ret = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", ret);

	for (int i = 0; i < output_size; ++i) {
		for (int j = i + 1; j < output_size; ++j) {
			rc = compare_manifest_class_id(result_class_info[i].class_id,
						       result_class_info[j].class_id);
			zassert_true((rc != 0), "the same uuid used for two manifests");
		}
	}

#ifdef CONFIG_SUIT_EXECUTION_MODE
	ret = suit_execution_mode_set(EXECUTION_MODE_INVOKE_RECOVERY);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unable to change execution mode");

	output_size = OUTPUT_MAX_SIZE;
	ret = suit_mci_supported_manifest_class_ids_get(result_class_info, &output_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS,
		      "suit_mci_supported_manifest_class_ids_get returned (%d)", ret);

	for (int i = 0; i < output_size; ++i) {
		for (int j = i + 1; j < output_size; ++j) {
			rc = compare_manifest_class_id(result_class_info[i].class_id,
						       result_class_info[j].class_id);
			zassert_true((rc != 0), "the same uuid used for two manifests");
		}
	}
#endif /* CONFIG_SUIT_EXECUTION_MODE */
}

ZTEST(mci_topology_tests, test_duplicate_ids_in_invoke_order)
{
	int rc = 0;
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	size_t output_size = OUTPUT_MAX_SIZE;

	ret = suit_mci_invoke_order_get(result_uuid, &output_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_mci_invoke_order_get returned (%d)", ret);

	for (int i = 0; i < output_size; ++i) {
		for (int j = i + 1; j < output_size; ++j) {
			rc = compare_manifest_class_id(result_uuid[i], result_uuid[j]);
			zassert_true((rc != 0), "the same uuid used for two manifests");
		}
	}

#ifdef CONFIG_SUIT_EXECUTION_MODE
	ret = suit_execution_mode_set(EXECUTION_MODE_INVOKE_RECOVERY);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unable to change execution mode");

	output_size = OUTPUT_MAX_SIZE;
	ret = suit_mci_invoke_order_get(result_uuid, &output_size);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_mci_invoke_order_get returned (%d)", ret);

	for (int i = 0; i < output_size; ++i) {
		for (int j = i + 1; j < output_size; ++j) {
			rc = compare_manifest_class_id(result_uuid[i], result_uuid[j]);
			zassert_true((rc != 0), "the same uuid used for two manifests");
		}
	}
#endif /* CONFIG_SUIT_EXECUTION_MODE */
}
