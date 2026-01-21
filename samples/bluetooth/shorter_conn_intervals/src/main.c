/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/console/console.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/latency.h>
#include <bluetooth/services/latency_client.h>
#include <bluetooth/scan.h>
#include <bluetooth/gatt_dm.h>

LOG_MODULE_REGISTER(app_main, LOG_LEVEL_INF);

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define INTERVAL_INITIAL	  0x8	/* 8 units, 10 ms */
#define INTERVAL_INITIAL_US	  10000 /* 10 ms */
#define INTERVAL_UPDATE_PERIOD_MS 3000	/* Update interval every 3 seconds */

static uint32_t test_intervals[] = {
	0,    /* Will be replaced with minimum supported interval */
	1000, /* 1 ms */
	1250, /* 1.25 ms */
	2000, /* 2 ms */
	4000  /* 4 ms */
};

/** @brief UUID of the SCI Min Interval Service. **/
#define BT_UUID_VS_SCI_MIN_INTERVAL_SERVICE_VAL                                                    \
	BT_UUID_128_ENCODE(0x1c840001, 0x49ac, 0x4905, 0x9702, 0x6e836da4cadd)

#define BT_UUID_VS_SCI_MIN_INTERVAL_SERVICE                                                        \
	BT_UUID_DECLARE_128(BT_UUID_VS_SCI_MIN_INTERVAL_SERVICE_VAL)

/** @brief UUID of the SCI Min Interval Characteristic. **/
#define BT_UUID_VS_SCI_MIN_INTERVAL_CHAR_VAL                                                       \
	BT_UUID_128_ENCODE(0x1c840002, 0x49ac, 0x4905, 0x9702, 0x6e836da4cadd)

#define BT_UUID_VS_SCI_MIN_INTERVAL_CHAR BT_UUID_DECLARE_128(BT_UUID_VS_SCI_MIN_INTERVAL_CHAR_VAL)

static K_SEM_DEFINE(phy_updated, 0, 1);
static K_SEM_DEFINE(min_interval_read_sem, 0, 1);
static K_SEM_DEFINE(frame_space_updated_sem, 0, 1);
static K_SEM_DEFINE(discovery_complete_sem, 0, 1);

static bool test_ready;
static bool initiate_conn_rate_update;
static bool is_central;

static uint32_t latency_response;

static uint16_t local_min_interval_us;
static uint16_t remote_min_interval_us;
static uint16_t common_min_interval_us;
static uint16_t remote_min_interval_handle;

static struct bt_conn *default_conn;
static struct bt_latency latency;
static struct bt_latency_client latency_client;
static struct bt_le_conn_param *conn_param =
	BT_LE_CONN_PARAM(INTERVAL_INITIAL, INTERVAL_INITIAL, 0, 400);
static struct bt_conn_info conn_info = {0};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_VS_SCI_MIN_INTERVAL_SERVICE_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* Example GATT service for exchanging minimum supported connection interval. */
static ssize_t read_min_interval(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	uint16_t value = sys_cpu_to_le16(local_min_interval_us);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value, sizeof(value));
}

BT_GATT_SERVICE_DEFINE(sci_min_interval_svc,
		       BT_GATT_PRIMARY_SERVICE(BT_UUID_VS_SCI_MIN_INTERVAL_SERVICE),
		       BT_GATT_CHARACTERISTIC(BT_UUID_VS_SCI_MIN_INTERVAL_CHAR, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_min_interval, NULL, NULL));

