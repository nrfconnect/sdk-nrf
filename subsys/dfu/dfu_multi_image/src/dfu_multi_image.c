/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <dfu/dfu_multi_image.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zcbor_decode.h>

#include <errno.h>
#include <string.h>

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS

#include <zephyr/settings/settings.h>

#define SETTINGS_MODULE "dfu_multi"
#define CBOR_HEADER_SETTING_NAME "cbor_hdr"
#define FULL_CBOR_HEADER_SETTING_NAME SETTINGS_MODULE "/" CBOR_HEADER_SETTING_NAME
#define IMAGE_FINISHED_SETTING_NAME "img_fin"
#define FULL_IMAGE_FINISHED_SETTING_NAME SETTINGS_MODULE "/" IMAGE_FINISHED_SETTING_NAME

#endif /* CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS */

#define FIXED_HEADER_SIZE sizeof(uint16_t)
#define CBOR_HEADER_NESTING_LEVEL 3
#define IMAGE_NO_FIXED_HEADER -2
#define IMAGE_NO_CBOR_HEADER -1

LOG_MODULE_REGISTER(dfu_multi_image, CONFIG_DFU_MULTI_IMAGE_LOG_LEVEL);

struct image_info {
	int32_t id;
	uint32_t size;
};

struct header {
	struct image_info images[CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT];
	size_t image_count;
};

struct dfu_multi_image_ctx {
	/* User configuration */
	uint8_t *buffer;
	size_t buffer_size;
	struct dfu_image_writer writers[CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT];
	size_t writer_count;

	/* Parsed header */
	struct header header;

	/* Current parser state */
	int cur_image_no;
	size_t cur_offset;
	size_t cur_item_offset;
	size_t cur_item_size;
	bool cur_item_opened;
#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
	/**
	 * The saved progress cannot be loaded from the init function,
	 * because the writers may not be registered yet and they are needed
	 * to restore the offset for each image.
	 * Therefore, the load_saved_progress function is called by the first
	 * write/offset function call.
	 * This flag is used to avoid loading the saved progress multiple times.
	 */
	bool saved_progress_loaded;
	/**
	 * The highest image number that was fully written before resetting
	 * the device or power loss.
	 */
	int max_loaded_finished_image_no;
#endif
};

static struct dfu_multi_image_ctx ctx;

static int parse_fixed_header(void)
{
	ctx.cur_item_size += sys_get_le16(ctx.buffer);
	ctx.cur_image_no = IMAGE_NO_CBOR_HEADER;

	return (ctx.cur_item_size <= ctx.buffer_size) ? 0 : -ENOMEM;
}

static bool parse_image_info(zcbor_state_t *states, struct image_info *image)
{
	bool res;

	res = zcbor_map_start_decode(states);
	res = res && zcbor_tstr_expect_lit(states, "id");
	res = res && zcbor_int32_decode(states, &image->id);
	res = res && zcbor_tstr_expect_lit(states, "size");
	res = res && zcbor_uint32_decode(states, &image->size);
	res = res && zcbor_map_end_decode(states);

	return res;
}

static int parse_cbor_header(void)
{
	bool res;
	uint_fast32_t image_count;

	ZCBOR_STATE_D(states, CBOR_HEADER_NESTING_LEVEL, ctx.buffer + FIXED_HEADER_SIZE,
		      ctx.cur_item_size - FIXED_HEADER_SIZE, 1, 0);

	res = zcbor_map_start_decode(states);
	res = res && zcbor_tstr_expect_lit(states, "img");
	res = res && zcbor_list_start_decode(states);
	res = res && zcbor_multi_decode(1, CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT, &image_count,
					(zcbor_decoder_t *)parse_image_info, states,
					ctx.header.images, sizeof(struct image_info));
	res = res && zcbor_list_end_decode(states);
	res = res && zcbor_map_end_decode(states);

	if (!res) {
		return zcbor_pop_error(states);
	}

	ctx.header.image_count = image_count;

	return 0;
}

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
static int save_cbor_header(void)
{
	int err = 0;

	/*
	 * Save the cbor header to NVM.
	 * Writing the field values as separate setting would largely increase
	 * code complexity and would not have any benefits in terms of memory usage.
	 */
	err = settings_save_one(FULL_CBOR_HEADER_SETTING_NAME,
				ctx.buffer + FIXED_HEADER_SIZE,
				ctx.cur_item_size - FIXED_HEADER_SIZE);

	if (err) {
		LOG_ERR("Error storing cbor header to settings (err %d)", err);
	}

	return err;
}

static int save_image_finished(uint8_t image_no)
{
	int err = 0;

	err = settings_save_one(FULL_IMAGE_FINISHED_SETTING_NAME, &image_no,
				sizeof(image_no));

	if (err) {
		LOG_ERR("Error storing  image write finished info to settings (err %d)", err);
	}

	return err;
}

#endif

