/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

 /**
  * @file
  * @brief DECT NR+ L2 Networking Layer public header.
  *
  */

#ifndef DECT_NET_L2_H_
#define DECT_NET_L2_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dect_net_l2 DECT NR+ L2 Networking Layer
 *
 * @brief DECT NR+ L2 Networking Layer public header.
 *
 * @details This header file contains the public API for the DECT NR+ L2 Networking Layer.
 *
 * Implements DECT NR+-aware IPv6 networking layer that integrates with Zephyr networking stack.
 * It provides intelligent packet routing based on DECT cluster topology and RD ID addressing.
 *
 * @{
 */

/**
 * @anchor DECT-MAC-SPEC
 * @brief DECT-2020 NR Part 4: MAC specification.
 * @details Links to pertaining specification and regulations.
 * - [DECT-2020 NR Part 4: MAC specification]
 * (https://www.etsi.org/deliver/etsi_ts/103600_103699/10363604/01.05.01_60/ts_10363604v010501p.pdf)
 */

 /** @cond INTERNAL_HIDDEN */

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

#define DECT_MTU 1280

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Common cause values.
 * @details These values are used to indicate the status of a DECT NR+ operation.
 */
enum dect_status_values {
	/** Success. */
	DECT_STATUS_OK = 0x0000,
	/** Generic failure. */
	DECT_STATUS_FAIL = 0x0001,
	/** Invalid parameter. */
	DECT_STATUS_INVALID_PARAM = 0x0002,
	/** Request not possible in current state. */
	DECT_STATUS_NOT_ALLOWED = 0x0003,
	/** Operation not possible due to missing configuration. */
	DECT_STATUS_NO_CONFIG = 0x0004,
	/** Given RD not found, operation cannot be completed. */
	DECT_STATUS_RD_NOT_FOUND = 0x0005,
	/** Unable to complete request due to temperature too high or low. */
	DECT_STATUS_TEMP_FAILURE = 0x0006,
	/**
	 * Unable to complete request due to missing radio resources to deliver the message to
	 * target RD.
	 */
	DECT_STATUS_NO_RESOURCES = 0x0007,
	/** Request failed due to no response from target RD. */
	DECT_STATUS_NO_RESPONSE = 0x0008,
	/** Request failed because target RD rejected it. */
	DECT_STATUS_NW_REJECT = 0x0009,
	/** Unable to complete request due to insufficient memory. */
	DECT_STATUS_NO_MEMORY = 0x000A,
	/** No RSSI scan results for the requested channel exists. */
	DECT_STATUS_NO_RSSI_RESULTS = 0x000B,
	/** TX request timeout. */
	DECT_STATUS_TX_TIMEOUT = 0x000C,

	/** OS error. */
	DECT_STATUS_OS_ERROR = 0xFFFE,

	/** Unknown error. */
	DECT_STATUS_UNKNOWN = 0xFFFF,
};

/** @brief Network beacon period.
 * @details Table 6.4.2.2-1 in @ref DECT-MAC-SPEC "DECT-2020 NR Part 4".
 */
enum dect_nw_beacon_period {
	DECT_NW_BEACON_PERIOD_50MS = 0,
	DECT_NW_BEACON_PERIOD_100MS = 1,
	DECT_NW_BEACON_PERIOD_500MS = 2,
	DECT_NW_BEACON_PERIOD_1000MS = 3,
	DECT_NW_BEACON_PERIOD_1500MS = 4,
	DECT_NW_BEACON_PERIOD_2000MS = 5,
	DECT_NW_BEACON_PERIOD_4000MS = 6,
	/* rest are reserved */
};

/** @brief Cluster beacon period.
 * @details Table 6.4.2.2-1 in @ref DECT-MAC-SPEC "DECT-2020 NR Part 4".
 */
enum dect_cluster_beacon_period {
	DECT_CLUSTER_BEACON_PERIOD_10MS = 0,
	DECT_CLUSTER_BEACON_PERIOD_50MS = 1,
	DECT_CLUSTER_BEACON_PERIOD_100MS = 2,
	DECT_CLUSTER_BEACON_PERIOD_500MS = 3,
	DECT_CLUSTER_BEACON_PERIOD_1000MS = 4,
	DECT_CLUSTER_BEACON_PERIOD_1500MS = 5,
	DECT_CLUSTER_BEACON_PERIOD_2000MS = 6,
	DECT_CLUSTER_BEACON_PERIOD_4000MS = 7,
	DECT_CLUSTER_BEACON_PERIOD_8000MS = 8,
	DECT_CLUSTER_BEACON_PERIOD_16000MS = 9,
	DECT_CLUSTER_BEACON_PERIOD_32000MS = 10,
	/* rest are reserved */
};

#define DECT_MAX_CHANNELS_IN_RSSI_SCAN	   20
#define DECT_MAX_CHANNELS_IN_NETWORK_SCAN_REQ  20
#define DECT_MAX_ADDITIONAL_NW_BEACON_CHANNELS 3

/** @brief RSSI scan parameters. */
struct dect_rssi_scan_params {
	uint8_t band;
	uint8_t frame_count_to_scan;
	uint8_t channel_count;
	uint16_t channel_list[DECT_MAX_CHANNELS_IN_RSSI_SCAN];
};

/** @brief Network scan parameters. */
struct dect_scan_params {
	uint8_t band;
	uint8_t channel_count;
	uint16_t channel_list[DECT_MAX_CHANNELS_IN_NETWORK_SCAN_REQ];
	uint16_t channel_scan_time_ms;
};

/** @brief Scan result event type. */
enum dect_scan_result_evt_type {
	DECT_SCAN_RESULT_TYPE_CLUSTER_BEACON = 0,
	DECT_SCAN_RESULT_TYPE_NW_BEACON,
};

/** @brief Network beacon data. */
struct dect_network_beacon_data {
	/** Current cluster channel. */
	uint16_t current_cluster_channel;

	/** Next cluster channel. */
	uint16_t next_cluster_channel;

	/** Number of additional network beacon channels included in beacon. */
	uint8_t num_network_beacon_channels;

	/** Network beacon channels. */
	uint16_t network_beacon_channels[DECT_MAX_ADDITIONAL_NW_BEACON_CHANNELS];
};

/** @brief Received signal information. */
struct dect_rx_signal_info {
	/** MCS index. @details See Table A-1 in @ref DECT-MAC-SPEC "DECT-2020 NR Part 4". */
	uint8_t mcs;
	/**
	 * Transmit power, [0,15].
	 * @details Table 6.2.1-3a in @ref DECT-MAC-SPEC "DECT-2020 NR Part 4".
	 */
	uint8_t transmit_power;
	/** Received signal strength indicator (RSSI-2) of PDC, @details value in dBm. */
	int8_t rssi_2;
	/** Received signal to noise indicator (SNR) of PDC, @details value in dB. */
	int8_t snr;
};

/** @brief Route info. */
struct dect_route_info {
	/** Route cost. */
	uint8_t route_cost;
	/** Sequence number of network level application data. */
	uint8_t application_sequence_number;
	/** Long RD ID of the FT at the top of the mesh tree (sink having internet connection). */
	uint32_t sink_address;
};

/**
 * @brief DECT scan result event, each result is provided to the net_mgmt_event_callback
 * via its info attribute (see net_mgmt.h)
 */
struct dect_scan_result_evt {
	/** Beacon type. */
	enum dect_scan_result_evt_type beacon_type;

	/** Reception channel number. */
	uint16_t channel;
	/** Transmitter Short Radio Device ID. */
	uint16_t transmitter_short_rd_id;
	/** Transmitter Long Radio Device ID. */
	uint32_t transmitter_long_rd_id;
	/** Network ID. */
	uint32_t network_id;

	/** Received signal information. */
	struct dect_rx_signal_info rx_signal_info;

	/* Network beacon data if beacon_type is DECT_SCAN_RESULT_TYPE_NW_BEACON. */
	struct dect_network_beacon_data network_beacon;

	/** Route info. */
	bool has_route_info;
	struct dect_route_info route_info;
};

/** @brief Scan result callback.
 *
 * @param iface Network interface.
 * @param status Scan result status.
 * @param entry Scan result entry.
 */
typedef void (*dect_scan_result_cb_t)(struct net_if *iface, enum dect_status_values status,
				      struct dect_scan_result_evt *entry);

/** @brief Associate request parameters. */
struct dect_associate_req_params {
	uint32_t target_long_rd_id;
};

/** @brief Associate release parameters. */
struct dect_associate_rel_params {
	uint32_t target_long_rd_id;
};

/** @brief Any suitable channel on set band. */
#define DECT_CLUSTER_CHANNEL_ANY 0

/** @brief Cluster start request parameters. */
struct dect_cluster_start_req_params {
	uint16_t channel;
};

/** @brief Cluster reconfiguration request parameters. */
struct dect_cluster_reconfig_req_params {
	uint16_t channel;
	int8_t max_beacon_tx_power_dbm;
	int8_t max_cluster_power_dbm;
	enum dect_cluster_beacon_period period;
};

/** @brief Cluster stop request parameters. */
struct dect_cluster_stop_req_params {
};

/** @brief Neighbor info request parameters. */
struct dect_neighbor_info_req_params {
	uint32_t long_rd_id;
};
/** @brief Network beacon start request parameters. */
struct dect_nw_beacon_start_req_params {
	uint16_t channel;
	uint8_t additional_ch_count;
	uint16_t additional_ch_list[DECT_MAX_ADDITIONAL_NW_BEACON_CHANNELS];
};

/** @brief Network beacon stop request parameters. */
struct dect_nw_beacon_stop_req_params {
};

/**
 * @brief Association reject causes.
 * @details Section 6.4.2.6 in @ref DECT-MAC-SPEC "DECT-2020 NR Part 4".
 */
enum dect_association_reject_cause {
	/** No radio capacity */
	DECT_ASSOCIATION_REJECT_CAUSE_NO_RADIO_CAPACITY = 0,
	/** No hardware capacity */
	DECT_ASSOCIATION_REJECT_CAUSE_NO_HW_CAPACITY = 1,
	/** Conflicted short ID */
	DECT_ASSOCIATION_REJECT_CAUSE_CONFLICTED_SHORT_ID = 2,
	/** Security needed */
	DECT_ASSOCIATION_REJECT_CAUSE_SECURITY_NEEDED = 3,
	/** Other reason */
	DECT_ASSOCIATION_REJECT_CAUSE_OTHER_REASON = 5,
	/** Other reasons than from @ref DECT-MAC-SPEC "DECT-2020 NR Part 4"  */
	DECT_ASSOCIATION_NO_RESPONSE = 6,
};

/**
 * @brief Association reject times.
 * @details Application must wait the time before re-attempt
 * the association with same parent.
 */
enum dect_association_reject_time {
	/** zero seconds */
	DECT_ASSOCIATION_REJECT_TIME_0S = 0,
	/** 5 seconds */
	DECT_ASSOCIATION_REJECT_TIME_5S = 1,
	/** 10 seconds */
	DECT_ASSOCIATION_REJECT_TIME_10S = 2,
	/** 30 seconds */
	DECT_ASSOCIATION_REJECT_TIME_30S = 3,
	/** 60 seconds */
	DECT_ASSOCIATION_REJECT_TIME_60S = 4,
	/** 120 seconds */
	DECT_ASSOCIATION_REJECT_TIME_120S = 5,
	/** 180 seconds */
	DECT_ASSOCIATION_REJECT_TIME_180S = 6,
	/** 300 seconds */
	DECT_ASSOCIATION_REJECT_TIME_300S = 7,
	/** 600 seconds */
	DECT_ASSOCIATION_REJECT_TIME_600S = 8,
};

/** @brief RSSI scan result verdict. */
enum dect_rssi_scan_result_verdict {
	DECT_RSSI_SCAN_VERDICT_UNKNOWN,
	DECT_RSSI_SCAN_VERDICT_FREE,
	DECT_RSSI_SCAN_VERDICT_POSSIBLE,
	DECT_RSSI_SCAN_VERDICT_BUSY,
};

/** @brief DECT RSSI scan result event. */
struct dect_rssi_scan_result_data {
	uint16_t channel;
	enum dect_rssi_scan_result_verdict frame_subslot_verdicts[48];
	bool all_subslots_free;
	bool another_cluster_detected_in_channel;
	uint8_t scan_suitable_percent;
	uint8_t free_subslot_cnt;
	uint8_t possible_subslot_cnt;
	uint8_t busy_subslot_cnt;

	/** Percentage of busy subslots over measured frames [0 .. 100] */
	uint8_t busy_percentage;

};

/** @brief DECT RSSI scan result event. */
struct dect_rssi_scan_result_evt {
	struct dect_rssi_scan_result_data rssi_scan_result;
};

/**
 * @brief Association release causes.
 * @details Table 6.4.2.6-1 in @ref DECT-MAC-SPEC "DECT-2020 NR Part 4".
 */
enum dect_association_release_cause {
	/** Connection termination. */
	DECT_RELEASE_CAUSE_CONNECTION_TERMINATION = 0,
	/** Mobility. */
	DECT_RELEASE_CAUSE_MOBILITY,
	/** Radio device has been inactive for too long. */
	DECT_RELEASE_CAUSE_LONG_INACTIVITY,
	/** Incompatible configuration. */
	DECT_RELEASE_CAUSE_INCOMPATIBLE_CONFIGURATION,
	/** Insufficient hardware resources. */
	DECT_RELEASE_CAUSE_INSUFFICIENT_HW_RESOURCES,
	/** Insufficient radio resources. */
	DECT_RELEASE_CAUSE_INSUFFICIENT_RADIO_RESOURCES,
	/** Bad radio quality. */
	DECT_RELEASE_CAUSE_BAD_RADIO_QUALITY,
	/** Security error. */
	DECT_RELEASE_CAUSE_SECURITY_ERROR,
	/** Other error. */
	DECT_RELEASE_CAUSE_OTHER_ERROR,
	/** Other reason. */
	DECT_RELEASE_CAUSE_OTHER_REASON,
	/** RACH resource failure (additional cause,
	 * not from @ref DECT-MAC-SPEC "DECT-2020 NR Part 4").
	 */
	DECT_RELEASE_CAUSE_CUSTOM_RACH_RESOURCE_FAILURE,
};

/** @brief DECT association released event. */
struct dect_association_released_evt {
	/** Long Radio Device ID of neighbor. */
	uint32_t long_rd_id;

	/** Cause of the association release. */
	enum dect_association_release_cause release_cause;
};

/** @brief Neighbor role. */
enum dect_neighbor_role {
	/** Device is our parent. */
	DECT_NEIGHBOR_ROLE_PARENT = 0,
	/** Device is our child. */
	DECT_NEIGHBOR_ROLE_CHILD = 1,
};

/** @brief Association change type. */
enum dect_association_change_type {
	/** Association created. */
	DECT_ASSOCIATION_CREATED = 0,
	/** Association released. */
	DECT_ASSOCIATION_RELEASED = 1,
	/** Association request rejected. */
	DECT_ASSOCIATION_REQ_REJECTED = 2,
	/** Association request failed in modem.  */
	DECT_ASSOCIATION_REQ_FAILED_MDM = 3,
};

/** @brief DECT association changed event. */
struct dect_association_changed_evt {
	/** Long Radio Device ID of neighbor. */
	uint32_t long_rd_id;

	/** Neighbor role. */
	enum dect_neighbor_role neighbor_role;

	/** Association change type. */
	enum dect_association_change_type association_change_type;

	union {
		/* Valid if association_change_type is DECT_ASSOCIATION_RELEASED. */
		struct {
			/** True if release was initiated by the neighbor. */
			bool neighbor_initiated;

			/** Release cause. */
			enum dect_association_release_cause release_cause;
		} association_released;

		/* Valid if association_change_type is DECT_ASSOCIATION_REQ_REJECTED. */
		struct {
			/** Association reject cause if not accepted. */
			enum dect_association_reject_cause reject_cause;

			/** Time how long the other RD shall prohibit sending new association
			 * requests to this RD.
			 */
			enum dect_association_reject_time reject_time;
		} association_req_rejected;

		/* Valid if association_change_type is DECT_ASSOCIATION_REQ_FAILED_MDM. */
		struct {
			/** Association request not delivered. */
			enum dect_status_values cause;
		} association_req_failed_mdm;
	};
};

/** @brief DECT cluster start resp event. */
struct dect_cluster_start_resp_evt {
	enum dect_status_values status;
	uint16_t cluster_channel;
};

/** @brief DECT network status event */
struct dect_network_status_evt {
	enum dect_network_status {
		/** Request failed */
		DECT_NETWORK_STATUS_FAILURE,
		/** Network created. */
		DECT_NETWORK_STATUS_CREATED,
		/** Network removed. */
		DECT_NETWORK_STATUS_REMOVED,
		/** Network joined. */
		DECT_NETWORK_STATUS_JOINED,
		/** Network unjoined. */
		DECT_NETWORK_STATUS_UNJOINED,
	} network_status;
	/** DECT error cause, if network_status is FAILURE. */
	enum dect_status_values dect_err_cause;
	/** OS error cause, if dect_err_cause is DECT_STATUS_OS_ERROR. */
	int os_err_cause;
};

/** @brief DECT sink status event */
struct dect_sink_status_evt {
	enum dect_sink_status {
		/** Sink connected. */
		DECT_SINK_STATUS_CONNECTED,
		/** Sink disconnected. */
		DECT_SINK_STATUS_DISCONNECTED,
	} sink_status;
	/** Interface towards Border Router / Internet. */
	struct net_if *br_iface;
};

/* Note: change also CONFIG_NET_MGMT_EVENT_INFO_DEFAULT_DATA_SIZE
 * if changing dect_neighbor_list_evt.
 */
#define DECT_L2_MAX_NEIGHBOR_LIST_ITEM_COUNT 50

 /** @cond INTERNAL_HIDDEN */

BUILD_ASSERT(CONFIG_NET_MGMT_EVENT_INFO_DEFAULT_DATA_SIZE >=
		     (DECT_L2_MAX_NEIGHBOR_LIST_ITEM_COUNT * sizeof(uint32_t) + sizeof(uint8_t) +
		      sizeof(int)),
	     "CONFIG_NET_MGMT_EVENT_INFO_DEFAULT_DATA_SIZE too small");

/**
 * INTERNAL_HIDDEN @endcond
 */

/** @brief Neighbor list event. */
struct dect_neighbor_list_evt {
	enum dect_status_values status;
	uint8_t neighbor_count;
	uint32_t neighbor_long_rd_ids[DECT_L2_MAX_NEIGHBOR_LIST_ITEM_COUNT];
};

/** @brief Neighbor info event. */
struct dect_neighbor_info_evt {
	enum dect_status_values status;
	uint32_t long_rd_id;

	/** Neighbor status information, valid only if status == DECT_STATUS_OK. */

	/** True if neighbor is associated. */
	bool associated;
	/** True if neighbor operates in FT mode. */
	bool ft_mode;

	/**
	 * PT-mode neighbor: cluster channel.
	 * FT-mode neighbor: Last known operating channel of its cluster.
	 */
	uint16_t channel;

	/** Network ID. */
	uint32_t network_id;

	/** Time in ms since neighbor is last seen. */
	uint32_t time_since_last_rx_ms;

	/** Last received signal information. */
	struct dect_rx_signal_info last_rx_signal_info;
	/**
	 * Average transmission power used by neighbor (PHY type 1 transmissions).
	 * @details Value in dB.
	 *
	 * Valid for FT-mode neighbors only.
	 */
	int8_t beacon_average_rx_txpower;
	/**
	 * Average received signal strength indicator (RSSI-2) of PDC
	 * (PHY type 1 transmissions).
	 * @details Value in dBm.
	 *
	 * Valid for FT-mode neighbors only.
	 */
	int16_t beacon_average_rx_rssi_2;
	/**
	 * Average received signal to noise indicator (SNR) of PDC
	 * (PHY type 1 transmissions).
	 * @details Value in dB.
	 *
	 * Valid for FT-mode neighbors only.
	 */
	int8_t beacon_average_rx_snr;

	struct dect_neighbor_status_info {
		/**
		 * Number of missed cluster beacons.
		 *
		 * Valid for FT-mode neighbors only.
		 */
		uint8_t total_missed_cluster_beacons;
		/**
		 * Current number of consecutive missed cluster beacons.
		 *
		 * Valid for FT-mode neighbors only.
		 */
		uint8_t current_consecutive_missed_cluster_beacons;
		/**
		 * Number of received paging IEs.
		 *
		 * Valid for FT-mode neighbors only.
		 */
		uint8_t num_rx_paging;
		/**
		 * Average MCS used (PHY type 2 transmissions).
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint8_t average_tx_mcs;
		/**
		 * Average transmission power (PHY type 2 transmissions).
		 * @details Value in dB.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		int8_t average_tx_txpower;
		/**
		 * Average MCS used by neighbor (PHY type 2 transmissions).
		 * @details
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint8_t average_rx_mcs;
		/**
		 * Average transmission power used by neighbor (PHY type 2 transmissions).
		 * @details Value in dB.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		int8_t average_rx_txpower;
		/**
		 * Average received signal to noise indicator (SNR) of PDC
		 * (PHY type 2 transmissions).
		 * @details Value in dB.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		int8_t average_rx_snr;
		/**
		 * Average received signal strength indicator (RSSI-2) of PDC
		 * (PHY type 2 transmissions).
		 * @details Value in dBm.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		int16_t average_rx_rssi_2;
		/**
		 * Number of RACH transmissions attempts.
		 * @details
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint16_t num_tx_attempts;
		/**
		 * Number of LBT failures.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint16_t num_lbt_failures;
		/**
		 * Number of received RACH PDCs.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint16_t num_rx_pdc;
		/**
		 * Number of RACH PDC CRC failures.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint16_t num_rx_pdc_crc_failures;
		/**
		 * Number of missed RACH response.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint16_t num_no_response;
		/**
		 * Number of received HARQ ACKs.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint16_t num_harq_ack;
		/**
		 * Number of received HARQ NACKs.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint16_t num_harq_nack;
		/**
		 * Number of transmitted ARQ retransmissions.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint16_t num_arq_retx;
		/**
		 * Data inactive time in ms.
		 *
		 * Note: Statistics are gathered only from PHY type 2 transmissions
		 * Beacon messages are excluded.
		 */
		uint32_t inactive_time_ms;
	} status_info;
};

/** @brief Cluster info event. */
struct dect_cluster_info_evt {
	enum dect_status_values status;

	/** Cluster status information, valid only if status == DECT_STATUS_OK. */
	struct dect_cluster_status_info {
		/** Number of received Association requests. */
		uint16_t num_association_requests;
		/** Number of failed Associations, includes rejections. */
		uint16_t num_association_failures;
		/** Number of neighbors. */
		uint16_t num_neighbors;
		/** Number of neighbors in FTPT-mode. */
		uint16_t num_ftpt_neighbors;
		/** Number of received RACH PDCs. */
		uint32_t num_rach_rx_pdc;
		/** Number of RACH PCC CRC failures. */
		uint32_t num_rach_rx_pcc_crc_failures;
		/** Current RSSI scan result of cluster channel */
		struct dect_rssi_scan_result_data rssi_result;
	} status_info;
};

/** @brief Common response event. */
struct dect_common_resp_evt {
	enum dect_status_values status;
};

/** DECT device type bitmap */
typedef uint32_t dect_device_type_t;

/** DECT device type flags */
#define DECT_DEVICE_TYPE_FT 0x01U  /** Fixed Terminal */
#define DECT_DEVICE_TYPE_PT 0x02U  /** Portable Terminal */

/**
 * @brief Settings command parameters write scope bitmap.
 * @details Note: If writing just specific scopes of the settings,
 * then all of the setting in given scope need to be filled.
 */
enum dect_settings_cmd_params_write_scope {
	DECT_SETTINGS_WRITE_SCOPE_ALL = 0xFFFF,
	DECT_SETTINGS_WRITE_SCOPE_AUTO_START = 0x0001,
	DECT_SETTINGS_WRITE_SCOPE_REGION = 0x0002,
	DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE = 0x0004,
	DECT_SETTINGS_WRITE_SCOPE_IDENTITIES = 0x0008,
	DECT_SETTINGS_WRITE_SCOPE_TX = 0x0010,
	DECT_SETTINGS_WRITE_SCOPE_POWER_SAVE = 0x0020,
	DECT_SETTINGS_WRITE_SCOPE_BAND_NBR = 0x0040,
	DECT_SETTINGS_WRITE_SCOPE_RSSI_SCAN = 0x0080,
	DECT_SETTINGS_WRITE_SCOPE_CLUSTER = 0x0100,
	DECT_SETTINGS_WRITE_SCOPE_NW_BEACON = 0x0200,
	DECT_SETTINGS_WRITE_SCOPE_ASSOCIATION = 0x0400,
	DECT_SETTINGS_WRITE_SCOPE_NETWORK_JOIN = 0x0800,
	DECT_SETTINGS_WRITE_SCOPE_SECURITY_CONFIGURATION = 0x1000,
};

/** @brief Settings command parameters. */
struct dect_settings_cmd_params {
	bool reset_to_driver_defaults;

	/** Following parameters are used when writing settings to the modem
	 *  by using settings_write command.
	 */

	/* Bitmask of dect_settings_cmd_params_write_scope */
	uint16_t write_scope_bitmap;
	/* Returned bitmask of dect_settings_cmd_params_write_scope for failed scopes,
	 * in success zero.
	 */
	uint16_t failure_scope_bitmap_out;
};

#define DECT_NW_BEACON_CHANNEL_NOT_USED UINT16_MAX

/** @brief FT device: cluster settings */
struct dect_settings_cluster {
	/** TX power used for cluster and network beacon transmission */
	int8_t max_beacon_tx_power_dbm;
	/** Cluster Max TX power */
	int8_t max_cluster_power_dbm;
	/** Cluster beacon send periodicity */
	enum dect_cluster_beacon_period beacon_period;
	/** Maximum number of associations to accept. */
	uint16_t max_num_neighbors;
	/** Neighbor inactivity timer that triggers association releasing.
	 *  Time in milliseconds, Use 0 to disable timer.
	 */
	uint32_t neighbor_inactivity_disconnect_timer_ms;

	/** Threshold when an operating channel load (=busy percentage) is so high that
	 *  the RD should start Operating Channel(s) and Subslot(s) selection.
	 *  Setting to zero disables the feature and the RD will not perform any
	 *  channel reselection automatically.
	 */
	uint8_t channel_loaded_percent;
};

/** @brief Network beacon settings. */
struct dect_settings_network_beacon {
	uint16_t channel;
	enum dect_nw_beacon_period beacon_period;
};

/** @brief Association settings. */
struct dect_settings_association {
	/**
	 * Consecutive cluster beacon misses before
	 * triggering procedures for a release of association.
	 */
	uint8_t max_beacon_rx_failures;

	/** Minimum RX sensitivity.
	 *  MIN_SENSITIVITY_LEVEL per @ref DECT-MAC-SPEC "DECT-2020 NR Part 4".
	 */
	int8_t min_sensitivity_dbm;
};

/** @brief Auto-start settings. */
struct dect_settings_auto_start {
	bool activate; /* if true, then auto-activate DECT NR+ stack on bootup */
};

#define DECT_SETT_NETWORK_JOIN_TARGET_FT_ANY 0

/** @brief Network join settings. */
struct dect_settings_network_join {
	/** Target FT Long Radio Device ID. */
	uint32_t target_ft_long_rd_id;
};

/** @brief RSSI scan settings. */
struct dect_settings_rssi_scan {
	/** Percentage of suitable subslots over measured frames [0 .. 100]. */
	uint8_t scan_suitable_percent;

	/** Time per channel in milliseconds. */
	uint32_t time_per_channel_ms;

	/** Free threshold in dBm. */
	int32_t free_threshold_dbm; /* if equal or less considered as free */
	/* rssi_scanning_busy_threshold <= possible < rssi_scanning_free_threshold*/
	int32_t busy_threshold_dbm; /* if higher considered as busy */
};

/** @brief Region setting. */
enum dect_settings_region {
	DECT_SETTINGS_REGION_EU = 0,
	DECT_SETTINGS_REGION_US = 1,
	DECT_SETTINGS_REGION_GLOBAL,
};

/** @brief Identities settings. */
struct dect_settings_identities {
	/** Network ID */
	uint32_t network_id;

	/** Transmitter Long Radio Device ID */
	uint32_t transmitter_long_rd_id;
};

/** @brief Common TX settings. */
struct dect_settings_common_tx {
	/** Maximum TX power. */
	int8_t max_power_dbm;

	/** Maximum MCS. */
	uint8_t max_mcs;
};

/** Security keys lengths. */
#define DECT_INTEGRITY_KEY_LENGTH 16
#define DECT_CIPHER_KEY_LENGTH    16

/** @brief Security mode. */
enum dect_security_mode {
	/** None. */
	DECT_SECURITY_MODE_NONE = 0,
	/** Mode 1. */
	DECT_SECURITY_MODE_1 = 1,
};

/** @brief Security configuration. */
struct dect_settings_security_conf {
	/** Security mode. */
	enum dect_security_mode mode;

	/** Security keys.*/
	uint8_t integrity_key[DECT_INTEGRITY_KEY_LENGTH];
	uint8_t cipher_key[DECT_CIPHER_KEY_LENGTH];
};

/** @brief DECT NR+ Settings. */
struct dect_settings {
	/** Command params. */
	struct dect_settings_cmd_params cmd_params;

	/* Auto-start settings. */
	struct dect_settings_auto_start auto_start;

	/** Region. */
	enum dect_settings_region region;

	/** DECT device type. */
	dect_device_type_t device_type;

	/** DECT identifiers. */
	struct dect_settings_identities identities;

	/** Common TX settings */
	struct dect_settings_common_tx tx;

	/** Power save. */
	bool power_save;

	/** Band. */
	uint8_t band_nbr;

	/** RSSI scan. */
	struct dect_settings_rssi_scan rssi_scan;

	/** Cluster settings. */
	struct dect_settings_cluster cluster;

	/** Network beacon settings. */
	struct dect_settings_network_beacon nw_beacon;

	/** Association settings. */
	struct dect_settings_association association;

	/** Network join settings. */
	struct dect_settings_network_join network_join;

	/** Security configuration. */
	struct dect_settings_security_conf sec_conf;
};

/** @brief Association data. */
struct dect_association_data {
	/** Long RD ID. */
	uint32_t long_rd_id;

	/** Local IPv6 address. */
	struct in6_addr local_ipv6_addr;

	/** Global IPv6 address set. */
	bool global_ipv6_addr_set;

	/** Global IPv6 address. */
	struct in6_addr global_ipv6_addr;
};

/** @brief DECT NR+ status information. */
struct dect_status_info {

	/** true when dect nr+ stack in modem is activated */
	bool mdm_activated;

	/** Cluster information */
	bool cluster_running;
	uint16_t cluster_channel;

	/** Network beacon information */
	bool nw_beacon_running;

	/** Parent info */
	uint8_t parent_count;
	struct dect_association_data parent_associations[1];

	/** Associated children */
	uint8_t child_count;
	struct dect_association_data
		child_associations[CONFIG_DECT_CLUSTER_MAX_CHILD_ASSOCIATION_COUNT];

	/** Border gateway and sink info */
	struct net_if *br_net_iface;
	bool br_global_ipv6_addr_prefix_set;
	struct in6_addr br_global_ipv6_addr_prefix;
	int br_global_ipv6_addr_prefix_len;

	/** Modem firmware version */
	char fw_version_str[100];
};

/** @brief DECT NR+ HAL API. */
struct dect_nr_hal_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Read dect status information (synchronous/blocking).
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param status_info Status info if successful
	 * @return 0 if ok, < 0 if error
	 */
	int (*status_info_get)(const struct device *dev, struct dect_status_info *status_info_out);

	/** Read dect settings (synchronous/blocking).
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param settings Settings if successful.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*settings_read)(const struct device *dev, struct dect_settings *settings_out);

	/** Write dect settings (synchronous/blocking).
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param settings_in Settings to write, and out in case of failure:
	 * failure_scope_bitmap_out contains bitmask of scopes that failed to be written.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*settings_write)(const struct device *dev, struct dect_settings *settings_in);

	/** Activate dect nr+ stack.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*activate_req)(const struct device *dev);

	/** De-activate dect nr+ stack.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 *
	 * @return 0 if ok, < 0 if error.
	 */
	int (*deactivate_req)(const struct device *dev);

	/** Execute a RSSI scan.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Scan parameters
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*rssi_scan)(const struct device *dev, struct dect_rssi_scan_params *params);

	/** Scan for dect beacons.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Scan parameters.
	 * @param cb Callback to be called for each result
	 *           cb parameter is the cb that should be called for each
	 *           result by the driver.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*scan)(const struct device *dev, struct dect_scan_params *params,
		    dect_scan_result_cb_t cb);

	/** Associate with FT.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Associate parameters.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*associate_req)(const struct device *dev, struct dect_associate_req_params *params);

	/** Release existing association.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Association Release parameters.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*associate_release)(const struct device *dev,
				 struct dect_associate_rel_params *params);

	/** FT: Start a cluster.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Cluster start parameters.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*cluster_start_req)(const struct device *dev,
				 struct dect_cluster_start_req_params *params);

	/** FT: Reconfigure a cluster.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Cluster reconfiguration parameters.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*cluster_reconfig_req)(const struct device *dev,
				    struct dect_cluster_reconfig_req_params *params);

	/** FT: Stop a cluster.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Cluster stop parameters.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*cluster_stop_req)(const struct device *dev,
				struct dect_cluster_stop_req_params *params);

	/** FT: Start a network beacon.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Parameters.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*nw_beacon_start_req)(const struct device *dev,
				   struct dect_nw_beacon_start_req_params *params);

	/** FT: Stop a network beacon.
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Parameters.
	 * @return 0 if ok, < 0 if error.
	 */
	int (*nw_beacon_stop_req)(const struct device *dev,
				  struct dect_nw_beacon_stop_req_params *params);

	/** Send a packet. */
	int (*send)(const struct device *dev, struct net_pkt *pkt);

	/** Request neighbor list. */
	int (*neighbor_list_req)(const struct device *dev);

	/** Request neighbor info. */
	int (*neighbor_info_req)(const struct device *dev,
				 struct dect_neighbor_info_req_params *params);

	/** Request cluster info. */
	int (*cluster_info_req)(const struct device *dev);

	/** FT: Request to create/enable a network as set in settings. */
	int (*network_create_req)(const struct device *dev);

	/** FT: Request to remove/disable a network. */
	int (*network_remove_req)(const struct device *dev);

	/** PT: Request to join a network as set in settings. */
	int (*network_join_req)(const struct device *dev);

	/** PT: Request to unjoin from a network. */
	int (*network_unjoin_req)(const struct device *dev);
};

