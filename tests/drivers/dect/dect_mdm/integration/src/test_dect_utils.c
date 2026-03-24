/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file test_dect_utils.c
 * @brief Common test utilities implementation for DECT integration tests
 */

#include "test_dect_utils.h"
#include "mock_nrf_modem_dect_mac.h"
#include <zephyr/net/net_mgmt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <errno.h>

LOG_MODULE_REGISTER(test_dect_utils, LOG_LEVEL_INF);

/* External event tracking variables from test_dect_integration.c */
extern bool dect_scan_result_received;
extern bool dect_scan_done_received;
extern enum dect_status_values dect_scan_done_status;
extern struct dect_scan_result_evt received_beacon_data;
extern bool beacon_data_valid;

extern bool dect_association_changed_received;
extern bool dect_association_created_received;
extern bool dect_association_failed_received;
extern struct dect_association_changed_evt received_association_data;
extern bool dect_network_status_received;
extern struct dect_network_status_evt received_network_status_data;
extern bool dect_activate_done_received;
extern enum dect_status_values dect_activate_done_status;
extern bool dect_deactivate_done_received;
extern enum dect_status_values dect_deactivate_done_status;
extern bool dect_rssi_scan_result_received;
extern struct dect_rssi_scan_result_evt received_rssi_scan_result_data;
extern bool dect_rssi_scan_done_received;
extern enum dect_status_values dect_rssi_scan_done_status;

/* External mock call counters */
extern int mock_nrf_modem_dect_control_configure_call_count;
extern int mock_nrf_modem_dect_control_functional_mode_set_call_count;
extern int mock_nrf_modem_dect_mac_cluster_configure_call_count;

int test_dect_network_scan(struct net_if *iface, const struct dect_scan_params *scan_params,
			   const struct test_dect_scan_beacon_params *beacon_params,
			   bool simulate_completion, struct test_dect_scan_result *result)
{
	int ret;

	if (!iface || !scan_params || !result) {
		return -EINVAL;
	}

	/* Initialize result structure */
	memset(result, 0, sizeof(*result));

	/* Reset scan event tracking flags */
	dect_scan_result_received = false;
	dect_scan_done_received = false;
	beacon_data_valid = false;

	/* State persists between tests - mock call counters are not reset */

	/* Call the real net_mgmt API for DECT scan - this should trigger
	 * nrf_modem_dect_mac_network_scan
	 */
	ret = net_mgmt(NET_REQUEST_DECT_SCAN, iface, (void *)scan_params, sizeof(*scan_params));

	if (ret != 0) {
		LOG_ERR("Network scan request failed: %d", ret);
		return ret;
	}

	/* Wait a short time for scan to start */
	k_sleep(K_MSEC(50));

	/* Optionally simulate cluster beacon reception */
	if (beacon_params && mock_ntf_callbacks.cluster_beacon_ntf) {
		struct nrf_modem_dect_mac_cluster_beacon_ntf_cb_params beacon_cb_params = {
			.channel = beacon_params->channel,
			.transmitter_short_rd_id = beacon_params->transmitter_short_rd_id,
			.transmitter_long_rd_id = beacon_params->transmitter_long_rd_id,
			.network_id = beacon_params->network_id,
			.rx_signal_info = {.mcs = beacon_params->mcs,
					   .transmit_power = beacon_params->transmit_power,
					   .rssi_2 = beacon_params->rssi_2,
					   .snr = beacon_params->snr}};

		LOG_DBG("Simulating cluster beacon callback: channel=%d, short_rd_id=0x%04X, "
			"long_rd_id=0x%08X",
			beacon_cb_params.channel, beacon_cb_params.transmitter_short_rd_id,
			beacon_cb_params.transmitter_long_rd_id);

		mock_ntf_callbacks.cluster_beacon_ntf(&beacon_cb_params);

		/* Wait for beacon processing */
		k_sleep(K_MSEC(100));
	} else if (beacon_params) {
		LOG_WRN("Beacon parameters provided but no cluster beacon callback registered");
	}

	/* Optionally simulate network beacon reception */
	if (beacon_params && mock_ntf_callbacks.network_beacon_ntf) {
		struct nrf_modem_dect_mac_network_beacon_ntf_cb_params nw_beacon_cb_params = {
			.channel = beacon_params->channel,
			.transmitter_short_rd_id = beacon_params->transmitter_short_rd_id,
			.transmitter_long_rd_id = beacon_params->transmitter_long_rd_id,
			.network_id = beacon_params->network_id,
			.rx_signal_info = {.mcs = beacon_params->mcs,
					   .transmit_power = beacon_params->transmit_power,
					   .rssi_2 = beacon_params->rssi_2,
					   .snr = beacon_params->snr},
			.number_of_ies = 0,
			.ies = NULL};

		LOG_DBG("Simulating network beacon callback: channel=%d, short_rd_id=0x%04X, "
			"long_rd_id=0x%08X",
			nw_beacon_cb_params.channel, nw_beacon_cb_params.transmitter_short_rd_id,
			nw_beacon_cb_params.transmitter_long_rd_id);

		mock_ntf_callbacks.network_beacon_ntf(&nw_beacon_cb_params);

		/* Wait for beacon processing */
		k_sleep(K_MSEC(100));
	}

	/* Optionally simulate scan completion */
	if (simulate_completion && mock_op_callbacks.network_scan) {
		struct nrf_modem_dect_mac_network_scan_cb_params scan_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK,
			.num_scanned_channels = scan_params->channel_count};

		LOG_DBG("Simulating scan completion: status=OK, channels=%d",
			scan_complete_params.num_scanned_channels);

		mock_op_callbacks.network_scan(&scan_complete_params);

		/* Allow time for scan completion processing */
		k_sleep(K_MSEC(100));
	}

	/* Populate result structure */
	result->scan_result_received = dect_scan_result_received;
	result->scan_done_received = dect_scan_done_received;
	result->scan_done_status = dect_scan_done_status;
	result->beacon_data_valid = beacon_data_valid;
	if (beacon_data_valid) {
		memcpy(&result->beacon_data, &received_beacon_data, sizeof(result->beacon_data));
	}

	LOG_DBG("Network scan completed: result_received=%s, done_received=%s, status=%d",
		result->scan_result_received ? "true" : "false",
		result->scan_done_received ? "true" : "false", result->scan_done_status);

	return 0;
}

