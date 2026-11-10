/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Shell commands for the Wi-Fi and Short-Range coexistence sample.
 *
 * Provides Bluetooth LE throughput (bt_*) and Wi-Fi UDP throughput (wifi_run_tput)
 * commands that are run at the same time to exercise the nRF71 coexistence driver.
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/zperf.h>

#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <bluetooth/services/throughput.h>

#ifdef CONFIG_NRF71_SR_COEX_DRIVER
#include <drivers/nrf71_sr_coex/nrf71_sr_coex_sr.h>
#include "nrf71_sr_coex_internal.h"
#endif

LOG_MODULE_REGISTER(sr_coex_shell, CONFIG_LOG_DEFAULT_LEVEL);

#define DEVICE_NAME	 CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* Time to wait for a connection or for one Bluetooth LE procedure to complete. */
#define THROUGHPUT_CONFIG_TIMEOUT_SEC 20

/* Payload of one GATT write: the L2CAP MTU minus the 3-byte ATT header. */
#define BT_TPUT_WRITE_LEN (CONFIG_BT_L2CAP_TX_MTU - 3)

/*
 * Only used to set up the connection. CONFIG_INTERVAL_* is applied afterwards by
 * connection_configuration_set().
 */
#define SETUP_INTERVAL_MIN	6	/* 6 units, 7.5 ms */
#define SETUP_INTERVAL_MAX	6	/* 6 units, 7.5 ms */
#define SETUP_SUPERVISION_TIMEOUT 400 /* 400 units, 4 s */

/* Requesting frame space of 0 us will result in the minimum frame space being used. */
#define REQUEST_MIN_FRAME_SPACE_US 0

/* Largest UDP payload that still fits a 1500 byte MTU after the IPv4 and UDP headers. */
#define WIFI_TPUT_MAX_UNFRAGMENTED_PKT 1472

/* Comfortably above the iperf datagram and client headers that zperf prepends. */
#define WIFI_TPUT_MIN_PKT 64

/*
 * The Bluetooth LE specification requires:
 * supervision timeout > (1 + latency) * connection interval max * 2
 * With the interval in 1.25 ms units and the timeout in 10 ms units this is:
 * timeout * 4 > (1 + latency) * interval max
 */
BUILD_ASSERT(CONFIG_COEX_SHELL_BT_INTERVAL_MIN <= CONFIG_COEX_SHELL_BT_INTERVAL_MAX,
		 "CONFIG_COEX_SHELL_BT_INTERVAL_MIN must not be larger than "
		 "CONFIG_COEX_SHELL_BT_INTERVAL_MAX");
BUILD_ASSERT(CONFIG_COEX_SHELL_BT_SUPERVISION_TIMEOUT * 4 >
		 (1 + CONFIG_COEX_SHELL_BT_CONN_LATENCY) * CONFIG_COEX_SHELL_BT_INTERVAL_MAX,
		 "CONFIG_COEX_SHELL_BT_SUPERVISION_TIMEOUT is too short for the connection "
		 "interval and latency");
BUILD_ASSERT(SETUP_SUPERVISION_TIMEOUT * 4 >
		 (1 + CONFIG_COEX_SHELL_BT_CONN_LATENCY) * SETUP_INTERVAL_MAX,
		 "CONFIG_COEX_SHELL_BT_CONN_LATENCY is too large for the setup supervision "
		 "timeout");

/*
 * Floating point is not supported by the log (CONFIG_CBPRINTF_FP_SUPPORT is off), so
 * times are printed as whole and fractional parts using integer arithmetic.
 */
#define CONN_INTERVAL_UNIT_US	1250U /* One connection interval unit is 1.25 ms. */
#define SUPERVISION_TIMEOUT_UNIT 10U	/* One supervision timeout unit is 10 ms. */

static K_SEM_DEFINE(throughput_sem, 0, 1);

/* Written from the Bluetooth callbacks and read from the shell thread. */
static volatile bool data_length_req;
static volatile bool test_ready;
static volatile bool connect_pending;
static volatile bool role_central;
static volatile bool adv_active;
/* true while the Bluetooth stack is enabled, i.e. the SR radio is "up". */
static volatile bool bt_up;
static volatile uint8_t bt_role;

#ifdef CONFIG_NRF71_SR_COEX_DRIVER
/*
 * Set by coex_disable_wifi_sr, cleared by coex_enable_wifi_sr. While set, bringing
 * Bluetooth up must not report the SR radio as ready to the coexistence driver,
 * because that would re-enable SR coexistence.
 */
static volatile bool coex_sr_user_disabled;
#endif
static struct bt_conn *volatile default_conn;

/*
 * Uptime in ms when default_conn was established and when the throughput test started
 * writing, or -1 if not applicable. Written from both the Bluetooth callbacks and the
 * shell thread, and 64-bit accesses are not atomic, so guard them with bt_time_lock.
 */
static struct k_spinlock bt_time_lock;
static int64_t bt_conn_start_ms = -1;
static int64_t bt_test_start_ms = -1;

static struct bt_throughput throughput;
static const struct bt_uuid *uuid128 = BT_UUID_THROUGHPUT;
static struct bt_gatt_exchange_params exchange_params;

static const struct bt_le_conn_param *setup_conn_param =
	BT_LE_CONN_PARAM(SETUP_INTERVAL_MIN, SETUP_INTERVAL_MAX,
			 CONFIG_COEX_SHELL_BT_CONN_LATENCY, SETUP_SUPERVISION_TIMEOUT);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
				0xBB, 0x4A, 0xFF, 0x4F, 0xAD, 0x03, 0x41, 0x5D,
				0xA9, 0x6C, 0x9D, 0x6C, 0xDD, 0xDA, 0x83, 0x04),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct test_params {
	struct bt_le_conn_param *conn_param;
	struct bt_conn_le_phy_param *phy;
	struct bt_conn_le_data_len_param *data_len;
	uint16_t frame_space_us;
} test_params = {
	.conn_param = BT_LE_CONN_PARAM(CONFIG_COEX_SHELL_BT_INTERVAL_MIN,
				       CONFIG_COEX_SHELL_BT_INTERVAL_MAX,
				       CONFIG_COEX_SHELL_BT_CONN_LATENCY,
				       CONFIG_COEX_SHELL_BT_SUPERVISION_TIMEOUT),
	.phy = BT_CONN_LE_PHY_PARAM_2M,
	.data_len = BT_LE_DATA_LEN_PARAM_MAX,
	.frame_space_us = REQUEST_MIN_FRAME_SPACE_US,
};

/* ---- Logging helpers ---- */

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0:
		return "No packets";
	case BT_GAP_LE_PHY_1M:
		return "LE 1M";
	case BT_GAP_LE_PHY_2M:
		return "LE 2M";
	case BT_GAP_LE_PHY_CODED:
		return "LE Coded";
	default:
		return "Unknown";
	}
}

static void instruction_print(void)
{
	LOG_INF("You can use the Tab key to autocomplete your input.");
}

/* Print a connection interval given in 1.25 ms units, for example "6 (7.50 ms)". */
static void log_interval(const char *name, uint32_t units)
{
	const uint32_t us = units * CONN_INTERVAL_UNIT_US;

	LOG_INF("%s = %u (%u.%02u ms)", name, units, us / USEC_PER_MSEC,
		(us % USEC_PER_MSEC) / 10U);
}

/* Print a supervision timeout given in 10 ms units, for example "400 (4.00 s)". */
static void log_supervision_timeout(const char *name, uint32_t units)
{
	const uint32_t ms = units * SUPERVISION_TIMEOUT_UNIT;

	LOG_INF("%s = %u (%u.%02u s)", name, units, ms / MSEC_PER_SEC,
		(ms % MSEC_PER_SEC) / 10U);
}

