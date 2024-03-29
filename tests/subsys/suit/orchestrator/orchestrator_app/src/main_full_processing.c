/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if CONFIG_TESTS_SUIT_ORCHESTRATOR_APP_FULL_PROCESSING

#include <dfu/suit_dfu.h>
#include <suit_plat_err.h>
#include <suit_types.h>
#include <dfu/suit_dfu_fetch_source.h>
#include <suit_dfu_cache_rw.h>
#include <suit_dfu_cache.h>
#include <suit_metadata.h>
#include <zephyr/sys/util.h>

#include "orchestrator_app_tests_common.h"

#define CACHE_PARTITION_LABEL(N)   dfu_cache_partition_##N
#define CACHE_PARTITION_OFFSET(N)  FIXED_PARTITION_OFFSET(CACHE_PARTITION_LABEL(N))
#define CACHE_PARTITION_ADDRESS(N) suit_plat_mem_nvm_ptr_get(CACHE_PARTITION_OFFSET(N))
#define CACHE_PARTITION_SIZE(N)	   FIXED_PARTITION_SIZE(CACHE_PARTITION_LABEL(N))
#define CACHE_PARTITION_DEVICE(N)  FIXED_PARTITION_DEVICE(CACHE_PARTITION_LABEL(N))

/* valid envelope */
extern const uint8_t manifest_valid_buf[];
extern const size_t manifest_valid_len;

/* Envelope with unknown component type. */
extern const uint8_t manifest_unsupported_component_buf[];
extern const size_t manifest_unsupported_component_len;

/* Valid root envelope for testing dependency-resolution + payload-fetch sequences. */
extern const uint8_t manifest_valid_payload_fetch_buf[];
extern const size_t manifest_valid_payload_fetch_len;

/* Valid app envelope for testing payload-fetch sequences */
extern const uint8_t manifest_valid_payload_fetch_app_buf[];
extern const size_t manifest_valid_payload_fetch_app_len;

static const uint8_t payload1[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
				     0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11};
static const uint8_t payload2[30] = {0x1c, 0xf2, 0x76, 0xbd, 0x54, 0xc0, 0xb6, 0x82, 0xaf, 0xeb,
				     0xe9, 0x89, 0xb4, 0x5f, 0x96, 0xde, 0x0f, 0x81, 0x6a, 0xd7,
				     0xf6, 0x04, 0xcb, 0xe4, 0x94, 0xef, 0xa4, 0x3b, 0xf0, 0x4f};
static const uint8_t payload3[10] = {0xfa, 0xcd, 0x2d, 0x86, 0x5b, 0x6c, 0xf3, 0xea, 0x7a, 0xa9};

/* Mocks of functions provided via SSF services */
FAKE_VALUE_FUNC(int, suit_plat_component_compatibility_check, suit_manifest_class_id_t *,
		struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_authorize_sequence_num, enum suit_command_sequence,
		struct zcbor_string *, unsigned int);
FAKE_VALUE_FUNC(int, suit_plat_authorize_unsigned_manifest, struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_plat_authenticate_manifest, struct zcbor_string *, enum suit_cose_alg,
		struct zcbor_string *, struct zcbor_string *, struct zcbor_string *);

FAKE_VALUE_FUNC(suit_ssf_err_t, suit_check_installed_component_digest, suit_plat_mreg_t *, int,
		suit_plat_mreg_t *);
FAKE_VALUE_FUNC(suit_ssf_err_t, suit_get_supported_manifest_roles, suit_manifest_role_t *,
		size_t *);
FAKE_VALUE_FUNC(suit_ssf_err_t, suit_get_supported_manifest_info, suit_manifest_role_t,
		suit_ssf_manifest_class_info_t *);

static const suit_manifest_role_t supported_roles[] = {SUIT_MANIFEST_APP_ROOT,
						       SUIT_MANIFEST_APP_LOCAL_1};

/* RFC4122 uuid5(uuid.NAMESPACE_DNS, 'nordicsemi.com') */
static const suit_uuid_t nordic_vendor_id = {{0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f,
					      0x94, 0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4}};

/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_root') */
static const suit_uuid_t sample_root_class_id = {{0x3f, 0x6a, 0x3a, 0x4d, 0xcd, 0xfa, 0x58, 0xc5,
						  0xac, 0xce, 0xf9, 0xf5, 0x84, 0xc4, 0x11, 0x24}};

/* RFC4122 uuid5(nordic_vid, 'nRF54H20_sample_app') */
static const suit_uuid_t sample_app_class_id = {{0x08, 0xc1, 0xb5, 0x99, 0x55, 0xe8, 0x5f, 0xbc,
						 0x9e, 0x76, 0x7b, 0xc2, 0x9c, 0xe1, 0xb0, 0x4d}};

#define MAX_UPDATE_REGIONS 10
suit_plat_mreg_t requested_update_regions[MAX_UPDATE_REGIONS];

