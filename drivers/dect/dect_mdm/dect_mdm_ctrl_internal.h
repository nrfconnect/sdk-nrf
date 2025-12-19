/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file dect_mdm_ctrl_internal.h
 * @brief Internal definitions for DECT NR+ Control Module
 *
 * @details
 * This header file is for internal use only within the control module files:
 * - dect_mdm_ctrl.c
 * - dect_mdm_ctrl_mdm.c
 * - dect_mdm_ctrl_api.c
 *
 * Do not include this header in other modules.
 */

#ifndef DECT_MDM_CTRL_INTERNAL_H
#define DECT_MDM_CTRL_INTERNAL_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stddef.h>
#include <zephyr/net/net_if.h>

#include <nrf_modem_dect.h>
#include <net/dect/dect_net_l2_mgmt.h>
#include <net/dect/dect_net_l2.h>

#include "dect_mdm_common.h"
#include "dect_mdm_ctrl.h"

/* Internal message queue operations enum */
enum dect_mdm_ctrl_op {
	DECT_MDM_CTRL_OP_MDM_CAPABILITIES,
	DECT_MDM_CTRL_OP_MDM_CONFIGURE_RESP,
	DECT_MDM_CTRL_OP_MDM_CFUN_RESP,
	DECT_MDM_CTRL_OP_MDM_ACTIVATED,
	DECT_MDM_CTRL_OP_MDM_DEACTIVATED,
	DECT_MDM_CTRL_OP_AUTO_START,
	DECT_MDM_CTRL_OP_CLUSTER_START_REQ,
	DECT_MDM_CTRL_OP_CLUSTER_RECONFIG_REQ,
	DECT_MDM_CTRL_OP_CLUSTER_IPV6_PREFIX_CHANGE_RECONFIG_REQ,
	DECT_MDM_CTRL_OP_CLUSTER_CONFIG_RESP,
	DECT_MDM_CTRL_OP_CLUSTER_CH_LOAD_CHANGED,
	DECT_MDM_CTRL_OP_NEIGHBOR_INACTIVITY,
	DECT_MDM_CTRL_OP_RSSI_START_REQ_CMD,
	DECT_MDM_CTRL_OP_RSSI_START_REQ_CH_SELECTION,
	DECT_MDM_CTRL_OP_MDM_RSSI_RESULT,
	DECT_MDM_CTRL_OP_MDM_RSSI_COMPLETE,
	DECT_MDM_CTRL_OP_MDM_RSSI_STOPPED,
	DECT_MDM_CTRL_OP_MDM_NW_BEACON_START,
	DECT_MDM_CTRL_OP_MDM_NW_BEACON_START_OR_STOP_DONE,
	DECT_MDM_CTRL_OP_MDM_NW_BEACON_STOP,
	DECT_MDM_CTRL_OP_MDM_NW_SCAN_COMPLETE,
	DECT_MDM_CTRL_OP_MDM_NW_SCAN_STOPPED,
	DECT_MDM_CTRL_OP_MDM_CLUSTER_BEACON_RCVD,
	DECT_MDM_CTRL_OP_MDM_NW_BEACON_RCVD,
	DECT_MDM_CTRL_OP_MDM_CLUSTER_RCV_COMPLETE,
	DECT_MDM_CTRL_OP_MDM_CLUSTER_INFO,
	DECT_MDM_CTRL_OP_CLUSTER_BEACON_RX_FAILURE,
	DECT_MDM_CTRL_OP_NEIGHBOR_PAGING_FAILURE,
	DECT_MDM_CTRL_OP_MDM_NEIGHBOR_INFO,
	DECT_MDM_CTRL_OP_MDM_NEIGHBOR_LIST,
	DECT_MDM_CTRL_OP_MDM_FLOW_CONTROL,
	DECT_MDM_CTRL_OP_MDM_DLC_DATA_RESP,
	DECT_MDM_CTRL_OP_MDM_ASSOCIATION_IND,
	DECT_MDM_CTRL_OP_MDM_ASSOCIATION_RESP,
	DECT_MDM_CTRL_OP_MDM_ASSOCIATION_RELEASE_IND,
	DECT_MDM_CTRL_OP_MDM_ASSOCIATION_RELEASE_RESP,
	DECT_MDM_CTRL_OP_MDM_IPV6_CONFIG_CHANGED,
};

