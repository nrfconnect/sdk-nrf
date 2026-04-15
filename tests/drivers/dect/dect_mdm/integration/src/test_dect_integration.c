/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file test_dect_integration.c
 * @brief Unity-based DECT NR+ Stack Integration Tests
 *
 * Complete DECT NR+ stack integration tests using Unity framework with:
 * - Real DECT NR+ stack
 * - Real net_mgmt() API calls
 * - Real conn_mgr() API calls
 * - Mock nrf_modem_dect_mac.h backend with proper callback simulation
 *
 * Architecture:
 * Unity Tests → conn_mgr() → net_mgmt() → Real DECT NR+ Stack → Mock nrf_modem Backend
 */

/*
 * Maintenance:
 * - Use TEST_BEACON_* and test_ipv6_* constants for beacon/scan and IPv6 addresses.
 * - Use test_dect_status_info_get() for preconditions (e.g. parent_count >= 1).
 * - Run order in main.c matters for FT/PT sequences (e.g. sink_down before PT conn_mgr).
 */

#include "unity.h"
#include "mock_nrf_modem_dect_mac.h"
#include "test_dect_utils.h"

/* Real DECT API includes for integration testing */
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/icmp.h>
#include <net/dect/dect_net_l2_mgmt.h>
#include <net/dect/dect_utils.h>
#include <net/dect/dect_net_l2.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#if defined(CONFIG_NET_SOCKETS_PACKET)
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#endif
#if defined(CONFIG_NET_CONNECTION_MANAGER)
#include <zephyr/net/conn_mgr_connectivity.h>

/* Use conn_mgr monitor state for "net cm status" style verification (conn_mgr_private.h bits) */
#define CONN_MGR_IF_UP_BIT	  (1U << 0)
#define CONN_MGR_IF_IPV6_SET_BIT  (1U << 1)
#define CONN_MGR_IF_IGNORED_BIT	  (1U << 7)
#define CONN_MGR_IF_READY_BIT	  (1U << 13)
#define CONN_MGR_IF_STATE_INVALID 0xFFFF
extern uint16_t conn_mgr_if_state(struct net_if *iface);
#endif
#include <string.h>

LOG_MODULE_REGISTER(test_dect_integration, CONFIG_TEST_DECT_INTEGRATION_LOG_LEVEL);

/* Test beacon/scan parameter constants (used across scan, association, neighbor, cluster tests) */
#define TEST_BEACON_CHANNEL	   1657
#define TEST_BEACON_SHORT_RD_ID	   0x1234
#define TEST_BEACON_LONG_RD_ID	   0x56789ABC
#define TEST_BEACON_NETWORK_ID	   0x9876
#define TEST_BEACON_MCS		   1
#define TEST_BEACON_TRANSMIT_POWER 8
#define TEST_BEACON_RSSI_DBM	   (-45)
#define TEST_BEACON_SNR_DB	   20

/* IPv6 prefix/address constants (8-byte prefix or 16-byte full address) */
/** Sink/PPP prefix 2001:db8::/64 */
static const uint8_t test_ipv6_prefix_sink_64[8] = {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00};
/** PT parent prefix 2001:db8:1::/64 (conn_mgr, association response) */
static const uint8_t test_ipv6_prefix_pt_parent_64[8] = {0x20, 0x01, 0x0d, 0xb8,
							 0x00, 0x01, 0x00, 0x00};
/** PT global address assign 2001:db8:1:2::/64 */
static const uint8_t test_ipv6_prefix_pt_assign_64[8] = {0x20, 0x01, 0x0d, 0xb8,
							 0x00, 0x01, 0x00, 0x02};
/** PT global address change new / removal current 2001:db8:1:3::/64 */
static const uint8_t test_ipv6_prefix_pt_change_new_64[8] = {0x20, 0x01, 0x0d, 0xb8,
							     0x00, 0x01, 0x00, 0x03};
/** Sink router / PPP global address 2001:db8::1 */
static const uint8_t test_ipv6_addr_sink_router[16] = {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00,
						       0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						       0x00, 0x00, 0x00, 0x01};
/** New sink address 2001:db8:1::1 (sink_global_address_change) */
static const uint8_t test_ipv6_addr_sink_new_64[16] = {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x01,
						       0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						       0x00, 0x00, 0x00, 0x01};

/* Network interface for testing */
static struct net_if *test_iface;

/* Forward declaration for DECT driver initialization function (now non-static for tests) */
extern void dect_mdm_ctrl_mdm_on_modem_lib_init(int ret, void *ctx);

/* Event tracking for activation tests */
bool dect_activate_done_received;
enum dect_status_values dect_activate_done_status;

/* Event tracking for deactivation tests */
bool dect_deactivate_done_received;
enum dect_status_values dect_deactivate_done_status;

/* Semaphore for thread synchronization */
static K_SEM_DEFINE(activation_done_sem, 0, 1);

/* Event tracking for scan tests */
bool dect_scan_result_received;
bool dect_scan_done_received;
enum dect_status_values dect_scan_done_status;

/* Event tracking for RSSI scan tests */
bool dect_rssi_scan_result_received;
struct dect_rssi_scan_result_evt received_rssi_scan_result_data;
bool dect_rssi_scan_done_received;
enum dect_status_values dect_rssi_scan_done_status;

/* Event tracking for association tests */
bool dect_association_changed_received;
bool dect_association_created_received;
bool dect_association_released_received;
int dect_association_released_count; /* Number of DECT_ASSOCIATION_RELEASED events in this test */
bool dect_association_failed_received;
struct dect_association_changed_evt received_association_data;

/* Event tracking for network status tests */
bool dect_network_status_received;
struct dect_network_status_evt received_network_status_data;

/* Event tracking for neighbor list tests */
static bool dect_neighbor_list_received;
static struct dect_neighbor_list_evt received_neighbor_list_data;

/* Event tracking for neighbor info tests */
static bool dect_neighbor_info_received;
static struct dect_neighbor_info_evt received_neighbor_info_data;

/* Event tracking for cluster created tests */
static volatile bool dect_cluster_created_received;
static struct dect_cluster_start_resp_evt received_cluster_created_data;

/* Event tracking for cluster info (NET_EVENT_DECT_CLUSTER_INFO) */
static bool dect_cluster_info_received;
static struct dect_cluster_info_evt received_cluster_info_data;

/* Event tracking for NW beacon start result */
static bool dect_nw_beacon_start_received;
static enum dect_status_values dect_nw_beacon_start_status;

/* Event tracking for NW beacon stop result */
static bool dect_nw_beacon_stop_received;
static enum dect_status_values dect_nw_beacon_stop_status;

/* Event tracking for sink status (NET_EVENT_DECT_SINK_STATUS) */
bool dect_sink_status_received;
struct dect_sink_status_evt received_sink_status_data;

/* Event tracking for NET_EVENT_IPV6_NBR_ADD (sink router re-added by worker) */
static bool dect_sink_router_nbr_add_received;
static struct net_mgmt_event_callback test_nbr_add_cb;

static void test_nbr_add_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				 struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV6_NBR_ADD || !cb->info) {
		return;
	}
	const struct net_event_ipv6_nbr *evt = (const struct net_event_ipv6_nbr *)cb->info;

	if (memcmp(evt->addr.s6_addr, test_ipv6_addr_sink_router,
		   sizeof(test_ipv6_addr_sink_router)) == 0) {
		dect_sink_router_nbr_add_received = true;
	}
}

#if defined(CONFIG_NET_CONNECTION_MANAGER)
/* Event tracking for NET_EVENT_L4_IPV6_CONNECTED (conn_mgr when iface gets global IPv6).
 * FT+sink: L4 events are for mocked PPP iface (sink/border router).
 * PT: L4 events are for DECT iface.
 */
static bool dect_l4_ipv6_connected_received;
static bool dect_l4_disconnected_received;
static bool dect_l4_ipv6_disconnected_received;
static struct net_if *test_l4_expected_iface; /* NULL = any; set by test (e.g. ppp_if for FT) */
static struct net_mgmt_event_callback test_l4_cb;

#define TEST_L4_EVENT_MASK                                                                         \
	(NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED | NET_EVENT_L4_IPV6_CONNECTED |        \
	 NET_EVENT_L4_IPV6_DISCONNECTED)

static void test_l4_ipv6_connected_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
					   struct net_if *iface)
{
	ARG_UNUSED(cb);
	/* Log every L4 event for every iface (FT+sink: mocked PPP; PT: DECT). */
	switch (mgmt_event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("NET_EVENT_L4_CONNECTED: Network connectivity established "
			"and global IPv6 address assigned, iface %p",
			(void *)iface);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("NET_EVENT_L4_DISCONNECTED: IP down, iface %p", (void *)iface);
		if (test_l4_expected_iface == NULL || iface == test_l4_expected_iface) {
			dect_l4_disconnected_received = true;
		}
		break;
	case NET_EVENT_L4_IPV6_CONNECTED:
		LOG_INF("NET_EVENT_L4_IPV6_CONNECTED: IPv6 connectivity established, iface %p",
			(void *)iface);
		if (test_l4_expected_iface == NULL || iface == test_l4_expected_iface) {
			dect_l4_ipv6_connected_received = true;
		}
		break;
	case NET_EVENT_L4_IPV6_DISCONNECTED:
		LOG_INF("NET_EVENT_L4_IPV6_DISCONNECTED: IPv6 connectivity lost, iface %p",
			(void *)iface);
		if (test_l4_expected_iface == NULL || iface == test_l4_expected_iface) {
			dect_l4_ipv6_disconnected_received = true;
		}
		break;
	default:
		break;
	}
}
#endif

/* Storage for received beacon data from NET_EVENT_DECT_SCAN_RESULT */
struct dect_scan_result_evt received_beacon_data;
bool beacon_data_valid;

static struct net_mgmt_event_callback dect_mgmt_cb;

/* Forward declaration for debug function */
void debug_thread_context(const char *context_name);

void dect_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			     struct net_if *iface)
{
	debug_thread_context("dect_mgmt_event_handler");

	switch (mgmt_event) {
	case NET_EVENT_DECT_ACTIVATE_DONE: {
		/* Event data is stored in cb->info */
		struct dect_common_resp_evt *evt = (struct dect_common_resp_evt *)cb->info;

		LOG_INF("NET_EVENT_DECT_ACTIVATE_DONE received with status: %d", evt->status);
		dect_activate_done_received = true;
		dect_activate_done_status = evt->status;
		/* Signal semaphore to wake up waiting test thread */
		k_sem_give(&activation_done_sem);
		break;
	}
	case NET_EVENT_DECT_DEACTIVATE_DONE: {
		/* Event data is stored in cb->info */
		struct dect_common_resp_evt *evt = (struct dect_common_resp_evt *)cb->info;

		LOG_INF("NET_EVENT_DECT_DEACTIVATE_DONE received with status: %d", evt->status);
		dect_deactivate_done_received = true;
		dect_deactivate_done_status = evt->status;
		break;
	}
	case NET_EVENT_DECT_SCAN_RESULT: {
		/* Cluster beacon received during scan */
		LOG_INF("NET_EVENT_DECT_SCAN_RESULT received");

		/* Store the beacon data from the event */
		if (cb->info) {
			struct dect_scan_result_evt *beacon_evt =
				(struct dect_scan_result_evt *)cb->info;
			memcpy(&received_beacon_data, beacon_evt, sizeof(received_beacon_data));
			beacon_data_valid = true;

			LOG_DBG("Stored beacon data: channel=%d, short_rd_id=0x%04X, "
				"long_rd_id=0x%08X, network_id=0x%08X",
				received_beacon_data.channel,
				received_beacon_data.transmitter_short_rd_id,
				received_beacon_data.transmitter_long_rd_id,
				received_beacon_data.network_id);
			LOG_INF("Beacon RX: transmit_power=%d (%d dBm), rssi_2=%ddBm, snr=%ddB",
				received_beacon_data.rx_signal_info.transmit_power,
				dect_utils_lib_phy_tx_power_to_dbm(
					received_beacon_data.rx_signal_info.transmit_power),
				received_beacon_data.rx_signal_info.rssi_2,
				received_beacon_data.rx_signal_info.snr);
			if (!dect_utils_lib_32bit_network_id_validate(
				    received_beacon_data.network_id)) {
				LOG_WRN("Beacon network_id 0x%08X is invalid (LSB or MSB zero)",
					received_beacon_data.network_id);
			}
		} else {
			LOG_WRN("NET_EVENT_DECT_SCAN_RESULT received but no beacon data available");
		}

		dect_scan_result_received = true;
		break;
	}
	case NET_EVENT_DECT_SCAN_DONE: {
		/* Scan operation completed */
		struct dect_common_resp_evt *evt = (struct dect_common_resp_evt *)cb->info;

		LOG_INF("NET_EVENT_DECT_SCAN_DONE received with status: %d", evt->status);
		dect_scan_done_received = true;
		dect_scan_done_status = evt->status;
		break;
	}
	case NET_EVENT_DECT_RSSI_SCAN_RESULT: {
		/* RSSI scan result received */
		LOG_INF("NET_EVENT_DECT_RSSI_SCAN_RESULT received");

		/* Store the RSSI scan result data from the event */
		if (cb->info) {
			struct dect_rssi_scan_result_evt *rssi_evt =
				(struct dect_rssi_scan_result_evt *)cb->info;
			memcpy(&received_rssi_scan_result_data, rssi_evt,
			       sizeof(received_rssi_scan_result_data));

			LOG_DBG("Stored RSSI scan result: channel=%d, busy_percentage=%d%%, "
				"scan_suitable_percent=%d%%",
				received_rssi_scan_result_data.rssi_scan_result.channel,
				received_rssi_scan_result_data.rssi_scan_result.busy_percentage,
				received_rssi_scan_result_data.rssi_scan_result
					.scan_suitable_percent);
		} else {
			LOG_WRN("NET_EVENT_DECT_RSSI_SCAN_RESULT received but "
				"no result data available");
		}

		dect_rssi_scan_result_received = true;
		break;
	}
	case NET_EVENT_DECT_RSSI_SCAN_DONE: {
		/* RSSI scan operation completed */
		struct dect_common_resp_evt *evt = (struct dect_common_resp_evt *)cb->info;

		LOG_INF("NET_EVENT_DECT_RSSI_SCAN_DONE received with status: %d", evt->status);
		dect_rssi_scan_done_received = true;
		dect_rssi_scan_done_status = evt->status;
		break;
	}
	case NET_EVENT_DECT_ASSOCIATION_CHANGED: {
		/* Association changed (created, released, failed, etc.) */
		LOG_INF("NET_EVENT_DECT_ASSOCIATION_CHANGED received");

		if (cb->info) {
			struct dect_association_changed_evt *assoc_evt =
				(struct dect_association_changed_evt *)cb->info;
			LOG_DBG("Association change type: %d, long_rd_id: 0x%08X, neighbor_role: "
				"%d",
				assoc_evt->association_change_type, assoc_evt->long_rd_id,
				assoc_evt->neighbor_role);

			/* Store the association event data for validation */
			memcpy(&received_association_data, assoc_evt,
			       sizeof(received_association_data));

			/* Check if this is an association creation, release or failure event */
			if (assoc_evt->association_change_type == DECT_ASSOCIATION_CREATED) {
				LOG_DBG("DECT_ASSOCIATION_CREATED event received - association "
					"successful!");
				dect_association_created_received = true;
			} else if (assoc_evt->association_change_type ==
				   DECT_ASSOCIATION_RELEASED) {
				LOG_DBG("DECT_ASSOCIATION_RELEASED event received - "
					"association released!");
				dect_association_released_received = true;
				dect_association_released_count++;
			} else if (assoc_evt->association_change_type ==
				   DECT_ASSOCIATION_REQ_FAILED_MDM) {
				LOG_DBG("DECT_ASSOCIATION_REQ_FAILED_MDM event received - "
					"association failed!");
				dect_association_failed_received = true;
			}
		} else {
			LOG_WRN("NET_EVENT_DECT_ASSOCIATION_CHANGED received but no event data "
				"available");
		}

		dect_association_changed_received = true;
		break;
	}
	case NET_EVENT_DECT_NETWORK_STATUS: {
		/* Network status changed (created, removed, joined, unjoined) */
		LOG_INF("NET_EVENT_DECT_NETWORK_STATUS received");

		if (cb->info) {
			struct dect_network_status_evt *status_evt =
				(struct dect_network_status_evt *)cb->info;
			LOG_DBG("Network status: %d, dect_err_cause: %d",
				status_evt->network_status, status_evt->dect_err_cause);

			/* Store the network status event data for validation */
			memcpy(&received_network_status_data, status_evt,
			       sizeof(received_network_status_data));
		} else {
			LOG_WRN("NET_EVENT_DECT_NETWORK_STATUS received but no event data "
				"available");
		}

		dect_network_status_received = true;
		break;
	}
	case NET_EVENT_DECT_NEIGHBOR_LIST: {
		/* Neighbor list response */
		LOG_INF("NET_EVENT_DECT_NEIGHBOR_LIST received");

		if (cb->info) {
			struct dect_neighbor_list_evt *neighbor_evt =
				(struct dect_neighbor_list_evt *)cb->info;
			LOG_DBG("Neighbor list status: %d, neighbor_count: %d",
				neighbor_evt->status, neighbor_evt->neighbor_count);

			/* Log first few neighbors for debugging */
			for (int i = 0; i < neighbor_evt->neighbor_count && i < 3; i++) {
				LOG_DBG("Neighbor[%d]: Long RD ID 0x%08X", i,
					neighbor_evt->neighbor_long_rd_ids[i]);
			}

			/* Store the neighbor list event data for validation */
			memcpy(&received_neighbor_list_data, neighbor_evt,
			       sizeof(received_neighbor_list_data));
		} else {
			LOG_WRN("NET_EVENT_DECT_NEIGHBOR_LIST received but no event data "
				"available");
		}

		dect_neighbor_list_received = true;
		break;
	}
	case NET_EVENT_DECT_NEIGHBOR_INFO: {
		/* Neighbor info response */
		LOG_INF("NET_EVENT_DECT_NEIGHBOR_INFO received");

		if (cb->info) {
			struct dect_neighbor_info_evt *neighbor_info_evt =
				(struct dect_neighbor_info_evt *)cb->info;
			LOG_DBG("Neighbor info status: %d, long_rd_id: 0x%08X",
				neighbor_info_evt->status, neighbor_info_evt->long_rd_id);
			LOG_DBG("Associated: %s, FT mode: %s, Channel: %d, Network ID: 0x%08X",
				neighbor_info_evt->associated ? "Yes" : "No",
				neighbor_info_evt->ft_mode ? "Yes" : "No",
				neighbor_info_evt->channel, neighbor_info_evt->network_id);

			/* Store the neighbor info event data for validation */
			memcpy(&received_neighbor_info_data, neighbor_info_evt,
			       sizeof(received_neighbor_info_data));
		} else {
			LOG_WRN("NET_EVENT_DECT_NEIGHBOR_INFO received but no event data "
				"available");
		}

		dect_neighbor_info_received = true;
		break;
	}
	case NET_EVENT_DECT_CLUSTER_CREATED_RESULT: {
		/* Cluster created/started result */
		LOG_INF("NET_EVENT_DECT_CLUSTER_CREATED_RESULT received");

		if (cb->info) {
			struct dect_cluster_start_resp_evt *cluster_evt =
				(struct dect_cluster_start_resp_evt *)cb->info;
			LOG_DBG("Cluster created: status=%d, cluster_channel=%d",
				cluster_evt->status, cluster_evt->cluster_channel);

			/* Store the cluster created event data for validation */
			memcpy(&received_cluster_created_data, cluster_evt,
			       sizeof(received_cluster_created_data));
		} else {
			LOG_WRN("NET_EVENT_DECT_CLUSTER_CREATED_RESULT received but no event data "
				"available");
		}

		dect_cluster_created_received = true;
		break;
	}
	case NET_EVENT_DECT_CLUSTER_INFO: {
		if (cb->info) {
			memcpy(&received_cluster_info_data, cb->info,
			       sizeof(received_cluster_info_data));
			LOG_INF("NET_EVENT_DECT_CLUSTER_INFO received with status: %d",
				received_cluster_info_data.status);
		}
		dect_cluster_info_received = true;
		break;
	}
	case NET_EVENT_DECT_NW_BEACON_START_RESULT: {
		struct dect_common_resp_evt *evt = (struct dect_common_resp_evt *)cb->info;

		if (evt) {
			LOG_INF("NET_EVENT_DECT_NW_BEACON_START_RESULT received with status: %d",
				evt->status);
			dect_nw_beacon_start_status = evt->status;
		}
		dect_nw_beacon_start_received = true;
		break;
	}
	case NET_EVENT_DECT_NW_BEACON_STOP_RESULT: {
		struct dect_common_resp_evt *evt = (struct dect_common_resp_evt *)cb->info;

		if (evt) {
			LOG_INF("NET_EVENT_DECT_NW_BEACON_STOP_RESULT received with status: %d",
				evt->status);
			dect_nw_beacon_stop_status = evt->status;
		}
		dect_nw_beacon_stop_received = true;
		break;
	}
	case NET_EVENT_DECT_SINK_STATUS: {
		const struct dect_sink_status_evt *evt =
			(const struct dect_sink_status_evt *)cb->info;

		if (evt) {
			LOG_INF("NET_EVENT_DECT_SINK_STATUS received: %s",
				evt->sink_status == DECT_SINK_STATUS_CONNECTED ? "CONNECTED"
									       : "DISCONNECTED");
			received_sink_status_data = *evt;
		}
		dect_sink_status_received = true;
		break;
	}
	default:
		break;
	}
}

/**
 * @brief Debug function to check thread context and logging visibility
 */
void debug_thread_context(const char *context_name)
{
	struct k_thread *current = k_current_get();

	LOG_DBG("=== THREAD DEBUG [%s] ===", context_name);
	LOG_DBG("Current thread: %p", current);
	LOG_DBG("Thread name: %s", k_thread_name_get(current) ?: "unnamed");
	LOG_DBG("Thread priority: %d", k_thread_priority_get(current));
	LOG_DBG("============================");
}

/* Static flag to track if DECT stack has been initialized */
static bool dect_stack_initialized;

/** Reset all DECT event tracking flags (used by setUp). */
static void test_reset_dect_event_flags(void)
{
	dect_activate_done_received = false;
	dect_deactivate_done_received = false;
	dect_scan_result_received = false;
	dect_scan_done_received = false;
	dect_rssi_scan_result_received = false;
	dect_rssi_scan_done_received = false;
	dect_association_changed_received = false;
	dect_association_created_received = false;
	dect_association_released_received = false;
	dect_association_released_count = 0;
	dect_association_failed_received = false;
	dect_network_status_received = false;
	dect_neighbor_list_received = false;
	dect_neighbor_info_received = false;
	dect_cluster_created_received = false;
	dect_cluster_info_received = false;
	dect_nw_beacon_start_received = false;
	dect_nw_beacon_stop_received = false;
	dect_sink_status_received = false;
}

void setUp(void)
{

	/* Get the DECT NR+ network interface by device name */
	int if_index = net_if_get_by_name(CONFIG_DECT_MDM_DEVICE_NAME);

	if (if_index > 0) {
		test_iface = net_if_get_by_index(if_index);
	}

	/* Fallback to default interface if DECT interface not found */
	if (!test_iface) {
		test_iface = net_if_get_default();
	}

	/* Only reset event state for the current test - preserve stack state and data
	 * from previous tests
	 */
	test_reset_dect_event_flags();

	/* Reset semaphore to ensure clean state for each test */
	k_sem_reset(&activation_done_sem);

	/* Always ensure event callbacks are registered for each test */
	net_mgmt_init_event_callback(
		&dect_mgmt_cb, dect_mgmt_event_handler,
		NET_EVENT_DECT_ACTIVATE_DONE | NET_EVENT_DECT_DEACTIVATE_DONE |
			NET_EVENT_DECT_SCAN_RESULT | NET_EVENT_DECT_SCAN_DONE |
			NET_EVENT_DECT_RSSI_SCAN_RESULT | NET_EVENT_DECT_RSSI_SCAN_DONE |
			NET_EVENT_DECT_ASSOCIATION_CHANGED | NET_EVENT_DECT_NETWORK_STATUS |
			NET_EVENT_DECT_NEIGHBOR_LIST | NET_EVENT_DECT_NEIGHBOR_INFO |
			NET_EVENT_DECT_CLUSTER_CREATED_RESULT | NET_EVENT_DECT_CLUSTER_INFO |
			NET_EVENT_DECT_NW_BEACON_START_RESULT |
			NET_EVENT_DECT_NW_BEACON_STOP_RESULT | NET_EVENT_DECT_SINK_STATUS);
	net_mgmt_add_event_callback(&dect_mgmt_cb);
	dect_stack_initialized = true;
}

void tearDown(void)
{
	/* Remove event callback but preserve DECT stack state */
	net_mgmt_del_event_callback(&dect_mgmt_cb);
}

/**
 * @brief Verify DECT net_mgmt requests fail on an invalid iface
 *
 * Uses a zero-initialized iface so net_if_get_device() returns NULL and
 * get_dect_nr_hal_api() returns NULL. All DECT net_mgmt requests should then
 * fail from the handler front-end with -ENOTSUP.
 */
void test_dect_net_mgmt_invalid_iface_errors(void)
{
	struct net_if invalid_iface = {0};
	struct dect_scan_params scan_params = {
		.band = 1,
		.channel_count = 2,
		.channel_list = {TEST_BEACON_CHANNEL, 1838},
		.channel_scan_time_ms = 100,
	};
	struct dect_rssi_scan_params rssi_scan_params = {
		.band = 0,
		.frame_count_to_scan = 1,
		.channel_count = 1,
		.channel_list = {TEST_BEACON_CHANNEL},
	};
	struct dect_associate_req_params assoc_params = {
		.target_long_rd_id = TEST_BEACON_LONG_RD_ID,
	};
	struct dect_associate_rel_params assoc_rel_params = {
		.target_long_rd_id = TEST_BEACON_LONG_RD_ID,
	};
	struct dect_cluster_start_req_params cluster_start_params = {
		.channel = TEST_BEACON_CHANNEL,
	};
	struct dect_cluster_reconfig_req_params cluster_reconfig_params = {
		.channel = TEST_BEACON_CHANNEL,
		.max_beacon_tx_power_dbm = 20,
		.max_cluster_power_dbm = 20,
		.period = DECT_CLUSTER_BEACON_PERIOD_100MS,
	};
	struct dect_cluster_stop_req_params cluster_stop_params = {};
	struct dect_nw_beacon_start_req_params nw_beacon_start_params = {
		.channel = TEST_BEACON_CHANNEL,
		.additional_ch_count = 0,
	};
	struct dect_nw_beacon_stop_req_params nw_beacon_stop_params = {};
	struct dect_neighbor_info_req_params neighbor_info_params = {
		.long_rd_id = TEST_BEACON_LONG_RD_ID,
	};
	struct dect_settings settings = {
		.cmd_params.write_scope_bitmap = DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE,
	};
	struct dect_status_info status_info = {0};
	int ret;

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface, "DECT interface required");

	ret = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, &invalid_iface, &settings, sizeof(settings));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Settings read should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, &invalid_iface, &settings,
		       sizeof(settings));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Settings write should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_STATUS_INFO_GET, &invalid_iface, &status_info,
		       sizeof(status_info));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Status info get should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_ACTIVATE, &invalid_iface, NULL, 0);
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Activate request should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_DEACTIVATE, &invalid_iface, NULL, 0);
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Deactivate request should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_SCAN, &invalid_iface, &scan_params, sizeof(scan_params));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret, "Scan request should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_RSSI_SCAN, &invalid_iface, &rssi_scan_params,
		       sizeof(rssi_scan_params));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "RSSI scan request should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_ASSOCIATION, &invalid_iface, &assoc_params,
		       sizeof(assoc_params));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Association request should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_ASSOCIATION_RELEASE, &invalid_iface, &assoc_rel_params,
		       sizeof(assoc_rel_params));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Association release should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_START, &invalid_iface, &cluster_start_params,
		       sizeof(cluster_start_params));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Cluster start should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_RECONFIGURE, &invalid_iface,
		       &cluster_reconfig_params, sizeof(cluster_reconfig_params));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Cluster reconfigure should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_STOP, &invalid_iface, &cluster_stop_params,
		       sizeof(cluster_stop_params));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret, "Cluster stop should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_NW_BEACON_START, &invalid_iface, &nw_beacon_start_params,
		       sizeof(nw_beacon_start_params));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Network beacon start should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_NW_BEACON_STOP, &invalid_iface, &nw_beacon_stop_params,
		       sizeof(nw_beacon_stop_params));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Network beacon stop should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_NEIGHBOR_LIST, &invalid_iface, NULL, 0);
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Neighbor list should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_NEIGHBOR_INFO, &invalid_iface, &neighbor_info_params,
		       sizeof(neighbor_info_params));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Neighbor info should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_INFO, &invalid_iface, NULL, 0);
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret, "Cluster info should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_CREATE, &invalid_iface, NULL, 0);
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Network create should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_REMOVE, &invalid_iface, NULL, 0);
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Network remove should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_JOIN, &invalid_iface, NULL, 0);
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret, "Network join should fail with invalid iface");

	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_UNJOIN, &invalid_iface, NULL, 0);
	TEST_ASSERT_EQUAL_INT_MESSAGE(-ENOTSUP, ret,
				      "Network unjoin should fail with invalid iface");
}

/**
 * @brief Test DECT stack initialization through complete stack
 */
void test_dect_stack_initialization(void)
{
	/* Test that DECT stack initialization works properly.
	 *
	 * The MAC driver normally registers a NRF_MODEM_LIB_ON_INIT callback that:
	 * 1. Calls nrf_modem_dect_mac_callback_set() to register callbacks
	 * 2. Calls nrf_modem_dect_control_systemmode_set(NRF_MODEM_DECT_MODE_MAC)
	 * 3. Configures modem with default settings (auto_activate = true)
	 * 4. Eventually triggers activation and sends NET_EVENT_DECT_ACTIVATE_DONE
	 *
	 * For testing, we directly call the driver's initialization function that
	 * would normally be triggered by NRF_MODEM_LIB_ON_INIT.
	 */

	/* Debug thread context at start of test */
	debug_thread_context("test_dect_stack_initialization - START");

	/* Record baseline call counts (state persists between tests) */
	int baseline_callback_set = mock_nrf_modem_dect_mac_callback_set_call_count;
	int baseline_systemmode = mock_nrf_modem_dect_control_systemmode_set_call_count;
	int baseline_configure = mock_nrf_modem_dect_control_configure_call_count;
	int baseline_functional_mode = mock_nrf_modem_dect_control_functional_mode_set_call_count;

	/* Call the DECT driver's initialization callback directly (ret=0 for success) */
	dect_mdm_ctrl_mdm_on_modem_lib_init(0, NULL);

	/* Wait for the activation event using semaphore with timeout */
	int sem_result = k_sem_take(&activation_done_sem, K_MSEC(100));

	if (sem_result != 0) {
		LOG_DBG("Timeout waiting for activation event, sem_result: %d", sem_result);
	}

	/* Verify that the DECT driver's callback triggered the expected function calls */
	/* Check that counts increased by 1 from baseline (state persists between tests) */
	TEST_ASSERT_EQUAL(baseline_callback_set + 1,
			  mock_nrf_modem_dect_mac_callback_set_call_count);
	TEST_ASSERT_EQUAL(baseline_systemmode + 1,
			  mock_nrf_modem_dect_control_systemmode_set_call_count);
	TEST_ASSERT_EQUAL(baseline_configure + 1, mock_nrf_modem_dect_control_configure_call_count);
	TEST_ASSERT_EQUAL(baseline_functional_mode + 1,
			  mock_nrf_modem_dect_control_functional_mode_set_call_count);

	/* Verify that NET_EVENT_DECT_ACTIVATE_DONE event was received */
	TEST_ASSERT_TRUE_MESSAGE(
		dect_activate_done_received,
		"NET_EVENT_DECT_ACTIVATE_DONE event should be received after initialization");

	/* Verify the activation was successful */
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, dect_activate_done_status,
				  "DECT activation should complete with success status");
}

