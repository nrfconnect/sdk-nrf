/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>
#include <unistd.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/gatt_dm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(central, CONFIG_ACL_TEST_LOG_LEVEL);

#define REMOTE_DEVICE_NAME_PEER	    CONFIG_BT_DEVICE_NAME
#define REMOTE_DEVICE_NAME_PEER_LEN (sizeof(REMOTE_DEVICE_NAME_PEER) - 1)

static bool running;
static uint32_t acl_dummy_data_send_interval_ms = 100;
static uint32_t acl_dummy_data_size = 20;
struct peer_device {
	struct bt_conn *conn;
	struct k_work_delayable dummy_data_send_work;
};
static struct peer_device remote_peer[CONFIG_BT_MAX_CONN];
static struct bt_nus_client *nus[CONFIG_BT_MAX_CONN];
static struct bt_nus_client nus_client[CONFIG_BT_MAX_CONN];
/* Default ACL connection parameter configuration */
static struct bt_le_conn_param conn_param = {
	.interval_min = 32, /* ACL interval min = 40 ms (x 1.25 ms) */
	.interval_max = 32, /* ACL interval max = 40 ms (x 1.25 ms) */
	.latency = 0,	    /* ACL slave latency = 0 */
	.timeout = 100	    /* ACL supervision timeout = 1000 ms (x 10 ms)*/
};

static int scan_start(void);

static bool all_peers_connected(void)
{
	for (int i = 0; i < ARRAY_SIZE(remote_peer); i++) {
		if (remote_peer[i].conn == NULL) {
			return false;
		}
	}

	return true;
}

static int channel_index_get(const struct bt_conn *conn, uint8_t *index)
{
	if (conn == NULL) {
		LOG_ERR("No connection provided");
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(remote_peer); i++) {
		if (remote_peer[i].conn == conn) {
			*index = i;
			return 0;
		}
	}

	LOG_WRN("Connection not found");

	return -EINVAL;
}

static void work_dummy_data_send(struct k_work *work)
{
	int ret;
	char dummy_string[CONFIG_BT_MAX_CONN][CONFIG_BT_L2CAP_TX_MTU] = {0};
	static uint8_t channel_index;
	static uint32_t acl_send_count[CONFIG_BT_MAX_CONN];

	struct peer_device *peer = CONTAINER_OF(work, struct peer_device,
						dummy_data_send_work.work);

	ret = channel_index_get(peer->conn, &channel_index);
	if (ret) {
		LOG_ERR("Channel index not found");
		return;
	}
	acl_send_count[channel_index]++;
	sys_put_le32(acl_send_count[channel_index], dummy_string[channel_index]);

	ret = bt_nus_client_send(&nus_client[channel_index], dummy_string[channel_index],
				 acl_dummy_data_size);
	if (ret) {
		LOG_WRN("Failed to send, ret = %d", ret);
	}

	k_work_reschedule(&remote_peer[channel_index].dummy_data_send_work,
			  K_MSEC(acl_dummy_data_send_interval_ms));
}

static void discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	int ret;
	uint8_t channel_index;

	ret = channel_index_get(bt_gatt_dm_conn_get(dm), &channel_index);
	if (ret) {
		LOG_ERR("Channel index not found");
	}

	nus[channel_index] = context;

	ret = bt_nus_handles_assign(dm, nus[channel_index]);
	if (ret) {
		LOG_ERR("NUS handle assigne failed, ret = %d", ret);
	}
	bt_nus_subscribe_receive(nus[channel_index]);

	ret = bt_gatt_dm_data_release(dm);
	if (ret) {
		LOG_ERR("Release service discovery data failed, ret = %d", ret);
	}

	k_work_schedule(&remote_peer[channel_index].dummy_data_send_work, K_MSEC(100));

	if (all_peers_connected()) {
		LOG_INF("All devices connected");
	} else {
		LOG_INF("Not all devices connected");
		scan_start();
	}
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	LOG_ERR("NUS service not found");
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
	LOG_ERR("Error while discovering GATT database:, err = %d", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error,
};

