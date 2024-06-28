/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <shell/shell_bt_nus.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(shell_bt_nus, CONFIG_SHELL_BT_NUS_LOG_LEVEL);

SHELL_BT_NUS_DEFINE(shell_transport_bt_nus,
		    CONFIG_SHELL_BT_NUS_TX_RING_BUFFER_SIZE,
		    CONFIG_SHELL_BT_NUS_RX_RING_BUFFER_SIZE);

SHELL_DEFINE(shell_bt_nus, "bt_nus:~$ ", &shell_transport_bt_nus,
	     CONFIG_SHELL_BT_NUS_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_BT_NUS_LOG_MESSAGE_QUEUE_TIMEOUT,
	     SHELL_FLAG_OLF_CRLF);

static K_MUTEX_DEFINE(bt_data_mutex);
static K_SEM_DEFINE(shell_bt_nus_ready, 0, 1);

static bool is_init;

static void rx_callback(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	const struct shell_bt_nus *bt_nus =
		(const struct shell_bt_nus *)shell_transport_bt_nus.ctx;
	uint32_t done = ring_buf_put(bt_nus->rx_ringbuf, data, len);

	LOG_DBG("Received %d bytes.", len);
	if (done < len) {
		LOG_INF("RX ring buffer full. Dropping %d bytes", len - done);
	}

	bt_nus->ctrl_blk->handler(SHELL_TRANSPORT_EVT_RX_RDY,
				  bt_nus->ctrl_blk->context);
}

static void tx_try(const struct shell_bt_nus *bt_nus)
{
	uint8_t *buf;
	uint32_t size;
	uint32_t req_len = bt_nus_get_mtu(bt_nus->ctrl_blk->conn);

	int err = k_mutex_lock(&bt_data_mutex, K_FOREVER);

	if (err) {
		LOG_ERR("Failed to lock mutex, (err: %d)", err);
		__ASSERT(false, "Cannot lock mutex");
	}

	size = ring_buf_get_claim(bt_nus->tx_ringbuf, &buf, req_len);

	if (size) {
		int err, err2;

		err = bt_nus_send(bt_nus->ctrl_blk->conn, buf, size);
		err2 = ring_buf_get_finish(bt_nus->tx_ringbuf, size);
		__ASSERT_NO_MSG(err2 == 0);

		if (err == 0) {
			LOG_DBG("Sent %d bytes", size);
		} else {
			LOG_INF("Failed to send %d bytes (%d error)",
								size, err);
			bt_nus->ctrl_blk->tx_busy = 0;
		}
	} else {
		bt_nus->ctrl_blk->tx_busy = 0;
	}

	err = k_mutex_unlock(&bt_data_mutex);
	if (err) {
		LOG_ERR("Failed to unlock mutex, (err: %d)", err);
		__ASSERT(false, "Cannot unlock mutex");
	}
}

static void tx_callback(struct bt_conn *conn)
{
	const struct shell_bt_nus *bt_nus =
		(const struct shell_bt_nus *)shell_transport_bt_nus.ctx;


	LOG_DBG("Sent operation completed");
	tx_try(bt_nus);
	bt_nus->ctrl_blk->handler(SHELL_TRANSPORT_EVT_TX_RDY,
				  bt_nus->ctrl_blk->context);
}

static void send_enabled_callback(enum bt_nus_send_status status)
{
	if (status == BT_NUS_SEND_STATUS_ENABLED) {
		LOG_DBG("NUS notification has been enabled");
		k_sem_give(&shell_bt_nus_ready);
	}
}

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	const struct shell_bt_nus *bt_nus =
			(struct shell_bt_nus *)transport->ctx;

	LOG_DBG("Initialized");
	bt_nus->ctrl_blk->handler = evt_handler;
	bt_nus->ctrl_blk->context = context;

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking_tx)
{
	const struct shell_bt_nus *bt_nus =
			(struct shell_bt_nus *)transport->ctx;

	if (blocking_tx) {
		/* transport cannot work in blocking mode, shut down */
		bt_nus->ctrl_blk->conn = NULL;
		return -ENOTSUP;
	}

	LOG_DBG("Waiting for the NUS notification to be enabled");
	k_sem_take(&shell_bt_nus_ready, K_FOREVER);

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	const struct shell_bt_nus *bt_nus =
			(struct shell_bt_nus *)transport->ctx;

	*cnt = ring_buf_get(bt_nus->rx_ringbuf, data, length);

	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	const struct shell_bt_nus *bt_nus =
			(struct shell_bt_nus *)transport->ctx;

	if (bt_nus->ctrl_blk->conn == NULL) {
		*cnt = length;
		return 0;
	}

	*cnt = ring_buf_put(bt_nus->tx_ringbuf, data, length);
	LOG_DBG("Write req:%d accept:%d", length, *cnt);

	if (atomic_set(&bt_nus->ctrl_blk->tx_busy, 1) == 0) {
		tx_try(bt_nus);
	}

	return 0;
}

void shell_bt_nus_disable(void)
{
	const struct shell_bt_nus *bt_nus =
			(const struct shell_bt_nus *)shell_transport_bt_nus.ctx;

	bt_nus->ctrl_blk->conn = NULL;
	k_sem_give(&shell_bt_nus_ready);
}

void shell_bt_nus_enable(struct bt_conn *conn)
{
	int err;
	const struct shell_bt_nus *bt_nus =
			(const struct shell_bt_nus *)shell_transport_bt_nus.ctx;
	bool log_backend = CONFIG_SHELL_BT_NUS_INIT_LOG_LEVEL > 0;
	uint32_t level =
		(CONFIG_SHELL_BT_NUS_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_BT_NUS_INIT_LOG_LEVEL;

	bt_nus->ctrl_blk->conn = conn;

	k_sem_reset(&shell_bt_nus_ready);

	if (!is_init) {
		struct shell_backend_config_flags cfg_flags = SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

		err = shell_init(&shell_bt_nus, NULL, cfg_flags, log_backend, level);

		__ASSERT_NO_MSG(err == 0);
		is_init = true;
	}
}

const struct shell_transport_api shell_bt_nus_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read,
};

int shell_bt_nus_init(void)
{
	struct bt_nus_cb callbacks = {
		.received = rx_callback,
		.sent = tx_callback,
		.send_enabled = send_enabled_callback
	};

	return bt_nus_init(&callbacks);
}
