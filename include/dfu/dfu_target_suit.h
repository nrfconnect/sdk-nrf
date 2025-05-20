/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file dfu_target_suit.h
 *
 * @defgroup dfu_target_suit SUIT DFU Target
 * @{
 * @brief DFU Target for upgrades performed by SUIT
 */

#ifndef DFU_TARGET_SUIT_H__
#define DFU_TARGET_SUIT_H__

#include <stddef.h>
#include <dfu/dfu_target.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set buffer to use for flash write operations.
 *
 * @retval Non-negative value if successful, negative errno otherwise.
 */
int dfu_target_suit_set_buf(uint8_t *buf, size_t len);

/**
 * @brief Initialize the DFU target for the specific image and  perform the necessary steps to
 * receive the firmware.
 *
 * If you call this function, you must call dfu_target_suit_done() to finalize the firmware upgrade
 * before initializing any other images.
 *
 * @param[in] file_size Size of the current file being downloaded.
 * @param[in] img_num Image index.
 * @param[in] cb Callback for signaling events(unused).
 *
 * @return 0 If successful.
 * @return -ENODEV errno code if the buffer has not been initialized.
 * @return -ENXIO errno code if the partition dedicated for provided image is not found.
 * @return -ENOMEM errno code if the buffer is not large enough.
 * @return -EFAULT errno code if flash device assigned to the image is not available on the device.
 * @return -EBUSY errno code if the any image is already initialized and stream flash is in use.
 * @return other negative errno code if the initialization failed.
 */
int dfu_target_suit_init(size_t file_size, int img_num, dfu_target_callback_t cb);

/**
 * @brief Get offset of firmware
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @retval 0 If success. Otherwise, a negative value if unable to get the offset.
 */
int dfu_target_suit_offset_get(size_t *offset);

/**
 * @brief Write firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return 0 If successful.
 * @return -EFAULT errno code if the stream flash has not been initialized for any dfu image.
 * @return other negative errno code if the initialization failed.
 */
int dfu_target_suit_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize resources and finalize firmware upgrade if successful.

 * @param[in] successful Indicate whether the firmware was successfully recived.
 *
 * @retval 0 on success, negative errno otherwise.
 */
int dfu_target_suit_done(bool successful);

/**
 * @brief Schedule an update.
 *
 * This call processes SUIT envelope and requests images update.
 *
 * Firmware update will start after the device reboot.
 * You can reboot device, for example, by calling dfu_target_suit_reboot().
 *
 * @param[in] img_num Given image pair index or -1 for all
 *		      of image pair indexes.
 *
 * @retval 0 Successful request.
 * @retval negative_errno_code Negative error
 *	   code indicating the reason of failure.
 **/
int dfu_target_suit_schedule_update(int img_num);

/**
 * @brief Release resources and erase the download area.
 *
 * Cancels any ongoing updates.
 *
 * @retval negative_errno_code Negative error
 *	   code indicating the reason of failure.
 */
int dfu_target_suit_reset(void);

/**
 * @brief Reboot the device and apply the new image.
 *
 * The reboot can be delayed by setting the
 * CONFIG_DFU_TARGET_REBOOT_RESET_DELAY_MS Kconfig option value to the desired delay value.
 *
 * @retval negative_errno_code Negative error
 *	   code indicating the reason of failure.
 */
int dfu_target_suit_reboot(void);

#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_SUIT_H__ */

/**@} */
