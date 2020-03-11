/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file dfu_target_modem.h
 *
 * @defgroup dfu_target_modem Modem DFU Target
 * @{
 * @brief DFU Target for upgrades performed by Modem
 */

#ifndef DFU_TARGET_MODEM_H__
#define DFU_TARGET_MODEM_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief See if data in buf indicates modem upgrade.
 *
 * @retval true if data matches, false otherwise.
 */
bool dfu_target_modem_identify(const void *const buf);

/**
 * @brief Initialize dfu target, perform steps necessary to receive firmware.
 *
 * @param[in] file_size Size of the current file being downloaded.
 * @param[in] callback Callback function for signaling if the modem is not able
 *		       to service the erase request.
 *
 * @retval 0 If successful, negative value otherwise.
 */
int dfu_target_modem_init(size_t file_size, dfu_target_callback_t callback);

/**
 * @brief Get offset of firmware
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return 0 if success, otherwise negative value if unable to get the offset
 */
int dfu_target_modem_offset_get(size_t *offset);

/**
 * @brief Write firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_modem_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize resources and finalize firmware upgrade if successful.
 *
 * @param[in] successful Indicate whether the firmware was successfully recived.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_modem_done(bool successful);

#endif /* DFU_TARGET_MODEM_H__ */

/**@} */
