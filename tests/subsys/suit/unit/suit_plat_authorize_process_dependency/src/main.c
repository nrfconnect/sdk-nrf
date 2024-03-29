/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <mocks.h>

static suit_manifest_class_id_t parent_class_id = {{0x97, 0x05, 0x48, 0x23, 0x4c, 0x3d, 0x59, 0xa1,
						    0x89, 0x86, 0xa5, 0x46, 0x60, 0xa1, 0x4b,
						    0x0a}};

static suit_manifest_class_id_t child_class_id = {{0x5b, 0x46, 0x9f, 0xd1, 0x90, 0xee, 0x53, 0x9c,
						   0xa3, 0x18, 0x68, 0x1b, 0x03, 0x69, 0x5e, 0x36}};

static struct zcbor_string parent_component_id = {
	.value = (const uint8_t *)0x4321,
	.len = 321,
};

static struct zcbor_string child_component_id = {
	.value = (const uint8_t *)0xabcd,
	.len = 987,
};

static void test_before(void *data)
{
	/* Reset mocks */
	mocks_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

static suit_plat_err_t
suit_plat_decode_manifest_class_id_predefined_fake_func(struct zcbor_string *component_id,
							suit_manifest_class_id_t **class_id)
{
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	if (component_id == &parent_component_id) {
		*class_id = &parent_class_id;
	} else if (component_id == &child_component_id) {
		*class_id = &child_class_id;
	} else {
		zassert_unreachable("Invalid manifest component ID value");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	return SUIT_PLAT_SUCCESS;
}

ZTEST_SUITE(suit_platform_authorize_process_dependency_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_platform_authorize_process_dependency_tests, test_null_parent_component_id)
{
	int class_id_decode_results[] = {SUIT_PLAT_ERR_INVAL};

	SET_RETURN_SEQ(suit_plat_decode_manifest_class_id, class_id_decode_results,
		       ARRAY_SIZE(class_id_decode_results));

	int ret = suit_plat_authorize_process_dependency(NULL, &child_component_id, SUIT_SEQ_PARSE);

	/* Manifest sequence authorization fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Failed to catch null argument (NULL, _, PARSE)");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count,
		      ARRAY_SIZE(class_id_decode_results),
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.arg0_history[0], NULL,
		      "Invalid manifest class ID value (1st call)");
	zassert_not_equal(
		suit_plat_decode_manifest_class_id_fake.arg1_history[0], NULL,
		"The API must provide a valid pointer, to decode manifest class ID (1st call)");
	zassert_equal(suit_mci_manifest_process_dependency_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_process_dependency_validate() calls");
}

ZTEST(suit_platform_authorize_process_dependency_tests, test_null_child_component_id)
{
	int class_id_decode_results[] = {SUIT_PLAT_SUCCESS, SUIT_PLAT_ERR_INVAL};

	SET_RETURN_SEQ(suit_plat_decode_manifest_class_id, class_id_decode_results,
		       ARRAY_SIZE(class_id_decode_results));

	int ret =
		suit_plat_authorize_process_dependency(&parent_component_id, NULL, SUIT_SEQ_PARSE);

	/* Manifest sequence authorization fails */
	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret,
		      "Failed to catch null argument (NULL, _, PARSE)");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count,
		      ARRAY_SIZE(class_id_decode_results),
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.arg0_history[0], &parent_component_id,
		      "Invalid manifest class ID value (1st call)");
	zassert_not_equal(
		suit_plat_decode_manifest_class_id_fake.arg1_history[0], NULL,
		"The API must provide a valid pointer, to decode manifest class ID (1st call)");
	zassert_equal(suit_plat_decode_manifest_class_id_fake.arg0_history[1], NULL,
		      "Invalid manifest class ID value (2nd call)");
	zassert_not_equal(
		suit_plat_decode_manifest_class_id_fake.arg1_history[1], NULL,
		"The API must provide a valid pointer, to decode manifest class ID (2nd call)");
	zassert_equal(suit_mci_manifest_process_dependency_validate_fake.call_count, 0,
		      "Incorrect number of suit_mci_manifest_process_dependency_validate() calls");
}

ZTEST(suit_platform_authorize_process_dependency_tests, test_processing_denied)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_predefined_fake_func;
	suit_mci_manifest_process_dependency_validate_fake.return_val = MCI_ERR_NOACCESS;

	int ret = suit_plat_authorize_process_dependency(&parent_component_id, &child_component_id,
							 SUIT_SEQ_PARSE);

	/* Manifest sequence authorization fails */
	zassert_equal(SUIT_ERR_UNAUTHORIZED_COMPONENT, ret,
		      "Failed to block processing of invalid manifest hierarchy");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 2,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_process_dependency_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_process_dependency_validate() calls");
	zassert_equal(suit_mci_manifest_process_dependency_validate_fake.arg0_history[0],
		      &parent_class_id, "Invalid parent manifest class ID value");
	zassert_equal(suit_mci_manifest_process_dependency_validate_fake.arg1_history[0],
		      &child_class_id, "Invalid child manifest class ID value");
}

ZTEST(suit_platform_authorize_process_dependency_tests, test_processing_allowed)
{
	suit_plat_decode_manifest_class_id_fake.custom_fake =
		suit_plat_decode_manifest_class_id_predefined_fake_func;
	suit_mci_manifest_process_dependency_validate_fake.return_val = SUIT_PLAT_SUCCESS;

	int ret = suit_plat_authorize_process_dependency(&parent_component_id, &child_component_id,
							 SUIT_SEQ_PARSE);

	/* Manifest sequence authorization succeeds */
	zassert_equal(SUIT_SUCCESS, ret, "Failed to process valid manifest hierarchy");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_plat_decode_manifest_class_id_fake.call_count, 2,
		      "Incorrect number of suit_plat_decode_manifest_class_id() calls");
	zassert_equal(suit_mci_manifest_process_dependency_validate_fake.call_count, 1,
		      "Incorrect number of suit_mci_manifest_process_dependency_validate() calls");
	zassert_equal(suit_mci_manifest_process_dependency_validate_fake.arg0_history[0],
		      &parent_class_id, "Invalid parent manifest class ID value");
	zassert_equal(suit_mci_manifest_process_dependency_validate_fake.arg1_history[0],
		      &child_class_id, "Invalid child manifest class ID value");
}
