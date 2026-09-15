/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef VPR_OFFLOADING_LED_CONTROL_H__
#define VPR_OFFLOADING_LED_CONTROL_H__

#include <stdint.h>

/** @brief IPC endpoint name used by both parties. */
#define LED_CONTROL_EP_NAME "led_control"

/**
 * @name Messages for LED control library.
 * @{
 */

/** @brief Start LED blinking. */
#define LED_CONTROL_MSG_START 0

/** @brief Stop LED blinking. */
#define LED_CONTROL_MSG_STOP 1

#define PACKED
/**@} */

/** @brief Structure with LED blinking parameters. */
struct led_control_msg_start {
	/** Period when LED is on (in milliseconds). */
	uint32_t period_on;

	/** Period when LED is off (in milliseconds). */
	uint32_t period_off;
} PACKED;

/** @brief LED control message structure. */
struct led_control_msg {
	/** Message type. */
	uint8_t type;
	union {
		/** Data for the start message. */
		struct led_control_msg_start start;
	};
} PACKED;

/** @brief Function for starting LED blinking.
 *
 * @param period_on Time LED is on (in milliseconds).
 * @param period_off Time LED is off (in milliseconds).
 *
 * @retval 0 on success.
 * @retval negative if sending failed.
 */
int led_control_start(uint32_t period_on, uint32_t period_off);

/** @brief Function for starting LED blinking.
 *
 * @retval 0 on success.
 * @retval negative if sending failed.
 */
int led_control_stop(void);

#endif /* VPR_OFFLOADING_LED_CONTROL_H__ */