/** @brief IPv6 Address configuration */
struct dect_net_ipv6_prefix_config {
	/** Length of the IPv6 prefix. Length in bytes. Zero means no prefix. */
	int prefix_len;
	/** IPv6 prefix. */
	struct in6_addr prefix;
};

/** @brief DECT NR+ L2 context. */
struct dect_net_l2_context {
	/** L2 flags. */
	enum net_l2_flags flags;

	/** DECT device type. */
	dect_device_type_t device_type;

	/** Network ID. */
	uint32_t network_id;

	/** Transmitter Long Radio Device ID. */
	uint32_t transmitter_long_rd_id;

	/** Current IPv6 address configurations. */
	struct dect_net_ipv6_prefix_config ipv6_prefix_cfg;
	struct in6_addr local_ipv6_addr;
	bool global_ipv6_addr_set;
	struct in6_addr global_ipv6_addr;
};

#define DECT_L2 DECT
NET_L2_DECLARE_PUBLIC(DECT_L2);

/** L2 context type to be used with NET_L2_GET_CTX_TYPE. */
#define DECT_L2_CTX_TYPE struct dect_net_l2_context

/** @brief Calls from driver to L2. */

/**
 * @brief Initialize DECT NR+ L2 on the given network interface.
 *
 * @param iface Network interface.
 * @param driver_initial_settings Initial settings in the driver.
 */
