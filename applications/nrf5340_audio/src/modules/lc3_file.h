/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup nrf5340_audio_lc3_file LC3 File
 * @{
 * @brief LC3 file handling API for nRF5340 Audio applications.
 *
 * This module provides LC3 audio file I/O operations for reading and parsing LC3
 * encoded audio files from SD card storage. It handles LC3 file header parsing
 * and frame extraction, and provides file management functions for audio playback
 * applications. The module supports the LC3 file format specification with proper
 * header validation and frame-by-frame reading capabilities. It integrates with
 * the SD card module for SD card storage operations and provides error
 * handling for file system operations. It coordinates with @ref nrf5340_audio_datapath
 * for audio data delivery and integrates with Audio I2S module for
 * audio output processing in playback applications.
 */

#ifndef LC3_FILE_H__
#define LC3_FILE_H__

#include <stddef.h>
#include <stdint.h>

#include <zephyr/fs/fs.h>
#include <zephyr/sys/util.h>

/**
 * @brief LC3 file header structure.
 *
 * This structure defines the header format for LC3 audio files,
 * containing metadata about the audio content including sample rate,
 * bit rate, channel configuration, and signal length.
 */
struct lc3_file_header {
	uint16_t file_id;	 /**< Constant value, 0xCC1C */
	uint16_t hdr_size;	 /**< Header size, 0x0012 */
	uint16_t sample_rate;	 /**< Sample frequency / 100 */
	uint16_t bit_rate;	 /**< Bit rate / 100 (total for all channels) */
	uint16_t channels;	 /**< Number of channels */
	uint16_t frame_duration; /**< Frame duration in ms * 100 */
	uint16_t rfu;		 /**< Reserved for future use */
	uint16_t signal_len_lsb; /**< Number of samples in signal, 16 LSB */
	uint16_t signal_len_msb; /**< Number of samples in signal, 16 MSB (>> 16) */
} __packed;

/**
 * @brief LC3 file context structure.
 *
 * This structure maintains the state of an open LC3 file,
 * including the file object, parsed header information,
 * and calculated sample count.
 */
struct lc3_file_ctx {
	struct fs_file_t file_object;	/**< File system file object */
	struct lc3_file_header lc3_header;	/**< Parsed LC3 file header */
	uint32_t number_of_samples;	/**< Total number of samples in the file */
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
 *
 * @see @ref lc3_streamer_next_frame_get for streamer frame retrieval
 * @see @ref lc3_header_get for header information
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

/**
 * @}
 */

#endif /* LC3_FILE_H__ */
