/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/bluetooth/hci.h>

// Constants
#define CHMAP_BT_CONN_CH_COUNT		      37
#define CHMAP_BLE_BITMASK_SIZE		      5
#define CHMAP_DEFAULT_DESIRED_ACTIVE_CHANNELS 30
#define CHMAP_DEFAULT_MIN_ACTIVE_CHANNELS     5
#define FILTER_EVALUATION_SAMPLE_COUNT	      2000
#define FILTER_EVALUATIONS_BEFORE_RESET_COUNT 3

// Algorithm weight parameters
struct chmap_filter_params {
	float w_1;			  // Weight for CRC errors
	float w_2;			  // Weight for RX timeouts
	float w_3;			  // Weight for old rating
	float rating_threshold;		  // Threshold for disabling channels
	uint16_t min_packets_per_channel; // Minimum packets needed for rating
};

// Channel data structure
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

// Function declarations
static void calculate_channel_ratings(struct chmap_instance *chmap_instance);
static void generate_channel_map(struct chmap_instance *chmap_instance);
static void reset_sample_counters(struct chmap_instance *chmap_instance);
static void set_default_parameters(struct chmap_filter_params *params);
static void decrement_cooldown_time_and_update_state(struct chmap_instance *chmap_instance);
static void add_random_channel_to_channel_map(struct chmap_instance *chmap_instance);
void channel_map_filter_set_preferences(struct chmap_instance *chmap_instance,
					const uint8_t desired_active_channels,
					const uint8_t min_active_channels);

LOG_MODULE_REGISTER(algo, LOG_LEVEL_INF);

// Initialize the channel map algorithm
void channel_map_filter_algo_init(struct chmap_instance *chmap_instance)
{
	memset(chmap_instance, 0, sizeof(struct chmap_instance));

	// Set default algorithm parameters
	set_default_parameters(&chmap_instance->parameters);

	// Set default preferences on filter
	channel_map_filter_set_preferences(chmap_instance, CHMAP_DEFAULT_DESIRED_ACTIVE_CHANNELS,
					   CHMAP_DEFAULT_MIN_ACTIVE_CHANNELS);

	// Initialize all channels as enabled with perfect rating
	float start_rating = 1.0;
	for (int i = 0; i < CHMAP_BT_CONN_CH_COUNT; i++) {
		chmap_instance->bt_channels[i].rating = start_rating;
		chmap_instance->bt_channels[i].prev_rating = start_rating;
		chmap_instance->bt_channels[i].state = 1;
		chmap_instance->bt_channels[i].cooldown_time_remaining = 0;
	}

	// Initialize channel map with all channels enabled
	memset(chmap_instance->current_chn_bitmask, 0xFF, CHMAP_BLE_BITMASK_SIZE);
	memset(chmap_instance->suggested_chn_bitmask, 0xFF, CHMAP_BLE_BITMASK_SIZE);

	chmap_instance->active_channel_count = CHMAP_BT_CONN_CH_COUNT;
	chmap_instance->processed_samples_count = 0;
	chmap_instance->evaluations_since_last_evaluation = 0;
}

void print_channel_stats(struct chmap_instance *chmap_instance)
{
	if (!chmap_instance) {
		LOG_INF("ERROR: chmap_instance is NULL!\n");
		return;
	}

	LOG_INF("\n--- Algorithm Channel Report ---");
	LOG_INF("Ch | Total_packets_sent  | CRC_Errors | RX_Timeouts | Rating Made | Last Rating | "
		"State");
	LOG_INF("---|---------------------|------------|-------------|-------------|-------------|-"
		"------");

	for (int i = 0; i < CHMAP_BT_CONN_CH_COUNT; i++) {
		struct chmap_channel channel_data = chmap_instance->bt_channels[i];
		float rating_used_this_sample = channel_data.rating;
		float rating_used_prev_sample = channel_data.prev_rating;

		LOG_INF("%2d | %19d | %10d | %11d | %11.3f | %11.3f | %d", i,
			channel_data.total_packets_sent, channel_data.total_crc_errors,
			channel_data.total_rx_timeouts, (double)rating_used_this_sample,
			(double)rating_used_prev_sample, channel_data.state);
	}
}

