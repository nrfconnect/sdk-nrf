/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BL_VALIDATION_H__
#define BL_VALIDATION_H__

/*
 * The FW package will consist of (firmware | (padding) | validation_info),
 * where the firmware contains the firmware_info at a predefined location. The
 * padding is present if the validation_info needs alignment. The
 * validation_info is not directly referenced from the firmware_info since the
 * validation_info doesn't actually have to be placed after the firmware.
 */

#include <stdbool.h>
#include <fw_info.h>

/* EXT_API ID for the bl_validate_firmware EXT_API. */
#define BL_VALIDATE_FW_EXT_API_ID 0x1101

EXT_API_FUNCTION(bool, bl_validate_firmware, u32_t fw_dst_address,
					u32_t fw_src_address);

struct bl_validate_fw_ext_api {
	struct fw_info_ext_api header;
	struct {
		bl_validate_firmware_t bl_validate_firmware;
	} ext_api;
};


/** Function for validating firmware in place.
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


bool bl_validate_firmware_local(u32_t fw_address,
				const struct fw_info *fwinfo);

#endif /* BL_VALIDATION_H__ */
