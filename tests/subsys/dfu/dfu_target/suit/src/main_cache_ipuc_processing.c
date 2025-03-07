/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef CONFIG_TESTS_SUIT_DFU_TARGET_SUIT_CACHE_IPUC_PROCESSING

#include <dfu/dfu_target.h>
#include <dfu/dfu_target_suit.h>
#include <suit_ipuc.h>
#include <suit_dfu_cache_rw.h>
#include "common.h"

/* The SUIT envelopes are defined inside the respective manfest_*.c files. */
extern uint8_t manifest[];
extern const size_t manifest_len;
extern uint8_t dfu_cache_partition[];
extern const size_t dfu_cache_partition_len;
extern uint8_t manifest_with_payload[];
extern const size_t manifest_with_payload_len;

#define FLASH_WRITE_BLOCK_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), write_block_size)

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

#define IPUC_PARTITION_APP_ADDRESS                                                                 \
	suit_plat_mem_nvm_ptr_get(FIXED_PARTITION_OFFSET(cpuapp_slot_a_partition))
#define IPUC_PARTITION_RAD_ADDRESS                                                                 \
	suit_plat_mem_nvm_ptr_get(FIXED_PARTITION_OFFSET(cpurad_slot_a_partition))

static uint8_t dfu_target_buffer[TEST_IMAGE_SIZE];

FAKE_VALUE_FUNC(int, suit_ipuc_get_count, size_t *);
FAKE_VALUE_FUNC(int, suit_ipuc_get_info, size_t, struct zcbor_string *, suit_manifest_role_t *);
FAKE_VALUE_FUNC(int, suit_ipuc_write_setup, struct zcbor_string *, struct zcbor_string *,
		struct zcbor_string *);
FAKE_VALUE_FUNC(int, suit_ipuc_write, struct zcbor_string *, size_t, uintptr_t, size_t, bool);

static inline void mock_suit_ipuc_reset(void)
{
	RESET_FAKE(suit_ipuc_get_count);
	RESET_FAKE(suit_ipuc_get_info);
	RESET_FAKE(suit_ipuc_write_setup);
	RESET_FAKE(suit_ipuc_write);
}

/* clang-format off */
#ifndef CONFIG_SOC_SERIES_NRF54HX
static const uint8_t component_rad_0x80000_0x1000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x03, /* Radio domain executable */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x08, 0x00, 0x00,
		0x43, /* bstr(3) */
			0x19, /* uint16 */
				0x10, 0x00,
};
static const uint8_t component_app_0x190000_0x1000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x02, /* Application domain executable */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x19, 0x00, 0x00,
		0x43, /* bstr(3) */
			0x19, /* uint16 */
				0x10, 0x00,
};
#else /* CONFIG_SOC_SERIES_NRF54HX */
static const uint8_t component_rad_0x80000_0x1000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x03, /* Radio domain executable */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x0e, 0x08, 0x00, 0x00,
		0x43, /* bstr(3) */
			0x19, /* uint16 */
				0x10, 0x00,
};
static const uint8_t component_app_0x190000_0x1000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x02, /* Application domain executable */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x0e, 0x19, 0x00, 0x00,
		0x43, /* bstr(3) */
			0x19, /* uint16 */
				0x10, 0x00,
};
#endif /* CONFIG_SOC_SERIES_NRF54HX */
/* clang-format on */