static const struct dfu_image_writer *current_image_writer(void)
{
	if (ctx.cur_image_no >= 0 && (size_t)ctx.cur_image_no < ctx.header.image_count) {
		const int image_id = ctx.header.images[ctx.cur_image_no].id;

		for (size_t i = 0; i < ctx.writer_count; i++) {
			if (ctx.writers[i].image_id == image_id) {
				return &ctx.writers[i];
			}
		}
	}

	return NULL;
}

static void select_next_image(void)
{
	ctx.cur_item_offset = 0;
	ctx.cur_item_size = 0;

	while (++ctx.cur_image_no < ctx.header.image_count && current_image_writer() == NULL) {
		ctx.cur_offset += ctx.header.images[ctx.cur_image_no].size;
	}

	if (ctx.cur_image_no < ctx.header.image_count) {
		ctx.cur_item_size = ctx.header.images[ctx.cur_image_no].size;
	}
}

static int process_current_item(const uint8_t *chunk, size_t chunk_size)
{
	int err = 0;

	/* Process only remaining bytes of the current item (header or image) */
	chunk_size = MIN(chunk_size, ctx.cur_item_size - ctx.cur_item_offset);

	if (ctx.cur_image_no < 0) {
		memcpy(ctx.buffer + ctx.cur_item_offset, chunk, chunk_size);

		if (ctx.cur_item_offset + chunk_size == ctx.cur_item_size) {
			if (ctx.cur_image_no == IMAGE_NO_FIXED_HEADER) {
				err = parse_fixed_header();
			} else {
				err = parse_cbor_header();
#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
				if (!err) {
					err = save_cbor_header();
				}
#endif
			}
		}
	} else {
		/* Image data */
		const struct dfu_image_writer *writer = current_image_writer();

		if (!writer) {
			err = -ESPIPE;
		}

		if (!err && ctx.cur_item_offset == 0 && !ctx.cur_item_opened) {
			err = writer->open(writer->image_id,
					   ctx.header.images[ctx.cur_image_no].size);
			ctx.cur_item_opened = true;
		}

		if (!err) {
			err = writer->write(chunk, chunk_size);
		}

		if (!err && ctx.cur_item_offset + chunk_size == ctx.cur_item_size) {
#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
			save_image_finished((uint8_t) ctx.cur_image_no);
#endif
			err = writer->close(true);
			ctx.cur_item_opened = false;
		}
	}

	if (err) {
		return err;
	}

	ctx.cur_offset += chunk_size;
	ctx.cur_item_offset += chunk_size;

	if (ctx.cur_item_offset == ctx.cur_item_size) {
		select_next_image();
	}

	return chunk_size;
}

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS

static int module_settings_set(const char *key, size_t len_rd,
			       settings_read_cb read_cb, void *cb_arg)
{
	ssize_t len;
	int rc;

	if (ctx.buffer != NULL && ctx.cur_image_no == IMAGE_NO_FIXED_HEADER
	    && strcmp(key, CBOR_HEADER_SETTING_NAME) == 0) {
		if (len_rd > (ctx.buffer_size - FIXED_HEADER_SIZE)) {
			LOG_ERR("CBOR header in settings is too large");
			return -ENOMEM;
		}

		/* Restore the cbor header from the NVS settings if available */
		len = read_cb(cb_arg, ctx.buffer + FIXED_HEADER_SIZE,
			      ctx.buffer_size - FIXED_HEADER_SIZE);
		if (len < 0) {
			LOG_ERR("Can't read cbor header from storage");
			return len;
		}

		ctx.cur_item_size = FIXED_HEADER_SIZE + len;
		ctx.cur_image_no = IMAGE_NO_CBOR_HEADER;

		rc = parse_cbor_header();

		if (rc < 0) {
			LOG_ERR("Failed to parse cbor header loaded from storage: %d", rc);
			return rc;
		}

		ctx.cur_offset = ctx.cur_item_size;

		select_next_image();
	}

	if (ctx.buffer != NULL && strcmp(key, IMAGE_FINISHED_SETTING_NAME) == 0) {
		/* Restore the cbor header from the NVS settings if available */
		uint8_t finished_image_no = 0;

		if (len_rd != sizeof(finished_image_no)) {
			/* Should never get here */
			return -EINVAL;
		}

		len = read_cb(cb_arg, &finished_image_no, sizeof(finished_image_no));

		if (len < 0) {
			LOG_ERR("Can't read finished image number from storage");
			return len;
		}

		if (ctx.max_loaded_finished_image_no < finished_image_no) {
			ctx.max_loaded_finished_image_no = finished_image_no;
		}
	}

	return 0;
}