/* Internal state enums */
enum ctrl_pt_association_state {
	CTRL_PT_ASSOCIATION_STATE_NONE,
	CTRL_PT_ASSOCIATION_STATE_CLUSTER_FOUND,
	CTRL_PT_ASSOCIATION_STATE_CLUSTER_BEACON_RECEIVED,
	CTRL_PT_ASSOCIATION_STATE_ASSOCIATED,
};

enum ctrl_ft_cluster_state {
	CTRL_FT_CLUSTER_STATE_NONE = 0,
	CTRL_FT_CLUSTER_STATE_STARTING,
	CTRL_FT_CLUSTER_STATE_STARTED,
};

enum ctrl_ft_nw_beacon_state {
	CTRL_FT_NW_BEACON_STATE_NONE = 0,
	CTRL_FT_NW_BEACON_STATE_STARTING,
	CTRL_FT_NW_BEACON_STATE_STARTED,
	CTRL_FT_NW_BEACON_STATE_STOPPING,
};

enum ctrl_ft_network_state {
	CTRL_FT_NETWORK_STATE_NONE = 0,
	CTRL_FT_NETWORK_STATE_STARTING,
	CTRL_FT_NETWORK_STATE_CREATED,
};

enum ctrl_mdm_activation_state {
	CTRL_MDM_DEACTIVATED = 0,
	CTRL_MDM_ACTIVATE_REQ,
	CTRL_MDM_ACTIVATED,
	CTRL_MDM_DEACTIVATE_REQ,
	CTRL_MDM_REACTIVATING_DEACTIVATE,
	CTRL_MDM_REACTIVATING_CONFIGURE,
};

/* Internal structures */
struct dect_mdm_ctrl_rssi_scan_result_data {
	uint16_t channel;
	bool another_cluster_detected_in_channel;
	bool all_subslots_free;
	uint8_t busy_percentage;
	bool busy_percentage_ok;
	bool scan_suitable_percent_ok;
	uint8_t possible_subslot_cnt;
	uint8_t busy_subslot_cnt;
};

#define DECT_MDM_CTRL_RSSI_SCAN_RESULTS_MAX_CHANNELS 30

struct dect_mdm_ctrl_rssi_scan_data {
	bool cmd_on_going; /* actual RSSI command */
	uint8_t current_results_index;
	struct dect_mdm_ctrl_rssi_scan_result_data results[
		DECT_MDM_CTRL_RSSI_SCAN_RESULTS_MAX_CHANNELS];
};

#define DECT_MDM_CTRL_CLUSTER_SCAN_DATA_MAX_CHANNELS 30

struct dect_mdm_ctrl_cluster_channel_list_item {
	uint32_t long_rd_id;
	uint16_t channel;
	int8_t rssi_2;

	/* From route info IE */
	bool has_route_info;
	struct nrf_modem_dect_mac_route_info_ie route_info_ie;
};

struct dect_mdm_ctrl_scan_data {
	bool on_going;
	struct dect_mdm_ctrl_cluster_channel_list_item
		cluster_channels[DECT_MDM_CTRL_CLUSTER_SCAN_DATA_MAX_CHANNELS];
	uint8_t current_cluster_channel_index;
	dect_scan_result_cb_t scan_result_cb;
	struct nrf_modem_dect_mac_network_scan_params scan_params;
};

struct dect_mdm_ctrl_dlc_data_tx_info {
	uint32_t transaction_id;
	uint32_t data_len;
	bool req_on_going;
};

struct dect_mdm_ctrl_configure_params {
	bool debug;
	bool power_save;
	bool auto_start;
	bool auto_activate;
	int8_t tx_pwr;
	uint8_t tx_mcs;
	uint8_t band;
	uint8_t band_group_index;
	uint16_t channel;
	uint32_t long_rd_id;
	uint32_t network_id;
};

struct ctrl_pt_association_config {
	enum ctrl_pt_association_state pt_association_state;
	uint32_t parent_long_rd_id;
	uint32_t network_id;
	uint16_t channel;
};

