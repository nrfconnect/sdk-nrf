/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <dfu/dfu_multi_image.h>
#include <zephyr/dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_mcuboot.h>
#include <pm_config.h>
#include "dfu_multi_target.h"

#define DFU_TARGET_SCHEDULE_ALL_IMAGES -1

union dfu_multi_target_ver {
	uint32_t uint32_ver;
	struct {
		uint16_t revision;
		uint8_t minor;
		uint8_t major;
	};
};

static int dfu_multi_target_open(int image_id, size_t image_size);
static int dfu_multi_target_write(const uint8_t *chunk, size_t chunk_size);
static int dfu_multi_target_done_0(bool success);
#if CONFIG_UPDATEABLE_IMAGE_NUMBER > 1
static int dfu_multi_target_done_1(bool success);
#endif
#if CONFIG_UPDATEABLE_IMAGE_NUMBER > 2
static int dfu_multi_target_done_2(bool success);
#endif

LOG_MODULE_DECLARE(zigbee_fota);
static uint32_t images_to_download;

/** Buffer for parsing the multi-image header as well as DFU target buffer. */
static uint8_t image_buf[DFU_MULTI_TARGET_BUFFER_SIZE] __aligned(4);

/** DFU target writer for image 0 (single-app image or APP core image put in internal flash). */
static struct dfu_image_writer dfu_multi_target_writer_0 = {
	.image_id = 0,
	.open = dfu_multi_target_open,
	.write = dfu_multi_target_write,
	.close = dfu_multi_target_done_0
};

#if CONFIG_UPDATEABLE_IMAGE_NUMBER > 1
/** DFU target writer for image 1 (NET core). */
static struct dfu_image_writer dfu_multi_target_writer_1 = {
	.image_id = 1,
	.open = dfu_multi_target_open,
	.write = dfu_multi_target_write,
	.close = dfu_multi_target_done_1
};
#endif

#if CONFIG_UPDATEABLE_IMAGE_NUMBER > 2
/** DFU target writer for image 2 (split from APP core and to be put in external memory). */
static struct dfu_image_writer dfu_multi_target_writer_2 = {
	.image_id = 2,
	.open = dfu_multi_target_open,
	.write = dfu_multi_target_write,
	.close = dfu_multi_target_done_2
};
#endif

static int dfu_multi_target_open(int image_id, size_t image_size)
{
	LOG_INF("Open image: %d size: %d", image_id, image_size);

	if (image_id >= CONFIG_UPDATEABLE_IMAGE_NUMBER) {
		return -EINVAL;
	}

	return dfu_target_init(DFU_TARGET_IMAGE_TYPE_MCUBOOT, image_id, image_size, NULL);
}

static int dfu_multi_target_write(const uint8_t *chunk, size_t chunk_size)
{
	LOG_DBG("Write next %d bytes", chunk_size);

	return dfu_target_write(chunk, chunk_size);
}

static int dfu_multi_target_done_0(bool success)
{
	LOG_INF("Close image 0 for writing success: %d", success);

	if (success) {
		images_to_download &= ~(1 << 0);
	}

	return dfu_target_done(success);
}

#if CONFIG_UPDATEABLE_IMAGE_NUMBER > 1
static int dfu_multi_target_done_1(bool success)
{
	LOG_INF("Close image 1 for writing success: %d", success);

	if (success) {
		images_to_download &= ~(1 << 1);
	}

	return dfu_target_done(success);
}
#endif

#if CONFIG_UPDATEABLE_IMAGE_NUMBER > 2
static int dfu_multi_target_done_2(bool success)
{
	LOG_INF("Close image 2 for writing success: %d", success);

	if (success) {
		images_to_download &= ~(1 << 2);
	}

	return dfu_target_done(success);
}
#endif

uint32_t dfu_multi_target_get_version(void)
{
	struct mcuboot_img_header mcuboot_header;
	union dfu_multi_target_ver image_ver = {0};

	int err = boot_read_bank_header(PM_MCUBOOT_PRIMARY_ID,
					&mcuboot_header,
					sizeof(mcuboot_header));
	if (err) {
		LOG_WRN("Failed read app ver from image header (err %d)", err);
	} else {
		image_ver.major = mcuboot_header.h.v1.sem_ver.major;
		image_ver.minor = mcuboot_header.h.v1.sem_ver.minor;
		image_ver.revision = mcuboot_header.h.v1.sem_ver.revision;
	}

        return image_ver.uint32_ver;
}

int dfu_multi_target_init_default(void)
{
	int err;

	err = dfu_multi_image_init(image_buf, sizeof(image_buf));
	if (err) {
		LOG_ERR("Failed to initialize DFU multi image library (err %d)", err);
		return err;
	}

	images_to_download = 0;

	err = dfu_multi_image_register_writer(&dfu_multi_target_writer_0);
	if (err) {
		LOG_ERR("Failed to register DFU target writer for image 0 (err %d)", err);
		return err;
	}

	images_to_download |= (1 << 0);

#if CONFIG_UPDATEABLE_IMAGE_NUMBER > 1
	err = dfu_multi_image_register_writer(&dfu_multi_target_writer_1);
	if (err) {
		LOG_ERR("Failed to register DFU target writer for image 1 (err %d)", err);
		return err;
	}

	images_to_download |= (1 << 1);
#endif

#if CONFIG_UPDATEABLE_IMAGE_NUMBER > 2
	err = dfu_multi_image_register_writer(&dfu_multi_target_writer_2);
	if (err) {
		LOG_ERR("Failed to register DFU target writer for image 2 (err %d)", err);
		return err;
	}

	images_to_download |= (1 << 2);
#endif

	err = dfu_target_mcuboot_set_buf(image_buf, sizeof(image_buf));
	if (err) {
		LOG_ERR("Failed to set DFU target buffer (err %d)", err);
	}

	return err;
}

int dfu_multi_target_schedule_update(void)
{
	if (images_to_download != 0) {
		LOG_ERR("Failed to set schedule update. Missing images: 0x%x", images_to_download);
		return -ENOEXEC;
	}

	return dfu_target_schedule_update(DFU_TARGET_SCHEDULE_ALL_IMAGES);
}
