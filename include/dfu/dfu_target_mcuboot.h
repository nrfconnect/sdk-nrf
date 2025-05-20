/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file dfu_target_mcuboot.h
 *
 * @defgroup dfu_target_mcuboot MCUBoot DFU Target
 * @{
 * @brief DFU Target for upgrades performed by MCUBoot
 */

#ifndef DFU_TARGET_MCUBOOT_H__
#define DFU_TARGET_MCUBOOT_H__

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
int dfu_target_mcuboot_set_buf(uint8_t *buf, size_t len);

/**
 * @brief See if data in buf indicates MCUBoot style upgrade.
 *
 * @retval true if data matches, false otherwise.
 */
bool dfu_target_mcuboot_identify(const void *const buf);

/**
 * @brief Initialize dfu target, perform steps necessary to receive firmware.
 *
 * @param[in] file_size Size of the current file being downloaded.
 * @param[in] img_num Image pair index.
 * @param[in] cb Callback for signaling events(unused).
 *
 * @retval 0 If successful, negative errno otherwise.
 */
int dfu_target_mcuboot_init(size_t file_size, int img_num, dfu_target_callback_t cb);

/**
 * @brief Get offset of firmware
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return 0 if success, otherwise negative value if unable to get the offset
 */
int dfu_target_mcuboot_offset_get(size_t *offset);

/**
 * @brief Write firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_mcuboot_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize resources and finalize firmware upgrade if successful.

 * @param[in] successful Indicate whether the firmware was successfully recived.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_mcuboot_done(bool successful);

/**
 * @brief Schedule update of one or more images.
 *
 * This call requests images update. The update will be performed after
 * the device reset.
 *
 * @param[in] img_num Given image pair index or -1 for all
 *		      of image pair indexes.
 *
 * @return 0 for a successful request or a negative error
 *	   code identicating reason of failure.
 **/
int dfu_target_mcuboot_schedule_update(int img_num);

/**
 * @brief Release resources and erase the download area.
 *
 * Cancels any ongoing updates.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_mcuboot_reset(void);


#ifdef __cplusplus
}
#endif

#endif /* DFU_TARGET_MCUBOOT_H__ */

/**@} */
