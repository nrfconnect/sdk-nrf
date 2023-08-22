/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _STREAMCTRL_H_
#define _STREAMCTRL_H_

#include <stddef.h>
#include <zephyr/kernel.h>

/* State machine states for peer or stream. */
enum stream_state {
	STATE_STREAMING,
	STATE_PAUSED,
};

/**
 * @brief Get the current streaming state.
 *
 * @return      strm_state enum value.
 */
uint8_t stream_state_get(void);

/**
 * @brief Send audio data over the stream.
 *
 * @param data		Data to send.
 * @param size		Size of data.
 * @param num_ch	Number of audio channels.
 */
void streamctrl_send(void const *const data, size_t size, uint8_t num_ch);

/**
 * @brief Initialize the internal functionality and start streamctrl.
 *
 * @return 0 if successful.
 */
int streamctrl_start(void);

#endif /* _STREAMCTRL_H_ */
