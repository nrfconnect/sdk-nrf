/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _STREAMCTRL_H_
#define _STREAMCTRL_H_

#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

/* State machine states for peer or stream. */
enum stream_state {
	STATE_STREAMING,
	STATE_PAUSED,
};

/**
 * @brief	Get the current streaming state.
 *
 * @return	strm_state enum value.
 */
uint8_t stream_state_get(void);

/**
 * @brief	Send audio data over the stream.
 *
 * @param	audio_frame	Pointer to the audio buffer.
 */
void streamctrl_send(struct net_buf const *const audio_frame);

#endif /* _STREAMCTRL_H_ */