static int gatt_discover(struct bt_conn *conn)
{
	int ret;
	uint8_t channel_index;

	ret = channel_index_get(conn, &channel_index);
	if (ret) {
		LOG_ERR("Channel index not found");
	}

	ret = bt_gatt_dm_start(conn, BT_UUID_NUS_SERVICE, &discovery_cb,
			       &nus_client[channel_index]);
	return ret;
}

static uint8_t ble_data_received(struct bt_nus_client *nus, const uint8_t *data, uint16_t len)
{
	LOG_HEXDUMP_INF(data, len, "NUS received:");
	return BT_GATT_ITER_CONTINUE;
}

static int nus_client_init(void)
{
	int ret;
	struct bt_nus_client_init_param init = {.cb = {
							.received = ble_data_received,
						}};

	for (int i = 0; i < ARRAY_SIZE(nus_client); i++) {
		ret = bt_nus_client_init(&nus_client[i], &init);
		if (ret) {
			LOG_ERR("NUS Client [%d] initialization failed, ret = %d", i, ret);
			return ret;
		}
	}

	return 0;
}

static int device_found(uint8_t type, const uint8_t *data, uint8_t data_len,
			const bt_addr_le_t *addr)
{
	int ret;
	struct bt_conn *conn;

	if (all_peers_connected()) {
		LOG_DBG("All peripherals connected");
		return 0;
	}

	if ((data_len == REMOTE_DEVICE_NAME_PEER_LEN) &&
	    (strncmp(REMOTE_DEVICE_NAME_PEER, data, REMOTE_DEVICE_NAME_PEER_LEN) == 0)) {
		LOG_DBG("Device found");

		ret = bt_le_scan_stop();
		if (ret && ret != -EALREADY) {
			LOG_WRN("Stop scan failed: %d", ret);
		}

		ret = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, &conn_param, &conn);
		if (ret) {
			LOG_ERR("Could not init connection");
			scan_start();
			return ret;
		}

		for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
			if (remote_peer[i].conn == NULL) {
				remote_peer[i].conn = conn;
				break;
			}
		}

		return 0;
	}

	return -ENOENT;
}

/** @brief  Parse BLE advertisement package.
 */
static void ad_parse(struct net_buf_simple *p_ad, const bt_addr_le_t *addr)
{
	while (p_ad->len > 1) {
		uint8_t len = net_buf_simple_pull_u8(p_ad);
		uint8_t type;

		/* Check for early termination */
		if (len == 0) {
			return;
		}

		if (len > p_ad->len) {
			LOG_ERR("AD malformed");
			return;
		}

		type = net_buf_simple_pull_u8(p_ad);

		if (device_found(type, p_ad->data, len - 1, addr) == 0) {
			return;
		}

		(void)net_buf_simple_pull(p_ad, len - 1);
	}
}

static void on_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			    struct net_buf_simple *p_ad)
{
	if ((type == BT_GAP_ADV_TYPE_ADV_IND || type == BT_GAP_ADV_TYPE_EXT_ADV)) {
		/* Note: May lead to connection creation */
		ad_parse(p_ad, addr);
	}
}

static int scan_start(void)
{
	int ret;

	ret = bt_le_scan_start(BT_LE_SCAN_PASSIVE, on_device_found);
	if (ret && ret != -EALREADY) {
		LOG_WRN("Scanning failed to start: %d", ret);
		return ret;
	}

	LOG_INF("Scanning successfully started");
	return 0;
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	int ret;
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("ACL connection to %s failed, error %d", addr, err);
		bt_conn_unref(conn);
		scan_start();

		return;
	}

	ret = gatt_discover(conn);
	if (ret) {
		LOG_ERR("Failed to start the discovery procedure, ret = %d", ret);
	}

	/* ACL connection established */
	LOG_INF("Connected: %s", addr);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	int ret;
	uint8_t channel_index;
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);
	bt_conn_unref(conn);

	ret = channel_index_get(conn, &channel_index);
	if (ret) {
		LOG_WRN("Unknown connection");
	} else {
		remote_peer[channel_index].conn = NULL;
	}

	k_work_cancel_delayable(&remote_peer[channel_index].dummy_data_send_work);
	scan_start();
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static int central_start(void)
{
	int ret;

	ret = scan_start();
	if (ret) {
		return ret;
	}
	running = true;

	return 0;
}