static void log_effective_connection_params(struct bt_conn *conn, const char *stage)
{
	struct bt_conn_info info = {0};
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("Failed to get connection info (%d) at %s", err, stage);
		return;
	}

	if (info.type != BT_CONN_TYPE_LE) {
		return;
	}

	LOG_INF("%s", stage);
	log_interval("  interval", BT_GAP_US_TO_CONN_INTERVAL(info.le.interval_us));
	LOG_INF("  latency = %u", info.le.latency);
	log_supervision_timeout("  supervision", info.le.timeout);
}

static void log_connection_settings(void)
{
	log_interval("BLE setup connection interval MIN", SETUP_INTERVAL_MIN);
	log_interval("BLE setup connection interval MAX", SETUP_INTERVAL_MAX);
	LOG_INF("BLE setup connection latency = %d", CONFIG_COEX_SHELL_BT_CONN_LATENCY);
	log_supervision_timeout("BLE setup supervision timeout", SETUP_SUPERVISION_TIMEOUT);
	log_interval("BLE test connection interval MIN", CONFIG_COEX_SHELL_BT_INTERVAL_MIN);
	log_interval("BLE test connection interval MAX", CONFIG_COEX_SHELL_BT_INTERVAL_MAX);
	LOG_INF("BLE test connection latency = %d", CONFIG_COEX_SHELL_BT_CONN_LATENCY);
	log_supervision_timeout("BLE test supervision timeout",
				CONFIG_COEX_SHELL_BT_SUPERVISION_TIMEOUT);
}

/* ---- Connection reference handling ---- */

/*
 * default_conn is changed from the Bluetooth RX thread, so the shell thread must not
 * use it directly. The pointer is only ever swapped with the scheduler locked, which
 * guarantees that a reference taken in default_conn_get() is to a connection that is
 * still held by default_conn.
 */
static struct bt_conn *default_conn_swap(struct bt_conn *new_conn)
{
	struct bt_conn *old_conn;

	k_sched_lock();
	old_conn = default_conn;
	default_conn = new_conn;
	k_sched_unlock();

	return old_conn;
}

/*
 * Take a reference to the current connection, or return NULL when there is none. The
 * caller must release the returned reference with bt_conn_unref().
 */
static struct bt_conn *default_conn_get(void)
{
	struct bt_conn *conn = NULL;

	k_sched_lock();
	if (default_conn) {
		conn = bt_conn_ref(default_conn);
	}
	k_sched_unlock();

	return conn;
}

/* ---- GATT discovery and MTU exchange (central role) ---- */

static void exchange_func(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_exchange_params *params)
{
	struct bt_conn_info info = {0};
	int err;

	if (att_err) {
		LOG_ERR("MTU exchange failed (ATT err 0x%02x)", att_err);
		return;
	}

	LOG_INF("MTU exchange successful");

	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("Failed to get connection info %d", err);
		return;
	}

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		instruction_print();
		test_ready = true;
	}
}

static void discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	struct bt_throughput *tput = context;
	int err;

	LOG_INF("Service discovery completed");

	bt_gatt_dm_data_print(dm);

	err = bt_throughput_handles_assign(dm, tput);
	if (err) {
		LOG_ERR("Throughput handles assignment failed (err %d)", err);
		bt_gatt_dm_data_release(dm);
		return;
	}

	exchange_params.func = exchange_func;

	err = bt_gatt_exchange_mtu(bt_gatt_dm_conn_get(dm), &exchange_params);
	if (err) {
		LOG_ERR("MTU exchange failed (err %d)", err);
	} else {
		LOG_INF("MTU exchange pending");
	}

	bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	LOG_ERR("Service not found");
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
	LOG_ERR("Error while discovering GATT database: (%d)", err);
}

static struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error,
};

/* ---- Scanning and advertising ---- */

static void scan_filter_match(struct bt_scan_device_info *device_info,
				struct bt_scan_filter_match *filter_match,
				bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d", addr, connectable);
}

static void scan_filter_no_match(struct bt_scan_device_info *device_info,
				 bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	/* Called for every non-matching advertisement, so keep it out of the default log. */
	LOG_DBG("Filter not match. Address: %s connectable: %d", addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_ERR("Connecting failed");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match,
		scan_connecting_error, NULL);

static void scan_init(void)
{
	int err;
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = 0x0010,
		.window = 0x0010,
	};
	struct bt_scan_init_param init_param = {
		.connect_if_match = 1,
		.scan_param = &scan_param,
		.conn_param = setup_conn_param,
	};

	bt_scan_init(&init_param);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, uuid128);
	if (err) {
		LOG_ERR("Scanning filters cannot be set");
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on");
	}
}

static void scan_start(void)
{
	int err;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		LOG_ERR("Starting scanning failed (err %d)", err);
	}
}

static void adv_start(void)
{
	const struct bt_le_adv_param *adv_param =
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN,
				BT_GAP_ADV_FAST_INT_MIN_2,
				BT_GAP_ADV_FAST_INT_MAX_2,
				NULL);
	int err;

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Failed to start advertiser (%d)", err);
		return;
	}

	adv_active = true;
}

static void scan_adv_stop(void)
{
	int err;

	err = bt_scan_stop();
	if (err && err != -EALREADY) {
		LOG_ERR("Stopping scanning failed (err %d)", err);
	}

	/*
	 * bt_le_adv_stop() logs an error when no advertiser exists, so only call it
	 * when this sample started advertising and no connection has ended it.
	 */
	if (adv_active) {
		err = bt_le_adv_stop();
		if (err) {
			LOG_ERR("Stopping advertising failed (err %d)", err);
		}
		adv_active = false;
	}
}

/*
 * Restart scanning or advertising in the selected role while bt_cfg_tput is still
 * waiting for a connection.
 */
static void connect_retry(void)
{
	if (!connect_pending) {
		return;
	}

	if (role_central) {
		scan_start();
	} else {
		adv_start();
	}
}

static void select_role(bool is_central)
{
	/*
	 * bt_cfg_tput can be run again with a different role, so stop whatever the
	 * previous role left running before starting the new one.
	 */
	scan_adv_stop();

	/* Set the role first: connect_retry() runs from the Bluetooth callbacks. */
	role_central = is_central;

	if (is_central) {
		LOG_INF("Central. Starting scanning");
		scan_start();
	} else {
		LOG_INF("Peripheral. Starting advertising");
		adv_start();
	}
}

/* ---- Connection timing ---- */

/* A time in ms split for printing as HH:MM:SS.mmm. */
struct hms_time {
	unsigned int h;
	unsigned int m;
	unsigned int s;
	unsigned int ms;
};

#define HMS_FMT	    "%02u:%02u:%02u.%03u"
#define HMS_ARGS(t) (t).h, (t).m, (t).s, (t).ms

static struct hms_time hms_from_ms(int64_t ms)
{
	uint64_t total = (ms > 0) ? (uint64_t)ms : 0U;
	struct hms_time t = {
		.ms = (unsigned int)(total % MSEC_PER_SEC),
		.s = (unsigned int)((total / MSEC_PER_SEC) % 60U),
		.m = (unsigned int)((total / (60U * MSEC_PER_SEC)) % 60U),
		.h = (unsigned int)(total / (3600U * MSEC_PER_SEC)),
	};

	return t;
}

static void bt_conn_start_set(int64_t ms)
{
	k_spinlock_key_t key = k_spin_lock(&bt_time_lock);

	bt_conn_start_ms = ms;
	k_spin_unlock(&bt_time_lock, key);
}

static void bt_test_start_set(int64_t ms)
{
	k_spinlock_key_t key = k_spin_lock(&bt_time_lock);

	bt_test_start_ms = ms;
	k_spin_unlock(&bt_time_lock, key);
}

