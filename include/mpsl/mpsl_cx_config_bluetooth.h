/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mpsl_cx_config_bluetooth.h
 *
 * @defgroup mpsl_cx_bluetooth MPSL Coexistence configuration for Bluetooth Radio Coexistence
 * @ingroup  mpsl_cx
 *
 * @{
 */

#ifndef MPSL_CX_CONFIG_BT_H__
#define MPSL_CX_CONFIG_BT_H__

#include <stdint.h>

/** @brief Configures the Bluetooth Radio One Wire Coexistence interface.
 *
 * This function sets device interface parameters for the Coexistence module.
 * The module is used to control PTA interface through the given pins and resources.
 *
 * @retval   0             Coexistence interface successfully configured.
 * @retval   -NRF_EPERM    Coexistence interface is not available.
 *
 */

int32_t mpsl_cx_bt_interface_1wire_config_set(void);

/** @brief Configures the Bluetooth Radio Three Wire Coexistence interface.
 *
 * This function sets device interface parameters for the Coexistence module.
 * The module is used to control PTA interface through the given pins and resources.
 *
 * @retval   0             Coexistence interface successfully configured.
 * @retval   -NRF_EPERM    Coexistence interface is not available.
 *
 */

int32_t mpsl_cx_bt_interface_3wire_config_set(void);

#endif

/**@} */