int test_dect_association_request(
	struct net_if *iface, uint32_t target_long_rd_id,
	const struct test_dect_association_response_params *response_params,
	bool simulate_completion, bool simulate_network_joined,
	struct test_dect_association_result *result)
{
	int ret;
	struct dect_associate_req_params assoc_params = {.target_long_rd_id = target_long_rd_id};

	/* Default association response parameters */
	struct test_dect_association_response_params default_response = {
		.ack_status = true,
		.number_of_flows = 1,
		.number_of_rx_harq_processes = 4,
		.max_number_of_harq_re_rx = 3,
		.number_of_tx_harq_processes = 4,
		.max_number_of_harq_re_tx = 3};

	const struct test_dect_association_response_params *resp_params =
		response_params ? response_params : &default_response;

	if (!iface || !result) {
		return -EINVAL;
	}

	/* Initialize result structure */
	memset(result, 0, sizeof(*result));

	/* Reset association event tracking flags */
	dect_association_changed_received = false;
	dect_association_created_received = false;
	dect_association_failed_received = false;
	dect_network_status_received = false;

	/* State persists between tests - mock call counters are not reset */

	LOG_DBG("Associating with Long RD ID: 0x%08X", target_long_rd_id);

	/* Call the real net_mgmt API for DECT association - this should trigger
	 * nrf_modem_dect_mac_association
	 */
	ret = net_mgmt(NET_REQUEST_DECT_ASSOCIATION, iface, &assoc_params, sizeof(assoc_params));

	if (ret != 0) {
		LOG_ERR("Association request failed: %d", ret);
		return ret;
	}

	/* Wait a short time for association to start */
	k_sleep(K_MSEC(50));

	/* Optionally simulate association operation completion */
	if (simulate_completion && mock_op_callbacks.association) {
		struct nrf_modem_dect_mac_association_cb_params assoc_op_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK,
			.long_rd_id = target_long_rd_id,
			.rx_signal_info = {0},
			.ipv6_config = {0},
			.number_of_ies = 0,
			.ies = NULL};

