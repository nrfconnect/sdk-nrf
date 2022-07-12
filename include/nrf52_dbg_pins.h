/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF52_DBG_PINS_H__
#define NRF52_DBG_PINS_H__

#include <zephyr/types.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NRF52_DBG_PIN_1 0 // Pin P0.13
#define NRF52_DBG_PIN_2 1 // Pin P0.14
#define NRF52_DBG_PIN_3 2 // Pin P0.15
#define NRF52_DBG_PIN_4 3 // Pin P0.16
#define NRF52_DBG_PIN_5 4 // Pin P0.17
#define NRF52_DBG_PIN_6 5 // Pin P0.18
#define NRF52_DBG_PIN_7 6 // Pin P0.19
#define NRF52_DBG_PIN_8 7 // Pin P0.20

/** @brief Initialize the library to control the debug pins.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int nrf52_dbg_pins_init(void);

/** @brief Write debug pin port.
 *
 *  @param mask Mask value to write (LSB = NRF52_DBG_PIN_1).
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int nrf52_dbg_pin_port_set(uint8_t mask);

/** @brief Set a single debug pin.
 *
 *  @param dgb_pin_idx Index of the debug pin.
 *  @param onoff       Boolean value to set on pin.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int nrf52_dbg_pin_set(uint8_t dgb_pin_idx, bool onoff);

/** @brief Turn a single debug pin on.
 *
 *  @param dgb_pin_idx Index of the debug pin.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int nrf52_dbg_pin_on(uint8_t dgb_pin_idx);

/** @brief Turn a single debug pin off.
 *
 *  @param dgb_pin_idx Index of the debug pin.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int nrf52_dbg_pin_off(uint8_t dgb_pin_idx);

/** @brief Spike a single debug pin.
 *
 *  @param dgb_pin_idx Index of the debug pin.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int nrf52_dbg_pin_spike(uint8_t dgb_pin_idx);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NRF52_DBG_PINS_H__ */
