/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
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

/* Use a default pairing code if one hasn't been provisioned in flash. */
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE 20202021
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR 0xF00

/* Configure device configuration with exemplary data */
#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_NAME "Nordic"
#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_NAME "WeatherStation"
#define CHIP_DEVICE_CONFIG_DEFAULT_DEVICE_PRODUCT_REVISION_STRING                                  \
	"Prerelease weather station device"
#define CHIP_DEVICE_CONFIG_DEFAULT_DEVICE_PRODUCT_REVISION 1
#define CHIP_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION_STRING "Prerelease weather station firmware"
#define CHIP_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION 1
