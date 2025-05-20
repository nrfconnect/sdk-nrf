/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef CONFIG_TESTS_SUIT_DFU_TARGET_SUIT_SINGLE_PARTITION

#include <dfu/dfu_target.h>
#include <dfu/dfu_target_suit.h>

#include "common.h"

/* The SUIT envelopes are defined inside the respective manfest_*.c files. */
extern uint8_t manifest_with_payload[];
extern const size_t manifest_with_payload_len;

#define TEST_IMAGE_SIZE		 1024
#define TEST_IMAGE_ENVELOPE_NUM	 0
#define TEST_WRONG_IMAGE_CACHE_0 1
#define TEST_WRONG_IMAGE_CACHE_1 2

#define DFU_PARTITION_LABEL   dfu_partition
#define DFU_PARTITION_OFFSET  FIXED_PARTITION_OFFSET(DFU_PARTITION_LABEL)
#define DFU_PARTITION_ADDRESS suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET)
#define DFU_PARTITION_SIZE    FIXED_PARTITION_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_DEVICE  FIXED_PARTITION_DEVICE(DFU_PARTITION_LABEL)

static uint8_t dfu_target_buffer[TEST_IMAGE_SIZE];

static void setup_test(void *f)
{
	(void)f;

	reset_fakes();
}

ZTEST(dfu_target_suit, test_image_init)
{
	int rc;

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);
	zassert_equal(is_partition_empty(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE), true,
		      "Partition should be empty after calling dfu_target_reset");

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_ENVELOPE_NUM, TEST_IMAGE_SIZE,
			     NULL);
	zassert_equal(rc, -ENODEV,
		      "dfu_target should fail because the buffer has not been initialized. %d", rc);

	rc = dfu_target_suit_set_buf(dfu_target_buffer, sizeof(dfu_target_buffer));
	zassert_equal(rc, 0, "dfu_target should initialize the data buffer %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_ENVELOPE_NUM, TEST_IMAGE_SIZE,
			     NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_WRONG_IMAGE_CACHE_0, TEST_IMAGE_SIZE,
			     NULL);
	zassert_equal(rc, -ENOTSUP,
		      "dfu_target should fail because CACHE PROCESSING is not enabled: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_WRONG_IMAGE_CACHE_1, TEST_IMAGE_SIZE,
			     NULL);
	zassert_equal(rc, -ENOTSUP,
		      "dfu_target should fail because CACHE PROCESSING is not enabled: %d", rc);
}

ZTEST(dfu_target_suit, test_upload)
{
	int rc;
	size_t offset = 0;

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_set_buf(dfu_target_buffer, sizeof(dfu_target_buffer));
	zassert_equal(rc, 0, "dfu_target should initialize the data buffer %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_ENVELOPE_NUM,
			     TEST_IMAGE_SIZE * 1000, NULL);
	zassert_equal(rc, -EFBIG, "dfu_target should not allow too big files. %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_ENVELOPE_NUM, TEST_IMAGE_SIZE,
			     NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_write(manifest_with_payload, manifest_with_payload_len);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_offset_get(&offset);
	zassert_equal(rc, 0, "DFU target offset get should pass for image 0 %d", rc);
	zassert_equal(offset, manifest_with_payload_len,
		      "DFU target offset should be equal to image size");

	rc = dfu_target_done(true);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_schedule_update(0);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	dfu_target_suit_reboot();
	zassert_equal(suit_trigger_update_fake.call_count, 1,
		      "Incorrect number of suit_trigger_update() calls");

	rc = memcmp((uint8_t *)DFU_PARTITION_ADDRESS, manifest_with_payload,
		    manifest_with_payload_len);
	zassert_equal(rc, 0, "DFU partition content does not match the manifest %d", rc);

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);
	zassert_equal(is_partition_empty(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE), true,
		      "Partition should be empty after calling dfu_target_reset");
}

ZTEST_SUITE(dfu_target_suit, NULL, NULL, setup_test, NULL, NULL);

#endif /* CONFIG_TESTS_SUIT_DFU_TARGET_SUIT_SINGLE_PARTITION */
