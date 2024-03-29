/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief SUIT processing reports storage
 *
 * @details This module implements a SUIT storage submodule, allowing to save and read binary
 *          data, that represent SUIT processing report.
 *          This module does not interpret or validate the report data format.
 *          The main purpose is to detect if the report is present, and return the raw binary
 *          if found.
 *          The boot report presence can be interpreted as the recovery flag, thus it is allowed to
 *          store empty reports. In such case - report read will succeed and can be interpreted as
 *          indication of set recovery flag.
 *          There is no need to configure this area as part of the device setup procedure.
 *          The location of the report binaries inside the NVM memory is provided by the main SUIT
 *          storage module and passed to this module as part of the initialization procedure.
 */

#ifndef SUIT_STORAGE_REPORT_INTERNAL_H__
#define SUIT_STORAGE_REPORT_INTERNAL_H__

#include <stdint.h>
#include <stddef.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SUIT storage module managing processing reports.
 *
 * @retval SUIT_PLAT_SUCCESS           if successfully initialized.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 */
suit_plat_err_t suit_storage_report_internal_init(void);

/**
 * @brief Erase the report area.
 *
 * @param[in]  area_addr  Address of erase-block aligned memory containing report binary.
 * @param[in]  area_size  Size of erase-block aligned memory, containing report binary.
 *
 * @retval SUIT_PLAT_SUCCESS           if area was successfully erased.
 * @retval SUIT_PLAT_ERR_INVAL         if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_HW_NOT_READY  if NVM controller is unavailable.
 * @retval SUIT_PLAT_ERR_IO            if unable to change NVM contents.
 */
suit_plat_err_t suit_storage_report_internal_clear(uint8_t *area_addr, size_t area_size);

/**
 * @brief Save the binary data representing the processing report.
 *
 * @param[in]  area_addr  Address of erase-block aligned memory containing report binary.
 * @param[in]  area_size  Size of erase-block aligned memory, containing report binary.
 * @param[in]  buf        Binary data representing report to save.
 * @param[in]  len        Length of the binary data.
 *
 * @retval SUIT_PLAT_SUCCESS            if the value was successfully saved.
 * @retval SUIT_PLAT_ERR_INVAL          if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_SIZE           if binary data is too long.
 * @retval SUIT_PLAT_ERR_OUT_OF_BOUNDS  if the index value is too big.
 * @retval SUIT_PLAT_ERR_HW_NOT_READY   if NVM controller is unavailable.
 * @retval SUIT_PLAT_ERR_IO             if unable to change NVM contents.
 */
suit_plat_err_t suit_storage_report_internal_save(uint8_t *area_addr, size_t area_size,
						  const uint8_t *buf, size_t len);

/**
 * @brief Read the binary data representing the processing report.
 *
 * @param[in]   area_addr  Address of erase-block aligned memory containing report binary.
 * @param[in]   area_size  Size of erase-block aligned memory, containing report binary.
 * @param[in]   index      Index of the report slot.
 * @param[out]  buf        Pointer to the binary data representing the report.
 * @param[out]  len        Length of the report.
 *
 * @retval SUIT_PLAT_SUCCESS            if the value was successfully read.
 * @retval SUIT_PLAT_ERR_INVAL          if one of the input arguments is invalid (i.e. NULL).
 * @retval SUIT_PLAT_ERR_OUT_OF_BOUNDS  if the index value is too big.
 * @retval SUIT_PLAT_ERR_NOT_FOUND      if update report is not found.
 */
suit_plat_err_t suit_storage_report_internal_read(const uint8_t *area_addr, size_t area_size,
						  const uint8_t **buf, size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_STORAGE_REPORT_INTERNAL_H__ */
