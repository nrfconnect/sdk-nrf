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

/**
 * CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID
 *
 * 0x4E4C: nrfconnect light_bulb
 */
#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID 0x4E4C

/* Use a default pairing code if one hasn't been provisioned in flash. */
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE 12345678
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR 0xF00
