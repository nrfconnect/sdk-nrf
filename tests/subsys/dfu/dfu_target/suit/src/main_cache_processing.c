/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef CONFIG_TESTS_SUIT_DFU_TARGET_SUIT_CACHE_PROCESSING

#include <dfu/dfu_target.h>
#include <dfu/dfu_target_suit.h>

#include "common.h"

/* The SUIT envelopes are defined inside the respective manfest_*.c files. */
extern uint8_t manifest[];
extern const size_t manifest_len;
extern uint8_t dfu_cache_partition[];
extern const size_t dfu_cache_partition_len;
extern uint8_t manifest_with_payload[];
extern const size_t manifest_with_payload_len;

#define TEST_IMAGE_SIZE		1024
#define TEST_IMAGE_ENVELOPE_NUM 0
#define TEST_IMAGE_CACHE_1_NUM	2
#define TEST_IMAGE_CACHE_2_NUM	3
#define TEST_IMAGE_CACHE_3_NUM	4

#define DFU_PARTITION_LABEL   dfu_partition
#define DFU_PARTITION_OFFSET  FIXED_PARTITION_OFFSET(DFU_PARTITION_LABEL)
#define DFU_PARTITION_ADDRESS suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET)
#define DFU_PARTITION_SIZE    FIXED_PARTITION_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_DEVICE  FIXED_PARTITION_DEVICE(DFU_PARTITION_LABEL)

#define CACHE_PARTITION_LABEL(N)   dfu_cache_partition_##N
#define CACHE_PARTITION_OFFSET(N)  FIXED_PARTITION_OFFSET(CACHE_PARTITION_LABEL(N))
#define CACHE_PARTITION_ADDRESS(N) suit_plat_mem_nvm_ptr_get(CACHE_PARTITION_OFFSET(N))
#define CACHE_PARTITION_SIZE(N)	   FIXED_PARTITION_SIZE(CACHE_PARTITION_LABEL(N))
#define CACHE_PARTITION_DEVICE(N)  FIXED_PARTITION_DEVICE(CACHE_PARTITION_LABEL(N))

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
	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(1), CACHE_PARTITION_SIZE(1)), true,
		      "Partition should be empty after calling dfu_target_reset");
	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(2), CACHE_PARTITION_SIZE(2)), true,
		      "Partition should be empty after calling dfu_target_reset");
	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(3), CACHE_PARTITION_SIZE(3)), true,
		      "Partition should be empty after calling dfu_target_reset");

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_ENVELOPE_NUM, TEST_IMAGE_SIZE,
			     NULL);
	zassert_equal(rc, -ENODEV,
		      "dfu_target should fail because the buffer has not been initialized. %d", rc);

	rc = dfu_target_suit_set_buf(dfu_target_buffer, sizeof(dfu_target_buffer));
	zassert_equal(rc, 0, "dfu_target should initialize the data buffer %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_ENVELOPE_NUM,
			     TEST_IMAGE_SIZE * 1000, NULL);
	zassert_equal(rc, -EFBIG, "dfu_target should not allow too big files. %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_ENVELOPE_NUM, TEST_IMAGE_SIZE,
			     NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_ENVELOPE_NUM, TEST_IMAGE_SIZE,
			     NULL);
	zassert_equal(rc, -EBUSY,
		      "dfu_target should not initialize the same image twice until the stream is "
		      "not ended or reset. %d",
		      rc);

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_CACHE_1_NUM, TEST_IMAGE_SIZE,
			     NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);
}

ZTEST(dfu_target_suit, test_image_upload)
{
	int rc;
	size_t offset = 0;

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_write(manifest, TEST_IMAGE_SIZE);
	zassert_equal(rc, -EFAULT, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_offset_get(&offset);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);
	zassert_equal(offset, 0, "DFU target offset should be equal to 0");

	rc = dfu_target_suit_set_buf(dfu_target_buffer, sizeof(dfu_target_buffer));
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_ENVELOPE_NUM, manifest_len,
			     NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_write(manifest, manifest_len);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_offset_get(&offset);
	zassert_equal(rc, 0, "DFU target offset get should pass for image 0 %d", rc);
	zassert_equal(offset, manifest_len, "DFU target offset should be equal to image size");

	rc = dfu_target_done(true);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_CACHE_1_NUM,
			     dfu_cache_partition_len, NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_write(dfu_cache_partition, dfu_cache_partition_len);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_offset_get(&offset);
	zassert_equal(rc, 0, "DFU target offset get should pass for image 0 %d", rc);
	zassert_equal(offset, dfu_cache_partition_len,
		      "DFU target offset should be equal to image size");

	rc = dfu_target_done(true);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_schedule_update(0);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	dfu_target_suit_reboot();
	zassert_equal(suit_trigger_update_fake.call_count, 1,
		      "Incorrect number of suit_dfu_update_start() calls");

	rc = memcmp((uint8_t *)DFU_PARTITION_ADDRESS, manifest, manifest_len);
	zassert_equal(rc, 0, "DFU partition content does not match the manifest %d", rc);

	rc = memcmp((uint8_t *)CACHE_PARTITION_ADDRESS(1), dfu_cache_partition,
		    dfu_cache_partition_len);
	zassert_equal(rc, 0, "DFU cache partition content does not match the file content %d", rc);

	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(2), CACHE_PARTITION_SIZE(2)), true,
		      "Partition should be empty after because it was not used");
	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(3), CACHE_PARTITION_SIZE(3)), true,
		      "Partition should be empty after because it was not used");

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	zassert_equal(is_partition_empty(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE), true,
		      "Partition should be empty after calling dfu_target_reset");
	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(1), CACHE_PARTITION_SIZE(1)), true,
		      "Partition should be empty after calling dfu_target_reset");
	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(2), CACHE_PARTITION_SIZE(2)), true,
		      "Partition should be empty after calling dfu_target_reset");
	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(3), CACHE_PARTITION_SIZE(3)), true,
		      "Partition should be empty after calling dfu_target_reset");
}

