/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <mocks.h>
#include <suit_platform.h>

static suit_manifest_class_id_t sample_class_id = {{0xca, 0xd8, 0x52, 0x3a, 0xf8, 0x29, 0x5a, 0x9a,
						    0xba, 0x85, 0x2e, 0xa0, 0xb2, 0xf5, 0x77,
						    0xc9}};

static const suit_uuid_t nordic_vendor_id = {{0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f,
					      0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4}};

static intptr_t valid_component_handle = 0x1234;
static struct zcbor_string valid_component_id = {.value = (const uint8_t *)0x1234, .len = 123};
static struct zcbor_string valid_cid_uuid = {.value = (const uint8_t *)0x5678, .len = 16};
static struct zcbor_string valid_vid_uuid = {.value = (const uint8_t *)0x9012, .len = 16};
static struct zcbor_string valid_did_uuid = {.value = (const uint8_t *)0x3456, .len = 16};

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
	zassert_equal(valid_component_handle, handle, "Invalid component handle value");
	zassert_not_null(component_id, "The API must provide a valid pointer");

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

static int suit_plat_component_id_get_correct_fake_func(suit_component_t handle,
							struct zcbor_string **component_id)
{
	zassert_equal(valid_component_handle, handle, "Invalid component handle value");
	zassert_not_null(component_id, "The API must provide a valid pointer");

	*component_id = &valid_component_id;

	return SUIT_SUCCESS;
}

int suit_mci_supported_manifest_class_ids_get_correct_fake_func(
	suit_manifest_class_info_t *class_info, size_t *size)
{
	zassert_not_null(class_info, "The API must provide a valid class_id pointer");
	zassert_not_null(size, "The API must provide a valid size pointer");
	zassert_not_equal(*size, 0, "Invalid size value. Must be > 0");

	*size = 1;
	class_info[0].class_id = &sample_class_id;
	class_info[0].vendor_id = &nordic_vendor_id;

	return SUIT_PLAT_SUCCESS;
}

int suit_mci_supported_manifest_class_ids_get_invalid_fake_func(
	suit_manifest_class_info_t *class_info, size_t *size)
{
	zassert_not_null(class_info, "The API must provide a valid class_id pointer");
	zassert_not_null(size, "The API must provide a valid size pointer");
	zassert_not_equal(*size, 0, "Invalid size value. Must be > 0");

	return SUIT_PLAT_ERR_SIZE;
}

static int
suit_plat_component_compatibility_check_invalid_fake_func(const suit_manifest_class_id_t *class_id,
							  struct zcbor_string *component_id)
{
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	zassert_equal(&valid_component_id, component_id, "Invalid manifest component ID value");

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

static int
suit_plat_component_compatibility_check_correct_fake_func(const suit_manifest_class_id_t *class_id,
							  struct zcbor_string *component_id)
{
	zassert_not_equal(class_id, NULL,
			  "The API must provide a valid pointer, to decode manifest class ID");
	zassert_equal(&valid_component_id, component_id, "Invalid manifest component ID value");