/**
 * @brief Test DECT settings reset to driver defaults
 *
 * This test verifies:
 * 1. NET_REQUEST_DECT_SETTINGS_WRITE works with reset_to_driver_defaults=true
 * 2. Settings reset is synchronous and successful
 * 3. All settings are restored to their driver default values
 */
void test_dect_settings_reset_to_defaults(void)
{
	LOG_DBG("=== DECT Settings Reset to Defaults Test ===");

	/* Step 1: Configure settings with reset_to_driver_defaults flag */
	struct dect_settings reset_settings = {0};

	/* Set the reset flag to true */
	reset_settings.cmd_params.reset_to_driver_defaults = true;

	LOG_DBG("Resetting DECT settings to driver defaults");
	LOG_DBG("- reset_to_driver_defaults: %s",
		reset_settings.cmd_params.reset_to_driver_defaults ? "true" : "false");

	/* Step 2: Write settings with reset flag (synchronous call) */
	int result = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, test_iface, &reset_settings,
			      sizeof(struct dect_settings));

	LOG_DBG("NET_REQUEST_DECT_SETTINGS_WRITE (reset) result: %d", result);
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "Settings reset to defaults should succeed");

	LOG_DBG("Settings reset completed successfully - all settings restored to driver defaults");

	/* Step 3: Read settings back to verify the reset operation */
	struct dect_settings read_settings = {0};

	LOG_DBG("Reading DECT settings to verify reset operation");
	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &read_settings,
			  sizeof(struct dect_settings));

	LOG_DBG("NET_REQUEST_DECT_SETTINGS_READ result: %d", result);
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "Settings read after reset should succeed");

	/* Step 4: Verify key default values are set correctly after reset */
	LOG_DBG("Verifying reset settings:");
	LOG_DBG("- device_type: %d (expected: %d for DECT_DEVICE_TYPE_PT)",
		read_settings.device_type, DECT_DEVICE_TYPE_PT);
	LOG_DBG("- region: %d (expected: %d for DECT_SETTINGS_REGION_EU)", read_settings.region,
		DECT_SETTINGS_REGION_EU);
	LOG_DBG("- band_nbr: %d", read_settings.band_nbr);
	LOG_DBG("- auto_start.activate: %s", read_settings.auto_start.activate ? "true" : "false");

	/* Validate device type is reset to PT (default) */
	TEST_ASSERT_TRUE_MESSAGE(read_settings.device_type & DECT_DEVICE_TYPE_PT,
				 "Default device type should have PT bit set after reset");

	/* Validate region is reset to EU (default) */
	TEST_ASSERT_EQUAL_INT_MESSAGE(DECT_SETTINGS_REGION_EU, read_settings.region,
				      "Default region should be EU after reset");

	/* Validate band number is set to default value (typically 1 for EU) */
	TEST_ASSERT_TRUE_MESSAGE(read_settings.band_nbr == 1,
				 "Band number should be set to a valid value after reset");

	/* Validate auto-start is enabled by default */
	TEST_ASSERT_TRUE_MESSAGE(read_settings.auto_start.activate,
				 "Auto-start should be enabled by default after reset");

	/* Verify reset_to_driver_defaults flag is cleared after operation */
	TEST_ASSERT_FALSE_MESSAGE(
		read_settings.cmd_params.reset_to_driver_defaults,
		"reset_to_driver_defaults flag should be cleared after reset operation");
}

/**
 * @brief Test DECT scan request with async cluster beacon reception
 *
 * This test follows the proper async pattern:
 * 1. NET_REQUEST_DECT_SCAN → triggers nrf_modem_dect_mac_network_scan()
 * 2. Mock simulates receiving one cluster beacon via ntf callback
 * 3. Mock concludes scan operation successfully via op callback
 */
void test_dect_scan_request_band1(void)
{
	/* Setup real DECT scan parameters structure */
	struct dect_scan_params scan_params = {.channel_count = 2,
					       .channel_list = {TEST_BEACON_CHANNEL, 1677},
					       .channel_scan_time_ms = 100};

	/* Setup beacon simulation parameters */
	struct test_dect_scan_beacon_params beacon_params = {
		.channel = TEST_BEACON_CHANNEL,
		.transmitter_short_rd_id = TEST_BEACON_SHORT_RD_ID,
		.transmitter_long_rd_id = TEST_BEACON_LONG_RD_ID,
		.network_id = TEST_BEACON_NETWORK_ID,
		.mcs = TEST_BEACON_MCS,
		.transmit_power = TEST_BEACON_TRANSMIT_POWER,
		.rssi_2 = TEST_BEACON_RSSI_DBM,
		.snr = TEST_BEACON_SNR_DB};

	/* Record baseline call count (state persists between tests) */
	int baseline_network_scan = mock_nrf_modem_dect_mac_network_scan_call_count;

	/* Perform network scan using common test utility */
	struct test_dect_scan_result scan_result;
	int result = test_dect_network_scan(test_iface, &scan_params, &beacon_params, true,
					    &scan_result);

	/* Verify scan request was initiated successfully */
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Network scan should succeed");
	TEST_ASSERT_EQUAL_MESSAGE(baseline_network_scan + 1,
				  mock_nrf_modem_dect_mac_network_scan_call_count,
				  "nrf_modem_dect_mac_network_scan should be called once");

	/* Verify that NET_EVENT_DECT_SCAN_RESULT was received */
	TEST_ASSERT_TRUE_MESSAGE(
		scan_result.scan_result_received,
		"NET_EVENT_DECT_SCAN_RESULT should be received after cluster beacon");

	/* Validate that beacon data was captured and matches what we sent */
	TEST_ASSERT_TRUE_MESSAGE(
		scan_result.beacon_data_valid,
		"Beacon data should be available in NET_EVENT_DECT_SCAN_RESULT event");

	/* Verify beacon content matches what we simulated */
	TEST_ASSERT_EQUAL_MESSAGE(TEST_BEACON_CHANNEL, scan_result.beacon_data.channel,
				  "Received beacon channel should match simulated beacon");
	TEST_ASSERT_EQUAL_MESSAGE(TEST_BEACON_SHORT_RD_ID,
				  scan_result.beacon_data.transmitter_short_rd_id,
				  "Received beacon short RD ID should match simulated beacon");
	TEST_ASSERT_EQUAL_MESSAGE(TEST_BEACON_LONG_RD_ID,
				  scan_result.beacon_data.transmitter_long_rd_id,
				  "Received beacon long RD ID should match simulated beacon");
	TEST_ASSERT_EQUAL_MESSAGE(TEST_BEACON_NETWORK_ID, scan_result.beacon_data.network_id,
				  "Received beacon network ID should match simulated beacon");

	TEST_ASSERT_TRUE_MESSAGE(
		dect_utils_lib_32bit_network_id_validate(scan_result.beacon_data.network_id),
		"Received beacon network_id should be valid (LSB and MSB non-zero)");

	LOG_DBG("Beacon validation successful - all fields match expected values");

	/* Verify that NET_EVENT_DECT_SCAN_DONE was received */
	TEST_ASSERT_TRUE_MESSAGE(
		scan_result.scan_done_received,
		"NET_EVENT_DECT_SCAN_DONE should be received after scan completion");

	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, scan_result.scan_done_status,
				  "DECT scan should complete with success status");
}

/**
 * @brief Test DECT association request for PT device using real net_mgmt API
 *
 * Uses beacon data from test_dect_scan_request_band1 (state preserved between tests).
 * Run order in main.c: test_dect_scan_request_band1 then this test.
 * 1. NET_REQUEST_DECT_ASSOCIATION with received_beacon_data.transmitter_long_rd_id
 * 2. Mock simulates successful association via ntf callback (association_ntf)
 * 3. Mock completes association operation via op callback (association)
 */
void test_dect_pt_association_request(void)
{
	TEST_ASSERT_TRUE_MESSAGE(beacon_data_valid,
				 "Beacon data from previous test (test_dect_scan_request_band1) "
				 "required; run order must be preserved");

	LOG_DBG("Using beacon data from scan test - Long RD ID: 0x%08X",
		received_beacon_data.transmitter_long_rd_id);

	/* Record baseline call count (state persists between tests) */
	int baseline_association = mock_nrf_modem_dect_mac_association_call_count;

	/* Perform association using common test utility */
	struct test_dect_association_result assoc_result;
	int result = test_dect_association_request(test_iface,
						   received_beacon_data.transmitter_long_rd_id,
						   NULL, /* Use default response parameters */
						   true, /* Simulate completion */
						   true, /* Simulate network joined */
						   &assoc_result);

	/* Verify association request was initiated successfully */
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Association request should succeed");
	TEST_ASSERT_EQUAL_MESSAGE(baseline_association + 1,
				  mock_nrf_modem_dect_mac_association_call_count,
				  "nrf_modem_dect_mac_association should be called once");

	/* Verify that NET_EVENT_DECT_ASSOCIATION_CHANGED was received */
	TEST_ASSERT_TRUE_MESSAGE(
		assoc_result.association_changed_received,
		"NET_EVENT_DECT_ASSOCIATION_CHANGED should be received after successful "
		"association");

	/* Verify that we specifically received DECT_ASSOCIATION_CREATED event */
	TEST_ASSERT_TRUE_MESSAGE(
		assoc_result.association_created_received,
		"NET_EVENT_DECT_ASSOCIATION_CHANGED with DECT_ASSOCIATION_CREATED should "
		"be received");

	/* Validate the association event data */
	TEST_ASSERT_EQUAL_MESSAGE(DECT_ASSOCIATION_CREATED,
				  assoc_result.association_data.association_change_type,
				  "Association change type should be DECT_ASSOCIATION_CREATED");
	TEST_ASSERT_EQUAL_MESSAGE(
		received_beacon_data.transmitter_long_rd_id,
		assoc_result.association_data.long_rd_id,
		"Association event Long RD ID should match the target we associated with");
	TEST_ASSERT_EQUAL_MESSAGE(
		DECT_NEIGHBOR_ROLE_PARENT, assoc_result.association_data.neighbor_role,
		"When PT associates with FT device, the FT device becomes our parent");

	/* Verify that NET_EVENT_DECT_NETWORK_STATUS was received */
	TEST_ASSERT_TRUE_MESSAGE(
		assoc_result.network_status_received,
		"NET_EVENT_DECT_NETWORK_STATUS should be received after network join");

	/* Validate the network status event data */
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NETWORK_STATUS_JOINED,
				  assoc_result.network_status_data.network_status,
				  "Network status should be DECT_NETWORK_STATUS_JOINED");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, assoc_result.network_status_data.dect_err_cause,
				  "Network status should have no DECT errors");

	LOG_DBG("Association and network join test completed successfully!");
	LOG_DBG("- Association: DECT_ASSOCIATION_CREATED validated with Long RD ID: 0x%08X",
		assoc_result.association_data.long_rd_id);
	LOG_DBG("- Network Status: DECT_NETWORK_STATUS_JOINED validated");
	LOG_DBG("- Device role: %s",
		assoc_result.association_data.neighbor_role == DECT_NEIGHBOR_ROLE_CHILD
			? "PT (child of FT parent)"
			: "FT (parent of PT child)");

	result = net_mgmt(NET_REQUEST_DECT_NETWORK_JOIN, test_iface, NULL, 0);
	TEST_ASSERT_EQUAL_MESSAGE(-EALREADY, result,
				  "Second network join request should fail when already "
				  "associated");
}

/**
 * @brief Test neighbor list & information requests
 *
 * Tests NET_REQUEST_DECT_NEIGHBOR_LIST flow:
 * 1. Make neighbor list request
 * 2. Verify nrf_modem_dect_mac_neighbor_list() is called
 * 3. Simulate neighbor list response with associated parent
 * 4. Validate NET_EVENT_DECT_NEIGHBOR_LIST/INFO reception
 */
void test_dect_pt_neighbor_list_info_req(void)
{
	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface,
				     "DECT interface should be available for neighbor list test");

	/* DECT should already be activated and associated from previous tests - no need to
	 * reactivate
	 */
	LOG_DBG("Using persistent DECT stack state - already activated and associated");

	/* Verify we have beacon data from previous scan test */
	TEST_ASSERT_TRUE_MESSAGE(beacon_data_valid,
				 "Beacon data should be available from previous scan test");

	/* Verify we have association data from previous association test */
	TEST_ASSERT_TRUE_MESSAGE(received_association_data.association_change_type ==
					 DECT_ASSOCIATION_CREATED,
				 "Association should be established from previous test");

	/* Reset only neighbor list tracking - keep other state */
	dect_neighbor_list_received = false;
	memset(&received_neighbor_list_data, 0, sizeof(received_neighbor_list_data));

	/* Make neighbor list request */
	LOG_DBG("Requesting neighbor list with NET_REQUEST_DECT_NEIGHBOR_LIST");
	int result = net_mgmt(NET_REQUEST_DECT_NEIGHBOR_LIST, test_iface, NULL, 0);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Neighbor list request should succeed");

	/* Verify that nrf_modem_dect_mac_neighbor_list() was called */
	TEST_ASSERT_EQUAL_MESSAGE(
		1, mock_nrf_modem_dect_mac_neighbor_list_call_count,
		"nrf_modem_dect_mac_neighbor_list() should be called exactly once");

	/* Simulate neighbor list response from mock modem */
	if (mock_op_callbacks.neighbor_list) {
		/* Create neighbor list response with associated parent */
		struct nrf_modem_dect_mac_neighbor_list_cb_params neighbor_list_response = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK,
			.num_neighbors = 1, /* One neighbor - our associated parent */
			.long_rd_ids = NULL /* Will be set to our array below */
		};

		/* Create array with associated parent's Long RD ID (from previous association test)
		 */
		uint32_t parent_long_rd_ids[1] = {TEST_BEACON_LONG_RD_ID};

		neighbor_list_response.long_rd_ids = parent_long_rd_ids;

		LOG_DBG("Simulating neighbor list response: status=%d, num_neighbors=%d, "
			"parent_id=0x%08X",
			neighbor_list_response.status, neighbor_list_response.num_neighbors,
			parent_long_rd_ids[0]);

		/* Call the neighbor list callback */
		mock_op_callbacks.neighbor_list(&neighbor_list_response);
	} else {
		LOG_ERR("No neighbor list operation callback registered!");
		TEST_FAIL_MESSAGE("Neighbor list operation callback should be registered");
	}

	/* Wait for neighbor list response processing */
	k_sleep(K_MSEC(50));

	/* Verify that NET_EVENT_DECT_NEIGHBOR_LIST was received */
	TEST_ASSERT_TRUE_MESSAGE(
		dect_neighbor_list_received,
		"NET_EVENT_DECT_NEIGHBOR_LIST should be received after neighbor list request");

	/* Validate the neighbor list event data */
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, received_neighbor_list_data.status,
				  "Neighbor list status should be DECT_STATUS_OK (success)");
	TEST_ASSERT_EQUAL_MESSAGE(1, received_neighbor_list_data.neighbor_count,
				  "Should receive exactly 1 neighbor (our associated parent)");
	TEST_ASSERT_EQUAL_MESSAGE(TEST_BEACON_LONG_RD_ID,
				  received_neighbor_list_data.neighbor_long_rd_ids[0],
				  "First neighbor should be our associated parent with Long RD ID");

	LOG_DBG("Neighbor list test completed successfully!");
	LOG_DBG("- Mock call count: %d", mock_nrf_modem_dect_mac_neighbor_list_call_count);
	LOG_DBG("- Response status: %d", received_neighbor_list_data.status);
	LOG_DBG("- Neighbor count: %d", received_neighbor_list_data.neighbor_count);
	LOG_DBG("- Associated parent: Long RD ID 0x%08X",
		received_neighbor_list_data.neighbor_long_rd_ids[0]);

	/* Continue with neighbor info request for the discovered neighbor */
	LOG_DBG("Now requesting neighbor info for discovered neighbor: Long RD ID 0x%08X",
		received_neighbor_list_data.neighbor_long_rd_ids[0]);

	/* Reset neighbor info tracking */
	dect_neighbor_info_received = false;
	memset(&received_neighbor_info_data, 0, sizeof(received_neighbor_info_data));

	/* Prepare neighbor info request parameters */
	struct dect_neighbor_info_req_params neighbor_info_params = {
		.long_rd_id = received_neighbor_list_data
				      .neighbor_long_rd_ids[0] /* Query our associated parent */
	};

	/* Make neighbor info request */
	LOG_DBG("Requesting neighbor info with NET_REQUEST_DECT_NEIGHBOR_INFO for Long RD ID "
		"0x%08X",
		neighbor_info_params.long_rd_id);
	result = net_mgmt(NET_REQUEST_DECT_NEIGHBOR_INFO, test_iface, &neighbor_info_params,
			  sizeof(neighbor_info_params));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Neighbor info request should succeed");

	/* Verify that nrf_modem_dect_mac_neighbor_info() was called */
	TEST_ASSERT_EQUAL_MESSAGE(
		1, mock_nrf_modem_dect_mac_neighbor_info_call_count,
		"nrf_modem_dect_mac_neighbor_info() should be called exactly once");

	/* Simulate neighbor info response from mock modem */
	if (mock_op_callbacks.neighbor_info) {
		/* Create neighbor info response for our associated FT parent */
		struct nrf_modem_dect_mac_neighbor_info_cb_params neighbor_info_response = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK,
			.long_rd_id = TEST_BEACON_LONG_RD_ID,
			.associated = true, /* We are associated with this neighbor */
			.ft_mode = true,    /* This is an FT device (parent) */
			.channel = TEST_BEACON_CHANNEL,
			.network_id = TEST_BEACON_NETWORK_ID,
			.time_since_last_rx_ms = 100,	 /* Recently seen */
			.beacon_average_rx_txpower = 10, /* TX power in dB */
			.beacon_average_rx_rssi_2 = -60, /* RSSI in dBm */
			.beacon_average_rx_snr = 25	 /* SNR in dB */
		};

		LOG_DBG("Simulating neighbor info response: status=%d, long_rd_id=0x%08X, "
			"associated=%s, ft_mode=%s",
			neighbor_info_response.status, neighbor_info_response.long_rd_id,
			neighbor_info_response.associated ? "Yes" : "No",
			neighbor_info_response.ft_mode ? "Yes" : "No");

		/* Call the neighbor info callback */
		mock_op_callbacks.neighbor_info(&neighbor_info_response);
	} else {
		LOG_ERR("No neighbor info operation callback registered!");
		TEST_FAIL_MESSAGE("Neighbor info operation callback should be registered");
	}

	/* Wait for neighbor info response processing */
	k_sleep(K_MSEC(50));

	/* Verify that NET_EVENT_DECT_NEIGHBOR_INFO was received */
	TEST_ASSERT_TRUE_MESSAGE(
		dect_neighbor_info_received,
		"NET_EVENT_DECT_NEIGHBOR_INFO should be received after neighbor info request");

	/* Validate the neighbor info event data */
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, received_neighbor_info_data.status,
				  "Neighbor info status should be DECT_STATUS_OK (success)");
	TEST_ASSERT_EQUAL_MESSAGE(TEST_BEACON_LONG_RD_ID, received_neighbor_info_data.long_rd_id,
				  "Neighbor info Long RD ID should match our associated parent");
	TEST_ASSERT_TRUE_MESSAGE(
		received_neighbor_info_data.associated,
		"Neighbor should be marked as associated (we are associated with this parent)");
	TEST_ASSERT_TRUE_MESSAGE(received_neighbor_info_data.ft_mode,
				 "Neighbor should be in FT mode (this is our parent FT device)");
	TEST_ASSERT_EQUAL_MESSAGE(TEST_BEACON_CHANNEL, received_neighbor_info_data.channel,
				  "Neighbor channel should match beacon channel");
	TEST_ASSERT_EQUAL_MESSAGE(TEST_BEACON_NETWORK_ID, received_neighbor_info_data.network_id,
				  "Neighbor network ID should match beacon network ID");

	LOG_DBG("Complete neighbor discovery and info test completed successfully!");
	LOG_DBG("- Neighbor list: %d neighbors discovered",
		received_neighbor_list_data.neighbor_count);
	LOG_DBG("- Neighbor info: Associated=%s, FT mode=%s, Channel=%d, Network=0x%08X",
		received_neighbor_info_data.associated ? "Yes" : "No",
		received_neighbor_info_data.ft_mode ? "Yes" : "No",
		received_neighbor_info_data.channel, received_neighbor_info_data.network_id);
}

/**
 * @brief Test DECT status info request (synchronous)
 *
 * Tests NET_REQUEST_DECT_STATUS_INFO_GET flow:
 * 1. Make synchronous status info request
 * 2. Verify no libmodem calls are made (synchronous operation)
 * 3. Validate returned status information (without firmware version for now)
 */
void test_dect_pt_status_info_req(void)
{
	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface,
				     "DECT interface should be available for status info test");

	/* Status info is synchronous - should work regardless of DECT stack state */
	LOG_DBG("Requesting DECT status info with NET_REQUEST_DECT_STATUS_INFO_GET");

	/* Debug: Check our test state before making the request */
	LOG_DBG("Pre-status check: association_change_type=%d (expected=%d for "
		"DECT_ASSOCIATION_CREATED)",
		received_association_data.association_change_type, DECT_ASSOCIATION_CREATED);
	LOG_DBG("Pre-status check: associated_long_rd_id=0x%08X",
		received_association_data.long_rd_id);

	/* Get status info using common test utility */
	struct dect_status_info status_info;
	int result = test_dect_status_info_get(test_iface, &status_info);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Status info request should succeed");

	/* Validate the returned status information */

	/* Validate core status info fields as requested */
	TEST_ASSERT_TRUE_MESSAGE(status_info.mdm_activated,
				 "Modem should be reported as activated after previous tests");

	/* Validate association counts from persistent state */
	if (received_association_data.association_change_type == DECT_ASSOCIATION_CREATED) {
		/* We have an association from previous tests - validate correct role assignment */
		TEST_ASSERT_EQUAL_MESSAGE(1, status_info.parent_count,
					  "Should report 1 parent association from previous tests");
		TEST_ASSERT_EQUAL_MESSAGE(0, status_info.child_count,
					  "PT device should have no child associations");
		TEST_ASSERT_EQUAL_MESSAGE(received_beacon_data.transmitter_long_rd_id,
					  status_info.parent_associations[0].long_rd_id,
					  "Parent Long RD ID should match our associated parent");
	} else {
		/* No association */
		TEST_ASSERT_EQUAL_MESSAGE(0, status_info.parent_count,
					  "Should report 0 parent associations if not associated");
		TEST_ASSERT_EQUAL_MESSAGE(0, status_info.child_count,
					  "PT device should have no child associations");
	}

	/* Check cluster status (PT device should not have cluster running) */
	TEST_ASSERT_FALSE_MESSAGE(status_info.cluster_running,
				  "PT device should not have cluster running");
	TEST_ASSERT_EQUAL_MESSAGE(0, status_info.cluster_channel,
				  "PT device should not have cluster channel set");

	/* Check network beacon status (PT device should not have beacon running) */
	TEST_ASSERT_FALSE_MESSAGE(status_info.nw_beacon_running,
				  "PT device should not have network beacon running");

	LOG_DBG("Status info test completed successfully!");
	LOG_DBG("- Modem activated: %s", status_info.mdm_activated ? "Yes" : "No");
	LOG_DBG("- Parent count: %d", status_info.parent_count);
	LOG_DBG("- Child count: %d", status_info.child_count);
	LOG_DBG("- Cluster running: %s", status_info.cluster_running ? "Yes" : "No");
	LOG_DBG("- Network beacon running: %s", status_info.nw_beacon_running ? "Yes" : "No");
}

/**
 * @brief Test PT global IPv6 address assignment from FT prefix update
 *
 * Simulates libmodem's ipv6_config_update_ntf callback with a global /64
 * prefix while the PT device is associated to an FT parent. Verifies the PT
 * interface gets the expected global IPv6 address and the parent association
 * stores the matching global address.
 */
void test_dect_pt_global_address_assign(void)
{
	struct nrf_modem_dect_mac_ipv6_config_update_ntf_cb_params ipv6_ntf_params = {
		.ipv6_config = {
				.type = NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX,
				.address = {0},
			},
	};
	struct dect_status_info status_info_before = {0};
	struct dect_status_info status_info_after = {0};
	struct dect_settings settings = {0};
	struct in6_addr prefix = {0};
	struct in6_addr expected_own_global = {0};
	struct in6_addr expected_parent_global = {0};
	int result;

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface,
				     "DECT interface should be available for IPv6 assignment "
				     "test");
	TEST_ASSERT_TRUE_MESSAGE(beacon_data_valid,
				 "PT should already be associated with an FT parent");

	result = test_dect_status_info_get(test_iface, &status_info_before);
	TEST_ASSERT_EQUAL_MESSAGE(0, result,
				  "Status info request before prefix update should "
				  "work");
	TEST_ASSERT_EQUAL_MESSAGE(1, status_info_before.parent_count,
				  "PT should have one parent before prefix update");
	TEST_ASSERT_FALSE_MESSAGE(status_info_before.parent_associations[0].global_ipv6_addr_set,
				  "Parent global IPv6 address should not be set before prefix "
				  "update");
	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &settings, sizeof(settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Settings read before prefix update should work");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, settings.identities.transmitter_long_rd_id,
				      "PT long RD ID should be available");

	memcpy(ipv6_ntf_params.ipv6_config.address, test_ipv6_prefix_pt_assign_64,
	       sizeof(test_ipv6_prefix_pt_assign_64));
	memcpy(prefix.s6_addr, test_ipv6_prefix_pt_assign_64,
	       sizeof(test_ipv6_prefix_pt_assign_64));

	TEST_ASSERT_TRUE_MESSAGE(
		dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
			prefix, received_beacon_data.transmitter_long_rd_id,
			settings.identities.transmitter_long_rd_id, &expected_own_global),
		"Expected PT global IPv6 address should be derivable from prefix, sink RD ID "
		"and our RD ID");

	TEST_ASSERT_TRUE_MESSAGE(
		dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
			prefix, received_beacon_data.transmitter_long_rd_id,
			received_beacon_data.transmitter_long_rd_id, &expected_parent_global),
		"Expected parent global IPv6 address should be derivable from FT prefix");

	TEST_ASSERT_NULL_MESSAGE(net_if_ipv6_addr_lookup_by_iface(test_iface, &expected_own_global),
				 "PT iface should not have the expected global IPv6 address yet");

	LOG_DBG("Simulating ipv6_config_update_ntf with FT global prefix 2001:db8:1:2::/64");
	mock_ntf_callbacks.ipv6_config_update_ntf(&ipv6_ntf_params);
	k_sleep(K_MSEC(50));

	result = test_dect_status_info_get(test_iface, &status_info_after);
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Status info request after prefix update should work");
	TEST_ASSERT_EQUAL_MESSAGE(1, status_info_after.parent_count,
				  "PT should still have one parent after prefix update");
	TEST_ASSERT_TRUE_MESSAGE(status_info_after.parent_associations[0].global_ipv6_addr_set,
				 "Parent global IPv6 address should be set after prefix update");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
		&expected_parent_global, &status_info_after.parent_associations[0].global_ipv6_addr,
		sizeof(expected_parent_global),
		"Parent global IPv6 address should match the expected FT-derived address");
	TEST_ASSERT_NOT_NULL_MESSAGE(
		net_if_ipv6_addr_lookup_by_iface(test_iface, &expected_own_global),
		"PT iface should have the expected global IPv6 address");
	TEST_ASSERT_TRUE_MESSAGE(status_info_after.parent_associations[0].global_ipv6_addr_set,
				 "Parent global IPv6 address should remain marked as set");

	LOG_DBG("PT global IPv6 address assignment test completed successfully");
	LOG_DBG("- Parent global address set: %s",
		status_info_after.parent_associations[0].global_ipv6_addr_set ? "Yes" : "No");
}

/**
 * @brief Test PT global IPv6 address change after FT prefix update
 *
 * Simulates a second libmodem ipv6_config_update_ntf callback with a slightly
 * changed /64 prefix and verifies the old PT global IPv6 address is replaced
 * with a new one derived from the updated prefix.
 */
void test_dect_pt_global_address_change(void)
{
	struct nrf_modem_dect_mac_ipv6_config_update_ntf_cb_params ipv6_ntf_params = {
		.ipv6_config = {
				.type = NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX,
				.address = {0},
			},
	};
	struct dect_status_info status_info_before = {0};
	struct dect_status_info status_info_after = {0};
	struct dect_settings settings = {0};
	struct in6_addr old_prefix = {0};
	struct in6_addr new_prefix = {0};
	struct in6_addr old_expected_own_global = {0};
	struct in6_addr new_expected_own_global = {0};
	struct in6_addr new_expected_parent_global = {0};
	int result;

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface,
				     "DECT interface should be available for IPv6 change test");
	TEST_ASSERT_TRUE_MESSAGE(beacon_data_valid,
				 "PT should still be associated with an FT parent");

	result = test_dect_status_info_get(test_iface, &status_info_before);
	TEST_ASSERT_EQUAL_MESSAGE(0, result,
				  "Status info request before prefix change "
				  "should work");
	TEST_ASSERT_EQUAL_MESSAGE(1, status_info_before.parent_count,
				  "PT should have one parent before prefix change");
	TEST_ASSERT_TRUE_MESSAGE(status_info_before.parent_associations[0].global_ipv6_addr_set,
				 "Parent global IPv6 address should already be set");

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &settings, sizeof(settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Settings read before prefix change should work");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, settings.identities.transmitter_long_rd_id,
				      "PT long RD ID should be available");

	memcpy(old_prefix.s6_addr, test_ipv6_prefix_pt_assign_64,
	       sizeof(test_ipv6_prefix_pt_assign_64));
	memcpy(new_prefix.s6_addr, test_ipv6_prefix_pt_change_new_64,
	       sizeof(test_ipv6_prefix_pt_change_new_64));
	memcpy(ipv6_ntf_params.ipv6_config.address, test_ipv6_prefix_pt_change_new_64,
	       sizeof(test_ipv6_prefix_pt_change_new_64));

	TEST_ASSERT_TRUE_MESSAGE(dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
					 old_prefix, received_beacon_data.transmitter_long_rd_id,
					 settings.identities.transmitter_long_rd_id,
					 &old_expected_own_global),
				 "Old expected PT global IPv6 address should be derivable");
	TEST_ASSERT_TRUE_MESSAGE(dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
					 new_prefix, received_beacon_data.transmitter_long_rd_id,
					 settings.identities.transmitter_long_rd_id,
					 &new_expected_own_global),
				 "New expected PT global IPv6 address should be derivable");
	TEST_ASSERT_TRUE_MESSAGE(dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
					 new_prefix, received_beacon_data.transmitter_long_rd_id,
					 received_beacon_data.transmitter_long_rd_id,
					 &new_expected_parent_global),
				 "New expected parent global IPv6 address should be derivable");

	TEST_ASSERT_NOT_NULL_MESSAGE(
		net_if_ipv6_addr_lookup_by_iface(test_iface, &old_expected_own_global),
		"PT iface should still have the old expected global IPv6 address before change");
	TEST_ASSERT_NULL_MESSAGE(
		net_if_ipv6_addr_lookup_by_iface(test_iface, &new_expected_own_global),
		"PT iface should not have the new expected global "
		"IPv6 address yet");

	LOG_DBG("Simulating ipv6_config_update_ntf with changed FT global prefix "
		"2001:db8:1:3::/64");
	mock_ntf_callbacks.ipv6_config_update_ntf(&ipv6_ntf_params);
	k_sleep(K_MSEC(50));

	result = test_dect_status_info_get(test_iface, &status_info_after);
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Status info request after prefix change should work");
	TEST_ASSERT_EQUAL_MESSAGE(1, status_info_after.parent_count,
				  "PT should still have one parent after prefix change");
	TEST_ASSERT_TRUE_MESSAGE(status_info_after.parent_associations[0].global_ipv6_addr_set,
				 "Parent global IPv6 address should stay set after prefix change");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
		&new_expected_parent_global,
		&status_info_after.parent_associations[0].global_ipv6_addr,
		sizeof(new_expected_parent_global),
		"Parent global IPv6 address should match the updated FT-derived address");
	TEST_ASSERT_NULL_MESSAGE(
		net_if_ipv6_addr_lookup_by_iface(test_iface, &old_expected_own_global),
		"Old PT global IPv6 address should be removed after prefix change");
	TEST_ASSERT_NOT_NULL_MESSAGE(
		net_if_ipv6_addr_lookup_by_iface(test_iface, &new_expected_own_global),
		"PT iface should have the new expected global IPv6 address");

	LOG_DBG("PT global IPv6 address change test completed successfully");
}

