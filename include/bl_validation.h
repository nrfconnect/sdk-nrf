/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BL_VALIDATION_H__
#define BL_VALIDATION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <fw_info.h>

/** @defgroup bl_validation Bootloader firmware validation
 * @{
 */

/** Function for validating firmware.
 *
 * @details This will run a series of checks on the fw_info contents, then
 *          locate the validation info and check the signature of the image.
 *
 * @note If this function returns false, the fw_info struct can be invalidated
 *       by modifying its 'valid' field, causing any subsequent call to this
 *       function to quickly return false.
 *
 * @param[in] fw_address  Address the firmware is currently at.
 * @param[in] fwinfo      Firmware info structure belonging to the image.
 *
 * @retval  true   if the image is valid
 * @retval  false  if the image is invalid
 */
bool bl_validate_firmware(u32_t fw_dst_address, u32_t fw_src_address);

/* Typedef for use in EXT_API declaration */
typedef
bool (*bl_validate_firmware_t)(u32_t fw_dst_address, u32_t fw_src_address);


/** Function for validating firmware in place.
 *
 * @note This function is only available to the bootloader.
 *
 * @details See @ref bl_validate_firmware for more details.
 */
bool bl_validate_firmware_local(u32_t fw_address,
				const struct fw_info *fwinfo);


/**
 * @brief Structure describing the BL_VALIDATE_FW EXT_API.
 */
struct bl_validate_fw_ext_api {
	bl_validate_firmware_t bl_validate_firmware;
};

  /** @} */

#ifdef __cplusplus
}
#endif

#endif /* BL_VALIDATION_H__ */
