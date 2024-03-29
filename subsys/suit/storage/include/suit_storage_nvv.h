/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Manifest Non-volatile variables storage
 *
 * @details This module implements a SUIT storage submodule, allowing to access non-volatile
 *          variables (NVV) from the SUIT manifest sequences.
 *          This module implements a functionality to initialize the area with variables.
 *          There is no need to configure this area as part of the device setup procedure.
 *          The location of the NVV structure inside the NVM memory is provided by the main SUIT
 *          storage module and passed to this module as part of the initialization procedure.
 */

#ifndef SUIT_STORAGE_NVV_H__
#define SUIT_STORAGE_NVV_H__

#include <stdint.h>
#include <stddef.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SUIT_STORAGE_NVV_N_VARS 8

typedef uint32_t suit_storage_nvv_t[SUIT_STORAGE_NVV_N_VARS];

/**
 * @brief Initialize the SUIT storage module managing Manifest Non-volatile variables.
 *
 * @details If the memory is erased or invalid, this function will not initialize it with default
 *          values. The area can be filled with default values using @ref suit_storage_nvv_erase
 *
 * @retval SUIT_PLAT_SUCCESS           if successfully initialized.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 */
suit_plat_err_t suit_storage_nvv_init(void);

/**
 * @brief Erase the NVV area.
 *
 * @param[in]  area_addr  Address of erase-block aligned memory containing variables.
 * @param[in]  area_size  Size of erase-block aligned memory, containing variables.
 *
 * @retval SUIT_PLAT_SUCCESS           if NVVs were successfully erased.
 * @retval SUIT_PLAT_ERR_INVAL         if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_SIZE          unable to initialize NVVs inside the provided area.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 * @retval SUIT_PLAT_ERR_IO            if unable to change NVM contents.
 */
suit_plat_err_t suit_storage_nvv_erase(uint8_t *area_addr, size_t area_size);

/**
 * @brief Get the value of manifest non-volatile variable.
 *
 * @param[in]   area_addr  Address of erase-block aligned memory containing variables.
 * @param[in]   area_size  Size of erase-block aligned memory, containing variables.
 * @param[in]   index      Index of the variable.
 * @param[out]  value      Pointer to store the value.
 *
 * @retval SUIT_PLAT_SUCCESS        if the value was successfully read.
 * @retval SUIT_PLAT_ERR_INVAL      if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_SIZE       unable to read NVVs from the provided area.
 * @retval SUIT_PLAT_ERR_NOT_FOUND  if the index value is too big.
 */
suit_plat_err_t suit_storage_nvv_get(const uint8_t *area_addr, size_t area_size, size_t index,
				     uint32_t *value);

/**
 * @brief Set the value of manifest non-volatile variable.
 *
 * @param[in]  area_addr  Address of erase-block aligned memory containing variables.
 * @param[in]  area_size  Size of erase-block aligned memory, containing variables.
 * @param[in]  index      Index of the variable.
 * @param[in]  value      Value to set.
 *
 * @retval SUIT_PLAT_SUCCESS           if the value was successfully updated.
 * @retval SUIT_PLAT_ERR_INVAL         if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_SIZE          unable to store NVVs inside the provided area.
 * @retval SUIT_PLAT_ERR_NOT_FOUND     if the index value is too big.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 * @retval SUIT_PLAT_ERR_IO            if unable to change NVM contents.
 */
suit_plat_err_t suit_storage_nvv_set(uint8_t *area_addr, size_t area_size, size_t index,
				     uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_STORAGE_NVV_H__ */
