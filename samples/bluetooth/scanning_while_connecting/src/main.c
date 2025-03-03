/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <bluetooth/hci_vs_sdc.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);

#define MIN_CONN_INTERVAL (500) /* Minimum Connection Interval (interval_min * 1.25 ms) */
#define MAX_CONN_INTERVAL (500) /* Maximum Connection Interval (interval_min * 1.25 ms) */
#define CONN_LATENCY      (0)
#define CONN_TIMEOUT      (400) /* Supervision Timeout (timeout * 10 ms) */

typedef enum {
	/** The central scans for connectable addresses, then
	 * disables the scanner before starting to establish a
	 * connection to a scanned connectable address.
	 * Once the connection is established, the central must start the
	 * scanner again to repeat the process to establish more connections.
	 */
	SEQUENTIAL_SCAN_AND_CONNECT,

	/** The central scans for connectable addresses, and starts
	 * to establish a connection. While establishing a connection,
	 * the central can continue to scan and cache other connectable
	 * addresses. Once the pending connection is established, the
	 * central can immediately establish a connection to one of the
	 * cached addresses.
	 */
	CONCURRENT_SCAN_AND_CONNECT,

	/** The central scans for connectable addresses, and starts
	 * to establish a connection. While establishing a connection,
	 * the central can continue to scan and cache other connectable
	 * addresses. Once the pending connection is established, the
	 * central can add the cached addresses to the filter accept list, and
	 * establish a connection using a filter accept list.
	 */
	CONCURRENT_SCAN_AND_CONNECT_FILTER_ACCEPT_LIST,
} connection_establishment_mode_t;

static const connection_establishment_mode_t conn_establishment_modes[] = {
	SEQUENTIAL_SCAN_AND_CONNECT,
	CONCURRENT_SCAN_AND_CONNECT,
	CONCURRENT_SCAN_AND_CONNECT_FILTER_ACCEPT_LIST,
};

static connection_establishment_mode_t active_conn_establishment_mode;

K_SEM_DEFINE(all_devices_connected_sem, 0, 1);

K_SEM_DEFINE(all_devices_disconnected_sem, 0, 1);

/** Mutex is used to protect from concurrent reads
 * and writes in the ring buffer.
 */
K_MUTEX_DEFINE(mutex_ring_buf);

static const char adv_name[] = "Nordic Peripheral ID";
#define ADV_NAME_STR_MAX_LEN (sizeof(adv_name))

RING_BUF_DECLARE(connectable_peers_ring_buf, CONFIG_BT_MAX_CONN * sizeof(bt_addr_le_t));

static void scan_start(void);
static void scan_stop(void);

static volatile bool connection_establishment_ongoing;

static volatile uint32_t num_connections;

/** This is used to store device addresses that can be connected to later.
 *
 * This function can be changed to use another data structure
 * than a ring buffer to store the addresses.
 */
static uint32_t cache_peer_address(const bt_addr_le_t *addr)
{
	k_mutex_lock(&mutex_ring_buf, K_FOREVER);

	uint32_t bytes_written = ring_buf_put(&connectable_peers_ring_buf,
		(uint8_t *) addr, sizeof(bt_addr_le_t));

	k_mutex_unlock(&mutex_ring_buf);

	return bytes_written;
}

/** This is used to read and remove a device address from the ring buffer.
 *
 * This function can be changed to use another data structure
 * than a ring buffer to read the cached addresses.
 */
static bool read_cached_peer_addr(const bt_addr_le_t *out_addr)
{
	while (true) {
		k_mutex_lock(&mutex_ring_buf, K_FOREVER);

		uint32_t bytes_read = ring_buf_get(&connectable_peers_ring_buf,
			(uint8_t *) out_addr, sizeof(bt_addr_le_t));

		k_mutex_unlock(&mutex_ring_buf);

		if (bytes_read > 0) {
			struct bt_conn *conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, out_addr);

			if (conn) {
				/* We have already connected to this address. */
				bt_conn_unref(conn);
			} else {
				return true;
			}
		} else {
			return false;
		}
	}
}

