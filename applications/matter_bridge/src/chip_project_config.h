/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 *    @file
 *          Example project configuration file for CHIP.
 *
 *          This is a place to put application or project-specific overrides
 *          to the default configuration values for general CHIP features.
 *
 */

#pragma once

#define CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT CONFIG_BRIDGE_MAX_DYNAMIC_ENDPOINTS_NUMBER

/* Disable handling only single connection to avoid stopping Matter service BLE advertising
 * while other BLE bridge-related connections are open.
 */
#define CHIP_DEVICE_CONFIG_CHIPOBLE_SINGLE_CONNECTION 0
