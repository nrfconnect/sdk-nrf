/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#define CHIP_CONFIG_CONTROLLER_MAX_ACTIVE_DEVICES 2
/* Use a default pairing code if one hasn't been provisioned in flash. */
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE 20202021
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR 0xF00
#define CHIP_DEVICE_CONFIG_SED_SLOW_POLLING_INTERVAL 2000_ms32
