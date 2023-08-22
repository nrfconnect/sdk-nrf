/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LE_AUDIO_H_
#define _LE_AUDIO_H_

#include <zephyr/bluetooth/audio/audio.h>
#include <audio_defines.h>

#define LE_AUDIO_ZBUS_EVENT_WAIT_TIME	  K_MSEC(5)
#define LE_AUDIO_SDU_SIZE_OCTETS(bitrate) (bitrate / (1000000 / CONFIG_AUDIO_FRAME_DURATION_US) / 8)

#if (CONFIG_AUDIO_SAMPLE_RATE_48000_HZ)
#define BT_AUDIO_CODEC_CONFIG_FREQ    BT_AUDIO_CODEC_CONFIG_LC3_FREQ_48KHZ
#define BT_AUDIO_CODEC_CAPABILIY_FREQ BT_AUDIO_CODEC_LC3_FREQ_48KHZ
#elif (CONFIG_AUDIO_SAMPLE_RATE_24000_HZ)
#define BT_AUDIO_CODEC_CONFIG_FREQ    BT_AUDIO_CODEC_CONFIG_LC3_FREQ_24KHZ
#define BT_AUDIO_CODEC_CAPABILIY_FREQ BT_AUDIO_CODEC_LC3_FREQ_24KHZ
#elif (CONFIG_AUDIO_SAMPLE_RATE_16000_HZ)
#define BT_AUDIO_CODEC_CONFIG_FREQ    BT_AUDIO_CODEC_CONFIG_LC3_FREQ_16KHZ
#define BT_AUDIO_CODEC_CAPABILIY_FREQ BT_AUDIO_CODEC_LC3_FREQ_16KHZ
#endif /* (CONFIG_AUDIO_SAMPLE_RATE_48000_HZ) */

#define BT_BAP_LC3_PRESET_CONFIGURABLE(_loc, _stream_context, _bitrate)                            \
	BT_BAP_LC3_PRESET(                                                                         \
		BT_AUDIO_CODEC_LC3_CONFIG(BT_AUDIO_CODEC_CONFIG_FREQ,                              \
					  BT_AUDIO_CODEC_CONFIG_LC3_DURATION_10, _loc,             \
					  LE_AUDIO_SDU_SIZE_OCTETS(_bitrate), 1, _stream_context), \
		BT_AUDIO_CODEC_LC3_QOS_10_UNFRAMED(LE_AUDIO_SDU_SIZE_OCTETS(_bitrate),             \
						   CONFIG_BT_AUDIO_RETRANSMITS,                    \
						   CONFIG_BT_AUDIO_MAX_TRANSPORT_LATENCY_MS,       \
						   CONFIG_BT_AUDIO_PRESENTATION_DELAY_US))

/**
 * @brief Callback for receiving Bluetooth LE Audio data.
 *
 * @param	data		Pointer to received data.
 * @param	size		Size of received data.
 * @param	bad_frame	Indicating if the frame is a bad frame or not.
 * @param	sdu_ref		ISO timestamp.
 * @param	channel_index	Audio channel index.
 */
typedef void (*le_audio_receive_cb)(const uint8_t *const data, size_t size, bool bad_frame,
				    uint32_t sdu_ref, enum audio_channel channel_index,
				    size_t desired_size);

/**
 * @brief	Encoded audio data and information.
 *
 * @note	Container for SW codec (typically LC3) compressed audio data.
 */
struct le_audio_encoded_audio {
	uint8_t const *const data;
	size_t size;
	uint8_t num_ch;
};

#endif /* _LE_AUDIO_H_ */
