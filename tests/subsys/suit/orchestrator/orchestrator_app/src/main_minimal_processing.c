/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if CONFIG_TESTS_SUIT_ORCHESTRATOR_APP_MINIMAL_PROCESSING

#include <dfu/suit_dfu.h>

#include "orchestrator_app_tests_common.h"

/* Valid envelope */
extern const uint8_t manifest_valid_buf[];
extern const size_t manifest_valid_len;

#define MAX_UPDATE_REGIONS 10
suit_plat_mreg_t requested_update_regions[MAX_UPDATE_REGIONS];

int suit_trigger_custom_fake_correct_function(suit_plat_mreg_t *regions, size_t len)
{
	len = (len <= MAX_UPDATE_REGIONS) ? len : MAX_UPDATE_REGIONS;

	memcpy(requested_update_regions, regions, len * sizeof(suit_plat_mreg_t));

	return 0;
}

static void *setup_suite(void)
{
	zassert_equal(0, suit_dfu_initialize(), "SUIT DFU module not initialized correctly");

	zassert_true(device_is_ready(DFU_PARTITION_DEVICE), "Flash device not ready");

	return NULL;
}

static void setup_test(void *f)
{
	(void)f;

	ssf_fakes_reset();
	memset(requested_update_regions, 0XFF, sizeof(requested_update_regions));

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
}

ZTEST_SUITE(orchestrator_app_minimal_processing_tests, NULL, setup_suite, setup_test, NULL, NULL);

ZTEST(orchestrator_app_minimal_processing_tests, test_valid_envelope)
{
	suit_trigger_update_fake.custom_fake = suit_trigger_custom_fake_correct_function;

	fill_dfu_partition_with_data(manifest_valid_buf, manifest_valid_len);

	zassert_equal(0, suit_dfu_candidate_envelope_stored(), "Unexpected error code");
	zassert_equal(0, suit_dfu_candidate_preprocess(), "Unexpected error code");
	zassert_equal(0, suit_dfu_update_start(), "Unexpected error code");

	zassert_equal(suit_trigger_update_fake.call_count, 1,
		      "Incorrect number of suit_trigger_update() calls");
	zassert_equal(suit_trigger_update_fake.arg1_val, 1,
		      "Incorrect number of regions passed to suit_trigger_update()");

	zassert_equal(requested_update_regions[0].mem, DFU_PARTITION_ADDRESS);
	zassert_equal(requested_update_regions[0].size, manifest_valid_len);
}

ZTEST(orchestrator_app_minimal_processing_tests, test_malformed_envelope)
{
	uint8_t malformed_envelope[256] = {0};

	fill_dfu_partition_with_data(malformed_envelope, sizeof(malformed_envelope));

	zassert_not_equal(0, suit_dfu_candidate_envelope_stored(),
			  "suit_dfu_candidate_envelope_stored should have failed");
	zassert_not_equal(0, suit_dfu_update_start(), "suit_dfu_update_start should have failed");

	zassert_equal(suit_trigger_update_fake.call_count, 0,
		      "Incorrect number of suit_trigger_update() calls");
}

#endif /* CONFIG_TESTS_SUIT_ORCHESTRATOR_APP_MINIMAL_PROCESSING */
