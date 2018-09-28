/** @file
 *  @brief DIS sample for HID
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef __DIS_H
#define __DIS_H

/**
 * @file
 * @defgroup nrf_bt_dis BLE GATT Device Information Service API
 * @{
 * @brief API for the BLE GATT Device Information (DI) Service.
 *
 * This implementation is configured using Kconfig only.
 * No configuration arguments are given during initialization.
 *
 * Following characteristics are provided:
 * - Model Number String
 * - Manufacturer Name String
 * - PnP ID (optional)
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device Information Service initialization
 */
void dis_init(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* __DIS_H */
