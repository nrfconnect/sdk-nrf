/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UI_LED_H__
#define UI_LED_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BOARD_THINGY91_NRF9160_NS)
#define NUM_LEDS	3
#elif defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS) || defined(CONFIG_BOARD_NRF9161DK_NRF9161_NS)
#if defined(CONFIG_UI_LED_USE_PWM)
#define NUM_LEDS	1
#else
#define NUM_LEDS	4
#endif
#endif

/**
 * @brief Turn the LED on or off. If false the
 *        LEDs will be dark regardless of colour value.
 *
 * @param led_id The ID of the LED. [0, 3].
 * @param new_state The LED's new state.
 * @return int 0 if successful, negative error code if not.
 */
int ui_led_pwm_on_off(uint8_t led_id, bool new_state);

/**
 * @brief Set the intensity of the LED.
 *
 * @param led_id The ID of the LED. [0, 3].
 * @param[in] led_intensity Integer between [0, 255], describing
 *            a percentage of the maximum LED intensity.
 * @return int 0 if successful, negative error code if not.
 */
int ui_led_pwm_set_intensity(uint8_t led_id, uint8_t led_intensity);

/**
 * @brief Initialize the LEDs to use PWM.
 *
 * @return int 0 if successful, negative error code if not.
 */
int ui_led_pwm_init(void);

/**
 * @brief Turn the LED on or off. If false the
 *        LEDs will be dark regardless of colour value.
 *
 * @param led_id The ID of the LED. [0, 3].
 * @param new_state The LED's new state.
 * @return int 0 if successful, negative error code if not.
 */
int ui_led_gpio_on_off(uint8_t led_id, bool new_state);

/**
 * @brief Initialize the LEDs to use GPIO.
 *
 * @return int 0 if successful, negative error code if not.
 */
int ui_led_gpio_init(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_LED_H__ */
