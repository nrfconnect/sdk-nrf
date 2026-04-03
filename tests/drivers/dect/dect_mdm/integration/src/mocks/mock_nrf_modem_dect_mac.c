/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Mock implementation for nrf_modem_dect_mac.h using real headers
 * This provides mock implementations of the actual modem API functions
 */

#include <nrf_modem_dect.h>
#include <nrf_modem.h>
#include "mock_nrf_modem_dect_mac.h"
#include "unity.h"
#include <string.h>
#include <stdbool.h>
#include <zephyr/linker/sections.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Mock modem info enum - simplified version for testing */
enum modem_info {
	MODEM_INFO_FW_VERSION = 1
	/* Add other values as needed */
};

LOG_MODULE_REGISTER(mock_nrf_modem_dect_mac, LOG_LEVEL_INF);

/* Mock state tracking */
static bool mock_modem_initialized;
static bool mock_dect_activated;
static uint16_t mock_last_configured_cluster_channel;
bool mock_cluster_creation_band1; /* Track if this is a cluster creation RSSI scan at band 1 */
/** When true, mock invokes association_release op callback when
 * nrf_modem_dect_mac_association_release is called (e.g. network remove with children).
 */
bool mock_auto_invoke_association_release_cb;

/* Test-controlled inject (one-shot: used once then reset to OK / false). */
enum nrf_modem_dect_mac_err mock_inject_configure_status = NRF_MODEM_DECT_MAC_STATUS_OK;
enum nrf_modem_dect_mac_err mock_inject_functional_mode_status = NRF_MODEM_DECT_MAC_STATUS_OK;
enum nrf_modem_dect_mac_err mock_inject_association_status = NRF_MODEM_DECT_MAC_STATUS_OK;
bool mock_inject_association_rejected;
int mock_inject_association_reject_cause;
int mock_inject_association_reject_time;

struct nrf_modem_dect_mac_op_callbacks mock_op_callbacks;
struct nrf_modem_dect_mac_ntf_callbacks mock_ntf_callbacks;

/* Async callback simulation using timer-based approach */
struct async_callback_work_item {
	void (*callback_func)(void *arg);
	void *params;
	uint8_t param_data[256]; /* Buffer to store callback parameters */
};

static void async_callback_timer_handler(struct k_timer *timer)
{
	struct async_callback_work_item *work_item =
		(struct async_callback_work_item *)k_timer_user_data_get(timer);

	if (work_item && work_item->callback_func && work_item->params) {
		/* Execute the callback */
		work_item->callback_func(work_item->params);
	}

	/* Clean up dynamically allocated memory */
	if (work_item) {
		k_free(work_item);
	}
	k_free(timer);
}

/* Helper function to simulate asynchronous callback with timer-based delay */
void simulate_async_callback(void (*callback_func)(void *), void *params)
{
	/* Use dynamic memory allocation to avoid static variable conflicts */
	struct async_callback_work_item *work_item =
		k_malloc(sizeof(struct async_callback_work_item));
	struct k_timer *async_timer = k_malloc(sizeof(struct k_timer));

	if (callback_func && params && work_item && async_timer) {
		/* Store callback and params for timer handler */
		work_item->callback_func = callback_func;

		/* Copy parameters to our buffer - use actual size for capability_ntf or max size */
		size_t param_size;

		if (callback_func == (void (*)(void *))mock_ntf_callbacks.capability_ntf) {
			/* capability_ntf_cb_params is larger, use sizeof */
			param_size = sizeof(struct nrf_modem_dect_mac_capability_ntf_cb_params);
		} else if (callback_func == (void (*)(void *))mock_op_callbacks.cluster_info) {
			param_size = sizeof(struct nrf_modem_dect_mac_cluster_info_cb_params);
		} else if (callback_func ==
			   (void (*)(void *))mock_op_callbacks.association_release) {
			param_size =
				sizeof(struct nrf_modem_dect_mac_association_release_cb_params);
		} else {
			/* Default size for other callback parameter structures */
			param_size = 64;
		}

		if (param_size <= sizeof(work_item->param_data)) {
			memcpy(work_item->param_data, params, param_size);
			work_item->params = (void *)work_item->param_data;

			/* Initialize timer with work item as user data */
			k_timer_init(async_timer, async_callback_timer_handler, NULL);
			k_timer_user_data_set(async_timer, work_item);

			/* Schedule callback execution after short delay to simulate async behavior
			 */
			k_timer_start(async_timer, K_MSEC(5), K_NO_WAIT);
		} else {
			/* Cleanup on parameter size error */
			k_free(work_item);
			k_free(async_timer);
		}
	} else {
		/* Cleanup on allocation failure */
		if (!work_item || !async_timer) {
			LOG_ERR("MOCK: Failed to allocate memory for async callback simulation");
		}
		if (work_item) {
			k_free(work_item);
		}
		if (async_timer) {
			k_free(async_timer);
		}
	}
}

