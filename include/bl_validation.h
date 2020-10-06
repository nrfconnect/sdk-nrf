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
#include <zephyr/types.h>

/** @defgroup bl_validation Bootloader firmware validation
 * @{
 */

/** Function for validating firmware.
 *
 * @details This will run a series of checks on the @p fw_src_address contents,
 *          then locate the validation info and check the signature of the
 *          image.
 *
 * @param[in] fw_dst_address  Address where the firmware will be written.
 * @param[in] fw_src_address  Address of the firmware to be validated.
 *
 * @retval  true   if the image is valid
 * @retval  false  if the image is invalid
 */
bool bl_validate_firmware(uint32_t fw_dst_address, uint32_t fw_src_address);

/* Typedef for use in EXT_API declaration */
typedef
bool (*bl_validate_firmware_t)(uint32_t fw_dst_address, uint32_t fw_src_address);

/** Whether bl_validate_firmware() is available.
 *
 * @details This is only relevant when
 *          @option{CONFIG_BL_VALIDATE_FW_EXT_API_OPTIONAL} is set.
 *
 * @retval true  bl_validate_firmware() can be called and should work correctly.
 * @retval false bl_validate_firmware() is unavailable and will always return
 *               false because the undelying EXT_API is unavailable.
 */
bool bl_validate_firmware_available(void);

/** Function for validating firmware in place.
 *
 * @note This function is only available to the bootloader.
 *
 * @details See @ref bl_validate_firmware for more details.
 */
bool bl_validate_firmware_local(uint32_t fw_address,
				const struct fw_info *fwinfo);


/**
 * @brief Structure describing the BL_VALIDATE_FW EXT_API.
 */
struct bl_validate_fw_ext_api {
	bl_validate_firmware_t bl_validate_firmware;
};

/** Write version and slot to monotonic counter.
 *
 * @details The version is left-shifted 1 bit, and the slot is place as the LSB.
 *
 * @param[in]  version  Firmware version.
 * @param[in]  slot     Slot where firmware is located. Must be 0 or 1.
 *
 * @return See @ref set_monotonic_counter.
 */
int set_monotonic_version(uint16_t version, uint16_t slot);


/** Read version and slot from monotonic counter.
 *
 * @param[out]  slot_out  Slot where firmware is located. Can be NULL.
 *
 * @return Firmware version
 */
uint16_t get_monotonic_version(uint16_t *slot_out);

  /** @} */

#ifdef __cplusplus
}
#endif

#endif /* BL_VALIDATION_H__ */
