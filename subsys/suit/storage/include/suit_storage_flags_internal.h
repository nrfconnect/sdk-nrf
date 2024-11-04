/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief SUIT boot flags storage
 *
 * @details This module implements a SUIT storage submodule, allowing to save and read boot
 *          flags.
 *          There is no need to configure this area as part of the device setup procedure.
 *          The location of the flags inside the NVM memory is provided by the main SUIT
 *          storage module and passed to this module as part of the initialization procedure.
 */

#ifndef SUIT_STORAGE_FLAGS_INTERNAL_H__
#define SUIT_STORAGE_FLAGS_INTERNAL_H__

#include <stdint.h>
#include <stddef.h>
#include <suit_plat_err.h>
#include <suit_storage.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SUIT_STORAGE_FLAGS_N 4

typedef uint32_t suit_storage_flags_t[SUIT_STORAGE_FLAGS_N];

/**
 * @brief Initialize the SUIT storage module managing boot flags.
 *
 * @retval SUIT_PLAT_SUCCESS           if successfully initialized.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 */
suit_plat_err_t suit_storage_flags_internal_init(void);

/**
 * @brief Clear one of the boot flags.
 *
 * @param[in]  area_addr  Address of erase-block aligned memory containing flags.
 * @param[in]  area_size  Size of erase-block aligned memory, containing flags.
 * @param[in]  flag       SUIT boot flag to clear.
 *
 * @retval SUIT_PLAT_SUCCESS           if flag was successfully cleared.
 * @retval SUIT_PLAT_ERR_INVAL         if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 * @retval SUIT_PLAT_ERR_IO            if unable to change NVM contents.
 */
suit_plat_err_t suit_storage_flags_internal_clear(uint8_t *area_addr, size_t area_size,
						  suit_storage_flag_t flag);

/**
 * @brief Set one of the boot flags.
 *
 * @param[in]  area_addr  Address of erase-block aligned memory containing flags.
 * @param[in]  area_size  Size of erase-block aligned memory, containing flags.
 * @param[in]  flag       SUIT boot flag to set.
 *
 * @retval SUIT_PLAT_SUCCESS           if flag was successfully set.
 * @retval SUIT_PLAT_ERR_INVAL         if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 * @retval SUIT_PLAT_ERR_IO            if unable to change NVM contents.
 */
suit_plat_err_t suit_storage_flags_internal_set(uint8_t *area_addr, size_t area_size,
						suit_storage_flag_t flag);

/**
 * @brief Check if one of the boot flags is set.
 *
 * @param[in]  area_addr  Address of erase-block aligned memory containing flags.
 * @param[in]  area_size  Size of erase-block aligned memory, containing flags.
 * @param[in]  flag       SUIT boot flag to be checked.
 *
 * @retval SUIT_PLAT_SUCCESS           if flag is set.
 * @retval SUIT_PLAT_ERR_NOT_FOUND     if flag is cleared.
 * @retval SUIT_PLAT_ERR_INVAL         if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 */
suit_plat_err_t suit_storage_flags_internal_check(const uint8_t *area_addr, size_t area_size,
						  suit_storage_flag_t flag);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_STORAGE_REPORT_INTERNAL_H__ */
