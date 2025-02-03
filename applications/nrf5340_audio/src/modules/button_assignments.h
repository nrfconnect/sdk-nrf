/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Button assignments
 *
 * Button mappings are listed here.
 *
 */

#ifndef _BUTTON_ASSIGNMENTS_H_
#define _BUTTON_ASSIGNMENTS_H_

#include <zephyr/drivers/gpio.h>

/** @brief Constant to indicate an unused button assignment
 */
#define BUTTON_NOT_ASSIGNED 0xFF

/** @brief List of buttons and associated metadata
 */
#define BUTTON_VOLUME_DOWN DT_GPIO_PIN(DT_ALIAS(sw0), gpios)
#define BUTTON_VOLUME_DOWN_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios)

#define BUTTON_VOLUME_UP DT_GPIO_PIN(DT_ALIAS(sw1), gpios)
#define BUTTON_VOLUME_UP_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw1), gpios)

#define BUTTON_PLAY_PAUSE DT_GPIO_PIN(DT_ALIAS(sw2), gpios)
#define BUTTON_PLAY_PAUSE_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw2), gpios)

#if defined(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP)
#define BUTTON_TONE DT_GPIO_PIN(DT_ALIAS(sw3), gpios)
#define BUTTON_TONE_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw3), gpios)

#define BUTTON_MUTE DT_GPIO_PIN(DT_ALIAS(sw4), gpios)
#define BUTTON_MUTE_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw4), gpios)
#else
#define BUTTON_TONE BUTTON_NOT_ASSIGNED
#define BUTTON_TONE_FLAGS 0

#define BUTTON_MUTE DT_GPIO_PIN(DT_ALIAS(sw3), gpios)
#define BUTTON_MUTE_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw3), gpios)
#endif /* CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP */

#endif /* _BUTTON_ASSIGNMENTS_H_ */
