/*
 *  Copyright (c) 2021, PACKETCRAFT, INC.
 *
 *  SPDX-License-Identifier: LicenseRef-PCFT
 */

/** @file
 * @defgroup audio_app_i2s Audio I2S
 * @{
 * @brief Audio I2S interface API for Audio applications.
 *
 * This module provides the I2S (Inter-IC Sound) interface for audio data transfer
 * between the application and external audio hardware.
 */

#ifndef _AUDIO_I2S_H_
#define _AUDIO_I2S_H_

#include <zephyr/kernel.h>
#include <stdint.h>

/** @brief HFCLKAUDIO frequency value for 12.288-MHz output.
 *
 * This macro defines the frequency value for the High Frequency Clock Audio (HFCLKAUDIO)
 * to generate a 12.288-MHz output frequency. The value is calculated according to the formula
 * in the nRF5340 SoC documentation.
 */
#define HFCLKAUDIO_12_288_MHZ 0x9BA6

#define HFCLKAUDIO_12_165_MHZ 0x8FD8

#define HFCLKAUDIO_12_411_MHZ 0xA774

/**
 * @brief Calculate frame size in bytes based on audio configuration.
 *
 * This macro calculates the number of bytes in one audio frame based on the
 * configured I2S sample rate, number of channels, and bit depth. The frame
 * duration can be either 10 ms or 7.5 ms.
 */
#if ((CONFIG_AUDIO_FRAME_DURATION_US == 7500) && CONFIG_SW_CODEC_LC3)

#define FRAME_SIZE_BYTES                                                                           \
	((CONFIG_I2S_LRCK_FREQ_HZ / 1000 * 15 / 2) * CONFIG_I2S_CH_NUM *                           \
	 CONFIG_AUDIO_BIT_DEPTH_OCTETS)
#else
#define FRAME_SIZE_BYTES                                                                           \
	((CONFIG_I2S_LRCK_FREQ_HZ / 1000 * 10) * CONFIG_I2S_CH_NUM * CONFIG_AUDIO_BIT_DEPTH_OCTETS)
#endif /* ((CONFIG_AUDIO_FRAME_DURATION_US == 7500) && CONFIG_SW_CODEC_LC3) */

/** Calculate block size in bytes for I2S FIFO frame splitting. */
#define BLOCK_SIZE_BYTES (FRAME_SIZE_BYTES / CONFIG_FIFO_FRAME_SPLIT_NUM)

/**
 * @brief Calculate number of samples per I2S block.
 *
 * This macro calculates the number of audio samples in each I2S block,
 * accounting for the bit depth and 32-bit word alignment. The calculation
 * determines how many samples fit within a 32-bit word based on the
 * configured audio bit depth.
 */
#define I2S_SAMPLES_NUM                                                                            \
	(BLOCK_SIZE_BYTES / (CONFIG_AUDIO_BIT_DEPTH_OCTETS) / (32 / CONFIG_AUDIO_BIT_DEPTH_BITS))

/**
 * @brief I2S block complete event callback type
 *
 * @param frame_start_ts I2S frame start timestamp
 * @param rx_buf_released Pointer to the released buffer containing received data
 * @param tx_buf_released Pointer to the released buffer that was used to sent data
 */
typedef void (*i2s_blk_comp_callback_t)(uint32_t frame_start_ts, uint32_t *rx_buf_released,
					uint32_t const *tx_buf_released);

/**
 * @brief Supply the buffers to be used in the next part of the I2S transfer
 *
 * @param tx_buf Pointer to the buffer with data to be sent
 * @param rx_buf Pointer to the buffer for received data
 */
void audio_i2s_set_next_buf(const uint8_t *tx_buf, uint32_t *rx_buf);

/**
 * @brief Start the continuous I2S transfer
 *
 * @param tx_buf Pointer to the buffer with data to be sent
 * @param rx_buf Pointer to the buffer for received data
 */
void audio_i2s_start(const uint8_t *tx_buf, uint32_t *rx_buf);

/**
 * @brief Stop the continuous I2S transfer
 */
void audio_i2s_stop(void);

/**
 * @brief Register callback function for I2S block complete event
 *
 * @param blk_comp_callback Callback function
 */
void audio_i2s_blk_comp_cb_register(i2s_blk_comp_callback_t blk_comp_callback);

/**
 * @brief Initialize I2S module
 */
void audio_i2s_init(void);

/**
 * @}
 */

#endif /* _AUDIO_I2S_H_ */
