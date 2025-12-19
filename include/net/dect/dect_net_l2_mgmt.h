/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief DECT NR+ Management interface public header
 *
 */

#ifndef DECT_NET_L2_MGMT_H_
#define DECT_NET_L2_MGMT_H_

#include <zephyr/net/net_mgmt.h>
#include <net/dect/dect_net_l2.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dect_net_l2_mgmt DECT NR+ Net Management
 *
 * @brief DECT NR+ management interface public header.
 *
 * @details This header file contains the public API for the DECT NR+ management interface.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#define _NET_DECT_LAYER NET_MGMT_LAYER_L2
#define _NET_DECT_CODE	NET_MGMT_LAYER_CODE_USER1
#define _NET_DECT_BASE                                                                             \
	(NET_MGMT_IFACE_BIT | NET_MGMT_LAYER(_NET_DECT_LAYER) | NET_MGMT_LAYER_CODE(_NET_DECT_CODE))
#define _NET_DECT_EVENT (_NET_DECT_BASE | NET_MGMT_EVENT_BIT)

enum net_request_dect_cmd {
	/** Activate DECT NR+ Stack. */
	NET_REQUEST_DECT_CMD_ACTIVATE = 1,

	/** De-activate DECT NR+ Stack. */
	NET_REQUEST_DECT_CMD_DEACTIVATE,

	/** RSSI scan/measurement. */
	NET_REQUEST_DECT_CMD_RSSI_SCAN,

	/** Scan for DECT NR+ networks. */
	NET_REQUEST_DECT_CMD_SCAN,

	/** Associate with a FT. */
	NET_REQUEST_DECT_CMD_ASSOCIATION_REQ,

	/** Release existing association with a FT. */
	NET_REQUEST_DECT_CMD_ASSOCIATION_RELEASE,

	/** FT device type: start a cluster. */
	NET_REQUEST_DECT_CMD_CLUSTER_START_REQ,

	/** FT device type: manually reconfigure a cluster. */
	NET_REQUEST_DECT_CMD_CLUSTER_RECONFIG_REQ,

	/** FT device type: stop a cluster. */
	NET_REQUEST_DECT_CMD_CLUSTER_STOP_REQ,

	/** FT device type: start a network beacon. */
	NET_REQUEST_DECT_CMD_NW_BEACON_START_REQ,

	/** FT device type: stop a network beacon. */
	NET_REQUEST_DECT_CMD_NW_BEACON_STOP_REQ,

	/** FT device type: create a DECT NR+ network. */
	NET_REQUEST_DECT_CMD_NETWORK_CREATE_REQ,

	/** FT device type: remove a DECT NR+ network. */
	NET_REQUEST_DECT_CMD_NETWORK_REMOVE_REQ,

	/** PT device type: join to a DECT NR+ network. */
	NET_REQUEST_DECT_CMD_NETWORK_JOIN,

	/** PT device type: unjoin from a DECT NR+ network. */
	NET_REQUEST_DECT_CMD_NETWORK_UNJOIN,

	/** Read settings. */
	NET_REQUEST_DECT_CMD_SETTINGS_READ,

	/** Write settings. */
	NET_REQUEST_DECT_CMD_SETTINGS_WRITE,

	/** Get status information. */
	NET_REQUEST_DECT_CMD_STATUS_INFO_GET,

	/** Neighbor list request. */
	NET_REQUEST_DECT_CMD_NEIGHBOR_LIST_REQ,

	/** Neighbor info request. */
	NET_REQUEST_DECT_CMD_NEIGHBOR_INFO_REQ,