static int fetch_source_fn(const uint8_t *uri, size_t uri_length, uint32_t session_id)
{
	const uint8_t *data = NULL;
	size_t len = 0;

	zassert_not_null(uri);

	if (memcmp(uri, "app.suit", MIN(sizeof("app.suit"), uri_length)) == 0) {
		/* Fetch application envelope */
		data = manifest_valid_payload_fetch_app_buf;
		len = manifest_valid_payload_fetch_app_len;
	} else if (memcmp(uri, "payload1.bin", MIN(sizeof("payload1.bin"), uri_length)) == 0) {
		data = payload1;
		len = sizeof(payload1);
	} else if (memcmp(uri, "payload2.bin", MIN(sizeof("payload2.bin"), uri_length)) == 0) {
		data = payload2;
		len = sizeof(payload2);
	} else if (memcmp(uri, "payload3.bin", MIN(sizeof("payload3.bin"), uri_length)) == 0) {
		data = payload3;
		len = sizeof(payload3);
	} else {
		return -1;
	}

	return suit_dfu_fetch_source_write_fetched_data(session_id, data, len);
}

suit_ssf_err_t suit_get_supported_manifest_roles_custom_fake(suit_manifest_role_t *roles,
							     size_t *size)
{
	zassert_not_null(roles, "get supported roles - NULL roles argument");
	zassert_not_null(size, "get supported roles - NULL size argument");
	zassert_true(*size >= ARRAY_SIZE(supported_roles),
		     "Not enough space for storing supported roles");

	memcpy(roles, supported_roles, sizeof(supported_roles));
	*size = 2;

	return SUIT_PLAT_SUCCESS;
}

suit_ssf_err_t
suit_get_supported_manifest_info_custom_fake(suit_manifest_role_t role,
					     suit_ssf_manifest_class_info_t *class_info)
{
	zassert_not_null(class_info, "get supported manifest info - NULL info argument");

	if (role == SUIT_MANIFEST_APP_ROOT) {
		memcpy(&class_info->class_id, &sample_root_class_id, sizeof(sample_root_class_id));
	} else if (role == SUIT_MANIFEST_APP_LOCAL_1) {
		memcpy(&class_info->class_id, &sample_app_class_id, sizeof(sample_app_class_id));
	} else {
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	memcpy(&class_info->vendor_id, &nordic_vendor_id, sizeof(nordic_vendor_id));
	class_info->role = role;

	return SUIT_PLAT_SUCCESS;
}

int suit_update_trigger_custom_fake_correct_function(suit_plat_mreg_t *regions, size_t len)
{
	len = (len <= MAX_UPDATE_REGIONS) ? len : MAX_UPDATE_REGIONS;

	memcpy(requested_update_regions, regions, len * sizeof(suit_plat_mreg_t));

	return 0;
}

static void *setup_suite(void)
{
	zassert_equal(0, suit_dfu_initialize(), "SUIT DFU module not initialized correctly");

	zassert_true(device_is_ready(DFU_PARTITION_DEVICE), "Flash device not ready");

	zassert_equal(SUIT_PLAT_SUCCESS, suit_dfu_fetch_source_register(fetch_source_fn),
		      "Failed to register fetch source");

	return NULL;
}

static void cleanup_all_and_verify_empty(void)
{
	/* This function should perform all needed cleanup  */
	/* Note - this will clear part of the flash before each test and cause possible
	 * quick wearing off of the flash memory.
	 */
	zassert_equal(0, suit_dfu_cleanup(), "SUIT DFU cleanup failed");

	uint8_t *partition_address = (uint8_t *)DFU_PARTITION_ADDRESS;
	bool partition_empty = true;

	for (size_t i = 0; i < DFU_PARTITION_SIZE; i++) {
		if (partition_address[i] != 0xFF) {
			partition_empty = false;
			break;
		}
	}

	zassert_true(partition_empty, "DFU partition is not empty after cleanup");

	/* Check if cache partitions have been cleared */
	partition_address = (uint8_t *)CACHE_PARTITION_ADDRESS(1);
	partition_empty = true;
	for (size_t i = 0; i < CACHE_PARTITION_SIZE(1); i++) {
		if (partition_address[i] != 0xFF) {
			partition_empty = false;
			break;
		}
	}
	zassert_true(partition_empty, "Cache partition 1 is not empty after cleanup");

	/* Check if cache partitions have been cleared */
	partition_address = (uint8_t *)CACHE_PARTITION_ADDRESS(3);
	partition_empty = true;
	for (size_t i = 0; i < CACHE_PARTITION_SIZE(3); i++) {
		if (partition_address[i] != 0xFF) {
			partition_empty = false;
			break;
		}
	}
	zassert_true(partition_empty, "Cache partition 3 is not empty after cleanup");
}

static void setup_test(void *f)
{
	(void)f;

	ssf_fakes_reset();
	RESET_FAKE(suit_get_supported_manifest_roles);
	RESET_FAKE(suit_get_supported_manifest_info);

	memset(requested_update_regions, 0XFF, sizeof(requested_update_regions));

	cleanup_all_and_verify_empty();
}

ZTEST_SUITE(orchestrator_app_full_processing_tests, NULL, setup_suite, setup_test, NULL, NULL);

ZTEST(orchestrator_app_full_processing_tests, test_valid_envelope)
{
	suit_trigger_update_fake.custom_fake = suit_update_trigger_custom_fake_correct_function;

	fill_dfu_partition_with_data(manifest_valid_buf, manifest_valid_len);

	zassert_equal(0, suit_dfu_candidate_envelope_stored(), "Unexpected error code");
	zassert_equal(0, suit_dfu_candidate_preprocess(), "Unexpected error code");
	zassert_equal(0, suit_dfu_update_start(), "Unexpected error code");

	zassert_equal(suit_trigger_update_fake.call_count, 1,
		      "Incorrect number of suit_trigger_update() calls");
	/* Regions count argument - value of 4 because we have:
	 * - update candidate (always present)
	 * - cache pool 0 (always present if the dfu cache module is enabled)
	 * - cache pools 1 and 3 - defined in the devicetree overlay
	 */
	zassert_equal(suit_trigger_update_fake.arg1_val, 4,
		      "Incorrect number of regions passed to suit_trigger_update()");

	zassert_equal(requested_update_regions[0].mem, DFU_PARTITION_ADDRESS);
	zassert_equal(requested_update_regions[0].size, manifest_valid_len);
}

ZTEST(orchestrator_app_full_processing_tests, test_unsupported_component)
{
	fill_dfu_partition_with_data(manifest_unsupported_component_buf,
				     manifest_unsupported_component_len);

	zassert_equal(0, suit_dfu_candidate_envelope_stored(), "Unexpected error code");
	zassert_not_equal(0, suit_dfu_candidate_preprocess(),
			  "Processing envelope should have failed");
}

static void check_cache_slot_content(uint8_t cache_partition_id, const uint8_t *uri, size_t uri_len,
				     const uint8_t *expected_data, size_t expected_size)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	const uint8_t *partition_address;
	size_t partition_size;
	const uint8_t *result_data;
	size_t result_len;

	ret = suit_dfu_cache_rw_partition_info_get(cache_partition_id, &partition_address,
						   &partition_size);
	zassert_equal(SUIT_PLAT_SUCCESS, ret, "Getting cache partition %d info failed",
		      cache_partition_id);

	/* Find the given slot in cache */
	ret = suit_dfu_cache_search(uri, uri_len, &result_data, &result_len);
	zassert_equal(SUIT_PLAT_SUCCESS, ret, "Finding slot for URI in cache failed!");

	zassert_true(result_data >= partition_address &&
			     (result_data + result_len) < (partition_address + partition_size),
		     "Slot for the URI is not in partition %d", cache_partition_id);

	zassert_equal(result_len, expected_size, "Incorrect cache slot size");
	zassert_mem_equal(result_data, expected_data, expected_size,
			  "Incorrect cache slot contents");
}

