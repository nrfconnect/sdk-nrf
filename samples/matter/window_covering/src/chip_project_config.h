/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
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

#ifdef CONFIG_CHIP_THREAD_SSED
#define CHIP_DEVICE_CONFIG_SED_ACTIVE_INTERVAL 500_ms32
#define CHIP_DEVICE_CONFIG_SED_IDLE_INTERVAL 500_ms32
#else
#define CHIP_DEVICE_CONFIG_SED_IDLE_INTERVAL 2000_ms32
#endif