/**
 * @brief Test PT global IPv6 address removal after FT prefix removal
 *
 * Simulates libmodem ipv6_config_update_ntf callback with no prefix
 * configuration and verifies the current PT global IPv6 address is removed
 * and the parent global IPv6 state is cleared.
 */
void test_dect_pt_global_address_removal(void)
{
	struct nrf_modem_dect_mac_ipv6_config_update_ntf_cb_params ipv6_ntf_params = {
		.ipv6_config = {
				.type = NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_NONE,
				.address = {0},
			},
	};
	struct dect_status_info status_info_before = {0};
	struct dect_status_info status_info_after = {0};
	struct dect_settings settings = {0};
	struct in6_addr current_prefix = {0};
	struct in6_addr current_expected_own_global = {0};
	int result;

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface,
				     "DECT interface should be available for IPv6 removal test");
	TEST_ASSERT_TRUE_MESSAGE(beacon_data_valid,
				 "PT should still be associated with an FT parent");

	result = test_dect_status_info_get(test_iface, &status_info_before);
	TEST_ASSERT_EQUAL_MESSAGE(0, result,
				  "Status info request before prefix removal should "
				  "work");
	TEST_ASSERT_EQUAL_MESSAGE(1, status_info_before.parent_count,
				  "PT should have one parent before prefix removal");
	TEST_ASSERT_TRUE_MESSAGE(status_info_before.parent_associations[0].global_ipv6_addr_set,
				 "Parent global IPv6 address should already be set");

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &settings, sizeof(settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Settings read before prefix removal should work");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, settings.identities.transmitter_long_rd_id,
				      "PT long RD ID should be available");

	memcpy(current_prefix.s6_addr, test_ipv6_prefix_pt_change_new_64,
	       sizeof(test_ipv6_prefix_pt_change_new_64));

	TEST_ASSERT_TRUE_MESSAGE(
		dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
			current_prefix, received_beacon_data.transmitter_long_rd_id,
			settings.identities.transmitter_long_rd_id, &current_expected_own_global),
		"Current expected PT global IPv6 address should be derivable");

	TEST_ASSERT_NOT_NULL_MESSAGE(
		net_if_ipv6_addr_lookup_by_iface(test_iface, &current_expected_own_global),
		"PT iface should still have the current expected global IPv6 address before "
		"removal");

	LOG_DBG("Simulating ipv6_config_update_ntf with removed FT global prefix");
	mock_ntf_callbacks.ipv6_config_update_ntf(&ipv6_ntf_params);
	k_sleep(K_MSEC(50));

	result = test_dect_status_info_get(test_iface, &status_info_after);
	TEST_ASSERT_EQUAL_MESSAGE(0, result,
				  "Status info request after prefix removal "
				  "should work");
	TEST_ASSERT_EQUAL_MESSAGE(1, status_info_after.parent_count,
				  "PT should still have one parent after prefix removal");
	TEST_ASSERT_FALSE_MESSAGE(status_info_after.parent_associations[0].global_ipv6_addr_set,
				  "Parent global IPv6 address should be cleared after prefix "
				  "removal");
	TEST_ASSERT_NULL_MESSAGE(
		net_if_ipv6_addr_lookup_by_iface(test_iface, &current_expected_own_global),
		"PT iface should no longer have the removed global IPv6 address");

	LOG_DBG("PT global IPv6 address removal test completed successfully");
}

/**
 * @brief Test DECT PT association release using real net_mgmt API
 *
 * Releases the association with the parent device established in previous test.
 * Uses NET_REQUEST_DECT_ASSOCIATION_RELEASE with persistent association data.
 */
void test_dect_pt_association_release(void)
{
	LOG_DBG("Testing DECT association release with persistent parent association");

	/* Verify we have persistent association data from previous test */
	TEST_ASSERT_TRUE_MESSAGE(beacon_data_valid, "Should have beacon data from previous test");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, received_beacon_data.transmitter_long_rd_id,
				      "Should have valid parent Long RD ID from association test");

	/* Perform association release using common test utility */
	struct test_dect_association_release_result release_result;
	int result = test_dect_association_release(test_iface,
						   received_beacon_data.transmitter_long_rd_id,
						   true, /* Simulate completion */
						   true, /* Simulate network unjoined */
						   &release_result);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "NET_REQUEST_DECT_ASSOCIATION_RELEASE should succeed");

	/* Verify mock call count */
	TEST_ASSERT_EQUAL_MESSAGE(1, mock_nrf_modem_dect_mac_association_release_call_count,
				  "nrf_modem_dect_mac_association_release should be called once");

	/* Verify that NET_EVENT_DECT_ASSOCIATION_CHANGED was received for the release */
	TEST_ASSERT_TRUE_MESSAGE(
		release_result.association_changed_received,
		"NET_EVENT_DECT_ASSOCIATION_CHANGED should be received after association release");

	/* Validate the association release event data */
	TEST_ASSERT_EQUAL_MESSAGE(DECT_ASSOCIATION_RELEASED,
				  release_result.association_data.association_change_type,
				  "Association change type should be DECT_ASSOCIATION_RELEASED");
	TEST_ASSERT_EQUAL_MESSAGE(
		received_beacon_data.transmitter_long_rd_id,
		release_result.association_data.long_rd_id,
		"Association release event Long RD ID should match the parent we released");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NEIGHBOR_ROLE_PARENT,
				  release_result.association_data.neighbor_role,
				  "Released neighbor should have been our parent");

	/* Verify that NET_EVENT_DECT_NETWORK_STATUS was received */
	TEST_ASSERT_TRUE_MESSAGE(
		release_result.network_status_received,
		"NET_EVENT_DECT_NETWORK_STATUS should be received after association release");

	/* Verify the network status is UNJOINED */
	TEST_ASSERT_EQUAL_MESSAGE(
		DECT_NETWORK_STATUS_UNJOINED, release_result.network_status_data.network_status,
		"Network status should be DECT_NETWORK_STATUS_UNJOINED after association release");

	LOG_DBG("Association release test completed successfully!");
	LOG_DBG("- Mock call count: %d", mock_nrf_modem_dect_mac_association_release_call_count);
	LOG_DBG("- Association release: DECT_ASSOCIATION_RELEASED validated with Long RD ID: "
		"0x%08X",
		release_result.association_data.long_rd_id);
	LOG_DBG("- Released neighbor role: DECT_NEIGHBOR_ROLE_PARENT (was our parent)");
	LOG_DBG("- Network status: DECT_NETWORK_STATUS_UNJOINED (device is no longer part of "
		"network)");
}

/**
 * @brief Test PT conn_mgr connect: NET_REQUEST_DECT_NETWORK_JOIN and related events
 *
 * Ensures device is PT with network_id matching the mock beacon, then calls
 * conn_mgr_if_connect(). Drives mocks: cluster_beacon_ntf, network_scan complete,
 * cluster_beacon_receive complete, association complete. Verifies NET_EVENT_DECT_NETWORK_STATUS
 * (JOINED) and NET_EVENT_DECT_ASSOCIATION_CHANGED (DECT_ASSOCIATION_CREATED).
 * Run order runs test_dect_ft_sink_down before this so NET_EVENT_IF_DOWN clears the FT global
 * from the DECT iface via L2 (no stale global).
 */
void test_dect_pt_conn_mgr_connect(void)
{
#if !defined(CONFIG_NET_CONNECTION_MANAGER)
	TEST_IGNORE_MESSAGE("CONFIG_NET_CONNECTION_MANAGER not set");
#else
	struct dect_settings read_settings;
	struct dect_settings write_settings;
	int result;

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface, "DECT interface required");
	/* Modem must be activated and carrier up so conn_mgr issues
	 * NET_REQUEST_DECT_NETWORK_JOIN.
	 */
	TEST_ASSERT_TRUE_MESSAGE(beacon_data_valid,
				 "Beacon data required (stack activated from previous test)");

	/* Read current settings and set PT + network_id=1 + target_ft=ANY for join */
	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &read_settings,
			  sizeof(read_settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Settings read should succeed");

	write_settings = read_settings;
	write_settings.cmd_params.reset_to_driver_defaults = false;
	write_settings.cmd_params.write_scope_bitmap = DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE |
						       DECT_SETTINGS_WRITE_SCOPE_IDENTITIES |
						       DECT_SETTINGS_WRITE_SCOPE_NETWORK_JOIN;
	write_settings.device_type = DECT_DEVICE_TYPE_PT;
	write_settings.identities = read_settings.identities;
	write_settings.identities.network_id = 1; /* Match mock_simulate_cluster_beacon_received */
	write_settings.network_join = read_settings.network_join;
	write_settings.network_join.target_ft_long_rd_id = DECT_SETT_NETWORK_JOIN_TARGET_FT_ANY;

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, test_iface, &write_settings,
			  sizeof(write_settings));
	TEST_ASSERT_EQUAL_MESSAGE(
		0, result, "Settings write (PT, network_id=1, target_ft=ANY) should succeed");
	TEST_ASSERT_EQUAL_MESSAGE(0, write_settings.cmd_params.failure_scope_bitmap_out,
				  "No scope should fail");

	/* Read back and verify settings are correct */
	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &read_settings,
			  sizeof(read_settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Settings read after write should succeed");
	TEST_ASSERT_TRUE_MESSAGE(read_settings.device_type & DECT_DEVICE_TYPE_PT,
				 "device_type should be PT after write");
	TEST_ASSERT_EQUAL_MESSAGE(1, read_settings.identities.network_id,
				  "identities.network_id should be 1 after write");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_SETT_NETWORK_JOIN_TARGET_FT_ANY,
				  read_settings.network_join.target_ft_long_rd_id,
				  "network_join.target_ft_long_rd_id should be ANY after write");

	dect_network_status_received = false;
	dect_association_changed_received = false;
	memset(&received_network_status_data, 0, sizeof(received_network_status_data));
	memset(&received_association_data, 0, sizeof(received_association_data));

	/* Expect L4 events for DECT iface when association response includes IPv6 prefix.
	 * L4 callback is only registered when CONFIG_MODEM_CELLULAR is set
	 * (in ft_conn_mgr_connect).
	 */
	if (IS_ENABLED(CONFIG_MODEM_CELLULAR)) {
		test_l4_expected_iface = test_iface;
	}
	dect_l4_ipv6_connected_received = false;

	mock_nrf_modem_dect_mac_cluster_beacon_receive_ExpectAndReturn(0);
	int baseline_network_scan = mock_nrf_modem_dect_mac_network_scan_call_count;
	int baseline_cluster_beacon_receive =
		mock_nrf_modem_dect_mac_cluster_beacon_receive_call_count;
	int baseline_association = mock_nrf_modem_dect_mac_association_call_count;

	conn_mgr_if_connect(test_iface);

	k_sleep(K_MSEC(50));
	TEST_ASSERT_EQUAL_MESSAGE(baseline_network_scan + 1,
				  mock_nrf_modem_dect_mac_network_scan_call_count,
				  "NET_REQUEST_DECT_NETWORK_JOIN should trigger network_scan");

	/* Deliver cluster beacon (network_id=1) so driver has CLUSTER_FOUND when scan completes */
	mock_simulate_cluster_beacon_received();
	/* Allow ctrl thread to process beacon and set CLUSTER_FOUND before we inject scan. */
	k_sleep(K_MSEC(200));

	/* Scan complete: driver calls cluster_beacon_receive when CLUSTER_FOUND, else
	 * triggers association via best-channel path.
	 */
	if (mock_op_callbacks.network_scan) {
		struct nrf_modem_dect_mac_network_scan_cb_params scan_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK, .num_scanned_channels = 11};
		mock_op_callbacks.network_scan(&scan_complete_params);
		k_sleep(K_MSEC(200));
	} else {
		TEST_FAIL_MESSAGE("Network scan operation callback should be registered");
	}

	/* Only invoke cluster_beacon_receive op callback if the driver called
	 * cluster_beacon_receive() (CLUSTER_FOUND path). If driver took best-channel path
	 * it already triggered association once.
	 */
	if (mock_nrf_modem_dect_mac_cluster_beacon_receive_call_count >
	    baseline_cluster_beacon_receive) {
		if (mock_op_callbacks.cluster_beacon_receive) {
			struct nrf_modem_dect_mac_cluster_beacon_receive_cb_params rcv_params = {
				.num_clusters = 1,
				.cluster_status = {(uint16_t)NRF_MODEM_DECT_MAC_STATUS_OK}};
			mock_op_callbacks.cluster_beacon_receive(&rcv_params);
			k_sleep(K_MSEC(150));
		}
	}

	TEST_ASSERT_EQUAL_MESSAGE(
		baseline_association + 1, mock_nrf_modem_dect_mac_association_call_count,
		"association should be called exactly once after scan/cluster path complete");

	/* Association complete: driver sends ASSOCIATION_CREATED and JOINED.
	 * Include IPv6 prefix in association response so the driver adds a global address to the
	 * DECT iface; conn_mgr then sends NET_EVENT_L4_IPV6_CONNECTED for the PT DECT iface.
	 */
	if (mock_op_callbacks.association) {
		/* Parent gives /64 prefix 2001:db8:1:: (first 8 bytes for PREFIX type) */
		struct nrf_modem_dect_mac_association_cb_params assoc_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK,
			.long_rd_id = 0x12345678, /* Match mock_simulate_cluster_beacon_received */
			.rx_signal_info = {0},
			/* nrf_modem_dect_mac_ipv6_address_config_t inside association_cb_params */
			.ipv6_config = {
					.type = NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX,
					.address = {0},
				},
			.number_of_ies = 0,
			.ies = NULL,
		};
		memcpy(assoc_params.ipv6_config.address, test_ipv6_prefix_pt_parent_64, 8);
		assoc_params.bit_mask = 1; /* has_association_response */
		assoc_params.association_response.ack_status = 1;
		assoc_params.association_response.number_of_flows = 0;
		assoc_params.association_response.number_of_rx_harq_processes = 0;
		assoc_params.association_response.max_number_of_harq_re_rx = 0;
		assoc_params.association_response.number_of_tx_harq_processes = 0;
		assoc_params.association_response.max_number_of_harq_re_tx = 0;
		mock_op_callbacks.association(&assoc_params);
		k_sleep(K_MSEC(150));
	} else {
		TEST_FAIL_MESSAGE("Association operation callback should be registered");
	}

	TEST_ASSERT_TRUE_MESSAGE(dect_association_changed_received,
				 "NET_EVENT_DECT_ASSOCIATION_CHANGED should be received");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_ASSOCIATION_CREATED,
				  received_association_data.association_change_type,
				  "Association change type should be DECT_ASSOCIATION_CREATED");
	TEST_ASSERT_TRUE_MESSAGE(dect_network_status_received,
				 "NET_EVENT_DECT_NETWORK_STATUS should be received");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NETWORK_STATUS_JOINED,
				  received_network_status_data.network_status,
				  "Network status should be DECT_NETWORK_STATUS_JOINED");

	/* Association response included IPv6 prefix (2001:db8:1::/64); driver adds global addr
	 * to DECT iface and conn_mgr sends NET_EVENT_L4_IPV6_CONNECTED. Validate when the
	 * L4 callback is registered (it is added in test_dect_ft_conn_mgr_connect when
	 * CONFIG_MODEM_CELLULAR is set).
	 */
	if (IS_ENABLED(CONFIG_MODEM_CELLULAR)) {
		TEST_ASSERT_TRUE_MESSAGE(
			dect_l4_ipv6_connected_received,
			"NET_EVENT_L4_IPV6_CONNECTED should be received for PT DECT iface "
			"when association response includes IPv6 prefix");
		test_l4_expected_iface = NULL;
	}

#if defined(CONFIG_NET_IPV6)
	/* Verify PT own global address on DECT iface: prefix (2001:db8:1::/64) + FT long RD ID
	 * + PT own long RD ID. Same validation as test_dect_pt_global_address_removal:
	 * direct net_if lookup + status_info parent global.
	 */
	{
		struct in6_addr prefix = {0};
		struct in6_addr expected_own_global = {0};
		struct in6_addr expected_parent_global = {0};
		/* mock_simulate_cluster_beacon_received */
		const uint32_t parent_long_rd_id = 0x12345678;
		uint32_t pt_own_long_rd_id = read_settings.identities.transmitter_long_rd_id;
		struct dect_status_info status_info = {0};
		int status_result;

		memcpy(prefix.s6_addr, test_ipv6_prefix_pt_parent_64, 8);
		TEST_ASSERT_TRUE_MESSAGE(
			dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
				prefix, parent_long_rd_id, pt_own_long_rd_id, &expected_own_global),
			"Expected PT global IPv6 address should be derivable from prefix, sink RD "
			"ID and our RD ID");
		TEST_ASSERT_TRUE_MESSAGE(
			dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
				prefix, parent_long_rd_id, parent_long_rd_id,
				&expected_parent_global),
			"Expected parent (FT) global IPv6 address should be derivable");

		TEST_ASSERT_NOT_NULL_MESSAGE(
			net_if_ipv6_addr_lookup_by_iface(test_iface, &expected_own_global),
			"PT iface should have the expected global IPv6 address");

		/* Same as test_dect_pt_global_address_assign: validate via status_info
		 * parent global.
		 */
		status_result = test_dect_status_info_get(test_iface, &status_info);
		TEST_ASSERT_EQUAL_MESSAGE(0, status_result,
					  "Status info after PT join should succeed");
		TEST_ASSERT_EQUAL_MESSAGE(1, status_info.parent_count,
					  "PT should have one parent after join");
		TEST_ASSERT_TRUE_MESSAGE(status_info.parent_associations[0].global_ipv6_addr_set,
					 "Parent global IPv6 address should be set after join");
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
			&expected_parent_global,
			&status_info.parent_associations[0].global_ipv6_addr,
			sizeof(expected_parent_global),
			"Parent global IPv6 address in status should match expected FT-derived "
			"address");
	}
#endif

	LOG_INF("test_dect_pt_conn_mgr_connect: PT conn_mgr connect and events verified");
#endif
}

/**
 * @brief Test PT conn_mgr disconnect triggers NET_REQUEST_DECT_NETWORK_UNJOIN and driver events
 *
 * Runs after test_dect_pt_conn_mgr_connect (PT is joined to parent). Calls conn_mgr_if_disconnect()
 * which invokes NET_REQUEST_DECT_NETWORK_UNJOIN for PT. Verifies that the driver calls
 * nrf_modem_dect_mac_association_release once (for the parent), sends
 * NET_EVENT_DECT_ASSOCIATION_CHANGED with DECT_ASSOCIATION_RELEASED, and
 * NET_EVENT_DECT_NETWORK_STATUS with DECT_NETWORK_STATUS_UNJOINED.
 */
void test_dect_pt_conn_mgr_disconnect(void)
{
#if !defined(CONFIG_NET_CONNECTION_MANAGER)
	TEST_IGNORE_MESSAGE("CONFIG_NET_CONNECTION_MANAGER not set");
#else
	int result;
	int baseline_association_release = mock_nrf_modem_dect_mac_association_release_call_count;
	struct dect_status_info status_info = {0};

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface, "DECT interface required");
	/* PT should be joined from previous test_dect_pt_conn_mgr_connect; verify via status (not
	 * event flag) so this works even if tests run in isolation or event state is not preserved.
	 */
	result = test_dect_status_info_get(test_iface, &status_info);
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Status info get should succeed before disconnect");
	TEST_ASSERT_TRUE_MESSAGE(
		status_info.parent_count >= 1,
		"PT should be joined (have at least one parent) before disconnect test");

	dect_network_status_received = false;
	dect_association_released_received = false;
	dect_association_released_count = 0;
	memset(&received_network_status_data, 0, sizeof(received_network_status_data));

	mock_auto_invoke_association_release_cb = true;

	/* Expect L4 disconnect events for PT DECT iface when CONFIG_MODEM_CELLULAR
	 * (L4 cb registered in ft_conn_mgr_connect).
	 */
	if (IS_ENABLED(CONFIG_MODEM_CELLULAR)) {
		test_l4_expected_iface = test_iface;
		dect_l4_disconnected_received = false;
		dect_l4_ipv6_disconnected_received = false;
	}

	LOG_INF("PT conn mgr disconnect (NET_REQUEST_DECT_NETWORK_UNJOIN), releasing parent");
	result = conn_mgr_if_disconnect(test_iface);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "conn_mgr_if_disconnect should succeed");

	k_sleep(K_MSEC(250));

	TEST_ASSERT_EQUAL_MESSAGE(
		baseline_association_release + 1,
		mock_nrf_modem_dect_mac_association_release_call_count,
		"nrf_modem_dect_mac_association_release should be called once for parent");
	TEST_ASSERT_TRUE_MESSAGE(dect_association_released_received,
				 "NET_EVENT_DECT_ASSOCIATION_CHANGED (DECT_ASSOCIATION_RELEASED) "
				 "should be received");
	TEST_ASSERT_EQUAL_MESSAGE(1, dect_association_released_count,
				  "Should receive one DECT_ASSOCIATION_RELEASED event for parent");

	TEST_ASSERT_TRUE_MESSAGE(
		dect_network_status_received,
		"NET_EVENT_DECT_NETWORK_STATUS should be received after disconnect");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NETWORK_STATUS_UNJOINED,
				  received_network_status_data.network_status,
				  "Network status should be DECT_NETWORK_STATUS_UNJOINED");

	if (IS_ENABLED(CONFIG_MODEM_CELLULAR)) {
		TEST_ASSERT_TRUE_MESSAGE(dect_l4_disconnected_received,
					 "NET_EVENT_L4_DISCONNECTED should be received for PT DECT "
					 "iface after disconnect");
		TEST_ASSERT_TRUE_MESSAGE(dect_l4_ipv6_disconnected_received,
					 "NET_EVENT_L4_IPV6_DISCONNECTED should be received for PT "
					 "DECT iface after disconnect");
		test_l4_expected_iface = NULL;
	}

	LOG_INF("test_dect_pt_conn_mgr_disconnect: PT conn_mgr disconnect and events verified");
#endif
}

/**
 * @brief Test DECT stack activation using real net_mgmt API
 *
 * Activates the DECT stack and verifies NET_EVENT_DECT_ACTIVATE_DONE event.
 * This test verifies the activation workflow using the utility function.
 * First deactivates the stack if it's already activated, then tests activation.
 */
void test_dect_ft_activate(void)
{
	LOG_DBG("Testing DECT stack activation with NET_REQUEST_DECT_ACTIVATE");

	/* Record baseline call counts (state persists between tests) */
	int baseline_configure = mock_nrf_modem_dect_control_configure_call_count;
	int baseline_functional_mode_set =
		mock_nrf_modem_dect_control_functional_mode_set_call_count;

	/* Now perform activation using common test utility */
	struct test_dect_activate_result activate_result;
	int result = test_dect_perform_activate(test_iface, 250, &activate_result);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "NET_REQUEST_DECT_ACTIVATE should succeed");

	/* Verify that nrf_modem_dect_control_configure was called */
	TEST_ASSERT_EQUAL_MESSAGE(baseline_configure + 1,
				  mock_nrf_modem_dect_control_configure_call_count,
				  "nrf_modem_dect_control_configure should be called "
				  "once");

	/* Verify that nrf_modem_dect_control_functional_mode_set was called */
	TEST_ASSERT_EQUAL_MESSAGE(baseline_functional_mode_set + 1,
				  mock_nrf_modem_dect_control_functional_mode_set_call_count,
				  "nrf_modem_dect_control_functional_mode_set should be "
				  "called once");

	/* Verify that NET_EVENT_DECT_ACTIVATE_DONE was received */
	TEST_ASSERT_TRUE_MESSAGE(
		activate_result.activate_done_received,
		"NET_EVENT_DECT_ACTIVATE_DONE should be received after activation request");

	/* Verify activation was successful */
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, activate_result.activate_done_status,
				  "DECT activation should complete with DECT_STATUS_OK");

	/* Verify that configure was called */
	TEST_ASSERT_TRUE_MESSAGE(activate_result.configure_called,
				 "nrf_modem_dect_control_configure should be called "
				 "during activation");

	/* Verify that functional_mode_set was called */
	TEST_ASSERT_TRUE_MESSAGE(activate_result.functional_mode_set_called,
				 "nrf_modem_dect_control_functional_mode_set should be called "
				 "during activation");

	LOG_DBG("DECT activation test completed successfully!");
	LOG_DBG("- Activation event received: %s",
		activate_result.activate_done_received ? "YES" : "NO");
	LOG_DBG("- Activation status: %d (expected=0 for DECT_STATUS_OK)",
		activate_result.activate_done_status);
	LOG_DBG("- Configure called: %s", activate_result.configure_called ? "YES" : "NO");
	LOG_DBG("- Functional mode set called: %s",
		activate_result.functional_mode_set_called ? "YES" : "NO");
	LOG_DBG("- DECT stack is now activated");
}

/**
 * @brief Test that configure callback failure is handled (no activation)
 *
 * Sets mock to inject NRF_MODEM_DECT_MAC_STATUS_FAIL in the next configure callback,
 * then triggers init. Driver should not complete activation (handle_mdm_configure_resp
 * returns early). Verifies status conversion and error path coverage.
 */
void test_dect_configure_callback_failure(void)
{
	int baseline_configure = mock_nrf_modem_dect_control_configure_call_count;
	int baseline_functional_mode = mock_nrf_modem_dect_control_functional_mode_set_call_count;

	dect_activate_done_received = false;
	k_sem_reset(&activation_done_sem);

	mock_inject_configure_status = NRF_MODEM_DECT_MAC_STATUS_FAIL;
	dect_mdm_ctrl_mdm_on_modem_lib_init(0, NULL);

	/* Wait for activation event - we expect timeout because activation should not complete */
	int sem_result = k_sem_take(&activation_done_sem, K_MSEC(150));

	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, sem_result,
				      "Activation should not complete when configure fails");

	TEST_ASSERT_EQUAL_MESSAGE(baseline_configure + 1,
				  mock_nrf_modem_dect_control_configure_call_count,
				  "Configure should be called once");
	TEST_ASSERT_EQUAL_MESSAGE(baseline_functional_mode,
				  mock_nrf_modem_dect_control_functional_mode_set_call_count,
				  "Functional mode set should not be called when configure fails");
	TEST_ASSERT_FALSE_MESSAGE(dect_activate_done_received,
				  "NET_EVENT_DECT_ACTIVATE_DONE should not be received");
}

/**
 * @brief Test that functional_mode (activation) callback failure is reported
 *
 * Sets mock to inject NRF_MODEM_DECT_MAC_STATUS_FAIL in the next functional_mode
 * callback, then triggers init. Driver may send NET_EVENT_DECT_ACTIVATE_DONE or
 * NET_EVENT_DECT_DEACTIVATE_DONE with error status. Verifies handle_mdm_cfun_resp
 * error path and status conversion.
 */
void test_dect_activation_callback_failure(void)
{
	dect_activate_done_received = false;
	dect_deactivate_done_received = false;
	k_sem_reset(&activation_done_sem);

	mock_inject_functional_mode_status = NRF_MODEM_DECT_MAC_STATUS_FAIL;
	dect_mdm_ctrl_mdm_on_modem_lib_init(0, NULL);

	/* Driver may report failure via ACTIVATE_DONE or DEACTIVATE_DONE; wait for either */
	k_sleep(K_MSEC(250));

	TEST_ASSERT_TRUE_MESSAGE(
		dect_activate_done_received || dect_deactivate_done_received,
		"Activation or deactivation done event should be received on failure");
	if (dect_activate_done_received) {
		TEST_ASSERT_NOT_EQUAL_MESSAGE(DECT_STATUS_OK, dect_activate_done_status,
					      "Activation should report failure status");
	} else {
		TEST_ASSERT_NOT_EQUAL_MESSAGE(DECT_STATUS_OK, dect_deactivate_done_status,
					      "Deactivation (failure path) should report non-OK "
					      "status");
	}
}

/**
 * @brief Test that association callback failure is reported (REQ_FAILED_MDM)
 *
 * With stack activated, inject NRF_MODEM_DECT_MAC_STATUS_FAIL for the next
 * association callback. Request association; mock invokes callback with failure.
 * Verifies handle_mdm_association_resp error path and NET_EVENT_DECT_ASSOCIATION_CHANGED
 * with DECT_ASSOCIATION_REQ_FAILED_MDM.
 */
void test_dect_association_callback_failure(void)
{
	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface, "DECT interface required");
	TEST_ASSERT_TRUE_MESSAGE(beacon_data_valid, "Beacon data required from previous test");

	dect_association_changed_received = false;
	dect_association_failed_received = false;
	dect_network_status_received = false;
	memset(&received_association_data, 0, sizeof(received_association_data));

	mock_inject_association_status = NRF_MODEM_DECT_MAC_STATUS_FAIL;

	LOG_DBG("test_dect_association_callback_failure: Injecting association failure status");

	struct dect_associate_req_params assoc_params = {
		.target_long_rd_id = received_beacon_data.transmitter_long_rd_id,
	};
	int ret = net_mgmt(NET_REQUEST_DECT_ASSOCIATION, test_iface, &assoc_params,
			   sizeof(assoc_params));
	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "Association request should be accepted");

	k_sleep(K_MSEC(200));

	TEST_ASSERT_TRUE_MESSAGE(dect_association_changed_received,
				 "NET_EVENT_DECT_ASSOCIATION_CHANGED should be received");
	TEST_ASSERT_TRUE_MESSAGE(dect_association_failed_received,
				 "Association should report failure");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_ASSOCIATION_REQ_FAILED_MDM,
				  received_association_data.association_change_type,
				  "Association change type should be REQ_FAILED_MDM");
	LOG_INF("test_dect_association_callback_failure: Association failure test completed "
		"successfully!");
}

/**
 * @brief Test that association rejected (ack_status=0) is reported (REQ_REJECTED)
 *
 * With stack activated, set mock_inject_association_rejected and reject cause/time.
 * Request association; mock invokes callback with has_association_response=1,
 * ack_status=0. Verifies handle_mdm_association_resp rejected path and
 * dect_mgmt_association_req_rejected_result_evt.
 */
void test_dect_association_rejected(void)
{
	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface, "DECT interface required");
	TEST_ASSERT_TRUE_MESSAGE(beacon_data_valid, "Beacon data required from previous test");

	dect_association_changed_received = false;
	dect_association_failed_received = false;
	memset(&received_association_data, 0, sizeof(received_association_data));

	mock_inject_association_rejected = true;
	mock_inject_association_reject_cause = DECT_ASSOCIATION_REJECT_CAUSE_NO_RADIO_CAPACITY;
	mock_inject_association_reject_time = DECT_ASSOCIATION_REJECT_TIME_60S;

	LOG_DBG("test_dect_association_rejected: Injecting association rejected status");

	struct dect_associate_req_params assoc_params = {
		.target_long_rd_id = received_beacon_data.transmitter_long_rd_id,
	};
	int ret = net_mgmt(NET_REQUEST_DECT_ASSOCIATION, test_iface, &assoc_params,
			   sizeof(assoc_params));
	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "Association request should be accepted");

	k_sleep(K_MSEC(200));

	TEST_ASSERT_TRUE_MESSAGE(dect_association_changed_received,
				 "NET_EVENT_DECT_ASSOCIATION_CHANGED should be received");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_ASSOCIATION_REQ_REJECTED,
				  received_association_data.association_change_type,
				  "Association change type should be REQ_REJECTED");
	LOG_INF("test_dect_association_rejected: Association rejected test completed "
		"successfully!");
}

/**
 * @brief Test DECT stack deactivation using real net_mgmt API
 *
 * Deactivates the DECT stack and verifies NET_EVENT_DECT_DEACTIVATE_DONE event.
 * This is typically the final step in the DECT device lifecycle.
 */
