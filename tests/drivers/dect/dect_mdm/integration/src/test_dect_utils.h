/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file test_dect_utils.h
 * @brief Common test utilities for DECT integration tests
 *
 * Provides reusable test utilities for DECT network operations such as scanning.
 */

#ifndef TEST_DECT_UTILS_H_
#define TEST_DECT_UTILS_H_

#include <zephyr/net/net_if.h>
#include <net/dect/dect_net_l2_mgmt.h>
#include <nrf_modem_dect.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Beacon simulation parameters for network scan utility
 */
struct test_dect_scan_beacon_params {
	uint16_t channel;		  /* Channel number */
	uint16_t transmitter_short_rd_id; /* Short RD ID */
	uint32_t transmitter_long_rd_id;  /* Long RD ID */
	uint16_t network_id;		  /* Network ID */
	int8_t mcs;			  /* MCS index */
	int8_t transmit_power;		  /* Transmit power [0,15] */
	int8_t rssi_2;			  /* RSSI in dBm */
	int8_t snr;			  /* SNR in dB */
};

/**
 * @brief Network scan result structure
 */
struct test_dect_scan_result {
	bool scan_result_received; /* True if NET_EVENT_DECT_SCAN_RESULT was received */
	bool scan_done_received;   /* True if NET_EVENT_DECT_SCAN_DONE was received */
	enum dect_status_values scan_done_status; /* Scan completion status */
	bool beacon_data_valid;			  /* True if beacon data is valid */
	struct dect_scan_result_evt beacon_data;  /* Received beacon data */
};

/**
 * @brief Association response simulation parameters
 */
struct test_dect_association_response_params {
	bool ack_status;		     /* Association accepted */
	uint8_t number_of_flows;	     /* Number of flows to accept */
	uint8_t number_of_rx_harq_processes; /* Number of RX HARQ processes */
	uint8_t max_number_of_harq_re_rx;    /* Max HARQ retransmissions (RX) */
	uint8_t number_of_tx_harq_processes; /* Number of TX HARQ processes */
	uint8_t max_number_of_harq_re_tx;    /* Max HARQ retransmissions (TX) */
};

/**
 * @brief Association result structure
 */
struct test_dect_association_result {
	bool association_changed_received; /* True if NET_EVENT_DECT_ASSOCIATION_CHANGED rcvd */
	bool association_created_received; /* True if DECT_ASSOCIATION_CREATED event received */
	bool association_failed_received;  /* True if association failed */
	struct dect_association_changed_evt association_data; /* Received association evt data */
	bool network_status_received; /* True if NET_EVENT_DECT_NETWORK_STATUS received */
	struct dect_network_status_evt network_status_data; /* Received nw status event data */
};

/**
 * @brief Perform a DECT network scan with optional beacon simulation
 *
 * This utility function performs a network scan operation and optionally simulates
 * beacon reception. It handles the complete scan workflow:
 * 1. Initiates the scan via net_mgmt API
 * 2. Optionally simulates cluster beacon reception
 * 3. Optionally simulates scan completion
 * 4. Returns scan results
 *
 * @param iface Network interface to use for the scan
 * @param scan_params Scan parameters (band, channels, scan time, etc.)
 * @param beacon_params Optional beacon simulation parameters. If NULL, no beacon
 *                      is simulated.
 * @param simulate_completion If true, simulate scan completion callback.
 *                            If false, caller must handle completion separately.
 * @param result Pointer to result structure to populate. Must not be NULL.
 *
 * @return 0 on success (scan request accepted), negative error code on failure
 */
int test_dect_network_scan(struct net_if *iface, const struct dect_scan_params *scan_params,
			   const struct test_dect_scan_beacon_params *beacon_params,
			   bool simulate_completion, struct test_dect_scan_result *result);