static int central_stop(void)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(remote_peer); i++) {
		if (remote_peer[i].conn != NULL) {
			ret = bt_conn_disconnect(remote_peer[i].conn,
						 BT_HCI_ERR_LOCALHOST_TERM_CONN);
			if (ret) {
				LOG_ERR("Disconnected with remote peer failed %d", ret);
			}
		}
	}

	ret = bt_le_scan_stop();
	if (ret && ret != -EALREADY) {
		LOG_WRN("Stop scan failed: %d", ret);
	}

	running = false;

	return 0;
}

static int central_print_cfg(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell,
		    "acl_int_min(x0.625ms) %d\nacl_int_max(x0.625ms) %d\nacl_latency "
		    "%d\nacl_timeout(x100ms) %d\nacl_payload_size %d\nacl_send_int(ms) %d\n",
		    conn_param.interval_min, conn_param.interval_max, conn_param.latency,
		    conn_param.timeout, acl_dummy_data_size, acl_dummy_data_send_interval_ms);
	return 0;
}

static int central_init(void)
{
	int ret;

	ret = nus_client_init();
	bt_conn_cb_register(&conn_callbacks);
	for (int i = 0; i < ARRAY_SIZE(remote_peer); i++) {
		k_work_init_delayable(&remote_peer[i].dummy_data_send_work, work_dummy_data_send);
	}

	return ret;
}

static struct option long_options[] = {{"acl_int_min", required_argument, NULL, 'n'},
				       {"acl_int_max", required_argument, NULL, 'x'},
				       {"acl_latency", required_argument, NULL, 's'},
				       {"acl_timeout", required_argument, NULL, 't'},
				       {"acl_send_int", required_argument, NULL, 'i'},
				       {"acl_paylod_size", required_argument, NULL, 'p'},
				       {0, 0, 0, 0}};

static const char short_options[] = "n:x:s:t:i:p";

static int argument_check(const struct shell *shell, uint8_t const *const input)
{
	char *end;
	int arg_val = strtol(input, &end, 10);

	if (*end != '\0' || (uint8_t *)end == input || (arg_val == 0 && !isdigit(input[0])) ||
	    arg_val < 0) {
		shell_error(shell, "Argument must be a positive integer %s", input);
		return -EINVAL;
	}

	if (running) {
		shell_error(shell, "Arguments can not be changed while running");
		return -EACCES;
	}

	return arg_val;
}

static int param_set(const struct shell *shell, size_t argc, char **argv)
{
	int result = argument_check(shell, argv[2]);
	int long_index = 0;
	int opt;

	if (result < 0) {
		return result;
	}

	if (running) {
		shell_error(shell, "Arguments can not be changed while running");
		return -EPERM;
	}

	optreset = 1;
	optind = 1;

	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		switch (opt) {
		case 'n':
			conn_param.interval_min = result;
			central_print_cfg(shell, 0, NULL);
			break;
		case 'x':
			conn_param.interval_max = result;
			central_print_cfg(shell, 0, NULL);
			break;
		case 's':
			conn_param.latency = result;
			central_print_cfg(shell, 0, NULL);
			break;
		case 't':
			conn_param.timeout = result;
			central_print_cfg(shell, 0, NULL);
			break;
		case 'i':
			acl_dummy_data_send_interval_ms = result;
			central_print_cfg(shell, 0, NULL);
			break;
		case 'p':
			acl_dummy_data_size = result;
			central_print_cfg(shell, 0, NULL);
			break;
		case ':':
			shell_error(shell, "Missing option parameter");
			break;
		case '?':
			shell_error(shell, "Unknown option: %c", opt);
			break;
		default:
			shell_error(shell, "Invalid option: %c", opt);
			break;
		}
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(central_cmd,
			       SHELL_CMD(init, NULL, "Init central NUS.", central_init),
			       SHELL_CMD(start, NULL, "Start central NUS.", central_start),
			       SHELL_CMD(stop, NULL, "Stop central NUS.", central_stop),
			       SHELL_CMD(cfg, NULL, "Print config.", central_print_cfg),
			       SHELL_CMD_ARG(set, NULL, "set", param_set, 3, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(central, &central_cmd, "Central NUS commands", NULL);