/* Print when the tracked link went down and how long it lasted. */
static void bt_disconnect_time_log(uint8_t reason)
{
	int64_t now = k_uptime_get();
	struct hms_time now_hms = hms_from_ms(now);
	int64_t conn_start;
	int64_t test_start;
	k_spinlock_key_t key = k_spin_lock(&bt_time_lock);

	conn_start = bt_conn_start_ms;
	test_start = bt_test_start_ms;
	bt_conn_start_ms = -1;
	k_spin_unlock(&bt_time_lock, key);

	LOG_INF("Disconnection time: uptime " HMS_FMT, HMS_ARGS(now_hms));

	if (conn_start >= 0) {
		struct hms_time start_hms = hms_from_ms(conn_start);
		struct hms_time up_hms = hms_from_ms(now - conn_start);

		LOG_INF("Connection time:    uptime " HMS_FMT, HMS_ARGS(start_hms));
		LOG_WRN("Bluetooth LE disconnected " HMS_FMT " (%lld ms) after connection, "
			"reason 0x%02x %s", HMS_ARGS(up_hms), now - conn_start, reason,
			bt_hci_err_to_str(reason));
	}

	if (test_start >= 0) {
		struct hms_time start_hms = hms_from_ms(test_start);
		struct hms_time run_hms = hms_from_ms(now - test_start);

		LOG_WRN("Bluetooth LE disconnected " HMS_FMT " (%lld ms) after the throughput "
			"test started at uptime " HMS_FMT, HMS_ARGS(run_hms), now - test_start,
			HMS_ARGS(start_hms));
	}
}

/* ---- Connection callbacks ---- */

static void connected(struct bt_conn *conn, uint8_t hci_err)
{
	struct bt_conn_info info = {0};
	struct hms_time conn_hms;
	int64_t now;
	int err;

	if (hci_err) {
		if (hci_err != BT_HCI_ERR_UNKNOWN_CONN_ID) {
			LOG_ERR("Connection failed, err 0x%02x %s", hci_err,
				bt_hci_err_to_str(hci_err));
		}

		/*
		 * UNKNOWN_CONN_ID means connection creation was canceled, for example
		 * because the peer did not respond in time. Scanning has stopped
		 * either way, so restart it if bt_cfg_tput is still waiting.
		 */
		connect_retry();
		return;
	}

	if (default_conn) {
		LOG_INF("Connection exists, disconnect second connection");
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	(void)default_conn_swap(bt_conn_ref(conn));
	now = k_uptime_get();
	bt_conn_start_set(now);
	conn_hms = hms_from_ms(now);

	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("Failed to get connection info %d", err);
		return;
	}

	bt_role = info.role;

	LOG_INF("Connected as %s at uptime " HMS_FMT,
		info.role == BT_CONN_ROLE_CENTRAL ? "central" : "peripheral",
		HMS_ARGS(conn_hms));

	log_effective_connection_params(conn, "Link established (setup parameters)");

	if (info.role == BT_CONN_ROLE_PERIPHERAL) {
		/* The controller stops advertising when the connection is made. */
		adv_active = false;

		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (err) {
			LOG_ERR("Failed to set security: %d", err);
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));

	/* A second connection rejected in connected() must not drop the tracked one. */
	if (conn != default_conn) {
		return;
	}

	bt_disconnect_time_log(reason);

	test_ready = false;
	bt_conn_unref(default_conn_swap(NULL));

	/* Re-connect using the same role. Retry as long as bt_cfg_tput is waiting. */
	connect_retry();
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
				enum bt_security_err security_err)
{
	struct bt_conn_info info = {0};
	int err;

	LOG_INF("Security changed: level %d, err: %d %s", level, security_err,
		bt_security_err_to_str(security_err));

	if (security_err != 0) {
		LOG_ERR("Failed to encrypt link");
		bt_conn_disconnect(conn, BT_HCI_ERR_PAIRING_NOT_SUPPORTED);
		return;
	}

	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("Failed to get connection info %d", err);
		return;
	}

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		err = bt_gatt_dm_start(conn, BT_UUID_THROUGHPUT, &discovery_cb, &throughput);
		if (err) {
			LOG_ERR("Discover failed (err %d)", err);
		}
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	LOG_INF("Connection parameters update request received");
	LOG_INF("Minimum interval: %d, Maximum interval: %d",
		param->interval_min, param->interval_max);
	LOG_INF("Latency: %d, Timeout: %d", param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
				 uint16_t latency, uint16_t timeout)
{
	LOG_INF("Connection parameters updated. interval: %d, latency: %d, timeout: %d",
		interval, latency, timeout);

	k_sem_give(&throughput_sem);
}

static void le_phy_updated(struct bt_conn *conn,
				struct bt_conn_le_phy_info *param)
{
	LOG_INF("LE PHY updated: TX PHY %s, RX PHY %s",
		phy2str(param->tx_phy), phy2str(param->rx_phy));

	k_sem_give(&throughput_sem);
}

static void le_data_length_updated(struct bt_conn *conn,
					struct bt_conn_le_data_len_info *info)
{
	if (!data_length_req) {
		return;
	}

	LOG_INF("LE data len updated: TX (len: %d time: %d) RX (len: %d time: %d)",
		info->tx_max_len, info->tx_max_time, info->rx_max_len, info->rx_max_time);

	data_length_req = false;
	k_sem_give(&throughput_sem);
}

#if defined(CONFIG_BT_FRAME_SPACE_UPDATE)
static void frame_space_updated(struct bt_conn *conn,
				const struct bt_conn_le_frame_space_updated *params)
{
	if (params->status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("Frame space update failed: %d", params->status);
		return;
	}

	LOG_INF("Frame space updated: frame space %d us, PHYs 0x%04x, spacing types 0x%04x",
		params->frame_space, params->phys, params->spacing_types);

	k_sem_give(&throughput_sem);
}
#endif /* CONFIG_BT_FRAME_SPACE_UPDATE */

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
	.le_phy_updated = le_phy_updated,
	.le_data_len_updated = le_data_length_updated,
#if defined(CONFIG_BT_FRAME_SPACE_UPDATE)
	.frame_space_updated = frame_space_updated,
#endif
	.security_changed = security_changed
};

/* ---- Throughput service callbacks ---- */

static uint8_t throughput_read(const struct bt_throughput_metrics *met)
{
	LOG_INF("[peer] received %u bytes (%u KB) in %u GATT writes at %u bps",
		met->write_len, met->write_len / 1024, met->write_count, met->write_rate);

	k_sem_give(&throughput_sem);

	return BT_GATT_ITER_STOP;
}

static void throughput_received(const struct bt_throughput_metrics *met)
{
	static uint32_t kb;

	/* One log message per KB would flood the log during the test. */
	if (met->write_len == 0) {
		kb = 0;
		printk("\n");
		return;
	}

	if ((met->write_len / 1024) != kb) {
		kb = (met->write_len / 1024);
		printk("=");
	}
}

static void throughput_send(const struct bt_throughput_metrics *met)
{
	LOG_INF("[local] received %u bytes (%u KB) in %u GATT writes at %u bps",
		met->write_len, met->write_len / 1024, met->write_count, met->write_rate);
}

static const struct bt_throughput_cb throughput_cb = {
	.data_read = throughput_read,
	.data_received = throughput_received,
	.data_send = throughput_send
};

/* ---- Bluetooth LE test setup ---- */

/*
 * One-time Bluetooth setup. The stock throughput sample is used as reference; here it
 * runs on the first bt_cfg_tput so Bluetooth stays off until it is needed.
 */