static suit_plat_err_t suit_ipuc_get_count_fake_func(size_t *count)
{
	zassert_not_null(count, "Caller must provide non-Null pointer");

	*count = 2;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_ipuc_get_info_fake_func(size_t idx, struct zcbor_string *component_id,
						    suit_manifest_role_t *role)
{
	zassert_not_null(component_id, "Caller must provide non-Null pointer to read component_id");
	zassert_not_null(role, "Caller must provide non-Null pointer to read role");
	zassert_true(idx < 2, "Unexpected idx value");

	if (idx == 0) {
		component_id->value = component_rad_0x80000_0x1000;
		component_id->len = sizeof(component_rad_0x80000_0x1000);
		*role = SUIT_MANIFEST_RAD_LOCAL_1;
	} else if (idx == 1) {
		component_id->value = component_app_0x190000_0x1000;
		component_id->len = sizeof(component_app_0x190000_0x1000);
		*role = SUIT_MANIFEST_APP_LOCAL_1;
	}

	return SUIT_PLAT_SUCCESS;
}

static void emulte_unaligned_write(size_t offset, uint8_t *buf, size_t len)
{
	uint8_t flash_write_buf[FLASH_WRITE_BLOCK_SIZE] = {0};
	size_t aligned_offset = 0;
	size_t bytes_cached = 0;
	size_t bytes_consumed = 0;

	aligned_offset = (offset / FLASH_WRITE_BLOCK_SIZE) * FLASH_WRITE_BLOCK_SIZE;
	if (aligned_offset != offset) {
		bytes_cached = offset - aligned_offset;
		bytes_consumed = MIN(FLASH_WRITE_BLOCK_SIZE - bytes_cached, len);

		zassert_equal(flash_read(SUIT_PLAT_INTERNAL_NVM_DEV, aligned_offset,
					 flash_write_buf, bytes_cached),
			      0, "Unable to cache data");
		memcpy(&flash_write_buf[bytes_cached], buf, bytes_consumed);
		zassert_equal(flash_write(SUIT_PLAT_INTERNAL_NVM_DEV, aligned_offset,
					  flash_write_buf, sizeof(flash_write_buf)),
			      0, "Unable to modify NVM to match requested changes");

		len -= bytes_consumed;
		buf += bytes_consumed;
		aligned_offset += sizeof(flash_write_buf);
	}

	if (len / FLASH_WRITE_BLOCK_SIZE > 0) {
		bytes_consumed = (len / FLASH_WRITE_BLOCK_SIZE) * FLASH_WRITE_BLOCK_SIZE;
		zassert_equal(flash_write(SUIT_PLAT_INTERNAL_NVM_DEV, aligned_offset, buf,
					  bytes_consumed),
			      0, "Unable to modify NVM to match requested changes");

		len -= bytes_consumed;
		buf += bytes_consumed;
		aligned_offset += bytes_consumed;
	}

	if (len > 0) {
		memset(flash_write_buf, 0xFF, sizeof(flash_write_buf));
		memcpy(flash_write_buf, buf, len);
		zassert_equal(flash_write(SUIT_PLAT_INTERNAL_NVM_DEV, aligned_offset,
					  flash_write_buf, sizeof(flash_write_buf)),
			      0, "Unable to modify NVM to match requested changes");
	}
}

static suit_plat_err_t suit_ipuc_write_fake_func(struct zcbor_string *component_id, size_t offset,
						 uintptr_t buffer, size_t chunk_size,
						 bool last_chunk)
{
	zassert_not_null(component_id, "Caller must provide non-Null pointer to read component_id");
	zassert_equal(component_id->len, sizeof(component_app_0x190000_0x1000),
		      "Invalid component ID length passed");
	if (memcmp(component_id->value, component_app_0x190000_0x1000, component_id->len) == 0) {
		emulte_unaligned_write(0x190000 + offset, (uint8_t *)buffer, chunk_size);
	} else if (memcmp(component_id->value, component_rad_0x80000_0x1000, component_id->len) ==
		   0) {
		emulte_unaligned_write(0x80000 + offset, (uint8_t *)buffer, chunk_size);
	} else {
		zassert_true(false, "Invalid component ID value passed");
	}

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_ipuc_write_setup_fake_func(struct zcbor_string *component_id,
						       struct zcbor_string *encryption_info,
						       struct zcbor_string *compression_info)
{
	zassert_is_null(encryption_info, "Decryption info unsupported by cache IPUCs");
	zassert_is_null(compression_info, "Decompression info unsupported by cache IPUCs");
	zassert_not_null(component_id, "Caller must provide non-Null pointer to read component_id");
	zassert_equal(component_id->len, sizeof(component_app_0x190000_0x1000),
		      "Invalid component ID length passed");
	if (memcmp(component_id->value, component_app_0x190000_0x1000, component_id->len) == 0) {
		zassert_equal(flash_erase(SUIT_PLAT_INTERNAL_NVM_DEV, 0x190000, 0x1000), 0,
			      "Unable to modify NVM to match requested changes");
	} else if (memcmp(component_id->value, component_rad_0x80000_0x1000, component_id->len) ==
		   0) {
		zassert_equal(flash_erase(SUIT_PLAT_INTERNAL_NVM_DEV, 0x80000, 0x1000), 0,
			      "Unable to modify NVM to match requested changes");
	} else {
		zassert_true(false, "Invalid component ID value passed");
	}

	return SUIT_PLAT_SUCCESS;
}

static void setup_test(void *f)
{
	(void)f;

	reset_fakes();
	mock_suit_ipuc_reset();

	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;
	suit_ipuc_write_fake.custom_fake = suit_ipuc_write_fake_func;
	suit_ipuc_write_setup_fake.custom_fake = suit_ipuc_write_setup_fake_func;

	/* Reinitialize DFU cache, so the values returned through fakes are applied. */
	suit_dfu_cache_rw_init();
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

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, CONFIG_SUIT_CACHE_SDFW_IPUC_ID + 1,
			     TEST_IMAGE_SIZE, NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, CONFIG_SUIT_CACHE_APP_IPUC_ID + 1,
			     TEST_IMAGE_SIZE, NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, 21, TEST_IMAGE_SIZE, NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, 31, TEST_IMAGE_SIZE, NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_reset();
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

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, 21, TEST_IMAGE_SIZE, NULL);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_write(dfu_cache_partition, dfu_cache_partition_len);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_suit_offset_get(&offset);
	zassert_equal(rc, 0, "DFU target offset get should pass for image 0 %d", rc);
	zassert_equal(offset, dfu_cache_partition_len,
		      "DFU target offset should be equal to image size");

	rc = dfu_target_done(true);
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, 31, TEST_IMAGE_SIZE, NULL);
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

	rc = memcmp((uint8_t *)IPUC_PARTITION_APP_ADDRESS, dfu_cache_partition,
		    dfu_cache_partition_len);
	zassert_equal(rc, 0, "DFU cache partition content does not match the file content %d", rc);

	rc = memcmp((uint8_t *)IPUC_PARTITION_RAD_ADDRESS, dfu_cache_partition,
		    dfu_cache_partition_len);
	zassert_equal(rc, 0, "DFU cache partition content does not match the file content %d", rc);

	rc = dfu_target_suit_reset();
	zassert_equal(rc, 0, "Unexpected failure: %d", rc);

	rc = memcmp((uint8_t *)IPUC_PARTITION_APP_ADDRESS, dfu_cache_partition,
		    dfu_cache_partition_len);
	zassert_equal(rc, 0, "DFU cache partition content does not match the file content %d", rc);

	rc = memcmp((uint8_t *)IPUC_PARTITION_RAD_ADDRESS, dfu_cache_partition,
		    dfu_cache_partition_len);
	zassert_equal(rc, 0, "DFU cache partition content does not match the file content %d", rc);
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

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, CONFIG_SUIT_CACHE_SDFW_IPUC_ID + 1,
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

	rc = dfu_target_init(DFU_TARGET_IMAGE_TYPE_SUIT, CONFIG_SUIT_CACHE_APP_IPUC_ID + 1,
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

#endif /* CONFIG_TESTS_SUIT_DFU_TARGET_SUIT_CACHE_IPUC_PROCESSING */
