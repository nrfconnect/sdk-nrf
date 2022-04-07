/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SW_CODEC_SELECT_H_
#define _SW_CODEC_SELECT_H_

#include <zephyr.h>

#if (CONFIG_SW_CODEC_SBC)
/* Frame length in samples */
#define SBC_FRAME_LEN_SAMPLES \
	(CONFIG_SBC_NO_OF_BLOCKS * CONFIG_SBC_NO_OF_SUBBANDS)
/* Frame rate in Hz */
#define SBC_FRAME_RATE_HZ (CONFIG_AUDIO_SAMPLE_RATE_HZ / SBC_FRAME_LEN_SAMPLES)
/* Frame size in bytes */
#define SBC_ENC_MONO_FRAME_SIZE (4 + (CONFIG_SBC_NO_OF_BLOCKS * \
	CONFIG_SBC_BITPOOL + 4 * CONFIG_SBC_NO_OF_SUBBANDS) / 8)
/* Bitrate in bits/s */
#define SBC_MONO_BITRATE (SBC_ENC_MONO_FRAME_SIZE * 8 * SBC_FRAME_RATE_HZ)
/* PCM frame size in bytes */
#define PCM_NUM_BYTES_SBC_FRAME_MONO ((SBC_FRAME_LEN_SAMPLES * CONFIG_AUDIO_BIT_DEPTH_BITS) / 8)
/* Size of PCM frames sent per BLE packet, in bytes */
#define SBC_PCM_NUM_BYTES_MONO \
	(PCM_NUM_BYTES_SBC_FRAME_MONO * CONFIG_SBC_NUM_FRAMES_PER_BLE_PACKET)
/* Encoded total frame size in bytes */
#define SBC_ENC_MAX_FRAME_SIZE \
	(SBC_ENC_MONO_FRAME_SIZE * CONFIG_SBC_NUM_FRAMES_PER_BLE_PACKET)

#else
#define SBC_ENC_MAX_FRAME_SIZE 0
#define SBC_PCM_NUM_BYTES_MONO 0
#define SBC_MONO_BITRATE 0
#endif /* CONFIG_SW_CODEC_SBC */

#if (CONFIG_SW_CODEC_LC3)
#define LC3_MAX_FRAME_SIZE_MS 10
#define LC3_ENC_MONO_FRAME_SIZE (CONFIG_LC3_MONO_BITRATE * LC3_MAX_FRAME_SIZE_MS / (8 * 1000))

#define LC3_PCM_NUM_BYTES_MONO                                                                     \
	(CONFIG_AUDIO_SAMPLE_RATE_HZ * CONFIG_AUDIO_BIT_DEPTH_OCTETS * LC3_MAX_FRAME_SIZE_MS / 1000)
#else
#define LC3_ENC_MONO_FRAME_SIZE 0
#define LC3_PCM_NUM_BYTES_MONO 0
#endif /* CONFIG_SW_CODEC_LC3 */

#define ENC_MAX_FRAME_SIZE MAX(LC3_ENC_MONO_FRAME_SIZE, SBC_ENC_MAX_FRAME_SIZE)
#define PCM_NUM_BYTES_MONO MAX(LC3_PCM_NUM_BYTES_MONO, SBC_PCM_NUM_BYTES_MONO)
#define PCM_NUM_BYTES_STEREO (PCM_NUM_BYTES_MONO * 2)

enum sw_codec_select {
	SW_CODEC_NONE,
	SW_CODEC_LC3, /**< Low Complexity Communication Codec */
	SW_CODEC_SBC, /**< Subband codec */
};

enum sw_codec_select_ch {
	SW_CODEC_ZERO_CHANNELS,
	SW_CODEC_MONO, /**< Only use one channel */
	SW_CODEC_STEREO, /**< Use both channels */
};

enum audio_channel {
	AUDIO_CH_L,
	AUDIO_CH_R,
	AUDIO_CH_NUM,
};

struct sw_codec_encoder {
	bool enabled;
	int bitrate;
	enum sw_codec_select_ch channel_mode;
	enum audio_channel audio_ch; /**< Only used if channel mode is mono */
};

struct sw_codec_decoder {
	bool enabled;
	enum sw_codec_select_ch channel_mode;
	enum audio_channel audio_ch; /**< Only used if channel mode is mono */
};

/** @brief  Sw_codec configuration structure
 */
struct sw_codec_config {
	enum sw_codec_select sw_codec; /**< sw_codec to be used, e.g. LC3, SBC etc */
	struct sw_codec_decoder decoder; /**< Struct containing settings for decoder */
	struct sw_codec_encoder encoder; /**< Struct containing settings for encoder */
	bool initialized; /**< Status of codec */
};

/**@brief	Encode PCM data and output encoded data
 *
 * @note	Takes in stereo PCM stream, will encode either one or two
 *		channels, based on channel_mode set during init
 *
 * @param[in]	pcm_data	Pointer to PCM data
 * @param[in]	pcm_size	Size of PCM data
 * @param[out]	encoded_data	Pointer to buffer to store encoded data
 * @param[out]	encoded_size	Size of encoded data
 *
 * @return	0 if success, error codes depends on sw_codec selected
 */
int sw_codec_encode(void *pcm_data, size_t pcm_size, uint8_t **encoded_data, size_t *encoded_size);

/**@brief	Decode encoded data and output PCM data
 *
 * @param[in]	encoded_data	Pointer to encoded data
 * @param[in]	encoded_size	Size of encoded data
 * @param[in]	bad_frame	Flag to indicate a missing/bad frame (only LC3)
 * @param[out]	pcm_data	Pointer to buffer to store decoded PCM data
 * @param[out]	pcm_size	Size of decoded data
 *
 * @return	0 if success, error codes depends on sw_codec selected
 */
int sw_codec_decode(uint8_t const *const encoded_data, size_t encoded_size, bool bad_frame,
		    void **pcm_data, size_t *pcm_size);

/**@brief	Uninitialize sw_codec and free allocated space
 *
 * @note	Must be called before calling init for another sw_codec
 *
 * @param[in]	sw_codec_cfg	Struct to tear down sw_codec
 *
 * @return	0 if success, error codes depends on sw_codec selected
 */
int sw_codec_uninit(struct sw_codec_config sw_codec_cfg);

/**@brief	Initialize sw_codec and statically or dynamically
 *		allocate memory to be used, depending on selected codec
 *		and its configuration.
 *
 * @param[in]	sw_codec_cfg	Struct to set up sw_codec
 *
 * @return	0 if success, error codes depends on sw_codec selected
 */
int sw_codec_init(struct sw_codec_config sw_codec_cfg);

#endif /* _SW_CODEC_SELECT_H_ */
