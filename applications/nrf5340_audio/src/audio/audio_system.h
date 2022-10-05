/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _AUDIO_SYSTEM_H_
#define _AUDIO_SYSTEM_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Toggle a test tone on and off
 *
 * @param[in] freq Desired frequency of tone, 0 = off
 *
 * @note A stream must already be running to use this feature
 *
 * @return 0 on success, and -ENOMEM if the frequency is too low (buffer overflow)
 */
int audio_encode_test_tone_set(uint32_t freq);

/**
 * @brief Decode data and then add it to TX FIFO buffer
 *
 * @param[in]	encoded_data		Pointer to encoded data
 * @param[in]	encoded_data_size	Size of encoded data
 * @param[in]	bad_frame		Indication on missed or incomplete frame
 *
 * @return 0 on success, error otherwise
 */
int audio_decode(void const *const encoded_data, size_t encoded_data_size, bool bad_frame);

/**
 * @brief Initialize and start both HW and SW audio codec
 */
void audio_system_start(void);

/**
 * @brief Stop all activities related to audio
 */
void audio_system_stop(void);

/**
 * @brief Drop oldest block from fifo_rx buffer
 *
 * @note  This can be used to adjust the timing of completed frame sampled
 *	  in relation to the connection interval in BLE, to reduce latency
 *
 * @return 0 on success, -ECANCELED otherwise
 */
int audio_system_fifo_rx_block_drop(void);

/**
 * @brief Initialize the audio_system
 */
void audio_system_init(void);

#endif /* _AUDIO_SYSTEM_H_ */
