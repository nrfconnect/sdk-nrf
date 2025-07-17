/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file dfu_target_custom.h
 * @defgroup dfu_target_custom Custom DFU Target
 * @{
 * @brief Custom DFU (Device Firmware Update) target implementation.
 *
 * This file contains the function declarations for a custom DFU target implementation.
 * It provides function prototypes for identifying, initializing, writing, and finalizing a custom
 * firmware update process.
 */

#ifndef DFU_TARGET_CUSTOM_H__
#define DFU_TARGET_CUSTOM_H__

#include <dfu/dfu_target.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Check if the provided buffer contains a custom firmware image.
 *
 * @param[in] buf Pointer to the buffer containing the potential firmware image.
 * @retval true if the buffer contains a valid custom firmware image, false otherwise.
 */
bool dfu_target_custom_identify(const void *const buf);

/**
 * @brief Initialize the custom DFU target.
 *
 * @param[in] file_size Size of the firmware file to be written.
 * @param[in] img_num Image number for multi-image DFU.
 * @param[in] cb Callback function to be called during the DFU process.
 * @retval 0 on success, negative errno code on failure.
 */
int dfu_target_custom_init(size_t file_size, int img_num, dfu_target_callback_t cb);

/**
 * @brief Get the current write offset for the custom DFU target.
 *
 * @param[out] offset Pointer to store the current write offset.
 * @retval 0 on success, negative errno code on failure.
 */
int dfu_target_custom_offset_get(size_t *offset);

/**
 * @brief Write data to the custom DFU target.
 *
 * @param[in] buf Pointer to the buffer containing the data to be written.
 * @param[in] len Length of the data to be written.
 * @retval 0 on success, negative errno code on failure.
 */
int dfu_target_custom_write(const void *const buf, size_t len);

/**
 * @brief Release resources and finalize the custom DFU process if successful.
 *
 * @param[in] successful True if the DFU process was successful, false otherwise.
 * @retval 0 on success, negative errno code on failure.
 */
int dfu_target_custom_done(bool successful);

/**
 * @brief Schedule an update for the custom DFU target.
 *
 * @param[in] img_num Image number for multi-image DFU.
 * @retval 0 on success, negative errno code on failure.
 */
int dfu_target_custom_schedule_update(int img_num);

/**
 * @brief Release resources and erase the download area.
 *
 * Cancel any ongoing updates.
 *
 * @retval 0 on success, negative errno code on failure.
 */
int dfu_target_custom_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_CUSTOM_H__*/
/**@} */
