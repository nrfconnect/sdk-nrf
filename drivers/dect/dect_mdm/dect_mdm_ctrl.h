/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_MDM_CTRL_H
#define DECT_MDM_CTRL_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/net/net_if.h>
#include <net/dect/dect_net_l2_mgmt.h>

#include "dect_mdm_common.h"

/**
 * @file dect_mdm_ctrl.h
 * @brief DECT NR+ Control Module Architecture
 *
 * @details
 * The control module manages DECT modem operations, cluster configuration,
 * network management, and device associations.
 *
 * Architecture:
 * - API Layer (dect_mdm_ctrl_api.c): Interface for dect_mdm_*.c usage.
 *   Functions validate state and queue operations to the message queue thread.
 *   Some API functions may also be called from the message queue thread when
 *   they don't queue operations (e.g., direct modem API calls are OK).
 * - Handler Layer (dect_mdm_ctrl.c): Processes operations asynchronously
 *   via a dedicated message queue thread. Handles modem callbacks and state
 *   transitions.
 * - Data Layer: Shared control data (ctrl_data) protected by mutex.
 *   Contains modem, cluster, network, and association state.
 *
 * Thread Safety: All ctrl_data access is protected by dect_mdm_ctrl_data_mtx.
 * Modem callbacks are queued as events and processed sequentially by the
 * message queue thread to avoid blocking modem operations.
 */

#define NRF91_DECT_UL_BUFFER_SIZE DECT_MTU

#define DECT_MDM_DATA_TX_HANDLE_START 1000
#define DECT_MDM_DATA_TX_HANDLE_END   1039
#define DECT_MDM_DATA_TX_HANDLE_IN_RANGE(x)                                                        \
	(x >= DECT_MDM_DATA_TX_HANDLE_START && x <= DECT_MDM_DATA_TX_HANDLE_END)
#define DECT_MDM_DATA_TX_HANDLE_COUNT                                                              \
	(DECT_MDM_DATA_TX_HANDLE_END - DECT_MDM_DATA_TX_HANDLE_START + 1)

/* Internal operation enum moved to dect_mdm_ctrl_internal.h */
struct dect_mdm_ctrl_api_tx_cmd_params {
	uint8_t flow_id;

	uint8_t data[DECT_MTU];
	uint32_t data_len;
	uint32_t long_rd_id;
	uint32_t transaction_id;
};

struct dect_mdm_ctrl_dlc_rx_data_with_pkt_ptr {
	struct net_if *iface;
	struct nrf_modem_dect_dlc_data_rx_ntf_cb_params mdm_params;
	size_t data_len;
	struct net_pkt *pkt;
};


#define DECT_MDM_DLC_DATA_INFO_MAX_COUNT 40
#define DECT_MDM_RSSI_MEAS_ARR_SIZE (DECT_RSSI_MEAS_SUBSLOT_COUNT / 8)

/* Internal DECT NR+ ctrl api:
 * Unless otherwise stated, all of these that return integer returns 0 if success
 * and negative on error.
 */

int dect_mdm_ctrl_api_mdm_configure_n_activate(void);
int dect_mdm_ctrl_api_mdm_deactivate_cmd(void);

int dect_mdm_ctrl_api_cluster_channel_get(void);
bool dect_mdm_ctrl_api_mdm_activated(void);

int dect_mdm_ctrl_api_mdm_reactivate(void);

int dect_mdm_ctrl_api_rssi_scan_start_cmd(struct nrf_modem_dect_mac_rssi_scan_params *params);

int dect_mdm_ctrl_api_nw_scan_cmd(struct nrf_modem_dect_mac_network_scan_params *params,
				 dect_scan_result_cb_t cb);

int dect_mdm_ctrl_api_tx_cmd(struct dect_mdm_ctrl_api_tx_cmd_params *params);
int dect_mdm_ctrl_api_associate_req_cmd(struct nrf_modem_dect_mac_association_params *params);
int dect_mdm_ctrl_api_associate_release_cmd(uint32_t long_rd_id,
					    enum nrf_modem_dect_mac_release_cause rel_cause);

int dect_mdm_ctrl_api_neighbor_info_req_cmd(struct nrf_modem_dect_mac_neighbor_info_params *params);

int dect_mdm_ctrl_api_cluster_start_req_cmd(struct dect_cluster_start_req_params *params);
int dect_mdm_ctrl_api_cluster_reconfig_req_cmd(struct dect_cluster_reconfig_req_params *params);
int dect_mdm_ctrl_api_cluster_reconfig_start_for_ipv6_prefix_cfg_changed(void);

int dect_mdm_ctrl_api_cluster_info_req_cmd(void);

int dect_mdm_ctrl_api_network_create_req_cmd(void);
int dect_mdm_ctrl_api_network_remove_req_cmd(void);
bool dect_mdm_ctrl_api_network_remove_req_cmd_allowed(void);

int dect_mdm_ctrl_api_network_join_req_cmd(void);
int dect_mdm_ctrl_api_network_unjoin_req_cmd(void);

int dect_mdm_ctrl_api_nw_beacon_start_req_cmd(struct dect_nw_beacon_start_req_params *params);
int dect_mdm_ctrl_api_nw_beacon_stop_req_cmd(struct dect_nw_beacon_stop_req_params *params);
bool dect_mdm_ctrl_api_nw_beacon_running(void);

int dect_mdm_ctrl_api_neighbor_list_req_cmd(void);

int dect_mdm_ctrl_api_init(struct net_if *iface);

/** Get a reference to the DECT modem capabilities notification callback parameters
 * @return Pointer to the capabilities notification callback parameters
 */
struct nrf_modem_dect_mac_capability_ntf_cb_params
	*dect_mdm_ctrl_api_mdm_capabilities_ref_get(void);
#endif /* DECT_MDM_CTRL_H */