// Check if algorithm is ready for evaluation and perform it
int channel_map_filter_algo_evaluate(struct chmap_instance *chmap_instance)
{
	if (!chmap_instance) {
		return -1;
	}

	if (chmap_instance->processed_samples_count < FILTER_EVALUATION_SAMPLE_COUNT) {
		return 0;
	}

	// Decrement cooldown_time_remaning for channels and update channel state
	decrement_cooldown_time_and_update_state(chmap_instance);

	// Calculate new ratings for all channels, also gives channels cooldown state if rating <
	// threshold
	calculate_channel_ratings(chmap_instance);

	// Generate new channel map based on ratings
	generate_channel_map(chmap_instance);

	// Add random channel/s that is done with cooldown to suggested channel map if we are
	// running low on channels
	if (chmap_instance->active_channel_count < chmap_instance->desired_active_channels) {
		add_random_channel_to_channel_map(chmap_instance);
	}

	if (chmap_instance->evaluations_since_last_evaluation >
	    FILTER_EVALUATIONS_BEFORE_RESET_COUNT) {
		// Uncomment for verbose printing every channel eval:
		print_channel_stats(chmap_instance);

		// Reset counters for next evaluation period
		reset_sample_counters(chmap_instance);
	}

	printk("Channel map evaluation completed successfully");
	printk("\nActive channels: %d\n", chmap_instance->active_channel_count);

	chmap_instance->evaluations_since_last_evaluation++;
	return 1; // Evaluation performed
}

// TODO: Test dynamically adaptive alternatives
// Decrement cooldown_time for channel, and update state if cooldown_time is over
static void decrement_cooldown_time_and_update_state(struct chmap_instance *chmap_instance)
{
	for (int i = 0; i < CHMAP_BT_CONN_CH_COUNT; i++) {
		if (chmap_instance->bt_channels[i].cooldown_time_remaining < 1) {
			continue;
		}
		chmap_instance->bt_channels[i].cooldown_time_remaining--;
		// If decrementation lead to cooldown_time_remaining = 0, update state too
		if (chmap_instance->bt_channels[i].cooldown_time_remaining == 0 &&
		    chmap_instance->bt_channels[i].state == 2) {
			chmap_instance->bt_channels[i].state = 0;
		}
	}
}

// Pick and add random channel(s) with state=0 (channels where cooldown_time done) back to channel
// map
static void add_random_channel_to_channel_map(struct chmap_instance *chmap_instance)
{
	// Count number of channels with state 0.
	uint8_t candidates[CHMAP_BT_CONN_CH_COUNT];
	size_t candidate_count = 0;

	for (int i = 0; i < CHMAP_BT_CONN_CH_COUNT; i++) {
		if (chmap_instance->bt_channels[i].state == 0) {
			candidates[candidate_count++] = i;
		}
	}

	if (candidate_count == 0) {
		return;
	}

	// Decide how many channels should be added to the channel map
	int add_channel_to_channel_map_count;
	if (chmap_instance->active_channel_count > 20) {
		add_channel_to_channel_map_count = 1;
	} else if (chmap_instance->active_channel_count > 10) {
		add_channel_to_channel_map_count = 2;
	} else {
		add_channel_to_channel_map_count = 3;
	}

	if (add_channel_to_channel_map_count > (int)candidate_count) {
		add_channel_to_channel_map_count = candidate_count;
	}

	// Pick a random random channel witstate = 0 and add it ti the channel map.
	for (int k = 0; k < add_channel_to_channel_map_count; k++) {
		uint32_t r = k + (sys_rand32_get() % (candidate_count - k));

		uint8_t tmp = candidates[k];
		candidates[k] = candidates[r];
		candidates[r] = tmp;
		uint8_t ch_idx = candidates[k];

		chmap_instance->bt_channels[ch_idx].state = 1;
		// update so prev rating is correct:
		chmap_instance->bt_channels[ch_idx].prev_rating =
			chmap_instance->bt_channels[ch_idx].rating;
		chmap_instance->active_channel_count++;

		uint8_t byte = ch_idx / 8;
		uint8_t bit = ch_idx % 8;
		chmap_instance->suggested_chn_bitmask[byte] |= (1 << bit);
	}
}

