/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <dfu/dfu_multi_image.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zcbor_decode.h>

#include <errno.h>
#include <string.h>

#define FIXED_HEADER_SIZE sizeof(uint16_t)
#define CBOR_HEADER_NESTING_LEVEL 3
#define IMAGE_NO_FIXED_HEADER -2
#define IMAGE_NO_CBOR_HEADER -1

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
			}
		}
	} else {
		/* Image data */
		const struct dfu_image_writer *writer = current_image_writer();

		if (!writer) {
			err = -ESPIPE;
		}

		if (!err && ctx.cur_item_offset == 0) {
			err = writer->open(writer->image_id,
					   ctx.header.images[ctx.cur_image_no].size);
		}

		if (!err) {
			err = writer->write(chunk, chunk_size);
		}

		if (!err && ctx.cur_item_offset + chunk_size == ctx.cur_item_size) {
			err = writer->close(true);
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
	return ctx.cur_offset;
}

int dfu_multi_image_done(bool success)
{
	const struct dfu_image_writer *writer = current_image_writer();
	int err = 0;

	/* Close any active writer if such exists */
	if (writer != NULL) {
		err = writer->close(success);
	}

	/* On success, verify that all images have been fully written */
	if (!err && success && ctx.cur_image_no != ctx.header.image_count) {
		return -ESPIPE;
	}

	return err;
}
