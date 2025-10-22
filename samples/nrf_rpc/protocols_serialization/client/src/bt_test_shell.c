/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stddef.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_test, CONFIG_NRF_PS_CLIENT_LOG_LEVEL);

/* UUID: 12345678-1234-5678-1234-56789abcdef0 */
#define BT_UUID_CUSTOM_SERVICE_VAL                                                                 \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BT_UUID_CUSTOM_CHAR_VAL                                                                    \
	BT_UUID_128_ENCODE(0xabcdef01, 0x2345, 0x6789, 0xabcd, 0xef0123456789)

#ifdef CONFIG_NRF_PS_CLIENT_BT_TEST_DATA_SIZE
#define TEST_DATA_SIZE (CONFIG_NRF_PS_CLIENT_BT_TEST_DATA_SIZE)
#else
#define TEST_DATA_SIZE (10)
#endif

static void connected(struct bt_conn *conn, uint8_t hci_err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void update_subscribed(void);
static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
static ssize_t read_custom_char(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset);
static ssize_t write_custom_char(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);
static struct bt_uuid_128 custom_char_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_CHAR_VAL);

static struct bt_gatt_indicate_params ind_params;

static struct bt_conn_cb conn_cb = {
	.connected = connected,
	.disconnected = disconnected,
};

typedef struct {
	uint8_t data[TEST_DATA_SIZE];
	bool notify_enabled;
	bool indicate_enabled;
	struct bt_conn *conn;
} test_svc_ctx_t;

static test_svc_ctx_t test_ctx = {
	.data = {},
	.notify_enabled = false,
	.indicate_enabled = false,
	.conn = NULL,
};

