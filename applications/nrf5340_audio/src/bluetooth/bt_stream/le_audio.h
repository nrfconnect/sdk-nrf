/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup audio_app_bt_stream LE Audio helper functions
 * @{
 * @brief LE Audio helper functions for Audio applications.
 *
 * This module provides the core LE Audio helper functions for both unicast (CIS)
 * and broadcast (BIS) modes.
 */

#ifndef _LE_AUDIO_H_
#define _LE_AUDIO_H_

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/net_buf.h>
#include <audio_defines.h>

#define LE_AUDIO_ZBUS_EVENT_WAIT_TIME	  K_MSEC(5)

#define LE_AUDIO_SDU_SIZE_OCTETS(bitrate) (bitrate / (1000000 / CONFIG_AUDIO_FRAME_DURATION_US) / 8)

#if CONFIG_SAMPLE_RATE_CONVERTER && CONFIG_AUDIO_SAMPLE_RATE_48000_HZ
#define BT_AUDIO_CODEC_CAPABILIY_FREQ                                                              \
	BT_AUDIO_CODEC_CAP_FREQ_48KHZ | BT_AUDIO_CODEC_CAP_FREQ_24KHZ |                            \
		BT_AUDIO_CODEC_CAP_FREQ_16KHZ

#elif CONFIG_AUDIO_SAMPLE_RATE_16000_HZ
#define BT_AUDIO_CODEC_CAPABILIY_FREQ BT_AUDIO_CODEC_CAP_FREQ_16KHZ

#elif CONFIG_AUDIO_SAMPLE_RATE_24000_HZ
#define BT_AUDIO_CODEC_CAPABILIY_FREQ BT_AUDIO_CODEC_CAP_FREQ_24KHZ

#elif CONFIG_AUDIO_SAMPLE_RATE_48000_HZ
#define BT_AUDIO_CODEC_CAPABILIY_FREQ BT_AUDIO_CODEC_CAP_FREQ_48KHZ

#else
#error No sample rate supported
#endif /* CONFIG_SAMPLE_RATE_CONVERTER */

/** Configure LC3 codec preset with customizable parameters for LE Audio streams
 *  using location, stream context, and bitrate parameters.
 */
#define BT_BAP_LC3_PRESET_CONFIGURABLE(_loc, _stream_context, _bitrate)                            \
	BT_BAP_LC3_PRESET(BT_AUDIO_CODEC_LC3_CONFIG(CONFIG_BT_AUDIO_PREF_SAMPLE_RATE_VALUE,        \
						    BT_AUDIO_CODEC_CFG_DURATION_10, _loc,          \
						    LE_AUDIO_SDU_SIZE_OCTETS(_bitrate), 1,         \
						    _stream_context),                              \
			  BT_BAP_QOS_CFG_UNFRAMED(10000u, LE_AUDIO_SDU_SIZE_OCTETS(_bitrate),      \
						  CONFIG_BT_AUDIO_RETRANSMITS,                     \
						  CONFIG_BT_AUDIO_MAX_TRANSPORT_LATENCY_MS,        \
						  CONFIG_BT_AUDIO_PRESENTATION_DELAY_US))

/**
 * @brief Callback for receiving Bluetooth LE Audio data.
 *
 * @param	audio_frame	Pointer to audio buffer.
 * @param	meta		Pointer to audio metadata.
 * @param	channel_index	Audio channel index.
 */
typedef void (*le_audio_receive_cb)(struct net_buf *audio_frame, struct audio_metadata *meta,
				    uint8_t channel_index);

/**
 * @brief Stream index structure for identifying audio streams.
 *
 * This structure provides a hierarchical index for identifying audio streams
 * in both unicast (CIS) and broadcast (BIS) modes.
 */
struct stream_index {
	/** Level 1: BIG (Broadcast Isochronous Group) or CIG (Connected Isochronous Group) */
	uint8_t lvl1;
	/** Level 2: Subgroups (only applicable to Broadcast) */
	uint8_t lvl2;
	/** Level 3: BIS (Broadcast Isochronous Stream) or CIS (Connected Isochronous Stream) */
	uint8_t lvl3;
};

/**
 * @brief	Get the number of active streams.
 *
 * @note	This function returns the number of streams that are currently active.
 *		An active stream is one that has been set up and is ready to receive data.
 *		This function is used by broadcast_sink, unicast_server and unicast_client.
 *
 * @return	Number of active BIS, or negative error code on failure.
 */
int le_audio_concurrent_sync_num_get(void);

/**
 * @brief Function to populate the audio metadata.
 *
 * @param[in] meta		Pointer to the audio metadata.
 * @param[in] stream		Pointer to the stream.
 * @param[in] info		Pointer to the ISO information.
 * @param[in] audio_frame	Pointer to the buffer.
 *
 * @return 0 if successful, error otherwise.
 */
int le_audio_metadata_populate(struct audio_metadata *meta, const struct bt_bap_stream *stream,
			       const struct bt_iso_recv_info *info,
			       const struct net_buf *audio_frame);

/**
 * @brief	Get the current state of an endpoint.
 *
 * @param[in]	ep	The endpoint to check.
 * @param[out]	state	The state of the endpoint.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_ep_state_get(struct bt_bap_ep const *const ep, uint8_t *state);

/**
 * @brief	Check if an endpoint is in the given state.
 *		If the endpoint is NULL, it is not in the
 *		given state, and this function returns false.
 *
 * @param[in]	ep	The endpoint to check.
 * @param[in]	state	The state to check for.
 *
 * @retval	true	The endpoint is in the given state.
 * @retval	false	Otherwise.
 */
bool le_audio_ep_state_check(struct bt_bap_ep const *const ep, enum bt_bap_ep_state state);

/**
 * @brief	Check if an endpoint has had the QoS configured.
 *		If the endpoint is NULL, it is not in the state, and this function returns false.
 *
 * @param[in]	ep	The endpoint to check.
 *
 * @retval	true	The endpoint QoS is configured.
 * @retval	false	Otherwise.
 */
bool le_audio_ep_qos_configured(struct bt_bap_ep const *const ep);

/**
 * @brief	Decode the audio sampling frequency in the codec configuration.
 *
 * @param[in]	codec		Pointer to the audio codec structure.
 * @param[out]	freq_hz		Pointer to the sampling frequency in Hz.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_freq_hz_get(const struct bt_audio_codec_cfg *codec, int *freq_hz);

/**
 * @brief	Decode the audio frame duration in us in the codec configuration.
 *
 * @param[in]	codec		Pointer to the audio codec structure.
 * @param[out]	frame_dur_us	Pointer to the frame duration in us.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_duration_us_get(const struct bt_audio_codec_cfg *codec, int *frame_dur_us);

/**
 * @brief	Decode the number of octets per frame in the codec configuration.
 *
 * @param[in]	codec		Pointer to the audio codec structure.
 * @param[out]	octets_per_sdu	Pointer to the number of octets per SDU.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_octets_per_frame_get(const struct bt_audio_codec_cfg *codec, uint32_t *octets_per_sdu);

/**
 * @brief	Decode the number of frame blocks per SDU in the codec configuration.
 *
 * @param[in]	codec			Pointer to the audio codec structure.
 * @param[out]	frame_blks_per_sdu	Pointer to the number of frame blocks per SDU.
 *
 * @return	0 for success, error otherwise.
 */
int le_audio_frame_blocks_per_sdu_get(const struct bt_audio_codec_cfg *codec,
				      uint32_t *frame_blks_per_sdu);

/**
 * @brief	Get the bitrate for the codec configuration.
 *
 * @details	Decodes the audio frame duration and the number of octets per fram from the codec
 *		configuration, and calculates the bitrate.
 *
 * @param[in]	codec	Pointer to the audio codec structure.
 * @param[out]	bitrate	Pointer to the bitrate in bps.
 */
int le_audio_bitrate_get(const struct bt_audio_codec_cfg *const codec, uint32_t *bitrate);

/**
 * @brief	Get the direction of the @p stream provided.
 *
 * @param[in]	stream	Stream to check direction for.
 *
 * @retval	BT_AUDIO_DIR_SINK	sink direction.
 * @retval	BT_AUDIO_DIR_SOURCE	source direction.
 * @retval	Negative value		Failed to get ep_info from host.
 *
 */
int le_audio_stream_dir_get(struct bt_bap_stream const *const stream);

/**
 * @brief	Check that the bitrate is within the supported range.
 *
 * @param[in]	codec	The audio codec structure.
 *
 * retval	true	The bitrate is in the supported range.
 * retval	false	Otherwise.
 */
bool le_audio_bitrate_check(const struct bt_audio_codec_cfg *codec);

/**
 * @brief	Check that the sample rate is supported.
 *
 * @param[in]	codec	The audio codec structure.
 *
 * retval	true	The sample rate is supported.
 * retval	false	Otherwise.
 */
bool le_audio_freq_check(const struct bt_audio_codec_cfg *codec);

/**
 * @brief	Print the codec configuration
 *
 * @param[in]	codec	Pointer to the audio codec structure.
 * @param[in]	dir	Direction to print the codec configuration for.
 */
void le_audio_print_codec(const struct bt_audio_codec_cfg *codec, enum bt_audio_dir dir);

/**
 * @}
 */

#endif /* _LE_AUDIO_H_ */