		/* Initialize association response structure */
		assoc_op_params.association_response.bit_mask = 0;
		assoc_op_params.association_response.ack_status = resp_params->ack_status;
		assoc_op_params.association_response.number_of_flows = resp_params->number_of_flows;
		assoc_op_params.association_response.number_of_rx_harq_processes =
			resp_params->number_of_rx_harq_processes;
		assoc_op_params.association_response.max_number_of_harq_re_rx =
			resp_params->max_number_of_harq_re_rx;
		assoc_op_params.association_response.number_of_tx_harq_processes =
			resp_params->number_of_tx_harq_processes;
		assoc_op_params.association_response.max_number_of_harq_re_tx =
			resp_params->max_number_of_harq_re_tx;

		/* Set the flag to indicate we have association response data */
		assoc_op_params.flags.has_association_response = 1;

		LOG_DBG("Simulating association operation completion: status=OK, "
			"long_rd_id=0x%08X, ack_status=%s",
			assoc_op_params.long_rd_id,
			assoc_op_params.association_response.ack_status ? "true" : "false");

		mock_op_callbacks.association(&assoc_op_params);

		/* Wait for association operation processing */
		k_sleep(K_MSEC(50));
		k_sleep(K_MSEC(100));
	} else if (simulate_completion) {
		LOG_WRN("Association completion requested but no association callback registered");
	}

	/* Optionally simulate network joined status after successful association */
	if (simulate_network_joined && simulate_completion) {
		LOG_DBG("Simulating network joined status after successful association");

		struct dect_network_status_evt network_joined_event = {
			.network_status = DECT_NETWORK_STATUS_JOINED,
			.dect_err_cause = DECT_STATUS_OK,
			.os_err_cause = 0};

		/* Simulate sending the network status event through the real DECT stack */
		dect_mgmt_network_status_evt(iface, network_joined_event);

		/* Wait for network status event processing */
		k_sleep(K_MSEC(50));
	}

	/* Populate result structure */
	result->association_changed_received = dect_association_changed_received;
	result->association_created_received = dect_association_created_received;
	result->association_failed_received = dect_association_failed_received;
	if (dect_association_changed_received) {
		memcpy(&result->association_data, &received_association_data,
		       sizeof(result->association_data));
	}

	result->network_status_received = dect_network_status_received;
	if (dect_network_status_received) {
		memcpy(&result->network_status_data, &received_network_status_data,
		       sizeof(result->network_status_data));
	}

	LOG_DBG("Association completed: changed_received=%s, created_received=%s, "
		"failed_received=%s, network_status_received=%s",
		result->association_changed_received ? "true" : "false",
		result->association_created_received ? "true" : "false",
		result->association_failed_received ? "true" : "false",
		result->network_status_received ? "true" : "false");

	return 0;
}

int test_dect_status_info_get(struct net_if *iface, struct dect_status_info *status_info)
{
	int ret;

	if (!iface || !status_info) {
		return -EINVAL;
	}

	/* Initialize status info structure */
	memset(status_info, 0, sizeof(*status_info));

	LOG_DBG("Requesting DECT status info with NET_REQUEST_DECT_STATUS_INFO_GET");

	/* Make synchronous status info request */
	ret = net_mgmt(NET_REQUEST_DECT_STATUS_INFO_GET, iface, status_info, sizeof(*status_info));

	if (ret != 0) {
		LOG_ERR("Status info request failed: %d", ret);
		return ret;
	}

	LOG_DBG("Status info received:");
	LOG_DBG("- mdm_activated: %s", status_info->mdm_activated ? "true" : "false");
	LOG_DBG("- parent_count: %d", status_info->parent_count);
	LOG_DBG("- child_count: %d", status_info->child_count);
	LOG_DBG("- cluster_running: %s", status_info->cluster_running ? "true" : "false");
	LOG_DBG("- nw_beacon_running: %s", status_info->nw_beacon_running ? "true" : "false");
	if (status_info->parent_count > 0) {
		LOG_DBG("- parent_associations[0].long_rd_id: 0x%08X",
			status_info->parent_associations[0].long_rd_id);
	}

	return 0;
}