/**
 * @brief Perform a DECT association request with optional completion simulation
 *
 * This utility function performs an association operation and optionally simulates
 * association completion and network joined status. It handles the complete
 * association workflow:
 * 1. Initiates the association via net_mgmt API
 * 2. Optionally simulates association operation completion callback
 * 3. Optionally simulates network joined status event
 * 4. Returns association results
 *
 * @param iface Network interface to use for the association
 * @param target_long_rd_id Long RD ID of the target device to associate with
 * @param response_params Optional association response parameters. If NULL, uses
 *                        default successful response parameters.
 * @param simulate_completion If true, simulate association completion callback.
 *                            If false, caller must handle completion separately.
 * @param simulate_network_joined If true, simulate network joined status event
 *                                 after successful association.
 * @param result Pointer to result structure to populate. Must not be NULL.
 *
 * @return 0 on success (association request accepted), negative error code on failure
 */
int test_dect_association_request(
	struct net_if *iface, uint32_t target_long_rd_id,
	const struct test_dect_association_response_params *response_params,
	bool simulate_completion, bool simulate_network_joined,
	struct test_dect_association_result *result);

/**
 * @brief Get DECT status information (synchronous)
 *
 * This utility function performs a synchronous status info request and returns
 * the current DECT stack status. This is a synchronous operation that does not
 * require callback simulation.
 *
 * @param iface Network interface to use for the status request
 * @param status_info Pointer to status info structure to populate. Must not be NULL.
 *
 * @return 0 on success, negative error code on failure
 */
int test_dect_status_info_get(struct net_if *iface, struct dect_status_info *status_info);

/**
 * @brief Association release result structure
 */
struct test_dect_association_release_result {
	bool association_changed_received; /* True if NET_EVENT_DECT_ASSOCIATION_CHANGED rcvd */
	struct dect_association_changed_evt association_data; /* Received ass event data */
	bool network_status_received; /* True if NET_EVENT_DECT_NETWORK_STATUS received */
	struct dect_network_status_evt network_status_data; /* Received nw status event data */
};

/**
 * @brief Perform a DECT association release request with optional completion simulation
 *
 * This utility function performs an association release operation and optionally simulates
 * association release completion and network unjoined status. It handles the complete
 * association release workflow:
 * 1. Initiates the association release via net_mgmt API
 * 2. Optionally simulates association release operation completion callback
 * 3. Optionally simulates network unjoined status event
 * 4. Returns association release results
 *
 * @param iface Network interface to use for the association release
 * @param target_long_rd_id Long RD ID of the target device to release association with
 * @param simulate_completion If true, simulate association release completion callback.
 *                            If false, caller must handle completion separately.
 * @param simulate_network_unjoined If true, simulate network unjoined status event
 *                                   after successful association release.
 * @param result Pointer to result structure to populate. Must not be NULL.
 *
 * @return 0 on success (association release request accepted), negative error code on failure
 */
int test_dect_association_release(struct net_if *iface, uint32_t target_long_rd_id,
				  bool simulate_completion, bool simulate_network_unjoined,
				  struct test_dect_association_release_result *result);

/**
 * @brief Deactivation result structure
 */
struct test_dect_deactivate_result {
	bool deactivate_done_received; /* True if NET_EVENT_DECT_DEACTIVATE_DONE was received */
	enum dect_status_values deactivate_done_status; /* Deactivation completion status */
};

/**
 * @brief Activation result structure
 */
struct test_dect_activate_result {
	bool activate_done_received; /* True if NET_EVENT_DECT_ACTIVATE_DONE was received */
	enum dect_status_values activate_done_status; /* Activation completion status */
	bool configure_called;		 /* True if nrf_modem_dect_control_configure was called */
	bool functional_mode_set_called; /* True if functional mode set API was called */
};

/**
 * @brief Perform a DECT stack activation request
 *
 * This utility function performs a DECT stack activation operation. It handles the
 * activation workflow:
 * 1. Initiates the activation via net_mgmt API
 * 2. Waits for async activation completion event
 * 3. Returns activation results
 *
 * @param iface Network interface to use for the activation
 * @param wait_timeout_ms Timeout in milliseconds to wait for activation completion.
 *                        If 0, uses default timeout (250ms).
 * @param result Pointer to result structure to populate. Must not be NULL.
 *
 * @return 0 on success (activation request accepted), negative error code on failure
 */