	/** Cluster info request. */
	NET_REQUEST_DECT_CMD_CLUSTER_INFO_REQ,
};

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @name Command Macros.
 *
 * @brief DECT NR+ net management commands.
 *
 * @details These DECT NR+ subsystem net management commands can be called
 * by applications via net_mgmt macro.
 *
 * @{
 */

/** Request to activate DECT NR+ stack */
#define NET_REQUEST_DECT_ACTIVATE (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_ACTIVATE)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_ACTIVATE);

/** Request to de-activate DECT NR+ stack */
#define NET_REQUEST_DECT_DEACTIVATE (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_DEACTIVATE)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_DEACTIVATE);

/**
 * Request a RSSI scan/measurement.
 *
 * See @ref dect_rssi_scan_params for command parameters.
 */
#define NET_REQUEST_DECT_RSSI_SCAN (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_RSSI_SCAN)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_RSSI_SCAN);

/**
 * Request a DECT NR+ network scan.
 *
 * See @ref dect_scan_params for command parameters.
 */
#define NET_REQUEST_DECT_SCAN (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_SCAN)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_SCAN);

/**
 * PT device type: Send a DECT NR+ association request.
 *
 * See @ref dect_associate_req_params for command parameters.
 */
#define NET_REQUEST_DECT_ASSOCIATION (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_ASSOCIATION_REQ)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_ASSOCIATION);

/**
 * Send a DECT NR+ association release.
 *
 * See @ref dect_associate_rel_params for command parameters.
 */
#define NET_REQUEST_DECT_ASSOCIATION_RELEASE                                                       \
	(_NET_DECT_BASE | NET_REQUEST_DECT_CMD_ASSOCIATION_RELEASE)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_ASSOCIATION_RELEASE);

/** FT device type: start a cluster as configured in related settings. */
#define NET_REQUEST_DECT_CLUSTER_START (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_CLUSTER_START_REQ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_CLUSTER_START);

/** FT device type: reconfigure a cluster. */
#define NET_REQUEST_DECT_CLUSTER_RECONFIGURE                                                       \
	(_NET_DECT_BASE | NET_REQUEST_DECT_CMD_CLUSTER_RECONFIG_REQ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_CLUSTER_RECONFIGURE);

/** FT device type: stop a cluster  */
#define NET_REQUEST_DECT_CLUSTER_STOP (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_CLUSTER_STOP_REQ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_CLUSTER_STOP);

/**
 * FT device type: start a network beacon as configured in related settings.
 *
 * See @ref dect_nw_beacon_start_req_params for additional command parameters.
 */
#define NET_REQUEST_DECT_NW_BEACON_START (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_NW_BEACON_START_REQ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_NW_BEACON_START);

/** FT device type: stop a network beacon. */
#define NET_REQUEST_DECT_NW_BEACON_STOP (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_NW_BEACON_STOP_REQ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_NW_BEACON_STOP);

/** Read DECT NR+ settings into @ref dect_settings. */
#define NET_REQUEST_DECT_SETTINGS_READ (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_SETTINGS_READ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_SETTINGS_READ);

/**
 * Write DECT NR+ settings.
 *
 * See @ref dect_settings for additional parameters.
 */
#define NET_REQUEST_DECT_SETTINGS_WRITE (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_SETTINGS_WRITE)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_SETTINGS_WRITE);

/** Get status information into @ref dect_status_info. */
#define NET_REQUEST_DECT_STATUS_INFO_GET (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_STATUS_INFO_GET)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_STATUS_INFO_GET);

/** Neighbor list request. */
#define NET_REQUEST_DECT_NEIGHBOR_LIST (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_NEIGHBOR_LIST_REQ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_NEIGHBOR_LIST);

/**
 * Neighbor info request.
 *
 * See @ref dect_neighbor_info_req_params for additional parameters.
 */
#define NET_REQUEST_DECT_NEIGHBOR_INFO (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_NEIGHBOR_INFO_REQ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_NEIGHBOR_INFO);

/** Cluster info request. */
#define NET_REQUEST_DECT_CLUSTER_INFO (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_CLUSTER_INFO_REQ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_CLUSTER_INFO);

/** FT device: Request to create a DECT NR+ network as configured in related settings. */
#define NET_REQUEST_DECT_NETWORK_CREATE (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_NETWORK_CREATE_REQ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_NETWORK_CREATE);

/** FT device: Request to remove a DECT NR+ network. */
#define NET_REQUEST_DECT_NETWORK_REMOVE (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_NETWORK_REMOVE_REQ)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_NETWORK_REMOVE);

/** PT device: Request to join to DECT NR+ network as configured in related settings. */
#define NET_REQUEST_DECT_NETWORK_JOIN (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_NETWORK_JOIN)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_NETWORK_JOIN);

