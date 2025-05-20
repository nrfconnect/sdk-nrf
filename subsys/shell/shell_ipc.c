/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <shell/shell_ipc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(shell_ipc, CONFIG_SHELL_IPC_LOG_LEVEL);

RING_BUF_DECLARE(shell_rx_ring_buffer, CONFIG_SHELL_IPC_BACKEND_RX_RING_BUFFER_SIZE);

/* Shell IPC transport API declaration. */
static const struct shell_transport_api shell_ipc_transport_api;

/* Shell IPC instance. */
static struct shell_ipc {
	/* IPC Service endpoint configuration. */
	struct ipc_ept_cfg ept_cfg;

	/* IPC Service endpoint structure. */
	struct ipc_ept ept;

	/* IPC Service endpoint bond semaphore. */
	struct k_sem ept_bond_sem;

	/* Shell transport handlers. */
	shell_transport_handler_t handler;

	/* Shell transport context. */
	void *context;

	/* Pointer to the Rx ring buffer instance. */
	struct ring_buf *rx_ringbuf;
} shell_ipc_transport = {
	.rx_ringbuf = &shell_rx_ring_buffer
};

static struct shell_transport shell_transport_ipc = {
	.api = &shell_ipc_transport_api,
	.ctx = &shell_ipc_transport
};

SHELL_DEFINE(shell_ipc, CONFIG_SHELL_PROMPT_IPC, &shell_transport_ipc,
	     CONFIG_SHELL_IPC_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_IPC_LOG_MESSAGE_QUEUE_TIMEOUT,
	     SHELL_FLAG_OLF_CRLF);

static void ipc_bound(void *priv)
{
	struct shell_ipc *sh_ipc = priv;

	LOG_DBG("Shell IPC Service endpoint bonded");

	k_sem_give(&sh_ipc->ept_bond_sem);
}

static void ipc_received(const void *data, size_t len, void *priv)
{
	struct shell_ipc *sh_ipc = priv;
	uint32_t written_bytes;

	LOG_DBG("Received %d bytes.", len);

	written_bytes = ring_buf_put(sh_ipc->rx_ringbuf, data, len);
	if (written_bytes < len) {
		LOG_INF("RX ring buffer full. Dropping %d bytes", len - written_bytes);
	}

	sh_ipc->handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_ipc->context);
}

static struct ipc_service_cb ipc_cb = {
	.bound = ipc_bound,
	.received = ipc_received
};

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	int err;
	struct shell_ipc *sh_ipc = transport->ctx;

	err = ipc_service_send(&sh_ipc->ept, data, length);
	if (err < 0) {
		return err;
	}

	*cnt = length;

	sh_ipc->handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_ipc->context);

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_ipc *sh_ipc = transport->ctx;

	*cnt = ring_buf_get(sh_ipc->rx_ringbuf, data, length);

	return 0;
}

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	int err;
	struct shell_ipc *ipc = transport->ctx;
	const struct device *dev = config;

	ipc->ept_cfg.name = CONFIG_SHELL_IPC_ENDPOINT_NAME;
	ipc->ept_cfg.cb = ipc_cb;
	ipc->ept_cfg.priv = ipc;
	ipc->handler = evt_handler;
	ipc->context = context;

	err = k_sem_init(&ipc->ept_bond_sem, 0, 1);
	if (err) {
		return err;
	}

	err = ipc_service_open_instance(dev);
	if (err < 0 && err != -EALREADY) {
		LOG_ERR("ipc_service_open_instance() failure\n");
		return err;
	}

	err = ipc_service_register_endpoint(dev, &ipc->ept, &ipc->ept_cfg);
	if (err < 0) {
		LOG_ERR("ipc_service_register_endpoint() failure\n");
		return err;
	}

	LOG_DBG("Shell IPC Service initialization done");

	k_sem_take(&ipc->ept_bond_sem, K_FOREVER);

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	ARG_UNUSED(transport);

	return 0;
}

static int enable(const struct shell_transport *transport,
		  bool blocking_tx)
{
	ARG_UNUSED(transport);
	ARG_UNUSED(blocking_tx);

	return 0;
}

static const struct shell_transport_api shell_ipc_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read,
};

static int enable_shell_ipc(void)
{

	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_ipc));
	bool log_backend = CONFIG_SHELL_IPC_INIT_LOG_LEVEL > 0;
	uint32_t level = (CONFIG_SHELL_IPC_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		      CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_IPC_INIT_LOG_LEVEL;
	static const struct shell_backend_config_flags cfg_flags =
					SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	return shell_init(&shell_ipc, dev, cfg_flags, log_backend, level);
}

SYS_INIT(enable_shell_ipc, APPLICATION, 0);

const struct shell *shell_backend_ipc_get_ptr(void)
{
	return &shell_ipc;
}