static int bt_stack_init(void)
{
	/* The scan filters and the throughput service must only be set up once. */
	static bool app_initialized;
	int err;

	if (!bt_up) {
		err = bt_enable(NULL);
		if (err && err != -EALREADY) {
			LOG_ERR("Bluetooth init failed (err %d)", err);
			return err;
		}

		bt_up = true;
		LOG_INF("Bluetooth initialized");

#ifdef CONFIG_NRF71_SR_COEX_DRIVER
		if (coex_sr_user_disabled) {
			/* The user disabled SR coexistence, do not re-enable it */
			LOG_INF("SR coexistence disabled by user, SR power notify skipped");
		} else {
			/* Coexistence driver keeps arbitration off until SR radio is up. */
			err = nrf71_sr_coex_sr_power_notify(NRF71_SR_COEX_SR_POWERED_UP_READY);
			if (err) {
				LOG_WRN("Coexistence not enabled (err %d)", err);
			}
		}
#endif
	}

	if (app_initialized) {
		return 0;
	}

	scan_init();

	err = bt_throughput_init(&throughput, &throughput_cb);
	if (err) {
		LOG_ERR("Throughput service initialization failed");
		return err;
	}

	app_initialized = true;

	return 0;
}

/*
 * Poll until a connection exists (or no longer exists) or the configuration timeout
 * expires. default_conn is updated from the Bluetooth callbacks.
 */
static bool wait_for_connection_state(bool want_connected)
{
	const k_timepoint_t end = sys_timepoint_calc(K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC));

	while ((default_conn != NULL) != want_connected) {
		if (sys_timepoint_expired(end)) {
			return false;
		}
		k_sleep(K_MSEC(100));
	}

	return true;
}

/*
 * On the central, wait for service discovery and the MTU exchange to finish
 * (test_ready) so the PHY, connection parameter and data length updates do not compete
 * with them on the link. Gives up early if the link drops.
 */
static int wait_for_test_ready(void)
{
	const k_timepoint_t end = sys_timepoint_calc(K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC));

	LOG_INF("Waiting for service discovery and MTU exchange");

	while (!test_ready) {
		if (!default_conn) {
			LOG_ERR("Disconnected before service discovery completed");
			return -ENOTCONN;
		}
		if (sys_timepoint_expired(end)) {
			LOG_ERR("Service discovery and MTU exchange timeout");
			return -ETIMEDOUT;
		}
		k_sleep(K_MSEC(100));
	}

	return 0;
}

/* Wait for the completion event of the Bluetooth LE procedure that was just started. */
static int procedure_wait(const char *name)
{
	if (k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC)) != 0) {
		LOG_ERR("%s timeout", name);
		return -ETIMEDOUT;
	}

	return 0;
}

static int connection_configuration_set(const struct bt_le_conn_param *conn_param,
					const struct bt_conn_le_phy_param *phy,
					const struct bt_conn_le_data_len_param *data_len,
					uint16_t frame_space_us)
{
	int err;
	struct bt_conn_info info = {0};
	struct bt_conn *conn = default_conn_get();

	if (!conn) {
		LOG_ERR("Not connected");
		return -ENOTCONN;
	}

	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("Failed to get connection info %d", err);
		goto out;
	}

	/*
	 * Drop any completion left over from a procedure the peer started, so each
	 * wait below only returns for the request made just before it.
	 */
	k_sem_reset(&throughput_sem);
	err = bt_conn_le_phy_update(conn, phy);
	if (err) {
		LOG_ERR("PHY update failed: %d", err);
		goto out;
	}

	LOG_INF("PHY update pending");
	err = procedure_wait("PHY update");
	if (err) {
		goto out;
	}

	/* Update when any parameter differs, not only the interval. */
	if (BT_GAP_US_TO_CONN_INTERVAL(info.le.interval_us) != conn_param->interval_max ||
		info.le.latency != conn_param->latency ||
		info.le.timeout != conn_param->timeout) {
		k_sem_reset(&throughput_sem);
		err = bt_conn_le_param_update(conn, conn_param);
		if (err) {
			LOG_ERR("Connection parameters update failed: %d", err);
			goto out;
		}

		LOG_INF("Connection parameters update pending");
		err = procedure_wait("Connection parameters update");
		if (err) {
			goto out;
		}
	}

	if (info.le.data_len->tx_max_len != data_len->tx_max_len) {
		data_length_req = true;
		k_sem_reset(&throughput_sem);

		err = bt_conn_le_data_len_update(conn, data_len);
		if (err) {
			data_length_req = false;
			LOG_ERR("LE data length update failed: %d", err);
			goto out;
		}

		LOG_INF("LE Data length update pending");
		err = procedure_wait("LE Data Length update");
		if (err) {
			data_length_req = false;
			goto out;
		}
	}

	if (IS_ENABLED(CONFIG_BT_FRAME_SPACE_UPDATE)) {
		struct bt_conn_le_frame_space_update_param fsu_params;

		fsu_params.frame_space_min = frame_space_us;
		fsu_params.frame_space_max = MAX(150, frame_space_us);
		fsu_params.phys = phy->pref_tx_phy | phy->pref_rx_phy;
		fsu_params.spacing_types = BT_CONN_LE_FRAME_SPACE_TYPES_MASK_ACL_IFS;

		k_sem_reset(&throughput_sem);
		err = bt_conn_le_frame_space_update(conn, &fsu_params);
		if (err) {
			LOG_ERR("Frame space update failed: %d", err);
			goto out;
		}

		LOG_INF("Frame space update pending");
		err = procedure_wait("Frame space update");
		if (err) {
			goto out;
		}
	}

	log_effective_connection_params(conn, "Connection configured (effective parameters)");

out:
	bt_conn_unref(conn);
	return err;
}

/*
 * Start scanning (central) or advertising (peripheral) and wait for the link. Gives up
 * after THROUGHPUT_CONFIG_TIMEOUT_SEC and stops scanning or advertising again.
 */
static int connect_in_role(bool is_central)
{
	log_connection_settings();

	connect_pending = true;
	select_role(is_central);

	LOG_INF("Waiting for connection");
	(void)wait_for_connection_state(true);
	connect_pending = false;

	if (!default_conn) {
		LOG_ERR("Cannot set up connection");
		scan_adv_stop();
		return -ENOTCONN;
	}

	return 0;
}

static int bt_throughput_test_init(bool is_central)
{
	struct bt_conn *conn;
	int err;

	err = bt_stack_init();
	if (err) {
		return err;
	}

	LOG_INF("Bluetooth LE role is %s", is_central ? "central" : "peripheral");

	conn = default_conn_get();
	if (conn) {
		const bool same_role = (bt_role == BT_CONN_ROLE_CENTRAL) == is_central;

		if (same_role) {
			LOG_INF("Already connected in this role, applying the test configuration");
			bt_conn_unref(conn);
		} else {
			LOG_INF("Connected in the other role, disconnecting to change role");
			err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			bt_conn_unref(conn);
			if (err) {
				LOG_ERR("Disconnect failed (err %d)", err);
				return err;
			}

			if (!wait_for_connection_state(false)) {
				LOG_ERR("Disconnect timeout");
				return -ETIMEDOUT;
			}
		}
	}

	if (!default_conn) {
		err = connect_in_role(is_central);
		if (err) {
			return err;
		}
	}

	if (is_central) {
		err = wait_for_test_ready();
		if (err) {
			return err;
		}
	}

	return connection_configuration_set(test_params.conn_param, test_params.phy,
						test_params.data_len, test_params.frame_space_us);
}

