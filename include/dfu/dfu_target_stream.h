/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file dfu_target_stream.h
 *
 * @defgroup dfu_target_stream Flash stream DFU Target
 * @{
 * @brief Provides an API for other DFU targets that need to write a large
 *        object to flash.
 */

#ifndef DFU_TARGET_STREAM_H__
#define DFU_TARGET_STREAM_H__

#include <stddef.h>
#include <zephyr/storage/stream_flash.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stream_flash_ctx *dfu_target_stream_get_stream(void);

/** @brief DFU target stream initialization structure. */
struct dfu_target_stream_init {
	/* The identifier of the stream, used for storing settings.*/
	const char *id;

	/* Flash device that the stream should be written to. */
	const struct device *fdev;

	/* Buffer used for stream flash writing. The length of the
	 * buffer must be smaller than a page size of `fdev`.
	 */
	uint8_t *buf;

	/* Length of `buf` */
	size_t len;

	/* Offset within `fdev` to write the stream to. */
	size_t offset;

	/* The number of bytes within fdev to use for the stream.
	 * Set to 0 to use all available memory in the device
	 * (starting from `offset`).
	 */
	size_t size;

	/* Callback invoked upon successful flash write operations. This
	 * can be used to inspect the actual written data.
	 */
	stream_flash_callback_t cb;
};

/**
 * @brief Initialize dfu target.
 *
 * @param[in] init Init parameters
 *
 * @retval Non-negative value if successful, negative errno otherwise.
 */
int dfu_target_stream_init(const struct dfu_target_stream_init *init);

/**
 * @brief Get the offset within the payload of the next byte to download.
 *
 * This is used to pick up an aborted download. If for instance 0x1000 bytes
 * of the payload has been downloaded and stored to flash, this would return
 * 0x1000. For this function to work across reboots, the option
 * `CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS` must be set.
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return Non-negative value if success, otherwise negative value if unable
 *         to get the offset
 */
int dfu_target_stream_offset_get(size_t *offset);

/**
 * @brief Write a chunk of firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return Non-negative value on success, negative errno otherwise.
 */
int dfu_target_stream_write(const uint8_t *buf, size_t len);

/**
 * @brief Release resources and finalize stream flash write if successful.

 * @param[in] successful Indicate whether the firmware was successfully
 * received.
 *
 * @return Non-negative value on success, negative errno otherwise.
 */
int dfu_target_stream_done(bool successful);

/**
 * @brief Release resources and erase the download area.
 *
 * Cancels any ongoing updates.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_stream_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_STREAM_H__ */

/**@} */
