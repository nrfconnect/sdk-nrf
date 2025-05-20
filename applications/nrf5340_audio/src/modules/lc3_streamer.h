/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LC3_STREAMER_H
#define LC3_STREAMER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

struct lc3_stream_cfg {
	uint32_t sample_rate_hz;
	uint32_t bit_rate_bps;
	uint32_t frame_duration_us;
};

/**
 * @brief Get the next frame for the stream.
 *
 * @details	Populates a pointer to the buffer holding the next frame. This buffer is valid until
 *		the next call to this function. or stream is closed.
 *
 * @param[in]	streamer_idx	Index of the streamer to get the next frame from.
 * @param[out]	frame_buffer	Pointer to the buffer holding the next frame.
 *
 * @retval 0		Success.
 * @retval -EINVAL	Invalid streamer index.
 * @retval -ENODATA	No more frames to read, call lc3_streamer_end_stream to clean context.
 * @retval -EFAULT	Module has not been initialized, or stream is not in a valid state. If
 *			stream has been playing an error has occurred preventing from further
 *			streaming. Call lc3_streamer_end_stream to clean context.
 */
int lc3_streamer_next_frame_get(const uint8_t streamer_idx, const uint8_t **const frame_buffer);

/**
 * @brief Verify that the LC3 header matches the stream configuration.
 *
 * @details	Verifies that the file is valid and can be used by the LC3 streamer.
 *		Since there is no standard header for LC3 files, the header might be different than
 *		what is defined in the struct lc3_file_header.
 *
 * @param[in]	filename	Name of the file to verify.
 * @param[in]	cfg		Stream configuration to compare against.
 *
 * @retval	true	Success.
 * @retval	false	Header is not matching the configuration.
 */
bool lc3_streamer_file_compatible_check(const char *const filename,
					const struct lc3_stream_cfg *const cfg);

/**
 * @brief Register a new stream that will be played by the LC3 streamer.
 *
 * @details Opens the specified file on the SD card, and prepares data so the stream can be
 *          started. When a frame is read by the application the next frame will be read from the
 *          file to ensure it's ready for streaming when needed.
 *
 * @param[in]   filename        Name of the file to open.
 * @param[out]  streamer_idx    Index of the streamer that was registered.
 * @param[in]   loop            If true, the stream will be looped when it reaches the end.
 *
 * @retval 0		Success.
 * @retval -EINVAL	Invalid filename or streamer_idx.
 * @retval -EAGAIN	No stream slot is available.
 * @retval -EFAULT	Module has not been initialized.
 */
int lc3_streamer_stream_register(const char *const filename, uint8_t *const streamer_idx,
				 const bool loop);

/**
 * @brief Get the number of active streams.
 *
 * @retval The number of streams currently playing.
 */
uint8_t lc3_streamer_num_active_streams(void);

/**
 * @brief Get the file path for a stream.
 *
 * @details If path buffer is smaller than the length of the actual path, the path will be
 *          truncated.
 *
 * @param[in]	streamer_idx	Index of the streamer.
 * @param[out]	path		Pointer for string to store file path in.
 * @param[in]	path_len	Length of string buffer.
 *
 * @retval	-EINVAL		Null pointers or invalid index given.
 * @retval	0		Success.
 */
int lc3_streamer_file_path_get(const uint8_t streamer_idx, char *const path, const size_t path_len);

/**
 * @brief Check if a stream is configured to loop.
 *
 * @param[in]	streamer_idx	Index of the streamer.
 *
 * @retval	true	Streamer is configured to loop.
 * @retval	false	Streamer is not configured to loop, or the streamer index is too high.
 */
bool lc3_streamer_is_looping(const uint8_t streamer_idx);

/**
 * @brief End a stream that's playing.
 *
 * @details Stops the streamer from playing the stream. Any open contexts will be closed, and any
 * active frame buffers will become invalid.
 *
 * @param[in]   streamer_idx    Index of the streamer to end.
 *
 * @retval 0		Success.
 * @retval -EINVAL	Invalid streamer index.
 */
int lc3_streamer_stream_close(const uint8_t streamer_idx);

/**
 * @brief Close all streams and drain the work queue.
 *
 * @retval -EFAULT	Module has not been initialized.
 * @retval 0		Success, other negative values are errors from k_work.
 */
int lc3_streamer_close_all_streams(void);

/**
 * @brief Initializes the LC3 streamer.
 *
 * @details Initializes the lc3_file module and the SD card driver. Initializes an internal work
 *          queue that is used to schedule fetching new frames from the SD card.
 *
 * @retval -EALREADY	Streamer is already initialized.
 * @retval 0		Success, other negative values are errors from lc3_file module.
 */
int lc3_streamer_init(void);

#endif /* LC3_STREAMER_H */
