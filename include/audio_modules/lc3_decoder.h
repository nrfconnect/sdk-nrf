/*
 * Copyright(c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef _LC3_DECODER_H_
#define _LC3_DECODER_H_

#include "audio_defines.h"
#include "audio_module.h"

#define LC3_DECODER_PCM_NUM_BYTES_MONO                                                             \
	((CONFIG_LC3_DECODER_SAMPLE_RATE_HZ * CONFIG_LC3_DECODER_BIT_DEPTH_OCTETS *                \
	  CONFIG_LC3_DECODER_FRAME_DURATION_US) /                                                  \
	 1000000)

/**
 * @brief Private pointer to the module's parameters.
 */
extern struct audio_module_description *lc3_decoder_description;

/**
 * @brief Private pointer to the decoders handle.
 */
struct lc3_decoder_handle;

/**
 * @brief The module configuration structure.
 */
struct lc3_decoder_configuration {
	/* Sample rate for the decoder instance */
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
	uint32_t carried_bits_pr_sample;

	/* Frame duration for this decoder instance */
	uint32_t data_len_us;

	/* A flag indicating if the decoded buffer is sample interleaved or not */
	bool interleaved;

	/* Active channel locations for this decoder instance */
	uint32_t locations;

	/* Maximum bitrate supported by the decoder. */
	uint32_t bitrate_bps_max;
};

/**
 * @brief  Private module context.
 */
struct lc3_decoder_context {
	/* Array of decoder channel handles */
	struct lc3_decoder_handle *lc3_dec_channel[CONFIG_LC3_DEC_CHANNELS_MAX];

	/* Number of decoder channel handles */
	uint32_t dec_handles_count;

	/* The decoder configuration */
	struct lc3_decoder_configuration config;

	/* Minimum coded bytes required for this decoder instance */
	uint16_t coded_bytes_req;

	/* Audio sample bytes per frame. */
	size_t sample_frame_bytes;

	/* Number of successive frames to which PLC has been applied */
	uint16_t plc_count;
};

#endif /* _LC3_DECODER_H_ */
