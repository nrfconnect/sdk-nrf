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
 * Two entry points:
 *   coex_cd_ble_chan_map_from_wifi()  builds a map and nothing else
 *   coex_cd_wifi_channel_notify()     builds it and applies it to the Controller
 *
 * The split exists because the mapping is pure arithmetic that can be checked
 * against the specification without a Bluetooth stack, while applying it needs
 * a live Controller.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* CD2CM / CM2CD message structures shared with RPU firmware. */
#include <nrf71_coex_if.h>

/* CD to Short-Range driver interface. */
#include <nrf71_cd_sr_if.h>

#include <nrf71_sr_coex_api.h>

LOG_MODULE_DECLARE(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

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

/** Guards last_chan_map, last_chan_map_valid, and the rate-limit timestamp. */
static K_MUTEX_DEFINE(coex_ble_lock);

/** Last map handed to the Controller, valid only once a call has succeeded. */
static uint8_t last_chan_map[COEX_BLE_CHAN_MAP_SIZE];
static bool last_chan_map_valid;

/** Uptime in ms of the last successful bt_le_set_chan_map(), for rate limiting. */
static int64_t last_applied_uptime_ms;

/** Centre frequency of a BLE data channel, in MHz. */
static uint16_t ble_data_channel_freq_mhz(unsigned int channel)
{
	if (channel <= BLE_DATA_CH_LOW_RUN_LAST) {
		return (uint16_t)(BLE_DATA_CH_LOW_RUN_BASE_MHZ +
				  (BLE_DATA_CH_SPACING_MHZ * channel));
	}

	return (uint16_t)(BLE_DATA_CH_HIGH_RUN_BASE_MHZ +
			  (BLE_DATA_CH_SPACING_MHZ * (channel - (BLE_DATA_CH_LOW_RUN_LAST + 1U))));
}

/** Count the data channels left available to the Controller in a map. */
static unsigned int ble_count_available_channels(const uint8_t *chan_map)
{
	unsigned int count = 0U;

	for (unsigned int channel = 0U; channel < COEX_BLE_NUM_DATA_CHANNELS; channel++) {
		if ((chan_map[channel / 8U] & (uint8_t)(1U << (channel % 8U))) != 0U) {
			count++;
		}
	}

	return count;
}

/**
 * Build the five-octet BLE data-channel classification map for a Wi-Fi channel.
 *
 * Computation only: the map is not handed to the Bluetooth Controller.
 */
int coex_cd_ble_chan_map_from_wifi(const struct coex_wifi_channel_info_t *channel_info,
				   uint8_t chan_map[COEX_BLE_CHAN_MAP_SIZE])
{
	unsigned int wifi_start_freq;
	unsigned int wifi_end_freq;
	unsigned int half_bandwidth;
	unsigned int available;

	if ((channel_info == NULL) || (chan_map == NULL)) {
		return -EINVAL;
	}

	if ((channel_info->band != COEX_WIFI_BAND_2_4_GHZ) &&
	    (channel_info->band != COEX_WIFI_BAND_5_GHZ) &&
	    (channel_info->band != COEX_WIFI_BAND_6_GHZ)) {
		return -EINVAL;
	}

	if (channel_info->guard_band_mhz > COEX_BLE_MAX_GUARD_BAND_MHZ) {
		return -EINVAL;
	}

	/*
	 * Data channels 0 to 36 start out unknown/available. Bits 37 to 39 are
	 * the advertising channels and stay clear, which is why the top byte is
	 * seeded with 0x1F rather than 0xFF.
	 */
	memset(chan_map, 0xFF, COEX_BLE_CHAN_MAP_SIZE - 1U);
	chan_map[COEX_BLE_CHAN_MAP_SIZE - 1U] = 0x1FU;

	/*
	 * Wi-Fi on 5 GHz or 6 GHz does not overlap the BLE band, so every data
	 * channel stays available and the centre frequency is not needed.
	 */
	if (channel_info->band != COEX_WIFI_BAND_2_4_GHZ) {
		return 0;
	}

	if ((channel_info->center_frequency_mhz == 0U) || (channel_info->bandwidth_mhz == 0U)) {
		return -EINVAL;
	}

	/* Guarded Wi-Fi range: centre plus and minus half the bandwidth, then the guard band. */
	half_bandwidth = (unsigned int)channel_info->bandwidth_mhz / 2U;
	if (channel_info->center_frequency_mhz <=
	    (half_bandwidth + channel_info->guard_band_mhz)) {
		return -EINVAL;
	}

	wifi_start_freq = (unsigned int)channel_info->center_frequency_mhz - half_bandwidth -
			  channel_info->guard_band_mhz;
	wifi_end_freq = (unsigned int)channel_info->center_frequency_mhz + half_bandwidth +
			channel_info->guard_band_mhz;

	for (unsigned int channel = 0U; channel < COEX_BLE_NUM_DATA_CHANNELS; channel++) {
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
	if (available < COEX_BLE_MIN_GOOD_CHANNELS) {
		LOG_ERR("BLE channel map leaves only %u data channel(s) available", available);
		return -EINVAL;
	}

	return 0;
}

/**
 * Wi-Fi connected channel or band change notification from the Wi-Fi driver.
 *
 * Recomputes the BLE data-channel classification and applies it through
 * bt_le_set_chan_map().
 */
int coex_cd_wifi_channel_notify(const struct coex_wifi_channel_info_t *channel_info)
{
	uint8_t chan_map[COEX_BLE_CHAN_MAP_SIZE];
	int64_t now_ms;
	int ret;

	ret = coex_cd_ble_chan_map_from_wifi(channel_info, chan_map);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&coex_ble_lock, K_FOREVER);

	/*
	 * The Bluetooth host requires at least one second between successive
	 * classifications. Rejecting is preferable to blocking, because this
	 * runs on the Wi-Fi driver's notification path.
	 */
	now_ms = k_uptime_get();
	if (last_chan_map_valid &&
	    ((now_ms - last_applied_uptime_ms) < (int64_t)COEX_BLE_CHAN_MAP_MIN_INTERVAL_MS)) {
		k_mutex_unlock(&coex_ble_lock);
		LOG_DBG("BLE channel map update rejected: minimum interval not elapsed");
		return -EBUSY;
	}

	ret = bt_le_set_chan_map(chan_map);
	if (ret != 0) {
		k_mutex_unlock(&coex_ble_lock);
		if (ret < 0) {
			LOG_ERR("bt_le_set_chan_map() failed: %s (%d)", strerror(-ret), ret);
			return ret;
		}

		/*
		 * A positive value is a Bluetooth HCI status. Every coex_cd_* API
		 * returns 0 or a negative errno, so the status is logged and
		 * reported as -EIO.
		 */
		LOG_ERR("bt_le_set_chan_map() failed: Bluetooth status 0x%02x", ret);
		return -EIO;
	}

	memcpy(last_chan_map, chan_map, sizeof(last_chan_map));
	last_chan_map_valid = true;
	last_applied_uptime_ms = now_ms;

	k_mutex_unlock(&coex_ble_lock);

	LOG_DBG("BLE channel map applied: %02x %02x %02x %02x %02x", chan_map[0], chan_map[1],
		chan_map[2], chan_map[3], chan_map[4]);

	return 0;
}

/** Return the last successfully applied channel map, or NULL if none. */
const uint8_t *coex_cd_get_last_ble_chan_map(void)
{
	const uint8_t *map;

	k_mutex_lock(&coex_ble_lock, K_FOREVER);
	map = last_chan_map_valid ? last_chan_map : NULL;
	k_mutex_unlock(&coex_ble_lock);

	return map;
}
