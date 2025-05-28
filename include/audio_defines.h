/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Globally accessible audio related defines
 *
 * @note: These defines are specific for Bluetooth Low Energy Audio
 */

#ifndef _AUDIO_DEFINES_H_
#define _AUDIO_DEFINES_H_

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

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

	/* Number of bytes per active location in the bitstream */
	uint32_t bytes_per_location;

	/* Number of bits used to carry a sample of size bits_per_sample.
	 * For example, say we have a 24 bit sample stored in a 32 bit
	 * word (int32_t), then:
	 *     bits_per_sample = 24
	 *     carrier_size    = 32
	 */
	uint8_t carried_bits_per_sample;

	/* The bit-rate for the data stream.
	 * For example, say we have a 16 bit sample stored in a 32 bit
	 * word (int_32), 100 samples in each of 2 locations and a sample rate of 48000 Hz then:
	 *     bitrate_bps = 32 * 48000 * 2
	 *                 = 3.072 Mbps
	 *
	 * For encoded data the bit-rate will be determined by the encoding process.
	 */
	uint32_t bitrate_bps;

	/* A flag to specify if the data is interleaved or de-interleaved
	 * (true = interleaved, false = de-interleaved).
	 *
	 * @note This flag is only valid for data types that support interleaving (e.g. PCM
	 * channel interleaving, where interleaved is LRLRLRLRL..... and de-interleaved is
	 * LLLLLRRRRRLL...
	 */
	bool interleaved;

	/* A 32 bit mask indicating which channel(s)/locations are active within
	 * the data. A bit set indicates the location is active and
	 * a count of these will give the number of locations within the
	 * audio stream.
	 * Note: This will follow the ANSI/CTA-861-G’s Table 34 codes
	 * for speaker placement (as used by Bluetooth Low Energy Audio).
	 *
	 * @note For Bluetooth Low Energy Audio a value of 0 indicates mono.
	 */
	uint32_t locations;

	/* Reference time stamp (e.g. ISO timestamp reference from BLE controller). */
	uint32_t ref_ts_us;

	/* The timestamp for when the data was received. */
	uint32_t data_rx_ts_us;

	/* A bit field to indicate if any channel has errors (1 = bad, 0 = good).
	 *
	 * Note: Timestamps are still valid when this is set.
	 */
	uint32_t bad_data;
};

/**
 * @brief Get the number of channels in the meta data.
 *
 * This function will count the number of bits set in the
 * locations field of the audio metadata.
 *
 * @param meta Pointer to the meta data structure.
 *
 * @return The number of channels.
 */
static inline uint8_t audio_metadata_num_ch_get(struct audio_metadata const *const meta)
{
	if (meta == NULL) {
		return 0;
	}

	if (meta->locations == 0) {
		return 1;
	}

	return POPCOUNT(meta->locations);
}

#endif /* _AUDIO_DEFINES_H_ */
