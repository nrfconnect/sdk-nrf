/*************************************************************************************************/
/*
 *  Copyright (c) 2021, PACKETCRAFT, INC.
 *  All rights reserved.
 */
/*************************************************************************************************/

/*
 * Redistribution and use of the Audio subsystem for nRF5340 Software, in binary
 * and source code forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *
 * 1. Redistributions of source code form must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary code form, except as embedded into a Nordic
 *    Semiconductor ASA nRF53 chip or a software update for such product,
 *    must reproduce the above copyright notice, this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of Packetcraft, Inc. nor Nordic Semiconductor ASA nor
 *    the names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA nRF53 chip.
 *
 * 5. Any software provided in binary or source code form under this license
 *    must not be reverse engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY PACKETCRAFT, INC. AND NORDIC SEMICONDUCTOR ASA
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL PACKETCRAFT, INC.,
 * NORDIC SEMICONDUCTOR ASA, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AUDIO_DATAPATH_H_
#define _AUDIO_DATAPATH_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#include "data_fifo.h"
#include "sw_codec_select.h"

/* Presentation delay defines in microseconds */
/* Allow some buffer time to allow for HCI Transport etc */
#define PRES_DLY_BUFFER_US 2500
#define MAX_PRES_DLY_US 40000
#define DEFAULT_PRES_DLY_US 10000
#define MIN_PRES_DLY_US ((DEC_TIME_US * CONFIG_BT_ISO_MAX_CHAN) + PRES_DLY_BUFFER_US)

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
 * @brief Adjust timing to make sure audio data is sent just in time for BLE event
 *
 * @note  The time from last anchor point is checked and then blocks of 1ms
 *        can be dropped to allow the sending of encoded data to be sent just
 *        before the connection interval opens up. This is done to reduce overall
 *        latency.
 *
 * @param[in]  sdu_ref_us  The sdu reference to the previous sent packet in µs
 */
void audio_datapath_just_in_time_check_and_adjust(uint32_t sdu_ref_us);

/**
 * @brief Update sdu_ref_us so that drift compensation can work correctly
 *
 * @note This function is only valid for gateway using I2S as audio source
 *       and unidirectional audio stream (gateway to headset(s))
 *
 * @param sdu_ref_us ISO timestamp reference from BLE controller
 */
void audio_datapath_sdu_ref_update(uint32_t sdu_ref_us);

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
