/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SW_CODEC_SELECT_H_
#define _SW_CODEC_SELECT_H_

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include "channel_assignment.h"

#if (CONFIG_SW_CODEC_LC3)
#include "LC3API.h"

#define LC3_MAX_FRAME_SIZE_MS	10
#define LC3_ENC_MONO_FRAME_SIZE (CONFIG_LC3_BITRATE_MAX * LC3_MAX_FRAME_SIZE_MS / (8 * 1000))

#define LC3_PCM_NUM_BYTES_MONO                                                                     \
	(CONFIG_AUDIO_SAMPLE_RATE_HZ * CONFIG_AUDIO_BIT_DEPTH_OCTETS * LC3_MAX_FRAME_SIZE_MS / 1000)
#define LC3_ENC_TIME_US	 3000
#define LC3_DEC_TIME_US	 1500
#define LC3_CHANNELS_MAX 2
#else
#define LC3_ENC_MONO_FRAME_SIZE 0
#define LC3_PCM_NUM_BYTES_MONO	0
#define LC3_ENC_TIME_US		0
#define LC3_DEC_TIME_US		0
#define LC3_CHANNELS_MAX	0
#endif /* CONFIG_SW_CODEC_LC3 */

/* Max will be used when multiple codecs are supported */
#define ENC_MAX_FRAME_SIZE	      MAX(LC3_ENC_MONO_FRAME_SIZE, 0)
#define ENC_MULTI_CHAN_MAX_FRAME_SIZE (LC3_ENC_MONO_FRAME_SIZE * LC3_CHANNELS_MAX)
#define ENC_TIME_US		      MAX(LC3_ENC_TIME_US, 0)
#define DEC_TIME_US		      MAX(LC3_DEC_TIME_US, 0)
#define PCM_NUM_BYTES_MONO	      MAX(LC3_PCM_NUM_BYTES_MONO, 0)
#define PCM_NUM_BYTES_STEREO	      (PCM_NUM_BYTES_MONO * 2)
#define PCM_NUM_BYTES_MULTI_CHAN      (PCM_NUM_BYTES_MONO * LC3_CHANNELS_MAX)

enum sw_codec_select {
	SW_CODEC_NONE,
	SW_CODEC_LC3, /* Low Complexity Communication Codec */
};

enum sw_codec_channel_mode {
	SW_CODEC_MONO = 1,
	SW_CODEC_STEREO,
};

/**
 * @brief Number of micro seconds in a second.
 *
 */
#define LC3_DECODER_US_IN_A_SECOND (1000000)

/**
 * @brief Private pointer to the module's parameters.
 */
extern struct audio_module_description *lc3_decoder_description;

/**
 * @brief The module configuration structure.
 */
struct lc3_decoder_configuration {
	/* Sample rate for the decoder instance. */
	uint32_t sample_rate_hz;

	/* Number of valid bits for a sample (bit depth).
	 * Typically 16 or 24.
	 */
	uint8_t bits_per_sample;

	/* Number of bits used to carry a sample of size bits_per_sample.
	 * For example, say we have a 24 bit sample stored in a 32 bit
	 * word (int32_t), then:
	 *     bits_per_sample = 24
	 *     carrier_size    = 32
	 */
	uint32_t carried_bits_per_sample;

	/* Frame duration for this decoder instance. */
	uint32_t data_len_us;

	/* A flag indicating if the decoded buffer is sample interleaved or not. */
	bool interleaved;

	/* Channel locations for this decoder instance. */
	uint32_t locations;

	/* Maximum bitrate supported by the decoder. */
	uint32_t bitrate_bps_max;
};

/**
 * @brief  Private module context.
 */
struct lc3_decoder_context {
	/* Array of decoder channel handles. */
	struct lc3_decoder_handle *lc3_dec_channel[2];

	/* Number of decoder channel handles. */
	uint32_t dec_handles_count;

	/* The decoder configuration. */
	struct lc3_decoder_configuration config;