ZTEST(dfu_target_suit, test_image_multi_cache_upload)
{
	int rc;
	size_t offset = 0;

	RESET_FAKE(suit_trigger_update);

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_set_buf(dfu_target_buffer, sizeof(dfu_target_buffer));
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_ENVELOPE_NUM, manifest_len,
			     NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_write(manifest, manifest_len);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_offset_get(&offset);
	zassert_equal(rc, 0, "DFU target offset get should pass for image 0 %d", rc);
	zassert_equal(offset, manifest_len, "DFU target offset should be equal to image size");

	rc = dfu_target_done(true);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_CACHE_1_NUM,
			     dfu_cache_partition_len, NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_write(dfu_cache_partition, dfu_cache_partition_len);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_offset_get(&offset);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);
	zassert_equal(offset, dfu_cache_partition_len,
		      "DFU target offset should be equal to image size");

	rc = dfu_target_done(true);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_CACHE_2_NUM,
			     dfu_cache_partition_len, NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_write(dfu_cache_partition, dfu_cache_partition_len);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_offset_get(&offset);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);
	zassert_equal(offset, dfu_cache_partition_len,
		      "DFU target offset should be equal to image size");

	rc = dfu_target_done(true);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, TEST_IMAGE_CACHE_3_NUM,
			     dfu_cache_partition_len, NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_write(dfu_cache_partition, dfu_cache_partition_len);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_offset_get(&offset);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);
	zassert_equal(offset, dfu_cache_partition_len,
		      "DFU target offset should be equal to image size");

	rc = dfu_target_done(true);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_schedule_update(0);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	dfu_target_suit_reboot();
	zassert_equal(suit_trigger_update_fake.call_count, 1,
		      "Incorrect number of suit_dfu_update_start() calls");

	rc = memcmp((uint8_t *)DFU_PARTITION_ADDRESS, manifest, manifest_len);
	zassert_equal(rc, 0, "DFU partition content does not match the manifest %d", rc);

	rc = memcmp((uint8_t *)CACHE_PARTITION_ADDRESS(1), dfu_cache_partition,
		    dfu_cache_partition_len);
	zassert_equal(rc, 0, "DFU cache partition content does not match the file content %d", rc);

	rc = memcmp((uint8_t *)CACHE_PARTITION_ADDRESS(2), dfu_cache_partition,
		    dfu_cache_partition_len);
	zassert_equal(rc, 0, "DFU cache partition content does not match the file content %d", rc);

	rc = memcmp((uint8_t *)CACHE_PARTITION_ADDRESS(3), dfu_cache_partition,
		    dfu_cache_partition_len);
	zassert_equal(rc, 0, "DFU cache partition content does not match the file content %d", rc);

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	zassert_equal(is_partition_empty(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE), true,
		      "Partition should be empty after calling dfu_target_reset");
	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(1), CACHE_PARTITION_SIZE(1)), true,
		      "Partition should be empty after calling dfu_target_reset");
	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(2), CACHE_PARTITION_SIZE(2)), true,
		      "Partition should be empty after calling dfu_target_reset");
	zassert_equal(is_partition_empty(CACHE_PARTITION_ADDRESS(3), CACHE_PARTITION_SIZE(3)), true,
		      "Partition should be empty after calling dfu_target_reset");
}

ZTEST_SUITE(dfu_target_suit, NULL, NULL, setup_test, NULL, NULL);

#endif /* CONFIG_TESTS_SUIT_DFU_TARGET_SUIT_CACHE_PROCESSING */
