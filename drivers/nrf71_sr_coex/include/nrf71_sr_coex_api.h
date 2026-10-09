/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Coexistence Driver (CD) APIs for Coexistence Manager (CM) configuration,
 *        runtime commands, BLE bad-channel mapping, and retained CM statistics.
 *
 * Each coex_cd_* API that talks to the CM posts a CD2CM command and blocks
 * until the matching CM2CD completion event is received before returning to the
 * caller.
 *
 * CD2CM command                  CM2CD completion event
 * -------------------------  ---------------------------------
 * CD2CM_ENABLE_COEXISTENCE       CM2CD_ENABLE_COEXISTENCE_EVENT
 * CD2CM_SET_PRIORITY_RANGES      CM2CD_SET_PRIORITY_RANGES_EVENT
 * CD2CM_UPDATE_COEX_USER_PARAMS  CM2CD_UPDATE_COEX_USER_PARAMS_EVENT
 * CD2CM_UPDATE_COEX_PARAMS       CM2CD_UPDATE_COEX_PARAMS_EVENT
 * CD2CM_GET_STATS                CM2CD_STATISTICS_EVENT
 */

#ifndef NRF71_SR_COEX_API_H__
#define NRF71_SR_COEX_API_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <nrf71_coex_if.h>
#include <nrf71_coex_hw_regs.h>

/** Largest blob accepted by coex_cd_update_coex_params_blob(), in bytes. */
#define CD2CM_COEX_PARAMS_MAX_BLOB_LEN 128U

/** Post CD2CM_ENABLE_COEXISTENCE and wait for CM2CD_ENABLE_COEXISTENCE_EVENT. */
int coex_cd_enable(bool enable);

/** Post CD2CM_SET_PRIORITY_RANGES and wait for CM2CD_SET_PRIORITY_RANGES_EVENT. */
int coex_cd_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range);

/** Post CD2CM_UPDATE_COEX_USER_PARAMS and wait for CM2CD_UPDATE_COEX_USER_PARAMS_EVENT. */
int coex_cd_update_user_params(const struct coex_user_params_t *user_params);

/**
 * Post CD2CM_UPDATE_COEX_PARAMS (default NRF_COEX_PARAMS blob) and wait for
 * CM2CD_UPDATE_COEX_PARAMS_EVENT.
 */
int coex_cd_update_coex_params(void);

/**
 * Post CD2CM_UPDATE_COEX_PARAMS with a caller-supplied blob and wait for
 * CM2CD_UPDATE_COEX_PARAMS_EVENT.
 */
int coex_cd_update_coex_params_blob(const uint8_t *blob, size_t blob_len);

/**
 * Post CD2CM_GET_STATS, wait for CM2CD_STATISTICS_EVENT, and retain the
 * latest statistics snapshot inside the driver.
 */
int coex_cd_get_stats(void);

/**
 * Copy the cm_stats_t retained from the last CM2CD_STATISTICS_EVENT into
 * stats.
 *
 * The copy is taken under the driver lock, so it is never torn by a statistics
 * event arriving concurrently.
 *
 * @retval 0        Snapshot copied.
 * @retval -EINVAL  stats is NULL.
 * @retval -ENODATA No statistics snapshot is retained.
 */
int coex_cd_get_last_stats(struct cm_stats_t *stats);

/**
 * Copy the patch statistics retained from the last CM2CD_STATISTICS_EVENT into
 * @p patch_stats.
 *
 * @retval 0        Snapshot copied.
 * @retval -EINVAL  patch_stats is NULL.
 * @retval -ENODATA The last statistics event carried no patch trailer, or no
 *                  statistics snapshot is retained.
 */
int coex_cd_get_last_patch_stats(struct cm_fsm_patch_stats_t *patch_stats);

/** Configure and enable COEXC hardware (host CCMALLOW / CCCONF programming). */
int coex_cd_configure_coexc(enum coex_antenna_cfg_type antenna_cfg_type);

/* ---- BLE bad-channel mapping ---- */

/** Octets in a BLE Host Channel Classification map. */
#define COEX_BLE_CHAN_MAP_SIZE       5U

/** BLE data channels, indices 0 to 36. Advertising channels are not classified. */
#define COEX_BLE_NUM_DATA_CHANNELS   37U

/** Minimum number of data channels that must stay available to the Controller. */
#define COEX_BLE_MIN_GOOD_CHANNELS   2U

/** Largest guard band accepted either side of the Wi-Fi channel, in MHz. */
#define COEX_BLE_MAX_GUARD_BAND_MHZ  3U

/** Minimum interval between two successive bt_le_set_chan_map() calls, in ms. */
#define COEX_BLE_CHAN_MAP_MIN_INTERVAL_MS 1000U

/** Wi-Fi operating band reported to the CD. */
enum coex_wifi_band_t {
	COEX_WIFI_BAND_2_4_GHZ = 0,
	COEX_WIFI_BAND_5_GHZ,
	COEX_WIFI_BAND_6_GHZ
};

/** Connected-state Wi-Fi channel description used for BLE channel classification. */
struct coex_wifi_channel_info_t {
	/** Operating band. */
	enum coex_wifi_band_t band;
	/** Channel centre frequency in MHz. */
	unsigned short center_frequency_mhz;
	/** Channel bandwidth in MHz. */
	unsigned short bandwidth_mhz;
	/** Guard band applied either side of the channel, 0 to 3 MHz. */
	unsigned char guard_band_mhz;
};

/**
 * Build the five-octet BLE data-channel classification map for a Wi-Fi channel.
 *
 * Computation only: the map is not handed to the Bluetooth Controller, which
 * makes the mapping verifiable without a running Bluetooth stack.
 */
int coex_cd_ble_chan_map_from_wifi(const struct coex_wifi_channel_info_t *channel_info,
				   uint8_t chan_map[COEX_BLE_CHAN_MAP_SIZE]);

/**
 * Wi-Fi connected channel or band change notification from the Wi-Fi driver.
 *
 * Recomputes the BLE data-channel classification and applies it through
 * bt_le_set_chan_map().
 *
 * @retval 0       Classification applied.
 * @retval -EINVAL Invalid channel description, or the map would leave fewer
 *                 than COEX_BLE_MIN_GOOD_CHANNELS data channels available.
 * @retval -EBUSY  Less than COEX_BLE_CHAN_MAP_MIN_INTERVAL_MS since the last
 *                 applied update.
 * @retval -EIO    The Bluetooth Controller rejected the command (HCI status).
 * @retval <0      Other negative errno from bt_le_set_chan_map().
 */
int coex_cd_wifi_channel_notify(const struct coex_wifi_channel_info_t *channel_info);

/**
 * Return the last successfully applied channel map, or NULL if none.
 *
 * The pointer refers to driver-owned storage that the next successful
 * coex_cd_wifi_channel_notify() overwrites.
 */
const uint8_t *coex_cd_get_last_ble_chan_map(void);

#endif /* NRF71_SR_COEX_API_H__ */
