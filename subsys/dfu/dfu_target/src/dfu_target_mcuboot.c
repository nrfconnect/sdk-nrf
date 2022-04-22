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

#define IS_ALIGNED_32(POINTER) (((uintptr_t)(const void *)(POINTER)) % 4 == 0)

#define _MB_SEC_PAT(i, x) PM_MCUBOOT_SECONDARY_ ## i ## _ ## x


#define _H_MB_SEC_LA(i) (PM_MCUBOOT_SECONDARY_## i ##_ADDRESS + \
			 PM_MCUBOOT_SECONDARY_## i ##_SIZE - 1)

#define _MB_SEC_LA(i, _) _H_MB_SEC_LA(i)

#define _STR_TARGET_NAME(i, _) STRINGIFY(MCUBOOT##i)

#ifdef PM_MCUBOOT_SECONDARY_1_ID
	#define TARGET_IMAGE_COUNT 2
#else
	#define TARGET_IMAGE_COUNT 1
#endif

/* For the first image the Partition Managed doesen't us 0 indice.
 * Let's define liberal macros with 0 for making code more generic.
 */
#define PM_MCUBOOT_SECONDARY_0_SIZE     PM_MCUBOOT_SECONDARY_SIZE
#define PM_MCUBOOT_SECONDARY_0_ADDRESS  PM_MCUBOOT_SECONDARY_ADDRESS
#define PM_MCUBOOT_SECONDARY_0_DEV_NAME PM_MCUBOOT_SECONDARY_DEV_NAME

static const size_t secondary_size[] = {
	LISTIFY(TARGET_IMAGE_COUNT, _MB_SEC_PAT, (,), SIZE)
};

static const off_t secondary_address[] = {
	LISTIFY(TARGET_IMAGE_COUNT, _MB_SEC_PAT, (,), ADDRESS)
};

static const char *const secondary_dev_name[] = {
	LISTIFY(TARGET_IMAGE_COUNT, _MB_SEC_PAT, (,), DEV_NAME)
};

static const off_t secondary_last_address[] = {
	LISTIFY(TARGET_IMAGE_COUNT, _MB_SEC_LA, (,))
};

static const char *const target_id_name[] = {
	LISTIFY(TARGET_IMAGE_COUNT, _STR_TARGET_NAME, (,))
};

static uint8_t *stream_buf;
static size_t stream_buf_len;
static size_t stream_buf_bytes;
static uint8_t curr_sec_img;

int dfu_ctx_mcuboot_set_b1_file(char *const file, bool s0_active,
				const char **selected_path)
{
	if (file == NULL || selected_path == NULL) {
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
	char *delimiter = strstr(file, " ");

	if (delimiter == NULL) {
		/* Could not find delimiter in input */
		*selected_path = NULL;
		return 0;
	}

	/* Insert null-terminator to split the dual filepath into two separate filepaths */
	*delimiter = '\0';
	const char *s0_path = file;
	const char *s1_path = delimiter + 1;

	*selected_path = s0_active ? s1_path : s0_path;
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

	if (!IS_ALIGNED_32(buf)) {
		return -EINVAL;
	}

	stream_buf = buf;
	stream_buf_len = len;

	return 0;
}

int dfu_target_mcuboot_init(size_t file_size, int img_num, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
	const struct device *flash_dev;
	int err;

	stream_buf_bytes = 0;

	if (stream_buf == NULL) {
		LOG_ERR("Missing stream_buf, call '..set_buf' before '..init");
		return -ENODEV;
	}

	if (file_size > secondary_size[img_num]) {
		LOG_ERR("Requested file too big to fit in flash %zu > 0x%x",
			file_size, secondary_size[img_num]);
		return -EFBIG;
	}

	flash_dev = device_get_binding(secondary_dev_name[img_num]);
	if (flash_dev == NULL) {
		LOG_ERR("Failed to get device '%s'",
			secondary_dev_name[img_num]);
		return -EFAULT;
	}

	err = dfu_target_stream_init(&(struct dfu_target_stream_init){
		.id = target_id_name[img_num],
		.fdev = flash_dev,
		.buf = stream_buf,
		.len = stream_buf_len,
		.offset = secondary_address[img_num],
		.size = secondary_size[img_num],
		.cb = NULL });
	if (err < 0) {
		LOG_ERR("dfu_target_stream_init failed %d", err);
		return err;
	}

	curr_sec_img = img_num;
	return 0;
}

int dfu_target_mcuboot_offset_get(size_t *out)
{
	int err = 0;

	err = dfu_target_stream_offset_get(out);
	if (err == 0) {
		*out += stream_buf_bytes;
	}

	return err;
}

int dfu_target_mcuboot_write(const void *const buf, size_t len)
{
	stream_buf_bytes = (stream_buf_bytes + len) % stream_buf_len;

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
		stream_buf_bytes = 0;

		err = stream_flash_erase_page(dfu_target_stream_get_stream(),
					secondary_last_address[curr_sec_img]);
		if (err != 0) {
			LOG_ERR("Unable to delete last page: %d", err);
			return err;
		}
	} else {
		LOG_INF("MCUBoot image upgrade aborted.");
	}

	return err;
}

static int dfu_target_mcuboot_schedule_one_img(int img_num)
{
	int err = 0;

	err = boot_request_upgrade_multi(img_num, BOOT_UPGRADE_TEST);
	if (err != 0) {
		LOG_ERR("boot_request_upgrade for image-%d error %d",
			img_num, err);
	} else {
		LOG_INF("MCUBoot image-%d upgrade scheduled. "
			"Reset device to apply", img_num);
	}

	return err;
}

int dfu_target_mcuboot_schedule_update(int img_num)
{
	int err = 0;

	if (img_num == -1) {
		for (int i = 0; i < TARGET_IMAGE_COUNT && !err; i++) {
			err = dfu_target_mcuboot_schedule_one_img(i);
		}
	} else if (img_num >= 0 && img_num < TARGET_IMAGE_COUNT) {
		err = dfu_target_mcuboot_schedule_one_img(img_num);
	} else {
		err = -ENOENT;
	}

	return err;
}
