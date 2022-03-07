/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

#include <errno.h>
#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <device.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>
#include <sys/ring_buffer.h>

#include <dk_buttons_and_leds.h>

#include <settings/settings.h>

#include <logging/log.h>

#define LOG_MODULE_NAME central_usb_nus_client
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

#define KEY_PASSKEY_ACCEPT DK_BTN1_MSK
#define KEY_PASSKEY_REJECT DK_BTN2_MSK

#define NUS_WRITE_TIMEOUT K_MSEC(150)


K_SEM_DEFINE(nus_write_sem, 0, 1);
K_SEM_DEFINE(nus_tx_sem, 0, 1);

static struct bt_conn *default_conn;
static struct bt_nus_client nus_client;

uint8_t *rx_buf = NULL;
uint16_t rx_buf_len = 0;

/*
 * Callback for receiving BLE data. Allocate a buffer and copy the data to it.
 * The main loop will check for non-NULL buffer and process it.
 * 
 * Buffer will be freed on interrupt that fills USB FIFO with this data.
 */
static uint8_t ble_data_received(struct bt_nus_client *nus,
						const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(nus);
	if (rx_buf == NULL) {
		rx_buf = k_malloc(len);
		memcpy(rx_buf, data, len);
		rx_buf_len = len;
	} else {
		LOG_WRN("Incoming BLE data discarded");
	}

	return BT_GATT_ITER_CONTINUE;
}

/*
 * Callback for when BLE data has been sent. Post the semaphore to indicate
 * that the device can send more data.
 */
static void ble_data_sent(struct bt_nus_client *nus, uint8_t err,
					const uint8_t *const data, uint16_t len)
{
	ARG_UNUSED(nus);

	k_sem_give(&nus_write_sem);

	if (err) {
		LOG_WRN("ATT error code: 0x%02X", err);
	}
}

static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	struct bt_nus_client *nus = context;
	LOG_INF("Service discovery completed");

	bt_gatt_dm_data_print(dm);

	bt_nus_handles_assign(dm, nus);
	bt_nus_subscribe_receive(nus);

	bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_INF("Service not found");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != default_conn) {
		return;
	}

	err = bt_gatt_dm_start(conn,
			       BT_UUID_NUS_SERVICE,
			       &discovery_cb,
			       &nus_client);
	if (err) {
		LOG_ERR("could not start the discovery procedure, error "
			"code: %d", err);
	}
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) {
		LOG_INF("MTU exchange done");
	} else {
		LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_INF("Failed to connect to %s (%d)", log_strdup(addr),
			conn_err);

		if (default_conn == conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;

			err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			if (err) {
				LOG_ERR("Scanning failed to start (err %d)",
					err);
			}
		}

		return;
	}

	LOG_INF("Connected: %s", log_strdup(addr));
	dk_set_led_on(DK_LED1);

	static struct bt_gatt_exchange_params exchange_params;

	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_WRN("MTU exchange failed (err %d)", err);
	}

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (err) {
		LOG_WRN("Failed to set security: %d", err);

		gatt_discover(conn);
	}

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr),
		reason);
	dk_set_led_off(DK_LED1);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)",
			err);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", log_strdup(addr),
			level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", log_strdup(addr),
			level, err);
	}

	gatt_discover(conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d",
		log_strdup(addr), connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

static int nus_client_init(void)
{
	int err;
	struct bt_nus_client_init_param init = {
		.cb = {
			.received = ble_data_received,
			.sent = ble_data_sent,
		}
	};

	err = bt_nus_client_init(&nus_client, &init);
	if (err) {
		LOG_ERR("NUS Client initialization failed (err %d)", err);
		return err;
	}

	LOG_INF("NUS Client module initialized");
	return err;
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

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
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	LOG_INF("Scan module initialized");
	return err;
}


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", log_strdup(addr));
}


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", log_strdup(addr),
		bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_WRN("Pairing failed conn: %s, reason %d", log_strdup(addr),
		reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

#define RING_BUF_SIZE 16
uint8_t ring_buffer_tx[RING_BUF_SIZE];

struct ring_buf ringbuf_tx;

/*
 * USB interrupt handler.
 */
static void interrupt_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		/*
		 * When the RX interrupt is ready, read the data from USB and put it in
		 * the ring buffer. Check if the ring buffer is full or if the data contains
		 * a carriage return. If so, give the semaphore for BLE send.
		 */
		if (uart_irq_rx_ready(dev)) {
			int recv_len, rb_len;
			uint8_t buffer[64];
			size_t len = MIN(ring_buf_space_get(&ringbuf_tx),
					 sizeof(buffer));

			recv_len = uart_fifo_read(dev, buffer, len);
			if (recv_len < 0) {
				LOG_ERR("Failed to read UART FIFO");
				recv_len = 0;
			};

			rb_len = ring_buf_put(&ringbuf_tx, buffer, recv_len);
			if (rb_len < recv_len) {
				LOG_ERR("Drop %u bytes", recv_len - rb_len);
			}
			if (ring_buf_size_get(&ringbuf_tx) >= RING_BUF_SIZE - 1) {
				k_sem_give(&nus_tx_sem);
			} else {
				for (int i = 0; i < recv_len; i++) {
					if (buffer[i] == '\r') {
						k_sem_give(&nus_tx_sem);
						break;
					}
				}
			}
		}
		/* 
		 * When the TX interrupt is ready, fill the USB FIFO with data from the RX buffer,
		 * then free the buffer.
		 */
		if (uart_irq_tx_ready(dev)) {
			int ret = uart_fifo_fill(dev, rx_buf, rx_buf_len);
			if (ret < rx_buf_len) {
				LOG_ERR("Drop %u bytes", rx_buf_len - ret);
			}
			k_free(rx_buf);
			rx_buf = NULL;
			uart_irq_tx_disable(dev);
			dk_set_led_off(DK_LED3);
		}
	}
}

