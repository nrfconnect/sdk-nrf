/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

<<<<<<< HEAD
/** @file
 *  @brief LED assignments
 *
 * LED mappings are listed here.
 */

=======
>>>>>>> 1fbc5f5ef0 (applications: nrf5340_audio: LED module for kits with other LED setup)
#ifndef _LED_ASSIGNMENTS_H_
#define _LED_ASSIGNMENTS_H_

#include <zephyr/devicetree.h>
#include <stdint.h>

/**
 * @brief Value to indicate an LED is not assigned to indicate an action.
 */
#define LED_AUDIO_NOT_ASSIGNED 0xFF

/**
 * @brief Mapping of LEDs on board to indication.
 */
#if DT_NODE_EXISTS(DT_NODELABEL(led_device_type))
#define LED_AUDIO_DEVICE_TYPE DT_NODE_CHILD_IDX(DT_NODELABEL(led_device_type))
#else
#define LED_AUDIO_DEVICE_TYPE LED_AUDIO_NOT_ASSIGNED
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(led_conn_status))
#define LED_AUDIO_CONN_STATUS DT_NODE_CHILD_IDX(DT_NODELABEL(led_conn_status))
#else
#define LED_AUDIO_CONN_STATUS LED_AUDIO_NOT_ASSIGNED
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(led_app_status))
#define LED_AUDIO_APP_STATUS DT_NODE_CHILD_IDX(DT_NODELABEL(led_app_status))
#else
#define LED_AUDIO_APP_STATUS LED_AUDIO_NOT_ASSIGNED
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(led_sync_status))
#define LED_AUDIO_SYNC_STATUS DT_NODE_CHILD_IDX(DT_NODELABEL(led_sync_status))
#else
#define LED_AUDIO_SYNC_STATUS LED_AUDIO_NOT_ASSIGNED
#endif

#endif /* _LED_ASSIGNMENTS_H_ */
