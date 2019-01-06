/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef DK_BUTTON_AND_LEDS_H__
#define DK_BUTTON_AND_LEDS_H__

/** @file dk_buttons_and_leds.h
 * @brief Module to handle buttons and LEDs on Nordic DKs.
 * @defgroup dk_buttons_and_leds DK buttons and leds
 * @{
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DK_NO_LEDS_MSK    (0)
#define DK_LED1           0
#define DK_LED2           1
#define DK_LED3           2
#define DK_LED4           3
#define DK_LED1_MSK       BIT(DK_LED1)
#define DK_LED2_MSK       BIT(DK_LED2)
#define DK_LED3_MSK       BIT(DK_LED3)
#define DK_LED4_MSK       BIT(DK_LED4)
#define DK_ALL_LEDS_MSK   (DK_LED1_MSK | DK_LED2_MSK |\
			   DK_LED3_MSK | DK_LED4_MSK)

#define DK_NO_BTNS_MSK   (0)
#define DK_BTN1          0
#define DK_BTN2          1
#define DK_BTN3          2
#define DK_BTN4          3
#define DK_BTN1_MSK      BIT(DK_BTN1)
#define DK_BTN2_MSK      BIT(DK_BTN2)
#define DK_BTN3_MSK      BIT(DK_BTN3)
#define DK_BTN4_MSK      BIT(DK_BTN4)
#define DK_ALL_BTNS_MSK  (DK_BTN1_MSK | DK_BTN2_MSK | \
			  DK_BTN2_MSK | DK_BTN3_MSK)
/**
 * @typedef button_handler_t
 * @brief Callback executed when button state change is detected
 *
 * @param button_state Bitmask of button states
 * @param has_changed Bitmask that shows wich buttons that have changed.
 */
typedef void (*button_handler_t)(u32_t button_state, u32_t has_changed);

/** @brief Initialize the leds library.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int dk_leds_init(void);

/** @brief Initialize the buttons library.
 *
 *  @param  button_handler Callback handler for button state changes.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int dk_buttons_init(button_handler_t button_handler);

/** @brief Read current button states
 *
 *  @param button_state Bitmask of button states
 *  @param has_changed Bitmask that shows wich buttons that have changed.
 */
void dk_read_buttons(u32_t *button_state, u32_t *has_changed);

/** @brief Get current buttons state
 *
 *  Get buttons state from internal variable.
 *
 *  @return Bibmask of button states.
 */
u32_t dk_get_buttons(void);

/** @brief Set value of LED pins
 *
 *  @param  leds Bitmask that that will turn on and of the LEDs
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int dk_set_leds(u32_t leds);


/** @brief Set value of LED pins.
 *
 *  @param  leds_on_mask Bitmask that that will turn on the corresponding
 *                       LEDs. In cases where it overlaps with
 *                       leds_off_mask, the leds_on_mask will have priority.
 *
 *  @param  leds_off_mask Bitmask that that will turn off the corresponding
 *                        LEDs. In cases where it overlaps with
 *                        leds_on_mask, the leds_on_mask will have priority.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int dk_set_leds_state(u32_t leds_on_mask, u32_t leds_off_mask);

/** @brief Set single LED value
 *
 *  Turns single LED on or off.
 *
 *  @param led_idx Index of the LED
 *  @param val     Value for the single LED: 1 - turn on, 0 - turn off
 *
 *  @return Zero on success or (negative) error code otherwise.
 *
 *  @sa dk_set_led_on
 *  @sa dk_set_led_off
 */
int dk_set_led(u8_t led_idx, u32_t val);

/** @brief Set single LED on
 *
 *  @param led_idx Index of the LED
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int dk_set_led_on(u8_t led_idx);

/** @brief Set single LED off
 *
 *  @param led_idx Index of the LED
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int dk_set_led_off(u8_t led_idx);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DK_BUTTON_AND_LEDS_H__ */