/** PT device: Request to unjoin from DECT NR+ network. */
#define NET_REQUEST_DECT_NETWORK_UNJOIN (_NET_DECT_BASE | NET_REQUEST_DECT_CMD_NETWORK_UNJOIN)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_DECT_NETWORK_UNJOIN);

/**
 * @}
 */

/**
 * @cond INTERNAL_HIDDEN
 */
enum {
	NET_EVENT_DECT_CMD_ACTIVATE_DONE_VAL,
	NET_EVENT_DECT_CMD_DEACTIVATE_DONE_VAL,
	NET_EVENT_DECT_CMD_SCAN_RESULT_VAL,
	NET_EVENT_DECT_CMD_SCAN_DONE_VAL,
	NET_EVENT_DECT_CMD_RSSI_SCAN_RESULT_VAL,
	NET_EVENT_DECT_CMD_RSSI_SCAN_DONE_VAL,
	NET_EVENT_DECT_CMD_NETWORK_STATUS_VAL,
	NET_EVENT_DECT_CMD_SINK_STATUS_VAL,
	NET_EVENT_DECT_CMD_ASSOCIATION_CHANGED_VAL,
	NET_EVENT_DECT_CMD_CLUSTER_START_RESP_VAL,
	NET_EVENT_DECT_CMD_CLUSTER_STOPPED_RESP_VAL,
	NET_EVENT_DECT_CMD_NW_BEACON_START_RESP_VAL,
	NET_EVENT_DECT_CMD_NW_BEACON_STOP_RESP_VAL,
	NET_EVENT_DECT_CMD_NEIGHBOR_LIST_RESP_VAL,
	NET_EVENT_DECT_CMD_NEIGHBOR_INFO_RESP_VAL,
	NET_EVENT_DECT_CMD_CLUSTER_INFO_RESP_VAL,

	NET_EVENT_DECT_CMD_MAX
};

BUILD_ASSERT(NET_EVENT_DECT_CMD_MAX <= NET_MGMT_MAX_COMMANDS,
	     "Number of events in net_event_dect_cmd exceeds the limit");