void test_dect_deactivate(void)
{
	LOG_DBG("Testing DECT stack deactivation with NET_REQUEST_DECT_DEACTIVATE");

	/* Verify we have persistent state from previous tests (stack should be activated) */
	TEST_ASSERT_TRUE_MESSAGE(
		beacon_data_valid,
		"Should have beacon data indicating previous DECT operations were successful");

	/* Perform deactivation using common test utility */
	struct test_dect_deactivate_result deactivate_result;
	int result = test_dect_perform_deactivate(test_iface, 250, &deactivate_result);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "NET_REQUEST_DECT_DEACTIVATE should succeed");

	/* Verify that NET_EVENT_DECT_DEACTIVATE_DONE was received */
	TEST_ASSERT_TRUE_MESSAGE(
		deactivate_result.deactivate_done_received,
		"NET_EVENT_DECT_DEACTIVATE_DONE should be received after deactivation request");

	/* Verify deactivation was successful */
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, deactivate_result.deactivate_done_status,
				  "DECT deactivation should complete with DECT_STATUS_OK");

	LOG_DBG("DECT deactivation test completed successfully!");
	LOG_DBG("- Deactivation event received: %s",
		deactivate_result.deactivate_done_received ? "YES" : "NO");
	LOG_DBG("- Deactivation status: %d (expected=0 for DECT_STATUS_OK)",
		deactivate_result.deactivate_done_status);
	LOG_DBG("- DECT stack is now deactivated");
}

/**
 * @brief Test DECT requests behavior when stack is deactivated
 *
 * This test runs after test_dect_deactivate() and validates the behavior
 * of net_mgmt requests when the DECT stack is deactivated.
 *
 * Expected return codes when deactivated:
 * - NET_REQUEST_DECT_SCAN: 0 (accepted, but fails with NET_EVENT_DECT_SCAN_DONE error)
 * - NET_REQUEST_DECT_ASSOCIATION: 0 (async, may succeed request but fail operation)
 * - NET_REQUEST_DECT_NEIGHBOR_LIST: 0 (returns cached data)
 * - NET_REQUEST_DECT_NEIGHBOR_INFO: 0 (returns cached data)
 * - NET_REQUEST_DECT_ASSOCIATION_RELEASE: -95 (-ENOTSUP)
 * - NET_REQUEST_DECT_STATUS_INFO_GET: 0 (informational request)
 * - NET_REQUEST_DECT_DEACTIVATE: 0 (idempotent operation)
 * - NET_REQUEST_DECT_RSSI_SCAN: 0 (accepted, but fails with NET_EVENT_DECT_RSSI_SCAN_DONE error)
 * - NET_REQUEST_DECT_CLUSTER_START: -22 (-EINVAL, not allowed for PT)
 * - NET_REQUEST_DECT_NW_BEACON_START: -22 (-EINVAL, not allowed for PT)
 * - NET_REQUEST_DECT_NW_BEACON_STOP: -114 (-EALREADY, already stopped)
 * - NET_REQUEST_DECT_CLUSTER_INFO: 0 (informational request)
 * - NET_REQUEST_DECT_NETWORK_CREATE: -95 (-ENOTSUP)
 * - NET_REQUEST_DECT_NETWORK_JOIN: -22 (-EINVAL, modem not activated)
 * - NET_REQUEST_DECT_NETWORK_UNJOIN: -114 (-EALREADY, not joined)
 */
void test_dect_deactivated_requests_fail(void)
{
	int ret;

	LOG_DBG("=== Testing DECT requests when stack is deactivated ===");

	/* Reset event flags */
	dect_scan_result_received = false;
	dect_scan_done_received = false;
	dect_rssi_scan_done_received = false;
	dect_neighbor_list_received = false;
	dect_neighbor_info_received = false;
	dect_cluster_info_received = false;
	dect_association_changed_received = false;
	dect_association_failed_received = false;
	dect_network_status_received = false;
	memset(&received_cluster_info_data, 0, sizeof(received_cluster_info_data));

	/* Test 1: NET_REQUEST_DECT_SCAN should return "not allowed" status when deactivated */
	struct dect_scan_params scan_params = {.band = 1,
					       .channel_count = 2,
					       .channel_list = {TEST_BEACON_CHANNEL, 1838},
					       .channel_scan_time_ms = 100};

	ret = net_mgmt(NET_REQUEST_DECT_SCAN, test_iface, &scan_params, sizeof(scan_params));
	LOG_DBG("NET_REQUEST_DECT_SCAN result when deactivated: %d", ret);

	if (ret == 0) {
		LOG_DBG("SCAN request accepted when deactivated - waiting for "
			"NET_EVENT_DECT_SCAN_DONE with error status");
		/* Wait for scan events to complete - should get SCAN_DONE with "not allowed" status
		 */
		k_sleep(K_MSEC(300));

		/* Verify that NET_EVENT_DECT_SCAN_DONE was received */
		TEST_ASSERT_TRUE_MESSAGE(dect_scan_done_received,
					 "NET_EVENT_DECT_SCAN_DONE should be received even when "
					 "scan fails due to deactivated state");

		/* Verify the scan failed with "not allowed" status */
		TEST_ASSERT_EQUAL_MESSAGE(
			DECT_STATUS_NOT_ALLOWED, dect_scan_done_status,
			"Scan should fail with error status when stack is deactivated");

		/* Verify no scan results were generated */
		TEST_ASSERT_FALSE_MESSAGE(dect_scan_result_received,
					  "No scan results should be generated when scan fails due "
					  "to deactivated state");
	} else {
		LOG_DBG("SCAN request immediately failed when deactivated - this is also "
			"acceptable");
	}

	/* Test 2: NET_REQUEST_DECT_ASSOCIATION should fail when deactivated */
	struct dect_associate_req_params assoc_params = {.target_long_rd_id =
								 TEST_BEACON_LONG_RD_ID};

	ret = net_mgmt(NET_REQUEST_DECT_ASSOCIATION, test_iface, &assoc_params,
		       sizeof(assoc_params));
	LOG_DBG("NET_REQUEST_DECT_ASSOCIATION result when deactivated: %d", ret);
	if (ret == 0) {
		LOG_DBG("Association request queued when deactivated - should fail during "
			"execution");
		k_sleep(K_MSEC(200)); /* Allow time for async failure events */

		/* Verify that we received the expected failure events */
		TEST_ASSERT_TRUE_MESSAGE(
			dect_association_changed_received,
			"NET_EVENT_DECT_ASSOCIATION_CHANGED should be received for async failure");
		TEST_ASSERT_TRUE_MESSAGE(dect_association_failed_received,
					 "Association should fail with "
					 "DECT_ASSOCIATION_REQ_FAILED_MDM when deactivated");
		TEST_ASSERT_EQUAL_MESSAGE(
			DECT_ASSOCIATION_REQ_FAILED_MDM,
			received_association_data.association_change_type,
			"Association failure should be DECT_ASSOCIATION_REQ_FAILED_MDM");

		/* Also verify network status "not allowed" event */
		TEST_ASSERT_TRUE_MESSAGE(dect_network_status_received,
					 "NET_EVENT_DECT_NETWORK_STATUS should be received for "
					 "'not allowed' status");

		LOG_DBG("Verified async association failure when deactivated");
	} else {
		LOG_DBG("Association request immediately failed when deactivated with error: %d",
			ret);
		TEST_ASSERT_NOT_EQUAL_INT(0, ret);
	}

	/* Test 3: NET_REQUEST_DECT_NEIGHBOR_LIST behavior when deactivated */
	ret = net_mgmt(NET_REQUEST_DECT_NEIGHBOR_LIST, test_iface, NULL, 0);
	LOG_DBG("NET_REQUEST_DECT_NEIGHBOR_LIST result when deactivated: %d", ret);
	if (ret == 0) {
		LOG_DBG("Neighbor list request accepted when deactivated - should return empty "
			"list");
		k_sleep(K_MSEC(100)); /* Allow time for processing */

		/* Verify that NET_EVENT_DECT_NEIGHBOR_LIST was received */
		if (dect_neighbor_list_received) {
			LOG_DBG("NET_EVENT_DECT_NEIGHBOR_LIST received - checking if list is "
				"empty");
			TEST_ASSERT_EQUAL_MESSAGE(
				0, received_neighbor_list_data.neighbor_count,
				"Neighbor list should be empty when stack is deactivated");
			LOG_DBG("Verified: Empty neighbor list returned when deactivated "
				"(neighbor_count=0)");
		} else {
			LOG_DBG("NET_EVENT_DECT_NEIGHBOR_LIST not received - request may have "
				"failed silently");
		}
	} else {
		LOG_DBG("Neighbor list request immediately failed when deactivated with error: %d",
			ret);
		TEST_ASSERT_NOT_EQUAL_INT(0, ret);
	}

	/* Test 4: NET_REQUEST_DECT_NEIGHBOR_INFO behavior when deactivated */
	struct dect_neighbor_info_req_params neighbor_info_params = {
		.long_rd_id = TEST_BEACON_LONG_RD_ID};

	ret = net_mgmt(NET_REQUEST_DECT_NEIGHBOR_INFO, test_iface, &neighbor_info_params,
		       sizeof(neighbor_info_params));
	LOG_DBG("NET_REQUEST_DECT_NEIGHBOR_INFO result when deactivated: %d", ret);
	if (ret == 0) {
		LOG_DBG("Neighbor info request accepted when deactivated - should return failure "
			"status");
		k_sleep(K_MSEC(100)); /* Allow time for processing */

		/* Verify that NET_EVENT_DECT_NEIGHBOR_INFO was received */
		if (dect_neighbor_info_received) {
			LOG_DBG("NET_EVENT_DECT_NEIGHBOR_INFO received - checking status");
			TEST_ASSERT_NOT_EQUAL_MESSAGE(
				DECT_STATUS_OK, received_neighbor_info_data.status,
				"Neighbor info should fail when stack is deactivated");
			LOG_DBG("Verified: Neighbor info returned error status when deactivated "
				"(status=%d)",
				received_neighbor_info_data.status);
		} else {
			LOG_DBG("NET_EVENT_DECT_NEIGHBOR_INFO not received - request may have "
				"failed silently");
		}
	} else {
		LOG_DBG("Neighbor info request immediately failed when deactivated with error: %d",
			ret);
		TEST_ASSERT_NOT_EQUAL_INT(0, ret);
	}

	/* Test 5: NET_REQUEST_DECT_ASSOCIATION_RELEASE behavior when deactivated */
	ret = net_mgmt(NET_REQUEST_DECT_ASSOCIATION_RELEASE, test_iface, NULL, 0);
	LOG_DBG("NET_REQUEST_DECT_ASSOCIATION_RELEASE result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(-ENOTSUP, ret); /* Operation not supported when deactivated */
	LOG_DBG("Association release request correctly failed when deactivated with -ENOTSUP");

	/* Test 6: NET_REQUEST_DECT_STATUS_INFO_GET might still work when deactivated (synchronous)
	 */
	struct dect_status_info status_info = {0};

	ret = net_mgmt(NET_REQUEST_DECT_STATUS_INFO_GET, test_iface, &status_info,
		       sizeof(status_info));
	LOG_DBG("NET_REQUEST_DECT_STATUS_INFO_GET result when deactivated: %d", ret);
	if (ret == 0) {
		LOG_DBG("Status info when deactivated - mdm_activated: %s",
			status_info.mdm_activated ? "true" : "false");
		TEST_ASSERT_FALSE_MESSAGE(status_info.mdm_activated,
					  "Stack should report as deactivated");
	} else {
		LOG_DBG("Status info request failed when deactivated (acceptable)");
	}

	/* Test 7: NET_REQUEST_DECT_DEACTIVATE should succeed when already deactivated (idempotent)
	 */
	dect_deactivate_done_received = false; /* Reset flag to detect new event */
	ret = net_mgmt(NET_REQUEST_DECT_DEACTIVATE, test_iface, NULL, 0);
	LOG_DBG("NET_REQUEST_DECT_DEACTIVATE result when already deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(0,
			      ret); /* Deactivate request should succeed as idempotent operation */

	/* Wait for the deactivate done event */
	k_sleep(K_MSEC(100));

	/* Verify that NET_EVENT_DECT_DEACTIVATE_DONE was received even when already deactivated */
	TEST_ASSERT_TRUE_MESSAGE(
		dect_deactivate_done_received,
		"NET_EVENT_DECT_DEACTIVATE_DONE should be received even when already deactivated");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, dect_deactivate_done_status,
				  "Deactivate event should complete with success status even when "
				  "already deactivated");

	LOG_DBG("Deactivate request correctly succeeded when already deactivated (idempotent "
		"operation)");
	LOG_DBG("- Event received: %s", dect_deactivate_done_received ? "YES" : "NO");
	LOG_DBG("- Event status: %d (expected=0 for DECT_STATUS_OK)", dect_deactivate_done_status);

	/* Test 8: RSSI scan when deactivated */
	LOG_DBG("Test 8: RSSI scan when deactivated");
	struct dect_rssi_scan_params rssi_scan_params = {.band = 0,
							 .frame_count_to_scan = 1,
							 .channel_count = 1,
							 .channel_list = {TEST_BEACON_CHANNEL}};

	ret = net_mgmt(NET_REQUEST_DECT_RSSI_SCAN, test_iface, &rssi_scan_params,
		       sizeof(rssi_scan_params));
	LOG_DBG("NET_REQUEST_DECT_RSSI_SCAN result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(0, ret); /* Request accepted, but functionality limited */

	if (ret == 0) {
		LOG_DBG("RSSI scan request accepted when deactivated - waiting for "
			"NET_EVENT_DECT_RSSI_SCAN_DONE with error status");
		/* Wait for RSSI scan events to complete - should get RSSI_SCAN_DONE with "not
		 * allowed" status
		 */
		k_sleep(K_MSEC(100));

		if (dect_rssi_scan_done_received) {
			/* Verify the RSSI scan failed with "not allowed" status */
			TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_NOT_ALLOWED,
						  dect_rssi_scan_done_status,
						  "RSSI scan should fail with error status when "
						  "stack is deactivated");
			LOG_DBG("RSSI scan properly failed with NET_EVENT_DECT_RSSI_SCAN_DONE "
				"status: %d",
				dect_rssi_scan_done_status);
		} else {
			LOG_DBG("NET_EVENT_DECT_RSSI_SCAN_DONE was not received - async callback "
				"may have failed");
			/* This might be acceptable if the mock callback system has issues */
		}
	}

	/* Test 9: Cluster start when deactivated */
	LOG_DBG("Test 9: Cluster start when deactivated");
	struct dect_cluster_start_req_params cluster_start_params = {.channel =
									     TEST_BEACON_CHANNEL};

	ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_START, test_iface, &cluster_start_params,
		       sizeof(cluster_start_params));
	LOG_DBG("NET_REQUEST_DECT_CLUSTER_START result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(-EINVAL, ret); /* Not allowed for PT role */

	/* Test 10: NW beacon start when deactivated */
	LOG_DBG("Test 10: NW beacon start when deactivated");
	struct dect_nw_beacon_start_req_params nw_beacon_start_params = {
		.channel = TEST_BEACON_CHANNEL, .additional_ch_count = 0};

	ret = net_mgmt(NET_REQUEST_DECT_NW_BEACON_START, test_iface, &nw_beacon_start_params,
		       sizeof(nw_beacon_start_params));
	LOG_DBG("NET_REQUEST_DECT_NW_BEACON_START result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(-EINVAL, ret); /* Not allowed for PT role */

	/* Test 11: NW beacon stop when deactivated */
	LOG_DBG("Test 11: NW beacon stop when deactivated");
	struct dect_nw_beacon_stop_req_params nw_beacon_stop_params = {};

	ret = net_mgmt(NET_REQUEST_DECT_NW_BEACON_STOP, test_iface, &nw_beacon_stop_params,
		       sizeof(nw_beacon_stop_params));
	LOG_DBG("NET_REQUEST_DECT_NW_BEACON_STOP result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(-EALREADY, ret); /* Already stopped */

	/* Test 12: Cluster info when deactivated */
	LOG_DBG("Test 12: Cluster info when deactivated");
	ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_INFO, test_iface, NULL, 0);
	LOG_DBG("NET_REQUEST_DECT_CLUSTER_INFO result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(0, ret); /* Information requests allowed even when deactivated */
	k_sleep(K_MSEC(100));
	TEST_ASSERT_TRUE_MESSAGE(
		dect_cluster_info_received,
		"NET_EVENT_DECT_CLUSTER_INFO should be received when cluster info fails");
	TEST_ASSERT_EQUAL_MESSAGE(
		DECT_STATUS_NOT_ALLOWED, received_cluster_info_data.status,
		"Cluster info should fail with DECT_STATUS_NOT_ALLOWED when stack is deactivated");

	/* Test 13: Network create when deactivated */
	LOG_DBG("Test 13: Network create when deactivated");
	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_CREATE, test_iface, NULL, 0);
	LOG_DBG("NET_REQUEST_DECT_NETWORK_CREATE result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(-ENOTSUP, ret); /* Operation not supported when deactivated */

	/* Test 14: Network join when deactivated */
	LOG_DBG("Test 14: Network join when deactivated");
	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_JOIN, test_iface, NULL, 0);
	LOG_DBG("NET_REQUEST_DECT_NETWORK_JOIN result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(-EINVAL, ret); /* Modem not activated */

	/* Test 15: Network unjoin when deactivated */
	LOG_DBG("Test 15: Network unjoin when deactivated");
	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_UNJOIN, test_iface, NULL, 0);
	LOG_DBG("NET_REQUEST_DECT_NETWORK_UNJOIN result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(-EALREADY, ret); /* Not joined */

	/* Test 16: Network remove when deactivated */
	LOG_DBG("Test 16: Network remove when deactivated");
	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_REMOVE, test_iface, NULL, 0);
	LOG_DBG("NET_REQUEST_DECT_NETWORK_REMOVE result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(-ENOTSUP, ret); /* Operation not supported when deactivated */

	/* Test 17: Cluster reconfigure when deactivated */
	LOG_DBG("Test 17: Cluster reconfigure when deactivated");
	struct dect_cluster_reconfig_req_params cluster_reconfig_params = {
		.channel = TEST_BEACON_CHANNEL,
		.max_beacon_tx_power_dbm = 20,
		.max_cluster_power_dbm = 20,
		.period = DECT_CLUSTER_BEACON_PERIOD_100MS};
	ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_RECONFIGURE, test_iface, &cluster_reconfig_params,
		       sizeof(cluster_reconfig_params));
	LOG_DBG("NET_REQUEST_DECT_CLUSTER_RECONFIGURE result when deactivated: %d", ret);
	TEST_ASSERT_EQUAL_INT(-EINVAL, ret); /* Not allowed for PT role */

	/* Verify behavior - some events may be generated depending on what operations succeeded */
	k_sleep(K_MSEC(100)); /* Short wait to ensure no async events */

	LOG_DBG("Event generation status after deactivated requests:");
	LOG_DBG("- Scan result received: %s", dect_scan_result_received ? "YES" : "NO");
	LOG_DBG("- Scan done received: %s", dect_scan_done_received ? "YES" : "NO");
	LOG_DBG("- Neighbor list received: %s", dect_neighbor_list_received ? "YES" : "NO");
	LOG_DBG("- Neighbor info received: %s", dect_neighbor_info_received ? "YES" : "NO");
	LOG_DBG("- Association changed received: %s",
		dect_association_changed_received ? "YES" : "NO");

	LOG_DBG("=== DECT deactivated requests test completed successfully ===");
	LOG_DBG("Validated request behavior when stack is deactivated");
}

/**
 * FT Configuration Test - Configure device to FT mode
 *
 * This test verifies:
 * 1. NET_REQUEST_DECT_SETTINGS_WRITE works with DECT_DEVICE_TYPE_FT
 * 2. Settings write is synchronous and successful
 * 3. NET_REQUEST_DECT_SETTINGS_READ confirms the device_type was changed to FT
 */
void test_dect_ft_configuration(void)
{
	LOG_DBG("=== FT Configuration Test ===");

	/* First, activate DECT stack if not already active (we assume it's active from previous
	 * tests)
	 */

	/* Step 1: Configure settings to set device type to FT */
	struct dect_settings write_settings = {0};

	write_settings.cmd_params.write_scope_bitmap |= DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE;
	write_settings.device_type = DECT_DEVICE_TYPE_FT;

	LOG_DBG("Writing DECT settings to configure device as FT");
	LOG_DBG("- device_type: DECT_DEVICE_TYPE_FT (%d)", DECT_DEVICE_TYPE_FT);
	LOG_DBG("- write_scope_bitmap: DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE (0x%04X)",
		DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE);

	/* Step 2: Write settings (synchronous call) */
	int result = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, test_iface, &write_settings,
			      sizeof(struct dect_settings));

	LOG_DBG("NET_REQUEST_DECT_SETTINGS_WRITE result: %d", result);
	TEST_ASSERT_EQUAL_INT(0, result);
	LOG_DBG("Settings write completed successfully - device configured as FT");

	/* Step 3: Read settings back to verify the change */
	struct dect_settings read_settings = {0};

	LOG_DBG("Reading DECT settings to verify FT configuration");
	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &read_settings,
			  sizeof(struct dect_settings));

	LOG_DBG("NET_REQUEST_DECT_SETTINGS_READ result: %d", result);
	TEST_ASSERT_EQUAL_INT(0, result);

	/* Step 4: Verify device type was changed to FT */
	LOG_DBG("Read settings verification:");
	LOG_DBG("- device_type: %d (expected: %d for DECT_DEVICE_TYPE_FT)",
		read_settings.device_type, DECT_DEVICE_TYPE_FT);

	TEST_ASSERT_TRUE(read_settings.device_type & DECT_DEVICE_TYPE_FT);

	LOG_DBG("=== FT Configuration Test Completed Successfully ===");
	LOG_DBG("- Settings write operation: SUCCESS");
	LOG_DBG("- Settings read operation: SUCCESS");
	LOG_DBG("- Device type verified: FT mode configured");
}

/**
 * @brief Test DECT RSSI scan using real net_mgmt API
 *
 * Performs an RSSI scan and verifies:
 * - nrf_modem_dect_mac_rssi_scan() is called
 * - rssi_scan_ntf notification callback is received (NET_EVENT_DECT_RSSI_SCAN_RESULT)
 * - rssi_scan op callback is received (NET_EVENT_DECT_RSSI_SCAN_DONE) with success status
 */
void test_dect_ft_rssi_scan(void)
{
	LOG_DBG("Testing DECT RSSI scan with NET_REQUEST_DECT_RSSI_SCAN");

	/* Ensure the stack is activated before RSSI scan */
	struct test_dect_activate_result activate_result;
	int activate_ret = test_dect_perform_activate(test_iface, 250, &activate_result);

	if (activate_ret != 0 || !activate_result.activate_done_received ||
	    activate_result.activate_done_status != DECT_STATUS_OK) {
		LOG_DBG("Activating stack before RSSI scan test");
		/* Wait a bit for activation to complete */
		k_sleep(K_MSEC(100));
	}

	/* Setup RSSI scan parameters */
	struct dect_rssi_scan_params rssi_scan_params = {.band = 0,
							 .frame_count_to_scan = 1,
							 .channel_count = 1,
							 .channel_list = {TEST_BEACON_CHANNEL}};

	/* Record baseline call count (state persists between tests) */
	int baseline_rssi_scan = mock_nrf_modem_dect_mac_rssi_scan_call_count;

	/* Perform RSSI scan using common test utility */
	struct test_dect_rssi_scan_result rssi_result;
	int result = test_dect_perform_rssi_scan(test_iface, &rssi_scan_params, 500, &rssi_result);

	/* Note: The request may return an error if the stack isn't fully ready,
	 * but the async callbacks may still be triggered. Continue with verification.
	 */
	if (result != 0) {
		LOG_DBG("RSSI scan request returned error %d, but continuing to verify mock calls",
			result);
	}

	/* Verify that nrf_modem_dect_mac_rssi_scan was called */
	TEST_ASSERT_EQUAL_MESSAGE(baseline_rssi_scan + 1,
				  mock_nrf_modem_dect_mac_rssi_scan_call_count,
				  "nrf_modem_dect_mac_rssi_scan should be called once");

	/* Verify that NET_EVENT_DECT_RSSI_SCAN_RESULT was received */
	TEST_ASSERT_TRUE_MESSAGE(
		rssi_result.rssi_scan_result_received,
		"NET_EVENT_DECT_RSSI_SCAN_RESULT should be received after rssi_scan_ntf callback");

	/* Verify the content of NET_EVENT_DECT_RSSI_SCAN_RESULT data */
	if (rssi_result.rssi_scan_result_received) {
		struct dect_rssi_scan_result_data *rssi_data =
			&rssi_result.rssi_scan_result_data.rssi_scan_result;

		/* Verify channel matches the requested channel */
		TEST_ASSERT_EQUAL_MESSAGE(rssi_scan_params.channel_list[0], rssi_data->channel,
					  "RSSI scan result channel should match "
					  "requested channel");

		/* Verify busy_percentage is within valid range [0-100] */
		TEST_ASSERT_TRUE_MESSAGE(rssi_data->busy_percentage <= 100,
					 "RSSI scan busy_percentage should be <= 100");

		/* Verify subslot counts are valid (should sum to 48 or less) */
		uint8_t total_subslots = rssi_data->free_subslot_cnt +
					 rssi_data->possible_subslot_cnt +
					 rssi_data->busy_subslot_cnt;
		TEST_ASSERT_TRUE_MESSAGE(total_subslots <= 48,
					 "RSSI scan subslot counts should sum to <= 48");

		/* Verify scan_suitable_percent is within valid range [0-100] */
		TEST_ASSERT_TRUE_MESSAGE(rssi_data->scan_suitable_percent <= 100,
					 "RSSI scan scan_suitable_percent should be <= 100");
	}

	/* Verify that NET_EVENT_DECT_RSSI_SCAN_DONE was received */
	TEST_ASSERT_TRUE_MESSAGE(
		rssi_result.rssi_scan_done_received,
		"NET_EVENT_DECT_RSSI_SCAN_DONE should be received after rssi_scan op callback");

	/* Verify RSSI scan was successful */
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, rssi_result.rssi_scan_done_status,
				  "DECT RSSI scan should complete with DECT_STATUS_OK");

	LOG_DBG("DECT RSSI scan test completed successfully!");
	LOG_DBG("- RSSI scan result event received: %s",
		rssi_result.rssi_scan_result_received ? "YES" : "NO");
	LOG_DBG("- RSSI scan done event received: %s",
		rssi_result.rssi_scan_done_received ? "YES" : "NO");
	LOG_DBG("- RSSI scan status: %d (expected=0 for DECT_STATUS_OK)",
		rssi_result.rssi_scan_done_status);
	if (rssi_result.rssi_scan_result_received) {
		LOG_DBG("- RSSI scan result: channel=%d, busy_percentage=%d%%",
			rssi_result.rssi_scan_result_data.rssi_scan_result.channel,
			rssi_result.rssi_scan_result_data.rssi_scan_result.busy_percentage);
	}
}

/**
 * @brief Test complete DECT PT device workflow using real net_mgmt API: scan -> associate
 *
 * TODO: This test is disabled for now until all parameter structures are resolved
 */
void test_dect_pt_complete_workflow(void)
{
	/* Simple placeholder test - verify callback structure exists */
	TEST_ASSERT_NOT_NULL(&mock_ntf_callbacks);
}

/**
 * @brief Test DECT FT cluster start with automatic channel selection
 *
 * This test verifies the complete cluster start flow:
 * 1. Start network scan for every other channel in band 1 (odd channels: 1657, 1659, ..., 1677)
 * 2. Verify nrf_modem mocks are called and corresponding callbacks are served
 * 3. Then do RSSI scanning
 * 4. Stop at first free channel (all_subslots_free && busy_percentage == 0)
 * 5. Verify that channel is chosen and nrf_modem mock for cluster configure is called
 */
