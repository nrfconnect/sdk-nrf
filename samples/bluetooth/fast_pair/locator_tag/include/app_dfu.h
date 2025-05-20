/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_DFU_H_
#define APP_DFU_H_

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/bluetooth/uuid.h>

/**
 * @defgroup fmdn_sample_dfu  Locator Tag sample Device Firmware Update (DFU) module
 * @brief Locator Tag sample Device Firmware Update (DFU) module
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Check if the GATT operations on the DFU GATT service are allowed.
 *
 *  When the DFU Kconfig is enabled, the sample statically registers the proprietary
 *  DFU GATT service and characteristics (SMP).
 *  Access to this service is allowed only when the device is in DFU mode.
 *
 *  The DFU mode timeout will be refreshed each time the device receives an
 *  authorized DFU GATT service operation.
 *
 *  @param uuid GATT characteristic UUID
 *  @return true if allowed, otherwise false
 */
bool app_dfu_bt_gatt_operation_allow(const struct bt_uuid *uuid);

/** Log the current firmware version. */
void app_dfu_fw_version_log(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* APP_DFU_H_ */