static const char *phy_to_str(uint8_t phy)
{
	switch (phy) {
	case 0:
		return "Unknown";
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

static const char *fsu_initiator_to_str(enum bt_conn_le_frame_space_update_initiator initiator)
{
	switch (initiator) {
	case BT_CONN_LE_FRAME_SPACE_UPDATE_INITIATOR_LOCAL_HOST:
		return "Local Host";
	case BT_CONN_LE_FRAME_SPACE_UPDATE_INITIATOR_LOCAL_CONTROLLER:
		return "Local Controller";
	case BT_CONN_LE_FRAME_SPACE_UPDATE_INITIATOR_PEER:
		return "Peer";
	default:
		return "Unknown";
	}
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d", addr, connectable);
}

static void scan_filter_no_match(struct bt_scan_device_info *device_info, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filter does not match. Address: %s connectable: %d", addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match, scan_connecting_error, NULL);

static void scan_init(void)
{
	int err;
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = 0x0010,
		.window = 0x0010,
	};

	struct bt_scan_init_param scan_init = {
		.connect_if_match = true, .scan_param = &scan_param, .conn_param = conn_param};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_VS_SCI_MIN_INTERVAL_SERVICE);
	if (err) {
		LOG_WRN("Scanning filters cannot be set (err %d)", err);
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		LOG_WRN("Filters cannot be turned on (err %d)", err);
	}
}

static uint8_t read_min_interval_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params, const void *data,
				    uint16_t length)
{
	if (err) {
		LOG_WRN("Failed to read remote min interval (err %u)", err);
		k_sem_give(&min_interval_read_sem);
		return BT_GATT_ITER_STOP;
	}

	if (data && length == sizeof(uint16_t)) {
		remote_min_interval_us = sys_get_le16(data);
	}

	k_sem_give(&min_interval_read_sem);

	return BT_GATT_ITER_STOP;
}

static int read_remote_min_interval(void)
{
	static struct bt_gatt_read_params read_params;
	int err;

	read_params.func = read_min_interval_cb;
	read_params.handle_count = 1;
	read_params.single.handle = remote_min_interval_handle;
	read_params.single.offset = 0;

	err = bt_gatt_read(default_conn, &read_params);
	if (err) {
		LOG_WRN("Failed to initiate read (err %d)", err);
		return err;
	}

	return 0;
}

static void sci_discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_value;

	LOG_INF("SCI service discovery completed");

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_VS_SCI_MIN_INTERVAL_CHAR);
	if (gatt_chrc) {
		gatt_value = bt_gatt_dm_attr_next(dm, gatt_chrc);
		if (gatt_value) {
			remote_min_interval_handle = gatt_value->handle;
			LOG_INF("Found SCI min interval characteristic, handle: 0x%04x",
				remote_min_interval_handle);
		} else {
			LOG_WRN("Failed to get characteristic value attribute");
			return;
		}
	} else {
		LOG_WRN("SCI min interval characteristic not found");
		return;
	}

	bt_gatt_dm_data_release(dm);

	test_ready = true;
	k_sem_give(&discovery_complete_sem);
}

static void sci_discovery_service_not_found(struct bt_conn *conn, void *context)
{
	LOG_INF("SCI service not found");
}

static void sci_discovery_error(struct bt_conn *conn, int err, void *context)
{
	LOG_INF("Error while discovering SCI service: (err %d)", err);
}

static struct bt_gatt_dm_cb sci_discovery_cb = {
	.completed = &sci_discovery_complete,
	.service_not_found = &sci_discovery_service_not_found,
	.error_found = &sci_discovery_error,
};

static void latency_discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	struct bt_latency_client *latency_client_ctx = context;
	int err;

	LOG_INF("Latency service discovery completed");

	bt_gatt_dm_data_print(dm);
	bt_latency_handles_assign(dm, latency_client_ctx);

	bt_gatt_dm_data_release(dm);

	/* Now discover the SCI min interval service */
	err = bt_gatt_dm_start(default_conn, BT_UUID_VS_SCI_MIN_INTERVAL_SERVICE, &sci_discovery_cb,
			       NULL);
	if (err) {
		LOG_WRN("SCI service discovery failed (err %d)", err);
	}
}

static void latency_discovery_service_not_found(struct bt_conn *conn, void *context)
{
	LOG_WRN("Latency service not found");
}

static void latency_discovery_error(struct bt_conn *conn, int err, void *context)
{
	LOG_WRN("Error while discovering GATT database: (err %d)", err);
}

static struct bt_gatt_dm_cb latency_discovery_cb = {
	.completed = &latency_discovery_complete,
	.service_not_found = &latency_discovery_service_not_found,
	.error_found = &latency_discovery_error,
};

static void adv_start(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_INF("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Advertising successfully started");
}

