/*
 * Copyright (c) 2020-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/stream_flash.h>
#include <stdio.h>
#include <dfu/dfu_target_stream.h>
#include <dfu_stream_flatten.h>

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
#define MODULE "dfu"
#define DFU_STREAM_OFFSET "stream/offset"
#include <zephyr/settings/settings.h>
#endif /* CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS */

LOG_MODULE_REGISTER(dfu_target_stream, CONFIG_DFU_TARGET_LOG_LEVEL);

static struct stream_flash_ctx stream;
static const char *current_id;

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS

static char current_name_key[32];

/**
 * @brief Store the information stored in the stream_flash instance so that it
 *        can be restored from flash in case of a power failure, reboot etc.
 */
static int store_progress(void)
{
	int err;
	size_t bytes_written = stream_flash_bytes_written(&stream);

	err = settings_save_one(current_name_key, &bytes_written,
				sizeof(bytes_written));

	if (err) {
		LOG_ERR("Problem storing offset (err %d)", err);
		return err;
	}

	return 0;
}

/**
 * @brief Function used by settings_load() to restore the stream_flash ctx.
 *	  See the Zephyr documentation of the settings subsystem for more
 *	  information.
 */
static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (current_id && !strcmp(key, current_id)) {
		int err;
		off_t absolute_offset;
		struct flash_pages_info page;
		ssize_t len = read_cb(cb_arg, &stream.bytes_written,
				      sizeof(stream.bytes_written));

		if (len != sizeof(stream.bytes_written)) {
			LOG_ERR("Can't read stream.bytes_written from storage");
			return len;
		}

		/* Zero bytes written - set last erased page to its default. */
		if (stream.bytes_written == 0) {
			stream.last_erased_page_start_offset = -1;
			return 0;
		}

		absolute_offset = stream.offset + stream.bytes_written - 1;

		err = flash_get_page_info_by_offs(stream.fdev,
						  absolute_offset,
						  &page);
		if (err != 0) {
			LOG_ERR("Error %d while getting page info", err);
			return err;
		}

		/* Update the last erased page to avoid deleting already
		 * written data.
		 */
		stream.last_erased_page_start_offset = page.start_offset;
	}

	return 0;
}
#endif /* CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS */

struct stream_flash_ctx *dfu_target_stream_get_stream(void)
{
	return &stream;
}

int dfu_target_stream_init(const struct dfu_target_stream_init *init)
{
	int err;

	if (current_id != NULL) {
		return -EFAULT;
	}

	if (init == NULL || init->id == NULL || init->fdev == NULL ||
	    init->buf == NULL) {
		return -EINVAL;
	}

	current_id = init->id;

	err = stream_flash_init(&stream, init->fdev, init->buf, init->len,
				init->offset, init->size, NULL);
	if (err) {
		LOG_ERR("stream_flash_init failed (err %d)", err);
		return err;
	}

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
	err = snprintf(current_name_key, sizeof(current_name_key), "%s/%s",
		       MODULE, current_id);
	if (err < 0 || err >= sizeof(current_name_key)) {
		LOG_ERR("Unable to generate current_name_key");
		return -EFAULT;
	}

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
	if (err && err != -EEXIST) {
		LOG_ERR("setting_register failed: (err %d)", err);
		return err;
	}

	err = settings_load();
	if (err) {
		LOG_ERR("settings_load failed (err %d)", err);
		return err;
	}
#endif /* CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS */

	return 0;
}

int dfu_target_stream_offset_get(size_t *out)
{
	*out = stream_flash_bytes_written(&stream);

	return 0;
}

int dfu_target_stream_write(const uint8_t *buf, size_t len)
{
	int err = stream_flash_buffered_write(&stream, buf, len, false);

	if (err != 0) {
		LOG_ERR("stream_flash_buffered_write error %d", err);
		return err;
	}

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
	err = store_progress();
	if (err != 0) {
		/* Failing to store progress is not a critical error you'll just
		 * be left to download a bit more if you fail and resume.
		 */
		LOG_WRN("Unable to store write progress: %d", err);
	}
#endif

	return err;
}

int dfu_target_stream_done(bool successful)
{
	int err = 0;

	if (successful) {
		err = stream_flash_buffered_write(&stream, NULL, 0, true);
		if (err != 0) {
			LOG_ERR("stream_flash_buffered_write error %d", err);
		}
#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
		/* Delete state so that a new call to 'init' will
		 * start with offset 0.
		 */
		err = settings_delete(current_name_key);
		if (err != 0) {
			LOG_ERR("setting_delete error %d", err);
		}

	} else {
		/* The stream has not completed, store the progress so that
		 * a new call to 'init' will pick up where we left off.
		 */
		err = store_progress();
		if (err != 0) {
			LOG_ERR("Unable to reset write progress: %d", err);
		}
#endif
	}

	current_id = NULL;

	return err;
}

int dfu_target_stream_reset(void)
{
	int err = 0;

	stream.buf_bytes = 0;
	stream.bytes_written = 0;

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
	err = settings_delete(current_name_key);
	if (err != 0) {
		LOG_ERR("settings_delete error %d", err);
	}
#endif

	/* No flash device specified, nothing to erase. */
	if (stream.fdev == NULL) {
		current_id = NULL;
		return 0;
	}

	/* Erase just the first page. Stream write will take care of erasing remaining pages
	 * on a next buffered_write round
	 */
	err = stream_flash_flatten_page(&stream, stream.offset);
	current_id = NULL;

	return err;
}