static bool add_cached_addresses_to_filter_accept_list(void)
{
	bool addr_added_to_filter_accept_list = false;

	while (true) {
		bt_addr_le_t addr;
		bool found_address = read_cached_peer_addr(&addr);

		if (!found_address) {
			/* We don't have any peer addresses cached. */
			break;
		}

		int err = bt_le_filter_accept_list_add(&addr);

		if (err) {
			LOG_ERR("bt_le_filter_accept_list_add failed (err %d)", err);
		} else {
			addr_added_to_filter_accept_list = true;
		}
	}

	return addr_added_to_filter_accept_list;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	LOG_DBG("Disconnected from addr %s (reason %u) %s", addr_str,
		reason, bt_hci_err_to_str(reason));

	num_connections--;
	if (num_connections == 0) {
		k_sem_give(&all_devices_disconnected_sem);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	if (err) {
		LOG_WRN("Failed to connect to %s (err %u) %s", addr_str,
			err, bt_hci_err_to_str(err));
	} else {
		num_connections++;
		LOG_INF("Connected to %s, number of connections %u", addr_str, num_connections);
		if (num_connections == CONFIG_BT_MAX_CONN) {
			/** We have connected to all advertisers.
			 * Give the semaphore to move on to the
			 * next round of connecting to peer advertisers.
			 */
			k_sem_give(&all_devices_connected_sem);
			return;
		}

		struct bt_le_conn_param conn_param = {
			.interval_min = MIN_CONN_INTERVAL,
			.interval_max = MAX_CONN_INTERVAL,
			.latency = CONN_LATENCY,
			.timeout = CONN_TIMEOUT,
		};

		/** A connection parameter update with a longer connection interval
		 * is done to make more time available for scanning.
		 */
		int zephyr_err = bt_conn_le_param_update(conn, &conn_param);

		if (zephyr_err) {
			LOG_ERR("bt_conn_le_param_update failed (err %d)", zephyr_err);
		}

	}

	connection_establishment_ongoing = false;

	if (active_conn_establishment_mode == SEQUENTIAL_SCAN_AND_CONNECT) {
		scan_start();
	}

}

static void try_connect(void)
{
	if (connection_establishment_ongoing) {
		return;
	}

	connection_establishment_ongoing = true;

	/** Interval and window of the create connection parameters
	 * must be the same as the interval and window of the scanner
	 * parameters to enable scanning and connecting concurrently.
	 */
	const struct bt_conn_le_create_param create_param = {
		.options = BT_CONN_LE_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL_MIN,
		.window = BT_GAP_SCAN_FAST_INTERVAL_MIN,
		.interval_coded = 0,
		.window_coded = 0,
		.timeout = 0,
	};

	int err;
	char addr_str[BT_ADDR_LE_STR_LEN];

	if ((active_conn_establishment_mode == SEQUENTIAL_SCAN_AND_CONNECT) ||
		(active_conn_establishment_mode == CONCURRENT_SCAN_AND_CONNECT)) {

		bt_addr_le_t addr;
		bool found_addr = read_cached_peer_addr(&addr);

		if (found_addr) {
			if (active_conn_establishment_mode == SEQUENTIAL_SCAN_AND_CONNECT) {
				scan_stop();
			}

			bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));

			struct bt_conn *unused_conn = NULL;

			LOG_DBG("Connecting to %s", addr_str);

			err = bt_conn_le_create(&addr, &create_param, BT_LE_CONN_PARAM_DEFAULT,
				&unused_conn);

			if (unused_conn) {
				bt_conn_unref(unused_conn);
			}

			if (err) {

				connection_establishment_ongoing = false;
				LOG_ERR("bt_conn_le_create failed (err %d)", err);

				if (active_conn_establishment_mode == SEQUENTIAL_SCAN_AND_CONNECT) {
					scan_start();
				}
			}
		}
	} else {

		bool addr_added_to_filter_accept_list
			= add_cached_addresses_to_filter_accept_list();

		if (addr_added_to_filter_accept_list) {

			LOG_DBG("Connecting using filter accept list");

			err = bt_conn_le_create_auto(&create_param, BT_LE_CONN_PARAM_DEFAULT);
			if (err) {
				connection_establishment_ongoing = false;
				LOG_ERR("bt_conn_le_create_auto failed (err %d)", err);
			}

		} else {
			connection_establishment_ongoing = false;
		}
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static bool adv_data_parse_cb(struct bt_data *data, void *user_data)
{
	char *rcvd_name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, ADV_NAME_STR_MAX_LEN - 1);
		memcpy(rcvd_name, data->data, len);
		rcvd_name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	/** We're only interested in connectable advertisers to
	 * show faster connection establishment
	 */
	if (info->adv_type != BT_GAP_ADV_TYPE_ADV_IND &&
		info->adv_type != BT_GAP_ADV_TYPE_EXT_ADV) {
		return;
	}

	char name_str[ADV_NAME_STR_MAX_LEN] = {0};

	bt_data_parse(buf, adv_data_parse_cb, name_str);

	if (strncmp(name_str, adv_name, ADV_NAME_STR_MAX_LEN) == 0) {

		char addr_str[BT_ADDR_LE_STR_LEN] = {0};

		bt_addr_le_to_str(info->addr, addr_str, sizeof(addr_str));

		uint32_t bytes_written = cache_peer_address(info->addr);

		if (bytes_written > 0) {
			LOG_DBG("Scanned and cached connectable addr %s", addr_str);
		}

		try_connect();
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void scan_start(void)
{
	int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE_CONTINUOUS, NULL);

	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
	}
	LOG_DBG("Started scanning");
}

