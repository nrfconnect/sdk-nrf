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
bool bl_validate_firmware_local(u32_t fw_address,
				const struct fw_info *fwinfo);

#endif /* BL_VALIDATION_H__ */