// Get the suggested channel map
uint8_t *channel_map_filter_algo_get_channel_map(struct chmap_instance *chmap_instance)
{
	if (!chmap_instance) {
		return NULL;
	}

	return chmap_instance->suggested_chn_bitmask;
}

void channel_map_filter_set_preferences(struct chmap_instance *chmap_instance,
					const uint8_t desired_active_channels,
					const uint8_t min_active_channels)
{
	if (!chmap_instance) {
		return;
	}

	if (desired_active_channels < min_active_channels || desired_active_channels > 40 ||
	    min_active_channels < 3) {
		printk("Desired filter preferences are out of bounds, pivoting to default values");
		chmap_instance->desired_active_channels = 35;
		chmap_instance->min_active_channels = 3;
	}

	else {
		chmap_instance->desired_active_channels = desired_active_channels;
		chmap_instance->min_active_channels = min_active_channels;
	}
}

// Set algorithm parameters
void channel_map_filter_algo_set_parameters(struct chmap_instance *chmap_instance,
					    const struct chmap_filter_params *params)
{
	if (!chmap_instance || !params) {
		return;
	}

	if (params->w_1 < 0.0f || params->w_1 > 1.0f) {
		printk("Invalid w_1 value: %f, clamping to 0.0", (double)params->w_1);
		return;
	} else if (params->w_2 < 0.0f || params->w_2 > 1.0f) {
		printk("Invalid w_2 value: %f, clamping to 0.0", (double)params->w_2);
		return;
	} else if (params->w_1 + params->w_2 != 1.0f) {
		printk("Invalid combination of w_1 and w_2. The sum of them should be 1.0 (w1: "
		       "%f, w2: %f)",
		       (double)params->w_1, (double)params->w_2);
		return;
	}

	chmap_instance->parameters = *params;
}

static void set_default_parameters(struct chmap_filter_params *params)
{
	params->w_1 = 0.15f;		      // Weight for CRC errors
	params->w_2 = 0.85f;		      // Weight for RX timeouts
	params->w_3 = 0.85f;		      // Weight for old rating
	params->rating_threshold = 0.8f;      // Disable channels below this rating
	params->min_packets_per_channel = 10; // Minimum packets needed for rating
}

static void calculate_channel_ratings(struct chmap_instance *chmap_instance)
{
	for (int i = 0; i < CHMAP_BT_CONN_CH_COUNT; i++) {
		struct chmap_channel *channel = &chmap_instance->bt_channels[i];
		uint16_t packets_sent = chmap_instance->bt_channels[i].total_packets_sent;

		// Skip channels where state != 1
		if (channel->state != 1) {
			continue;
		}

		// Skip channels with insufficient data
		if (packets_sent < chmap_instance->parameters.min_packets_per_channel) {
			continue;
		}

		if (packets_sent == 0) {
			printk("Zero packets sent for channel %d", i);
			continue;
		}

		// Calculate error rates
		float crc_error_rate = (float)channel->total_crc_errors / packets_sent;
		float rx_timeout_rate = (float)channel->total_rx_timeouts / packets_sent;

		float new_rating =
			(1.0f - chmap_instance->parameters.w_3) * channel->prev_rating +
			chmap_instance->parameters.w_3 *
				(1.0f - (chmap_instance->parameters.w_1 * crc_error_rate +
					 chmap_instance->parameters.w_2 * rx_timeout_rate));

		// Clamp rating between 0 and 1
		if (new_rating < 0.0f) {
			printk("Rating below 0.0 (%f), clamping to 0.0", (double)new_rating);
			new_rating = 0.0f;
		} else if (new_rating > 1.0f) {
			printk("Rating above 1.0 (%f), clamping to 1.0", (double)new_rating);
			new_rating = 1.0f;
		}

		channel->prev_rating = channel->rating;

		// Update rating
		channel->rating = new_rating;

		// update channel state, should be set to cooldown if rating below threshold.
		if (channel->rating < chmap_instance->parameters.rating_threshold &&
		    channel->state == 1) {
			channel->state = 2;
			channel->cooldown_time_remaining = 2;
		}
	}
}