int test_dect_association_release(struct net_if *iface, uint32_t target_long_rd_id,
				  bool simulate_completion, bool simulate_network_unjoined,
				  struct test_dect_association_release_result *result)
{
	int ret;
	struct dect_associate_rel_params release_params = {.target_long_rd_id = target_long_rd_id};

	if (!iface || !result) {
		return -EINVAL;
	}

	/* Initialize result structure */
	memset(result, 0, sizeof(*result));

	/* Reset association event tracking flags */
	dect_association_changed_received = false;
	dect_network_status_received = false;

	/* State persists between tests - mock call counters are not reset */

	LOG_DBG("Releasing association with Long RD ID: 0x%08X", target_long_rd_id);

	/* Request association release */
	ret = net_mgmt(NET_REQUEST_DECT_ASSOCIATION_RELEASE, iface, &release_params,
		       sizeof(release_params));

	if (ret != 0) {
		LOG_ERR("Association release request failed: %d", ret);
		return ret;
	}

	/* Optionally simulate association release operation completion */
	if (simulate_completion && mock_op_callbacks.association_release) {
		struct nrf_modem_dect_mac_association_release_cb_params release_op_params = {
			.long_rd_id = target_long_rd_id};

		LOG_DBG("Simulating association release operation completion for long_rd_id=0x%08X",
			release_op_params.long_rd_id);

		mock_op_callbacks.association_release(&release_op_params);

		/* Wait for association release processing */
		k_sleep(K_MSEC(100));
	} else if (simulate_completion) {
		LOG_WRN("Association release completion requested but no callback registered");
	}

	/* Optionally simulate network unjoined status after association release */
	if (simulate_network_unjoined && simulate_completion) {
		LOG_DBG("Simulating network unjoined status after association release");

		/* Reset network status event flag */
		dect_network_status_received = false;

		struct dect_network_status_evt network_unjoined_event = {
			.network_status = DECT_NETWORK_STATUS_UNJOINED,
			.dect_err_cause = DECT_STATUS_OK,
			.os_err_cause = 0};

		/* Simulate sending the network status event through the real DECT stack */
		dect_mgmt_network_status_evt(iface, network_unjoined_event);

		/* Wait for network status event processing */
		k_sleep(K_MSEC(50));
	}

	/* Populate result structure */
	result->association_changed_received = dect_association_changed_received;
	if (dect_association_changed_received) {
		memcpy(&result->association_data, &received_association_data,
		       sizeof(result->association_data));
	}

	result->network_status_received = dect_network_status_received;
	if (dect_network_status_received) {
		memcpy(&result->network_status_data, &received_network_status_data,
		       sizeof(result->network_status_data));
	}

	LOG_DBG("Association release completed: changed_received=%s, network_status_received=%s",
		result->association_changed_received ? "true" : "false",
		result->network_status_received ? "true" : "false");

	return 0;
}

int test_dect_perform_activate(struct net_if *iface, uint32_t wait_timeout_ms,
			       struct test_dect_activate_result *result)
{
	int ret;
	uint32_t timeout = wait_timeout_ms > 0 ? wait_timeout_ms : 250;

	if (!iface || !result) {
		return -EINVAL;
	}

	/* Initialize result structure */
	memset(result, 0, sizeof(*result));

	/* Reset activation event flag */
	dect_activate_done_received = false;

	/* Record baseline call counts (state persists between tests) */
	int baseline_configure = mock_nrf_modem_dect_control_configure_call_count;
	int baseline_functional_mode_set =
		mock_nrf_modem_dect_control_functional_mode_set_call_count;

	LOG_DBG("Requesting DECT stack activation with NET_REQUEST_DECT_ACTIVATE");

	/* Request DECT stack activation - no parameters needed */
	ret = net_mgmt(NET_REQUEST_DECT_ACTIVATE, iface, NULL, 0);

	if (ret != 0) {
		LOG_ERR("DECT activation request failed: %d", ret);
		return ret;
	}

	/* Wait a short time for the driver to call nrf_modem_dect_control_configure */
	k_sleep(K_MSEC(20));

	/* Verify that nrf_modem_dect_control_configure was called */
	if (mock_nrf_modem_dect_control_configure_call_count != baseline_configure + 1) {
		LOG_WRN("nrf_modem_dect_control_configure call count mismatch: expected %d, got %d",
			baseline_configure + 1, mock_nrf_modem_dect_control_configure_call_count);
	}