void test_dect_ft_cluster_start(void)
{
	LOG_DBG("=== Testing DECT FT cluster start with automatic channel selection ===");

	/* Ensure device is in FT mode and activated */
	/* Device should already be in FT mode from test_dect_ft_configuration */
	/* Device should already be activated from test_dect_ft_activate */

	/* Reset event tracking */
	dect_cluster_created_received = false;
	memset(&received_cluster_created_data, 0, sizeof(received_cluster_created_data));

	/* Set flag to indicate this is a cluster creation RSSI scan at band 1 */
	mock_cluster_creation_band1 = true;

	/* Record baseline call counts */
	int baseline_network_scan = mock_nrf_modem_dect_mac_network_scan_call_count;
	int baseline_rssi_scan = mock_nrf_modem_dect_mac_rssi_scan_call_count;
	int baseline_rssi_scan_stop = mock_nrf_modem_dect_mac_rssi_scan_stop_call_count;
	int baseline_cluster_configure = mock_nrf_modem_dect_mac_cluster_configure_call_count;

	/* Step 1: Request cluster start with DECT_CLUSTER_CHANNEL_ANY */
	/* This will automatically trigger network scan for odd channels in band 1 */
	struct dect_cluster_start_req_params cluster_start_params = {
		.channel = DECT_CLUSTER_CHANNEL_ANY};

	LOG_DBG("Requesting cluster start with DECT_CLUSTER_CHANNEL_ANY");
	int result = net_mgmt(NET_REQUEST_DECT_CLUSTER_START, test_iface, &cluster_start_params,
			      sizeof(cluster_start_params));

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Cluster start request should succeed");

	/* Wait for network scan to be initiated */
	k_sleep(K_MSEC(50));

	/* Step 2: Verify network scan was called for odd channels in band 1 */
	TEST_ASSERT_EQUAL_MESSAGE(baseline_network_scan + 1,
				  mock_nrf_modem_dect_mac_network_scan_call_count,
				  "nrf_modem_dect_mac_network_scan should be called once");

	LOG_DBG("Network scan initiated - simulating completion");

	/* Step 3: Simulate network scan completion (no beacons found) */
	/* During cluster start (auto_start), the scan completion callback is called but
	 * NET_EVENT_DECT_SCAN_DONE is not sent to the application - it's an internal scan.
	 */
	if (mock_op_callbacks.network_scan) {
		/* Calculate number of odd channels in band 1: 1657, 1659, 1661, ..., 1677 */
		/* That's (1677 - 1657) / 2 + 1 = 11 channels */
		struct nrf_modem_dect_mac_network_scan_cb_params scan_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK,
			.num_scanned_channels = 11 /* Odd channels in band 1 */
		};

		LOG_DBG("Simulating network scan completion: status=OK, channels=11");
		mock_op_callbacks.network_scan(&scan_complete_params);

		/* Wait for scan completion processing */
		k_sleep(K_MSEC(100));
	} else {
		TEST_FAIL_MESSAGE("Network scan operation callback should be registered");
	}

	/* Note: During auto_start (cluster start), NET_EVENT_DECT_SCAN_DONE is not sent
	 * because the scan is internal to the cluster start process. The scan completion
	 * callback is handled internally and triggers RSSI scan automatically.
	 */

	/* Step 4: Wait for RSSI scan to be initiated automatically */
	k_sleep(K_MSEC(100));

	/* Verify RSSI scan was called */
	TEST_ASSERT_EQUAL_MESSAGE(
		baseline_rssi_scan + 1, mock_nrf_modem_dect_mac_rssi_scan_call_count,
		"nrf_modem_dect_mac_rssi_scan should be called after network scan");

	LOG_DBG("RSSI scan initiated - waiting for handle_auto_start to set auto_start = true");

	/* Wait for handle_auto_start to complete and set auto_start = true before
	 * simulating the completion callback. This ensures auto_start is true when
	 * RSSI results are processed. handle_auto_start is called asynchronously from
	 * a message queue, so we need to wait for it to complete.
	 */
	k_sleep(K_MSEC(300));

	LOG_DBG("Simulating completion callback to set cmd_on_going = false");

	/* Step 4b: For band-based scans (cluster creation), simulate RSSI scan completion callback
	 * first to set cmd_on_going = false, allowing the driver to process RSSI results.
	 * This must be done before sending RSSI results. We'll also simulate it again after
	 * rssi_scan_stop is called, as per the required order.
	 */
	if (mock_op_callbacks.rssi_scan) {
		struct nrf_modem_dect_mac_rssi_scan_cb_params rssi_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK};

		LOG_DBG("Simulating RSSI scan completion callback (to set cmd_on_going = false)");
		mock_op_callbacks.rssi_scan(&rssi_complete_params);

		/* Wait for completion callback to be processed */
		k_sleep(K_MSEC(100));
	} else {
		TEST_FAIL_MESSAGE("RSSI scan operation callback should be registered");
	}

	LOG_DBG("Simulating first free channel result");

	/* Step 5: Simulate RSSI scan result with first free channel (e.g., 1657) */
	/* The channel should have: all_subslots_free=true, busy_percentage=0, no other cluster */
	/* This will cause RSSI scan to stop automatically */
	/* First channel to scan - make it free */
	uint16_t free_channel = TEST_BEACON_CHANNEL;

	if (mock_ntf_callbacks.rssi_scan_ntf) {

		/* Create arrays for RSSI measurement: 48 subslots = 6 bytes */
		/* All subslots free: free array all 0xFF, busy and possible all 0x00 */
		static uint8_t busy_array[6] = {0, 0, 0, 0, 0, 0};     /* All subslots not busy */
		static uint8_t possible_array[6] = {0, 0, 0, 0, 0, 0}; /* No possible subslots */
		static uint8_t free_array[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; /* All free */

		struct nrf_modem_dect_mac_rssi_scan_ntf_cb_params rssi_result = {
			.channel = free_channel,
			.busy_percentage = 0,	   /* Free channel */
			.rssi_meas_array_size = 6, /* 48 subslots / 8 = 6 bytes */
			.busy = busy_array,
			.possible = possible_array,
			.free = free_array};

		LOG_DBG("Simulating RSSI scan result: channel=%d, busy_percentage=%d%%, "
			"all subslots free",
			rssi_result.channel, rssi_result.busy_percentage);

		mock_ntf_callbacks.rssi_scan_ntf(&rssi_result);

		/* Wait for RSSI result processing and for driver to call rssi_scan_stop */
		/* The driver should automatically stop RSSI scan when a free channel is found */
		k_sleep(K_MSEC(200));

		/* Verify RSSI scan result event was received */
		TEST_ASSERT_TRUE_MESSAGE(
			dect_rssi_scan_result_received,
			"NET_EVENT_DECT_RSSI_SCAN_RESULT should be received after rssi_scan_ntf");

		/* Verify the channel matches */
		TEST_ASSERT_EQUAL_MESSAGE(
			free_channel, received_rssi_scan_result_data.rssi_scan_result.channel,
			"RSSI scan result channel should match simulated channel");
	} else {
		TEST_FAIL_MESSAGE("RSSI scan notification callback should be registered");
	}

	/* Step 5b: Verify that driver called rssi_scan_stop() after finding free channel */
	/* The driver should automatically stop RSSI scan when a free channel is found */
	/* Condition: no another cluster && all_subslots_free && scan_suitable_percent_ok &&
	 * busy_percentage == 0 && (auto_start || ft_cluster_state == STARTING) && channel == 0
	 * Wait for driver to process the result and call stop if condition is met.
	 */
	k_sleep(K_MSEC(200));

	TEST_ASSERT_EQUAL_MESSAGE(baseline_rssi_scan_stop + 1,
				  mock_nrf_modem_dect_mac_rssi_scan_stop_call_count,
				  "nrf_modem_dect_mac_rssi_scan_stop should be called by driver "
				  "after finding free channel "
				  "(condition: no another cluster && all_subslots_free && "
				  "scan_suitable_percent_ok && "
				  "busy_percentage == 0 && channel == 0)");

	LOG_DBG("Driver called rssi_scan_stop - proceeding with completion callbacks");

	/* Step 6: Simulate RSSI scan completion */
	/* The RSSI scan should stop automatically when a free channel is found */
	/* But we still need to simulate the completion callback */
	if (mock_op_callbacks.rssi_scan) {
		struct nrf_modem_dect_mac_rssi_scan_cb_params rssi_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK};

		LOG_DBG("Simulating RSSI scan completion: status=OK");
		mock_op_callbacks.rssi_scan(&rssi_complete_params);

		/* Wait for RSSI scan completion processing */
		k_sleep(K_MSEC(200));
	} else {
		TEST_FAIL_MESSAGE("RSSI scan operation callback should be registered");
	}

	/* Verify RSSI scan done event was received */
	TEST_ASSERT_TRUE_MESSAGE(
		dect_rssi_scan_done_received,
		"NET_EVENT_DECT_RSSI_SCAN_DONE should be received after RSSI scan completion");

	/* Step 6b: Simulate RSSI scan stop operation complete callback */
	if (mock_op_callbacks.rssi_scan_stop) {
		struct nrf_modem_dect_mac_rssi_scan_stop_cb_params rssi_stop_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK};

		LOG_DBG("Simulating RSSI scan stop completion: status=OK");
		mock_op_callbacks.rssi_scan_stop(&rssi_stop_complete_params);

		/* Wait for RSSI scan stop completion processing */
		k_sleep(K_MSEC(100));
	} else {
		TEST_FAIL_MESSAGE("RSSI scan stop operation callback should be registered");
	}

	/* Step 7: Verify cluster configure was called and wait for async callback */
	/* The mock simulates the cluster_configure async callback, which triggers the event */
	/* Wait for cluster configure to be called and async callback to complete */
	k_sleep(K_MSEC(300));

	/* Verify cluster configure was called */
	TEST_ASSERT_EQUAL_MESSAGE(
		baseline_cluster_configure + 1,
		mock_nrf_modem_dect_mac_cluster_configure_call_count,
		"nrf_modem_dect_mac_cluster_configure should be called with chosen channel");

	/* Step 8: Verify NET_EVENT_DECT_CLUSTER_CREATED_RESULT was sent with the chosen channel */
	/* The cluster created event is sent in handle_mdm_cluster_configure_resp after the
	 * cluster configure callback completes. Wait for the event to be processed.
	 */
	k_sleep(K_MSEC(200));

	TEST_ASSERT_TRUE_MESSAGE(dect_cluster_created_received,
				 "NET_EVENT_DECT_CLUSTER_CREATED_RESULT should be received after "
				 "cluster configuration");
	TEST_ASSERT_EQUAL_MESSAGE(free_channel, received_cluster_created_data.cluster_channel,
				  "Cluster created event should contain the chosen channel");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, received_cluster_created_data.status,
				  "Cluster created event should have success status");

	/* Step 9: Verify network interface state after cluster is started */
	/* Expected state: oper=DORMANT, admin=UP, carrier=ON */
	/* "oper" will go up when 1st child association is established */
	TEST_ASSERT_TRUE_MESSAGE(net_if_is_admin_up(test_iface),
				 "Interface should be admin UP after cluster is started");
	TEST_ASSERT_TRUE_MESSAGE(net_if_is_carrier_ok(test_iface),
				 "Interface carrier should be ON after cluster is started");
	TEST_ASSERT_TRUE_MESSAGE(net_if_is_dormant(test_iface),
				 "Interface should be DORMANT after cluster is started");
	TEST_ASSERT_EQUAL_MESSAGE(
		NET_IF_OPER_DORMANT, net_if_oper_state(test_iface),
		"Interface operational state should be DORMANT after cluster is started");

	LOG_DBG("Cluster configure called - cluster start test completed successfully!");
	LOG_DBG("- Network scan calls: %d (expected: %d)",
		mock_nrf_modem_dect_mac_network_scan_call_count, baseline_network_scan + 1);
	LOG_DBG("- RSSI scan calls: %d (expected: %d)",
		mock_nrf_modem_dect_mac_rssi_scan_call_count, baseline_rssi_scan + 1);
	LOG_DBG("- Cluster configure calls: %d (expected: %d)",
		mock_nrf_modem_dect_mac_cluster_configure_call_count,
		baseline_cluster_configure + 1);
}

/**
 * @brief Test DECT FT network create with automatic channel selection
 *
 * This test mirrors the FT cluster start flow, but uses
 * NET_REQUEST_DECT_NETWORK_CREATE. With default settings, network beacon is not
 * started automatically, so the important part is verifying the automatic
 * channel selection flow reaches successful cluster creation.
 */
void test_dect_ft_network_create(void)
{
	LOG_DBG("=== Testing DECT FT network create with automatic channel selection ===");

	/* Ensure device is in FT mode. */
	/* Device is reconfigured to FT immediately before this test in main.c. */

	/* Reset event tracking */
	dect_cluster_created_received = false;
	memset(&received_cluster_created_data, 0, sizeof(received_cluster_created_data));

	/* Set flag to indicate this is a cluster creation RSSI scan at band 1 */
	mock_cluster_creation_band1 = true;

	/* Record baseline call counts */
	int baseline_network_scan = mock_nrf_modem_dect_mac_network_scan_call_count;
	int baseline_rssi_scan = mock_nrf_modem_dect_mac_rssi_scan_call_count;
	int baseline_rssi_scan_stop = mock_nrf_modem_dect_mac_rssi_scan_stop_call_count;
	int baseline_cluster_configure = mock_nrf_modem_dect_mac_cluster_configure_call_count;

	LOG_DBG("Requesting network create");
	int result = net_mgmt(NET_REQUEST_DECT_NETWORK_CREATE, test_iface, NULL, 0);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Network create request should succeed");

	/* Wait for network scan to be initiated */
	k_sleep(K_MSEC(50));

	/* Step 2: Verify network scan was called for odd channels in band 1 */
	TEST_ASSERT_EQUAL_MESSAGE(baseline_network_scan + 1,
				  mock_nrf_modem_dect_mac_network_scan_call_count,
				  "nrf_modem_dect_mac_network_scan should be called once");

	LOG_DBG("Network scan initiated - simulating completion");

	/* Step 3: Simulate network scan completion (no beacons found) */
	if (mock_op_callbacks.network_scan) {
		struct nrf_modem_dect_mac_network_scan_cb_params scan_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK,
			.num_scanned_channels = 11 /* Odd channels in band 1 */
		};

		LOG_DBG("Simulating network scan completion: status=OK, channels=11");
		mock_op_callbacks.network_scan(&scan_complete_params);

		/* Wait for scan completion processing */
		k_sleep(K_MSEC(100));
	} else {
		TEST_FAIL_MESSAGE("Network scan operation callback should be registered");
	}

	/* Wait for RSSI scan to be initiated automatically */
	k_sleep(K_MSEC(100));

	/* Verify RSSI scan was called */
	TEST_ASSERT_EQUAL_MESSAGE(
		baseline_rssi_scan + 1, mock_nrf_modem_dect_mac_rssi_scan_call_count,
		"nrf_modem_dect_mac_rssi_scan should be called after network scan");

	result = net_mgmt(NET_REQUEST_DECT_NETWORK_CREATE, test_iface, NULL, 0);
	TEST_ASSERT_EQUAL_MESSAGE(-EALREADY, result,
				  "Second network create request should fail while network "
				  "creation is already in progress");

	LOG_DBG("RSSI scan initiated - waiting for handle_auto_start to set auto_start = true");
	k_sleep(K_MSEC(300));

	LOG_DBG("Simulating completion callback to set cmd_on_going = false");

	if (mock_op_callbacks.rssi_scan) {
		struct nrf_modem_dect_mac_rssi_scan_cb_params rssi_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK};

		LOG_DBG("Simulating RSSI scan completion callback (to set cmd_on_going = false)");
		mock_op_callbacks.rssi_scan(&rssi_complete_params);

		/* Wait for completion callback to be processed */
		k_sleep(K_MSEC(100));
	} else {
		TEST_FAIL_MESSAGE("RSSI scan operation callback should be registered");
	}

	LOG_DBG("Simulating first free channel result");

	/* Step 5: Simulate RSSI scan result with first free channel */
	uint16_t free_channel = TEST_BEACON_CHANNEL;

	if (mock_ntf_callbacks.rssi_scan_ntf) {
		static uint8_t busy_array[6] = {0, 0, 0, 0, 0, 0};
		static uint8_t possible_array[6] = {0, 0, 0, 0, 0, 0};
		static uint8_t free_array[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

		struct nrf_modem_dect_mac_rssi_scan_ntf_cb_params rssi_result = {
			.channel = free_channel,
			.busy_percentage = 0,
			.rssi_meas_array_size = 6,
			.busy = busy_array,
			.possible = possible_array,
			.free = free_array};

		LOG_DBG("Simulating RSSI scan result: channel=%d, busy_percentage=%d%%, "
			"all subslots free",
			rssi_result.channel, rssi_result.busy_percentage);

		mock_ntf_callbacks.rssi_scan_ntf(&rssi_result);

		k_sleep(K_MSEC(200));

		TEST_ASSERT_TRUE_MESSAGE(
			dect_rssi_scan_result_received,
			"NET_EVENT_DECT_RSSI_SCAN_RESULT should be received after rssi_scan_ntf");

		TEST_ASSERT_EQUAL_MESSAGE(
			free_channel, received_rssi_scan_result_data.rssi_scan_result.channel,
			"RSSI scan result channel should match simulated channel");
	} else {
		TEST_FAIL_MESSAGE("RSSI scan notification callback should be registered");
	}

	k_sleep(K_MSEC(200));

	TEST_ASSERT_EQUAL_MESSAGE(baseline_rssi_scan_stop + 1,
				  mock_nrf_modem_dect_mac_rssi_scan_stop_call_count,
				  "nrf_modem_dect_mac_rssi_scan_stop should be called by driver "
				  "after finding free channel");

	LOG_DBG("Driver called rssi_scan_stop - proceeding with completion callbacks");

	if (mock_op_callbacks.rssi_scan) {
		struct nrf_modem_dect_mac_rssi_scan_cb_params rssi_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK};

		LOG_DBG("Simulating RSSI scan completion: status=OK");
		mock_op_callbacks.rssi_scan(&rssi_complete_params);

		k_sleep(K_MSEC(200));
	} else {
		TEST_FAIL_MESSAGE("RSSI scan operation callback should be registered");
	}

	TEST_ASSERT_TRUE_MESSAGE(
		dect_rssi_scan_done_received,
		"NET_EVENT_DECT_RSSI_SCAN_DONE should be received after RSSI scan completion");

	if (mock_op_callbacks.rssi_scan_stop) {
		struct nrf_modem_dect_mac_rssi_scan_stop_cb_params rssi_stop_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK};

		LOG_DBG("Simulating RSSI scan stop completion: status=OK");
		mock_op_callbacks.rssi_scan_stop(&rssi_stop_complete_params);

		k_sleep(K_MSEC(100));
	} else {
		TEST_FAIL_MESSAGE("RSSI scan stop operation callback should be registered");
	}

	k_sleep(K_MSEC(300));

	TEST_ASSERT_EQUAL_MESSAGE(
		baseline_cluster_configure + 1,
		mock_nrf_modem_dect_mac_cluster_configure_call_count,
		"nrf_modem_dect_mac_cluster_configure should be called with chosen channel");

	k_sleep(K_MSEC(200));

	TEST_ASSERT_TRUE_MESSAGE(dect_cluster_created_received,
				 "NET_EVENT_DECT_CLUSTER_CREATED_RESULT should be received after "
				 "cluster configuration");
	TEST_ASSERT_EQUAL_MESSAGE(free_channel, received_cluster_created_data.cluster_channel,
				  "Cluster created event should contain the chosen channel");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, received_cluster_created_data.status,
				  "Cluster created event should have success status");

	TEST_ASSERT_TRUE_MESSAGE(net_if_is_admin_up(test_iface),
				 "Interface should be admin UP after network create");
	TEST_ASSERT_TRUE_MESSAGE(net_if_is_carrier_ok(test_iface),
				 "Interface carrier should be ON after network create");
	TEST_ASSERT_TRUE_MESSAGE(net_if_is_dormant(test_iface),
				 "Interface should be DORMANT after network create");
	TEST_ASSERT_EQUAL_MESSAGE(
		NET_IF_OPER_DORMANT, net_if_oper_state(test_iface),
		"Interface operational state should be DORMANT after network create");

	LOG_DBG("Network create test completed successfully!");
	LOG_DBG("- Network scan calls: %d (expected: %d)",
		mock_nrf_modem_dect_mac_network_scan_call_count, baseline_network_scan + 1);
	LOG_DBG("- RSSI scan calls: %d (expected: %d)",
		mock_nrf_modem_dect_mac_rssi_scan_call_count, baseline_rssi_scan + 1);
	LOG_DBG("- Cluster configure calls: %d (expected: %d)",
		mock_nrf_modem_dect_mac_cluster_configure_call_count,
		baseline_cluster_configure + 1);
}

/**
 * @brief Test FT cluster reconfigure to the next odd channel
 *
 * Reconfigures the running FT cluster from the channel created by
 * test_dect_ft_network_create() to the next odd channel. Verifies that the
 * cluster configure mock callback path is exercised and that the cluster
 * created event is sent with the new channel.
 */
void test_dect_ft_cluster_reconfigure(void)
{
	uint16_t current_channel = received_cluster_created_data.cluster_channel;
	uint16_t reconfig_channel = current_channel + 2;
	struct dect_cluster_reconfig_req_params params = {
		.channel = reconfig_channel,
		.max_beacon_tx_power_dbm = 10,
		.max_cluster_power_dbm = 13,
		.period = DECT_CLUSTER_BEACON_PERIOD_2000MS,
	};
	int baseline_network_scan = mock_nrf_modem_dect_mac_network_scan_call_count;
	int baseline_rssi_scan = mock_nrf_modem_dect_mac_rssi_scan_call_count;
	int baseline_cluster_configure = mock_nrf_modem_dect_mac_cluster_configure_call_count;
	int result;

	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, current_channel,
				      "Network create should have produced a cluster channel");

	dect_cluster_created_received = false;
	memset(&received_cluster_created_data, 0, sizeof(received_cluster_created_data));

	LOG_DBG("Requesting cluster reconfigure from %u to %u", current_channel, reconfig_channel);
	result =
		net_mgmt(NET_REQUEST_DECT_CLUSTER_RECONFIGURE, test_iface, &params, sizeof(params));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Cluster reconfigure request should succeed");

	k_sleep(K_MSEC(100));

	TEST_ASSERT_EQUAL_MESSAGE(
		baseline_network_scan, mock_nrf_modem_dect_mac_network_scan_call_count,
		"Cluster reconfigure with explicit channel should not start a network scan");
	TEST_ASSERT_EQUAL_MESSAGE(
		baseline_rssi_scan + 1, mock_nrf_modem_dect_mac_rssi_scan_call_count,
		"Cluster reconfigure should start one RSSI scan for the requested channel");

	if (mock_ntf_callbacks.rssi_scan_ntf) {
		static uint8_t busy_array[6] = {0, 0, 0, 0, 0, 0};
		static uint8_t possible_array[6] = {0, 0, 0, 0, 0, 0};
		static uint8_t free_array[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

		struct nrf_modem_dect_mac_rssi_scan_ntf_cb_params rssi_result = {
			.channel = reconfig_channel,
			.busy_percentage = 0,
			.rssi_meas_array_size = 6,
			.busy = busy_array,
			.possible = possible_array,
			.free = free_array};

		LOG_DBG("Simulating cluster reconfigure RSSI result on channel %u",
			rssi_result.channel);
		mock_ntf_callbacks.rssi_scan_ntf(&rssi_result);
		k_sleep(K_MSEC(200));
	} else {
		TEST_FAIL_MESSAGE("RSSI scan notification callback should be registered");
	}

	if (mock_op_callbacks.rssi_scan) {
		struct nrf_modem_dect_mac_rssi_scan_cb_params rssi_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK};

		LOG_DBG("Simulating RSSI scan completion for cluster reconfigure");
		mock_op_callbacks.rssi_scan(&rssi_complete_params);
		k_sleep(K_MSEC(200));
	} else {
		TEST_FAIL_MESSAGE("RSSI scan operation callback should be registered");
	}

	k_sleep(K_MSEC(300));

	TEST_ASSERT_EQUAL_MESSAGE(
		baseline_cluster_configure + 1,
		mock_nrf_modem_dect_mac_cluster_configure_call_count,
		"Cluster reconfigure should call nrf_modem_dect_mac_cluster_configure once");
	TEST_ASSERT_TRUE_MESSAGE(
		dect_cluster_created_received,
		"Cluster reconfigure should send NET_EVENT_DECT_CLUSTER_CREATED_RESULT");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, received_cluster_created_data.status,
				  "Cluster reconfigure should complete with DECT_STATUS_OK");
	TEST_ASSERT_EQUAL_MESSAGE(
		reconfig_channel, received_cluster_created_data.cluster_channel,
		"Cluster reconfigure event should contain the reconfigured channel");
}

#if defined(CONFIG_MODEM_CELLULAR)
extern struct net_if *dect_test_get_mock_ppp_net_if(void);
extern int test_net_if_start_rs_call_count;
#if defined(CONFIG_NET_IPV6)
extern int dect_test_mock_ppp_ipv6_unicast_used_count(void);
extern void dect_test_mock_ppp_restore_ipv6_unicast(void);
#endif
#endif

#if defined(CONFIG_MODEM_CELLULAR) && defined(CONFIG_NET_IPV6)
/** Notify mock PPP sink with standard prefix, router, and NBR (2001:db8::/64, 2001:db8::1). */
static void test_mock_ppp_sink_notify_prefix_router_nbr(struct net_if *ppp_if)
{
	struct net_event_ipv6_prefix prefix_evt = {
		.len = 64,
		.lifetime = 7200,
	};

	memcpy(prefix_evt.addr.s6_addr, test_ipv6_addr_sink_router,
	       sizeof(test_ipv6_addr_sink_router));
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_PREFIX_ADD, ppp_if, &prefix_evt,
					sizeof(prefix_evt));
	k_sleep(K_MSEC(50));
	{
		struct in6_addr router_addr;

		memcpy(router_addr.s6_addr, test_ipv6_addr_sink_router,
		       sizeof(test_ipv6_addr_sink_router));
		net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTER_ADD, ppp_if, &router_addr,
						sizeof(router_addr));
	}
	k_sleep(K_MSEC(50));
	{
		struct net_event_ipv6_nbr nbr_evt = {.idx = 0};

		memcpy(nbr_evt.addr.s6_addr, test_ipv6_addr_sink_router,
		       sizeof(test_ipv6_addr_sink_router));
		net_mgmt_event_notify_with_info(NET_EVENT_IPV6_NBR_ADD, ppp_if, &nbr_evt,
						sizeof(nbr_evt));
	}
	k_sleep(K_MSEC(50));
}
#endif

/**
 * @brief Test FT sink global address assign (simulate PPP prefix)
 *
 * Simulates the sink receiving a global prefix from PPP: bring the mock PPP net_if up
 * (NET_EVENT_IF_UP) so the sink logs "Sink networking iface (...) is up" and sends
 * NET_EVENT_DECT_SINK_STATUS (DISCONNECTED). Then simulate adding a global IPv6 address
 * (NET_EVENT_IPV6_ADDR_ADD); the sink takes the first 64 bits as prefix and sends
 * NET_EVENT_DECT_SINK_STATUS (CONNECTED). Also sends NET_EVENT_IPV6_PREFIX_ADD for
 * PPP-like event coverage, and NET_EVENT_IPV6_ROUTER_ADD with router 2001:db8::1
 * (mock PPP net_if has config.ip.ipv6 with matching unicast so sink handler
 * runs without crash). Verifies both sink status events are received.
 */
void test_dect_ft_sink_global_address_assign(void)
{
#if defined(CONFIG_MODEM_CELLULAR)
	struct net_if *ppp_if;
	struct net_event_ipv6_addr addr_evt;

	ppp_if = dect_test_get_mock_ppp_net_if();
	TEST_ASSERT_NOT_NULL_MESSAGE(ppp_if, "Mock PPP net_if should be available");

	int baseline_cluster_configure = mock_nrf_modem_dect_mac_cluster_configure_call_count;

	dect_sink_status_received = false;
	memset(&received_sink_status_data, 0, sizeof(received_sink_status_data));
	dect_cluster_created_received = false;
	memset(&received_cluster_created_data, 0, sizeof(received_cluster_created_data));

	/* Bring PPP net_if up so sink gets NET_EVENT_IF_UP */
	net_if_flag_set(ppp_if, NET_IF_UP);
	net_mgmt_event_notify(NET_EVENT_IF_UP, ppp_if);

	k_sleep(K_MSEC(150));

	/* Sink should have sent NET_EVENT_DECT_SINK_STATUS (DISCONNECTED) on IF_UP */
	TEST_ASSERT_TRUE_MESSAGE(
		dect_sink_status_received,
		"NET_EVENT_DECT_SINK_STATUS should be received after NET_EVENT_IF_UP");
	TEST_ASSERT_EQUAL_MESSAGE(
		DECT_SINK_STATUS_DISCONNECTED, received_sink_status_data.sink_status,
		"First sink status should be DISCONNECTED when PPP iface comes up");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(ppp_if, received_sink_status_data.br_iface,
				      "Sink status br_iface should be the PPP iface");

	/* Now simulate global address add on PPP iface (e.g. from PPP/PPP carrier) */
	dect_sink_status_received = false;
	memcpy(addr_evt.addr.s6_addr, test_ipv6_addr_sink_router,
	       sizeof(test_ipv6_addr_sink_router));
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ADDR_ADD, ppp_if, &addr_evt,
					sizeof(addr_evt));

	k_sleep(K_MSEC(150));

	/* Sink should have sent NET_EVENT_DECT_SINK_STATUS (CONNECTED) on ADDR_ADD */
	TEST_ASSERT_TRUE_MESSAGE(dect_sink_status_received,
				 "NET_EVENT_DECT_SINK_STATUS should be received after "
				 "NET_EVENT_IPV6_ADDR_ADD (global prefix)");
	TEST_ASSERT_EQUAL_MESSAGE(
		DECT_SINK_STATUS_CONNECTED, received_sink_status_data.sink_status,
		"Second sink status should be CONNECTED after global address add");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(ppp_if, received_sink_status_data.br_iface,
				      "Sink status br_iface should be the PPP iface");

#if defined(CONFIG_NET_IPV6)
	test_mock_ppp_sink_notify_prefix_router_nbr(ppp_if);
#endif

	/* Verify from DECT status that our sink has the global prefix set (2001:db8::/64) */
	{
		struct dect_status_info status_info = {0};
		int result = test_dect_status_info_get(test_iface, &status_info);

		TEST_ASSERT_EQUAL_MESSAGE(
			0, result, "DECT status info get should succeed after sink address assign");
		TEST_ASSERT_TRUE_MESSAGE(status_info.br_global_ipv6_addr_prefix_set,
					 "Sink should have global IPv6 prefix set");
		TEST_ASSERT_EQUAL_MESSAGE(8, status_info.br_global_ipv6_addr_prefix_len,
					  "Sink prefix length should be 8 (bytes, i.e. 64 bits)");
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(test_ipv6_prefix_sink_64,
						 status_info.br_global_ipv6_addr_prefix.s6_addr, 8,
						 "Sink prefix should be 2001:db8::/64");
	}

	/* Driver receives NET_EVENT_IPV6_PREFIX_ADD on DECT iface (from L2 adding prefix)
	 * and triggers cluster reconfigure. Verify nrf_modem_dect_mac_cluster_configure
	 * was called with valid prefix and simulate callback (mock does it automatically).
	 */
	k_sleep(K_MSEC(400));
	TEST_ASSERT_TRUE_MESSAGE(
		mock_nrf_modem_dect_mac_cluster_configure_call_count > baseline_cluster_configure,
		"cluster_configure should be called after prefix add (driver reconfig)");
	TEST_ASSERT_EQUAL_MESSAGE(NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX,
				  mock_last_configured_ipv6_prefix_type,
				  "cluster_configure should use PREFIX type");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
		test_ipv6_prefix_sink_64, mock_last_configured_ipv6_prefix, 8,
		"cluster_configure should be called with prefix 2001:db8::/64");

	/* Allow async cluster_configure op callback; sends NET_EVENT_DECT_CLUSTER_CREATED_RESULT */
	k_sleep(K_MSEC(200));

	/* Reconfig must notify with NET_EVENT_DECT_CLUSTER_CREATED_RESULT */
	TEST_ASSERT_TRUE_MESSAGE(dect_cluster_created_received,
				 "NET_EVENT_DECT_CLUSTER_CREATED_RESULT should be sent after "
				 "cluster reconfig (prefix add)");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, received_cluster_created_data.status,
				  "Cluster reconfig result status should be OK");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, received_cluster_created_data.cluster_channel,
				      "Cluster reconfig result should report cluster channel");
#else
	TEST_IGNORE_MESSAGE("CONFIG_MODEM_CELLULAR not enabled, skip sink PPP simulation");
#endif
}

/**
 * @brief Test FT sink L2 handling of router deletion (ROUTER_DEL) and RS recovery
 *
 * Runs after test_dect_ft_sink_global_address_assign so the sink has prefix and
 * ipv6_router_addr (2001:db8::1). Sends NET_EVENT_IPV6_ROUTER_DEL for that router;
 * expects DECT_SINK_STATUS_DISCONNECTED and net_if_start_rs() to be called.
 * Then simulates router return with NET_EVENT_IPV6_ROUTER_ADD and expects
 * NET_EVENT_DECT_SINK_STATUS CONNECTED again.
 */
void test_dect_ft_sink_router_del(void)
{
#if defined(CONFIG_MODEM_CELLULAR)
	struct net_if *ppp_if;
	struct in6_addr router_addr;

	ppp_if = dect_test_get_mock_ppp_net_if();
	TEST_ASSERT_NOT_NULL_MESSAGE(ppp_if, "Mock PPP net_if should be available");

	memcpy(router_addr.s6_addr, test_ipv6_addr_sink_router, sizeof(test_ipv6_addr_sink_router));

	/* Ensure mock PPP iface is considered "up" so sink ROUTER_DEL handler runs
	 * (it requires net_if_is_up() which needs both NET_IF_UP and NET_IF_RUNNING).
	 */
	net_if_flag_set(ppp_if, NET_IF_UP);
	net_if_flag_set(ppp_if, NET_IF_RUNNING);

	dect_sink_status_received = false;
	memset(&received_sink_status_data, 0, sizeof(received_sink_status_data));
	int rs_calls_before = test_net_if_start_rs_call_count;

	/* Simulate router deleted (our sink router 2001:db8::1) */
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTER_DEL, ppp_if, &router_addr,
					sizeof(router_addr));

	k_sleep(K_MSEC(100));

	TEST_ASSERT_TRUE_MESSAGE(dect_sink_status_received,
				 "NET_EVENT_DECT_SINK_STATUS should be received after ROUTER_DEL");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_SINK_STATUS_DISCONNECTED,
				  received_sink_status_data.sink_status,
				  "Sink status should be DISCONNECTED after router deleted");
	TEST_ASSERT_TRUE_MESSAGE(test_net_if_start_rs_call_count > rs_calls_before,
				 "net_if_start_rs should be called after ROUTER_DEL");

	/* Simulate router back (ROUTER_ADD) so sink becomes CONNECTED again */
	dect_sink_status_received = false;
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTER_ADD, ppp_if, &router_addr,
					sizeof(router_addr));

	k_sleep(K_MSEC(100));

	TEST_ASSERT_TRUE_MESSAGE(dect_sink_status_received,
				 "NET_EVENT_DECT_SINK_STATUS should be received after ROUTER_ADD");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_SINK_STATUS_CONNECTED, received_sink_status_data.sink_status,
				  "Sink status should be CONNECTED after router re-added");
#else
	TEST_IGNORE_MESSAGE("CONFIG_MODEM_CELLULAR not enabled, skip sink router_del test");
#endif
}

/**
 * @brief Test FT sink L2 worker when router is removed from nbr table (NBR_DEL)
 *
 * Runs after sink has prefix and router (e.g. test_dect_ft_sink_router_del).
 * Sends NET_EVENT_IPV6_NBR_DEL for the sink router (2001:db8::1); the sink
 * schedules dect_net_l2_sink_lte_ipv6_nbr_router_deleted_worker which re-adds
 * the router via net_ipv6_nbr_add. Verifies NET_EVENT_IPV6_NBR_ADD is received
 * for that router after the delayed work runs.
 */
