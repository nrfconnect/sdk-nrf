/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup nrf5340_audio_button Audio Button Handler
 * @{
 * @brief Button handler API for nRF5340 Audio applications.
 *
 * This module provides button input handling and debouncing functionality for user
 * interface control. It supports multiple button configurations and provides
 * thread-safe button state detection with debouncing to prevent false triggers.
 * The module handles button press events for volume control, play/pause operations,
 * channel selection, and device configuration. It integrates with the application
 * state management through Zephyr's ZBUS messaging system to provide responsive
 * user interface control across all nRF5340 Audio application modes and device roles.
 */

#ifndef _BUTTON_HANDLER_H_
#define _BUTTON_HANDLER_H_

#include <stdint.h>
#include <zephyr/drivers/gpio.h>

/** @brief Initialize button handler, with buttons defined in button_assignments.h.
 *
 * @note This function may only be called once - there is no reinitialize.
 *
 * @return 0 if successful.
 * @return -ENODEV	gpio driver not found
 */
int button_handler_init(void);

/** @brief Check button state.
 *
 * @param[in] button_pin Button pin
 * @param[out] button_pressed Button state. True if currently pressed, false otherwise
 *
 * @return 0 if success, an error code otherwise.
 */
int button_pressed(gpio_pin_t button_pin, bool *button_pressed);

/**
 * @}
 */

#endif /* _BUTTON_HANDLER_H_ */
