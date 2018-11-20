/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

#include <errno.h>
#include <zephyr.h>
#include <misc/byteorder.h>
#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_c.h>
#include <bluetooth/common/gatt_dm.h>
#include <bluetooth/common/scan.h>

#include <uart.h>

/* UART payload buffer element size. */
#define UART_BUF_SIZE 20

static struct device  *uart;

struct uart_data_t {
	void *fifo_reserved;
	u8_t  data[UART_BUF_SIZE];
	u16_t len;
};

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static struct bt_conn *default_conn;
static struct bt_gatt_nus_c nus_c;

static void ble_data_sent(u8_t err, const u8_t *const data, u16_t len)
{
	struct uart_data_t *buf;

	/* Retrieve buffer context. */
	buf = CONTAINER_OF(data, struct uart_data_t, data);
	k_free(buf);

	if (err) {
		printk("ATT error code: 0x%02X\n", err);
	}
}

static u8_t ble_data_received(const u8_t *const data, u16_t len)
{
	for (u16_t pos = 0; pos != len;) {
		struct uart_data_t *tx = k_malloc(sizeof(*tx));

		if (!tx) {
			printk("Not able to allocate UART send data buffer\n");
			return BT_GATT_ITER_CONTINUE;
		}

		/* Keep the last byte of TX buffer for potential LF char. */
		size_t tx_data_size = sizeof(tx->data) - 1;

		if ((len - pos) > tx_data_size) {
			tx->len = tx_data_size;
		} else {
			tx->len = (len - pos);
		}

		memcpy(tx->data, &data[pos], tx->len);

		pos += tx->len;

		/* Append the LF character when the CR character triggered
		 * transmission from the peer.
		 */
		if ((pos == len) && (data[len - 1] == '\r')) {
			tx->data[tx->len] = '\n';
			tx->len++;
		}

		k_fifo_put(&fifo_uart_tx_data, tx);
	}

	/* Start the UART transfer by enabling the TX ready interrupt */
	uart_irq_tx_enable(uart);

	return BT_GATT_ITER_CONTINUE;
}

static void uart_cb(struct device *uart)
{
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
		struct uart_data_t *buf =
			k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		u16_t written = 0;

		/* Nothing in the FIFO, nothing to send */
		if (!buf) {
			uart_irq_tx_disable(uart);
			return;
		}

		while (buf->len > written) {
			written += uart_fifo_fill(uart,
						  &buf->data[written],
						  buf->len - written);
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

static void discovery_complete(struct bt_conn *conn,
			       const struct bt_gatt_attr *attrs,
			       size_t attrs_len)
{
	printk("Service discovery completed\n");

	bt_gatt_dm_data_print();

	bt_gatt_nus_c_handles_assign(&nus_c, conn, attrs, attrs_len);
	bt_gatt_nus_c_tx_notif_enable(&nus_c);

	bt_gatt_dm_data_release();
}

static void discovery_service_not_found(struct bt_conn *conn)
{
	printk("Service not found\n");
}

static void discovery_error(struct bt_conn *conn, int err)
{
	printk("Error while discovering GATT database: (%d)\n", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void connected(struct bt_conn *conn, u8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%d)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);
	if (bt_conn_security(conn, BT_SECURITY_MEDIUM)) {
		printk("Failed to set security level %d\n", BT_SECURITY_HIGH);
	}

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		printk("Stop LE scan failed (err %d)\n", err);
	}

	if (conn == default_conn) {
		err = bt_gatt_dm_start(conn, BT_UUID_NUS_SERVICE,
				       &discovery_cb);

		if (err) {
			printk("Failed to start discovery process (err:%d)\n",
				err);
		}
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	printk("Security level was raised to %d\n", level);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %d\n",
		addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

static struct bt_scan_cb scan_cb = {
	.filter_match = scan_filter_match,
	.connecting_error = scan_connecting_error,
	.connecting = scan_connecting,
};

static int uart_init(void)
{
	uart = device_get_binding(DT_UART_0_NAME);
	if (!uart) {
		printk("UART binding failed\n");
		return -ENXIO;
	}

	uart_irq_callback_set(uart, uart_cb);
	uart_irq_rx_enable(uart);

	printk("UART initialized\n");
	return 0;
}

static int nus_client_init(void)
{
	int err;
	struct bt_gatt_nus_c_init_param nus_c_init_obj = {
		.cbs = {
			.data_received = ble_data_received,
			.data_sent = ble_data_sent,
		}
	};

	err = bt_gatt_nus_c_init(&nus_c, &nus_c_init_obj);
	if (err) {
		printk("NUS Client initialization failed (err %d)\n", err);
		return err;
	}

	printk("NUS Client module initialized\n");
	return err;
}

static int scan_init(void)
{
	int err;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_NUS_SERVICE);
	if (err) {
		printk("Scanning filters cannot be set (err %d)\n", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
		return err;
	}

	printk("Scan module initialized\n");
	return err;
}

void main(void)
{
	int err;

	printk("Starting NUS Client example\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	int (*module_init[])(void) = {uart_init, scan_init, nus_client_init};
	for (size_t i = 0; i < ARRAY_SIZE(module_init); i++) {
		err = (*module_init[i])();
		if (err) {
			return;
		}
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}
	printk("Scanning successfully started\n");

	for (;;) {
		/* Wait indefinitely for data to be sent over Bluetooth */
		struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data,
						     K_FOREVER);

		err = bt_gatt_nus_c_send(&nus_c, buf->data, buf->len);
		if (err) {
			printk("Failed to send data over BLE connection"
			       "(err %d)\n", err);
		}
	}
}
