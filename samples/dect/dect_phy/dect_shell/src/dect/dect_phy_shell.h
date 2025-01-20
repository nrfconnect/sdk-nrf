/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_SHELL_H
#define DECT_PHY_SHELL_H

#include <zephyr/kernel.h>
#include <nrf_modem_dect_phy.h>

#include "dect_common.h"
#include "dect_common_settings.h"
#include "dect_phy_common.h"
#include "dect_app_time.h"

/************************************************************************************************/

enum dect_phy_common_role {
	DECT_PHY_COMMON_ROLE_NONE = 0,
	DECT_PHY_COMMON_ROLE_CLIENT,
	DECT_PHY_COMMON_ROLE_SERVER,
};

struct dect_phy_perf_params {
	enum dect_phy_common_role role;
	bool debugs;
	uint32_t destination_transmitter_id;
	uint16_t channel;
	int16_t duration_secs;
	int8_t tx_power_dbm;
	uint8_t tx_mcs;
	uint8_t slot_count;
	uint8_t subslot_gap_count;
	uint8_t slot_gap_count;
	uint64_t slot_gap_count_in_mdm_ticks; /* Actual used gap in modem ticks */
	int8_t expected_rx_rssi_level;

	bool use_harq;
	uint32_t mdm_init_harq_expiry_time_us;
	uint8_t mdm_init_harq_process_count;
	uint8_t client_harq_process_nbr_max;

	/* RX duration for receiving HARQ feedback for our TX */
	uint8_t client_harq_feedback_rx_subslot_count;

	/* Gap between TX and starting RX for receiving HARQ feedback */
	uint8_t client_harq_feedback_rx_delay_subslot_count;

	/* Gap between RX and starting TX for sending HARQ feedback when requested */
	uint8_t server_harq_feedback_tx_delay_subslot_count;

	/* Gap between HARQ feedback TX end and re-starting a server RX. */
	uint8_t server_harq_feedback_tx_rx_delay_subslot_count;
};

enum dect_phy_rf_tool_mode {
	DECT_PHY_RF_TOOL_MODE_NONE = 0,
	DECT_PHY_RF_TOOL_MODE_RX,
	DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS,
	DECT_PHY_RF_TOOL_MODE_TX,
	DECT_PHY_RF_TOOL_MODE_RX_TX,
};

struct dect_phy_rf_tool_params {
	uint32_t destination_transmitter_id;
	uint16_t channel;
	enum dect_phy_rf_tool_mode mode;
	enum dect_phy_rf_tool_mode peer_mode;
	int8_t tx_power_dbm;
	uint8_t tx_mcs;
	int8_t expected_rx_rssi_level; /* Receiver gain setting */
	bool find_rx_sync;	       /* Continuous RX until sync found and then
					* RX part of the frame is started from received RX STF time
					*/
	bool continuous; /* if false: single shot, i.e. only frame_repeat_count_intervals  */

	/* Radio frame structure
	 * interval with DECT_PHY_RF_TOOL_MODE_RX:
	 * rx_frame_start_offset + rx_subslot_count + rx_post_idle_subslot_count
	 * interval with DECT_PHY_RF_TOOL_MODE_TX:
	 * tx_frame_start_offset + tx_subslot_count + tx_post_idle_subslot_count
	 * interval with DECT_PHY_RF_TOOL_MODE_RX_TX:
	 * rx_frame_start_offset + rx_subslot_count + rx_post_idle_subslot_count + tx_subslot_count
	 * + tx_post_idle_subslot_count
	 */
	uint32_t frame_repeat_count;
	uint32_t frame_repeat_count_intervals;
	uint8_t rx_frame_start_offset;
	uint8_t rx_subslot_count;
	uint8_t rx_post_idle_subslot_count;
	uint8_t tx_frame_start_offset;
	uint8_t tx_subslot_count;
	uint8_t tx_post_idle_subslot_count;
};

struct dect_phy_ping_params {
	enum dect_phy_common_role role;
	bool debugs;
	uint16_t channel;

	uint32_t destination_transmitter_id;
	int16_t interval_secs;
	int16_t timeout_msecs;
	int32_t ping_count;
	int8_t tx_power_dbm;
	uint8_t tx_mcs;
	uint8_t tx_lbt_period_symbols;
	int8_t tx_lbt_rssi_busy_threshold_dbm;
	uint8_t slot_count;

	int8_t expected_rx_rssi_level;

	bool pwr_ctrl_automatic;
	uint8_t pwr_ctrl_pdu_expected_rx_rssi_level;

	bool rssi_reporting_enabled;

	bool use_harq;
};

/******************************************************************************/

#define DECT_PHY_SHELL_RSSI_SCAN_DEFAULT_DURATION_MS \
	((DECT_PHY_SETT_DEFAULT_BEACON_TX_INTERVAL_SECS * 1000) + 10)
struct dect_phy_rssi_scan_params {
	bool suspend_scheduler;
	bool only_allowed_channels; /* At band #1, scan only allowed channels per Harmonized std  */

	int8_t busy_rssi_limit;
	int8_t free_rssi_limit;

	uint32_t channel;
	uint32_t scan_time_ms;

	uint16_t interval_secs; /* Used for two purposes: actual scanning interval and
				 * and for reporting interval RX command.
				 */

	enum dect_phy_settings_rssi_scan_result_verdict_type result_verdict_type;
	struct dect_phy_settings_rssi_scan_verdict_type_subslot_params type_subslots_params;

	/* "hidden" params */
	bool stop_on_1st_free_channel; /* stop on 1st totally free channel */