	/* Wait for configure op callback to complete (this triggers functional_mode_set) */
	k_sleep(K_MSEC(20));

	/* Verify that nrf_modem_dect_control_functional_mode_set was called after configure */
	if (mock_nrf_modem_dect_control_functional_mode_set_call_count !=
	    baseline_functional_mode_set + 1) {
		LOG_WRN("nrf_modem_dect_control_functional_mode_set "
			"call count mismatch: expected %d, got %d",
			baseline_functional_mode_set + 1,
			mock_nrf_modem_dect_control_functional_mode_set_call_count);
	}

	/* The mock automatically simulates the control_functional_mode callback via
	 * simulate_async_callback, which triggers the async callback chain that eventually
	 * results in NET_EVENT_DECT_ACTIVATE_DONE. Wait for the full activation process.
	 */
	LOG_DBG("DECT activation request sent, waiting %d ms for completion "
		"(configure and functional_mode_set called, waiting for async callback)",
		timeout);

	/* Wait for activation processing with async callback */
	k_sleep(K_MSEC(timeout));

	/* Populate result structure */
	result->activate_done_received = dect_activate_done_received;
	result->activate_done_status = dect_activate_done_status;
	result->configure_called =
		(mock_nrf_modem_dect_control_configure_call_count == baseline_configure + 1);
	result->functional_mode_set_called =
		(mock_nrf_modem_dect_control_functional_mode_set_call_count ==
		 baseline_functional_mode_set + 1);

	LOG_DBG("DECT activation completed: done_received=%s, status=%d, configure_called=%s, "
		"functional_mode_set_called=%s",
		result->activate_done_received ? "true" : "false", result->activate_done_status,
		result->configure_called ? "true" : "false",
		result->functional_mode_set_called ? "true" : "false");

	return 0;
}

int test_dect_perform_deactivate(struct net_if *iface, uint32_t wait_timeout_ms,
				 struct test_dect_deactivate_result *result)
{
	int ret;
	uint32_t timeout = wait_timeout_ms > 0 ? wait_timeout_ms : 250;

	if (!iface || !result) {
		return -EINVAL;
	}

	/* Initialize result structure */
	memset(result, 0, sizeof(*result));

	/* Reset deactivation event flag */
	dect_deactivate_done_received = false;

	LOG_DBG("Requesting DECT stack deactivation with NET_REQUEST_DECT_DEACTIVATE");

	/* Request DECT stack deactivation - no parameters needed */
	ret = net_mgmt(NET_REQUEST_DECT_DEACTIVATE, iface, NULL, 0);

	if (ret != 0) {
		LOG_ERR("DECT deactivation request failed: %d", ret);
		return ret;
	}

	LOG_DBG("DECT deactivation request sent, waiting %d ms for completion", timeout);

	/* Wait for deactivation processing with async callback */
	k_sleep(K_MSEC(timeout));

	/* Populate result structure */
	result->deactivate_done_received = dect_deactivate_done_received;
	result->deactivate_done_status = dect_deactivate_done_status;

	LOG_DBG("DECT deactivation completed: done_received=%s, status=%d",
		result->deactivate_done_received ? "true" : "false",
		result->deactivate_done_status);

	return 0;
}