/* Note: We don't access driver semaphores directly - the driver's own callbacks handle that */

/* CMock-style call counters */
int mock_nrf_modem_dect_mac_callback_set_call_count;
int mock_nrf_modem_dect_mac_network_scan_call_count;
int mock_nrf_modem_dect_mac_cluster_beacon_receive_call_count;
int mock_nrf_modem_dect_mac_association_call_count;
int mock_nrf_modem_dect_mac_association_release_call_count;
int mock_nrf_modem_dect_mac_neighbor_list_call_count;
/* Mock neighbor info call counter */
int mock_nrf_modem_dect_mac_neighbor_info_call_count;
int mock_nrf_modem_dect_control_systemmode_set_call_count;
int mock_nrf_modem_dect_control_configure_call_count;
int mock_nrf_modem_dect_control_functional_mode_set_call_count;
int mock_nrf_modem_dect_mac_rssi_scan_call_count;
int mock_nrf_modem_dect_mac_rssi_scan_stop_call_count;
int mock_nrf_modem_dect_mac_cluster_configure_call_count;
int mock_nrf_modem_dect_mac_cluster_info_call_count;
uint8_t mock_last_configured_ipv6_prefix[MOCK_IPV6_PREFIX_LEN];
int mock_last_configured_ipv6_prefix_type;
int mock_nrf_modem_dect_dlc_data_tx_call_count;

/* Storage for last DLC data TX parameters */
uint8_t mock_last_dlc_data_tx_data[MOCK_DLC_DATA_TX_MAX_SIZE];
size_t mock_last_dlc_data_tx_len;
uint32_t mock_last_dlc_data_tx_long_rd_id;
uint32_t mock_dlc_data_tx_long_rd_ids[MOCK_DLC_DATA_TX_RECORD_MAX];
int mock_dlc_data_tx_record_count;

/* Remove the extern - this is where we define the variables */

/* Expected return values for CMock-style functions */
static int expected_callback_set_return;
static int expected_network_scan_return;
static int expected_cluster_beacon_receive_return;

/* Mock function implementations */

int nrf_modem_dect_mac_callback_set(const struct nrf_modem_dect_mac_op_callbacks *op_cb,
				    const struct nrf_modem_dect_mac_ntf_callbacks *ntf_cb)
{
	mock_nrf_modem_dect_mac_callback_set_call_count++;

	printk("MOCK: callback_set called, op_cb=%p, ntf_cb=%p\n", op_cb, ntf_cb);

	if (op_cb) {
		memcpy(&mock_op_callbacks, op_cb, sizeof(mock_op_callbacks));
		printk("MOCK: Copied operation callbacks\n");
	}
	if (ntf_cb) {
		memcpy(&mock_ntf_callbacks, ntf_cb, sizeof(mock_ntf_callbacks));
		printk("MOCK: Copied notification callbacks, cluster_beacon_ntf=%p\n",
		       mock_ntf_callbacks.cluster_beacon_ntf);
	}
	mock_modem_initialized = true;
	return expected_callback_set_return;
}

int nrf_modem_dect_mac_network_scan(struct nrf_modem_dect_mac_network_scan_params *params)
{
	mock_nrf_modem_dect_mac_network_scan_call_count++;

	/* Check if the DECT stack is activated */
	if (!mock_dect_activated) {
		/* Stack is deactivated - simulate immediate failure via async callback */
		if (mock_op_callbacks.network_scan) {
			struct nrf_modem_dect_mac_network_scan_cb_params scan_fail_params = {
				.status = NRF_MODEM_DECT_MAC_STATUS_NOT_ALLOWED,
				.num_scanned_channels = 0};
			LOG_DBG("MOCK: Network scan not allowed - DECT stack is deactivated");
			simulate_async_callback((void (*)(void *))mock_op_callbacks.network_scan,
						&scan_fail_params);
		}
		return 0; /* Request accepted but will fail asynchronously */
	}

	/* Stack is activated - normal operation */
	/* For proper async testing, we don't auto-trigger callbacks here.
	 * The test will manually trigger:
	 * 1. ntf_callbacks.cluster_beacon_ntf (simulate beacon reception)
	 * 2. op_callbacks.network_scan (simulate scan completion)
	 */

	return expected_network_scan_return;
}