BT_GATT_SERVICE_DEFINE(custom_svc, BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),
		       BT_GATT_CHARACTERISTIC(&custom_char_uuid.uuid,
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
						      BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE,
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					      read_custom_char, write_custom_char, &test_ctx),
		       BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static ssize_t read_custom_char(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	test_svc_ctx_t *ctx = attr->user_data;
	uint16_t actual_len = MIN(len, TEST_DATA_SIZE - offset);

	LOG_INF("BT Test Read. off %d len %d", offset, len);

	if (offset >= TEST_DATA_SIZE) {
		LOG_ERR("BT Test Invalid offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	LOG_HEXDUMP_INF(ctx->data, TEST_DATA_SIZE, "BT Test State of data");
	LOG_HEXDUMP_INF(ctx->data + offset, actual_len, "BT Test Read:");

	return bt_gatt_attr_read(conn, attr, buf, len, offset, ctx->data, TEST_DATA_SIZE);
}

static ssize_t write_custom_char(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	test_svc_ctx_t *ctx = attr->user_data;
	uint16_t actual_len = MIN(len, TEST_DATA_SIZE - offset);

	if (offset >= TEST_DATA_SIZE) {
		LOG_ERR("BT Test Invalid offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (actual_len < len) {
		LOG_ERR("BT Test Write length exceeds the size of the data. requested %d, max %d",
			len, TEST_DATA_SIZE);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(ctx->data + offset, buf, actual_len);
	LOG_INF("BT Test write data at offset %d", offset);
	LOG_HEXDUMP_INF(buf, actual_len, "BT Test write data");
	LOG_HEXDUMP_INF(ctx->data, TEST_DATA_SIZE, "BT Test Final state of data after write");
	update_subscribed();
	return actual_len;
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	test_ctx.notify_enabled = (value & BT_GATT_CCC_NOTIFY) != 0;
	test_ctx.indicate_enabled = (value & BT_GATT_CCC_INDICATE) != 0;
	LOG_INF("BT Test Notify: %s, Indicate: %s", test_ctx.notify_enabled ? "ON" : "OFF",
		test_ctx.indicate_enabled ? "ON" : "OFF");
}

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	if (err) {
		LOG_ERR("BT Test Indication confirmation error: %d", err);
		return;
	}

	LOG_INF("BT Test Indication confirmed by client");
}

static void update_subscribed(void)
{
	int err = -ENOENT;

	if (test_ctx.conn && test_ctx.notify_enabled) {
		err = bt_gatt_notify(test_ctx.conn, &custom_svc.attrs[1], test_ctx.data,
				     TEST_DATA_SIZE);
		if (err) {
			LOG_ERR("BT Test Notify error: %d", err);
		} else {
			LOG_HEXDUMP_INF(test_ctx.data, TEST_DATA_SIZE, "BT Test Notified:");
		}
	}
	if (test_ctx.conn && test_ctx.indicate_enabled) {
		ind_params.attr = &custom_svc.attrs[1];
		ind_params.func = indicate_cb;
		ind_params.data = test_ctx.data;
		ind_params.len = TEST_DATA_SIZE;
		ind_params.destroy = NULL;
		err = bt_gatt_indicate(test_ctx.conn, &ind_params);
		if (err) {
			LOG_ERR("BT Test Indicate error: %d", err);
		} else {
			LOG_HEXDUMP_INF(test_ctx.data, TEST_DATA_SIZE, "BT Test Indication sent:");
		}
	}
}

static void connected(struct bt_conn *conn, uint8_t hci_err)
{
	if (hci_err) {
		LOG_ERR("BT Test Connected error: %d", hci_err);
		return;
	}
	test_ctx.conn = conn;
	LOG_INF("BT Test Connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	test_ctx.conn = NULL;
	test_ctx.notify_enabled = false;
	test_ctx.indicate_enabled = false;
	LOG_INF("BT Test Disconnected, reason: %d", reason);
}

static int advertise(void)
{
	int err;
	(void)bt_conn_cb_register(&conn_cb);
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		(void)bt_conn_cb_unregister(&conn_cb);
	}
	return err;
}

static int cmd_advertise(const struct shell *sh, size_t argc, char *argv[])
{
	bool start;
	int err;

	if (!strcmp(argv[1], "on")) {
		start = true;
	} else if (!strcmp(argv[1], "off")) {
		start = false;
	} else {
		shell_error(sh, "BT Test Invalid argument: %s", argv[1]);
		return -EINVAL;
	}
	if (start) {
		err = advertise();
	} else {
		err = bt_le_adv_stop();
		(void)bt_conn_cb_unregister(&conn_cb);
	}
	if (err < 0) {
		shell_error(sh, "BT Test Failed to %s advertising: %d", start ? "start" : "stop",
			    err);
		return -ENOEXEC;
	}
	shell_print(sh, "BT Test Advertising %s", start ? "started" : "stopped");
	return 0;
}

static int cmd_read(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_hexdump(sh, test_ctx.data, TEST_DATA_SIZE);
	shell_print(sh, "BT Test Read Characteristic value");
	return 0;
}

static int cmd_write(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value[TEST_DATA_SIZE] = {0};
	size_t len = 0;
	size_t len_hex = 0;

	if (argc < 2) {
		shell_error(sh, "BT Test Usage: write <hex_value>");
		return -EINVAL;
	}

	/* Limit the length of the hex value to the size of the data */
	len_hex = strnlen(argv[1], (TEST_DATA_SIZE * 2) + 1);

	if (len_hex > TEST_DATA_SIZE * 2) {
		shell_error(sh, "BT Test Write length exceeds the size of the data");
		return -EINVAL;
	}

	len = hex2bin(argv[1], len_hex, value, sizeof(value));
	if (len == 0) {
		shell_error(sh, "BT Test Failed to parse hex value: %s", argv[1]);
		return -EINVAL;
	}

	memcpy(test_ctx.data, value, len);
	shell_hexdump(sh, test_ctx.data, TEST_DATA_SIZE);
	shell_print(sh, "BT Test Wrote characteristic value");
	update_subscribed();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	bt_test_cmds, SHELL_CMD_ARG(advertise, NULL, "on|off", cmd_advertise, 2, 0),
	SHELL_CMD_ARG(read, NULL, "Read characteristic value", cmd_read, 1, 0),
	SHELL_CMD_ARG(write, NULL, "Write characteristic value <hex>", cmd_write, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(bt_test, &bt_test_cmds, "BLE test service commands", NULL, 2, 0);