static int load_saved_progress(void)
{
	int err = 0;
	static struct settings_handler sh = {
		.name = SETTINGS_MODULE,
		.h_set = module_settings_set,
	};

	LOG_INF("Loading saved progress from NVM");

	ctx.max_loaded_finished_image_no = -1;

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

	/* Load settings from NVM. If the update was interrupted, the context will be restored */
	err = settings_load();
	if (err) {
		LOG_ERR("settings_load failed (err %d)", err);
		return err;
	}

	if (ctx.cur_image_no == IMAGE_NO_FIXED_HEADER) {
		LOG_INF("No saved progress found");
		/* Progress was not stored in settings, start from the beginning */
		ctx.saved_progress_loaded = true;
		return 0;
	}

	if (ctx.cur_image_no < 0) {
		/* Should never get here */
		return -EFAULT;
	}

	while (ctx.cur_image_no < ctx.header.image_count) {
		const struct dfu_image_writer *writer = current_image_writer();

		if (writer == NULL) {
			LOG_WRN("No writer registered for image %d!", ctx.cur_image_no);
			err = -EPIPE;
			break;
		}
		if (writer->offset == NULL) {
			LOG_WRN("No function to read offset registered for image %d!",
				ctx.cur_image_no);
			err = -EPIPE;
			break;
		}

		err = writer->open(writer->image_id,
				   ctx.header.images[ctx.cur_image_no].size);

		if (!err) {
			ctx.cur_item_opened = true;
		}

		if (!err) {
			err = writer->offset(&ctx.cur_item_offset);
		}

		if (!err) {

			if (ctx.cur_image_no <= ctx.max_loaded_finished_image_no) {
				/* This image was fully written before power loss/reset */
				ctx.cur_offset += ctx.cur_item_size;
			} else {
				ctx.cur_offset += ctx.cur_item_offset;

				if (ctx.cur_item_offset < ctx.cur_item_size) {
					/*
					 * Writing to the current image is not finished.
					 * Resume writing from the current offset.
					 */
					break;
				}
			}
			/*
			 * The current image is already fully written.
			 * Close the writer and move to the next image.
			 */
			err = writer->close(true);
			ctx.cur_item_opened = false;
		} else {
			LOG_ERR("Failed to restore image %d offset", ctx.cur_image_no);
			break;
		}

		select_next_image();
	}

	if (!err) {
		ctx.saved_progress_loaded = true;
	} else {
		LOG_ERR("Error loading saved progress");
	}

	return err;
}

#endif /* CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS */

int dfu_multi_image_init(uint8_t *buffer, size_t buffer_size)
{
	if (buffer == NULL || buffer_size < FIXED_HEADER_SIZE) {
		return -EINVAL;
	}

	memset(&ctx, 0, sizeof(ctx));
	ctx.buffer = buffer;
	ctx.buffer_size = buffer_size;
	ctx.cur_image_no = IMAGE_NO_FIXED_HEADER;
	ctx.cur_item_size = FIXED_HEADER_SIZE;

	return 0;
}

int dfu_multi_image_register_writer(const struct dfu_image_writer *writer)
{
	if (ctx.writer_count == CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT) {
		return -ENOMEM;
	}

	ctx.writers[ctx.writer_count++] = *writer;
	return 0;
}

int dfu_multi_image_write(size_t offset, const uint8_t *chunk, size_t chunk_size)
{
	int result;
	size_t chunk_offset = 0;

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
	if (!ctx.saved_progress_loaded) {
		/* Load saved progress from NVM if available */
		int err = load_saved_progress();

		if (err) {
			return err;
		}
	}
#endif /* CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS */

	if (offset > ctx.cur_offset) {
		/* Unexpected data gap */
		return -ESPIPE;
	}

	while (1) {
		/* Skip ahead to the current write offset */
		chunk_offset += (ctx.cur_offset - offset);

		if (chunk_offset >= chunk_size) {
			break;
		}

		result = process_current_item(chunk + chunk_offset, chunk_size - chunk_offset);

		if (result <= 0) {
			return result;
		}

		chunk_offset += (size_t)result;
		offset += (size_t)result;
	}

	return 0;
}

size_t dfu_multi_image_offset(void)
{
#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
	if (!ctx.saved_progress_loaded) {
		/* Load saved progress from NVM if available */
		int err = load_saved_progress();

		if (err) {
			return err;
		}
	}
#endif /* CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS */

	return ctx.cur_offset;
}

int dfu_multi_image_done(bool success)
{
#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
	if (!ctx.saved_progress_loaded) {
		/* Load saved progress from NVM if available */
		int err = load_saved_progress();

		if (err) {
			return err;
		}
	}
#endif /* CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS */

	const struct dfu_image_writer *writer = current_image_writer();
	int err = 0;

	/* Close any active writer if such exists */
	if (writer != NULL) {
		err = writer->close(success);
	}

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS

	if (success) {
		err = settings_delete(FULL_CBOR_HEADER_SETTING_NAME);
		if (err != 0) {
			LOG_ERR("Error deleting stored context from NVM %d", err);
		}
		err = settings_delete(FULL_IMAGE_FINISHED_SETTING_NAME);
		if (err != 0) {
			LOG_ERR("Error deleting stored context from NVM %d", err);
		}
	}
#endif /* CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS */

	/* On success, verify that all images have been fully written */
	if (!err && success && ctx.cur_image_no != ctx.header.image_count) {
		return -ESPIPE;
	}

	return err;
}
