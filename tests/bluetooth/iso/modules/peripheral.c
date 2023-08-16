/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/byteorder.h>
#include <bluetooth/services/nus.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(peripheral, CONFIG_ACL_TEST_LOG_LEVEL);

#define LE_AUDIO_EXTENDED_ADV_CONN_NAME                                                            \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONNECTABLE |                        \
				BT_LE_ADV_OPT_USE_NAME,                                            \
			0x100, 0x200, NULL)

static struct k_work adv_work;
static struct bt_conn *default_conn;
static struct bt_le_ext_adv *adv_ext;

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

static void advertising_process(struct k_work *work)
{
	int ret;

	ret = bt_le_ext_adv_start(adv_ext, BT_LE_EXT_ADV_START_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to start advertising set. Err: %d", ret);
		return;
	}

	LOG_INF("Advertising successfully started");
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		LOG_ERR("Failed to connect, err: %d", err);
		default_conn = NULL;
		advertising_start();
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected: %s", addr);

	default_conn = bt_conn_ref(conn);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		LOG_WRN("Disconnected on wrong conn");
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	advertising_start();
}

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	uint32_t count = 0;
	static uint32_t last_count;
	static uint32_t counts_fail;
	static uint32_t counts_success;

	count = sys_get_le32(data);

	if (last_count) {
		if (count != (last_count + 1)) {
			counts_fail++;
		} else {
			counts_success++;
		}
	}

	last_count = count;

	if ((count % CONFIG_PRINT_CONN_INTERVAL) == 0) {
		LOG_INF("ACL RX. Count: %d, Failed: %d, Success: %d", count, counts_fail,
			counts_success);
	}
}

static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
};

static struct bt_conn_cb conn_callbacks = {.connected = connected_cb,
					   .disconnected = disconnected_cb};

static int peripheral_start(void)
{
	advertising_start();

	return 0;
}

static int peripheral_init(void)
{
	int ret;

	bt_nus_init(&nus_cb);
	bt_conn_cb_register(&conn_callbacks);
	k_work_init(&adv_work, advertising_process);

	ret = bt_le_ext_adv_create(LE_AUDIO_EXTENDED_ADV_CONN_NAME, NULL, &adv_ext);
	if (ret) {
		LOG_ERR("Failed to create advertising set, %d", ret);
		return ret;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(peripheral_cmd,
			       SHELL_CMD(init, NULL, "Init peripheral NUS.", peripheral_init),
			       SHELL_CMD(start, NULL, "Start peripheral NUS.", peripheral_start),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(peripheral, &peripheral_cmd, "Peripheral NUS commands", NULL);