static int bt_throughput_test_run(const struct shell *shell,
				  const struct bt_le_conn_param *conn_param,
				  const struct bt_conn_le_phy_param *phy,
				  const struct bt_conn_le_data_len_param *data_len,
				  uint32_t ble_test_duration,
				  uint16_t frame_space_us)
{
	static char dummy[BT_TPUT_WRITE_LEN];
	struct hms_time start_hms;
	int err;
	int write_err;
	int64_t stamp;
	int64_t delta;
	uint64_t data = 0;

	if (!default_conn) {
		shell_error(shell, "Device is disconnected. "
				"Connect to the peer device before running test");
		return -ENOTCONN;
	}

	/* Only the central discovers the throughput service, so only it can write to it. */
	if (bt_role != BT_CONN_ROLE_CENTRAL) {
		shell_error(shell, "The test must be run from the central. "
				"Use 'bt_cfg_tput 1' to connect as central");
		return -ENOTSUP;
	}

	if (!test_ready) {
		shell_error(shell, "Device is not ready. "
				"Please wait for the service discovery and MTU exchange end");
		return -EAGAIN;
	}

	shell_print(shell, "\nStarting Bluetooth LE throughput test\n");

	err = connection_configuration_set(conn_param, phy, data_len, frame_space_us);
	if (err) {
		return err;
	}

	shell_print(shell, "Bluetooth LE throughput test is in progress ");
	shell_print(shell, "and requires around %u seconds to complete.\n", ble_test_duration);

	/* Make sure that all BLE procedures are finished. */
	k_sleep(K_MSEC(500));

	/* Reset the metrics on the peer. */
	err = bt_throughput_write(&throughput, dummy, 1);
	if (err) {
		shell_error(shell, "Reset peer metrics failed.");
		return err;
	}

	stamp = k_uptime_get();
	start_hms = hms_from_ms(stamp);
	bt_test_start_set(stamp);
	LOG_INF("Throughput test started at uptime " HMS_FMT, HMS_ARGS(start_hms));

	while (true) {
		write_err = bt_throughput_write(&throughput, dummy, sizeof(dummy));
		if (write_err) {
			int64_t elapsed = k_uptime_get() - stamp;
			struct hms_time elapsed_hms = hms_from_ms(elapsed);

			shell_error(shell, "GATT write failed (err %d) " HMS_FMT " (%lld ms) after "
					   "the throughput test started", write_err,
					   HMS_ARGS(elapsed_hms), elapsed);
			break;
		}
		data += sizeof(dummy);
		if (k_uptime_get() - stamp > (int64_t)ble_test_duration * MSEC_PER_SEC) {
			break;
		}
	}

	bt_test_start_set(-1);
	delta = k_uptime_delta(&stamp);

	LOG_INF("Done");
	LOG_INF("[local] sent %llu bytes (%llu KB) in %lld ms at %llu kbps",
		data, data / 1024, delta, delta > 0 ? (data * 8 / (uint64_t)delta) : 0ULL);

	/* The link is usually gone after a write error, so skip the peer read. */
	if (write_err) {
		return write_err;
	}

	/* Read the metrics back from the peer. */
	k_sem_reset(&throughput_sem);
	err = bt_throughput_read(&throughput);
	if (err) {
		shell_error(shell, "GATT read failed (err %d)", err);
		return err;
	}

	if (k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT_SEC))) {
		shell_warn(shell, "Peer metrics were not received");
	}

	instruction_print();

	return 0;
}

/* ---- Shell argument parsing ---- */

/* Parse a decimal shell argument that must be in the range min..max. */
static int parse_ulong_range(const struct shell *shell, const char *name, const char *arg,
				 unsigned long min, unsigned long max, unsigned long *out)
{
	char *end = NULL;
	unsigned long value = strtoul(arg, &end, 10);

	if (end == arg || *end != '\0' || value < min || value > max) {
		shell_error(shell, "%s: '%s' must be a number from %lu to %lu",
				name, arg, min, max);
		return -EINVAL;
	}

	*out = value;

	return 0;
}

/* Parse a shell argument that must be exactly "0" or "1". */
static int parse_flag(const struct shell *shell, const char *name, const char *arg, bool *out)
{
	char *end = NULL;
	unsigned long value = strtoul(arg, &end, 10);

	if (end == arg || *end != '\0' || value > 1) {
		shell_error(shell, "%s: '%s' must be 0 or 1", name, arg);
		return -EINVAL;
	}

	*out = (value == 1);

	return 0;
}

/* Parse a packet size given either as plain bytes ("1024") or kilobytes ("1K"). */
static int parse_packet_size(const struct shell *shell, const char *arg, uint16_t *out)
{
	char *end = NULL;
	unsigned long value = strtoul(arg, &end, 10);
	unsigned long bytes;

	if (end == arg) {
		shell_error(shell, "pktSize: '%s' is not a number", arg);
		return -EINVAL;
	}

	if (*end == 'K' || *end == 'k') {
		/* Clamp before scaling so a huge value cannot wrap into the valid range. */
		bytes = MIN(value, CONFIG_NET_ZPERF_MAX_PACKET_SIZE + 1UL) * 1024;
		end++;
	} else {
		bytes = value;
	}

	if (*end != '\0') {
		shell_error(shell,
				"pktSize: trailing characters in '%s', expected bytes or a K suffix",
				arg);
		return -EINVAL;
	}

	if (bytes < WIFI_TPUT_MIN_PKT || bytes > CONFIG_NET_ZPERF_MAX_PACKET_SIZE) {
		shell_error(shell, "pktSize: %lu out of range, must be %d to %d bytes",
				bytes, WIFI_TPUT_MIN_PKT, CONFIG_NET_ZPERF_MAX_PACKET_SIZE);
		shell_error(shell, "zperf silently clamps above its maximum, "
				"so the request is rejected here");
		return -EINVAL;
	}

	if (bytes > WIFI_TPUT_MAX_UNFRAGMENTED_PKT) {
		shell_warn(shell, "pktSize: %lu exceeds %d, datagrams will be IP fragmented",
				bytes, WIFI_TPUT_MAX_UNFRAGMENTED_PKT);
	}

	*out = (uint16_t)bytes;

	return 0;
}

/* Parse a rate given in bit/s ("10000000"), kbit/s ("10000K") or Mbit/s ("10M"). */
static int parse_rate_kbps(const struct shell *shell, const char *arg, uint32_t *out)
{
	char *end = NULL;
	unsigned long value = strtoul(arg, &end, 10);
	unsigned long kbps;

	if (end == arg) {
		shell_error(shell, "baudrate: '%s' is not a number", arg);
		return -EINVAL;
	}

	if (*end == 'M' || *end == 'm') {
		if (value > UINT32_MAX / 1000) {
			shell_error(shell, "baudrate: '%s' is too large", arg);
			return -EINVAL;
		}
		kbps = value * 1000;
		end++;
	} else if (*end == 'K' || *end == 'k') {
		kbps = value;
		end++;
	} else {
		kbps = value / 1000;
	}

	if (*end != '\0' || kbps == 0) {
		shell_error(shell,
				"baudrate: '%s' must be at least 1K, with an optional K or M suffix",
				arg);
		return -EINVAL;
	}

	*out = (uint32_t)kbps;

	return 0;
}

/* ---- Bluetooth LE shell commands ---- */

#if defined(CONFIG_BT_FRAME_SPACE_UPDATE)
static int frame_space_cmd(const struct shell *shell, size_t argc, char **argv)
{
	unsigned long frame_space_us;
	int err;

	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (argc > 2) {
		shell_error(shell, "%s: bad parameters count", argv[0]);
		return -EINVAL;
	}

	err = parse_ulong_range(shell, "frame space", argv[1], 0, 10000, &frame_space_us);
	if (err) {
		return err;
	}

	test_params.frame_space_us = (uint16_t)frame_space_us;

	shell_print(shell, "Minimum frame space: %d us", test_params.frame_space_us);

	return 0;
}
#endif /* CONFIG_BT_FRAME_SPACE_UPDATE */