void dect_net_l2_init(struct net_if *iface, struct dect_settings *driver_initial_settings);

/**
 * @brief Inform L2 that settings have been changed.
 *
 * @param iface Network interface.
 * @param driver_current_settings Current settings in the driver.
 */
void dect_net_l2_settings_changed(struct net_if *iface,
				  struct dect_settings *driver_current_settings);

/**
 * @brief Inform L2 that association with a parent has been created. PT device.
 *
 * @param iface Network interface.
 * @param parent_long_rd_id Long Radio Device ID of the parent.
 * @param ipv6_prefix_config IPv6 prefix configuration for this parent.
 */
void dect_net_l2_parent_association_created(struct net_if *iface, uint32_t parent_long_rd_id,
					    struct dect_net_ipv6_prefix_config *ipv6_prefix_config);

/**
 * @brief Inform L2 that association with a child has been created. FT device.
 *
 * @param iface Network interface.
 * @param target_long_rd_id Long Radio Device ID of the child.
 */
void dect_net_l2_child_association_created(struct net_if *iface, uint32_t target_long_rd_id);

/**
 * @brief Inform L2 that association has been released.
 *
 * @param iface Network interface.
 * @param long_rd_id Long Radio Device ID of the neighbor.
 * @param cause Release cause.
 * @param neighbor_initiated True if release was initiated by the neighbor.
 */
void dect_net_l2_association_removed(struct net_if *iface, uint32_t long_rd_id,
				     enum dect_association_release_cause cause,
				     bool neighbor_initiated);

/**
 * @brief PT device: Inform L2 that IPv6 address configuration of a parent has changed.
 *
 * @param iface Network interface.
 * @param parent_long_rd_id Long Radio Device ID of the parent.
 * @param ipv6_prefix_config New IPv6 prefix configuration for this parent.
 */
void dect_net_l2_parent_ipv6_config_changed(struct net_if *iface, uint32_t parent_long_rd_id,
					    struct dect_net_ipv6_prefix_config *ipv6_prefix_config);

/**
 * @}
 */
 #ifdef __cplusplus
}
#endif

#endif /* DECT_NET_L2_H_ */
