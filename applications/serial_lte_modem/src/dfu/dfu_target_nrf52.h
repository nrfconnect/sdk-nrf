/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file dfu_target_nrf52.h
 *
 * @defgroup dfu_target_nrf52 NRF52 DFU Target
 * @{
 * @brief DFU Target for upgrades performed by NRF52 DFU
 */

#ifndef DFU_TARGET_NRF52_H__
#define DFU_TARGET_NRF52_H__

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
int dfu_target_nrf52_set_buf(uint8_t *buf, size_t len);

/**
 * @brief Initialize dfu target, perform steps necessary to receive firmware.
 *
 * @param[in] file_size Size of the current file being downloaded.
 *
 * @retval 0 If successful, negative errno otherwise.
 */
int dfu_target_nrf52_init(size_t file_size);

/**
 * @brief Get offset of firmware
 *
 * @param[out] offset Returns the offset of the firmware upgrade.
 *
 * @return 0 if success, otherwise negative value if unable to get the offset
 */
int dfu_target_nrf52_offset_get(size_t *offset);

/**
 * @brief Write firmware data.
 *
 * @param[in] buf Pointer to data that should be written.
 * @param[in] len Length of data to write.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_nrf52_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize resources and finalize firmware upgrade if successful.

 * @param[in] successful Indicate whether the firmware was successfully recived.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_target_nrf52_done(bool successful);

#endif /* DFU_TARGET_NRF52_H__ */

/**@} */