static const char *phy_str(const struct bt_conn_le_phy_param *phy)
{
	switch (phy->pref_tx_phy) {
	case BT_GAP_LE_PHY_1M:
		return "1 Mbps";
	case BT_GAP_LE_PHY_2M:
		return "2 Mbps";
	case BT_GAP_LE_PHY_CODED:
		if (phy->options == BT_CONN_LE_PHY_OPT_CODED_S2) {
			return "Coded S2";
		} else if (phy->options == BT_CONN_LE_PHY_OPT_CODED_S8) {
			return "Coded S8";
		}
		__fallthrough;
	default:
		return "Unknown";
	}
}

static int print_bt_configuration(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "==== Current BT throughput configuration ====");
	shell_print(shell, "Data length:\t\t%d", test_params.data_len->tx_max_len);
	shell_print(shell, "Connection interval:\t%d units",
			test_params.conn_param->interval_min);
	shell_print(shell, "Supervision timeout:\t%d units",
			test_params.conn_param->timeout);
	shell_print(shell, "Preferred PHY:\t\t%s", phy_str(test_params.phy));
#if defined(CONFIG_BT_FRAME_SPACE_UPDATE)
	shell_print(shell, "Frame space:\t\t%d us", test_params.frame_space_us);
#endif

	return 0;
}

static int bt_configure_throughput(const struct shell *shell, size_t argc, char **argv)
{
	bool is_central;
	int ret;

	if (argc < 2) {
		shell_error(shell, "invalid # of args : %zu", argc);
		shell_print(shell, "Usage: bt_cfg_tput bt_role\n");
		shell_print(shell, "		bt_role: 1 for central, 0 for peripheral");
		return -ENOEXEC;
	}

	ret = parse_flag(shell, "bt_role", argv[1], &is_central);
	if (ret) {
		shell_print(shell, "		bt_role: 1 for central, 0 for peripheral");
		return ret;
	}

	ret = bt_throughput_test_init(is_central);
	if (ret) {
		shell_error(shell, "Bluetooth LE tput config - FAIL");
	} else {
		shell_print(shell, "Bluetooth LE tput config - SUCCESS");
	}

	return ret;
}

static int bt_run_throughput(const struct shell *shell, size_t argc, char **argv)
{
	unsigned long ble_test_duration;
	int err;

	if (argc < 2) {
		shell_error(shell, "invalid # of args : %zu", argc);
		shell_print(shell, "Usage: bt_run_tput ble_test_duration\n");
		shell_print(shell, "		ble_test_duration: Test duration in seconds");
		return -ENOEXEC;
	}

	err = parse_ulong_range(shell, "ble_test_duration", argv[1], 1,
				UINT32_MAX / MSEC_PER_SEC, &ble_test_duration);
	if (err) {
		return err;
	}

	err = bt_throughput_test_run(shell, test_params.conn_param, test_params.phy,
					 test_params.data_len, (uint32_t)ble_test_duration,
					 test_params.frame_space_us);
	if (err) {
		shell_error(shell, "Bluetooth LE throughput - FAIL");
	} else {
		shell_print(shell, "Bluetooth LE throughput - SUCCESS");
	}

	return err;
}

static int bt_disconnection(const struct shell *shell, size_t argc, char **argv)
{
	struct bt_conn *conn;
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	conn = default_conn_get();
	if (!conn) {
		shell_error(shell, "Not connected!");
		return -ENOTCONN;
	}

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	bt_conn_unref(conn);
	if (err) {
		shell_error(shell, "Bluetooth LE disconnection - FAIL");
	} else {
		shell_print(shell, "Bluetooth LE disconnection - SUCCESS");
	}

	return err;
}

/* ---- Wi-Fi throughput shell commands ---- */

/* Default iperf port, used for the zperf server when no port is given. */
#define WIFI_TPUT_DEFAULT_PORT 5001

/* Rate in kbps, with 1 kbit = 1024 bits, of len bytes transferred in time_us. */
static uint32_t wifi_tput_rate_kbps(uint64_t len, uint64_t time_us)
{
	if (time_us == 0U) {
		return 0U;
	}

	return (uint32_t)((len * 8U * USEC_PER_SEC) / (time_us * 1024U));
}

static void udp_upload_results_cb(enum zperf_status status,
				struct zperf_results *result,
				void *user_data)
{
	uint32_t client_rate_in_kbps = 0U;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New UDP session started");
		break;
	case ZPERF_SESSION_PERIODIC_RESULT:
		/* Ignored. */
		break;
	case ZPERF_SESSION_FINISHED:
		LOG_INF("Wi-Fi benchmark: Upload completed!");
		if (!result) {
			LOG_ERR("Result is NULL, Zperf session error");
			break;
		}

		client_rate_in_kbps = wifi_tput_rate_kbps(
			(uint64_t)result->nb_packets_sent * result->packet_size,
			result->client_time_in_us);

		LOG_INF("Upload results:");
		LOG_INF("%llu bytes in %llu ms",
			(uint64_t)result->nb_packets_sent * result->packet_size,
			(result->client_time_in_us / USEC_PER_MSEC));
		LOG_INF("%u packets sent", result->nb_packets_sent);
		LOG_INF("%u packets lost", result->nb_packets_lost);
		LOG_INF("%u packets received", result->nb_packets_rcvd);
		LOG_INF("Client rate: %u kbps", client_rate_in_kbps);
		break;
	case ZPERF_SESSION_ERROR:
		LOG_ERR("UDP session error");
		break;
	}
}

#if defined(CONFIG_NET_ZPERF_SERVER)
static void udp_download_results_cb(enum zperf_status status,
				struct zperf_results *result,
				void *user_data)
{
	ARG_UNUSED(user_data);

	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New UDP session started");
		break;
	case ZPERF_SESSION_PERIODIC_RESULT:
		/* Ignored. */
		break;
	case ZPERF_SESSION_FINISHED:
		LOG_INF("Wi-Fi benchmark: UDP download completed!");
		if (!result) {
			LOG_ERR("Result is NULL, Zperf session error");
			break;
		}

		LOG_INF("Download results:");
		LOG_INF("%llu bytes in %llu ms", result->total_len,
			(result->time_in_us / USEC_PER_MSEC));
		LOG_INF("%u packets received", result->nb_packets_rcvd);
		LOG_INF("%u packets lost", result->nb_packets_lost);
		LOG_INF("%u packets out of order", result->nb_packets_outorder);
		LOG_INF("Jitter: %u us", result->jitter_in_us);
		LOG_INF("Server rate: %u kbps",
			wifi_tput_rate_kbps(result->total_len, result->time_in_us));
		LOG_INF("UDP server is still running, stop it with: wifi_stop_tput udp");
		break;
	case ZPERF_SESSION_ERROR:
		LOG_ERR("UDP session error");
		break;
	}
}

#endif /* CONFIG_NET_ZPERF_SERVER */

static int parse_ipv4_addr(const char *host, struct sockaddr_in *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET, host, &addr->sin_addr);
	if (ret < 0) {
		LOG_ERR("Invalid IPv4 address %s", host);
		return -EINVAL;
	}

	LOG_INF("IPv4 address %s", host);

	return 0;
}

/* Check that the Wi-Fi interface is connected and has an IPv4 address. */
static int wifi_link_check(const struct shell *shell)
{
	struct wifi_iface_status status = { 0 };
	struct net_if *iface = net_if_get_first_wifi();
	int ret;

	if (!iface) {
		shell_error(shell, "No Wi-Fi interface found");
		return -ENODEV;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status, sizeof(status));
	if (ret) {
		shell_error(shell, "Failed to read Wi-Fi interface status (%d)", ret);
		return ret;
	}

	if (status.state < WIFI_STATE_COMPLETED) {
		shell_error(shell, "Wi-Fi is not connected (state: %s)",
				wifi_state_txt(status.state));
		shell_error(shell,
				"Connect first: wifi connect -s <SSID> -k <key_mgmt> -p <passphrase>");
		return -ENOTCONN;
	}

	if (net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED) == NULL) {
		shell_error(shell, "Wi-Fi is connected but has no IPv4 address yet");
		shell_error(shell, "Wait for DHCP to complete, then retry");
		return -EADDRNOTAVAIL;
	}

	return 0;
}