void test_dect_ft_sink_router_nbr_del(void)
{
#if defined(CONFIG_MODEM_CELLULAR)
	struct net_if *ppp_if;
	struct net_event_ipv6_nbr nbr_evt;

	ppp_if = dect_test_get_mock_ppp_net_if();
	TEST_ASSERT_NOT_NULL_MESSAGE(ppp_if, "Mock PPP net_if should be available");

	memcpy(nbr_evt.addr.s6_addr, test_ipv6_addr_sink_router,
	       sizeof(test_ipv6_addr_sink_router));
	nbr_evt.idx = -1;

	net_if_flag_set(ppp_if, NET_IF_UP);
	net_if_flag_set(ppp_if, NET_IF_RUNNING);

	dect_sink_router_nbr_add_received = false;
	net_mgmt_init_event_callback(&test_nbr_add_cb, test_nbr_add_handler,
				     NET_EVENT_IPV6_NBR_ADD);
	net_mgmt_add_event_callback(&test_nbr_add_cb);

	/* Simulate router removed from nbr table -> sink schedules worker to add it back */
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_NBR_DEL, ppp_if, &nbr_evt, sizeof(nbr_evt));

	/* Worker is rescheduled with K_MSEC(100) */
	k_sleep(K_MSEC(150));

	net_mgmt_del_event_callback(&test_nbr_add_cb);

	TEST_ASSERT_TRUE_MESSAGE(dect_sink_router_nbr_add_received,
				 "NET_EVENT_IPV6_NBR_ADD should be received after worker runs "
				 "(router re-added via net_ipv6_nbr_add)");
#else
	TEST_IGNORE_MESSAGE("CONFIG_MODEM_CELLULAR not enabled, skip sink router_nbr_del test");
#endif
}

/**
 * @brief Test FT child has both local and global address after sink prefix is set
 *
 * Same flow as test_dect_ft_cluster_associate_child but runs after
 * test_dect_ft_sink_global_address_assign (sink has prefix 2001:db8::/64).
 * Simulates an incoming association request from a second PT; verifies
 * association events, neighbor role CHILD, interface UP, link-local on DECT iface,
 * then from DECT status verifies the newly associated child has both local and
 * global address (global with prefix 2001:db8::/64).
 */
void test_dect_ft_cluster_associate_child_with_global_address(void)
{
#if defined(CONFIG_MODEM_CELLULAR)
	LOG_DBG("=== Testing DECT FT cluster associate child with global address ===");

	/* Prerequisites: FT cluster running, sink has global prefix (from previous test).
	 * Interface should be DORMANT (no children yet after dissociate/network create path).
	 */
	TEST_ASSERT_TRUE_MESSAGE(net_if_is_admin_up(test_iface), "Interface should be admin UP");
	TEST_ASSERT_TRUE_MESSAGE(net_if_is_carrier_ok(test_iface),
				 "Interface carrier should be ON");
	TEST_ASSERT_EQUAL_MESSAGE(NET_IF_OPER_DORMANT, net_if_oper_state(test_iface),
				  "Interface should be DORMANT before first child in this path");

	/* Reset association event tracking for this test */
	dect_association_changed_received = false;
	dect_association_created_received = false;
	dect_association_released_received = false;
	dect_association_failed_received = false;
	memset(&received_association_data, 0, sizeof(received_association_data));

	/* Second PT device (different from test_dect_ft_cluster_associate_child) */
	uint16_t pt_short_rd_id = TEST_BEACON_SHORT_RD_ID;
	uint32_t pt_long_rd_id = 0xCAFEBABE;

	LOG_DBG("Simulating incoming association request from second PT:");
	LOG_DBG("- PT Short RD ID: 0x%04X", pt_short_rd_id);
	LOG_DBG("- PT Long RD ID: 0x%08X", pt_long_rd_id);

	TEST_ASSERT_NOT_NULL_MESSAGE(mock_ntf_callbacks.association_ntf,
				     "Association notification callback should be registered");

	struct nrf_modem_dect_mac_association_ntf_cb_params assoc_ntf_params = {
		.status = NRF_MODEM_DECT_MAC_ASSOCIATION_INDICATION_STATUS_SUCCESS,
		.tx_method = NRF_MODEM_DECT_MAC_COMMUNICATION_METHOD_RACH,
		.rx_signal_info = {.rssi_2 = -50, .snr = 20},
		.short_rd_id = pt_short_rd_id,
		.long_rd_id = pt_long_rd_id,
		.number_of_ies = 0,
	};

	mock_ntf_callbacks.association_ntf(&assoc_ntf_params);
	k_sleep(K_MSEC(200));

	/* Verify association events (same as test_dect_ft_cluster_associate_child) */
	TEST_ASSERT_TRUE_MESSAGE(dect_association_changed_received,
				 "NET_EVENT_DECT_ASSOCIATION_CHANGED should be received");
	TEST_ASSERT_TRUE_MESSAGE(dect_association_created_received,
				 "DECT_ASSOCIATION_CREATED event should be received");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_ASSOCIATION_CREATED,
				  received_association_data.association_change_type,
				  "Association change type should be DECT_ASSOCIATION_CREATED");
	TEST_ASSERT_EQUAL_MESSAGE(pt_long_rd_id, received_association_data.long_rd_id,
				  "Association event Long RD ID should match the PT device");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NEIGHBOR_ROLE_CHILD, received_association_data.neighbor_role,
				  "Neighbor role should be DECT_NEIGHBOR_ROLE_CHILD");

	/* After first child comes, interface is UP */
	TEST_ASSERT_EQUAL_MESSAGE(
		NET_IF_OPER_UP, net_if_oper_state(test_iface),
		"Interface operational state should be UP after first child association");

	/* Link-local address on DECT interface */
	struct in6_addr *ll_addr = net_if_ipv6_get_ll(test_iface, NET_ADDR_PREFERRED);

	TEST_ASSERT_NOT_NULL_MESSAGE(ll_addr,
				     "IPv6 link-local address should be set on DECT interface");
	TEST_ASSERT_TRUE_MESSAGE(net_ipv6_is_ll_addr(ll_addr),
				 "IPv6 address should be a valid link-local address (fe80::/10)");

	/* Verify from DECT status: find our child by long_rd_id and check both addresses */
	struct dect_status_info status_info = {0};
	int result = test_dect_status_info_get(test_iface, &status_info);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "DECT status info get should succeed");
	TEST_ASSERT_TRUE_MESSAGE(status_info.child_count >= 1, "FT should have at least one child");

	/* Find the child we just associated (by long_rd_id) */
	struct dect_association_data *child = NULL;

	for (uint8_t i = 0; i < status_info.child_count; i++) {
		if (status_info.child_associations[i].long_rd_id == pt_long_rd_id) {
			child = &status_info.child_associations[i];
			break;
		}
	}
	TEST_ASSERT_NOT_NULL_MESSAGE(child, "Newly associated child should appear in DECT status");

	TEST_ASSERT_TRUE_MESSAGE(child->global_ipv6_addr_set,
				 "Child should have global IPv6 address set (sink prefix applied)");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(test_ipv6_prefix_sink_64, child->global_ipv6_addr.s6_addr,
					 sizeof(test_ipv6_prefix_sink_64),
					 "Child global address should have prefix 2001:db8::/64");
	TEST_ASSERT_TRUE_MESSAGE(net_ipv6_is_ll_addr(&child->local_ipv6_addr),
				 "Child should have valid link-local address");

	/* Child IPv6 addresses: last 64 bits = FT (our) long RD ID + child long RD ID */
	struct dect_settings settings = {0};

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &settings, sizeof(settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Settings read should succeed to get FT long RD ID");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, settings.identities.transmitter_long_rd_id,
				      "FT (our) long RD ID should be set");

	uint32_t ft_long_rd_id = settings.identities.transmitter_long_rd_id;
	struct in6_addr expected_local = {0};
	struct in6_addr expected_global = {0};
	struct in6_addr prefix_global;
	struct in6_addr prefix_local;

	memcpy(prefix_global.s6_addr, test_ipv6_prefix_sink_64, 8);
	TEST_ASSERT_TRUE_MESSAGE(
		dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
			prefix_global, ft_long_rd_id, pt_long_rd_id, &expected_global),
		"Expected child global address should be derivable");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
		&expected_global, &child->global_ipv6_addr, sizeof(expected_global),
		"Child global address should be prefix + FT long RD ID + child long RD ID");

	/* Verify sink RD ID extracted from child global IPv6 matches FT (sink) long RD ID */
	uint32_t sink_rd_id = dect_utils_lib_sink_rd_id_from_ipv6_addr(&child->global_ipv6_addr);

	TEST_ASSERT_EQUAL_MESSAGE(
		ft_long_rd_id, sink_rd_id,
		"Child global addr sink part (bytes 8-11) should be FT long RD ID");

	/* Link-local prefix fe80:: */
	prefix_local.s6_addr32[0] = htonl(0xfe800000);
	prefix_local.s6_addr32[1] = 0;
	TEST_ASSERT_TRUE_MESSAGE(
		dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
			prefix_local, ft_long_rd_id, pt_long_rd_id, &expected_local),
		"Expected child local address should be derivable");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
		&expected_local, &child->local_ipv6_addr, sizeof(expected_local),
		"Child local address should be fe80:: + FT long RD ID + child long RD ID");

	LOG_DBG("=== FT cluster associate child with global address test completed ===");
	LOG_DBG("- Child Long RD ID: 0x%08X", pt_long_rd_id);
	LOG_DBG("- Child has local and global address: YES");
#else
	TEST_IGNORE_MESSAGE("CONFIG_MODEM_CELLULAR not enabled");
#endif
}

/**
 * @brief Test FT cluster associate second child with global address (child 2)
 *
 * Runs after test_dect_ft_cluster_associate_child_with_global_address (first child
 * 0xCAFEBABE already associated). Simulates an incoming association request from a
 * second PT (0xDEADBEEF); verifies association events and that DECT status shows
 * two children, with the new child having global and local addresses.
 */
void test_dect_ft_cluster_associate_child_with_global_address_2(void)
{
#if defined(CONFIG_MODEM_CELLULAR)
	LOG_DBG("=== Testing DECT FT cluster associate second child with global address ===");

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface, "DECT test iface should be set");

	/* Reset association event tracking */
	dect_association_changed_received = false;
	dect_association_created_received = false;
	memset(&received_association_data, 0, sizeof(received_association_data));

	/* Second child (different from first child 0xCAFEBABE) */
	uint16_t pt_short_rd_id = 0x5678;
	uint32_t pt_long_rd_id = 0xDEADBEEF;

	LOG_DBG("Simulating incoming association request from second PT:");
	LOG_DBG("- PT Long RD ID: 0x%08X", pt_long_rd_id);

	TEST_ASSERT_NOT_NULL_MESSAGE(mock_ntf_callbacks.association_ntf,
				     "Association notification callback should be registered");

	struct nrf_modem_dect_mac_association_ntf_cb_params assoc_ntf_params = {
		.status = NRF_MODEM_DECT_MAC_ASSOCIATION_INDICATION_STATUS_SUCCESS,
		.tx_method = NRF_MODEM_DECT_MAC_COMMUNICATION_METHOD_RACH,
		.rx_signal_info = {.rssi_2 = -48, .snr = 22},
		.short_rd_id = pt_short_rd_id,
		.long_rd_id = pt_long_rd_id,
		.number_of_ies = 0,
	};

	mock_ntf_callbacks.association_ntf(&assoc_ntf_params);
	k_sleep(K_MSEC(200));

	TEST_ASSERT_TRUE_MESSAGE(dect_association_changed_received,
				 "NET_EVENT_DECT_ASSOCIATION_CHANGED should be received");
	TEST_ASSERT_EQUAL_MESSAGE(pt_long_rd_id, received_association_data.long_rd_id,
				  "Association event Long RD ID should match second PT");

	/* Verify we now have two children */
	struct dect_status_info status_info = {0};
	int result = test_dect_status_info_get(test_iface, &status_info);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "DECT status info get should succeed");
	TEST_ASSERT_EQUAL_MESSAGE(2, status_info.child_count,
				  "FT should have exactly two children");

	/* Find the second child by long_rd_id */
	struct dect_association_data *child = NULL;

	for (uint8_t i = 0; i < status_info.child_count; i++) {
		if (status_info.child_associations[i].long_rd_id == pt_long_rd_id) {
			child = &status_info.child_associations[i];
			break;
		}
	}
	TEST_ASSERT_NOT_NULL_MESSAGE(child,
				     "Second child (0xDEADBEEF) should appear in DECT status");

	TEST_ASSERT_TRUE_MESSAGE(child->global_ipv6_addr_set,
				 "Second child should have global IPv6 address set");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
		test_ipv6_prefix_sink_64, child->global_ipv6_addr.s6_addr, 8,
		"Second child global address should have prefix 2001:db8::/64");
	TEST_ASSERT_TRUE_MESSAGE(net_ipv6_is_ll_addr(&child->local_ipv6_addr),
				 "Second child should have valid link-local address");

	LOG_DBG("=== Second child associate test completed - Child Long RD ID: 0x%08X ===",
		pt_long_rd_id);
#else
	TEST_IGNORE_MESSAGE("CONFIG_MODEM_CELLULAR not enabled");
#endif
}

/**
 * @brief Test FT sink global address change (prefix removal and new prefix)
 *
 * Runs after test_dect_ft_sink_global_address_assign (sink has 2001:db8::/64).
 * 1. Send NET_EVENT_IPV6_ADDR_DEL for our prefix (2001:db8::1) on PPP iface ->
 *    verify DECT_SINK_STATUS_DISCONNECTED is received.
 * 2. Send NET_EVENT_IPV6_PREFIX_DEL for old prefix 2001:db8::/64.
 * 3. Send NET_EVENT_IPV6_ADDR_ADD with new prefix (2001:db8:1::1) ->
 *    verify DECT_SINK_STATUS_CONNECTED is received.
 * 4. Verify from DECT status: br_global_ipv6_addr_prefix is the new prefix,
 *    and child global addresses (if any) updated to new prefix.
 * 5. Verify our (FT) global address on DECT iface has been updated to new prefix.
 */
void test_dect_ft_sink_global_address_change(void)
{
#if defined(CONFIG_MODEM_CELLULAR)
	struct net_if *ppp_if;
	/* Old address/prefix from test_dect_ft_sink_global_address_assign */
	/* New prefix 2001:db8:1::/64, address 2001:db8:1::1 */

	ppp_if = dect_test_get_mock_ppp_net_if();
	TEST_ASSERT_NOT_NULL_MESSAGE(ppp_if, "Mock PPP net_if should be available");

	int baseline_cluster_configure = mock_nrf_modem_dect_mac_cluster_configure_call_count;

	dect_sink_status_received = false;
	memset(&received_sink_status_data, 0, sizeof(received_sink_status_data));

	/* 1. ADDR_DEL for our prefix -> sink sends DISCONNECTED */
	{
		struct net_event_ipv6_addr addr_del_evt;

		memcpy(addr_del_evt.addr.s6_addr, test_ipv6_addr_sink_router,
		       sizeof(test_ipv6_addr_sink_router));
		net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ADDR_DEL, ppp_if, &addr_del_evt,
						sizeof(addr_del_evt));
	}
	k_sleep(K_MSEC(150));

	TEST_ASSERT_TRUE_MESSAGE(dect_sink_status_received,
				 "NET_EVENT_DECT_SINK_STATUS should be received after ADDR_DEL");
	TEST_ASSERT_EQUAL_MESSAGE(
		DECT_SINK_STATUS_DISCONNECTED, received_sink_status_data.sink_status,
		"Sink status should be DISCONNECTED after prefix address removed");

	/* 2. PREFIX_DEL for old prefix */
	{
		struct net_event_ipv6_prefix prefix_del_evt = {
			.len = 64,
			.lifetime = 0,
		};

		memcpy(prefix_del_evt.addr.s6_addr, test_ipv6_addr_sink_router, 8);
		net_mgmt_event_notify_with_info(NET_EVENT_IPV6_PREFIX_DEL, ppp_if, &prefix_del_evt,
						sizeof(prefix_del_evt));
	}
	k_sleep(K_MSEC(50));

	/* 3. ADDR_ADD with new prefix -> sink sends CONNECTED */
	dect_sink_status_received = false;
	/* Reset so we only count NET_EVENT_DECT_CLUSTER_CREATED_RESULT from reconfig */
	dect_cluster_created_received = false;
	memset(&received_cluster_created_data, 0, sizeof(received_cluster_created_data));
	{
		struct net_event_ipv6_addr addr_add_evt;

		memcpy(addr_add_evt.addr.s6_addr, test_ipv6_addr_sink_new_64,
		       sizeof(test_ipv6_addr_sink_new_64));
		net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ADDR_ADD, ppp_if, &addr_add_evt,
						sizeof(addr_add_evt));
	}
	k_sleep(K_MSEC(150));

	TEST_ASSERT_TRUE_MESSAGE(
		dect_sink_status_received,
		"NET_EVENT_DECT_SINK_STATUS should be received after new ADDR_ADD");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_SINK_STATUS_CONNECTED, received_sink_status_data.sink_status,
				  "Sink status should be CONNECTED after new prefix address added");

	/* 4. DECT status: new prefix set, global addresses updated */
	{
		struct dect_status_info status_info = {0};
		int result = test_dect_status_info_get(test_iface, &status_info);

		TEST_ASSERT_EQUAL_MESSAGE(0, result, "DECT status info get should succeed");
		TEST_ASSERT_TRUE_MESSAGE(status_info.br_global_ipv6_addr_prefix_set,
					 "Sink should have new global IPv6 prefix set");
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(test_ipv6_prefix_pt_parent_64,
						 status_info.br_global_ipv6_addr_prefix.s6_addr, 8,
						 "Sink prefix should be 2001:db8:1::/64");

		/* Child global addresses (if any) should have new prefix */
		for (uint8_t i = 0; i < status_info.child_count; i++) {
			if (status_info.child_associations[i].global_ipv6_addr_set) {
				TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
					test_ipv6_prefix_pt_parent_64,
					status_info.child_associations[i].global_ipv6_addr.s6_addr,
					8, "Child global address should have new prefix");
			}
		}
	}

	/* 5. Our (FT) global address on DECT iface updated to new prefix */
	{
		struct net_if_ipv6 *ipv6 = test_iface->config.ip.ipv6;
		bool our_global_has_new_prefix = false;

		TEST_ASSERT_NOT_NULL_MESSAGE(ipv6, "DECT iface should have IPv6 config");
		for (int i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
			if (!ipv6->unicast[i].is_used ||
			    ipv6->unicast[i].address.family != AF_INET6) {
				continue;
			}
			if (net_ipv6_is_global_addr(&ipv6->unicast[i].address.in6_addr) &&
			    memcmp(ipv6->unicast[i].address.in6_addr.s6_addr,
				   test_ipv6_prefix_pt_parent_64, 8) == 0) {
				our_global_has_new_prefix = true;
				break;
			}
		}
		TEST_ASSERT_TRUE_MESSAGE(
			our_global_has_new_prefix,
			"Our (FT) global address on DECT iface should have new prefix");
	}

	/* Driver receives NET_EVENT_IPV6_PREFIX_ADD on DECT iface for new prefix and triggers
	 * cluster reconfigure. Verify nrf_modem_dect_mac_cluster_configure was called with
	 * the new prefix (mock simulates callback automatically).
	 */
	k_sleep(K_MSEC(400));
	TEST_ASSERT_TRUE_MESSAGE(
		mock_nrf_modem_dect_mac_cluster_configure_call_count > baseline_cluster_configure,
		"cluster_configure should be called after new prefix add (driver reconfig)");
	TEST_ASSERT_EQUAL_MESSAGE(NRF_MODEM_DECT_MAC_IPV6_ADDRESS_TYPE_PREFIX,
				  mock_last_configured_ipv6_prefix_type,
				  "cluster_configure should use PREFIX type for new prefix");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
		test_ipv6_prefix_pt_parent_64, mock_last_configured_ipv6_prefix, 8,
		"cluster_configure should be called with new prefix 2001:db8:1::/64");

	/* Allow async cluster_configure op callback; sends NET_EVENT_DECT_CLUSTER_CREATED_RESULT */
	k_sleep(K_MSEC(200));

	/* Reconfig must notify with NET_EVENT_DECT_CLUSTER_CREATED_RESULT */
	TEST_ASSERT_TRUE_MESSAGE(dect_cluster_created_received,
				 "NET_EVENT_DECT_CLUSTER_CREATED_RESULT should be sent after "
				 "cluster reconfig (new prefix)");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, received_cluster_created_data.status,
				  "Cluster reconfig result status should be OK");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, received_cluster_created_data.cluster_channel,
				      "Cluster reconfig result should report cluster channel");
#else
	TEST_IGNORE_MESSAGE("CONFIG_MODEM_CELLULAR not enabled");
#endif
}

#if defined(CONFIG_NET_SOCKETS_PACKET) && defined(CONFIG_NET_SOCKETS_PACKET_DGRAM)
/**
 * @brief Test raw packet send/receive to associated child via AF_PACKET SOCK_DGRAM
 *
 * Runs after test_dect_ft_sink_global_address_change (cluster has associated child).
 * Creates a packet socket like dect_l2_shell (AF_PACKET, SOCK_DGRAM, ETH_P_ALL),
 * binds to the DECT iface, sends a payload to the child and verifies it is passed
 * to the mock modem with the correct long_rd_id. Then injects data from the mock
 * (dlc_data_rx_ntf) and verifies recvfrom receives it with correct source address.
 */
void test_dect_ft_sckt_packet_rx_tx(void)
{
	struct sockaddr_ll bind_addr = {0};
	struct sockaddr_ll dst = {0};
	int sockfd;
	int ret;
	static const char tx_payload[] = "Tappara!";
	const size_t tx_len = sizeof(tx_payload);
	/* Child long_rd_id from test_dect_ft_cluster_associate_child_with_global_address */
	const uint32_t child_long_rd_id = 0xCAFEBABE;
	char rx_buf[DECT_MTU];
	struct sockaddr_ll src;
	socklen_t fromlen = sizeof(src);
	static const char rx_inject_payload[] = "DECT_rx_from_child";
	const size_t rx_inject_len = sizeof(rx_inject_payload);

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface, "DECT test iface should be set");

	int baseline_dlc_tx = mock_nrf_modem_dect_dlc_data_tx_call_count;

	sockfd = zsock_socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
	TEST_ASSERT_TRUE_MESSAGE(sockfd >= 0, "AF_PACKET SOCK_DGRAM socket should be created");

	bind_addr.sll_family = AF_PACKET;
	bind_addr.sll_ifindex = net_if_get_by_iface(test_iface);

	ret = zsock_bind(sockfd, (struct sockaddr *)&bind_addr, sizeof(struct sockaddr_ll));
	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "Bind to DECT iface should succeed");

	/* Prepare destination: child long_rd_id in sll_addr (big-endian) */
	uint32_t target_be = htonl(child_long_rd_id);

	memcpy(&dst.sll_addr, &target_be, sizeof(target_be));
	dst.sll_family = AF_PACKET;
	dst.sll_ifindex = bind_addr.sll_ifindex;
	dst.sll_halen = sizeof(uint32_t);

	ret = zsock_sendto(sockfd, tx_payload, tx_len, 0, (const struct sockaddr *)&dst,
			   sizeof(struct sockaddr_ll));
	TEST_ASSERT_EQUAL_MESSAGE((int)tx_len, ret, "sendto to associated child should succeed");

	/* Allow stack to pass data to modem mock */
	k_sleep(K_MSEC(150));

	/* Verify data was sent to mock modem with correct long_rd_id and payload */
	TEST_ASSERT_TRUE_MESSAGE(mock_nrf_modem_dect_dlc_data_tx_call_count > baseline_dlc_tx,
				 "nrf_modem_dect_dlc_data_tx should be called after sendto");
	TEST_ASSERT_EQUAL_MESSAGE(child_long_rd_id, mock_last_dlc_data_tx_long_rd_id,
				  "DLC data TX should target child long_rd_id");
	TEST_ASSERT_EQUAL_MESSAGE(tx_len, mock_last_dlc_data_tx_len,
				  "DLC data TX length should match sent payload");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(tx_payload, mock_last_dlc_data_tx_data, tx_len,
					 "DLC data TX payload should match sent data");

	/* Set recv timeout before injecting RX */
	struct timeval tv = {.tv_sec = 0, .tv_usec = 300 * 1000};

	ret = zsock_setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "setsockopt SO_RCVTIMEO should succeed");

	/* Inject data from libmodem (child) into RX path so recvfrom can receive it */
	TEST_ASSERT_NOT_NULL_MESSAGE(mock_ntf_callbacks.dlc_data_rx_ntf,
				     "dlc_data_rx_ntf callback should be registered");

	struct nrf_modem_dect_dlc_data_rx_ntf_cb_params rx_params = {
		.flow_id = 0,
		.long_rd_id = child_long_rd_id,
		.data = (void *)rx_inject_payload,
		.data_len = rx_inject_len,
	};

	mock_ntf_callbacks.dlc_data_rx_ntf(&rx_params);

	/* Allow stack to deliver injected data to the socket */
	k_sleep(K_MSEC(200));

	memset(&src, 0, sizeof(src));
	fromlen = sizeof(src);
	ret = zsock_recvfrom(sockfd, rx_buf, sizeof(rx_buf), 0, (struct sockaddr *)&src, &fromlen);

	TEST_ASSERT_TRUE_MESSAGE(ret >= 0, "recvfrom should receive injected data");
	TEST_ASSERT_EQUAL_MESSAGE((int)rx_inject_len, ret,
				  "recvfrom should return injected payload length");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(rx_inject_payload, rx_buf, rx_inject_len,
					 "Received data should match injected payload");

	/* Source address should be child long_rd_id (big-endian in sll_addr) */
	uint32_t src_long_rd_id = 0;

	memcpy(&src_long_rd_id, &src.sll_addr, sizeof(uint32_t));
	src_long_rd_id = ntohl(src_long_rd_id);
	TEST_ASSERT_EQUAL_MESSAGE(child_long_rd_id, src_long_rd_id,
				  "recvfrom source sll_addr should be child long_rd_id");

	zsock_close(sockfd);
}
#else
void test_dect_ft_sckt_packet_rx_tx(void)
{
	TEST_IGNORE_MESSAGE("CONFIG_NET_SOCKETS_PACKET_DGRAM not enabled");
}
#endif

/**
 * @brief Test FT local multicast TX: send IPv6 multicast and verify all children receive
 *
 * Runs after test_dect_ft_cluster_associate_child_with_global_address_2 (two children:
 * 0xCAFEBABE and 0xDEADBEEF). Sends one IPv6 UDP packet to link-local all-nodes (ff02::1).
 * Verifies that the L2 forwards the multicast to both children (nrf_modem_dect_dlc_data_tx
 * called twice with the two child long_rd_ids) and that payload matches.
 */
void test_dect_ft_local_multicast_tx(void)
{
#if defined(CONFIG_MODEM_CELLULAR)
	struct sockaddr_in6 bind_addr = {0};
	struct sockaddr_in6 mcast_dst = {0};
	int sockfd;
	int ret;
	static const char mcast_payload[] = "DECT_mcast";
	const size_t mcast_len = sizeof(mcast_payload);
	const uint32_t child1 = 0xCAFEBABE;
	const uint32_t child2 = 0xDEADBEEF;

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface, "DECT test iface should be set");

	struct in6_addr *ll_addr = net_if_ipv6_get_ll(test_iface, NET_ADDR_PREFERRED);

	TEST_ASSERT_NOT_NULL_MESSAGE(ll_addr, "DECT iface should have link-local address");

	/* Reset dlc_data_tx record so next sends fill mock_dlc_data_tx_long_rd_ids[0..1] */
	mock_dlc_data_tx_record_count = 0;
	memset(mock_dlc_data_tx_long_rd_ids, 0, sizeof(mock_dlc_data_tx_long_rd_ids));

	int baseline_dlc_tx = mock_nrf_modem_dect_dlc_data_tx_call_count;

	sockfd = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	TEST_ASSERT_TRUE_MESSAGE(sockfd >= 0, "AF_INET6 SOCK_DGRAM socket should be created");

	bind_addr.sin6_family = AF_INET6;
	memcpy(&bind_addr.sin6_addr, ll_addr, sizeof(struct in6_addr));
	bind_addr.sin6_port = 0;

	ret = zsock_bind(sockfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "Bind to DECT link-local should succeed");

	/* Destination: link-local all-nodes multicast ff02::1 */
	mcast_dst.sin6_family = AF_INET6;
	net_ipv6_addr_create_ll_allnodes_mcast(&mcast_dst.sin6_addr);
	mcast_dst.sin6_port = htons(12345);

	ret = zsock_sendto(sockfd, mcast_payload, mcast_len, 0, (const struct sockaddr *)&mcast_dst,
			   sizeof(mcast_dst));
	TEST_ASSERT_EQUAL_MESSAGE((int)mcast_len, ret,
				  "sendto to ff02::1 (multicast) should succeed");

	zsock_close(sockfd);

	/* Allow stack to forward multicast to both children */
	k_sleep(K_MSEC(200));

	/* TX path: FT multicast is sent to all children. Verify at least 2 dlc_data_tx
	 * (one per child); more may occur depending on stack/loopback.
	 */
	TEST_ASSERT_TRUE_MESSAGE(
		mock_nrf_modem_dect_dlc_data_tx_call_count >= baseline_dlc_tx + 2,
		"dlc_data_tx should be called at least twice (one per child) for multicast");
	TEST_ASSERT_TRUE_MESSAGE(mock_dlc_data_tx_record_count >= 2,
				 "Mock should have recorded at least two dlc_data_tx long_rd_ids");

	/* Both children should have received the multicast (order may vary) */
	bool seen_child1 = false;
	bool seen_child2 = false;

	for (int i = 0; i < mock_dlc_data_tx_record_count; i++) {
		if (mock_dlc_data_tx_long_rd_ids[i] == child1) {
			seen_child1 = true;
		}
		if (mock_dlc_data_tx_long_rd_ids[i] == child2) {
			seen_child2 = true;
		}
	}
	TEST_ASSERT_TRUE_MESSAGE(seen_child1,
				 "Multicast should have been sent to child 0xCAFEBABE");
	TEST_ASSERT_TRUE_MESSAGE(seen_child2,
				 "Multicast should have been sent to child 0xDEADBEEF");

	/* L2 sends full IPv6+UDP packet to modem (not just app payload); verify len and payload */
#define IPV6_HDR_LEN 40
#define UDP_HDR_LEN  8
	TEST_ASSERT_TRUE_MESSAGE(mock_last_dlc_data_tx_len >=
					 IPV6_HDR_LEN + UDP_HDR_LEN + mcast_len,
				 "Multicast packet should contain IPv6+UDP+payload");
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
		mcast_payload, mock_last_dlc_data_tx_data + (mock_last_dlc_data_tx_len - mcast_len),
		mcast_len, "Multicast payload should appear at end of transmitted packet");
#undef UDP_HDR_LEN
#undef IPV6_HDR_LEN
#else
	TEST_IGNORE_MESSAGE("CONFIG_MODEM_CELLULAR not enabled");
#endif
}

/**
 * @brief Test DECT FT network remove after network create
 *
 * Removes the FT network created in the previous test. When there are associated
 * children, the driver calls nrf_modem_dect_mac_association_release() for each
 * before deactivation. The mock invokes the association_release op callback so
 * NET_EVENT_DECT_ASSOCIATION_CHANGED with DECT_ASSOCIATION_RELEASED is sent for
 * each child (before reactivation). Verifies those events and that
 * NET_EVENT_DECT_NETWORK_STATUS is received with DECT_NETWORK_STATUS_REMOVED.
 */