int test_dect_perform_rssi_scan(struct net_if *iface, struct dect_rssi_scan_params *params,
				uint32_t wait_timeout_ms, struct test_dect_rssi_scan_result *result)
{
	int ret;
	uint32_t timeout = wait_timeout_ms > 0 ? wait_timeout_ms : 500;

	if (!iface || !params || !result) {
		return -EINVAL;
	}

	/* Initialize result structure */
	memset(result, 0, sizeof(*result));

	/* Reset RSSI scan event flags */
	dect_rssi_scan_result_received = false;
	dect_rssi_scan_done_received = false;

	/* State persists between tests - mock call counters are not reset */

	LOG_DBG("Requesting DECT RSSI scan with NET_REQUEST_DECT_RSSI_SCAN");

	/* Request DECT RSSI scan */
	ret = net_mgmt(NET_REQUEST_DECT_RSSI_SCAN, iface, params, sizeof(*params));

	if (ret != 0) {
		LOG_WRN("DECT RSSI scan request returned error: %d "
			"(may be accepted but fail async)",
			ret);
		/* Continue anyway - the request may be accepted but fail asynchronously */
		/* Wait a bit to see if the driver still processes it */
		k_sleep(K_MSEC(20));
	}

	/* Wait a short time for the driver to call nrf_modem_dect_mac_rssi_scan */
	k_sleep(K_MSEC(10));

	/* The mock automatically simulates the rssi_scan_ntf notification callback via
	 * simulate_async_callback, which triggers NET_EVENT_DECT_RSSI_SCAN_RESULT.
	 * Then it simulates the rssi_scan op callback which triggers NET_EVENT_DECT_RSSI_SCAN_DONE.
	 * Wait for the full RSSI scan process.
	 */
	LOG_DBG("DECT RSSI scan request sent, waiting %d ms for completion "
		"(rssi_scan called, waiting for async callbacks)",
		timeout);

	/* Wait for RSSI scan processing with async callbacks */
	k_sleep(K_MSEC(timeout));

	/* Populate result structure */
	result->rssi_scan_result_received = dect_rssi_scan_result_received;
	result->rssi_scan_done_received = dect_rssi_scan_done_received;
	result->rssi_scan_done_status = dect_rssi_scan_done_status;
	if (dect_rssi_scan_result_received) {
		memcpy(&result->rssi_scan_result_data, &received_rssi_scan_result_data,
		       sizeof(result->rssi_scan_result_data));

		/* Verify the content of RSSI scan result data */
		struct dect_rssi_scan_result_data *rssi_data =
			&result->rssi_scan_result_data.rssi_scan_result;

		/* Verify channel matches the requested channel */
		if (params && params->channel_count > 0) {
			if (rssi_data->channel != params->channel_list[0]) {
				LOG_WRN("RSSI scan result channel mismatch: expected %d, got %d",
					params->channel_list[0], rssi_data->channel);
			}
		}

		LOG_DBG("DECT RSSI scan result data: channel=%d, busy_percentage=%d%%, "
			"all_subslots_free=%s, scan_suitable_percent=%d%%, "
			"free_subslot_cnt=%d, possible_subslot_cnt=%d, busy_subslot_cnt=%d, "
			"another_cluster_detected=%s",
			rssi_data->channel, rssi_data->busy_percentage,
			rssi_data->all_subslots_free ? "true" : "false",
			rssi_data->scan_suitable_percent, rssi_data->free_subslot_cnt,
			rssi_data->possible_subslot_cnt, rssi_data->busy_subslot_cnt,
			rssi_data->another_cluster_detected_in_channel ? "true" : "false");
	}

	LOG_DBG("DECT RSSI scan completed: result_received=%s, done_received=%s, status=%d",
		result->rssi_scan_result_received ? "true" : "false",
		result->rssi_scan_done_received ? "true" : "false", result->rssi_scan_done_status);

	return 0;
}

int test_dect_perform_cluster_configure(struct net_if *iface, uint32_t wait_timeout_ms,
					struct test_dect_cluster_configure_result *result)
{
	uint32_t timeout = wait_timeout_ms > 0 ? wait_timeout_ms : 200;

	ARG_UNUSED(iface);

	if (!result) {
		return -EINVAL;
	}

	/* Initialize result structure */
	memset(result, 0, sizeof(*result));

	/* Record baseline call count (state persists between tests) */
	int baseline_cluster_configure = mock_nrf_modem_dect_mac_cluster_configure_call_count;

	LOG_DBG("Waiting for cluster configure to be called and async callback to complete");

	/* Wait for cluster configure processing with async callback */
	/* The mock automatically simulates the cluster_configure op callback via
	 * simulate_async_callback, which triggers the async callback chain.
	 * Wait for the full cluster configure process.
	 */
	k_sleep(K_MSEC(timeout));

	/* Populate result structure */
	result->cluster_configure_called =
		(mock_nrf_modem_dect_mac_cluster_configure_call_count > baseline_cluster_configure);

	LOG_DBG("DECT cluster configure completed: called=%s (call_count=%d, baseline=%d)",
		result->cluster_configure_called ? "true" : "false",
		mock_nrf_modem_dect_mac_cluster_configure_call_count, baseline_cluster_configure);

	return 0;
}
