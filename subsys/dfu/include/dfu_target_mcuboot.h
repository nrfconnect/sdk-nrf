/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Find correct MCUBoot update file path entry in space separated string.
 *
 * When supporting upgrades of the MCUBoot bootloader, two variants of the new
 * firmware is presented, each linked against a different address.
 * This function is told what slot is active, and sets the update pointer to
 * point to the correct none active entry in the file path.
 *
 * @param[in, out] file      pointer to file path with space separator. Note
 *                           that the space separator can be replaced by a NULL
 *                           terminator.
 * @param[in]      s0_active bool indicating if S0 is the currently active slot.
 * @param[out]     update    pointer to correct file MCUBoot bootloader upgrade.
 *                           Will be set to NULL if no space separator is found.
 *
 * @retval 0 If successful (note that this does not imply that a space
 *           separator was found) negative errno otherwise.
 */
int dfu_ctx_mcuboot_set_b1_file(const char *file, bool s0_active,
				const char **update);

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
 * @param[in] cb Callback for signaling events(unused).
 *
 * @retval 0 If successful, negative errno otherwise.
 */
int dfu_target_mcuboot_init(size_t file_size, dfu_target_callback_t cb);

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

#endif /* DFU_TARGET_MCUBOOT_H__ */

/**@} */
