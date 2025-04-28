/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _STREAMCTRL_H_
#define _STREAMCTRL_H_

#include <stddef.h>
#include <zephyr/kernel.h>
#include <audio_defines.h>

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
 * @param audio_frame		Pointer to the audio data to send.
 */
void streamctrl_send(struct audio_data const *const audio_frame);

#endif /* _STREAMCTRL_H_ */