int nrf_modem_dect_mac_cluster_beacon_receive(
	struct nrf_modem_dect_mac_cluster_beacon_receive_params *params)
{
	mock_nrf_modem_dect_mac_cluster_beacon_receive_call_count++;
	mock_dect_activated = true;
	return expected_cluster_beacon_receive_return;
}

int nrf_modem_dect_mac_association(struct nrf_modem_dect_mac_association_params *params)
{
	mock_nrf_modem_dect_mac_association_call_count++;
	LOG_DBG("MOCK: nrf_modem_dect_mac_association called with long_rd_id=0x%08X, "
		"network_id=0x%08X",
		params->long_rd_id, params->network_id);

	/* Check if the DECT stack is activated */
	if (!mock_dect_activated) {
		/* Stack is deactivated - simulate association failure via async callback */
		if (mock_op_callbacks.association) {
			struct nrf_modem_dect_mac_association_cb_params assoc_fail_params = {
				.status = NRF_MODEM_DECT_MAC_STATUS_NOT_ALLOWED,
				.long_rd_id = params->long_rd_id};
			LOG_DBG("MOCK: Association not allowed - DECT stack is deactivated");
			simulate_async_callback((void (*)(void *))mock_op_callbacks.association,
						&assoc_fail_params);
		}
		return 0; /* Request accepted but will fail asynchronously */
	}

	/* Activated path: optionally inject failure or rejected result (one-shot). */
	if (mock_op_callbacks.association) {
		if (mock_inject_association_rejected) {
			struct nrf_modem_dect_mac_association_cb_params rej_params = {
				.status = NRF_MODEM_DECT_MAC_STATUS_OK,
				.long_rd_id = params->long_rd_id,
				.rx_signal_info = {0},
				.ipv6_config = {0},
				.number_of_ies = 0,
				.ies = NULL,
			};
			rej_params.flags.has_association_response = 1;
			rej_params.association_response.ack_status = 0;
			rej_params.association_response.reject_cause =
				mock_inject_association_reject_cause;
			rej_params.association_response.reject_time =
				mock_inject_association_reject_time;
			mock_inject_association_rejected = false;
			simulate_async_callback((void (*)(void *))mock_op_callbacks.association,
						&rej_params);
			return 0;
		}
		if (mock_inject_association_status != NRF_MODEM_DECT_MAC_STATUS_OK) {
			struct nrf_modem_dect_mac_association_cb_params fail_params = {
				.status = mock_inject_association_status,
				.long_rd_id = params->long_rd_id,
			};
			enum nrf_modem_dect_mac_err save = mock_inject_association_status;

			mock_inject_association_status = NRF_MODEM_DECT_MAC_STATUS_OK;
			simulate_async_callback((void (*)(void *))mock_op_callbacks.association,
						&fail_params);
			(void)save;
			return 0;
		}
	}

	return 0;
}

