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
#define BT_AUDIO_CODEC_CONFIG_FREQ    BT_CODEC_CONFIG_LC3_FREQ_48KHZ
#define BT_AUDIO_CODEC_CAPABILIY_FREQ BT_CODEC_LC3_FREQ_48KHZ
#elif (CONFIG_AUDIO_SAMPLE_RATE_24000_HZ)
#define BT_AUDIO_CODEC_CONFIG_FREQ    BT_CODEC_CONFIG_LC3_FREQ_24KHZ
#define BT_AUDIO_CODEC_CAPABILIY_FREQ BT_CODEC_LC3_FREQ_24KHZ
#elif (CONFIG_AUDIO_SAMPLE_RATE_16000_HZ)
#define BT_AUDIO_CODEC_CONFIG_FREQ    BT_CODEC_CONFIG_LC3_FREQ_16KHZ
#define BT_AUDIO_CODEC_CAPABILIY_FREQ BT_CODEC_LC3_FREQ_16KHZ
#endif /* (CONFIG_AUDIO_SAMPLE_RATE_48000_HZ) */

#define BT_BAP_LC3_PRESET_CONFIGURABLE(_loc, _stream_context, _bitrate)                            \
	BT_BAP_LC3_PRESET(                                                                         \
		BT_CODEC_LC3_CONFIG(BT_AUDIO_CODEC_CONFIG_FREQ, BT_CODEC_CONFIG_LC3_DURATION_10,   \
				    _loc, LE_AUDIO_SDU_SIZE_OCTETS(_bitrate), 1, _stream_context), \
		BT_CODEC_LC3_QOS_10_UNFRAMED(LE_AUDIO_SDU_SIZE_OCTETS(_bitrate),                   \
					     CONFIG_BT_AUDIO_RETRANSMITS,                          \
					     CONFIG_BT_AUDIO_MAX_TRANSPORT_LATENCY_MS,             \
					     CONFIG_BT_AUDIO_PRESENTATION_DELAY_US))

enum audio_roles {
	UNICAST_SERVER_SINK = BIT(0),
	UNICAST_SERVER_SOURCE = BIT(1),
	UNICAST_SERVER_BIDIR = (BIT(0) | BIT(1)),
	UNICAST_CLIENT_SINK = BIT(2),
	UNICAST_CLIENT_SOURCE = BIT(3),
	UNICAST_CLIENT_BIDIR = (BIT(2) | BIT(3)),
	BROADCAST_SINK = BIT(4),
	BROADCAST_SOURCE = BIT(5),
};

/* Some pre-defined use cases */
enum le_audio_device_type {
	LE_AUDIO_RECEIVER = (UNICAST_SERVER_SINK | BROADCAST_SINK),
	LE_AUDIO_HEADSET = (UNICAST_SERVER_BIDIR | BROADCAST_SINK),
	LE_AUDIO_MICROPHONE = (UNICAST_SERVER_SOURCE),
	LE_AUDIO_SENDER = (UNICAST_CLIENT_SOURCE | BROADCAST_SOURCE),
	LE_AUDIO_BROADCASTER = (BROADCAST_SOURCE),
	LE_AUDIO_GATEWAY = (UNICAST_CLIENT_BIDIR | BROADCAST_SOURCE),
};

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
struct encoded_audio {
	uint8_t const *const data;
	size_t size;
	uint8_t num_ch;
};

/**
 * @brief	Stop the LE Audio role given by @p role.
 *
 * @param[in]	role	Type of role to stop, can only specify one at a time.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_stop(enum audio_roles role);

/**
 * @brief	Start the LE Audio role given by @p role.
 *
 * @param[in]	role	Type of role to start, can only specify one at a time.
 *
 * @retval	0 for success, error otherwise.
 */
int le_audio_start(enum audio_roles role);

/**
 * @brief	Send the Bluetooth LE Audio data.
 *
 * @param[in]	enc_audio	Encoded audio struct.
 * @param[in]	role		Specify what role to send through, can send to multiple roles.
 *
 * @retval	0		Operation successful.
 * @retval	-ENXIO		The feature is disabled.
 * @retval	error		Otherwise.
 */
int le_audio_send(struct encoded_audio enc_audio, enum audio_roles role);

/**
 * @brief	Enable the LE Audio device.
 *
 * @param[in]	type		Type of LE Audio device to enable, can either be a
 *				single device type or a selection of audio_roles.
 * @param[in]	recv_cb		Callback for receiving Bluetooth LE Audio data.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_enable(enum le_audio_device_type type, le_audio_receive_cb recv_cb);

/**
 * @brief	Disable the LE Audio device.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_disable(void);

#endif /* _LE_AUDIO_H_ */
