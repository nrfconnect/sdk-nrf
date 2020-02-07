/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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
#include <drivers/flash.h>
#include <pm_config.h>
#include <logging/log.h>
#include <dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <dfu/flash_img.h>
#include <settings/settings.h>

LOG_MODULE_REGISTER(dfu_target_mcuboot, CONFIG_DFU_TARGET_LOG_LEVEL);

#define MAX_FILE_SEARCH_LEN 500
#define MCUBOOT_HEADER_MAGIC 0x96f3b83d

static struct flash_img_context flash_img;

int dfu_ctx_mcuboot_set_b1_file(const char *file, bool s0_active,
				const char **update)
{
	if (file == NULL || update == NULL) {
		return -EINVAL;
	}

	/* Ensure that 'file' is null-terminated. */
	if (strnlen(file, MAX_FILE_SEARCH_LEN) == MAX_FILE_SEARCH_LEN) {
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

#define MODULE "dfu"
#define FILE_FLASH_IMG "mcuboot/flash_img"
/**
 * @brief Store the information stored in the flash_img instance so that it can
 *	  be restored from flash in case of a power failure, reboot etc.
 */
static int store_flash_img_context(void)
{
	if (IS_ENABLED(CONFIG_DFU_TARGET_MCUBOOT_SAVE_PROGRESS)) {
		char key[] = MODULE "/" FILE_FLASH_IMG;
		int err = settings_save_one(key, &flash_img, sizeof(flash_img));

		if (err) {
			LOG_ERR("Problem storing offset (err %d)", err);
			return err;
		}
	}

	return 0;
}

/**
 * @brief Function used by settings_load() to restore the flash_img variable.
 *	  See the Zephyr documentation of the settings subsystem for more
 *	  information.
 */
static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(key, FILE_FLASH_IMG)) {
		ssize_t len = read_cb(cb_arg, &flash_img,
				      sizeof(flash_img));
		if (len != sizeof(flash_img)) {
			LOG_ERR("Can't read flash_img from storage");
			return len;
		}
	}

	return 0;
}

bool dfu_target_mcuboot_identify(const void *const buf)
{
	/* MCUBoot headers starts with 4 byte magic word */
	return *((const u32_t *)buf) == MCUBOOT_HEADER_MAGIC;
}

int dfu_target_mcuboot_init(size_t file_size, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
	int err = flash_img_init(&flash_img);

	if (err != 0) {
		LOG_ERR("flash_img_init error %d", err);
		return err;
	}

	if (file_size > PM_MCUBOOT_SECONDARY_SIZE) {
		LOG_ERR("Requested file too big to fit in flash %d > %d",
			file_size, PM_MCUBOOT_SECONDARY_SIZE);
		return -EFBIG;
	}

	if (IS_ENABLED(CONFIG_DFU_TARGET_MCUBOOT_SAVE_PROGRESS)) {
		static struct settings_handler sh = {
			.name = MODULE,
			.h_set = settings_set,
		};

		/* settings_subsys_init is idempotent so this is safe to do. */
		err = settings_subsys_init();
		if (err) {
			LOG_ERR("settings_subsys_init failed (err %d)", err);
			return err;
		}

		err = settings_register(&sh);
		if (err) {
			LOG_ERR("Cannot register settings (err %d)", err);
			return err;
		}

		err = settings_load();
		if (err) {
			LOG_ERR("Cannot load settings (err %d)", err);
			return err;
		}
	}

	return 0;
}

int dfu_target_mcuboot_offset_get(size_t *out)
{
	*out = flash_img_bytes_written(&flash_img);
	return 0;
}

int dfu_target_mcuboot_write(const void *const buf, size_t len)
{
	int err = flash_img_buffered_write(&flash_img, (u8_t *)buf, len, false);

	if (err != 0) {
		LOG_ERR("flash_img_buffered_write error %d", err);
		return err;
	}

	err = store_flash_img_context();
	if (err != 0) {
		/* Failing to store progress is not a critical error you'll just
		 * be left to download a bit more if you fail and resume.
		 */
		LOG_WRN("Unable to store write progress: %d", err);
	}

	return 0;
}

static void reset_flash_context(void)
{
	/* Need to set bytes_written to 0 */
	int err = flash_img_init(&flash_img);

	if (err) {
		LOG_ERR("Unable to re-initialize flash_img");
	}
	err = store_flash_img_context();
	if (err != 0) {
		LOG_ERR("Unable to reset write progress: %d", err);
	}
}

int dfu_target_mcuboot_done(bool successful)
{
	int err = 0;

	if (successful) {
		err = flash_img_buffered_write(&flash_img, NULL, 0, true);
		if (err != 0) {
			LOG_ERR("flash_img_buffered_write error %d", err);
			reset_flash_context();
			return err;
		}

		err = boot_request_upgrade(BOOT_UPGRADE_TEST);
		if (err != 0) {
			LOG_ERR("boot_request_upgrade error %d", err);
			reset_flash_context();
			return err;
		}

		LOG_INF("MCUBoot image upgrade scheduled. Reset the device to "
			"apply");
	} else {
		LOG_INF("MCUBoot image upgrade aborted.");
	}

	reset_flash_context();
	return err;
}
