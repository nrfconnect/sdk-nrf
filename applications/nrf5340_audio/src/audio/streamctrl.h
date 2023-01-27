/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _STREAMCTRL_H_
#define _STREAMCTRL_H_

#include <stddef.h>
#include <zephyr/kernel.h>

/* State machine states for peer/stream */
enum stream_state {
	STATE_STREAMING,
	STATE_PAUSED,
};

/** @brief Get current streaming state
 *
 * @return      strm_state enum value
 */
uint8_t stream_state_get(void);

/** @brief Send encoded data over the stream
 *
 * @param data		Data to send
 * @param size		Size of data
 * @param num_ch	Number of audio channels
 */
void streamctrl_encoded_data_send(void const *const data, size_t size, uint8_t num_ch);

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

/** @brief Init internal functionality and start streamctrl
 *
 *  @return 0 if successful.
 */
int streamctrl_start(void);

#endif /* _STREAMCTRL_H_ */
