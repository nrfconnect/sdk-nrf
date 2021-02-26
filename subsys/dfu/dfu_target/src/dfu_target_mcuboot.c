/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Ensure 'strnlen' is available even with -std=c99. If
 * _POSIX_C_SOURCE was defined we will get a warning when referencing
 * 'strnlen'. If this proves to cause trouble we could easily
 * re-implement strnlen instead, perhaps with a different name, as it
 * is such a simple function.
 */
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include <string.h>

#include <zephyr.h>
#include <pm_config.h>
#include <logging/log.h>
#include <nrfx.h>
#include <dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>

LOG_MODULE_REGISTER(dfu_target_mcuboot, CONFIG_DFU_TARGET_LOG_LEVEL);

#define MAX_FILE_SEARCH_LEN 500
#define MCUBOOT_HEADER_MAGIC 0x96f3b83d
#define MCUBOOT_SECONDARY_LAST_PAGE_ADDR                                       \
	(PM_MCUBOOT_SECONDARY_ADDRESS + PM_MCUBOOT_SECONDARY_SIZE - 1)

static uint8_t *stream_buf;
static size_t stream_buf_len;

int dfu_ctx_mcuboot_set_b1_file(const char *file, bool s0_active,
				const char **update)
{
	if (file == NULL || update == NULL) {
		LOG_ERR("Got NULL pointer");
		return -EINVAL;
	}

	if (!nrfx_is_in_ram(file)) {
		LOG_ERR("'file' pointer is not located in RAM");
		return -EFAULT;
	}

	/* Ensure that 'file' is null-terminated. */
	if (strnlen(file, MAX_FILE_SEARCH_LEN) == MAX_FILE_SEARCH_LEN) {
		LOG_ERR("Input is not null terminated");
		return -ENOTSUP;
	}

	/* We have verified that there is a null-terminator, so this is safe */
	char *space = strstr(file, " ");

	if (space == NULL) {
		/* Could not find space separator in input */
		*update = NULL;

		return 0;
	}

	if (s0_active) {
		/* Point after space to 'activate' second file path (S1) */
		*update = space + 1;
	} else {
		*update = file;

		/* Insert null-terminator to 'activate' first file path (S0) */
		*space = '\0';
	}

	return 0;
}

bool dfu_target_mcuboot_identify(const void *const buf)
{
	/* MCUBoot headers starts with 4 byte magic word */
	return *((const uint32_t *)buf) == MCUBOOT_HEADER_MAGIC;
}

int dfu_target_mcuboot_set_buf(uint8_t *buf, size_t len)
{
	if (buf == NULL) {
		return -EINVAL;
	}

	stream_buf = buf;
	stream_buf_len = len;

	return 0;
}

int dfu_target_mcuboot_init(size_t file_size, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
	const struct device *flash_dev;
	int err;

	if (stream_buf == NULL) {
		LOG_ERR("Missing stream_buf, call '..set_buf' before '..init");
		return -ENODEV;
	}

	if (file_size > PM_MCUBOOT_SECONDARY_SIZE) {
		LOG_ERR("Requested file too big to fit in flash %zu > 0x%x",
			file_size, PM_MCUBOOT_SECONDARY_SIZE);
		return -EFBIG;
	}

	flash_dev = device_get_binding(PM_MCUBOOT_SECONDARY_DEV_NAME);
	if (flash_dev == NULL) {
		LOG_ERR("Failed to get device '%s'",
			PM_MCUBOOT_SECONDARY_DEV_NAME);
		return -EFAULT;
	}

	err = dfu_target_stream_init(&(struct dfu_target_stream_init){
		.id = "MCUBOOT",
		.fdev = flash_dev,
		.buf = stream_buf,
		.len = stream_buf_len,
		.offset = PM_MCUBOOT_SECONDARY_ADDRESS,
		.size = PM_MCUBOOT_SECONDARY_SIZE,
		.cb = NULL });
	if (err < 0) {
		LOG_ERR("dfu_target_stream_init failed %d", err);
		return err;
	}

	return 0;
}

int dfu_target_mcuboot_offset_get(size_t *out)
{
	return dfu_target_stream_offset_get(out);
}

int dfu_target_mcuboot_write(const void *const buf, size_t len)
{
	return dfu_target_stream_write(buf, len);
}

int dfu_target_mcuboot_done(bool successful)
{
	int err = 0;

	err = dfu_target_stream_done(successful);
	if (err != 0) {
		LOG_ERR("dfu_target_stream_done error %d", err);
		return err;
	}

	if (successful) {
		err = stream_flash_erase_page(dfu_target_stream_get_stream(),
					      MCUBOOT_SECONDARY_LAST_PAGE_ADDR);
		if (err != 0) {
			LOG_ERR("Unable to delete last page: %d", err);
			return err;
		}
		err = boot_request_upgrade(BOOT_UPGRADE_TEST);
		if (err != 0) {
			LOG_ERR("boot_request_upgrade error %d", err);
			return err;
		}

		LOG_INF("MCUBoot image upgrade scheduled. "
			"Reset device to apply");
	} else {
		LOG_INF("MCUBoot image upgrade aborted.");
	}

	return err;
}
