/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FILTER_ALGO_H
#define FILTER_ALGO_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file
 * @defgroup Channel_Map_Update Definitions for the Autonomous Map update sample.
 * @{
 * @brief Definitions for the Autonomous Map update sample.
 *
 * This file contains common definitions and API declarations
 * used by the sample.
 */

// Constants
#define CHMAP_BT_CONN_CH_COUNT 37
#define CHMAP_BLE_BITMASK_SIZE 5

// Forward declarations
struct chmap_instance;
struct chmap_filter_params;
struct chmap_qos_sample;

// Algorithm weight parameters
struct chmap_filter_params {
	float w_1;			  // Weight for CRC errors
	float w_2;			  // Weight for RX timeouts
	float w_3;			  // Weight for old rating
	float rating_threshold;		  // Threshold for disabling channels
	uint16_t min_packets_per_channel; // Minimum packets needed for rating
};

// QoS sample structure
struct chmap_qos_sample {
	uint8_t channel_index;
	uint16_t packets_sent;
	uint16_t crc_ok_count;
	uint16_t crc_error_count;
	uint16_t rx_timeout_count;
};

// Channel data structure (public parts)
struct chmap_channel {
	float rating;
	float prev_rating;
	uint8_t state;
	uint8_t cooldown_time_remaining; // 0=disabled, 1=enabled, 2=cooldown
	uint16_t total_packets_sent;
	uint16_t total_crc_ok;
	uint16_t total_crc_errors;
	uint16_t total_rx_timeouts;
};

// Main channel map instance
struct chmap_instance {
	struct chmap_filter_params parameters;
	struct chmap_channel bt_channels[CHMAP_BT_CONN_CH_COUNT];
	uint8_t desired_active_channels;
	uint8_t min_active_channels;
	uint8_t active_channel_count;
	uint8_t suggested_chn_bitmask[CHMAP_BLE_BITMASK_SIZE];
	uint8_t current_chn_bitmask[CHMAP_BLE_BITMASK_SIZE];
	uint16_t processed_samples_count;
	uint8_t evaluations_since_last_evaluation;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the channel map algorithm
 *
 * @param chmap_instance Pointer to the channel map instance
 */
void channel_map_filter_algo_init(struct chmap_instance *chmap_instance);

/**
 * @brief Check if algorithm is ready for evaluation and perform it
 *
 * @param chmap_instance Pointer to the channel map instance
 * @return 1 if evaluation was performed, 0 if not ready, negative on error
 */
int channel_map_filter_algo_evaluate(struct chmap_instance *chmap_instance);

/**
 * @brief Get the suggested channel map
 *
 * @param chmap_instance Pointer to the channel map instance
 * @return Pointer to the suggested channel map bitmask (5 bytes)
 */
uint8_t *channel_map_filter_algo_get_channel_map(struct chmap_instance *chmap_instance);

/**
 * @brief Set algorithm parameters
 *
 * @param chmap_instance Pointer to the channel map instance
 * @param params Pointer to the new parameters
 */
void channel_map_filter_algo_set_parameters(struct chmap_instance *chmap_instance,
					    const struct chmap_filter_params *params);

/**
 * @brief Set algorithm preferences, falls back to defaults if not used
 *
 * @param chmap_instance Pointer to the channel map instance
 * @param desired_active_channels The desired amount of channels the filter should operate at
 * @param min_active_channels The minimum amount of channels that have to remain non-blocked
 */
void channel_map_filter_set_preferences(struct chmap_instance *chmap_instance,
					const uint8_t desired_active_channels,
					const uint8_t min_active_channels);

/**
 * @brief Helper function to create a QoS sample
 *
 * @param sample Pointer to the sample structure to fill
 * @param channel_index Channel index (0-36)
 * @param packets_sent Number of packets sent
 * @param crc_ok_count Number of packets with CRC OK
 * @param crc_error_count Number of packets with CRC errors
 * @param rx_timeout_count Number of RX timeouts
 */
static inline void
channel_map_filter_algo_create_sample(struct chmap_qos_sample *sample, uint8_t channel_index,
				      uint16_t packets_sent, uint16_t crc_ok_count,
				      uint16_t crc_error_count, uint16_t rx_timeout_count)
{
	sample->channel_index = channel_index;
	sample->packets_sent = packets_sent;
	sample->crc_ok_count = crc_ok_count;
	sample->crc_error_count = crc_error_count;
	sample->rx_timeout_count = rx_timeout_count;
}

/**
 * @brief Function to reset the counters (data) used in a sample
 *
 * @param chmap_instance Pointer to the channel map instance

void reset_sample_counters(struct chmap_instance *chmap_instance);
*/

/**
 * @brief Function to set the suggested channel bitmask to current channel bitmask
 *
 * @param chmap_instance Pointer to the channel map instance
 */
void set_suggested_bitmask_to_current_bitmask(struct chmap_instance *chmap_instance);

#ifdef __cplusplus
}
#endif

#endif // FILTER_ALGO_H