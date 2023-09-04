/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LE_AUDIO_H_
#define _LE_AUDIO_H_

#include <zephyr/kernel.h>
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

#if CONFIG_TRANSPORT_CIS
#if CONFIG_BT_BAP_UNICAST_CONFIGURABLE
#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SINK                                               \
	BT_BAP_LC3_PRESET_CONFIGURABLE(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA,  \
				       CONFIG_BT_AUDIO_BITRATE_UNICAST_SINK)

#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SOURCE                                             \
	BT_BAP_LC3_PRESET_CONFIGURABLE(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA,  \
				       CONFIG_BT_AUDIO_BITRATE_UNICAST_SRC)

#elif CONFIG_BT_BAP_UNICAST_16_2_1
#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SINK                                               \
	BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA)

#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SOURCE                                             \
	BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_UNICAST_24_2_1
#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SINK                                               \
	BT_BAP_LC3_UNICAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA)

#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SOURCE                                             \
	BT_BAP_LC3_UNICAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA)

#else
#error Unsupported LC3 codec preset for unicast
#endif /* CONFIG_BT_BAP_UNICAST_CONFIGURABLE */
#endif /* CONFIG_TRANSPORT_CIS */

#if CONFIG_TRANSPORT_BIS
#if CONFIG_BT_AUDIO_BROADCAST_CONFIGURABLE
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_PRESET_CONFIGURABLE(                                                            \
		BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,                      \
		BT_AUDIO_CONTEXT_TYPE_MEDIA, CONFIG_BT_AUDIO_BITRATE_BROADCAST_SRC)

#elif CONFIG_BT_BAP_BROADCAST_16_2_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_24_2_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_16_2_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_16_2_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_24_2_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_24_2_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#else
#error Unsupported LC3 codec preset for broadcast
#endif /* CONFIG_BT_AUDIO_BROADCAST_CONFIGURABLE */
#endif /* CONFIG_TRANSPORT_BIS */

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
 * @brief	Callback for using the timestamp of the previously sent audio packet.
 *
 * @note	Can be used for drift calculation or compensation.
 *
 * @param[in]	timestamp	The timestamp of the previously sent audio packet.
 * @param[in]	adjust		Indicate if the sdu_ref should be used to adjust timing.
 */
typedef void (*le_audio_timestamp_cb)(uint32_t timestamp, bool adjust);

enum le_audio_user_defined_action {
	LE_AUDIO_USER_DEFINED_ACTION_1,
	LE_AUDIO_USER_DEFINED_ACTION_2,
	LE_AUDIO_USER_DEFINED_ACTION_NUM
};

/**
 * @brief	Encoded audio data and information.
 * Container for SW codec (typically LC3) compressed audio data.
 */
struct encoded_audio {
	uint8_t const *const data;
	size_t size;
	uint8_t num_ch;
};

/**
 * @brief	Generic function for a user defined button press.
 *
 * @param[in]	action	User defined action.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_user_defined_button_press(enum le_audio_user_defined_action action);

/**
 * @brief	Get configuration for audio stream.
 *
 * @param[out]	bitrate		Pointer to the bitrate used; can be NULL.
 * @param[out]	sampling_rate	Pointer to the sampling rate used; can be NULL.
 * @param[out]	pres_delay	Pointer to the presentation delay used; can be NULL.
 *
 * @retval	0		Operation successful.
 * @retval	-ENXIO		The feature is disabled.
 * @retval	-ENOTSUP	The feature is not supported,
 * @retval	error		Otherwise
 */
int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate, uint32_t *pres_delay);

/**
 * @brief	Set pointer to the connection.
 *
 * @note	Used by CIS.
 *
 * @param[in]	conn	The connection pointer.
 */
void le_audio_conn_set(struct bt_conn *conn);

/**
 * @brief	Set periodic advertising sync.
 *
 * @param[in]	pa_sync		Pointer to the periodic advertising sync.
 * @param[in]	broadcast_id	Broadcast ID of the periodic advertising.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_pa_sync_set(struct bt_le_per_adv_sync *pa_sync, uint32_t broadcast_id);

/**
 * @brief	Notify about disconnected connection.
 *
 * @note	Used by CIS.
 *
 * @param[in]	conn	The connection pointer.
 */
void le_audio_conn_disconnected(struct bt_conn *conn);

/**
 * @brief	Set pointer to extended advertisement with periodic adv configured
 *
 * @note	Used by BIS.
 *
 * @note	Will also start the broadcast_source.
 *
 * @param[in]	ext_adv	Pointer to the extended advertisement.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_ext_adv_set(struct bt_le_ext_adv *ext_adv);

/**
 * @brief	Get bt_data containing the data to advertise.
 *
 * @param[out]	adv		Pointer to the pointer of bt_data to advertise.
 * @param[out]	adv_size	Pointer to size of @p adv.
 * @param[in]	periodic	Specify if the data is for periodic advertisement.
 */
void le_audio_adv_get(const struct bt_data **adv, size_t *adv_size, bool periodic);

/**
 * @brief	Resume the Bluetooth LE Audio stream.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_play(void);

/**
 * @brief	Pause the Bluetooth LE Audio stream.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_pause(void);

/**
 * @brief	Send the Bluetooth LE Audio data.
 *
 * @param[in]	enc_audio	Encoded audio struct.
 *
 * @retval	0	Operation successful
 * @retval	-ENXIO	The feature is disabled,
 * @retval	error	Otherwise.
 */
int le_audio_send(struct encoded_audio enc_audio);

/**
 * @brief	Enable Bluetooth LE Audio.
 *
 * @param[in]	recv_cb			Callback for receiving Bluetooth LE Audio data.
 * @param[in]	timestmp_cb		Callback for using timestamp.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_enable(le_audio_receive_cb recv_cb, le_audio_timestamp_cb timestmp_cb);

/**
 * @brief	Disable Bluetooth LE Audio.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_disable(void);

#endif /* _LE_AUDIO_H_ */
