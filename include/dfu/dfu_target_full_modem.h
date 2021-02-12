/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file dfu_target_full_modem.h
 *
 * @defgroup dfu_target_full_modem Full Modem DFU Target
 * @{
 * @brief DFU Target full modem updates.
 */

#ifndef DFU_TARGET_FULL_MODEM_H__
#define DFU_TARGET_FULL_MODEM_H__

#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <dfu/dfu_target.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dfu_target_fmfu_fdev {
	/* Flash device to use for storing the modem firmware. */
	const struct device *dev;

	/* Offset within the device to store the modem firmware. */
	off_t offset;

	/* Number of bytes to set aside in the device for storing the modem
	 * firmware. Set this to '0' to use all available flash (starting from
	 * `offset`.
	 */
	size_t size;
};

struct dfu_target_full_modem_params {
	/* Buffer for writes to `fmfu_dev`. */
	uint8_t *buf;

	/* Length of `buf` */
	size_t len;

	/* Flash device for storing the modem firmware */
	struct dfu_target_fmfu_fdev *dev;
};

/**
 * @brief Configure resources required by dfu_target_full_modem
 *
 * @param[in] params Pointer to dfu target full modem update parameters.
 *
 * @retval non-negative integer if successful, otherwise negative errno.
 */
int dfu_target_full_modem_cfg(const struct dfu_target_full_modem_params *params);

/**
 * @brief See if data in buf indicates a full modem update.
 *
 * @param[in] buf Pointer to data to check.
 *
 * @retval true if data matches, false otherwise.
 */
bool dfu_target_full_modem_identify(const void *const buf);

/**
 * @brief Initialize dfu target, perform steps necessary to receive firmware.
 *
 * @param[in] file_size Size of the current file being downloaded.
 * @param[in] callback  Not in use. In place to be compatible with DFU target
 *                      API.
 *
 * @retval 0 If successful, negative errno otherwise.
 */
int dfu_target_full_modem_init(size_t file_size,
			       dfu_target_callback_t callback);

/**
 * @brief Get offset of firmware
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return 0 if success, otherwise negative value if unable to get the offset
 */
int dfu_target_full_modem_offset_get(size_t *offset);

/**
 * @brief Write firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_full_modem_write(const void *const buf, size_t len);

/**
 * @brief De-initialize resources and finalize firmware upgrade if successful.

 * @param[in] successful Indicate whether the firmware was successfully
 *            received.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_full_modem_done(bool successful);

#endif /* DFU_TARGET_FULL_MODEM_H__ */

/**@} */