static void scan_start(void)
{
	int err;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		LOG_WRN("Starting scanning failed (err %d)", err);
		return;
	}

	LOG_INF("Scanning successfully started");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_WRN("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
		return;
	}

	default_conn = bt_conn_ref(conn);
	err = bt_conn_get_info(default_conn, &conn_info);
	if (err) {
		LOG_WRN("Getting conn info failed (err %d)", err);
		return;
	}

	/* make sure we're not scanning or advertising */
	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		bt_scan_stop();
	} else {
		bt_le_adv_stop();
	}

	LOG_INF("Connected as %s",
		conn_info.role == BT_CONN_ROLE_CENTRAL ? "central" : "peripheral");
	LOG_INF("Conn. interval is %u us", conn_info.le.interval_us);

#if defined(CONFIG_BT_SMP)
	if (conn_info.role == BT_CONN_ROLE_PERIPHERAL) {
		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (err) {
			LOG_WRN("Failed to set security: %d", err);
		}
	}
#else
	/* Start discovery immediately if encryption is not enabled */
	err = bt_gatt_dm_start(default_conn, BT_UUID_LATENCY, &latency_discovery_cb,
			       &latency_client);
	if (err) {
		LOG_WRN("Discover failed (err %d)", err);
	}
#endif /* CONFIG_BT_SMP */
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));

	test_ready = false;

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	/* Restart scanning or advertising based on configured role */
	if (is_central) {
		LOG_INF("Reconnecting as central");
		scan_start();
	} else {
		LOG_INF("Reconnecting as peripheral");
		adv_start();
	}
}

#if defined(CONFIG_BT_SMP)
void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	LOG_INF("Security changed: level %i, err: %i %s", level, err, bt_security_err_to_str(err));

	if (err != 0) {
		LOG_WRN("Failed to encrypt link");
		bt_conn_disconnect(conn, BT_HCI_ERR_PAIRING_NOT_SUPPORTED);
		return;
	}

	/* Start service discovery when link is encrypted */
	int gatt_err = bt_gatt_dm_start(default_conn, BT_UUID_LATENCY, &latency_discovery_cb,
					&latency_client);
	if (gatt_err) {
		LOG_WRN("Discover failed (err %d)", gatt_err);
	}
}
#endif /* CONFIG_BT_SMP */

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	/* Ignore peer parameter preferences. */
	return false;
}

static int set_conn_rate_defaults(uint32_t interval_min_us, uint32_t interval_max_us)
{
	const struct bt_conn_le_conn_rate_param params = {
		.interval_min_125us = interval_min_us / 125,
		.interval_max_125us = interval_max_us / 125,
		.subrate_min = 1,
		.subrate_max = 1,
		.max_latency = 5,
		.continuation_number = 0,
		.supervision_timeout_10ms = 400,
		.min_ce_len_125us = BT_HCI_LE_SCI_CE_LEN_MIN_125US,
		.max_ce_len_125us = BT_HCI_LE_SCI_CE_LEN_MAX_125US,
	};

	int err = bt_conn_le_conn_rate_set_defaults(&params);

	if (err) {
		LOG_WRN("Set default rate parameters failed (err %d)", err);
		return err;
	}

	LOG_INF("SCI default connection rate parameters set (min=%u us, max=%u us)",
		interval_min_us, interval_max_us);
	return 0;
}

static int select_lowest_frame_space(void)
{
	const struct bt_conn_le_frame_space_update_param params = {
		.phys = BT_HCI_LE_FRAME_SPACE_UPDATE_PHY_2M_MASK,
		.spacing_types = BT_CONN_LE_FRAME_SPACE_TYPES_MASK_ACL_IFS,
		.frame_space_min = 0,
		.frame_space_max = 150,
	};

	int err = bt_conn_le_frame_space_update(default_conn, &params);

	if (err) {
		LOG_WRN("Frame space update request failed (err %d)", err);
		return err;
	}

	/* Wait for frame space update to complete */
	k_sem_take(&frame_space_updated_sem, K_FOREVER);
	return 0;
}

static int conn_rate_request(uint32_t interval_min_us, uint32_t interval_max_us)
{
	const struct bt_conn_le_conn_rate_param params = {
		.interval_min_125us = interval_min_us / 125,
		.interval_max_125us = interval_max_us / 125,
		.subrate_min = 1,
		.subrate_max = 1,
		.max_latency = 0,
		.continuation_number = 0,
		.supervision_timeout_10ms = 400,
		.min_ce_len_125us = BT_HCI_LE_SCI_CE_LEN_MIN_125US,
		.max_ce_len_125us = BT_HCI_LE_SCI_CE_LEN_MAX_125US,
	};

	int err = bt_conn_le_conn_rate_request(default_conn, &params);

	if (err) {
		LOG_WRN("Connection rate request failed (err %d)", err);
		return err;
	}

	return 0;
}

