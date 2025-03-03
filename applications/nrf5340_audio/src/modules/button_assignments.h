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
#if DT_NODE_EXISTS(DT_ALIAS(sw_vol_down))
#define BUTTON_VOLUME_DOWN	 DT_GPIO_PIN(DT_ALIAS(sw_vol_down), gpios)
#define BUTTON_VOLUME_DOWN_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw_vol_down), gpios)
#else
#define BUTTON_VOLUME_DOWN	 BUTTON_NOT_ASSIGNED
#define BUTTON_VOLUME_DOWN_FLAGS 0
#endif

#if DT_NODE_EXISTS(DT_ALIAS(sw_vol_up))
#define BUTTON_VOLUME_UP       DT_GPIO_PIN(DT_ALIAS(sw_vol_up), gpios)
#define BUTTON_VOLUME_UP_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw_vol_up), gpios)
#else
#define BUTTON_VOLUME_UP       BUTTON_NOT_ASSIGNED
#define BUTTON_VOLUME_UP_FLAGS 0
#endif

#if DT_NODE_EXISTS(DT_ALIAS(sw_play_pause))
#define BUTTON_PLAY_PAUSE	DT_GPIO_PIN(DT_ALIAS(sw_play_pause), gpios)
#define BUTTON_PLAY_PAUSE_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw_play_pause), gpios)
#else
#define BUTTON_PLAY_PAUSE	BUTTON_NOT_ASSIGNED
#define BUTTON_PLAY_PAUSE_FLAGS 0
#endif

#if DT_NODE_EXISTS(DT_ALIAS(sw_tone))
#define BUTTON_TONE	  DT_GPIO_PIN(DT_ALIAS(sw_tone), gpios)
#define BUTTON_TONE_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw_tone), gpios)
#else
#define BUTTON_TONE	  BUTTON_NOT_ASSIGNED
#define BUTTON_TONE_FLAGS 0
#endif

#if DT_NODE_EXISTS(DT_ALIAS(sw_mute))
#define BUTTON_MUTE	  DT_GPIO_PIN(DT_ALIAS(sw_mute), gpios)
#define BUTTON_MUTE_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw_mute), gpios)
#else
#define BUTTON_MUTE	  BUTTON_NOT_ASSIGNED
#define BUTTON_MUTE_FLAGS 0
#endif

#endif /* _BUTTON_ASSIGNMENTS_H_ */