static void generate_channel_map(struct chmap_instance *chmap_instance)
{
	// Clear suggested channel map
	memset(chmap_instance->suggested_chn_bitmask, 0, CHMAP_BLE_BITMASK_SIZE);

	uint8_t active_count = 0;

	// Enable channels above threshold
	for (int i = 0; i < CHMAP_BT_CONN_CH_COUNT; i++) {
		// Skip channel if channel has cooldown or it is disabled
		if (chmap_instance->bt_channels[i].state == 2 ||
		    chmap_instance->bt_channels[i].state == 0) {
			continue;
		}
		if (chmap_instance->bt_channels[i].rating >=
		    chmap_instance->parameters.rating_threshold) {
			int byte_index = i / 8;
			int bit_index = i % 8;
			chmap_instance->suggested_chn_bitmask[byte_index] |= (1 << bit_index);
			chmap_instance->bt_channels[i].state = 1;
			active_count++;
		} else {
			chmap_instance->bt_channels[i].state = 0;
		}
	}

	// Ensure minimum number of active channels
	if (active_count < chmap_instance->min_active_channels) {
		printk("Warning: Too few active channels (%d), enabling best channels\n",
		       active_count);

		// Find and enable the best channels, possible that channels that are in cooldown
		// gets enabled
		for (int needed = chmap_instance->min_active_channels - active_count; needed > 0;
		     needed--) {
			float best_rating = -1.0f;
			int best_channel = -1;

			for (int i = 0; i < chmap_instance->min_active_channels; i++) {
				if ((chmap_instance->bt_channels[i].state == 0 ||
				     chmap_instance->bt_channels[i].state == 2) &&
				    chmap_instance->bt_channels[i].rating > best_rating) {
					best_rating = chmap_instance->bt_channels[i].rating;
					best_channel = i;
				}
			}

			if (best_channel >= 0) {
				int byte_index = best_channel / 8;
				int bit_index = best_channel % 8;
				chmap_instance->suggested_chn_bitmask[byte_index] |=
					(1 << bit_index);
				chmap_instance->bt_channels[best_channel].state = 1;
				active_count++;
			}
		}
	}

	chmap_instance->active_channel_count = active_count;
}

static void reset_sample_counters(struct chmap_instance *chmap_instance)
{
	for (int i = 0; i < CHMAP_BT_CONN_CH_COUNT; i++) {
		chmap_instance->bt_channels[i].total_packets_sent = 0;
		chmap_instance->bt_channels[i].total_crc_ok = 0;
		chmap_instance->bt_channels[i].total_crc_errors = 0;
		chmap_instance->bt_channels[i].total_rx_timeouts = 0;
	}

	chmap_instance->processed_samples_count = 0;
	chmap_instance->evaluations_since_last_evaluation = 0;
}

void set_suggested_bitmask_to_current_bitmask(struct chmap_instance *chmap_instance)
{
	if (!chmap_instance) {
		printk("chmap_instance is NULL");
		return;
	}

	uint8_t *new_channel_map = channel_map_filter_algo_get_channel_map(chmap_instance);
	if (!new_channel_map) {
		printk("new_channel_map is NULL");
		return;
	}

	memcpy(chmap_instance->current_chn_bitmask, new_channel_map, CHMAP_BLE_BITMASK_SIZE);
}
