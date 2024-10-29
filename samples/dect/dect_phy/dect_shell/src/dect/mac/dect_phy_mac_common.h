/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_MAC_COMMON_H
#define DECT_PHY_MAC_COMMON_H

#include <zephyr/kernel.h>
#include <stdint.h>

/******************************************************************************/

struct dect_phy_mac_beacon_start_params {
	bool ch_acc_bypass_rssi_scan;
	bool ch_acc_rssi_scan_all;
	bool ch_acc_busy_channel_use;
	bool ch_acc_use_all_channels;

	uint16_t beacon_channel;
	int8_t tx_power_dbm;
};

struct dect_phy_mac_beacon_scan_params {
	uint32_t duration_secs;
	uint32_t channel;
	int32_t expected_rssi_level;
	bool clear_nbr_cache_before_scan;
	bool suspend_scheduler;

	int8_t busy_rssi_limit;
	int8_t free_rssi_limit;
	uint16_t rssi_interval_secs;
};

struct dect_phy_mac_rach_tx_params {
	uint32_t target_long_rd_id;
	uint8_t mcs;
	int8_t tx_power_dbm;

	char tx_data_str[DECT_DATA_MAX_LEN]; /* Note: cannot be that much on payload */
};

struct dect_phy_mac_associate_params {
	uint32_t target_long_rd_id;
	uint8_t mcs;
	int8_t tx_power_dbm;
};

/******************************************************************************/

#endif /* DECT_PHY_MAC_COMMON_H */