	uint16_t dont_stop_on_this_channel; /* Considered as BUSY */
	bool dont_stop_on_nbr_channels;	    /* Neighbor channels considered as BUSY */
};
struct dect_phy_rx_cmd_params {
	uint32_t handle;
	uint32_t channel;
	uint32_t network_id;
	uint32_t duration_secs;
	int8_t expected_rssi_level;
	bool suspend_scheduler;

	bool ch_acc_use_all_channels;
	int8_t busy_rssi_limit;
	int8_t free_rssi_limit;
	uint16_t rssi_interval_secs;

	struct nrf_modem_dect_phy_rx_filter filter;
};

/**************************************************************************************************/

struct dect_phy_common_op_event_msgq_item {
	uint8_t id;
	void *data;
};

/************************************************************************************************/

#define DECT_PHY_COMMON_RX_CMD_HANDLE	 50
#define DECT_PHY_COMMON_RSSI_SCAN_HANDLE 60

#define DECT_PHY_RADIO_MODE_CONFIG_HANDLE 70

#define DECT_PHY_MAC_BEACON_LMS_RSSI_SCAN_HANDLE 99

#define DECT_PHY_MAC_BEACON_TX_HANDLE		 100
#define DECT_PHY_MAC_BEACON_RX_RACH_HANDLE_START 101
#define DECT_PHY_MAC_BEACON_RX_RACH_HANDLE_END	 999
#define DECT_PHY_MAC_BEACON_RX_RACH_HANDLE_IN_RANGE(x)                                             \
	(x >= DECT_PHY_MAC_BEACON_RX_RACH_HANDLE_START &&                                          \
	 x <= DECT_PHY_MAC_BEACON_RX_RACH_HANDLE_END)

#define DECT_PHY_MAC_BEACON_RA_RESP_TX_HANDLE 1000

#define DECT_PHY_MAC_CLIENT_RA_TX_HANDLE		1001
#define DECT_PHY_MAC_CLIENT_RA_TX_CONTINUOUS_HANDLE	1002
#define DECT_PHY_MAC_CLIENT_ASSOCIATION_TX_HANDLE	1003
#define DECT_PHY_MAC_CLIENT_ASSOCIATION_RX_HANDLE	1004
#define DECT_PHY_MAC_CLIENT_ASSOCIATION_REL_TX_HANDLE	1005
#define DECT_PHY_MAC_CLIENT_ASSOCIATED_BG_SCAN		1006
/* Following handles from DECT_PHY_MAC_CLIENT_ASSOCIATED_BG_SCAN + 1 to DECT_PHY_MAC_MAX_NEIGBORS
 * are reserved for associated bg scannings.
 */

#define DECT_PHY_PERF_TX_HANDLE_START 10000
#define DECT_PHY_PERF_TX_HANDLE_END   10049
#define DECT_PHY_PERF_TX_HANDLE_IN_RANGE(x)                                                        \
	(x >= DECT_PHY_PERF_TX_HANDLE_START && x <= DECT_PHY_PERF_TX_HANDLE_END)

#define DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_START 10050
#define DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_END   10099
#define DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_IN_RANGE(x)                                          \
	(x >= DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_START &&                                       \
	 x <= DECT_PHY_PERF_HARQ_FEEDBACK_RX_HANDLE_END)

#define DECT_PHY_PERF_RESULTS_REQ_TX_HANDLE  11000 /* Client requesting perf results */
#define DECT_PHY_PERF_RESULTS_RESP_TX_HANDLE 11001 /* Server responding perf results */

#define DECT_PHY_PERF_RESULTS_RESP_RX_HANDLE 11002 /* Client for receiving perf results */

#define DECT_PHY_PERF_SERVER_RX_HANDLE 12000

#define DECT_PHY_PING_CLIENT_TX_HANDLE	  13000 /* For sending ping req */
#define DECT_PHY_PING_CLIENT_RE_TX_HANDLE 13001 /* For re-sending ping req */
#define DECT_PHY_PING_CLIENT_RX_HANDLE	  13500 /* For receiving ping resp */

#define DECT_PHY_PING_SERVER_RX_HANDLE 14000 /* For receiving ping req */
#define DECT_PHY_PING_SERVER_TX_HANDLE 14500 /* For sending ping resp */

#define DECT_PHY_PING_RESULTS_REQ_TX_HANDLE  15000 /* Client requesting ping results */
#define DECT_PHY_PING_RESULTS_RESP_TX_HANDLE 15001 /* Server responding ping results */
#define DECT_PHY_PING_RESULTS_RESP_RX_HANDLE 15002 /* Client for receiving ping results*/

#define DECT_PHY_RF_TOOL_SYNCH_WAIT_RX_HANDLE 16000

#define DECT_PHY_RF_TOOL_RX_CONT_HANDLE 16001

#define DECT_PHY_RF_TOOL_RX_HANDLE_START 16002
#define DECT_PHY_RF_TOOL_RX_HANDLE_END	 16999
#define DECT_PHY_RF_TOOL_RX_HANDLE_IN_RANGE(x)                                                     \
	(x >= DECT_PHY_RF_TOOL_RX_HANDLE_START && x <= DECT_PHY_RF_TOOL_RX_HANDLE_END)

#define DECT_PHY_RF_TOOL_TX_HANDLE_START 17000
#define DECT_PHY_RF_TOOL_TX_HANDLE_END	 17999
#define DECT_PHY_RF_TOOL_TX_HANDLE_IN_RANGE(x)                                                     \
	(x >= DECT_PHY_RF_TOOL_TX_HANDLE_START && x <= DECT_PHY_RF_TOOL_TX_HANDLE_END)

/************************************************************************************************/

#endif /* DECT_PHY_SHELL_H */
