/*
 *  Copyright (c) 2021, PACKETCRAFT, INC.
 *
 *  SPDX-License-Identifier: LicenseRef-PCFT
 */

#ifndef _AUDIO_DATAPATH_H_
#define _AUDIO_DATAPATH_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#include "data_fifo.h"
#include "sw_codec_select.h"

/**
 * @brief Mixes a tone into the I2S TX stream
 *
 * @param freq Tone frequency [Hz]
 * @param dur_ms Tone duration [ms]. 0 = forever
 * @param amplitude Tone amplitude [0, 1]
 *
 * @return 0 if successful, error otherwise
 */
int audio_datapath_tone_play(uint16_t freq, uint16_t dur_ms, float amplitude);

/**
 * @brief Stops tone playback
 */
void audio_datapath_tone_stop(void);

/**
 * @brief Set the presentation delay
 *
 * @param delay_us The presentation delay in µs
 *
 * @return 0 if successful, error otherwise
 */
int audio_datapath_pres_delay_us_set(uint32_t delay_us);

/**
 * @brief Get the current presentation delay
 *
 * @param delay_us  The presentation delay in µs
 */
void audio_datapath_pres_delay_us_get(uint32_t *delay_us);

/**
 * @brief Update sdu_ref_us so that drift compensation can work correctly
 *
 * @note This function is only valid for gateway using I2S as audio source
 *       and unidirectional audio stream (gateway to headset(s))
 *
 * @param sdu_ref_us    ISO timestamp reference from BLE controller
 * @param adjust        Indicate if the sdu_ref should be used to adjust timing
 */
void audio_datapath_sdu_ref_update(uint32_t sdu_ref_us, bool adjust);

/**
 * @brief Input an audio data frame which is processed and outputted over I2S
 *
 * @note A frame of raw encoded audio data is inputted, and this data then is decoded
 *       and processed before being outputted over I2S. The audio is synchronized
 *       using sdu_ref_us
 *
 * @param buf Pointer to audio data frame
 * @param size Size of audio data frame in bytes
 * @param sdu_ref_us ISO timestamp reference from BLE controller
 * @param bad_frame Indicating if the audio frame is bad or not
 * @param recv_frame_ts_us Timestamp of when audio frame was received
 */
void audio_datapath_stream_out(const uint8_t *buf, size_t size, uint32_t sdu_ref_us, bool bad_frame,
			       uint32_t recv_frame_ts_us);

/**
 * @brief Start the audio datapath module
 *
 * @note The continuously running I2S is started
 *
 * @param fifo_rx Pointer to FIFO structure where I2S RX data is put
 *
 * @return 0 if successful, error otherwise
 */
int audio_datapath_start(struct data_fifo *fifo_rx);

/**
 * @brief Stop the audio datapath module
 *
 * @return 0 if successful, error otherwise
 */
int audio_datapath_stop(void);

/**
 * @brief Initialize the audio datapath module
 *
 * @return 0 if successful, error otherwise
 */
int audio_datapath_init(void);

#endif /* _AUDIO_DATAPATH_H_ */
