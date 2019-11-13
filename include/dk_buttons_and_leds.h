/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef DK_BUTTON_AND_LEDS_H__
#define DK_BUTTON_AND_LEDS_H__

/** @file dk_buttons_and_leds.h
 * @brief Module for handling buttons and LEDs on Nordic DKs.
 * @defgroup dk_buttons_and_leds DK buttons and LEDs
 * @{
 */

#include <zephyr/types.h>
#include <sys/slist.h>

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
 * @brief Callback that is executed when a button state change is detected.
 *
 * @param button_state Bitmask of button states.
 * @param has_changed Bitmask that shows which buttons have changed.
 */
typedef void (*button_handler_t)(u32_t button_state, u32_t has_changed);

/** Button handler list entry. */
struct button_handler {
	button_handler_t cb; /**< Callback function. */
	sys_snode_t node; /**< Linked list node, for internal use. */
};

/** @brief Initialize the library to control the LEDs.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int dk_leds_init(void);

/** @brief Initialize the library to read the button state.
 *
 *  @param  button_handler Callback handler for button state changes.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int dk_buttons_init(button_handler_t button_handler);

/** @brief Add a dynamic button handler callback.
 *
 * In addition to the button handler function passed to
 * @ref dk_buttons_init, any number of button handlers can be added and removed
 * at runtime.
 *
 * @param[in] handler Handler structure. Must point to statically allocated
 * memory.
 */
void dk_button_handler_add(struct button_handler *handler);

/** @brief Remove a dynamic button handler callback.
 *
 * @param[in] handler Handler to remove.
 *
 * @retval 0 Successfully removed the handler.
 * @retval -ENOENT This button handler was not present.
 */
int dk_button_handler_remove(struct button_handler *handler);

/** @brief Read current button states.
 *
 *  @param button_state Bitmask of button states.
 *  @param has_changed Bitmask that shows which buttons have changed.
 */
void dk_read_buttons(u32_t *button_state, u32_t *has_changed);

/** @brief Get current button state from internal variable.
 *
 *  @return Bitmask of button states.
 */
u32_t dk_get_buttons(void);

/** @brief Set value of LED pins as specified in one bitmask.
 *
 *  @param  leds Bitmask that defines which LEDs to turn on and off.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int dk_set_leds(u32_t leds);


/** @brief Set value of LED pins as specified in two bitmasks.
 *
 *  @param  leds_on_mask  Bitmask that defines which LEDs to turn on.
 *                        If this bitmask overlaps with @p leds_off_mask,
 *                        @p leds_on_mask has priority.
 *
 *  @param  leds_off_mask Bitmask that defines which LEDs to turn off.
 *                        If this bitmask overlaps with @p leds_on_mask,
 *                        @p leds_on_mask has priority.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int dk_set_leds_state(u32_t leds_on_mask, u32_t leds_off_mask);

/** @brief Set a single LED value.
 *
 *  This function turns a single LED on or off.
 *
 *  @param led_idx Index of the LED.
 *  @param val     Value for the LED: 1 - turn on, 0 - turn off
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 *
 *  @sa dk_set_led_on, dk_set_led_off
 */
int dk_set_led(u8_t led_idx, u32_t val);

/** @brief Turn a single LED on.
 *
 *  @param led_idx Index of the LED.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int dk_set_led_on(u8_t led_idx);

/** @brief Turn a single LED off.
 *
 *  @param led_idx Index of the LED.
 *
 *  @retval 0           If the operation was successful.
 *                      Otherwise, a (negative) error code is returned.
 */
int dk_set_led_off(u8_t led_idx);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DK_BUTTON_AND_LEDS_H__ */
