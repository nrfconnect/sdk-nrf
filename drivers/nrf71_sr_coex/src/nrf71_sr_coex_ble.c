/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief BLE bad-channel mapping.
 *
 * Translates the connected-state Wi-Fi channel into the five-octet BLE Host
 * Channel Classification accepted by bt_le_set_chan_map(), so the Bluetooth
 * Controller stops using data channels that sit under the Wi-Fi carrier.
 *
 * Only the lower 37 bits of the map classify BLE data channels. A bit set to 1
 * means the channel is unknown and remains available to the Controller; 0 means
 * it is known to be bad. Bits 37 to 39 line up with the three advertising
 * channels, which this API cannot classify, and are always left clear.
 *
 * While this is enabled the coexistence driver owns the host channel
 * classification: on disconnect and on Wi-Fi admin down it restores the
 * all-channels map, which overwrites anything the application set itself.
 *
 * bt_le_set_chan_map() ends in a blocking HCI command, so it is never called
 * from the Wi-Fi state callback. The callback only stores the wanted map and
 * returns. A work item on a dedicated work queue applies it:
 *   - only the newest wanted map is kept, so a burst of Wi-Fi channel changes
 *     becomes one HCI command and no update is dropped;
 *   - the work item waits out the one-second minimum interval between two
 *     bt_le_set_chan_map() calls instead of rejecting an update.
 * The work queue is separate from the system work queue because some Bluetooth
 * host configurations process HCI events there, and a blocking HCI command from
 * that queue would then never complete.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "nrf71_sr_coex_internal.h"

