/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief General Central tester sample
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/settings/settings.h>

/* UART payload buffer element size. */
#define UART_BUF_SIZE 20

static const struct device *uart;

struct uart_data_t {
	void *fifo_reserved;
	uint8_t data[UART_BUF_SIZE];
	uint16_t len;
};

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static K_SEM_DEFINE(bt_conn_sem, 0, 2);

static struct bt_conn *default_conn;

static char *dev_names[] = {
	"Nordic_UART_Service",
	"Nordic_Throughput",
	"NCS_HIDS_keyboard",
	"NCS_HIDS_mouse",
	"Nordic_LBS",
	"Test beacon",
};

static char *hid_uuid = (char *)BT_UUID_HIDS;

static void uart_cb(const struct device *uart, void *user_data)
{
	ARG_UNUSED(user_data);
	static struct uart_data_t *rx;

	uart_irq_update(uart);

	if (uart_irq_rx_ready(uart)) {
		int data_length;

		if (!rx) {
			rx = k_malloc(sizeof(*rx));
			if (rx) {
				rx->len = 0;
			} else {
				printk("Not able to allocate UART receive buffer\n");
				return;
			}
		}

		data_length = uart_fifo_read(uart, &rx->data[rx->len],
					     UART_BUF_SIZE - rx->len);
		rx->len += data_length;

		if (rx->len > 0) {
			/* Send buffer to Bluetooth unit if either buffer size
			 * is reached or the char \n or \r is received, which
			 * ever comes first
			 */
			if ((rx->len == UART_BUF_SIZE) ||
			    (rx->data[rx->len - 1] == '\n') ||
			    (rx->data[rx->len - 1] == '\r')) {
				k_fifo_put(&fifo_uart_rx_data, rx);
				rx = NULL;
			}
		}
	}

	if (uart_irq_tx_ready(uart)) {
		struct uart_data_t *buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		uint16_t written = 0;

		/* Nothing in the FIFO, nothing to send */
		if (!buf) {
			uart_irq_tx_disable(uart);
			return;
		}

		while (buf->len > written) {
			written += uart_fifo_fill(uart, &buf->data[written], buf->len - written);
		}

		while (!uart_irq_tx_complete(uart)) {
			/* Wait for the last byte to get
			 * shifted out of the module
			 */
		}

		if (k_fifo_is_empty(&fifo_uart_tx_data)) {
			uart_irq_tx_disable(uart);
		}

		k_free(buf);
	}
}

static void discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	printk("Service discovery completed\n");

	bt_gatt_dm_data_print(dm);

	bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	printk("Service not found\n");
	printk("FAILURE - Service not found\n");
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
	printk("Error while discovering GATT database: (%d)\n", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	if (conn == default_conn) {
		int err = bt_gatt_dm_start(conn, NULL, &discovery_cb, NULL);

		if (err) {
			printk("Failed to start discovery process (err:%d)\n", err);
		}
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%d)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);
	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (err) {
		printk("Failed to set security: %d\n", err);

		gatt_discover(conn);
	}

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		printk("Stop LE scan failed (err %d)\n", err);
	}

	k_sem_give(&bt_conn_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	k_sem_give(&bt_conn_sem);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level, err);
	}

	gatt_discover(conn);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %d\n", addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
	printk("FAILURE - Connection failed\n");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error,
		scan_connecting);

static int uart_init(void)
{
	uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!uart) {
		printk("UART binding failed\n");
		return -ENXIO;
	}

	uart_irq_callback_set(uart, uart_cb);
	uart_irq_rx_enable(uart);

	printk("UART initialized\n");
	return 0;
}

static int scan_init(void)
{
	int err;
	int i;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	for (i = 0; i < ARRAY_SIZE(dev_names); i++) {
		printk("Device %s\n", dev_names[i]);
		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, dev_names[i]);
		if (err) {
			printk("Scanning filters cannot be set (err %d)\n", err);
			printk("FAILURE - Cannot set filters\n");
			return err;
		}
	}

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, hid_uuid);
	if (err) {
		printk("Scanning filters cannot be set (err %d)\n", err);
		printk("FAILURE - Cannot set filters\n");
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER | BT_SCAN_UUID_FILTER,
				    false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
		printk("FAILURE - Cannot set filters\n");
		return err;
	}

	printk("Scan module initialized\n");
	return err;
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	printk("Paired conn: %p, bonded: %d\n", conn, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	printk("Pairing failed conn: %p, reason %d\n", conn, reason);
}

static struct bt_conn_auth_info_cb auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

int main(void)
{
	int err;

	printk("Starting BLE test application\n");

	err = bt_conn_auth_info_cb_register(&auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks.\n");
		return 1;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 1;
	}
	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bt_conn_cb_register(&conn_callbacks);

	int (*module_init[])(void) = { uart_init, scan_init };
	for (size_t i = 0; i < ARRAY_SIZE(module_init); i++) {
		err = (*module_init[i])();
		if (err) {
			return 1;
		}
	}

	do {
		k_sleep(K_MSEC(500));

		err = bt_scan_stop();
		if ((!err) && (err != -EALREADY)) {
			printk("Stop LE scan failed (err %d)\n", err);
		}

		err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			printk("FAILULRE - Scanning failed to start\n");
			continue;
		}
		printk("Scanning successfully started\n");

		err = k_sem_take(&bt_conn_sem, K_MSEC(2000));
		if (err) {
			printk("Connection failed - timeout\n");
			continue;
		}
		k_sleep(K_MSEC(5000));

		err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			printk("FAILURE - Disconnection failed\n");
			printk("Disconnect failed with error: (err %d)\n", err);
			continue;
		}
		printk("Disconnect successful\n");

		err = k_sem_take(&bt_conn_sem, K_MSEC(2000));
		if (err) {
			printk("Disconnection failed - timeout");
			continue;
		}
		k_sleep(K_MSEC(3000));

		err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			printk("FAILULRE - Scanning failed to start\n");
			continue;
		}
		printk("Scanning successfully started\n");

		err = k_sem_take(&bt_conn_sem, K_MSEC(2000));
		if (err) {
			printk("Connection failed - timeout");
			continue;
		}
		k_sleep(K_MSEC(10000));

		err = bt_conn_disconnect(default_conn,
					 BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			printk("FAILURE - Disconnection failed\n");
			printk("Disconnect failed with error: (err %d)\n", err);
			continue;
		}
		printk("Disconnect successful\n");

		printk("SUCCESS\n");
		return 0;
	} while (1);
}
