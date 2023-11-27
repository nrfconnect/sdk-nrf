/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <lib/core/CHIPError.h>

namespace ThreadWifiSwitch
{
/**
 * Returns whether it's Thread and not Wi-Fi that is currently used as a Matter transport.
 */
bool IsThreadActive();

/**
 * Start either Thread or Wi-Fi Matter transport based on the current configuration.
 *
 * Initializes Matter components, such as network commissioning instance or DNS-SD
 * implementation, based on the active Matter transport.
 */
CHIP_ERROR StartCurrentTransport();

/**
 * Initiates switching between Thread and Wi-Fi.
 *
 * The switching involves resetting the device to its factory setting, setting a flag
 * indicating the active transport, and rebooting the device.
 */
void SwitchTransport();
} // namespace ThreadWifiSwitch