static void conn_rate_changed(struct bt_conn *conn, uint8_t status,
			      const struct bt_conn_le_conn_rate_changed *params)
{
	if (status == BT_HCI_ERR_SUCCESS) {
		LOG_INF("Connection rate changed: "
			"interval %u us, "
			"subrate factor %d, "
			"peripheral latency %d, "
			"continuation number %d, "
			"supervision timeout %d ms",
			params->interval_us, params->subrate_factor, params->peripheral_latency,
			params->continuation_number, params->supervision_timeout_10ms * 10);
	} else {
		LOG_WRN("Connection rate change failed (HCI status 0x%02x %s)", status,
			bt_hci_err_to_str(status));
	}
}

static int update_to_2m_phy(void)
{
	struct bt_conn_le_phy_param phy;

	phy.options = BT_CONN_LE_PHY_OPT_NONE;
	phy.pref_rx_phy = BT_GAP_LE_PHY_2M;
	phy.pref_tx_phy = BT_GAP_LE_PHY_2M;

	int err = bt_conn_le_phy_update(default_conn, &phy);

	if (err) {
		LOG_WRN("PHY update failed: %d", err);
		return err;
	}

	k_sem_take(&phy_updated, K_FOREVER);
	return 0;
}

static void le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
	LOG_INF("LE PHY updated: TX PHY %s, RX PHY %s", phy_to_str(param->tx_phy),
		phy_to_str(param->rx_phy));
	k_sem_give(&phy_updated);
}

static void frame_space_updated(struct bt_conn *conn,
				const struct bt_conn_le_frame_space_updated *params)
{
	if (params->status == BT_HCI_ERR_SUCCESS) {
		LOG_INF("Frame space updated: %u us, PHYs: 0x%02x, spacing types: 0x%04x, "
			"initiator: %s",
			params->frame_space, params->phys, params->spacing_types,
			fsu_initiator_to_str(params->initiator));
	} else {
		LOG_WRN("Frame space update failed (HCI status 0x%02x %s)", params->status,
			bt_hci_err_to_str(params->status));
	}

	k_sem_give(&frame_space_updated_sem);
}

static void latency_response_handler(const void *buf, uint16_t len)
{
	uint32_t latency_time;

	if (len == sizeof(latency_time)) {
		/* compute how much time was spent */
		latency_time = *((uint32_t *)buf);
		uint32_t cycles_spent = k_cycle_get_32() - latency_time;

		latency_response = (uint32_t)k_cyc_to_ns_floor64(cycles_spent) / 2000;
	}
}

static const struct bt_latency_client_cb latency_client_cb = {.latency_response =
								      latency_response_handler};