static void wifi_tput_usage(const struct shell *shell)
{
	shell_print(shell, "Usage:");
	shell_print(shell, "  wifi_run_tput udp upload <peerIP> <destPort> <duration> "
			"<pktSize> <baudrate>");
	shell_print(shell, "  wifi_run_tput tcp upload <peerIP> <destPort> <duration> <pktSize>");
	shell_print(shell, "  wifi_run_tput <udp|tcp> download [<port>]");
	shell_print(shell, "		peerIP - IPv4 address of the iperf server on the test PC");
	shell_print(shell, "		destPort - port of the iperf server");
	shell_print(shell, "		duration - test duration in seconds");
	shell_print(shell, "		pktSize - bytes (e.g.1024) or kilobytes (e.g. 1K)");
	shell_print(shell, "		baudrate - bit/s, or kbit/s or Mbit/s "
			"with a K or M suffix");
	shell_print(shell, "		port - local port of the zperf server, default %d",
			WIFI_TPUT_DEFAULT_PORT);
	shell_print(shell, "For download, the sample runs the server and the test PC runs the "
			"iperf client. Stop the server with wifi_stop_tput <udp|tcp>.");
}

/* Parse the protocol argument, "udp" or "tcp". Set *tcp to true for TCP. */
static int parse_protocol(const struct shell *shell, const char *arg, bool *tcp)
{
	if (strcmp(arg, "udp") == 0) {
		*tcp = false;
		return 0;
	}

	shell_error(shell, "protocol: '%s' must be udp or tcp", arg);

	return -EINVAL;
}

/*
 * Upload: the sample is the zperf client and sends to the iperf server on the test PC.
 * argv[0] is the peer IP, followed by the port, duration, packet size and, for UDP, the
 * rate. TCP sends as fast as the link allows, so it takes no rate.
 */
static int wifi_tput_upload(const struct shell *shell, bool tcp, size_t argc, char **argv)
{
	struct zperf_upload_params params = { 0 };
	struct sockaddr_in peer_addr = {
		.sin_family = AF_INET,
	};
	const size_t expected_argc = tcp ? 4 : 5;
	unsigned long port;
	unsigned long duration_sec;
	uint16_t packet_size = 0;
	uint32_t rate_kbps = 0;
	int ret;

	if (argc != expected_argc) {
		shell_error(shell, "invalid # of args for %s upload : %zu", tcp ? "tcp" : "udp",
				argc);
		wifi_tput_usage(shell);
		return -ENOEXEC;
	}

	if (parse_ipv4_addr(argv[0], &peer_addr) != 0) {
		shell_error(shell, "peerIP: '%s' is not a valid IPv4 address", argv[0]);
		return -EINVAL;
	}

	ret = parse_ulong_range(shell, "destPort", argv[1], 1, UINT16_MAX, &port);
	if (ret) {
		return ret;
	}

	ret = parse_ulong_range(shell, "duration", argv[2], 1, UINT32_MAX / MSEC_PER_SEC,
				&duration_sec);
	if (ret) {
		return ret;
	}

	ret = parse_packet_size(shell, argv[3], &packet_size);
	if (ret) {
		return ret;
	}

	if (!tcp) {
		ret = parse_rate_kbps(shell, argv[4], &rate_kbps);
		if (ret) {
			return ret;
		}
	}

	ret = wifi_link_check(shell);
	if (ret) {
		return ret;
	}

	peer_addr.sin_port = htons((uint16_t)port);

	params.duration_ms = (uint32_t)duration_sec * MSEC_PER_SEC;
	params.rate_kbps = rate_kbps;
	params.packet_size = packet_size;
	memcpy(&params.peer_addr, &peer_addr, sizeof(peer_addr));

	LOG_INF("Starting Wi-Fi benchmark: Zperf %s client", tcp ? "tcp" : "udp");

	ret = zperf_udp_upload_async(&params, udp_upload_results_cb, NULL);

	if (ret) {
		shell_error(shell, "Starting the %s upload failed (err %d)", tcp ? "TCP" : "UDP",
				ret);
	}

	return ret;
}

#if defined(CONFIG_NET_ZPERF_SERVER)
/* Print the iperf client command to run on the test PC for a download test. */
static void wifi_tput_print_iperf_client(const struct shell *shell, bool tcp,
					 const struct net_in_addr *addr, uint16_t port)
{
	char addr_str[NET_IPV4_ADDR_LEN] = "<DUT IP>";

	(void)net_addr_ntop(AF_INET, addr, addr_str, sizeof(addr_str));

	shell_print(shell, "On the test PC, run for example:");
	if (tcp) {
		shell_print(shell, "  iperf -c %s -i 1 --len 1024 -p %u -w 24K -t 20", addr_str,
				port);
	} else {
		shell_print(shell, "  iperf -c %s -i 1 -t 20 -u -b 5M -p %u", addr_str, port);
	}
}

/*
 * Download: the sample runs the zperf server and the iperf client on the test PC sends
 * to it. The results are printed when the client ends the session. The server keeps
 * running for further sessions until wifi_stop_tput is run.
 */
static int wifi_tput_download(const struct shell *shell, bool tcp, size_t argc, char **argv)
{
	struct zperf_download_params params = { 0 };
	struct sockaddr_in bind_addr = {
		.sin_family = AF_INET,
	};
	unsigned long port = WIFI_TPUT_DEFAULT_PORT;
	struct net_in_addr *addr;
	int ret;

	if (argc > 1) {
		shell_error(shell, "invalid # of args for download : %zu", argc);
		wifi_tput_usage(shell);
		return -ENOEXEC;
	}

	if (argc == 1) {
		ret = parse_ulong_range(shell, "port", argv[0], 1, UINT16_MAX, &port);
		if (ret) {
			return ret;
		}
	}

	ret = wifi_link_check(shell);
	if (ret) {
		return ret;
	}

	/*
	 * Bind to the address printed for the iperf client. Without one, zperf binds to
	 * CONFIG_NET_CONFIG_MY_IPV4_ADDR, which is not the address DHCP assigned.
	 */
	addr = net_if_ipv4_get_global_addr(net_if_get_first_wifi(), NET_ADDR_PREFERRED);
	if (!addr) {
		shell_error(shell, "Wi-Fi interface has no IPv4 address");
		return -EADDRNOTAVAIL;
	}

	bind_addr.sin_addr = *addr;
	memcpy(&params.addr, &bind_addr, sizeof(bind_addr));
	params.port = (uint16_t)port;

	ret = zperf_udp_download(&params, udp_download_results_cb, NULL);

	if (ret == -EALREADY) {
		shell_error(shell, "%s server is already running, stop it with: "
				"wifi_stop_tput %s", tcp ? "TCP" : "UDP", tcp ? "tcp" : "udp");
		return ret;
	}

	if (ret) {
		shell_error(shell, "Starting the %s server failed (err %d)", tcp ? "TCP" : "UDP",
				ret);
		return ret;
	}

	LOG_INF("Starting Wi-Fi benchmark: Zperf %s server on port %u", tcp ? "tcp" : "udp",
		params.port);
	wifi_tput_print_iperf_client(shell, tcp, &bind_addr.sin_addr, params.port);

	return 0;
}

