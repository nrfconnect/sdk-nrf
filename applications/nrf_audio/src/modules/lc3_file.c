/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "lc3_file.h"
#include "sd_card.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sd_card_lc3_file, CONFIG_MODULE_SD_CARD_LC3_FILE_LOG_LEVEL);

#define LC3_FILE_ID 0xCC1C

static void lc3_header_print(struct lc3_file_header *header)
{
	if (header == NULL) {
		LOG_ERR("Nullptr received");
		return;
	}

	LOG_DBG("File ID: 0x%04x", header->file_id);
	LOG_DBG("Header size: %d", header->hdr_size);
	LOG_DBG("Sample rate: %d Hz", header->sample_rate * 100);
	LOG_DBG("Bit rate: %d bps", header->bit_rate * 100);
	LOG_DBG("Channels: %d", header->channels);
	LOG_DBG("Frame duration: %d us", header->frame_duration * 10);
	LOG_DBG("Num samples: %d", header->signal_len_lsb | (header->signal_len_msb << 16));
}

int lc3_header_get(struct lc3_file_ctx const *const file, struct lc3_file_header *header)
{
	if ((file == NULL) || (header == NULL)) {
		LOG_ERR("Nullptr received");
		return -EINVAL;
	}

	*header = file->lc3_header;

	return 0;
}

int lc3_file_frame_get(struct lc3_file_ctx *file, uint8_t *buffer, size_t buffer_size)
{
	int ret;

	if ((file == NULL) || (buffer == NULL)) {
		LOG_ERR("Nullptr received");
		return -EINVAL;
	}

	/* Read frame header */
	uint16_t frame_header;
	size_t frame_header_size = sizeof(frame_header);

	ret = sd_card_read((char *)&frame_header, &frame_header_size, &file->file_object);
	if (ret) {
		LOG_ERR("Failed to read frame header: %d", ret);
		return ret;
	}

	if ((frame_header_size == 0) || (frame_header == 0)) {
		LOG_DBG("No more frames to read");
		return -ENODATA;
	}

	LOG_DBG("Size of frame is %d", frame_header);

	if (buffer_size < frame_header) {
		LOG_ERR("Buffer size too small: %d < %d", buffer_size, frame_header);
		return -ENOMEM;
	}

	/* Read frame data */
	size_t frame_size = frame_header;

	ret = sd_card_read((char *)buffer, &frame_size, &file->file_object);
	if (ret) {
		LOG_ERR("Failed to read frame data: %d", ret);
		return ret;
	}

	if (frame_size != frame_header) {
		LOG_ERR("Frame size mismatch: %d != %d", frame_size, frame_header);
		return -EIO;
	}

	return 0;
}

int lc3_file_open(struct lc3_file_ctx *file, const char *file_name)
{
	int ret;
	size_t size = sizeof(file->lc3_header);

	if ((file == NULL) || (file_name == NULL)) {
		LOG_ERR("Nullptr received");
		return -EINVAL;
	}

	ret = sd_card_open(file_name, &file->file_object);
	if (ret) {
		LOG_ERR("Failed to open file: %d", ret);
		return ret;
	}

	/* Read LC3 header and store in struct */
	ret = sd_card_read((char *)&file->lc3_header, &size, &file->file_object);
	if (ret) {
		LOG_ERR("Failed to read the LC3 header: %d", ret);
		return ret;
	}

	/* Debug: Print header */
	lc3_header_print(&file->lc3_header);

	if (file->lc3_header.file_id != LC3_FILE_ID) {
		LOG_ERR("Invalid file ID: 0x%04x", file->lc3_header.file_id);
		return -EINVAL;
	}

	return 0;
}

int lc3_file_close(struct lc3_file_ctx *file)
{
	int ret;

	if (file == NULL) {
		LOG_ERR("Nullptr received");
		return -EINVAL;
	}

	ret = sd_card_close(&file->file_object);
	if (ret) {
		LOG_ERR("Failed to close file: %d", ret);
		return ret;
	}

	return ret;
}

int lc3_file_init(void)
{
	int ret;

	ret = sd_card_init();
	if (ret) {
		LOG_ERR("Failed to initialize SD card: %d", ret);
		return ret;
	}

	return 0;
}
