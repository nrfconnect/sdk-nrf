/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LC3_FILE_H__
#define LC3_FILE_H__

#include <stddef.h>
#include <stdint.h>

#include <zephyr/fs/fs.h>
#include <zephyr/sys/util.h>

struct lc3_file_header {
	uint16_t file_id;	 /* Constant value, 0xCC1C */
	uint16_t hdr_size;	 /* Header size, 0x0012 */
	uint16_t sample_rate;	 /* Sample frequency / 100 */
	uint16_t bit_rate;	 /* Bit rate / 100 (total for all channels) */
	uint16_t channels;	 /* Number of channels */
	uint16_t frame_duration; /* Frame duration in ms * 100 */
	uint16_t rfu;		 /* Reserved for future use */
	uint16_t signal_len_lsb; /* Number of samples in signal, 16 LSB */
	uint16_t signal_len_msb; /* Number of samples in signal, 16 MSB (>> 16) */
} __packed;

struct lc3_file_ctx {
	struct fs_file_t file_object;
	struct lc3_file_header lc3_header;
	uint32_t number_of_samples;
};

/**
 * @brief Get the LC3 header from the file.
 *
 * @param[in]	file	Pointer to the file context.
 * @param[out]	header	Pointer to the header structure to store the header.
 *
 * @retval -EINVAL	Invalid file context.
 * @retval 0		Success.
 */
int lc3_header_get(struct lc3_file_ctx const *const file, struct lc3_file_header *header);

/**
 * @brief Get the next LC3 frame from the file.
 *
 * @param[in]	file		Pointer to the file context.
 * @param[out]	buffer		Pointer to the buffer to store the frame.
 * @param[in]	buffer_size	Size of the buffer.
 *
 * @retval -ENODATA	No more frames to read.
 * @retval 0		Success.
 */
int lc3_file_frame_get(struct lc3_file_ctx *file, uint8_t *buffer, size_t buffer_size);

/**
 * @brief Open a LC3 file for reading
 *
 * @details Opens the file and reads the LC3 header.
 *
 * @param[in]	file		Pointer to the file context.
 * @param[in]	file_name	Name of the file to open.
 *
 * @retval -ENODEV	SD card init failed. SD card likely not inserted.
 * @retval 0		Success.
 */
int lc3_file_open(struct lc3_file_ctx *file, const char *file_name);

/**
 * @brief Close a LC3 file.
 *
 * @param[in]	file	Pointer to the file context.
 *
 * @retval -EPERM	SD card operation is not ongoing.
 * @retval 0		Success.
 */
int lc3_file_close(struct lc3_file_ctx *file);

/**
 * @brief Initialize the LC3 file module.
 *
 * Initializes the SD card and mounts the file system.
 *
 * @retval -ENODEV	SD card init failed. SD card likely not inserted.
 * @retval 0		Success.
 */
int lc3_file_init(void);

#endif /* LC3_FILE_H__ */