LOG_MODULE_DECLARE(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

/** BLE data channels, indices 0 to 36. Advertising channels are not classified. */
#define BLE_NUM_DATA_CHANNELS 37U

/** Minimum number of data channels that must stay available to the Controller. */
#define BLE_MIN_GOOD_CHANNELS 2U

/** Minimum interval between two successive bt_le_set_chan_map() calls, in ms. */
#define BLE_CHAN_MAP_MIN_INTERVAL_MS 1000U

/** Wi-Fi centre frequencies above this are not in the 2.4 GHz band. */
#define WIFI_2G4_MAX_MHZ 2500U

/*
 * BLE channel centre frequencies. Data channels are split into two runs by
 * advertising channel 38 at 2426 MHz:
 *   data 0 to 10   2404 to 2424 MHz
 *   data 11 to 36  2428 to 2478 MHz
 */
#define BLE_DATA_CH_LOW_RUN_BASE_MHZ  2404U
#define BLE_DATA_CH_HIGH_RUN_BASE_MHZ 2428U
#define BLE_DATA_CH_LOW_RUN_LAST      10U
#define BLE_DATA_CH_SPACING_MHZ       2U

/** Each BLE channel occupies its centre frequency plus and minus 1 MHz. */
#define BLE_CH_HALF_WIDTH_MHZ 1U

/**
 * Guards the pending map, the applied map, apply_in_flight and the rate-limit
 * timestamp. Never held across bt_le_set_chan_map().
 */
static K_MUTEX_DEFINE(coex_ble_lock);

/** Map every data channel available, bits 37 to 39 clear: what "no Wi-Fi" means. */
static const uint8_t default_chan_map[CD_BLE_CHAN_MAP_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};

/** Newest map wanted by the Wi-Fi side, waiting for the work item to apply it. */
static uint8_t pending_chan_map[CD_BLE_CHAN_MAP_SIZE];
static bool pending_valid;

/** True while the work item is inside bt_le_set_chan_map(). */
static bool apply_in_flight;

/** Retries left for the map being applied; refilled by every new connect or clear. */
static unsigned int apply_retries_left;

/**
 * Last non-default map handed to the Controller, valid only after a call has
 * succeeded and until the default map is applied again.
 */
static uint8_t last_chan_map[CD_BLE_CHAN_MAP_SIZE];
static bool last_chan_map_valid;

/** Uptime in ms of the last successful bt_le_set_chan_map(), for rate limiting. */
static int64_t last_applied_uptime_ms;
static bool last_applied_ts_valid;

static void ble_apply_work_handler(struct k_work *work);

static K_THREAD_STACK_DEFINE(coex_ble_workq_stack, CONFIG_NRF71_SR_COEX_BLE_WORKQ_STACK_SIZE);
static struct k_work_q coex_ble_workq;
static K_WORK_DELAYABLE_DEFINE(coex_ble_work, ble_apply_work_handler);

/** Centre frequency of a BLE data channel, in MHz. */
static unsigned int ble_data_channel_freq_mhz(unsigned int channel)
{
	if (channel <= BLE_DATA_CH_LOW_RUN_LAST) {
		return BLE_DATA_CH_LOW_RUN_BASE_MHZ + (BLE_DATA_CH_SPACING_MHZ * channel);
	}

	return BLE_DATA_CH_HIGH_RUN_BASE_MHZ +
	       (BLE_DATA_CH_SPACING_MHZ * (channel - (BLE_DATA_CH_LOW_RUN_LAST + 1U)));
}

/** Count the data channels left available to the Controller in a map. */
static unsigned int ble_count_available_channels(const uint8_t *chan_map)
{
	unsigned int count = 0U;

	for (unsigned int channel = 0U; channel < BLE_NUM_DATA_CHANNELS; channel++) {
		if ((chan_map[channel / 8U] & (uint8_t)(1U << (channel % 8U))) != 0U) {
			count++;
		}
	}

	return count;
}

/** Build the BLE data-channel classification map for a Wi-Fi channel. */
static int ble_chan_map_from_wifi(unsigned int center_freq_mhz, unsigned int bandwidth_mhz,
				  uint8_t chan_map[CD_BLE_CHAN_MAP_SIZE])
{
	const unsigned int guard_mhz = CONFIG_NRF71_SR_COEX_BLE_GUARD_BAND_MHZ;
	unsigned int wifi_start_freq;
	unsigned int wifi_end_freq;
	unsigned int half_bandwidth;
	unsigned int available;

	memcpy(chan_map, default_chan_map, CD_BLE_CHAN_MAP_SIZE);

	/* Wi-Fi outside 2.4 GHz does not overlap the BLE band. */
	if (center_freq_mhz > WIFI_2G4_MAX_MHZ) {
		return 0;
	}

	half_bandwidth = bandwidth_mhz / 2U;
	if ((bandwidth_mhz == 0U) || (center_freq_mhz <= (half_bandwidth + guard_mhz))) {
		return -EINVAL;
	}

	wifi_start_freq = center_freq_mhz - half_bandwidth - guard_mhz;
	wifi_end_freq = center_freq_mhz + half_bandwidth + guard_mhz;

	for (unsigned int channel = 0U; channel < BLE_NUM_DATA_CHANNELS; channel++) {
		unsigned int centre = ble_data_channel_freq_mhz(channel);
		unsigned int ble_start_freq = centre - BLE_CH_HALF_WIDTH_MHZ;
		unsigned int ble_end_freq = centre + BLE_CH_HALF_WIDTH_MHZ;

		/* Boundary contact counts as overlap. */
		if ((ble_start_freq <= wifi_end_freq) && (ble_end_freq >= wifi_start_freq)) {
			chan_map[channel / 8U] &= (uint8_t)~(1U << (channel % 8U));
		}
	}

	/*
	 * A map that leaves the Controller almost nowhere to go is worse than no
	 * classification at all, so it is rejected rather than applied.
	 */
	available = ble_count_available_channels(chan_map);
	if (available < BLE_MIN_GOOD_CHANNELS) {
		LOG_ERR("BLE channel map leaves only %u data channel(s) available", available);
		return -EINVAL;
	}

	return 0;
}

/**
 * Milliseconds left before bt_le_set_chan_map() may be called again, 0 if it may
 * be called now. Must be called with coex_ble_lock held.
 */
static int64_t ble_remaining_interval_ms_locked(void)
{
	int64_t elapsed_ms;

	if (!last_applied_ts_valid) {
		return 0;
	}

	elapsed_ms = k_uptime_get() - last_applied_uptime_ms;
	if (elapsed_ms >= (int64_t)BLE_CHAN_MAP_MIN_INTERVAL_MS) {
		return 0;
	}

	return (int64_t)BLE_CHAN_MAP_MIN_INTERVAL_MS - elapsed_ms;
}

/** Queue the work item. Must be called with coex_ble_lock held. */
static void ble_schedule_locked(void)
{
	(void)k_work_reschedule_for_queue(&coex_ble_workq, &coex_ble_work,
					  K_MSEC(ble_remaining_interval_ms_locked()));
}

/** Apply the newest wanted map to the Bluetooth Controller. */
static void ble_apply_work_handler(struct k_work *work)
{
	uint8_t chan_map[CD_BLE_CHAN_MAP_SIZE];
	unsigned int retries_left = 0U;
	bool retry_scheduled = false;
	int ret;

	ARG_UNUSED(work);

	k_mutex_lock(&coex_ble_lock, K_FOREVER);

	if (!pending_valid) {
		k_mutex_unlock(&coex_ble_lock);
		return;
	}

	/*
	 * The interval can still be running if a call that finished after this
	 * item was queued moved the timestamp. Wait it out and keep the map pending.
	 */
	if (ble_remaining_interval_ms_locked() > 0) {
		ble_schedule_locked();
		k_mutex_unlock(&coex_ble_lock);
		return;
	}

	memcpy(chan_map, pending_chan_map, sizeof(chan_map));
	pending_valid = false;
	apply_in_flight = true;
	k_mutex_unlock(&coex_ble_lock);

	ret = bt_le_set_chan_map(chan_map);

	k_mutex_lock(&coex_ble_lock, K_FOREVER);

	apply_in_flight = false;
	if (ret == 0) {
		last_applied_uptime_ms = k_uptime_get();
		last_applied_ts_valid = true;
		apply_retries_left = 0U;
		if (memcmp(chan_map, default_chan_map, sizeof(chan_map)) == 0) {
			last_chan_map_valid = false;
		} else {
			memcpy(last_chan_map, chan_map, sizeof(last_chan_map));
			last_chan_map_valid = true;
		}
	} else if (!pending_valid && (apply_retries_left > 0U)) {
		/*
		 * The Controller did not take the map, typically because Bluetooth is
		 * not enabled yet when Wi-Fi connects. Keep the map wanted and try again
		 * later. A newer map (pending_valid) makes this retry pointless.
		 */
		apply_retries_left--;
		memcpy(pending_chan_map, chan_map, sizeof(pending_chan_map));
		pending_valid = true;
		retry_scheduled = true;
		retries_left = apply_retries_left;
	}

	if (retry_scheduled) {
		(void)k_work_reschedule_for_queue(
			&coex_ble_workq, &coex_ble_work,
			K_MSEC(CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP_RETRY_DELAY_MS));
	} else if (pending_valid) {
		/* A newer map arrived while the command was running. */
		ble_schedule_locked();
	}

	k_mutex_unlock(&coex_ble_lock);

	if (ret == 0) {
		LOG_DBG("BLE channel map applied: %02x %02x %02x %02x %02x", chan_map[0],
			chan_map[1], chan_map[2], chan_map[3], chan_map[4]);
		return;
	}

	if (ret < 0) {
		LOG_ERR("bt_le_set_chan_map() failed (%d)", ret);
	} else {
		/* A positive value is a Bluetooth HCI status. */
		LOG_ERR("bt_le_set_chan_map() failed: Bluetooth status 0x%02x", ret);
	}

	if (retry_scheduled) {
		LOG_WRN("Retrying the BLE channel map in %u ms, %u retries left after this",
			(unsigned int)CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP_RETRY_DELAY_MS,
			retries_left);
	}
}

void cd_ble_init(void)
{
	const struct k_work_queue_config wq_cfg = {
		.name = "coex_ble",
	};

	k_work_queue_start(&coex_ble_workq, coex_ble_workq_stack,
			   K_THREAD_STACK_SIZEOF(coex_ble_workq_stack),
			   K_LOWEST_APPLICATION_THREAD_PRIO, &wq_cfg);
}

void cd_ble_wifi_connected(unsigned int center_freq_mhz, unsigned int bandwidth_mhz)
{
	uint8_t chan_map[CD_BLE_CHAN_MAP_SIZE];
	int ret;

	ret = ble_chan_map_from_wifi(center_freq_mhz, bandwidth_mhz, chan_map);
	if (ret != 0) {
		LOG_WRN("No BLE channel map for Wi-Fi at %u MHz / %u MHz (%d)", center_freq_mhz,
			bandwidth_mhz, ret);
		return;
	}

	k_mutex_lock(&coex_ble_lock, K_FOREVER);
	memcpy(pending_chan_map, chan_map, sizeof(pending_chan_map));
	pending_valid = true;
	apply_retries_left = CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP_RETRIES;
	ble_schedule_locked();
	k_mutex_unlock(&coex_ble_lock);
}

void cd_ble_wifi_disconnected(void)
{
	k_mutex_lock(&coex_ble_lock, K_FOREVER);

	if (!last_chan_map_valid && !apply_in_flight) {
		/* The Controller is on its default map and nothing is on its way. */
		pending_valid = false;
		apply_retries_left = 0U;
		(void)k_work_cancel_delayable(&coex_ble_work);
		k_mutex_unlock(&coex_ble_lock);
		return;
	}

	memcpy(pending_chan_map, default_chan_map, sizeof(pending_chan_map));
	pending_valid = true;
	apply_retries_left = CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP_RETRIES;
	ble_schedule_locked();
	k_mutex_unlock(&coex_ble_lock);
}

bool cd_ble_last_chan_map(uint8_t chan_map[CD_BLE_CHAN_MAP_SIZE])
{
	bool valid;

	k_mutex_lock(&coex_ble_lock, K_FOREVER);
	valid = last_chan_map_valid;
	if (valid) {
		memcpy(chan_map, last_chan_map, CD_BLE_CHAN_MAP_SIZE);
	}
	k_mutex_unlock(&coex_ble_lock);

	return valid;
}