int test_dect_perform_activate(struct net_if *iface, uint32_t wait_timeout_ms,
			       struct test_dect_activate_result *result);

/**
 * @brief Perform a DECT stack deactivation request
 *
 * This utility function performs a DECT stack deactivation operation. It handles the
 * deactivation workflow:
 * 1. Initiates the deactivation via net_mgmt API
 * 2. Waits for async deactivation completion event
 * 3. Returns deactivation results
 *
 * @param iface Network interface to use for the deactivation
 * @param wait_timeout_ms Timeout in milliseconds to wait for deactivation completion.
 *                        If 0, uses default timeout (250ms).
 * @param result Pointer to result structure to populate. Must not be NULL.
 *
 * @return 0 on success (deactivation request accepted), negative error code on failure
 */
int test_dect_perform_deactivate(struct net_if *iface, uint32_t wait_timeout_ms,
				 struct test_dect_deactivate_result *result);

/**
 * @brief RSSI scan result structure
 */
struct test_dect_rssi_scan_result {
	bool rssi_scan_result_received; /* True if NET_EVENT_DECT_RSSI_SCAN_RESULT was rcvd */
	bool rssi_scan_done_received;	/* True if NET_EVENT_DECT_RSSI_SCAN_DONE was rcvd */
	enum dect_status_values rssi_scan_done_status;		/* RSSI scan completion status */
	struct dect_rssi_scan_result_evt rssi_scan_result_data; /* Received RSSI scan results */
};

/**
 * @brief Perform a DECT RSSI scan request
 *
 * This utility function performs a DECT RSSI scan operation. It handles the RSSI scan workflow:
 * 1. Initiates the RSSI scan via net_mgmt API
 * 2. Continues even if net_mgmt returns an error, allowing async completion checks
 * 3. Waits for async RSSI scan result notification (rssi_scan_ntf callback)
 * 4. Captures whether NET_EVENT_DECT_RSSI_SCAN_RESULT is received
 * 5. Waits for async RSSI scan completion (rssi_scan op callback)
 * 6. Captures NET_EVENT_DECT_RSSI_SCAN_DONE status in the result structure
 * 7. Returns collected RSSI scan results
 *
 * @param iface Network interface to use for the RSSI scan
 * @param params Pointer to RSSI scan parameters. Must not be NULL.
 * @param wait_timeout_ms Timeout in milliseconds to wait for RSSI scan completion.
 *                        If 0, uses default timeout (500ms).
 * @param result Pointer to result structure to populate. Must not be NULL.
 *
 * @return 0 on completion, negative error code only for invalid input parameters
 */
int test_dect_perform_rssi_scan(struct net_if *iface, struct dect_rssi_scan_params *params,
				uint32_t wait_timeout_ms,
				struct test_dect_rssi_scan_result *result);

/**
 * @brief Cluster configure result structure
 */
struct test_dect_cluster_configure_result {
	bool cluster_configure_called;
};

/**
 * @brief Perform a DECT cluster configure operation
 *
 * This utility function verifies that cluster configure was called and waits for
 * the async callback to complete. The mock automatically simulates the async callback.
 *
 * @param iface Network interface (unused, kept for consistency with other utilities)
 * @param wait_timeout_ms Timeout in milliseconds to wait for cluster configure completion.
 *                        If 0, uses default timeout (200ms).
 * @param result Pointer to result structure to populate. Must not be NULL.
 *
 * @return 0 on success, negative error code on failure
 */
int test_dect_perform_cluster_configure(struct net_if *iface, uint32_t wait_timeout_ms,
					struct test_dect_cluster_configure_result *result);

#ifdef __cplusplus
}
#endif

#endif /* TEST_DECT_UTILS_H_ */
