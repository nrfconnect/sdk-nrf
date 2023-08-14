/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Globally accessible audio related defines
 */

#ifndef _AUDIO_DEFINES_H_
#define _AUDIO_DEFINES_H_

#include <zephyr/types.h>
#include <stdbool.h>

/**
 * @brief Audio channel assignment values
 */
enum audio_channel {
	AUDIO_CH_L,
	AUDIO_CH_R,
	AUDIO_CH_NUM,
};

/**
 * @brief Audio data coding.
 */
enum coding {
	/* Raw PCM audio data */
	PCM = 1,

	/* LC3-coded audio data */
	LC3
};

struct audio_metadata {
	/* Indicates the data type/encoding. */
	enum coding data_coding;

	/* Playback length in microseconds. */
	uint32_t data_len_us;

	/* The audio sample rate.
	 * This may typically be 16000, 24000, 44100 or 48000 Hz.
	 */
	uint32_t sample_rate_hz;

	/* Number of valid bits for a sample (bit depth). Typically 16 or 24. */
	uint8_t bits_per_sample;

	/* Number of bits used to carry a sample of size bits_per_sample.
	 * For example, say we have a 24 bit sample stored in a 32 bit
	 * word (int32_t), then:
	 *     bits_per_sample = 24
	 *     carrier_size    = 32
	 */
	uint8_t carried_bits_pr_sample;

	/* A 32 bit mask indicating which channel(s)/locations are active within
	 * the data. A bit set indicates the location is active and
	 * a count of these will give the number of locations within the
	 * audio stream.
	 * Note: This will follow the ANSI/CTA-861-G’s Table 34 codes
	 * for speaker placement (as used by Bluetooth Low Energy Audio).
	 */
	uint32_t locations;

	/* Reference time stamp (e.g. ISO timestamp reference from BLE controller). */
	uint32_t reference_ts_us;

	/* The timestamp for when the data was received. */
	uint32_t data_rx_ts_us;

	/* A Boolean flag to indicate this data has errors
	 * (true = bad, false = good).
	 * Note: Timestamps are still valid even though this flag is set.
	 */
	bool bad_data;
};

/**
 * @brief A unit of audio.
 *
 * This unit can be used anywhere, so it may be an audio block, a frame or something else.
 * It may contain encoded or raw data, as well as a single or multiple channels.
 */
struct audio_data {
	/* A pointer to the raw or coded data (e.g., PCM, LC3, etc.) buffer. */
	void *data;

	/* The size in bytes of the data buffer.
	 * To get the size of each channel, this value must be divided by the number of
	 * used channels. Metadata is not included in this figure.
	 */
	size_t data_size;

	/* Additional information describing the audio data.
	 */
	struct audio_metadata meta;
};

#endif /* _AUDIO_DEFINES_H_ */
