/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file dfu_target_flash.h
 *
 * @defgroup dfu_target_flash Flash DFU Target
 * @{
 * @brief DFU Target for upgrades written straight to flash devices.
 */

#ifndef DFU_TARGET_FLASH_H__
#define DFU_TARGET_FLASH_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set device configuration of dfu target flash.
 *
 * @retval 0 If successful, negative errno otherwise.
 */
int dfu_target_flash_cfg(const char *dev_name, size_t start, size_t end,
			 void *buf, size_t buf_len);

/**
 * @brief See if data in buf indicates flash device style upgrade.
 *
 * @retval true if data matches, false otherwise.
 */
bool dfu_target_flash_identify(const void *const buf);

/**
 * @brief Initialize dfu target, perform steps necessary to receive firmware.
 *
 * @param[in] file_size Size of the current file being downloaded.
 *
 * @retval 0 If successful, negative errno otherwise.
 */
int dfu_target_flash_init(size_t file_size);

/**
 * @brief Get offset of firmware
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return 0 if success, otherwise negative value if unable to get the offset
 */
int dfu_target_flash_offset_get(size_t *offset);

/**
 * @brief Write firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_flash_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize resources and finalize firmware upgrade if successful.

 * @param[in] successful Indicate whether the firmware was successfully recived.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_flash_done(bool successful);

#endif /* DFU_TARGET_FLASH_H__ */

/**@} */
