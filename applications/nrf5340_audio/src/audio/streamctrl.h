/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup nrf5340_audio_streamctrl Audio Stream Control
 * @{
 * @brief Stream control API for nRF5340 Audio applications.
 *
 * This module provides stream state management and control functions for audio streaming
 * operations. It manages the streaming state (playing/paused) and provides functions to
 * send audio frames to the appropriate output streams. The stream control interfaces with
 * the audio system and Bluetooth modules to coordinate audio data flow and state transitions
 * between unicast (CIS) and broadcast (BIS) modes.
 */

#ifndef _STREAMCTRL_H_
#define _STREAMCTRL_H_

#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

/**
 * @brief Stream state enumeration for audio streaming operations.
 */
enum stream_state {
	STATE_STREAMING,	/**< Audio is currently being streamed */
	STATE_PAUSED,		/**< Audio streaming is paused */
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
 *
 * @see @ref audio_system_start for audio system initialization
 * @see @ref audio_datapath_stream_out for audio output processing
 */
void streamctrl_send(struct net_buf const *const audio_frame);

/**
 * @}
 */

#endif /* _STREAMCTRL_H_ */