	/* Minimum coded bytes required for this decoder instance. */
	uint16_t coded_bytes_req;

	/* Audio sample bytes per frame. */
	size_t sample_frame_bytes;

	/* Number of successive frames to which PLC has been applied. */
	uint16_t plc_count;
};

struct sw_codec_encoder {
	bool enabled;
	int bitrate;
	enum sw_codec_channel_mode channel_mode;
	uint8_t num_ch;
	enum audio_channel audio_ch;
	uint32_t sample_rate_hz;
};

struct sw_codec_decoder {
	bool enabled;
	enum sw_codec_channel_mode channel_mode; /* Mono or stereo. */
	uint8_t num_ch;				 /* Number of decoder channels. */
	enum audio_channel audio_ch;		 /* Used to choose which channel to use. */
	uint32_t sample_rate_hz;
	struct lc3_decoder_context context;
};

/**
 * @brief  Sw_codec configuration structure.
 */
struct sw_codec_config {
	enum sw_codec_select sw_codec;	 /* sw_codec to be used, e.g. LC3, etc. */
	struct sw_codec_decoder decoder; /* Struct containing settings for decoder. */
	struct sw_codec_encoder encoder; /* Struct containing settings for encoder. */
	bool initialized;		 /* Status of codec. */
};

/**
 * @brief	Check if the software codec is initialized.
 *
 * @retval	true	SW codec is initialized.
 * @retval	false	SW codec is not initialized.
 */
bool sw_codec_is_initialized(void);

/**
 * @brief	Encode PCM data and output encoded data.
 *
 * @note	Takes in stereo PCM stream, will encode either one or two
 *		channels, based on channel_mode set during init.
 *
 * @param[in]	audio_frame_pcm	Pointer to the audio PCM buffer.
 * @param[out]	audio_frame_lc3	Pointer to the LC3 coded buffer.
 *
 * @return	0 if success, error codes depends on sw_codec selected.
 */
int sw_codec_encode(struct net_buf *audio_frame_pcm, struct net_buf *audio_frame_lc3);

/**
 * @brief	Decode encoded data and output PCM data.
 *
 * @param[in]	audio_frame_in	Pointer to the audio input buffer.
 * @param[in]	audio_frame_out	Pointer to the audio output buffer.
 *
 * @return	0 if success, error codes depends on sw_codec selected.
 */
int sw_codec_decode(struct net_buf const *const audio_frame_in,
		    struct net_buf *const audio_frame_out);

/**
 * @brief	Uninitialize the software codec and free the allocated space.
 *
 * @note	Must be called before calling init for another sw_codec.
 *
 * @param[in]	sw_codec_cfg	Struct to tear down sw_codec.
 *
 * @return	0 if success, error codes depends on sw_codec selected.
 */
int sw_codec_uninit(struct sw_codec_config sw_codec_cfg);

/**
 * @brief	Initialize the software codec and statically or dynamically
 *		allocate memory to be used, depending on the selected codec
 *		and its configuration.
 *
 * @param[in]	sw_codec_cfg	Struct to set up sw_codec.
 *
 * @return	0 if success, error codes depends on sw_codec selected.
 */
int sw_codec_init(struct sw_codec_config sw_codec_cfg);

/**
 * @brief Get the number of channels in the meta data.
 *
 * This function will count the number of bits set in the
 * locations field of the audio metadata. If the Bluetooth
 * LE audio special case of locations is zero, a mono channel
 * is indicated by returning the channel number as one.
 *
 * @param meta[in]	Pointer to the meta data structure.
 *
 * @return The number of channels.
 */
static inline uint8_t sw_codec_metadata_num_ch_get(struct audio_metadata const *const meta)
{
	if (meta == NULL) {
		return 0;
	}

	if (meta->locations == 0) {
		return 1;
	}

	return audio_metadata_num_ch_get(meta);
}

#endif /* _SW_CODEC_SELECT_H_ */