static void test_run(void)
{
	int err;

	if (!test_ready) {
		/* disconnected while blocking inside _getchar() */
		return;
	}

	test_ready = false;

	/* Update link parameters to satisfy minimum supported connection interval requirements. */
	if (initiate_conn_rate_update) {
		/* The lowest connection intervals can only be achieved on the 2M PHY. */
		err = update_to_2m_phy();
		if (err) {
			return;
		}

		/* A smaller frame space is required. */
		err = select_lowest_frame_space();
		if (err) {
			LOG_WRN("Frame space update failed (err %d)", err);
			return;
		}
	}

	/* Read remote min interval if characteristic was found */
	if (remote_min_interval_handle != 0) {
		err = read_remote_min_interval();
		if (err == 0) {
			k_sem_take(&min_interval_read_sem, K_FOREVER);

			/* Select the shortest interval supported by both devices. */
			common_min_interval_us = MAX(local_min_interval_us, remote_min_interval_us);
			test_intervals[0] = common_min_interval_us;

			LOG_INF("Minimum connection intervals: Local: %u us, Peer: %u us, "
				"Common: %u us",
				local_min_interval_us, remote_min_interval_us,
				common_min_interval_us);
		}
	}

	uint8_t interval_index = 0;
	int64_t next_update_time = 0;

	/* Start sending timestamps to the peer device */
	while (default_conn) {
		uint32_t time = k_cycle_get_32();

		err = bt_latency_request(&latency_client, &time, sizeof(time));
		if (err && err != -EALREADY) {
			LOG_WRN("Latency failed (err %d)", err);
		}

		k_sleep(K_MSEC(200)); /* wait for latency response */

		if (latency_response) {
			LOG_INF("Transmission Latency: %u us", latency_response);
		} else {
			LOG_WRN("Did not receive a latency response");
		}

		latency_response = 0;

		if (initiate_conn_rate_update) {
			int64_t current_time = k_uptime_get();

			if (current_time >= next_update_time) {
				next_update_time = current_time + INTERVAL_UPDATE_PERIOD_MS;

				uint32_t new_interval_us = test_intervals[interval_index];

				LOG_INF("Requesting new connection interval: %u us",
					new_interval_us);

				/* As a central, the lowest supported interval of the peripheral
				 * needs to be obtained before initiating a connection rate request,
				 * as interval_min cannot be lower than the peripheral's minimum
				 * supported interval. As a peripheral, calculating the common
				 * minimum interval is optional, as requesting a range will allow
				 * the central to select the lowest interval it can support.
				 */
				err = conn_rate_request(new_interval_us, INTERVAL_INITIAL_US);

				if (err) {
					LOG_WRN("Connection rate update failed (err %d)", err);
					return;
				}

				/* Move to next interval in the sequence */
				interval_index = (interval_index + 1) % ARRAY_SIZE(test_intervals);
			}
		}
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_phy_updated = le_phy_updated,
	.le_param_req = le_param_req,
	.conn_rate_changed = conn_rate_changed,
	.frame_space_updated = frame_space_updated,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif /* CONFIG_BT_SMP */
};

int main(void)
{
	int err;

	console_init();

	LOG_INF("Starting Bluetooth Shorter Connection Intervals sample");

	err = bt_enable(NULL);
	if (err) {
		LOG_WRN("Bluetooth init failed (err %d)", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized");

	/* Read local minimum connection interval */
	err = bt_conn_le_read_min_conn_interval(&local_min_interval_us);
	if (err) {
		LOG_WRN("Failed to read min conn interval (err %d)", err);
		return 0;
	}
	LOG_INF("Local minimum connection interval: %u us", local_min_interval_us);

	/* Set the initial allowed range of parameters that can be requested by the peripheral.
	 * They will be overridden by any calls to bt_conn_le_conn_rate_request().
	 */
	err = set_conn_rate_defaults(local_min_interval_us, INTERVAL_INITIAL_US);
	if (err) {
		LOG_WRN("Failed to set conn rate defaults");
		return 0;
	}

	err = bt_latency_init(&latency, NULL);
	if (err) {
		LOG_WRN("Latency service initialization failed (err %d)", err);
		return 0;
	}

	err = bt_latency_client_init(&latency_client, &latency_client_cb);
	if (err) {
		LOG_WRN("Latency client initialization failed (err %d)", err);
		return 0;
	}

	while (true) {
		LOG_INF("Should this device initiate connection interval updates?\n"
			"Type y (yes, this device will initiate) or n (no, peer will initiate): ");

		char input_char = console_getchar();

		LOG_INF("");

		if (input_char == 'y') {
			initiate_conn_rate_update = true;
			LOG_INF("This device will initiate connection interval updates");
			break;
		} else if (input_char == 'n') {
			initiate_conn_rate_update = false;
			LOG_INF("The peer device will initiate connection interval updates");
			break;
		}

		LOG_INF("Invalid input");
	}

	while (true) {
		LOG_INF("Choose device role - type c (central) or p (peripheral): ");

		char input_char = console_getchar();

		LOG_INF("");

		if (input_char == 'c') {
			is_central = true;
			LOG_INF("Central. Starting scanning");
			scan_init();
			scan_start();
			break;
		} else if (input_char == 'p') {
			is_central = false;
			LOG_INF("Peripheral. Starting advertising");
			adv_start();
			break;
		}

		LOG_INF("Invalid role");
	}

	for (;;) {
		/* Wait for service discovery to complete */
		k_sem_take(&discovery_complete_sem, K_FOREVER);

		test_run();
	}
}
