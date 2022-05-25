/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file streamctrl.h
 *  @brief Stream control - control transfer and processing of the audio streams
 *
 * This is the module responsible for "running the system" after initial setup.
 * The module assumes that before _start() is called
 * - hw has been initialized
 * - the ble subsystem has been initialized
 *
 * After being started, the module will handling setup and control of
 * user interface inputs, audio processing and audio transport.
 */

#ifndef _STREAMCTRL_H_
#define _STREAMCTRL_H_

#include <stddef.h>
#include <zephyr/kernel.h>

/* State machine states for peer/stream */
enum stream_state {
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_LINK_READY,
	STATE_STREAMING,
	STATE_PAUSED,
	STATE_DISCONNECTED,
};

/** @brief Get current streaming state
 *
 * @return      stream_state enum value
 */
uint8_t stream_state_get(void);

/** @brief Send encoded data over the stream
 *
 * @param data	Data to send
 * @param len	Length of data to send
 */
void streamctrl_encoded_data_send(void const *const data, size_t len);

/** @brief Drives streamctrl state machine
 *
 * This function drives the streamctrl state machine.
 * The function will read and handle events coming in to the streamctrl
 * module, and configure/run the rest of the system accordingly.
 *
 * Run this function repeatedly, e.g. in an eternal loop, to keep the
 * system running.
 */
void streamctrl_event_handler(void);

/** @brief Start streamctrl
 *
 *  @return 0 if successful.
 */
int streamctrl_start(void);

#endif /* _STREAMCTRL_H_ */
