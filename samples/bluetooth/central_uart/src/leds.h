/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LEDS_H_
#define LEDS_H_

#include <stdbool.h>

/**
 * @brief Initialize status LEDs (if configured).
 *
 * The status LEDs may be enabled by setting the following devicetree chosen properties:
 * - nordic,central-uart-ble-led: BLE activity LED
 * - nordic,central-uart-log-led: Log activity LED
 */
void leds_init(void);

/**
 * @brief Drive BLE LED on when connected, off when disconnected.
 */
void leds_set_ble_connected(bool connected);

/**
 * @brief Indicate BLE traffic (RX or TX).
 *
 * While connected, briefly blinks the BLE LED to indicate traffic.
 */
void leds_indicate_ble_traffic(void);

#endif /* LEDS_H_ */