enum net_event_dect_cmd {
	/** Activation done. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_ACTIVATE_DONE),
	/** De-activation done. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_DEACTIVATE_DONE),
	/** Scan results available. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_SCAN_RESULT),
	/** Scan done. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_SCAN_DONE),
	/** RSSI scan results available. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_RSSI_SCAN_RESULT),
	/** RSSI scan done. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_RSSI_SCAN_DONE),
	/** FT: Network status. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_NETWORK_STATUS),
	/** FT: Sink status event. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_SINK_STATUS),
	/** Association changed event. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_ASSOCIATION_CHANGED),
	/** Cluster started event. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_CLUSTER_START_RESP),
	/** Cluster stop response. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_CLUSTER_STOPPED_RESP),
	/** Network beacon started event. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_NW_BEACON_START_RESP),
	/** Network beacon stopped event. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_NW_BEACON_STOP_RESP),
	/** Neighbor list response. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_NEIGHBOR_LIST_RESP),
	/** Neighbor info response. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_NEIGHBOR_INFO_RESP),
	/** Cluster info response. */
	NET_MGMT_CMD(NET_EVENT_DECT_CMD_CLUSTER_INFO_RESP),
};
/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @name Event Macros.
 *
 * @brief DECT NR+ net management events.
 *
 * @details These DECT NR+ subsystem net management events can be
 * subscribed to by applications via net_mgmt_init_event_callback,
 * net_mgmt_add_event_callback and net_mgmt_del_event_callback.
 *
 * @{
 */

/**
 * Signals the result of the @ref NET_REQUEST_DECT_ACTIVATE net management command.
 * Also used to signal unsolicited result of the re-activation of the DECT NR+ stack.
 *
 * See @ref dect_status_values for event parameters.
 */
#define NET_EVENT_DECT_ACTIVATE_DONE (_NET_DECT_EVENT | NET_EVENT_DECT_CMD_ACTIVATE_DONE)

/**
 * Signals the result of the @ref NET_REQUEST_DECT_DEACTIVATE net management command.
 *
 * See @ref dect_status_values for event parameters.
 */
#define NET_EVENT_DECT_DEACTIVATE_DONE (_NET_DECT_EVENT | NET_EVENT_DECT_CMD_DEACTIVATE_DONE)

/**
 * Signals the result of the @ref NET_REQUEST_DECT_RSSI_SCAN net management command and
 * sent also unsolicitedly when connecting.
 *
 * See @ref dect_rssi_scan_result_evt for event parameters.
 */
#define NET_EVENT_DECT_RSSI_SCAN_RESULT (_NET_DECT_EVENT | NET_EVENT_DECT_CMD_RSSI_SCAN_RESULT)

/**
 * Signals the completion of the @ref NET_REQUEST_DECT_RSSI_SCAN net management command and
 * sent also unsolicitedly when connecting.
 *
 * See @ref dect_common_resp_evt for event parameters.
 */
#define NET_EVENT_DECT_RSSI_SCAN_DONE (_NET_DECT_EVENT | NET_EVENT_DECT_CMD_RSSI_SCAN_DONE)

/**
 * Signals the result of the @ref NET_REQUEST_DECT_SCAN net management command.
 *
 * See @ref dect_scan_result_evt for event parameters.
 */
#define NET_EVENT_DECT_SCAN_RESULT (_NET_DECT_EVENT | NET_EVENT_DECT_CMD_SCAN_RESULT)

/**
 * Signals the completion of the @ref NET_REQUEST_DECT_SCAN net management command
 * and unsolicitedly when connecting.
 *
 * See @ref dect_common_resp_evt for event parameters.
 */
#define NET_EVENT_DECT_SCAN_DONE (_NET_DECT_EVENT | NET_EVENT_DECT_CMD_SCAN_DONE)

/**
 * Signals association related changes, including the result of the
 * @ref NET_REQUEST_DECT_ASSOCIATION and @ref NET_REQUEST_DECT_ASSOCIATION_RELEASE net management
 * commands but sent also when unsolicitedly released or otherwise changed.
 * See @ref dect_association_changed_evt for event parameters.
 */
#define NET_EVENT_DECT_ASSOCIATION_CHANGED                                                         \
	(_NET_DECT_EVENT | NET_EVENT_DECT_CMD_ASSOCIATION_CHANGED)

/**
 * Signals network status changes.
 *
 * See @ref dect_network_status_evt for event parameters.
 */
#define NET_EVENT_DECT_NETWORK_STATUS                                                              \
	(_NET_DECT_EVENT |                                                                         \
	 NET_EVENT_DECT_CMD_NETWORK_STATUS) /* created/removed/joined/unjoined */

/**
 * Signals sink status changes.
 *
 * See @ref dect_sink_status_evt for event parameters.
 */
#define NET_EVENT_DECT_SINK_STATUS                                                                 \
	(_NET_DECT_EVENT |                                                                         \
	 NET_EVENT_DECT_CMD_SINK_STATUS) /* connected/disconnected */

/**
 * Signals the result of the @ref NET_REQUEST_DECT_CLUSTER_START and
 * @ref NET_REQUEST_DECT_CLUSTER_RECONFIGURE net management commands.
 */
#define NET_EVENT_DECT_CLUSTER_CREATED_RESULT                                                      \
	(_NET_DECT_EVENT | NET_EVENT_DECT_CMD_CLUSTER_START_RESP)

/**
 * Signals the result of the @ref NET_REQUEST_DECT_CLUSTER_STOP net management command.
 * See @ref dect_common_resp_evt for event parameters.
 */
#define NET_EVENT_DECT_CLUSTER_STOPPED_RESULT                                                      \
	(_NET_DECT_EVENT | NET_EVENT_DECT_CMD_CLUSTER_STOPPED_RESP)

/**
 * Signals the result of the @ref NET_REQUEST_DECT_NW_BEACON_START net management command.
 * Sent also unsolicitedly when connecting with network beacon configuration.
 * See @ref dect_common_resp_evt for event parameters.
 */
#define NET_EVENT_DECT_NW_BEACON_START_RESULT                                                      \
	(_NET_DECT_EVENT | NET_EVENT_DECT_CMD_NW_BEACON_START_RESP)

/**
 * Signals the result of the @ref NET_REQUEST_DECT_NW_BEACON_STOP net management command.
 * See @ref dect_common_resp_evt for event parameters.
 */
#define NET_EVENT_DECT_NW_BEACON_STOP_RESULT                                                       \
	(_NET_DECT_EVENT | NET_EVENT_DECT_CMD_NW_BEACON_STOP_RESP)

/**
 * Signals the result of the @ref NET_REQUEST_DECT_NEIGHBOR_LIST net management command.
 * See @ref dect_neighbor_list_evt for event parameters.
 */
#define NET_EVENT_DECT_NEIGHBOR_LIST (_NET_DECT_EVENT | NET_EVENT_DECT_CMD_NEIGHBOR_LIST_RESP)

/**
 * Signals the result of the @ref NET_REQUEST_DECT_NEIGHBOR_INFO net management command.
 * See @ref dect_neighbor_info_evt for event parameters.
 */
#define NET_EVENT_DECT_NEIGHBOR_INFO (_NET_DECT_EVENT | NET_EVENT_DECT_CMD_NEIGHBOR_INFO_RESP)

/**
 * Signals the result of the @ref NET_REQUEST_DECT_CLUSTER_INFO net management command.
 * See @ref dect_cluster_info_evt for event parameters.
 */
#define NET_EVENT_DECT_CLUSTER_INFO (_NET_DECT_EVENT | NET_EVENT_DECT_CMD_CLUSTER_INFO_RESP)
/* Helper functions for driver to send L2 events */

/** Send NET_EVENT_DECT_ACTIVATE_DONE.
 *
 * @param iface Network interface.
 * @param status Status of the activation request.
 */
void dect_mgmt_activate_done_evt(struct net_if *iface, enum dect_status_values status);

/** Send NET_EVENT_DECT_DEACTIVATE_DONE
 * @param iface Network interface.
 * @param status Status of the deactivation request.
 */
void dect_mgmt_deactivate_done_evt(struct net_if *iface, enum dect_status_values status);

/** Send NET_EVENT_DECT_RSSI_SCAN_RESULT.
 * @param iface Network interface.
 * @param result_data RSSI scan result data.
 */
void dect_mgmt_rssi_scan_result_evt(struct net_if *iface,
				    struct dect_rssi_scan_result_evt result_data);

/** Send NET_EVENT_DECT_SCAN_DONE.
 * @param iface Network interface.
 * @param status Status of the scan request.
 */
void dect_mgmt_rssi_scan_done_evt(struct net_if *iface, enum dect_status_values status);

/** Send NET_EVENT_DECT_ASSOCIATION_CHANGED.
 * @param iface Network interface.
 * @param evt_data Association changed event data.
 */
void dect_mgmt_association_changed_evt(struct net_if *iface,
				       struct dect_association_changed_evt evt_data);

/** Send NET_EVENT_DECT_ASSOCIATION_CHANGED with DECT_ASSOCIATION_CREATED
 *  association change type and DECT_NEIGHBOR_ROLE_PARENT.
 * @param iface Network interface.
 * @param long_rd_id Long Radio Identifier of the remote DECT NR+ device.
 */
void dect_mgmt_parent_association_created_evt(struct net_if *iface, uint32_t long_rd_id);

/** Send NET_EVENT_DECT_ASSOCIATION_CHANGED with DECT_ASSOCIATION_CREATED
 *  association change type and DECT_NEIGHBOR_ROLE_CHILD.
 * @param iface Network interface.
 * @param long_rd_id Long Radio Identifier of the remote DECT NR+ device.
 */
void dect_mgmt_child_association_created_evt(struct net_if *iface, uint32_t long_rd_id);

/** Send NET_EVENT_DECT_ASSOCIATION_CHANGED with DECT_ASSOCIATION_RELEASED
 *  association change type.
 * @param iface Network interface.
 * @param neighbor_long_rd_id Long Radio Identifier of the remote DECT NR+ device.
 * @param neighbor_role Role of the remote DECT NR+ device.
 * @param neighbor_initiated True if the release was initiated by the neighbor device.
 * @param cause Cause of the association release.
 */
void dect_mgmt_association_released_evt(struct net_if *iface, uint32_t neighbor_long_rd_id,
					enum dect_neighbor_role neighbor_role,
					bool neighbor_initiated,
					enum dect_association_release_cause cause);

/** Send NET_EVENT_DECT_ASSOCIATION_CHANGED with DECT_ASSOCIATION_REQ_FAILED_MDM
 *  association change type.
 * @param iface Network interface.
 * @param long_rd_id Long Radio Identifier of the remote DECT NR+ device.
 * @param cause Cause of the association request failure from modem.
 */
void dect_mgmt_association_req_failed_mdm_result_evt(struct net_if *iface, uint32_t long_rd_id,
						     enum dect_status_values cause);

/** Send NET_EVENT_DECT_ASSOCIATION_CHANGED with DECT_ASSOCIATION_REQ_REJECTED
 *  association change type.
 * @param iface Network interface.
 * @param long_rd_id Long Radio Identifier of the remote DECT NR+ device.
 * @param reject_cause Cause of the association request rejection from remote DECT NR+ device.
 * @param reject_time Time how long the other RD shall prohibit sending new association request.
 */
void dect_mgmt_association_req_rejected_result_evt(struct net_if *iface, uint32_t long_rd_id,
						   enum dect_association_reject_cause reject_cause,
						   enum dect_association_reject_time reject_time);

/** Send NET_EVENT_DECT_CLUSTER_CREATED_RESULT.
 * @param iface Network interface.
 * @param resp_data Response data of the cluster start request.
 */
void dect_mgmt_cluster_created_evt(struct net_if *iface,
				   struct dect_cluster_start_resp_evt resp_data);

/** Send NET_EVENT_DECT_CLUSTER_STOPPED_RESULT.
 * @param iface Network interface.
 * @param status Status of the cluster stop request.
 */
void dect_mgmt_cluster_stopped_evt(struct net_if *iface, enum dect_status_values status);

/** Send NET_EVENT_DECT_CLUSTER_INFO.
 * @param iface Network interface
 * @param resp_data Cluster information.
 */
void dect_mgmt_cluster_info_evt(struct net_if *iface, struct dect_cluster_info_evt resp_data);

/** Send NET_EVENT_DECT_NETWORK_STATUS.
 * @param iface Network interface.
 * @param network_status_data Network status data.
 */
void dect_mgmt_network_status_evt(struct net_if *iface,
				  struct dect_network_status_evt network_status_data);

/** Send NET_EVENT_DECT_SINK_STATUS.
 * @param iface Network interface for dect.
 * @param sink_status_data Sink status data.
 */
void dect_mgmt_sink_status_evt(struct net_if *iface,
			       struct dect_sink_status_evt sink_status_data);

/** Send NET_EVENT_DECT_NW_BEACON_START_RESULT.
 * @param iface Network interface.
 * @param status Status of the network beacon start request.
 */
void dect_mgmt_nw_beacon_start_evt(struct net_if *iface, enum dect_status_values status);

/** Send NET_EVENT_DECT_NW_BEACON_STOP_RESULT.
 * @param iface Network interface.
 * @param status Status of the network beacon stop request.
 */
void dect_mgmt_nw_beacon_stop_evt(struct net_if *iface, enum dect_status_values status);

/** Send NET_EVENT_DECT_NEIGHBOR_LIST.
 * @param iface Network interface.
 * @param resp_data Response data of the neighbor list request.
 */
void dect_mgmt_neighbor_list_evt(struct net_if *iface, struct dect_neighbor_list_evt resp_data);

/** Send NET_EVENT_DECT_NEIGHBOR_INFO.
 * @param iface Network interface.
 * @param resp_data Response data of the neighbor info request.
 */
void dect_mgmt_neighbor_info_evt(struct net_if *iface, struct dect_neighbor_info_evt resp_data);

/**
 * @}
 */

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* DECT_NET_L2_MGMT_H_ */
