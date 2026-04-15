/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * CMock control functions for nrf_modem_dect_mac.h
 * Uses real API definitions from nrfxlib, only provides test control functions
 */

#ifndef MOCK_NRF_MODEM_DECT_MAC_H_
#define MOCK_NRF_MODEM_DECT_MAC_H_

#include <stdint.h>
#include <stdbool.h>
/* Note: Real API definitions come from nrf_modem_dect_mac.h included in source files */

#ifdef __cplusplus
extern "C" {
#endif
/* Include real API for structures, but we'll mock the implementations */
#include <nrf_modem_dect.h>

/* Mock call tracking variables */
extern int mock_nrf_modem_dect_mac_callback_set_call_count;
extern int mock_nrf_modem_dect_mac_network_scan_call_count;
extern int mock_nrf_modem_dect_mac_cluster_beacon_receive_call_count;
extern int mock_nrf_modem_dect_mac_association_call_count;
extern int mock_nrf_modem_dect_mac_association_release_call_count;
extern int mock_nrf_modem_dect_mac_neighbor_list_call_count;
extern int mock_nrf_modem_dect_mac_neighbor_info_call_count;
extern int mock_nrf_modem_dect_control_systemmode_set_call_count;
extern int mock_nrf_modem_dect_control_configure_call_count;
extern int mock_nrf_modem_dect_control_functional_mode_set_call_count;
extern int mock_nrf_modem_dect_mac_rssi_scan_call_count;
extern int mock_nrf_modem_dect_mac_rssi_scan_stop_call_count;
extern int mock_nrf_modem_dect_mac_cluster_configure_call_count;
extern int mock_nrf_modem_dect_mac_cluster_info_call_count;
/* Last cluster_configure IPv6 prefix (8 bytes) and type for test verification */
#define MOCK_IPV6_PREFIX_LEN 8
extern uint8_t mock_last_configured_ipv6_prefix[MOCK_IPV6_PREFIX_LEN];
extern int mock_last_configured_ipv6_prefix_type; /* NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_* */
extern int mock_nrf_modem_dect_dlc_data_tx_call_count;
extern bool mock_cluster_creation_band1; /* Flag to indicate cluster creation RSSI scan at band 1 */
/** Set true before network remove so mock invokes association_release op callback for each child */
extern bool mock_auto_invoke_association_release_cb;

/* Test-controlled inject: next callback will use this status (one-shot, then reset to OK). */
extern enum nrf_modem_dect_mac_err mock_inject_configure_status;
extern enum nrf_modem_dect_mac_err mock_inject_functional_mode_status;
extern enum nrf_modem_dect_mac_err mock_inject_association_status;
/* When true, next association callback is rejected (has_association_response=1, ack_status=0). */
extern bool mock_inject_association_rejected;
extern int mock_inject_association_reject_cause; /* e.g. DECT_ASSOCIATION_REJECT_CAUSE_* */
extern int mock_inject_association_reject_time;	 /* e.g. DECT_ASSOCIATION_REJECT_TIME_60S */

/* Storage for last DLC data TX parameters (for verification) */
#define MOCK_DLC_DATA_TX_MAX_SIZE   256
#define MOCK_DLC_DATA_TX_RECORD_MAX 8
extern uint8_t mock_last_dlc_data_tx_data[MOCK_DLC_DATA_TX_MAX_SIZE];
extern size_t mock_last_dlc_data_tx_len;
extern uint32_t mock_last_dlc_data_tx_long_rd_id;
/** Last N long_rd_ids passed to dlc_data_tx (for multicast verification) */
extern uint32_t mock_dlc_data_tx_long_rd_ids[MOCK_DLC_DATA_TX_RECORD_MAX];
extern int mock_dlc_data_tx_record_count;

/* Mock return values */
extern int mock_nrf_modem_dect_mac_callback_set_return;
extern int mock_nrf_modem_dect_mac_network_scan_return;
extern int mock_nrf_modem_dect_mac_cluster_beacon_receive_return;

/* Stored callbacks for simulation */
extern struct nrf_modem_dect_mac_ntf_callbacks mock_ntf_callbacks;
extern struct nrf_modem_dect_mac_op_callbacks mock_op_callbacks;

/* CMock control functions for tests - do NOT redefine anything from real API */

/**
 * Reset the mock to initial state
 */
void mock_nrf_modem_dect_mac_reset(void);

/**
 * CMock-style expectation functions
 */
void mock_nrf_modem_dect_mac_callback_set_ExpectAndReturn(int return_value);
void mock_nrf_modem_dect_mac_network_scan_ExpectAndReturn(int return_value);
void mock_nrf_modem_dect_mac_cluster_beacon_receive_ExpectAndReturn(int return_value);
void mock_nrf_modem_dect_mac_network_scan_ExpectAndReturn(int return_value);
void mock_nrf_modem_dect_mac_association_ExpectAndReturn(int return_value);
int mock_nrf_modem_dect_control_systemmode_set_ExpectAndReturn(int return_value);
int mock_nrf_modem_dect_mac_configure_ExpectAndReturn(int return_value);

/**
 * Set return value for next function call
 * @param return_value The value to return from next mock function call
 */
void mock_nrf_modem_dect_mac_set_return_value(int return_value);

/**
 * Get number of mock function calls made
 * @return Number of calls to mock functions
 */
int mock_nrf_modem_dect_mac_get_call_count(void);

/**
 * Check if mock is initialized
 * @return True if initialized, false otherwise
 */
bool mock_nrf_modem_dect_mac_is_initialized(void);

/**
 * Simulate cluster beacon reception
 */
void mock_simulate_cluster_beacon_received(void);

/**
 * Simulate libmodem initialization (NRF_MODEM_LIB_ON_INIT callback)
 */
void mock_simulate_nrf_modem_lib_init(void);

/**
 * Expose callbacks for test access
 */
extern struct nrf_modem_dect_mac_ntf_callbacks mock_ntf_callbacks;
extern struct nrf_modem_dect_mac_op_callbacks mock_op_callbacks;

#ifdef __cplusplus
}
#endif

#endif /* MOCK_NRF_MODEM_DECT_MAC_H_ */