	return SUIT_SUCCESS;
}

int suit_metadata_uuid_compare_invalid_fake_func(const suit_uuid_t *uuid1, const suit_uuid_t *uuid2)
{
	zassert_not_null(uuid1, "The API must provide a valid uuid1 pointer");
	zassert_not_null(uuid2, "The API must provide a valid uuid2 pointer");

	return MCI_ERR_MANIFESTCLASSID;
}

int suit_metadata_uuid_compare_correct_fake_func(const suit_uuid_t *uuid1, const suit_uuid_t *uuid2)
{
	zassert_not_null(uuid1, "The API must provide a valid uuid1 pointer");
	zassert_not_null(uuid2, "The API must provide a valid uuid2 pointer");

	return SUIT_PLAT_SUCCESS;
}

ZTEST_SUITE(suit_plat_class_check_cid_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_plat_class_check_cid_tests, test_null_cid_uuid)
{
	int ret = suit_plat_check_cid(valid_component_handle, NULL);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_cid_tests, test_null_cid_value_uuid)
{
	static struct zcbor_string invalid_cid_uuid = {.value = NULL, .len = 123};

	int ret = suit_plat_check_cid(valid_component_handle, &invalid_cid_uuid);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_cid_tests, test_cid_len_0_uuid)
{
	static struct zcbor_string invalid_cid_uuid = {.value = (const uint8_t *)0x5678, .len = 0};

	int ret = suit_plat_check_cid(valid_component_handle, &invalid_cid_uuid);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_cid_tests, test_invalid_suit_mci_supported_manifest_class_ids_get)
{
	suit_mci_supported_manifest_class_ids_get_fake.custom_fake =
		suit_mci_supported_manifest_class_ids_get_invalid_fake_func;

	int ret = suit_plat_check_cid(valid_component_handle, &valid_cid_uuid);

	zassert_equal(SUIT_ERR_CRASH, ret, "Check should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_cid_tests, test_invalid_suit_plat_component_id_get)
{
	suit_mci_supported_manifest_class_ids_get_fake.custom_fake =
		suit_mci_supported_manifest_class_ids_get_correct_fake_func;
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_invalid_fake_func;

	int ret = suit_plat_check_cid(valid_component_handle, &valid_cid_uuid);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Check should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_cid_tests, test_invalid_suit_plat_component_compatibility_check)
{
	suit_mci_supported_manifest_class_ids_get_fake.custom_fake =
		suit_mci_supported_manifest_class_ids_get_correct_fake_func;
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_component_compatibility_check_fake.custom_fake =
		suit_plat_component_compatibility_check_invalid_fake_func;

	int ret = suit_plat_check_cid(valid_component_handle, &valid_cid_uuid);

	zassert_equal(SUIT_FAIL_CONDITION, ret, "Check should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_cid_tests, test_invalid_suit_metadata_uuid_compare)
{
	suit_mci_supported_manifest_class_ids_get_fake.custom_fake =
		suit_mci_supported_manifest_class_ids_get_correct_fake_func;
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_component_compatibility_check_fake.custom_fake =
		suit_plat_component_compatibility_check_correct_fake_func;
	suit_metadata_uuid_compare_fake.custom_fake = suit_metadata_uuid_compare_invalid_fake_func;

	int ret = suit_plat_check_cid(valid_component_handle, &valid_cid_uuid);

	zassert_equal(SUIT_FAIL_CONDITION, ret, "Check should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 1,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_cid_tests, test_OK)
{
	suit_mci_supported_manifest_class_ids_get_fake.custom_fake =
		suit_mci_supported_manifest_class_ids_get_correct_fake_func;
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_component_compatibility_check_fake.custom_fake =
		suit_plat_component_compatibility_check_correct_fake_func;
	suit_metadata_uuid_compare_fake.custom_fake = suit_metadata_uuid_compare_correct_fake_func;

	int ret = suit_plat_check_cid(valid_component_handle, &valid_cid_uuid);

	zassert_equal(SUIT_SUCCESS, ret, "Check should have succeeded");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 1,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST_SUITE(suit_plat_class_check_vid_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_plat_class_check_vid_tests, test_null_vid_uuid)
{
	int ret = suit_plat_check_vid(valid_component_handle, NULL);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_vid_tests, test_null_vid_value_uuid)
{
	static struct zcbor_string invalid_vid_uuid = {.value = NULL, .len = 123};

	int ret = suit_plat_check_vid(valid_component_handle, &invalid_vid_uuid);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_vid_tests, test_vid_len_0_uuid)
{
	static struct zcbor_string invalid_vid_uuid = {.value = (const uint8_t *)0x5678, .len = 0};

	int ret = suit_plat_check_vid(valid_component_handle, &invalid_vid_uuid);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 0,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_vid_tests, test_invalid_suit_mci_supported_manifest_class_ids_get)
{
	suit_mci_supported_manifest_class_ids_get_fake.custom_fake =
		suit_mci_supported_manifest_class_ids_get_invalid_fake_func;

	int ret = suit_plat_check_vid(valid_component_handle, &valid_vid_uuid);

	zassert_equal(SUIT_ERR_CRASH, ret, "Check should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_vid_tests, test_invalid_suit_plat_component_id_get)
{
	suit_mci_supported_manifest_class_ids_get_fake.custom_fake =
		suit_mci_supported_manifest_class_ids_get_correct_fake_func;
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_invalid_fake_func;

	int ret = suit_plat_check_vid(valid_component_handle, &valid_vid_uuid);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Check should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 0,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_vid_tests, test_invalid_suit_plat_component_compatibility_check)
{
	suit_mci_supported_manifest_class_ids_get_fake.custom_fake =
		suit_mci_supported_manifest_class_ids_get_correct_fake_func;
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_component_compatibility_check_fake.custom_fake =
		suit_plat_component_compatibility_check_invalid_fake_func;

	int ret = suit_plat_check_vid(valid_component_handle, &valid_vid_uuid);

	zassert_equal(SUIT_FAIL_CONDITION, ret, "Check should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 0,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_vid_tests, test_invalid_suit_metadata_uuid_compare)
{
	suit_mci_supported_manifest_class_ids_get_fake.custom_fake =
		suit_mci_supported_manifest_class_ids_get_correct_fake_func;
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_component_compatibility_check_fake.custom_fake =
		suit_plat_component_compatibility_check_correct_fake_func;
	suit_metadata_uuid_compare_fake.custom_fake = suit_metadata_uuid_compare_invalid_fake_func;

	int ret = suit_plat_check_vid(valid_component_handle, &valid_vid_uuid);

	zassert_equal(SUIT_FAIL_CONDITION, ret, "Check should have failed");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 1,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST(suit_plat_class_check_vid_tests, test_OK)
{
	suit_mci_supported_manifest_class_ids_get_fake.custom_fake =
		suit_mci_supported_manifest_class_ids_get_correct_fake_func;
	suit_plat_component_id_get_fake.custom_fake = suit_plat_component_id_get_correct_fake_func;
	suit_plat_component_compatibility_check_fake.custom_fake =
		suit_plat_component_compatibility_check_correct_fake_func;
	suit_metadata_uuid_compare_fake.custom_fake = suit_metadata_uuid_compare_correct_fake_func;

	int ret = suit_plat_check_vid(valid_component_handle, &valid_vid_uuid);

	zassert_equal(SUIT_SUCCESS, ret, "Check should have succeeded");

	/* Check expected call counts for fake functions */
	zassert_equal(suit_mci_supported_manifest_class_ids_get_fake.call_count, 1,
		      "Incorrect number of suit_mci_supported_manifest_class_ids_get() calls");
	zassert_equal(suit_plat_component_id_get_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_id_get() calls");
	zassert_equal(suit_plat_component_compatibility_check_fake.call_count, 1,
		      "Incorrect number of suit_plat_component_compatibility_check() calls");
	zassert_equal(suit_metadata_uuid_compare_fake.call_count, 1,
		      "Incorrect number of suit_metadata_uuid_compare() calls");
}

ZTEST_SUITE(suit_plat_class_check_did_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_plat_class_check_did_tests, test_null_did_uuid)
{
	int ret = suit_plat_check_did(valid_component_handle, NULL);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");
}

ZTEST(suit_plat_class_check_did_tests, test_null_did_value_uuid)
{
	static struct zcbor_string invalid_did_uuid = {.value = NULL, .len = 123};

	int ret = suit_plat_check_did(valid_component_handle, &invalid_did_uuid);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");
}

ZTEST(suit_plat_class_check_did_tests, test_did_len_0_uuid)
{
	static struct zcbor_string invalid_did_uuid = {.value = (const uint8_t *)0x5678, .len = 0};

	int ret = suit_plat_check_did(valid_component_handle, &invalid_did_uuid);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMPONENT_ID, ret, "Failed to catch null argument");
}

ZTEST(suit_plat_class_check_did_tests, test_did_valid_did)
{
	int ret = suit_plat_check_did(valid_component_handle, &valid_did_uuid);

	zassert_equal(SUIT_ERR_UNSUPPORTED_COMMAND, ret, "Failed to catch null argument");
}
