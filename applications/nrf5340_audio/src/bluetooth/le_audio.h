/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LE_AUDIO_H_
#define _LE_AUDIO_H_

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/audio/audio.h>

#define DEVICE_NAME_PEER CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_PEER_LEN (sizeof(DEVICE_NAME_PEER) - 1)

#define LE_AUDIO_SDU_SIZE_OCTETS(bitrate) (bitrate / (1000000 / CONFIG_AUDIO_FRAME_DURATION_US) / 8)
#define LE_AUDIO_PRES_DELAY_US 10000u

#if CONFIG_TRANSPORT_CIS
#define BT_AUDIO_LC3_UNICAST_PRESET_RECOMMENDED(_loc, _stream_context)                             \
	BT_AUDIO_LC3_PRESET(                                                                       \
		BT_CODEC_LC3_CONFIG(                                                               \
			BT_CODEC_CONFIG_LC3_FREQ_48KHZ, BT_CODEC_CONFIG_LC3_DURATION_10, _loc,     \
			LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 1, _stream_context),         \
		BT_CODEC_LC3_QOS_10_UNFRAMED(LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 2u,     \
					     20u, LE_AUDIO_PRES_DELAY_US))

#if CONFIG_AUDIO_SOURCE_USB
/* Only 48kHz is supported when using USB */
#if !CONFIG_BT_AUDIO_UNICAST_RECOMMENDED
#error USB only supports 48kHz
#endif /* !CONFIG_BT_AUDIO_UNICAST_RECOMMENDED */
#define BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO                                                  \
	BT_AUDIO_LC3_UNICAST_PRESET_RECOMMENDED(BT_AUDIO_LOCATION_FRONT_LEFT,                      \
						BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_AUDIO_SOURCE_I2S
#if CONFIG_BT_AUDIO_UNICAST_RECOMMENDED
#define BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO                                                  \
	BT_AUDIO_LC3_UNICAST_PRESET_RECOMMENDED(BT_AUDIO_LOCATION_FRONT_LEFT,                      \
						BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_AUDIO_UNICAST_16_2_1
#define BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO                                                  \
	BT_AUDIO_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,                           \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_AUDIO_UNICAST_24_2_1
#define BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO                                                  \
	BT_AUDIO_LC3_UNICAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,                           \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#else
#error Unsupported LC3 codec preset for unicast
#endif /* CONFIG_BT_AUDIO_UNICAST_RECOMMENDED */
#endif /* CONFIG_AUDIO_SOURCE_USB */
#endif /* CONFIG_TRANSPORT_CIS */

#if CONFIG_TRANSPORT_BIS
#define BT_AUDIO_LC3_BROADCAST_PRESET_RECOMMENDED(_loc, _stream_context)                           \
	BT_AUDIO_LC3_PRESET(                                                                       \
		BT_CODEC_LC3_CONFIG(                                                               \
			BT_CODEC_CONFIG_LC3_FREQ_48KHZ, BT_CODEC_CONFIG_LC3_DURATION_10, _loc,     \
			LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 1, _stream_context),         \
		BT_CODEC_LC3_QOS_10_UNFRAMED(LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 4u,     \
					     20u, LE_AUDIO_PRES_DELAY_US))

#if CONFIG_AUDIO_SOURCE_USB
#if !CONFIG_BT_AUDIO_BROADCAST_RECOMMENDED
#error USB only supports 48kHz
#endif /* !CONFIG_BT_AUDIO_BROADCAST_RECOMMENDED */
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_RECOMMENDED(BT_AUDIO_LOCATION_FRONT_LEFT,                    \
						  BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_AUDIO_SOURCE_I2S
#if CONFIG_BT_AUDIO_BROADCAST_RECOMMENDED
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_RECOMMENDED(BT_AUDIO_LOCATION_FRONT_LEFT,                    \
						  BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_AUDIO_BROADCAST_16_2_1
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,                         \
					     BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_AUDIO_BROADCAST_24_2_1
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,                         \
					     BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_AUDIO_BROADCAST_16_2_2
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_16_2_2(BT_AUDIO_LOCATION_FRONT_LEFT,                         \
					     BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_AUDIO_BROADCAST_24_2_2
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_24_2_2(BT_AUDIO_LOCATION_FRONT_LEFT,                         \
					     BT_AUDIO_CONTEXT_TYPE_MEDIA)

#else
#error Unsupported LC3 codec preset for broadcast
#endif /* CONFIG_BT_AUDIO_BROADCAST_RECOMMENDED */
#endif /* CONFIG_AUDIO_SOURCE_USB */
#endif /* CONFIG_TRANSPORT_BIS */

enum le_audio_evt_type {
	LE_AUDIO_EVT_CONFIG_RECEIVED,
	LE_AUDIO_EVT_STREAMING,
	LE_AUDIO_EVT_NOT_STREAMING,
	LE_AUDIO_EVT_NUM_EVTS
};

struct le_audio_evt {
	enum le_audio_evt_type le_audio_evt_type;
};

/**
 * @brief Callback for receiving Bluetooth LE Audio data
 *
 * @param data		Pointer to received data
 * @param size		Size of received data
 * @param bad_frame	Indicating if the frame is a bad frame or not
 * @param sdu_ref	ISO timestamp
 */
typedef void (*le_audio_receive_cb)(const uint8_t *const data, size_t size, bool bad_frame,
				    uint32_t sdu_ref);

/**
 * @brief Generic function for a user defined button press
 *
 * @return	0 for success,
 *		error otherwise
 */
int le_audio_user_defined_button_press(void);

/**
 * @brief Get configuration for audio stream
 *
 * @param bitrate	Pointer to bitrate used
 * @param sampling_rate	Pointer to sampling rate used
 *
 * @return	0 for success,
 *		-ENXIO if the feature is disabled,
 *		error otherwise
 */
int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate);

/**
 * @brief	Increase volume by one step
 *
 * @return	0 for success,
 *		-ENXIO if the feature is disabled,
 *		error otherwise
 */
int le_audio_volume_up(void);

/**
 * @brief	Decrease volume by one step
 *
 * @return	0 for success,
 *		-ENXIO if the feature is disabled,
 *		error otherwise
 */
int le_audio_volume_down(void);

/**
 * @brief	Mute volume
 *
 * @return	0 for success,
 *		-ENXIO if the feature is disabled,
 *		error otherwise
 */
int le_audio_volume_mute(void);

/**
 * @brief	Either resume or pause the Bluetooth LE Audio stream,
 *              depending on the current state of the stream
 *
 * @return	0 for success, error otherwise
 */
int le_audio_play_pause(void);

/**
 * @brief Send Bluetooth LE Audio data
 *
 * @param data	Data to send
 * @param size	Size of data to send
 *
 * @return	0 for success,
 *		-ENXIO if the feature is disabled,
 *		error otherwise
 */
int le_audio_send(uint8_t const *const data, size_t size);

/**
 * @brief Enable Bluetooth LE Audio
 *
 * @param recv_cb	Callback for receiving Bluetooth LE Audio data
 *
 * @return		0 for success, error otherwise
 */
int le_audio_enable(le_audio_receive_cb recv_cb);

/**
 * @brief Disable Bluetooth LE Audio
 *
 * @return	0 for success, error otherwise
 */
int le_audio_disable(void);

#endif /* _LE_AUDIO_H_ */