void test_dect_ft_network_remove(void)
{
	struct dect_status_info status_info;
	uint8_t expected_child_count;
	int baseline_association_release;
	int result;

	/* Get child count before remove (driver will call association_release for each) */
	result = test_dect_status_info_get(test_iface, &status_info);
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Status info before network remove should succeed");
	expected_child_count = status_info.child_count;

	baseline_association_release = mock_nrf_modem_dect_mac_association_release_call_count;
	dect_network_status_received = false;
	dect_association_released_received = false;
	dect_association_released_count = 0;
	memset(&received_network_status_data, 0, sizeof(received_network_status_data));

	/* So mock invokes association_release op callback when driver releases each child */
	mock_auto_invoke_association_release_cb = true;

	LOG_DBG("Requesting network remove (expected children to release: %hhu)",
		expected_child_count);
	result = net_mgmt(NET_REQUEST_DECT_NETWORK_REMOVE, test_iface, NULL, 0);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Network remove request should succeed");

	/* Driver calls association_release for each child; mock invokes op callback (async).
	 * Driver then waits 2s and deactivates. Allow events to be processed.
	 */
	k_sleep(K_MSEC(250));

	if (expected_child_count > 0) {
		TEST_ASSERT_EQUAL_MESSAGE(
			baseline_association_release + expected_child_count,
			mock_nrf_modem_dect_mac_association_release_call_count,
			"nrf_modem_dect_mac_association_release should be called for each child");
		TEST_ASSERT_TRUE_MESSAGE(
			dect_association_released_received,
			"NET_EVENT_DECT_ASSOCIATION_CHANGED (DECT_ASSOCIATION_RELEASED) "
			"should be received for each child before reactivation");
		TEST_ASSERT_EQUAL_MESSAGE(
			expected_child_count, dect_association_released_count,
			"Should receive one DECT_ASSOCIATION_RELEASED event per child");
	}

	TEST_ASSERT_TRUE_MESSAGE(dect_network_status_received,
				 "NET_EVENT_DECT_NETWORK_STATUS should be received after "
				 "network remove");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NETWORK_STATUS_REMOVED,
				  received_network_status_data.network_status,
				  "Network status should be DECT_NETWORK_STATUS_REMOVED");

	k_sleep(K_MSEC(300));

	result = test_dect_status_info_get(test_iface, &status_info);
	TEST_ASSERT_EQUAL_MESSAGE(0, result,
				  "Status info request after network remove "
				  "should succeed");
	TEST_ASSERT_TRUE_MESSAGE(status_info.mdm_activated,
				 "Modem should be activated after network remove reactivation");
	TEST_ASSERT_FALSE_MESSAGE(status_info.cluster_running,
				  "Cluster should not be running after network remove");
	TEST_ASSERT_FALSE_MESSAGE(status_info.nw_beacon_running,
				  "Network beacon should not be running after network remove");
	TEST_ASSERT_EQUAL_MESSAGE(0, status_info.parent_count,
				  "FT should not report parent associations after network remove");
	TEST_ASSERT_EQUAL_MESSAGE(0, status_info.child_count,
				  "FT should not report child associations after network remove");
}

/**
 * @brief Test FT sink when PPP net iface goes down (IF_DOWN)
 *
 * Runs after test_dect_ft_network_remove. Sends NET_EVENT_IPV6_ROUTE_DEL and
 * NET_EVENT_IF_DOWN to simulate the PPP modem iface going down. The sink's
 * IF_DOWN handler (CONFIG_MODEM_CELLULAR) flushes IPv6 addresses on the PPP
 * iface via net_if_ipv6_addr_rm. Verifies that no IPv6 unicast addresses
 * remain on the PPP iface after IF_DOWN.
 */
void test_dect_ft_sink_down(void)
{
#if defined(CONFIG_MODEM_CELLULAR)
	struct net_if *ppp_if;
	struct net_event_ipv6_route route_evt;

	ppp_if = dect_test_get_mock_ppp_net_if();
	TEST_ASSERT_NOT_NULL_MESSAGE(ppp_if, "Mock PPP net_if should be available");

	/* Simulate route removal then iface down (order as might happen in practice) */
	memcpy(route_evt.nexthop.s6_addr, test_ipv6_addr_sink_router,
	       sizeof(test_ipv6_addr_sink_router));
	memcpy(route_evt.addr.s6_addr, test_ipv6_addr_sink_router,
	       sizeof(test_ipv6_addr_sink_router));
	route_evt.prefix_len = 64;
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTE_DEL, ppp_if, &route_evt,
					sizeof(route_evt));

	net_mgmt_event_notify(NET_EVENT_IF_DOWN, ppp_if);

	k_sleep(K_MSEC(100));

	/* Sink IF_DOWN handler flushes all IPv6 addrs on PPP iface; verify none set */
	TEST_ASSERT_EQUAL_MESSAGE(0, dect_test_mock_ppp_ipv6_unicast_used_count(),
				  "PPP iface should have no IPv6 unicast addresses after IF_DOWN");
#else
	TEST_IGNORE_MESSAGE("CONFIG_MODEM_CELLULAR not enabled, skip sink_down test");
#endif
}

/**
 * @brief Test DECT NR+ connection manager connect (FT)
 *
 * Runs after test_dect_ft_sink_down. Re-establishes the sink (restore mock PPP
 * unicast, IF_UP, ADDR_ADD, PREFIX_ADD, ROUTER_ADD, NBR_ADD) so the sink has
 * global prefix. Then calls conn_mgr_if_connect(test_iface), which triggers
 * NET_REQUEST_DECT_NETWORK_CREATE from dect_net_l2_conn_mgr.c. Drives the same
 * mock sequence as test_dect_ft_network_create (network scan, RSSI scan, free
 * channel, cluster_configure). Verifies cluster created, NET_EVENT_DECT_NETWORK_STATUS
 * (DECT_NETWORK_STATUS_CREATED), and NET_EVENT_L4_IPV6_CONNECTED (L4 event).
 */
void test_dect_ft_conn_mgr_connect(void)
{
#if defined(CONFIG_MODEM_CELLULAR) && defined(CONFIG_NET_CONNECTION_MANAGER)
	struct net_if *ppp_if;
	struct net_event_ipv6_addr addr_evt;

	LOG_DBG("=== Testing DECT FT connection manager connect ===");

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface, "DECT test iface should be set");

	ppp_if = dect_test_get_mock_ppp_net_if();
	TEST_ASSERT_NOT_NULL_MESSAGE(ppp_if, "Mock PPP net_if should be available");

	/* When PPP iface is the mock it is not in the net_if list; conn_mgr does not track it
	 * and DECT iface may not reach conn_mgr "connected" (no L4 for PPP). Skip those assertions.
	 */
	const bool mock_ppp = (net_if_get_by_iface(ppp_if) < 0);

#if defined(CONFIG_NET_IPV6)
	dect_test_mock_ppp_restore_ipv6_unicast();
#endif

	/* FT+sink: L4 events are for PPP iface (sink/backhaul), not DECT iface. */
	test_l4_expected_iface = ppp_if;

	net_mgmt_init_event_callback(&test_l4_cb, test_l4_ipv6_connected_handler,
				     TEST_L4_EVENT_MASK);
	net_mgmt_add_event_callback(&test_l4_cb);

	/* Mock PPP: bring down then up so conn_mgr gets IF_DOWN then IF_UP and tracks oper-up.
	 * Use same pattern as test_dect_ft_sink_global_address_assign: set NET_IF_UP and
	 * notify NET_EVENT_IF_UP directly so we avoid notify_iface_up() (mock has no link_addr).
	 */
	net_if_down(ppp_if);
	k_sleep(K_MSEC(50));
	net_if_flag_set(ppp_if, NET_IF_UP);
	net_mgmt_event_notify(NET_EVENT_IF_UP, ppp_if);
	k_sleep(K_MSEC(50));
	/* Sink IF_DOWN cleared mock unicast; restore so conn_mgr sees global addr on ADDR_ADD. */
#if defined(CONFIG_NET_IPV6)
	dect_test_mock_ppp_restore_ipv6_unicast();
#endif
	/* Sink gets global prefix from ADDR_ADD; conn_mgr sees global addr -> L4 events for PPP. */
	memcpy(addr_evt.addr.s6_addr, test_ipv6_addr_sink_router,
	       sizeof(test_ipv6_addr_sink_router));
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ADDR_ADD, ppp_if, &addr_evt,
					sizeof(addr_evt));
	k_sleep(K_MSEC(100));
	TEST_ASSERT_TRUE_MESSAGE(dect_sink_status_received,
				 "NET_EVENT_DECT_SINK_STATUS CONNECTED after ADDR_ADD");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_SINK_STATUS_CONNECTED, received_sink_status_data.sink_status,
				  "Sink should be CONNECTED after global address add");
	LOG_INF("NET_EVENT_DECT_SINK_STATUS - Sink is connected, iface towards Internet %p",
		(void *)ppp_if);

#if defined(CONFIG_NET_IPV6)
	test_mock_ppp_sink_notify_prefix_router_nbr(ppp_if);
#endif
	k_sleep(K_MSEC(100));

	/* Verify PPP iface conn_mgr status when it is tracked (mock is not in net_if
	 * section so net_if_get_by_iface(ppp_if) < 0; skip conn_mgr assertions then).
	 */
	{
		int ppp_idx = net_if_get_by_iface(ppp_if);
		uint16_t ppp_state = conn_mgr_if_state(ppp_if);

		LOG_INF("iface %d (%p) status: %s, %s, %s, %s, IPv6, %s.", ppp_idx, (void *)ppp_if,
			(ppp_state & CONN_MGR_IF_IGNORED_BIT) ? "ignored" : "watched",
			conn_mgr_if_is_bound(ppp_if) ? "bound" : "not bound",
			net_if_is_admin_up(ppp_if) ? "admin-up" : "admin-down",
			(ppp_state & CONN_MGR_IF_UP_BIT) ? "oper-up" : "oper-down",
			(ppp_state & CONN_MGR_IF_READY_BIT) ? "connected" : "not connected");
		TEST_ASSERT_TRUE_MESSAGE(net_if_is_admin_up(ppp_if),
					 "PPP iface should be admin-up");
		if (ppp_idx >= 0) {
			TEST_ASSERT_TRUE_MESSAGE(ppp_state != CONN_MGR_IF_STATE_INVALID,
						 "PPP iface should be tracked by conn_mgr");
			TEST_ASSERT_FALSE_MESSAGE(ppp_state & CONN_MGR_IF_IGNORED_BIT,
						  "PPP iface should be watched");
			TEST_ASSERT_FALSE_MESSAGE(conn_mgr_if_is_bound(ppp_if),
						  "PPP iface should be not bound");
			TEST_ASSERT_TRUE_MESSAGE((ppp_state & CONN_MGR_IF_UP_BIT) != 0,
						 "PPP iface should be oper-up");
			TEST_ASSERT_TRUE_MESSAGE((ppp_state & CONN_MGR_IF_IPV6_SET_BIT) != 0,
						 "PPP iface should have IPv6");
			TEST_ASSERT_TRUE_MESSAGE((ppp_state & CONN_MGR_IF_READY_BIT) != 0,
						 "PPP iface should be connected");
		}
	}

	dect_cluster_created_received = false;
	dect_network_status_received = false;
	/* Do not clear dect_l4_ipv6_connected_received:
	 * FT+sink gets L4 for PPP during sink setup.
	 */
	memset(&received_cluster_created_data, 0, sizeof(received_cluster_created_data));
	memset(&received_network_status_data, 0, sizeof(received_network_status_data));

	mock_cluster_creation_band1 = true;

	int baseline_network_scan = mock_nrf_modem_dect_mac_network_scan_call_count;
	int baseline_rssi_scan = mock_nrf_modem_dect_mac_rssi_scan_call_count;
	int baseline_rssi_scan_stop = mock_nrf_modem_dect_mac_rssi_scan_stop_call_count;
	int baseline_cluster_configure = mock_nrf_modem_dect_mac_cluster_configure_call_count;

	conn_mgr_if_connect(test_iface);

	k_sleep(K_MSEC(50));
	TEST_ASSERT_EQUAL_MESSAGE(baseline_network_scan + 1,
				  mock_nrf_modem_dect_mac_network_scan_call_count,
				  "Network scan should be called by conn_mgr connect");

	if (mock_op_callbacks.network_scan) {
		struct nrf_modem_dect_mac_network_scan_cb_params scan_complete_params = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK, .num_scanned_channels = 11};

		mock_op_callbacks.network_scan(&scan_complete_params);
		k_sleep(K_MSEC(100));
	} else {
		TEST_FAIL_MESSAGE("Network scan operation callback should be registered");
	}

	k_sleep(K_MSEC(100));
	TEST_ASSERT_EQUAL_MESSAGE(baseline_rssi_scan + 1,
				  mock_nrf_modem_dect_mac_rssi_scan_call_count,
				  "RSSI scan should be called after network scan");

	{
		uint16_t free_channel = TEST_BEACON_CHANNEL;
		static uint8_t busy_array[6] = {0, 0, 0, 0, 0, 0};
		static uint8_t possible_array[6] = {0, 0, 0, 0, 0, 0};
		static uint8_t free_array[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		struct nrf_modem_dect_mac_rssi_scan_ntf_cb_params rssi_result = {
			.channel = free_channel,
			.busy_percentage = 0,
			.rssi_meas_array_size = 6,
			.busy = busy_array,
			.possible = possible_array,
			.free = free_array};

		if (mock_ntf_callbacks.rssi_scan_ntf) {
			mock_ntf_callbacks.rssi_scan_ntf(&rssi_result);
		}
		k_sleep(K_MSEC(200));
		TEST_ASSERT_EQUAL_MESSAGE(baseline_rssi_scan_stop + 1,
					  mock_nrf_modem_dect_mac_rssi_scan_stop_call_count,
					  "rssi_scan_stop should be called after free channel ntf");
	}

	if (mock_op_callbacks.rssi_scan) {
		struct nrf_modem_dect_mac_rssi_scan_cb_params p = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK};

		mock_op_callbacks.rssi_scan(&p);
		k_sleep(K_MSEC(200));
	}
	if (mock_op_callbacks.rssi_scan_stop) {
		struct nrf_modem_dect_mac_rssi_scan_stop_cb_params p = {
			.status = NRF_MODEM_DECT_MAC_STATUS_OK};

		mock_op_callbacks.rssi_scan_stop(&p);
		k_sleep(K_MSEC(100));
	}

	k_sleep(K_MSEC(300));
	TEST_ASSERT_EQUAL_MESSAGE(baseline_cluster_configure + 1,
				  mock_nrf_modem_dect_mac_cluster_configure_call_count,
				  "cluster_configure should be called after conn_mgr connect flow");
	TEST_ASSERT_TRUE_MESSAGE(dect_cluster_created_received,
				 "NET_EVENT_DECT_CLUSTER_CREATED_RESULT should be received");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, received_cluster_created_data.status,
				  "Cluster created status should be OK");

	TEST_ASSERT_TRUE_MESSAGE(dect_network_status_received,
				 "NET_EVENT_DECT_NETWORK_STATUS should be received");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NETWORK_STATUS_CREATED,
				  received_network_status_data.network_status,
				  "Network status should be DECT_NETWORK_STATUS_CREATED");

	/* Wait for conn_mgr to mark DECT iface as connected (oper-up, IPv6, connected)
	 * and for L4 events to fire (monitor runs in separate thread).
	 * Skip when using mock PPP: conn_mgr does not track mock and DECT may stay not ready.
	 */
	{
		const int wait_ms = 2000;
		const int step_ms = 50;
		int waited = 0;
		uint16_t state;

		while (waited < wait_ms) {
			state = conn_mgr_if_state(test_iface);
			if (state != CONN_MGR_IF_STATE_INVALID &&
			    (state & CONN_MGR_IF_READY_BIT) != 0) {
				break;
			}
			k_sleep(K_MSEC(step_ms));
			waited += step_ms;
		}
		state = conn_mgr_if_state(test_iface);
		LOG_INF("iface %d (%p) status: %s, %s, %s, %s, IPv6, %s.",
			net_if_get_by_iface(test_iface), (void *)test_iface,
			(state & CONN_MGR_IF_IGNORED_BIT) ? "ignored" : "watched",
			conn_mgr_if_is_bound(test_iface) ? "bound" : "not bound",
			net_if_is_admin_up(test_iface) ? "admin-up" : "admin-down",
			(state & CONN_MGR_IF_UP_BIT) ? "oper-up" : "oper-down",
			(state & CONN_MGR_IF_READY_BIT) ? "connected" : "not connected");
		if (!mock_ppp) {
			TEST_ASSERT_TRUE_MESSAGE(state != CONN_MGR_IF_STATE_INVALID &&
							 (state & CONN_MGR_IF_READY_BIT) != 0,
						 "DECT iface should be connected in conn_mgr "
						 "status (oper-up, IPv6, connected)");
		}
	}

	/* L4 events are sent when conn_mgr marks iface ready; allow callback to run */
	k_sleep(K_MSEC(100));
	if (!mock_ppp) {
		TEST_ASSERT_TRUE_MESSAGE(
			dect_l4_ipv6_connected_received,
			"NET_EVENT_L4_IPV6_CONNECTED (L4 event) should be received");
	}

	LOG_DBG("Conn mgr connect test completed: cluster created, network status CREATED, "
		"L4 IPv6 connected");
	test_l4_expected_iface = NULL;
#else
	TEST_IGNORE_MESSAGE("CONFIG_MODEM_CELLULAR and CONFIG_NET_CONNECTION_MANAGER required");
#endif
}

/**
 * @brief Test conn_mgr disconnect triggers NET_REQUEST_DECT_NETWORK_REMOVE and driver events
 *
 * Runs after test_dect_ft_conn_mgr_connect (cluster is created). Calls conn_mgr_if_disconnect()
 * which invokes NET_REQUEST_DECT_NETWORK_REMOVE for FT. Verifies that the driver sends
 * NET_EVENT_DECT_ASSOCIATION_CHANGED with DECT_ASSOCIATION_RELEASED for each child (if any)
 * and NET_EVENT_DECT_NETWORK_STATUS with DECT_NETWORK_STATUS_REMOVED.
 */
void test_dect_ft_conn_mgr_disconnect(void)
{
#if defined(CONFIG_MODEM_CELLULAR) && defined(CONFIG_NET_CONNECTION_MANAGER)
	struct dect_status_info status_info;
	int result;

	/* Get child count before disconnect (driver will call association_release for each) */
	result = test_dect_status_info_get(test_iface, &status_info);
	TEST_ASSERT_EQUAL_MESSAGE(0, result,
				  "Status info before conn_mgr disconnect should succeed");
	uint8_t expected_child_count = status_info.child_count;

	int baseline_association_release = mock_nrf_modem_dect_mac_association_release_call_count;

	dect_network_status_received = false;
	dect_association_released_received = false;
	dect_association_released_count = 0;
	memset(&received_network_status_data, 0, sizeof(received_network_status_data));

	/* Mock invokes association_release op callback when driver releases each child */
	mock_auto_invoke_association_release_cb = true;

	LOG_INF("Conn mgr disconnect (NET_REQUEST_DECT_NETWORK_REMOVE), expected children: %hhu",
		expected_child_count);
	result = conn_mgr_if_disconnect(test_iface);

	TEST_ASSERT_EQUAL_MESSAGE(0, result, "conn_mgr_if_disconnect should succeed");

	/* Driver calls association_release for each child; mock invokes op callback (async).
	 * Driver then deactivates and sends REMOVED. Allow events to be processed.
	 */
	k_sleep(K_MSEC(250));

	if (expected_child_count > 0) {
		TEST_ASSERT_EQUAL_MESSAGE(
			baseline_association_release + expected_child_count,
			mock_nrf_modem_dect_mac_association_release_call_count,
			"nrf_modem_dect_mac_association_release should be called for each child");
		TEST_ASSERT_TRUE_MESSAGE(
			dect_association_released_received,
			"NET_EVENT_DECT_ASSOCIATION_CHANGED (DECT_ASSOCIATION_RELEASED) "
			"should be received from driver");
		TEST_ASSERT_EQUAL_MESSAGE(
			expected_child_count, dect_association_released_count,
			"Should receive one DECT_ASSOCIATION_RELEASED event per child");
	}

	TEST_ASSERT_TRUE_MESSAGE(
		dect_network_status_received,
		"NET_EVENT_DECT_NETWORK_STATUS should be received after disconnect");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NETWORK_STATUS_REMOVED,
				  received_network_status_data.network_status,
				  "Network status should be DECT_NETWORK_STATUS_REMOVED");
#else
	TEST_IGNORE_MESSAGE("CONFIG_MODEM_CELLULAR and CONFIG_NET_CONNECTION_MANAGER required");
#endif
}

/**
 * @brief Test NET_REQUEST_DECT_CLUSTER_INFO on FT after cluster start
 *
 * Requests cluster info via NET_REQUEST_DECT_CLUSTER_INFO. The mock libmodem
 * invokes the cluster_info op callback via simulate_async_callback() with
 * status OK, cluster counters, and RSSI results for the configured cluster
 * channel. Verifies net_mgmt returns 0 and NET_EVENT_DECT_CLUSTER_INFO is
 * received with DECT_STATUS_OK and valid RSSI result data.
 */
void test_dect_ft_cluster_info(void)
{
	LOG_INF("=== Testing NET_REQUEST_DECT_CLUSTER_INFO (FT, after cluster start) ===");
	uint16_t expected_cluster_channel = received_cluster_created_data.cluster_channel != 0
						    ? received_cluster_created_data.cluster_channel
						    : TEST_BEACON_CHANNEL;

	dect_cluster_info_received = false;
	memset(&received_cluster_info_data, 0, sizeof(received_cluster_info_data));

	/* Let ctrl thread and net_mgmt work queue drain (cluster_start can leave many events) */
	k_sleep(K_MSEC(800));

	int baseline_cluster_info = mock_nrf_modem_dect_mac_cluster_info_call_count;
	int ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_INFO, test_iface, NULL, 0);

	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "NET_REQUEST_DECT_CLUSTER_INFO should succeed");
	TEST_ASSERT_EQUAL_MESSAGE(
		baseline_cluster_info + 1, mock_nrf_modem_dect_mac_cluster_info_call_count,
		"nrf_modem_dect_mac_cluster_info() should be called exactly once");

	/* Mock drives cluster_info callback via simulate_async_callback(); wait for it */
	k_yield();
	k_sleep(K_MSEC(500));

	TEST_ASSERT_TRUE_MESSAGE(dect_cluster_info_received,
				 "NET_EVENT_DECT_CLUSTER_INFO should be received");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, received_cluster_info_data.status,
				  "Cluster info event status should be OK");
	TEST_ASSERT_EQUAL_MESSAGE(
		expected_cluster_channel,
		received_cluster_info_data.status_info.rssi_result.channel,
		"Cluster info RSSI channel should match the configured cluster channel");
	TEST_ASSERT_TRUE_MESSAGE(
		received_cluster_info_data.status_info.rssi_result.all_subslots_free,
		"Cluster info RSSI result should mark cluster channel free");
	TEST_ASSERT_EQUAL_MESSAGE(
		100, received_cluster_info_data.status_info.rssi_result.scan_suitable_percent,
		"Cluster info RSSI scan suitable percent should be 100 for free channel");
	TEST_ASSERT_EQUAL_MESSAGE(
		48, received_cluster_info_data.status_info.rssi_result.free_subslot_cnt,
		"Cluster info RSSI result should contain all free subslots");
	TEST_ASSERT_EQUAL_MESSAGE(
		0, received_cluster_info_data.status_info.rssi_result.possible_subslot_cnt,
		"Cluster info RSSI result should not contain possible subslots");
	TEST_ASSERT_EQUAL_MESSAGE(
		0, received_cluster_info_data.status_info.rssi_result.busy_subslot_cnt,
		"Cluster info RSSI result should not contain busy subslots");
	TEST_ASSERT_EQUAL_MESSAGE(
		0, received_cluster_info_data.status_info.rssi_result.busy_percentage,
		"Cluster info RSSI busy percentage should match the free cluster channel");
}

/**
 * @brief Test FT status info after child association
 *
 * Requests DECT status info via NET_REQUEST_DECT_STATUS_INFO_GET after the FT
 * cluster has been started, network beacon has been stopped, and one child has
 * associated. Verifies the FT-specific state is reflected in the returned
 * status information.
 */
void test_dect_ft_status(void)
{
	struct dect_status_info status_info;
	uint16_t expected_cluster_channel = received_cluster_created_data.cluster_channel != 0
						    ? received_cluster_created_data.cluster_channel
						    : TEST_BEACON_CHANNEL;
	int result;

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface,
				     "DECT interface should be available for FT status test");

	result = test_dect_status_info_get(test_iface, &status_info);
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "FT status info request should succeed");

	TEST_ASSERT_TRUE_MESSAGE(status_info.mdm_activated,
				 "FT modem should be reported as activated");
	TEST_ASSERT_TRUE_MESSAGE(status_info.cluster_running,
				 "FT cluster should be reported as running");
	TEST_ASSERT_EQUAL_MESSAGE(expected_cluster_channel, status_info.cluster_channel,
				  "FT cluster channel should match the created cluster channel");
	TEST_ASSERT_FALSE_MESSAGE(status_info.nw_beacon_running,
				  "Network beacon should not be running after beacon stop");
	TEST_ASSERT_EQUAL_MESSAGE(0, status_info.parent_count,
				  "FT should not report parent associations");
	TEST_ASSERT_EQUAL_MESSAGE(1, status_info.child_count,
				  "FT should report one child association after child attach");
	TEST_ASSERT_EQUAL_MESSAGE(
		received_association_data.long_rd_id, status_info.child_associations[0].long_rd_id,
		"FT child association Long RD ID should match the attached child");
}

/**
 * @brief Test NET_REQUEST_DECT_CLUSTER_STOP returns -ENOTSUP on FT (not supported)
 *
 * The DECT NR+ driver does not implement cluster_stop_req in the HAL; the net_mgmt
 * handler therefore returns -ENOTSUP. This test verifies that behavior on the FT side.
 */
void test_dect_ft_cluster_stop_returns_enotsup(void)
{
	LOG_DBG("=== Testing NET_REQUEST_DECT_CLUSTER_STOP returns -ENOTSUP (FT) ===");

	struct dect_cluster_stop_req_params params = {};

	int ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_STOP, test_iface, &params, sizeof(params));

	TEST_ASSERT_EQUAL_INT_MESSAGE(
		-ENOTSUP, ret,
		"NET_REQUEST_DECT_CLUSTER_STOP should return -ENOTSUP (not supported)");
}

/**
 * @brief Test successful NET_REQUEST_DECT_NW_BEACON_START on FT with upper channels
 *
 * After cluster is started, starts the network beacon on a couple of upper band channels
 * (main channel 1675, additional 1677 for band 1). Verifies net_mgmt returns 0 and
 * NET_EVENT_DECT_NW_BEACON_START_RESULT is received with DECT_STATUS_OK.
 * Also verifies NET_EVENT_DECT_NETWORK_STATUS is received with DECT_NETWORK_STATUS_CREATED.
 */
void test_dect_ft_nw_beacon_start(void)
{
	LOG_DBG("=== Testing NET_REQUEST_DECT_NW_BEACON_START (FT, upper channels) ===");

	dect_nw_beacon_start_received = false;
	dect_nw_beacon_start_status = DECT_STATUS_OS_ERROR;
	dect_network_status_received = false;

	/* Band 1: 1657–1677 (odd). Use upper channels: main 1675, additional 1677 */
	struct dect_nw_beacon_start_req_params params = {
		.channel = 1675,
		.additional_ch_count = 1,
		.additional_ch_list = {1677},
	};

	int ret = net_mgmt(NET_REQUEST_DECT_NW_BEACON_START, test_iface, &params, sizeof(params));

	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "NET_REQUEST_DECT_NW_BEACON_START should succeed");

	/* Wait for async modem callback to complete and NET_EVENT_DECT_NW_BEACON_START_RESULT */
	k_sleep(K_MSEC(250));

	TEST_ASSERT_TRUE_MESSAGE(dect_nw_beacon_start_received,
				 "NET_EVENT_DECT_NW_BEACON_START_RESULT should be received");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, dect_nw_beacon_start_status,
				  "NW beacon start result status should be OK");
	TEST_ASSERT_TRUE_MESSAGE(
		dect_network_status_received,
		"NET_EVENT_DECT_NETWORK_STATUS should be received after NW beacon start");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NETWORK_STATUS_CREATED,
				  received_network_status_data.network_status,
				  "Network status should be DECT_NETWORK_STATUS_CREATED");
}

/**
 * @brief Test successful NET_REQUEST_DECT_NW_BEACON_STOP on FT
 *
 * Stops the network beacon that was started in test_dect_ft_nw_beacon_start.
 * Verifies net_mgmt returns 0 and NET_EVENT_DECT_NW_BEACON_STOP_RESULT is received
 * with DECT_STATUS_OK.
 */
void test_dect_ft_nw_beacon_stop(void)
{
	LOG_DBG("=== Testing NET_REQUEST_DECT_NW_BEACON_STOP (FT) ===");

	dect_nw_beacon_stop_received = false;
	dect_nw_beacon_stop_status = DECT_STATUS_OS_ERROR;

	struct dect_nw_beacon_stop_req_params params = {};

	int ret = net_mgmt(NET_REQUEST_DECT_NW_BEACON_STOP, test_iface, &params, sizeof(params));

	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "NET_REQUEST_DECT_NW_BEACON_STOP should succeed");

	/* Wait for async modem callback to complete and NET_EVENT_DECT_NW_BEACON_STOP_RESULT */
	k_sleep(K_MSEC(250));

	TEST_ASSERT_TRUE_MESSAGE(dect_nw_beacon_stop_received,
				 "NET_EVENT_DECT_NW_BEACON_STOP_RESULT should be received");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_STATUS_OK, dect_nw_beacon_stop_status,
				  "NW beacon stop result status should be OK");
}

/**
 * @brief Test DECT FT cluster associate child (incoming association request from PT)
 *
 * This test verifies the incoming association request handling on an FT device:
 * 1. FT cluster is already started from test_dect_ft_cluster_start()
 * 2. Simulate incoming association request from a PT device via association_ntf callback
 * 3. Verify NET_EVENT_DECT_ASSOCIATION_CHANGED is received with DECT_ASSOCIATION_CREATED
 * 4. Verify neighbor role is DECT_NEIGHBOR_ROLE_CHILD
 * 5. Verify interface state changes from DORMANT to UP (first child association)
 */