static int wifi_stop_throughput(const struct shell *shell, size_t argc, char **argv)
{
	bool tcp;
	int ret;

	if (argc != 2) {
		shell_error(shell, "Usage: wifi_stop_tput <udp|tcp>");
		return -ENOEXEC;
	}

	ret = parse_protocol(shell, argv[1], &tcp);
	if (ret) {
		return ret;
	}

	ret = zperf_udp_download_stop();

	if (ret) {
		shell_error(shell, "%s server is not running", tcp ? "TCP" : "UDP");
		return ret;
	}

	shell_print(shell, "%s server stopped", tcp ? "TCP" : "UDP");

	return 0;
}
#endif /* CONFIG_NET_ZPERF_SERVER */

static int wifi_run_throughput(const struct shell *shell, size_t argc, char **argv)
{
	bool tcp;
	int ret;

	if (argc < 3) {
		shell_error(shell, "invalid # of args : %zu", argc);
		wifi_tput_usage(shell);
		return -ENOEXEC;
	}

	ret = parse_protocol(shell, argv[1], &tcp);
	if (ret) {
		return ret;
	}

	if (strcmp(argv[2], "upload") == 0) {
		ret = wifi_tput_upload(shell, tcp, argc - 3, &argv[3]);
	} else if (strcmp(argv[2], "download") == 0) {
#if defined(CONFIG_NET_ZPERF_SERVER)
		ret = wifi_tput_download(shell, tcp, argc - 3, &argv[3]);
#else
		shell_error(shell, "Download is not supported, enable CONFIG_NET_ZPERF_SERVER");
		ret = -ENOTSUP;
#endif
	} else {
		shell_error(shell, "direction: '%s' must be upload or download", argv[2]);
		ret = -EINVAL;
	}

	if (ret) {
		shell_error(shell, "Running Wi-Fi throughput - FAIL");
	}

	return ret;
}

/* ---- Coexistence enable/disable shell commands ---- */

#ifdef CONFIG_NRF71_SR_COEX_DRIVER

/*
 * Wi-Fi side: coex_cm_enable() sends CD2CM_ENABLE_COEXISTENCE to the Coexistence Manager
 * on the RPU and waits for the completion event. With false, Wi-Fi no longer posts
 * requests to COEXC. It fails with -ENETDOWN while the Wi-Fi transport is down (for
 * example after "net iface down"), because there is no CM to talk to.
 *
 * The driver configures the CM itself on every Wi-Fi interface up and ends with
 * coex_cm_enable(true), so a request sent while that runs would be overridden. The
 * state check below only narrows that window: the driver has no API to serialize an
 * application request with its own configuration.
 */
static int coex_wifi_set(const struct shell *shell, bool enable)
{
	const char *state = cd_wifi_state_name();
	int err;

	if (strcmp(state, "wifi down") == 0) {
		shell_error(shell, "Wi-Fi interface is not up, Wi-Fi coexistence not changed");
		return -ENETDOWN;
	}

	if (strcmp(state, "failed") == 0) {
		shell_error(shell, "Coexistence Manager configuration failed, bring the Wi-Fi "
				   "interface down and up to retry");
		return -EIO;
	}

	if (strcmp(state, "active") != 0) {
		shell_error(shell, "Coexistence Manager is not active (state: %s), try again later",
				state);
		return -EBUSY;
	}

	err = coex_cm_enable(enable);
	if (err) {
		shell_error(shell, "coex_cm_enable(%s) failed (err %d)%s",
				enable ? "true" : "false", err,
				(err == -ENETDOWN) ? ": Wi-Fi interface is not up" : "");
		return err;
	}

	shell_print(shell, "Wi-Fi coexistence %s (coex_cm_enable(%s))",
			enable ? "enabled" : "disabled", enable ? "true" : "false");

	if (!enable) {
		shell_warn(shell, "Wi-Fi coexistence is enabled again the next time the "
				  "Wi-Fi interface comes up");
	}

	return 0;
}

/*
 * SR side: cd_sr_enable() asks the SR driver to start (true) or stop (false) posting its
 * hardware client requests to COEXC. It returns 0 when the request was applied.
 */
static int coex_sr_set(const struct shell *shell, bool enable)
{
	int err = cd_sr_enable(enable);

	if (err) {
		shell_error(shell, "cd_sr_enable(%s) rejected by the SR driver (err %d)",
				enable ? "true" : "false", err);
		return -EIO;
	}

	coex_sr_user_disabled = !enable;

	shell_print(shell, "SR coexistence %s (cd_sr_enable(%s))",
			enable ? "enabled" : "disabled", enable ? "true" : "false");

	return 0;
}

/*
 * Run the SR step even when the Wi-Fi step fails, so SR coexistence can be changed
 * while the Wi-Fi interface is down, and report the first error. While the CM is
 * being configured, change neither side so that a retry applies both together.
 */
static int coex_wifi_sr_set(const struct shell *shell, bool enable)
{
	int err_wifi = coex_wifi_set(shell, enable);
	int err_sr;

	if (err_wifi == -EBUSY) {
		shell_error(shell, "SR coexistence not changed");
		return err_wifi;
	}

	err_sr = coex_sr_set(shell, enable);

	return err_wifi ? err_wifi : err_sr;
}

static int cmd_coex_enable_wifi_sr(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return coex_wifi_sr_set(shell, true);
}

static int cmd_coex_disable_wifi_sr(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return coex_wifi_sr_set(shell, false);
}

SHELL_CMD_REGISTER(coex_enable_wifi_sr, NULL,
			"Enable Wi-Fi and SR coexistence: coex_cm_enable(true) and cd_sr_enable(true)",
			cmd_coex_enable_wifi_sr);
SHELL_CMD_REGISTER(coex_disable_wifi_sr, NULL,
			"Disable Wi-Fi and SR coexistence: coex_cm_enable(false) and "
			"cd_sr_enable(false)",
			cmd_coex_disable_wifi_sr);

#endif /* CONFIG_NRF71_SR_COEX_DRIVER */

/* ---- Shell command registration ---- */

SHELL_CMD_REGISTER(bt_cfg_tput, NULL,
			"Connect and configure the Bluetooth LE throughput test\n"
			"Usage: bt_cfg_tput <bt_role: 1 central, 0 peripheral>",
			bt_configure_throughput);
SHELL_CMD_REGISTER(bt_run_tput, NULL,
			"Run the Bluetooth LE throughput test (central only)\n"
			"Usage: bt_run_tput <duration in seconds>",
			bt_run_throughput);
SHELL_CMD_REGISTER(bt_disconnect, NULL, "Disconnect the Bluetooth LE connection",
			bt_disconnection);
SHELL_CMD_REGISTER(bt_print_cfg, NULL, "Print the Bluetooth LE throughput configuration",
			print_bt_configuration);
#if defined(CONFIG_BT_FRAME_SPACE_UPDATE)
SHELL_CMD_REGISTER(bt_frame_space, NULL,
			"Set the minimum Bluetooth LE frame space\n"
			"Usage: bt_frame_space <us: 0 to 10000>",
			frame_space_cmd);
#endif
SHELL_CMD_REGISTER(wifi_run_tput, NULL,
			"Run the Wi-Fi UDP or TCP throughput test\n"
			"Usage: wifi_run_tput udp upload <peerIP> <destPort> <duration> "
			"<pktSize> <baudrate>\n"
			"       wifi_run_tput tcp upload <peerIP> <destPort> <duration> <pktSize>\n"
			"       wifi_run_tput <udp|tcp> download [<port>]",
			wifi_run_throughput);
#if defined(CONFIG_NET_ZPERF_SERVER)
SHELL_CMD_REGISTER(wifi_stop_tput, NULL,
			"Stop the Wi-Fi throughput server started for a download test\n"
			"Usage: wifi_stop_tput <udp|tcp>",
			wifi_stop_throughput);
#endif