void main(void)
{
	int err;
	const struct device *dev;
	uint32_t baudrate;

	dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	if (!device_is_ready(dev)) {
		LOG_ERR("CDC-ACM device not ready");
		return;
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)", err);
		return;
	}

	err = usb_enable(NULL);
	if (err) {
		LOG_ERR("USB enable failed (err %d)", err);
		return;
	}

	ring_buf_init(&ringbuf_tx, sizeof(ring_buffer_tx), ring_buffer_tx);

	k_sleep(K_SECONDS(1));

	err = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (err) {
		LOG_WRN("Failed to get baudrate (err %d)", err);
	} else {
		LOG_INF("UART baudrate: %u", baudrate);
	}

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization callbacks.");
		return;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	int (*module_init[])(void) = {scan_init, nus_client_init};
	for (size_t i = 0; i < ARRAY_SIZE(module_init); i++) {
		err = (*module_init[i])();
		if (err) {
			return;
		}
	}

	LOG_INF("Starting Bluetooth Central USB example");

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return;
	}

	LOG_INF("Scanning successfully started");
	uart_irq_callback_set(dev, interrupt_handler);
	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	for (;;) {
		if (!k_sem_take(&nus_tx_sem, K_NO_WAIT)) {
			uint8_t buffer[RING_BUF_SIZE];
			int rb_len;

			rb_len = ring_buf_get(&ringbuf_tx, buffer, sizeof(buffer));
			if (!rb_len) {
				LOG_DBG("Ring buffer empty, do not send");
				continue;
			}
			dk_set_led_on(DK_LED2);
			err = bt_nus_client_send(&nus_client, buffer, rb_len);
			if (err) {
				LOG_WRN("Failed to send data over BLE connection"
					"(err %d)", err);
			}
			err = k_sem_take(&nus_write_sem, NUS_WRITE_TIMEOUT);
			if (err) {
				LOG_WRN("NUS send timeout");
			}
			dk_set_led_off(DK_LED2);
		}
		if (rx_buf != NULL) {
			dk_set_led_on(DK_LED3);
			uart_irq_tx_enable(dev);
		}
		k_sleep(K_MSEC(10));
	}
}