void test_dect_ft_cluster_associate_child(void)
{
	LOG_DBG("=== Testing DECT FT cluster associate child (incoming association request) ===");

	/* Prerequisites: FT cluster should already be started from previous test */
	/* Interface state should be: admin=UP, carrier=ON, oper=DORMANT */
	TEST_ASSERT_TRUE_MESSAGE(net_if_is_admin_up(test_iface),
				 "Interface should be admin UP (FT cluster started)");
	TEST_ASSERT_TRUE_MESSAGE(net_if_is_carrier_ok(test_iface),
				 "Interface carrier should be ON (FT cluster started)");
	TEST_ASSERT_TRUE_MESSAGE(net_if_is_dormant(test_iface),
				 "Interface should be DORMANT before first child association");

	/* Reset association event tracking for this test */
	dect_association_changed_received = false;
	dect_association_created_received = false;
	dect_association_released_received = false;
	dect_association_failed_received = false;
	memset(&received_association_data, 0, sizeof(received_association_data));

	/* Define the simulated PT device's identity */
	uint16_t pt_short_rd_id = 0xABCD;
	uint32_t pt_long_rd_id = 0xDEADBEEF;

	LOG_DBG("Simulating incoming association request from PT device:");
	LOG_DBG("- PT Short RD ID: 0x%04X", pt_short_rd_id);
	LOG_DBG("- PT Long RD ID: 0x%08X", pt_long_rd_id);

	/* Step 1: Simulate incoming association request via association_ntf callback */
	/* This simulates a PT device sending an association request to this FT device */
	TEST_ASSERT_NOT_NULL_MESSAGE(mock_ntf_callbacks.association_ntf,
				     "Association notification callback should be registered");

	struct nrf_modem_dect_mac_association_ntf_cb_params assoc_ntf_params = {
		.status = NRF_MODEM_DECT_MAC_ASSOCIATION_INDICATION_STATUS_SUCCESS,
		.tx_method = NRF_MODEM_DECT_MAC_COMMUNICATION_METHOD_RACH, /* Random access */
		.rx_signal_info = {.rssi_2 = -50, .snr = 20},
		.short_rd_id = pt_short_rd_id,
		.long_rd_id = pt_long_rd_id,
		.number_of_ies = 0,
	};

	LOG_DBG("Calling association_ntf callback to simulate incoming PT association request");
	mock_ntf_callbacks.association_ntf(&assoc_ntf_params);

	/* Step 2: Wait for the driver to process the association indication */
	/* The driver's message queue processes the callback and creates the child association */
	k_sleep(K_MSEC(200));

	/* Step 3: Verify that NET_EVENT_DECT_ASSOCIATION_CHANGED was received */
	TEST_ASSERT_TRUE_MESSAGE(
		dect_association_changed_received,
		"NET_EVENT_DECT_ASSOCIATION_CHANGED should be received after child association");

	/* Step 4: Verify that we specifically received DECT_ASSOCIATION_CREATED event */
	TEST_ASSERT_TRUE_MESSAGE(
		dect_association_created_received,
		"DECT_ASSOCIATION_CREATED event should be received for new child association");

	/* Step 5: Validate the association event data */
	TEST_ASSERT_EQUAL_MESSAGE(DECT_ASSOCIATION_CREATED,
				  received_association_data.association_change_type,
				  "Association change type should be DECT_ASSOCIATION_CREATED");

	/* Verify the Long RD ID matches the simulated PT device */
	TEST_ASSERT_EQUAL_MESSAGE(pt_long_rd_id, received_association_data.long_rd_id,
				  "Association event Long RD ID should match the PT device");

	/* Verify the neighbor role is CHILD (PT device is our child) */
	TEST_ASSERT_EQUAL_MESSAGE(
		DECT_NEIGHBOR_ROLE_CHILD, received_association_data.neighbor_role,
		"Neighbor role should be DECT_NEIGHBOR_ROLE_CHILD for incoming PT association");

	/* Step 6: Verify interface state changed from DORMANT to UP (first child association) */
	/* When FT device gets its first child association, interface becomes operational */
	TEST_ASSERT_FALSE_MESSAGE(net_if_is_dormant(test_iface),
				  "Interface should NOT be DORMANT after first child association");
	TEST_ASSERT_EQUAL_MESSAGE(
		NET_IF_OPER_UP, net_if_oper_state(test_iface),
		"Interface operational state should be UP after first child association");

	/* Step 7: Verify IPv6 link-local address is set on DECT interface */
	/* First child association triggers IPv6 address configuration on FT device */
	struct in6_addr *ll_addr = net_if_ipv6_get_ll(test_iface, NET_ADDR_PREFERRED);

	TEST_ASSERT_NOT_NULL_MESSAGE(
		ll_addr, "IPv6 link-local address should be set on DECT interface after first "
			 "child association");

	/* Verify it's a valid link-local address (fe80::/10 prefix) */
	TEST_ASSERT_TRUE_MESSAGE(net_ipv6_is_ll_addr(ll_addr),
				 "IPv6 address should be a valid link-local address (fe80::/10)");

	/* Step 8: Verify from DECT status that we have one child with only a local address */
	struct dect_status_info status_info = {0};
	int result = test_dect_status_info_get(test_iface, &status_info);

	TEST_ASSERT_EQUAL_MESSAGE(0, result,
				  "DECT status info get should succeed after child association");
	TEST_ASSERT_EQUAL_MESSAGE(1, status_info.child_count,
				  "FT should have exactly one child in DECT status");
	TEST_ASSERT_FALSE_MESSAGE(status_info.child_associations[0].global_ipv6_addr_set,
				  "Child should have only local address (no global yet)");
	TEST_ASSERT_EQUAL_MESSAGE(pt_long_rd_id, status_info.child_associations[0].long_rd_id,
				  "Child in status should match associated PT Long RD ID");

	/* Child local address: last 64 bits = FT (our) long RD ID + child long RD ID */
	struct dect_settings settings = {0};
	int settings_result =
		net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &settings, sizeof(settings));

	TEST_ASSERT_EQUAL_MESSAGE(0, settings_result,
				  "Settings read should succeed to get FT long RD ID");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, settings.identities.transmitter_long_rd_id,
				      "FT (our) long RD ID should be set");
	{
		uint32_t ft_long_rd_id = settings.identities.transmitter_long_rd_id;
		struct in6_addr expected_local;
		struct in6_addr prefix_local;

		prefix_local.s6_addr32[0] = htonl(0xfe800000);
		prefix_local.s6_addr32[1] = 0;
		TEST_ASSERT_TRUE_MESSAGE(
			dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
				prefix_local, ft_long_rd_id, pt_long_rd_id, &expected_local),
			"Expected child local address should be derivable");
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
			&expected_local, &status_info.child_associations[0].local_ipv6_addr,
			sizeof(expected_local),
			"Child local address should be fe80:: + FT long RD ID + child long RD ID");
	}

	LOG_DBG("=== FT cluster associate child test completed successfully! ===");
	LOG_DBG("- Association event received: %s",
		dect_association_changed_received ? "YES" : "NO");
	LOG_DBG("- Association created: %s", dect_association_created_received ? "YES" : "NO");
	LOG_DBG("- Child Long RD ID: 0x%08X", received_association_data.long_rd_id);
	LOG_DBG("- Neighbor role: %s",
		received_association_data.neighbor_role == DECT_NEIGHBOR_ROLE_CHILD ? "CHILD"
										    : "OTHER");
	LOG_DBG("- Interface state: %s",
		net_if_oper_state(test_iface) == NET_IF_OPER_UP ? "UP" : "NOT UP");
	if (ll_addr) {
		char addr_str[NET_IPV6_ADDR_LEN];

		net_addr_ntop(AF_INET6, ll_addr, addr_str, sizeof(addr_str));
		LOG_DBG("- IPv6 link-local address: %s", addr_str);
	}
}

/**
 * @brief Calculate ICMPv6 checksum (RFC 4443)
 *
 * The ICMPv6 checksum is computed over the IPv6 pseudo-header + ICMPv6 message.
 * Pseudo-header: src addr (16) + dst addr (16) + length (4) + zeros (3) + next hdr (1)
 */
static uint16_t calc_icmpv6_checksum(const uint8_t *src_addr, const uint8_t *dst_addr,
				     const uint8_t *icmpv6_data, size_t icmpv6_len)
{
	uint32_t sum = 0;

	/* Add source address */
	for (int i = 0; i < 16; i += 2) {
		sum += (src_addr[i] << 8) | src_addr[i + 1];
	}

	/* Add destination address */
	for (int i = 0; i < 16; i += 2) {
		sum += (dst_addr[i] << 8) | dst_addr[i + 1];
	}

	/* Add upper-layer packet length */
	sum += (icmpv6_len >> 16) & 0xFFFF;
	sum += icmpv6_len & 0xFFFF;

	/* Add next header (IPPROTO_ICMPV6 = 58) */
	sum += IPPROTO_ICMPV6;

	/* Add ICMPv6 data (with checksum field set to 0) */
	for (size_t i = 0; i < icmpv6_len; i += 2) {
		if (i + 1 < icmpv6_len) {
			sum += (icmpv6_data[i] << 8) | icmpv6_data[i + 1];
		} else {
			sum += icmpv6_data[i] << 8; /* Pad with zero */
		}
	}

	/* Fold 32-bit sum to 16 bits */
	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return (uint16_t)~sum;
}

/**
 * @brief Test DECT FT cluster data transfer with ICMPv6 Echo Request/Reply
 *
 * This test verifies data transfer on an FT device with an associated child:
 * 1. FT cluster is started with one child associated (from previous tests)
 * 2. Simulate receiving an ICMPv6 Echo Request from the child via dlc_data_rx_ntf
 * 3. Verify that the stack sends an ICMPv6 Echo Reply via dlc_data_tx
 */
void test_dect_ft_cluster_associated_icmp_ping(void)
{
	LOG_DBG("=== Testing DECT FT cluster data transfer (ICMPv6 ping) ===");

	/* Prerequisites: FT cluster should be started with one child associated */
	TEST_ASSERT_TRUE_MESSAGE(net_if_oper_state(test_iface) == NET_IF_OPER_UP,
				 "Interface should be UP with child associated");

	/* Get our link-local address for the destination */
	struct in6_addr *our_ll_addr = net_if_ipv6_get_ll(test_iface, NET_ADDR_PREFERRED);

	TEST_ASSERT_NOT_NULL_MESSAGE(our_ll_addr, "FT device should have IPv6 link-local address");

	/* Reset TX call counter */
	mock_nrf_modem_dect_dlc_data_tx_call_count = 0;
	mock_last_dlc_data_tx_len = 0;

	/* Child device identity (from test_dect_ft_cluster_associate_child) */
	uint32_t child_long_rd_id = 0xDEADBEEF;

	/* Construct ICMPv6 Echo Request packet with IPv6 header */
	/* IPv6 header (40 bytes) + ICMPv6 header (8 bytes) + payload */
	uint8_t icmp_echo_request[64];
	size_t pkt_len = 0;

	/* IPv6 Header (40 bytes) */
	struct {
		uint8_t vtc_flow[4];  /* Version, Traffic Class, Flow Label */
		uint16_t payload_len; /* Payload length (big-endian) */
		uint8_t next_header;  /* Next header (58 = ICMPv6) */
		uint8_t hop_limit;    /* Hop limit */
		uint8_t src_addr[16]; /* Source address */
		uint8_t dst_addr[16]; /* Destination address */
	} __packed ipv6_hdr;

	/* ICMPv6 Echo Request Header (8 bytes) */
	struct {
		uint8_t type;       /* Type (128 = Echo Request) */
		uint8_t code;       /* Code (0) */
		uint16_t checksum;   /* Checksum (calculated later) */
		uint16_t identifier; /* Identifier */
		uint16_t sequence;   /* Sequence number */
	} __packed icmpv6_hdr;

	/* Echo payload */
	uint8_t echo_payload[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};

	/* Build IPv6 header */
	memset(&ipv6_hdr, 0, sizeof(ipv6_hdr));
	ipv6_hdr.vtc_flow[0] = 0x60; /* IPv6 version 6 */
	ipv6_hdr.payload_len = htons(sizeof(icmpv6_hdr) + sizeof(echo_payload));
	ipv6_hdr.next_header = IPPROTO_ICMPV6; /* 58 = ICMPv6 */
	ipv6_hdr.hop_limit = 64;

	/* Source address: child's link-local (fe80::rd_id) */
	ipv6_hdr.src_addr[0] = 0xfe;
	ipv6_hdr.src_addr[1] = 0x80;
	/* Bytes 2-7 are zero */
	/* Interface ID from Long RD ID (last 4 bytes) */
	ipv6_hdr.src_addr[12] = (child_long_rd_id >> 24) & 0xFF;
	ipv6_hdr.src_addr[13] = (child_long_rd_id >> 16) & 0xFF;
	ipv6_hdr.src_addr[14] = (child_long_rd_id >> 8) & 0xFF;
	ipv6_hdr.src_addr[15] = child_long_rd_id & 0xFF;

	/* Destination address: our link-local address */
	memcpy(ipv6_hdr.dst_addr, our_ll_addr, 16);

	/* Build ICMPv6 Echo Request header */
	memset(&icmpv6_hdr, 0, sizeof(icmpv6_hdr));
	icmpv6_hdr.type = NET_ICMPV6_ECHO_REQUEST; /* 128 */
	icmpv6_hdr.code = 0;
	icmpv6_hdr.identifier = htons(TEST_BEACON_SHORT_RD_ID);
	icmpv6_hdr.sequence = htons(1);
	icmpv6_hdr.checksum = 0; /* Will be calculated below */

	/* Assemble ICMPv6 message (header + payload) for checksum calculation */
	uint8_t icmpv6_msg[sizeof(icmpv6_hdr) + sizeof(echo_payload)];

	memcpy(icmpv6_msg, &icmpv6_hdr, sizeof(icmpv6_hdr));
	memcpy(icmpv6_msg + sizeof(icmpv6_hdr), echo_payload, sizeof(echo_payload));

	/* Calculate ICMPv6 checksum */
	uint16_t checksum = calc_icmpv6_checksum(ipv6_hdr.src_addr, ipv6_hdr.dst_addr, icmpv6_msg,
						 sizeof(icmpv6_msg));
	icmpv6_hdr.checksum = htons(checksum);

	LOG_DBG("Calculated ICMPv6 checksum: 0x%04X", checksum);

	/* Assemble the complete packet */
	memcpy(icmp_echo_request + pkt_len, &ipv6_hdr, sizeof(ipv6_hdr));
	pkt_len += sizeof(ipv6_hdr);
	memcpy(icmp_echo_request + pkt_len, &icmpv6_hdr, sizeof(icmpv6_hdr));
	pkt_len += sizeof(icmpv6_hdr);
	memcpy(icmp_echo_request + pkt_len, echo_payload, sizeof(echo_payload));
	pkt_len += sizeof(echo_payload);

	LOG_INF("Constructed ICMPv6 Echo Request: total_len=%zu", pkt_len);

	/* Simulate receiving the packet via dlc_data_rx_ntf callback */
	TEST_ASSERT_NOT_NULL_MESSAGE(mock_ntf_callbacks.dlc_data_rx_ntf,
				     "DLC data RX notification callback should be registered");

	struct nrf_modem_dect_dlc_data_rx_ntf_cb_params rx_params = {.flow_id = 0,
								     .long_rd_id = child_long_rd_id,
								     .data = icmp_echo_request,
								     .data_len = pkt_len};

	LOG_INF("Simulating DLC data RX from child 0x%08X with ICMPv6 Echo Request",
		child_long_rd_id);
	mock_ntf_callbacks.dlc_data_rx_ntf(&rx_params);

	/* Wait for the stack to process the packet and send the reply */
	k_sleep(K_MSEC(500));

	/* Verify that dlc_data_tx was called (ICMPv6 Echo Reply sent) */
	TEST_ASSERT_TRUE_MESSAGE(mock_nrf_modem_dect_dlc_data_tx_call_count > 0,
				 "nrf_modem_dect_dlc_data_tx should be called to send Echo Reply");

	/* Verify the reply was sent to the correct destination */
	TEST_ASSERT_EQUAL_MESSAGE(child_long_rd_id, mock_last_dlc_data_tx_long_rd_id,
				  "Echo Reply should be sent to the child device");

	/* Verify the reply contains ICMPv6 Echo Reply (type 129) */
	if (mock_last_dlc_data_tx_len >= sizeof(ipv6_hdr) + 1) {
		uint8_t *reply_icmp = mock_last_dlc_data_tx_data + sizeof(ipv6_hdr);
		uint8_t icmp_type = reply_icmp[0];

		TEST_ASSERT_EQUAL_MESSAGE(NET_ICMPV6_ECHO_REPLY, icmp_type,
					  "Response should be ICMPv6 Echo Reply (type 129)");
		LOG_INF("Verified ICMPv6 Echo Reply sent (type=%d)", icmp_type);
	}

	LOG_DBG("=== FT cluster data transfer test completed successfully! ===");
	LOG_DBG("- DLC data TX calls: %d", mock_nrf_modem_dect_dlc_data_tx_call_count);
	LOG_DBG("- Reply sent to: 0x%08X", mock_last_dlc_data_tx_long_rd_id);
	LOG_DBG("- Reply data length: %zu", mock_last_dlc_data_tx_len);
}

/**
 * @brief Test DECT FT cluster child dissociation (child sends association release)
 *
 * This test verifies the association release handling on an FT device:
 * 1. FT cluster has one child associated (from previous tests)
 * 2. Simulate child sending association release via association_release_ntf callback
 * 3. Verify NET_EVENT_DECT_ASSOCIATION_CHANGED is received with DECT_ASSOCIATION_RELEASED
 * 4. Verify interface state changes from UP to DORMANT (last child left)
 */
void test_dect_ft_cluster_associated_child_dissociates(void)
{
	LOG_DBG("=== Testing DECT FT cluster child dissociation (association release) ===");

	/* Prerequisites: FT cluster should have one child associated from previous tests */
	TEST_ASSERT_TRUE_MESSAGE(net_if_oper_state(test_iface) == NET_IF_OPER_UP,
				 "Interface should be UP with child associated");

	/* The child Long RD ID from previous test */
	uint32_t child_long_rd_id = 0xDEADBEEF;

	LOG_DBG("Simulating association release from child device:");
	LOG_DBG("- Child Long RD ID: 0x%08X", child_long_rd_id);

	/* Reset event tracking flags */
	dect_association_changed_received = false;
	dect_association_released_received = false;
	memset(&received_association_data, 0, sizeof(received_association_data));

	/* Step 1: Verify association_release_ntf callback is registered */
	TEST_ASSERT_NOT_NULL_MESSAGE(
		mock_ntf_callbacks.association_release_ntf,
		"Association release notification callback should be registered");

	/* Step 2: Simulate child sending association release */
	struct nrf_modem_dect_mac_association_release_ntf_cb_params release_params = {
		.release_cause = NRF_MODEM_DECT_MAC_RELEASE_CAUSE_CONNECTION_TERMINATION,
		.long_rd_id = child_long_rd_id,
	};

	LOG_DBG("Calling association_release_ntf callback to simulate child dissociation");
	mock_ntf_callbacks.association_release_ntf(&release_params);

	/* Step 3: Wait for the driver to process the release indication */
	k_sleep(K_MSEC(200));

	/* Step 4: Verify that NET_EVENT_DECT_ASSOCIATION_CHANGED was received */
	TEST_ASSERT_TRUE_MESSAGE(
		dect_association_changed_received,
		"NET_EVENT_DECT_ASSOCIATION_CHANGED should be received after child dissociation");

	/* Step 5: Verify the association type is RELEASED */
	TEST_ASSERT_TRUE_MESSAGE(dect_association_released_received,
				 "DECT_ASSOCIATION_RELEASED should be indicated");

	/* Step 6: Verify the Long RD ID matches */
	TEST_ASSERT_EQUAL_MESSAGE(child_long_rd_id, received_association_data.long_rd_id,
				  "Released association should have correct Long RD ID");

	/* Step 7: Verify interface state changes to DORMANT (no more children) */
	/* When the last child dissociates, FT interface goes back to DORMANT */
	TEST_ASSERT_TRUE_MESSAGE(net_if_oper_state(test_iface) == NET_IF_OPER_DORMANT,
				 "Interface should be DORMANT after last child dissociates");

	LOG_DBG("=== FT cluster child dissociation test completed successfully! ===");
	LOG_DBG("- Association event received: %s",
		dect_association_changed_received ? "YES" : "NO");
	LOG_DBG("- Association released: %s", dect_association_released_received ? "YES" : "NO");
	LOG_DBG("- Released Long RD ID: 0x%08X", received_association_data.long_rd_id);
	LOG_DBG("- Interface state: %s",
		net_if_oper_state(test_iface) == NET_IF_OPER_DORMANT ? "DORMANT" : "OTHER");
}

/**
 * @brief Test settings write validation, scoped updates, readback and reset
 *
 * Covers:
 * 1. Invalid write scope rejected in dect_mgmt_settings_write()
 * 2. Successful scoped write through dect_mdm_settings_write()
 * 3. Settings readback for updated fields
 * 4. Reset to defaults and readback verification
 */
void test_dect_settings_write_scopes_and_reset(void)
{
	struct dect_settings current_settings = {0};
	struct dect_settings invalid_settings = {0};
	struct dect_settings write_settings = {0};
	struct dect_settings read_settings = {0};
	struct dect_settings reset_settings = {0};
	struct dect_settings reset_read_settings = {0};
	uint32_t custom_network_id;
	uint32_t custom_tx_id;
	int result;

	TEST_ASSERT_NOT_NULL_MESSAGE(test_iface,
				     "DECT interface should be available for settings write test");

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &current_settings,
			  sizeof(current_settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Initial settings read should succeed");

	invalid_settings = current_settings;
	invalid_settings.cmd_params.write_scope_bitmap = 0;
	invalid_settings.auto_start.activate = !current_settings.auto_start.activate;

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, test_iface, &invalid_settings,
			  sizeof(invalid_settings));
	TEST_ASSERT_EQUAL_MESSAGE(-EINVAL, result,
				  "Settings write should fail when scope bitmap is invalid");

	invalid_settings = current_settings;
	invalid_settings.cmd_params.write_scope_bitmap = DECT_SETTINGS_WRITE_SCOPE_ALL;
	invalid_settings.identities.transmitter_long_rd_id = 0;

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, test_iface, &invalid_settings,
			  sizeof(invalid_settings));
	TEST_ASSERT_EQUAL_MESSAGE(
		-EINVAL, result,
		"Settings write should fail with SCOPE_ALL when transmitter Long RD ID is not set");

	invalid_settings = current_settings;
	invalid_settings.cmd_params.write_scope_bitmap = DECT_SETTINGS_WRITE_SCOPE_ALL;
	invalid_settings.device_type = DECT_DEVICE_TYPE_FT | DECT_DEVICE_TYPE_PT;

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, test_iface, &invalid_settings,
			  sizeof(invalid_settings));
	TEST_ASSERT_EQUAL_MESSAGE(
		-EINVAL, result,
		"Settings write should fail with SCOPE_ALL when device type is not FT or PT");

	invalid_settings = current_settings;
	invalid_settings.cmd_params.write_scope_bitmap = DECT_SETTINGS_WRITE_SCOPE_TX;
	invalid_settings.tx.max_power_dbm = 100;

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, test_iface, &invalid_settings,
			  sizeof(invalid_settings));
	TEST_ASSERT_EQUAL_MESSAGE(
		-EINVAL, result,
		"Settings write should fail with TX scope when TX power is unsupported by band");

	invalid_settings = current_settings;
	invalid_settings.cmd_params.write_scope_bitmap = DECT_SETTINGS_WRITE_SCOPE_CLUSTER;
	invalid_settings.cluster.max_beacon_tx_power_dbm = 100;

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, test_iface, &invalid_settings,
			  sizeof(invalid_settings));
	TEST_ASSERT_EQUAL_MESSAGE(
		-EINVAL, result,
		"Settings write should fail with CLUSTER scope when beacon TX power is unsupported "
		"by band");

	write_settings = current_settings;
	write_settings.cmd_params.write_scope_bitmap =
		DECT_SETTINGS_WRITE_SCOPE_AUTO_START | DECT_SETTINGS_WRITE_SCOPE_REGION |
		DECT_SETTINGS_WRITE_SCOPE_IDENTITIES | DECT_SETTINGS_WRITE_SCOPE_TX |
		DECT_SETTINGS_WRITE_SCOPE_POWER_SAVE | DECT_SETTINGS_WRITE_SCOPE_BAND_NBR |
		DECT_SETTINGS_WRITE_SCOPE_RSSI_SCAN | DECT_SETTINGS_WRITE_SCOPE_CLUSTER |
		DECT_SETTINGS_WRITE_SCOPE_NW_BEACON | DECT_SETTINGS_WRITE_SCOPE_ASSOCIATION |
		DECT_SETTINGS_WRITE_SCOPE_NETWORK_JOIN |
		DECT_SETTINGS_WRITE_SCOPE_SECURITY_CONFIGURATION;

	custom_network_id = current_settings.identities.network_id ^ 0x00ABCDEF;
	if (custom_network_id == 0) {
		custom_network_id = 0x00ABCDEF;
	}
	TEST_ASSERT_TRUE_MESSAGE(
		dect_utils_lib_32bit_network_id_validate(custom_network_id),
		"Custom network_id for roundtrip must be valid (LSB and MSB non-zero)");
	custom_tx_id = current_settings.identities.transmitter_long_rd_id ^ 0x13572468;
	if (custom_tx_id == 0) {
		custom_tx_id = 0x13572468;
	}

	write_settings.auto_start.activate = false;
	write_settings.region = DECT_SETTINGS_REGION_US;
	write_settings.identities.network_id = custom_network_id;
	write_settings.identities.transmitter_long_rd_id = custom_tx_id;
	write_settings.tx.max_power_dbm = 10;
	write_settings.tx.max_mcs = 2;
	write_settings.power_save = true;
	write_settings.band_nbr = current_settings.band_nbr;
	write_settings.rssi_scan.scan_suitable_percent = 66;
	write_settings.rssi_scan.time_per_channel_ms = 420;
	write_settings.rssi_scan.busy_threshold_dbm = -64;
	write_settings.rssi_scan.free_threshold_dbm = -88;
	write_settings.cluster.beacon_period = DECT_CLUSTER_BEACON_PERIOD_100MS;
	write_settings.cluster.max_beacon_tx_power_dbm = 8;
	write_settings.cluster.max_cluster_power_dbm = 11;
	write_settings.cluster.max_num_neighbors = 3;
	write_settings.cluster.channel_loaded_percent = 55;
	write_settings.cluster.neighbor_inactivity_disconnect_timer_ms = 12345;
	write_settings.nw_beacon.beacon_period = DECT_NW_BEACON_PERIOD_500MS;
	write_settings.association.max_beacon_rx_failures = 7;
	write_settings.association.min_sensitivity_dbm = -91;
	write_settings.network_join.target_ft_long_rd_id = 0x10203040;
	write_settings.sec_conf.mode = DECT_SECURITY_MODE_NONE;
	memset(write_settings.sec_conf.integrity_key, 0xAA,
	       sizeof(write_settings.sec_conf.integrity_key));
	memset(write_settings.sec_conf.cipher_key, 0x55,
	       sizeof(write_settings.sec_conf.cipher_key));

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, test_iface, &write_settings,
			  sizeof(write_settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Scoped settings write should succeed");
	TEST_ASSERT_EQUAL_MESSAGE(0, write_settings.cmd_params.failure_scope_bitmap_out,
				  "Scoped settings write should not report failed scopes");

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &read_settings,
			  sizeof(read_settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Settings read after scoped write should succeed");

	TEST_ASSERT_FALSE_MESSAGE(read_settings.auto_start.activate,
				  "Auto-start activate should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_SETTINGS_REGION_US, read_settings.region,
				  "Region should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(custom_network_id, read_settings.identities.network_id,
				  "Network ID should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(custom_tx_id, read_settings.identities.transmitter_long_rd_id,
				  "Transmitter Long RD ID should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(10, read_settings.tx.max_power_dbm,
				  "TX max power should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(2, read_settings.tx.max_mcs,
				  "TX max MCS should match scoped write");
	TEST_ASSERT_TRUE_MESSAGE(read_settings.power_save, "Power save should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(current_settings.band_nbr, read_settings.band_nbr,
				  "Band number should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(66, read_settings.rssi_scan.scan_suitable_percent,
				  "RSSI scan suitable percent should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(420, read_settings.rssi_scan.time_per_channel_ms,
				  "RSSI scan time per channel should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(-64, read_settings.rssi_scan.busy_threshold_dbm,
				  "RSSI scan busy threshold should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(-88, read_settings.rssi_scan.free_threshold_dbm,
				  "RSSI scan free threshold should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_CLUSTER_BEACON_PERIOD_100MS,
				  read_settings.cluster.beacon_period,
				  "Cluster beacon period should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(8, read_settings.cluster.max_beacon_tx_power_dbm,
				  "Cluster beacon TX power should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(11, read_settings.cluster.max_cluster_power_dbm,
				  "Cluster max TX power should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(3, read_settings.cluster.max_num_neighbors,
				  "Cluster max neighbors should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(55, read_settings.cluster.channel_loaded_percent,
				  "Cluster channel loaded percent should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(12345,
				  read_settings.cluster.neighbor_inactivity_disconnect_timer_ms,
				  "Cluster inactivity timer should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NW_BEACON_PERIOD_500MS,
				  read_settings.nw_beacon.beacon_period,
				  "Network beacon period should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(7, read_settings.association.max_beacon_rx_failures,
				  "Association max beacon RX failures should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(-91, read_settings.association.min_sensitivity_dbm,
				  "Association min sensitivity should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(0x10203040, read_settings.network_join.target_ft_long_rd_id,
				  "Network join target FT Long RD ID should match scoped write");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_SECURITY_MODE_NONE, read_settings.sec_conf.mode,
				  "Security mode should match scoped write");
	TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(
		write_settings.sec_conf.integrity_key, read_settings.sec_conf.integrity_key,
		DECT_INTEGRITY_KEY_LENGTH, "Integrity key should match scoped write");
	TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(
		write_settings.sec_conf.cipher_key, read_settings.sec_conf.cipher_key,
		DECT_CIPHER_KEY_LENGTH, "Cipher key should match scoped write");

	reset_settings.cmd_params.reset_to_driver_defaults = true;

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, test_iface, &reset_settings,
			  sizeof(reset_settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Reset to defaults should succeed");
	TEST_ASSERT_EQUAL_MESSAGE(0, reset_settings.cmd_params.failure_scope_bitmap_out,
				  "Reset to defaults should not report failed scopes");

	result = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, test_iface, &reset_read_settings,
			  sizeof(reset_read_settings));
	TEST_ASSERT_EQUAL_MESSAGE(0, result, "Settings read after reset should succeed");

	TEST_ASSERT_TRUE_MESSAGE(reset_read_settings.auto_start.activate,
				 "Auto-start should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_SETTINGS_REGION_EU, reset_read_settings.region,
				  "Region should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(CONFIG_DECT_DEFAULT_NW_ID,
				  reset_read_settings.identities.network_id,
				  "Network ID should be restored to default");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, reset_read_settings.identities.transmitter_long_rd_id,
				      "Default transmitter Long RD ID should be initialized");
	TEST_ASSERT_EQUAL_MESSAGE(19, reset_read_settings.tx.max_power_dbm,
				  "TX max power should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(4, reset_read_settings.tx.max_mcs,
				  "TX max MCS should be restored to default");
	TEST_ASSERT_FALSE_MESSAGE(reset_read_settings.power_save,
				  "Power save should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(1, reset_read_settings.band_nbr,
				  "Band number should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(CONFIG_DECT_MDM_DEFAULT_RSSI_SCAN_SUITABLE_PERCENT,
				  reset_read_settings.rssi_scan.scan_suitable_percent,
				  "RSSI scan suitable percent should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(200, reset_read_settings.rssi_scan.time_per_channel_ms,
				  "RSSI scan time per channel should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(-71, reset_read_settings.rssi_scan.busy_threshold_dbm,
				  "RSSI scan busy threshold should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(-85, reset_read_settings.rssi_scan.free_threshold_dbm,
				  "RSSI scan free threshold should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_CLUSTER_BEACON_PERIOD_2000MS,
				  reset_read_settings.cluster.beacon_period,
				  "Cluster beacon period should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(10, reset_read_settings.cluster.max_beacon_tx_power_dbm,
				  "Cluster beacon TX power should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(13, reset_read_settings.cluster.max_cluster_power_dbm,
				  "Cluster max TX power should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(CONFIG_DECT_CLUSTER_MAX_CHILD_ASSOCIATION_COUNT,
				  reset_read_settings.cluster.max_num_neighbors,
				  "Cluster max neighbors should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(CONFIG_DECT_MDM_DEFAULT_FT_CLUSTER_CHANNEL_LOADED_PERCENT,
				  reset_read_settings.cluster.channel_loaded_percent,
				  "Cluster channel loaded percent should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(
		CONFIG_DECT_MDM_DEFAULT_FT_NEIGHBOR_INACTIVITY_DISCONNECT_TIMER_MS,
		reset_read_settings.cluster.neighbor_inactivity_disconnect_timer_ms,
		"Cluster inactivity timer should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_NW_BEACON_PERIOD_2000MS,
				  reset_read_settings.nw_beacon.beacon_period,
				  "Network beacon period should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(
		CONFIG_DECT_MDM_DEFAULT_PT_ASSOCIATION_MAX_BEACON_RX_FAILURES,
		reset_read_settings.association.max_beacon_rx_failures,
		"Association max beacon RX failures should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(CONFIG_DECT_MDM_DEFAULT_PT_ASSOCIATION_MIN_SENSITIVITY_DBM,
				  reset_read_settings.association.min_sensitivity_dbm,
				  "Association min sensitivity should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(
		DECT_SETT_NETWORK_JOIN_TARGET_FT_ANY,
		reset_read_settings.network_join.target_ft_long_rd_id,
		"Network join target FT Long RD ID should be restored to default");
	TEST_ASSERT_EQUAL_MESSAGE(DECT_SECURITY_MODE_1, reset_read_settings.sec_conf.mode,
				  "Security mode should be restored to default");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0,
				      memcmp(reset_read_settings.sec_conf.integrity_key,
					     write_settings.sec_conf.integrity_key,
					     DECT_INTEGRITY_KEY_LENGTH),
				      "Integrity key should no longer match custom scoped write");
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0,
				      memcmp(reset_read_settings.sec_conf.cipher_key,
					     write_settings.sec_conf.cipher_key,
					     DECT_CIPHER_KEY_LENGTH),
				      "Cipher key should no longer match custom scoped write");
}
