/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_COMMON_SETTINGS_H
#define DECT_COMMON_SETTINGS_H

#include <zephyr/kernel.h>

#define DECT_PHY_SETT_TREE_KEY		"dect_phy_settings"
#define DECT_PHY_SETT_COMMON_CONFIG_KEY "common_settings"

/************************************************************************************************/

#define DECT_PHY_DEFAULT_NETWORK_ID			0x12345678
#define DECT_PHY_DEFAULT_TRANSMITTER_LONG_RD_ID		38
#define DECT_PHY_API_SCHEDULER_OFFSET_US		(10000)

#define DECT_PHY_SETT_DEFAULT_BEACON_TX_INTERVAL_SECS	2

#define DECT_PHY_SETT_DEFAULT_TX_POWER_DBM		-12
#define DECT_PHY_SETT_DEFAULT_TX_MCS			0

#define DECT_PHY_SETT_DEFAULT_RX_EXPECTED_RSSI_LEVEL	0 /* fast AGC (Automatic Gain Control) */

/************************************************************************************************/

/* ch 5.1.2 and Table 7.2-1 (MAC spec) */

/* "busy" if max(RSSI-1) > DECT_PHY_SETT_DEFAULT_RSSI_SCAN_THRESHOLD_MAX */
#define DECT_PHY_SETT_DEFAULT_RSSI_SCAN_THRESHOLD_MAX -71

/* "possible" if DECT_PHY_SETT_DEFAULT_RSSI_SCAN_THRESHOLD_MIN < max(RSSI-1) ≤
 * DECT_PHY_SETT_DEFAULT_RSSI_SCAN_THRESHOLD_MAX
 */

/* free" if max(RSSI-1) ≤ DECT_PHY_SETT_DEFAULT_RSSI_SCAN_THRESHOLD_MIN*/
#define DECT_PHY_SETT_DEFAULT_RSSI_SCAN_THRESHOLD_MIN -85

/************************************************************************************************/
struct dect_phy_settings_common_tx {
	int32_t power_dbm;
	uint8_t mcs;
};

struct dect_phy_settings_common_rx {
	int32_t expected_rssi_level;
};

struct dect_phy_settings_harq {
	/* HARQ parameters for modem init */
	uint32_t mdm_init_harq_expiry_time_us;
	uint8_t mdm_init_harq_process_count;

	/* Following with HARQ (data TX side - waiting on HARQ feedback)
	 * only when scheduler harq_feedback_requested is set:
	 */

	/* A Gap between our TX and starting RX for receiving HARQ feedback */
	uint8_t harq_feedback_rx_delay_subslot_count;

	/* RX duration for receiving HARQ feedback for our TX */
	uint8_t harq_feedback_rx_subslot_count;

	/* Following with HARQ (data rx side - sending HARQ feedback): */

	/* Gap between RX and starting TX for sending HARQ feedback when requested */
	uint8_t harq_feedback_tx_delay_subslot_count;
};

struct dect_phy_settings_scheduler {
	/* Scheduling delay in microseconds */
	uint64_t scheduling_delay_us;
};

struct dect_phy_settings_rssi_scan {
	uint32_t time_per_channel_ms;
	int32_t free_threshold; /* if equal or less considered as free */
	/* rssi_scanning_busy_threshold <= possible < rssi_scanning_free_threshold*/
	int32_t busy_threshold; /* if higher considered as busy */
};
struct dect_phy_settings_common {
	uint32_t network_id;
	uint32_t transmitter_id;
	uint16_t band_nbr;
	uint16_t short_rd_id; /* Generated random value, cannot be set */
};

struct dect_phy_settings {
	struct dect_phy_settings_common common;
	struct dect_phy_settings_rssi_scan rssi_scan;

	struct dect_phy_settings_common_tx tx;
	struct dect_phy_settings_common_rx rx;

	/* Common HARQ settings */
	struct dect_phy_settings_harq harq;

	/* Common scheduler settings */
	struct dect_phy_settings_scheduler scheduler;
};

int dect_common_settings_defaults_set(void);
int dect_common_settings_read(struct dect_phy_settings *dect_common_sett);
struct dect_phy_settings *dect_common_settings_ref_get(void);

int dect_common_settings_write(struct dect_phy_settings *dect_common_sett);

#endif /* DECT_COMMON_SETTINGS_H */