ZTEST(orchestrator_app_full_processing_tests, test_valid_envelope_payload_fetch)
{
	suit_trigger_update_fake.custom_fake = suit_update_trigger_custom_fake_correct_function;
	suit_get_supported_manifest_roles_fake.custom_fake =
		suit_get_supported_manifest_roles_custom_fake;
	suit_get_supported_manifest_info_fake.custom_fake =
		suit_get_supported_manifest_info_custom_fake;

	fill_dfu_partition_with_data(manifest_valid_payload_fetch_buf,
				     manifest_valid_payload_fetch_len);

	zassert_equal(0, suit_dfu_candidate_envelope_stored(), "Unexpected error code");
	zassert_equal(0, suit_dfu_candidate_preprocess(), "Unexpected error code");
	zassert_equal(0, suit_dfu_update_start(), "Unexpected error code");

	zassert_equal(suit_trigger_update_fake.call_count, 1,
		      "Incorrect number of suit_trigger_update() calls");
	/* Regions count argument - value of 4 because we have:
	 * - update candidate (always present)
	 * - cache pool 0 (always present if the dfu cache module is enabled)
	 * - cache pools 1 and 3 - defined in the devicetree overlay
	 */
	zassert_equal(suit_trigger_update_fake.arg1_val, 4,
		      "Incorrect number of regions passed to suit_trigger_update()");

	zassert_equal(requested_update_regions[0].mem, DFU_PARTITION_ADDRESS);
	zassert_equal(requested_update_regions[0].size, manifest_valid_payload_fetch_len);

	check_cache_slot_content(0, "payload1.bin", sizeof("payload1.bin"), payload1,
				 sizeof(payload1));
	check_cache_slot_content(1, "payload2.bin", sizeof("payload2.bin"), payload2,
				 sizeof(payload2));
	check_cache_slot_content(3, "payload3.bin", sizeof("payload3.bin"), payload3,
				 sizeof(payload3));
	cleanup_all_and_verify_empty();
}

#endif /* CONFIG_TESTS_SUIT_ORCHESTRATOR_APP_FULL_PROCESSING */