static void scan_stop(void)
{
	int err = bt_le_scan_stop();

	if (err) {
		LOG_ERR("Failed to stop scanning (err %d)", err);
	}
	LOG_DBG("Stopped scanning");
}

static void disconnect(struct bt_conn *conn, void *data)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	if (err) {
		LOG_ERR("Failed disconnection %s", addr);
	}
}

static void print_conn_establishment_mode(
	connection_establishment_mode_t active_conn_establishment_mode)
{
	switch (active_conn_establishment_mode) {
	case SEQUENTIAL_SCAN_AND_CONNECT:
		LOG_INF("SEQUENTIAL_SCAN_AND_CONNECT: ");
		break;
	case CONCURRENT_SCAN_AND_CONNECT:
		LOG_INF("CONCURRENT_SCAN_AND_CONNECT: ");
		break;
	case CONCURRENT_SCAN_AND_CONNECT_FILTER_ACCEPT_LIST:
		LOG_INF("CONCURRENT_SCAN_AND_CONNECT_FILTER_ACCEPT_LIST: ");
		break;
	default:
		break;
	}
}

int main(void)
{
	int err;

	int64_t uptime_start_scan_ms;
	int64_t total_uptime_create_all_connections_ms;

	err = bt_enable(NULL);

	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized\n");

	err = bt_le_scan_cb_register(&scan_callbacks);
	if (err) {
		LOG_ERR("Scan callback register failed (err %d)", err);
		return 0;
	}
	err = bt_conn_cb_register(&conn_callbacks);
	if (err) {
		LOG_ERR("Conn callback register failed (err %d)", err);
		return 0;
	}

	/* Set a small spacing between central links to leave more time for scanning. */
	sdc_hci_cmd_vs_central_acl_event_spacing_set_t event_spacing_params = {
		.central_acl_event_spacing_us = 2000,
	};

	hci_vs_sdc_central_acl_event_spacing_set(&event_spacing_params);

	/** Set a small connection event length to leave more time for scanning.
	 *
	 * This can also be set at compile time by using the Kconfig option
	 * CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT in the prj.conf file.
	 */
	sdc_hci_cmd_vs_event_length_set_t event_length_params = {
		.event_length_us = 2000,
	};

	hci_vs_sdc_event_length_set(&event_length_params);

	/** Connection event extension is turned on by default. This is turned on by default
	 * to achieve higher throughput. As a consequence, the scheduler will schedule
	 * longer connection events at the cost of scanning events. In this sample application,
	 * we try to achieve faster connection establishment, so we turn connection
	 * event extension off. Thus, the controller schedules more time for scanning.
	 *
	 * This can also be turned off at compile time by setting the Kconfig option
	 * BT_CTLR_SDC_CONN_EVENT_EXTEND_DEFAULT=n in the prj.conf file.
	 *
	 * See our scheduling documentation under nrfxlib/softdevice_controller for
	 * more information.
	 */
	sdc_hci_cmd_vs_conn_event_extend_t conn_event_extend_params = {
		.enable = 0,
	};

	hci_vs_sdc_conn_event_extend(&conn_event_extend_params);

	for (uint8_t i = 0; i < ARRAY_SIZE(conn_establishment_modes); i++) {

		active_conn_establishment_mode = conn_establishment_modes[i];

		num_connections = 0;
		connection_establishment_ongoing = false;
		ring_buf_reset(&connectable_peers_ring_buf);

		print_conn_establishment_mode(active_conn_establishment_mode);
		LOG_INF("starting sample benchmark");

		uptime_start_scan_ms = k_uptime_get();
		scan_start();

		/* Wait until the connect callback is called for all connections */
		k_sem_take(&all_devices_connected_sem, K_FOREVER);

		scan_stop();

		total_uptime_create_all_connections_ms = (k_uptime_get() - uptime_start_scan_ms);

		LOG_INF("%lld seconds to create %u connections",
			(total_uptime_create_all_connections_ms / 1000),
			CONFIG_BT_MAX_CONN);

		LOG_INF("Disconnecting connections...");
		bt_conn_foreach(BT_CONN_TYPE_LE, disconnect, NULL);

		/* Wait until the disconnect callback is called for all connections */
		k_sem_take(&all_devices_disconnected_sem, K_FOREVER);
		LOG_INF("---------------------------------------------------------------------");
		LOG_INF("---------------------------------------------------------------------");
	}
}