/* Main control data structure - full definition needed for API access */
struct dect_mdm_ctrl_data {
	bool debug;
	struct nrf_modem_dect_mac_capability_ntf_cb_params mdm_capas;
	enum ctrl_ft_cluster_state ft_cluster_state;
	enum ctrl_ft_network_state ft_network_state;
	enum ctrl_ft_nw_beacon_state ft_nw_beacon_state;
	uint16_t ft_requested_cluster_channel;
	uint16_t ft_cluster_reconfig_prev_cluster_channel;
	struct dect_cluster_reconfig_req_params ft_cluster_reconfig_params;
	bool ft_cluster_reconfig_ongoing;
	enum ctrl_mdm_activation_state mdm_activation_state;
	struct ctrl_pt_association_config ass_config;
	enum nrf_modem_dect_mac_release_cause last_rel_cause;
	struct dect_mdm_ctrl_rssi_scan_data rssi_scan_data;
	struct dect_mdm_ctrl_scan_data scan_data;
	struct dect_mdm_ctrl_configure_params configure_params;
	bool tx_mdm_flow_ctrl_on;
#if defined(CONFIG_DECT_MDM_NRF_TX_FLOW_CTRL_BASED_ON_MDM_TX_DLC_REQS)
	uint32_t total_unacked_tx_data_amount;
	uint16_t total_unacked_req_amount;
	struct dect_mdm_ctrl_dlc_data_tx_info
		dlc_data_tx_infos[DECT_MDM_DLC_DATA_INFO_MAX_COUNT];
#endif
	struct net_if *iface;
};

/* Event structures used for message queue communication */
struct dect_mdm_ctrl_rssi_measurement_data_evt {
	struct nrf_modem_dect_mac_rssi_result rssi_result;
};

#define DECT_MDM_MAX_NEIGHBOR_LIST_COUNT 50
struct dect_mdm_ctrl_neighbor_list_resp_evt {
	int status;
	uint8_t num_neighbors;
	uint32_t neighbor_long_rd_ids[DECT_MDM_MAX_NEIGHBOR_LIST_COUNT];
};

struct dect_mdm_ctrl_dlc_data_tx_evt_data_item {
	uint32_t transaction_id;
};

struct dect_mdm_ctrl_dlc_data_tx_resp_evt {
	enum nrf_modem_dect_mac_err status;
	uint8_t flow_id;
	uint32_t long_rd_id;

	uint8_t num_acked_data;
	struct dect_mdm_ctrl_dlc_data_tx_evt_data_item acked_data[1];
};

/* External mutex and semaphores (defined in dect_mdm_ctrl.c) */
extern struct k_mutex dect_mdm_ctrl_data_mtx;
extern struct k_sem dect_mdm_ctrl_reactivate_sema;
extern struct k_sem dect_mdm_libmodem_api_sema;

/* Helper macros for ctrl_data mutex access */
#define CTRL_DATA_LOCK()   k_mutex_lock(&dect_mdm_ctrl_data_mtx, K_FOREVER)
#define CTRL_DATA_UNLOCK() k_mutex_unlock(&dect_mdm_ctrl_data_mtx)

/* Strategic getters for most commonly accessed single fields */
enum ctrl_mdm_activation_state dect_mdm_ctrl_get_mdm_activation_state(void);
enum ctrl_ft_cluster_state dect_mdm_ctrl_get_ft_cluster_state(void);
enum ctrl_ft_nw_beacon_state dect_mdm_ctrl_get_ft_nw_beacon_state(void);
enum ctrl_pt_association_state dect_mdm_ctrl_get_pt_association_state(void);

/* Internal function declarations for message queue operations */
int dect_mdm_ctrl_internal_msgq_data_op_add(enum dect_mdm_ctrl_op event_id, void *data,
					    size_t data_size);
int dect_mdm_ctrl_internal_msgq_non_data_op_add(enum dect_mdm_ctrl_op event_id);

/* Internal function to get control data reference (for API layer) */
struct dect_mdm_ctrl_data *dect_mdm_ctrl_internal_data_get(void);

/* Internal helper functions */
int dect_mdm_ctrl_internal_modem_configure_req_from_settings(void);
int dect_mdm_ctrl_internal_mdm_deactivate_req(void);
bool dect_mdm_ctrl_internal_nw_beacon_common_can_be_started(uint16_t channel);
int dect_mdm_ctrl_internal_init(struct net_if *iface);

#endif /* DECT_MDM_CTRL_INTERNAL_H */