int nrf_modem_dect_mac_rssi_scan(struct nrf_modem_dect_mac_rssi_scan_params *params)
{
	mock_nrf_modem_dect_mac_rssi_scan_call_count++;

	LOG_DBG("MOCK: nrf_modem_dect_mac_rssi_scan called with channel_scan_length=%d, "
		"num_channels=%d, band=%d",
		params ? params->channel_scan_length : 0, params ? params->num_channels : 0,
		params ? params->band : 0);

	/* Check if the DECT stack is activated */
	if (!mock_dect_activated) {
		/* Stack is deactivated - simulate immediate failure via async callback */
		if (mock_op_callbacks.rssi_scan) {
			struct nrf_modem_dect_mac_rssi_scan_cb_params rssi_fail_params = {
				.status = NRF_MODEM_DECT_MAC_STATUS_NOT_ALLOWED};

			LOG_DBG("MOCK: RSSI scan not allowed - DECT stack is deactivated");
			simulate_async_callback((void (*)(void *))mock_op_callbacks.rssi_scan,
						&rssi_fail_params);
		}
		return 0; /* Request accepted but will fail asynchronously */
	}

	/* Stack is activated - simulate RSSI scan with async callbacks */
	if (params && params->num_channels > 0) {
		/* Reconfig path (IPv6 prefix change): single channel, one frame.
		 * Modem may bypass actual RSSI result but must still call completion
		 * callback so the driver proceeds to cluster_configure.
		 */
		bool reconfig_scan =
			(params->num_channels == 1 && params->channel_scan_length == 1);

		if (reconfig_scan) {
			if (mock_ntf_callbacks.rssi_scan_ntf) {
				static uint8_t busy_array[6] = {0};
				static uint8_t possible_array[6] = {0};
				static uint8_t free_array[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

				struct nrf_modem_dect_mac_rssi_scan_ntf_cb_params ntf_params = {
					.channel = params->channel_list[0],
					.busy_percentage = 10,
					.rssi_meas_array_size = 6,
					.busy = busy_array,
					.possible = possible_array,
					.free = free_array};

				LOG_DBG("MOCK: Reconfig RSSI scan - simulating rssi_scan_ntf "
					"for channel %d",
					ntf_params.channel);
				simulate_async_callback(
					(void (*)(void *))mock_ntf_callbacks.rssi_scan_ntf,
					&ntf_params);
			}
			if (mock_op_callbacks.rssi_scan) {
				struct nrf_modem_dect_mac_rssi_scan_cb_params rssi_done_params = {
					.status = NRF_MODEM_DECT_MAC_STATUS_OK};

				LOG_DBG("MOCK: Reconfig RSSI scan - simulating rssi_scan "
					"completion callback (for cluster_configure)");
				simulate_async_callback(
					(void (*)(void *))mock_op_callbacks.rssi_scan,
					&rssi_done_params);
			}
		} else if (mock_cluster_creation_band1) {
			/* Cluster creation RSSI scan (band 1, 11 odd channels per ETSI EN 301
			 * 406-2, V3.0.1, ch 4.3.2.3): Do NOT auto-simulate completion callback -
			 * the test case will simulate it manually in the correct order: RSSI
			 * results -> rssi_scan_stop() called -> rssi completion -> rssi stop
			 * completion
			 */
			LOG_DBG("MOCK: Cluster creation RSSI scan (flag set by test) - "
				"completion callback will be handled by test case");
		} else {
			/* Simulate RSSI scan notification callback for each channel
			 * For test purposes, simulate one result
			 */
			if (mock_ntf_callbacks.rssi_scan_ntf) {
				/* Allocate arrays for busy, possible, free subslots */
				static uint8_t busy_array[6] = {0};
				static uint8_t possible_array[6] = {0};
				static uint8_t free_array[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

				struct nrf_modem_dect_mac_rssi_scan_ntf_cb_params ntf_params = {
					.channel = params->channel_list[0],
					.busy_percentage = 10, /* 10% busy */
					.rssi_meas_array_size = 6,
					.busy = busy_array,
					.possible = possible_array,
					.free = free_array};

				LOG_DBG("MOCK: Simulating rssi_scan_ntf callback for channel %d",
					ntf_params.channel);
				simulate_async_callback(
					(void (*)(void *))mock_ntf_callbacks.rssi_scan_ntf,
					&ntf_params);
			}

			/* Simulate RSSI scan completion callback for regular explicit channel list
			 * scans
			 */
			if (mock_op_callbacks.rssi_scan) {
				struct nrf_modem_dect_mac_rssi_scan_cb_params rssi_done_params = {
					.status = NRF_MODEM_DECT_MAC_STATUS_OK};

				LOG_DBG("MOCK: Simulating rssi_scan op callback with success "
					"status");
				simulate_async_callback(
					(void (*)(void *))mock_op_callbacks.rssi_scan,
					&rssi_done_params);
			}
		}
	}
	/* For band-based scans (num_channels == 0):
	 * Completion callback is NOT simulated here - the test case will simulate it manually
	 */

	return 0;
}

int nrf_modem_dect_mac_association_release(
	struct nrf_modem_dect_mac_association_release_params *params)
{
	mock_nrf_modem_dect_mac_association_release_call_count++;
	LOG_DBG("MOCK: nrf_modem_dect_mac_association_release called with long_rd_id=0x%08X, "
		"release_cause=%d",
		params->long_rd_id, params->release_cause);

	/* Simulate modem completing the release via op callback (e.g. for network remove) */
	if (mock_auto_invoke_association_release_cb && mock_op_callbacks.association_release) {
		struct nrf_modem_dect_mac_association_release_cb_params cb_params = {
			.long_rd_id = params->long_rd_id,
		};

		simulate_async_callback((void (*)(void *))mock_op_callbacks.association_release,
					&cb_params);
	}
	return 0;
}

/* Stub implementations for other MAC functions */

int nrf_modem_dect_mac_cluster_beacon_receive_stop(void)
{
	return 0;
}

int nrf_modem_dect_mac_cluster_configure(struct nrf_modem_dect_mac_cluster_configure_params *params)
{
	mock_nrf_modem_dect_mac_cluster_configure_call_count++;

	if (params && params->cluster_config) {
		struct nrf_modem_dect_mac_cluster_config *cc = params->cluster_config;

		mock_last_configured_cluster_channel = cc->cluster_channel;
		mock_last_configured_ipv6_prefix_type = cc->ipv6_config.type;
		if (cc->ipv6_config.type == NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX) {
			memcpy(mock_last_configured_ipv6_prefix, cc->ipv6_config.address,
			       MOCK_IPV6_PREFIX_LEN);
		}
		LOG_DBG("MOCK: nrf_modem_dect_mac_cluster_configure called with channel=%d",
			params->cluster_config->cluster_channel);
	} else {
		mock_last_configured_ipv6_prefix_type = NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_NONE;
		LOG_DBG("MOCK: nrf_modem_dect_mac_cluster_configure called with NULL params");
	}

	if (mock_modem_initialized) {
		/* Simulate asynchronous callback after successful cluster configuration */
		if (mock_op_callbacks.cluster_configure) {
			struct nrf_modem_dect_mac_cluster_configure_cb_params cb_params = {
				.status = NRF_MODEM_DECT_MAC_STATUS_OK};

			LOG_DBG("MOCK: Simulating cluster_configure op callback with success "
				"status");
			simulate_async_callback(
				(void (*)(void *))mock_op_callbacks.cluster_configure, &cb_params);
		}
		return 0;
	}

	return 0;
}

int nrf_modem_dect_mac_network_scan_stop(void)
{
	return 0;
}

int nrf_modem_dect_mac_rssi_scan_stop(void)
{
	mock_nrf_modem_dect_mac_rssi_scan_stop_call_count++;

	LOG_DBG("MOCK: nrf_modem_dect_mac_rssi_scan_stop called");

	/* Note: For band-based scans (cluster creation phase), the test manually calls
	 * the operation callbacks in the correct order:
	 * 1. RSSI scan completion callback (rssi_scan op callback)
	 * 2. RSSI scan stop completion callback (rssi_scan_stop op callback)
	 * So we don't auto-simulate them here.
	 */
	return 0;
}

int nrf_modem_dect_mac_network_beacon_configure(
	struct nrf_modem_dect_mac_network_beacon_configure_params *params)
{
	if (params && mock_op_callbacks.network_beacon_configure) {
		struct nrf_modem_dect_mac_network_beacon_configure_cb_params cb_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK,
		};
		simulate_async_callback(
			(void (*)(void *))mock_op_callbacks.network_beacon_configure, &cb_params);
	}
	return 0;
}

int nrf_modem_dect_mac_cluster_info(void)
{
	mock_nrf_modem_dect_mac_cluster_info_call_count++;
	LOG_DBG("MOCK: nrf_modem_dect_mac_cluster_info called");

	if (!mock_dect_activated) {
		if (mock_op_callbacks.cluster_info) {
			struct nrf_modem_dect_mac_cluster_info_cb_params cb_params = {
				.status = NRF_MODEM_DECT_MAC_STATUS_NOT_ALLOWED,
			};

			LOG_DBG("MOCK: Cluster info not allowed - DECT stack is deactivated");
			simulate_async_callback((void (*)(void *))mock_op_callbacks.cluster_info,
						&cb_params);
		}
		return 0;
	}

	if (mock_op_callbacks.cluster_info) {
		struct nrf_modem_dect_mac_cluster_info_cb_params cb_params;
		static uint8_t busy_array[6] = {0};
		static uint8_t possible_array[6] = {0};
		static uint8_t free_array[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

		memset(&cb_params, 0, sizeof(cb_params));
		cb_params.status = NRF_MODEM_DECT_MAC_STATUS_OK;
		cb_params.info.num_association_requests = 0;
		cb_params.info.num_association_failures = 0;
		cb_params.info.num_neighbors = 0;
		cb_params.info.num_ftpt_neighbors = 0;
		cb_params.info.num_rach_rx_pdc = 0;
		cb_params.info.num_rach_rx_pcc_crc_failures = 0;
		cb_params.info.rssi_result.channel = mock_last_configured_cluster_channel != 0
							     ? mock_last_configured_cluster_channel
							     : 1657;
		cb_params.info.rssi_result.busy_percentage = 0;
		for (int i = 0; i < ARRAY_SIZE(free_array); i++) {
			cb_params.info.rssi_result.busy[i] = busy_array[i];
			cb_params.info.rssi_result.possible[i] = possible_array[i];
			cb_params.info.rssi_result.free[i] = free_array[i];
		}
		simulate_async_callback((void (*)(void *))mock_op_callbacks.cluster_info,
					&cb_params);
	}
	return 0;
}

int nrf_modem_dect_mac_neighbor_info(struct nrf_modem_dect_mac_neighbor_info_params *params)
{
	mock_nrf_modem_dect_mac_neighbor_info_call_count++;

	if (params) {
		LOG_DBG("MOCK: nrf_modem_dect_mac_neighbor_info called with long_rd_id=0x%08X",
			params->long_rd_id);
	} else {
		LOG_DBG("MOCK: nrf_modem_dect_mac_neighbor_info called with NULL params");
	}

	if (mock_modem_initialized) {
		/* Simulate successful neighbor info request - will call back later */
		return 0;
	}

	return -1;
}

/* Mock neighbor list function */
int nrf_modem_dect_mac_neighbor_list(void)
{
	mock_nrf_modem_dect_mac_neighbor_list_call_count++;

	LOG_DBG("MOCK: nrf_modem_dect_mac_neighbor_list called");

	if (mock_modem_initialized) {
		/* Simulate successful neighbor list request - will call back later */
		return 0;
	}

	return -1;
}

/* Simulate NRF_MODEM_LIB_ON_INIT callback system - now using direct function call */
int nrf_modem_dect_dlc_data_tx(struct nrf_modem_dect_dlc_data_tx_params *params)
{
	mock_nrf_modem_dect_dlc_data_tx_call_count++;

	if (params) {
		LOG_INF("MOCK: nrf_modem_dect_dlc_data_tx called: long_rd_id=0x%08X, "
			"data_len=%zu, flow_id=%d",
			params->long_rd_id, params->data_len, params->flow_id);

		/* Store the transmitted data for verification */
		mock_last_dlc_data_tx_long_rd_id = params->long_rd_id;
		mock_last_dlc_data_tx_len = params->data_len;
		if (params->data && params->data_len <= MOCK_DLC_DATA_TX_MAX_SIZE) {
			memcpy(mock_last_dlc_data_tx_data, params->data, params->data_len);
		}

		/* Record long_rd_id for multicast tests (ring of last N) */
		if (mock_dlc_data_tx_record_count < MOCK_DLC_DATA_TX_RECORD_MAX) {
			mock_dlc_data_tx_long_rd_ids[mock_dlc_data_tx_record_count++] =
				params->long_rd_id;
		} else {
			memmove(&mock_dlc_data_tx_long_rd_ids[0], &mock_dlc_data_tx_long_rd_ids[1],
				(MOCK_DLC_DATA_TX_RECORD_MAX - 1) *
					sizeof(mock_dlc_data_tx_long_rd_ids[0]));
			mock_dlc_data_tx_long_rd_ids[MOCK_DLC_DATA_TX_RECORD_MAX - 1] =
				params->long_rd_id;
		}

		/* Simulate async TX completion callback */
		if (mock_op_callbacks.dlc_data_tx) {
			struct nrf_modem_dect_dlc_data_tx_cb_params tx_cb_params = {
				.status = NRF_MODEM_DECT_MAC_STATUS_OK,
				.long_rd_id = params->long_rd_id,
				.flow_id = params->flow_id,
				.transaction_id = params->transaction_id};
			simulate_async_callback((void (*)(void *))mock_op_callbacks.dlc_data_tx,
						&tx_cb_params);
		}
	}

	return 0;
}

int nrf_modem_dect_dlc_data_discard(struct nrf_modem_dect_dlc_data_discard_params *params)
{
	return 0;
}

/* NRF Modem DECT Control functions needed by MAC driver */
int nrf_modem_dect_control_configure(struct nrf_modem_dect_control_configure_params *params)
{
	mock_nrf_modem_dect_control_configure_call_count++;

	/* Simulate asynchronous callback (use inject status for one-shot failure). */
	if (mock_op_callbacks.control_configure) {
		struct nrf_modem_dect_mac_control_configure_cb_params cb_params = {
			.status = mock_inject_configure_status,
		};
		enum nrf_modem_dect_mac_err save = mock_inject_configure_status;

		mock_inject_configure_status = NRF_MODEM_DECT_MAC_STATUS_OK;
		simulate_async_callback((void (*)(void *))mock_op_callbacks.control_configure,
					&cb_params);
		(void)save;
	}
	return 0;
}

int nrf_modem_dect_control_functional_mode_set(enum nrf_modem_dect_control_functional_mode mode)
{
	mock_nrf_modem_dect_control_functional_mode_set_call_count++;

	LOG_DBG("MOCK: nrf_modem_dect_control_functional_mode_set called with mode=%d", mode);

	/* Simulate successful functional mode change (or inject failure for tests). */
	if (mode == NRF_MODEM_DECT_CONTROL_FUNCTIONAL_MODE_ACTIVATE) {
		mock_dect_activated = true; /* Update activation state */
		if (mock_op_callbacks.control_functional_mode) {
			struct nrf_modem_dect_mac_control_functional_mode_cb_params cb_params = {
				.status = mock_inject_functional_mode_status,
			};
			enum nrf_modem_dect_mac_err save = mock_inject_functional_mode_status;

			mock_inject_functional_mode_status = NRF_MODEM_DECT_MAC_STATUS_OK;
			simulate_async_callback(
				(void (*)(void *))mock_op_callbacks.control_functional_mode,
				&cb_params);
			(void)save;
		}
		return 0;
	} else if (mode == NRF_MODEM_DECT_CONTROL_FUNCTIONAL_MODE_DEACTIVATE) {
		mock_dect_activated = false; /* Update activation state */
		/* Simulate asynchronous callback after successful deactivation */
		if (mock_op_callbacks.control_functional_mode) {
			struct nrf_modem_dect_mac_control_functional_mode_cb_params cb_params = {
				.status = NRF_MODEM_DECT_MAC_STATUS_OK};

			LOG_DBG("Calling control_functional_mode callback for DEACTIVATE");
			simulate_async_callback(
				(void (*)(void *))mock_op_callbacks.control_functional_mode,
				&cb_params);
		}
		return 0;
	}
	return 0;
}

/* CMock-style expectation functions */
void mock_nrf_modem_dect_mac_callback_set_ExpectAndReturn(int return_value)
{
	expected_callback_set_return = return_value;
}

void mock_nrf_modem_dect_mac_network_scan_ExpectAndReturn(int return_value)
{
	expected_network_scan_return = return_value;
}

void mock_nrf_modem_dect_mac_cluster_beacon_receive_ExpectAndReturn(int return_value)
{
	expected_cluster_beacon_receive_return = return_value;
}

void mock_nrf_modem_dect_mac_reset(void)
{
	/* Reset call counters and state, but preserve registered callbacks */
	mock_nrf_modem_dect_mac_callback_set_call_count = 0;
	mock_nrf_modem_dect_mac_network_scan_call_count = 0;
	mock_nrf_modem_dect_mac_cluster_beacon_receive_call_count = 0;
	mock_nrf_modem_dect_mac_association_call_count = 0;
	mock_nrf_modem_dect_mac_association_release_call_count = 0;
	mock_nrf_modem_dect_mac_neighbor_list_call_count = 0;
	mock_nrf_modem_dect_mac_neighbor_info_call_count = 0;
	mock_nrf_modem_dect_control_systemmode_set_call_count = 0;
	mock_nrf_modem_dect_control_configure_call_count = 0;
	mock_nrf_modem_dect_control_functional_mode_set_call_count = 0;
	mock_nrf_modem_dect_mac_rssi_scan_call_count = 0;
	mock_nrf_modem_dect_mac_rssi_scan_stop_call_count = 0;
	mock_nrf_modem_dect_mac_cluster_configure_call_count = 0;
	mock_nrf_modem_dect_mac_cluster_info_call_count = 0;
	mock_nrf_modem_dect_dlc_data_tx_call_count = 0;
	expected_callback_set_return = 0;
	expected_network_scan_return = 0;
	expected_cluster_beacon_receive_return = 0;
	mock_modem_initialized = false;
	mock_dect_activated = false;
	mock_last_configured_cluster_channel = 0;
	mock_cluster_creation_band1 = false;
	mock_auto_invoke_association_release_cb = false;
	memset(mock_last_configured_ipv6_prefix, 0, MOCK_IPV6_PREFIX_LEN);
	mock_last_configured_ipv6_prefix_type = NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_NONE;
	mock_inject_configure_status = NRF_MODEM_DECT_MAC_STATUS_OK;
	mock_inject_functional_mode_status = NRF_MODEM_DECT_MAC_STATUS_OK;
	mock_inject_association_status = NRF_MODEM_DECT_MAC_STATUS_OK;
	mock_inject_association_rejected = false;
	mock_inject_association_reject_cause = 0;
	mock_inject_association_reject_time = 0;
	mock_last_dlc_data_tx_len = 0;
	mock_last_dlc_data_tx_long_rd_id = 0;
	mock_dlc_data_tx_record_count = 0;
	memset(mock_last_dlc_data_tx_data, 0, sizeof(mock_last_dlc_data_tx_data));
	memset(mock_dlc_data_tx_long_rd_ids, 0, sizeof(mock_dlc_data_tx_long_rd_ids));

	/* Note: We do NOT reset mock_op_callbacks and mock_ntf_callbacks here
	 * because they represent the real DECT driver's callback registration
	 * which should persist across test cases. The driver registers these
	 * once during initialization and they remain valid.
	 */
}

/* Simulate cluster beacon reception for testing */
void mock_simulate_cluster_beacon_received(void)
{
	if (mock_ntf_callbacks.cluster_beacon_ntf) {
		struct nrf_modem_dect_mac_cluster_beacon_ntf_cb_params beacon_params = {
			.channel = 1722, /* Band 1 channel */
			.transmitter_short_rd_id = 0x1234,
			.transmitter_long_rd_id = 0x12345678,
			.network_id = 1,
			.number_of_ies = 0,
			.ies = NULL};
		mock_ntf_callbacks.cluster_beacon_ntf(&beacon_params);
	}
}

/*
 * Note: The MAC driver registers its initialization callback using:
 * NRF_MODEM_LIB_ON_INIT(dect_nrf91_ctrl_api_init_hook, dect_nrf91_ctrl_mdm_on_modem_lib_init, NULL)
 *
 * This callback is automatically executed during system initialization, and it calls
 * nrf_modem_dect_mac_callback_set() which we mock below.
 *
 * For testing purposes, we don't need to explicitly simulate nrf_modem_lib_init() -
 * the MAC driver's NRF_MODEM_LIB_ON_INIT callback will execute during system startup.
 */

/* Mock control functions needed for initialization */

/* Mock control functions for additional initialization functions */

int nrf_modem_dect_control_systemmode_set(enum nrf_modem_dect_control_systemmode mode)
{
	mock_nrf_modem_dect_control_systemmode_set_call_count++;

	/* Simulate success for MAC mode */
	if (mode == NRF_MODEM_DECT_MODE_MAC) {
		/* First, simulate capability_ntf callback before op callback */
		if (mock_ntf_callbacks.capability_ntf) {
			struct nrf_modem_dect_mac_capability_ntf_cb_params capability_params = {
				.max_mcs = 4,
				.num_band_info_elems = 1,
				.band_info_elems = {
					[0] = {.band_group_index =
						       NRF_MODEM_DECT_MAC_PHY_BAND_GROUP_IDX0,
					       .band = NRF_MODEM_DECT_MAC_PHY_BAND1,
					       .power_class = 3,
					       .min_carrier = 1657,
					       .max_carrier = 1677}}};
			simulate_async_callback((void (*)(void *))mock_ntf_callbacks.capability_ntf,
						&capability_params);
		}

		/* Then, simulate asynchronous op callback after successful operation */
		if (mock_op_callbacks.control_systemmode) {
			struct nrf_modem_dect_mac_control_systemmode_cb_params cb_params = {
				.status = 0 /* Success */
			};
			simulate_async_callback(
				(void (*)(void *))mock_op_callbacks.control_systemmode, &cb_params);
		}
		return 0;
	}
	return -1;
}

int mock_nrf_modem_dect_control_systemmode_set_ExpectAndReturn(int return_value)
{
	/* Mock expectation function - for now just return the expected value */
	return return_value;
}

/* Note: nrf_modem_dect_mac_configure does not exist in the API - removed */

/* Simulate NRF_MODEM_LIB_ON_INIT callback system - now using direct function call */
void mock_simulate_nrf_modem_lib_init(void)
{
	/* Set mock modem as initialized */
	mock_modem_initialized = true;

	/* Note: Tests can now directly call dect_nrf91_ctrl_mdm_on_modem_lib_init()
	 * since it's been made non-static for testing.
	 */
}
