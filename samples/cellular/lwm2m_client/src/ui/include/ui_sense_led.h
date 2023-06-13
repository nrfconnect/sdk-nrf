/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UI_SENSE_LED_H__
#define UI_SENSE_LED_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Turn the sense LEDs on or off.
 *
 * @param[in] new_state The LEDs' new state.
 * @return int 0 if successful, negative error code if not.
 */
int ui_sense_led_on_off(bool new_state);

/**
 * @brief Initialize the sense LEDs.
 *
 * @return int 0 if successful, negative error code if not.
 */
int ui_sense_led_init(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_SENSE_LED_H__ */
