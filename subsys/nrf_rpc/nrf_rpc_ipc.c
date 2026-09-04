/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc.h>
#include <nrf_rpc_tr.h>
#include <nrf_rpc_errno.h>
#include <nrf_rpc/nrf_rpc_ipc.h>

#if CONFIG_OPENAMP
#include <openamp/rpmsg.h>
#endif /* CONFIG_OPENAMP */
#include <zephyr/ipc/ipc_service.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_rpc_ipc, CONFIG_NRF_RPC_TR_LOG_LEVEL);

#define EPT_BIND_TIMEOUT_MS (CONFIG_NRF_RPC_IPC_SERVICE_BIND_TIMEOUT_MS)

/* Utility macro for dumping content of the packets with limit of 32 bytes
 * to prevent overflowing the logs.
 */
#define DUMP_LIMITED_DBG(memory, len, text)				       \
do {									       \
	if ((len) > 32) {						       \
		LOG_HEXDUMP_DBG(memory, 32, text " (truncated)");	       \
	} else {							       \
		LOG_HEXDUMP_DBG(memory, (len), text);			       \
	}								       \
} while (0)

/* Endpoint states */
enum {
	/* Endpoint is uninitialized */
	NRF_RPC_IPC_STATE_UNINITIALIZED,
	/* Waiting for bond */
	NRF_RPC_IPC_STATE_WAITING,
	/* Endpoint is bonded and ready for transmission */
	NRF_RPC_IPC_STATE_READY,
	/* Error on endpoint, -EPIPE should be returned */
	NRF_RPC_IPC_STATE_ERROR
};

/* Translates error code from the lower layer to nRF RPC error code. */
static int translate_error(int ll_err)
{
	switch (ll_err) {
	case -EINVAL:
		return -NRF_EINVAL;
	case -EIO:
		return -NRF_EIO;
	case -EALREADY:
		return -NRF_EALREADY;
	case -EBADMSG:
		return -NRF_EBADMSG;

#if CONFIG_OPENAMP
	case RPMSG_ERR_BUFF_SIZE:
	case RPMSG_ERR_NO_MEM:
	case RPMSG_ERR_NO_BUFF:
		return -NRF_ENOMEM;
	case RPMSG_ERR_PARAM:
		return -NRF_EINVAL;
	case RPMSG_ERR_DEV_STATE:
	case RPMSG_ERR_INIT:
	case RPMSG_ERR_ADDR:
		return -NRF_EIO;
#endif /* CONFIG_OPENAMP */

	default:
		if (ll_err < 0) {
			return -NRF_EIO;
		}
		break;
	}

	return 0;
}

static void ept_bound(void *priv)
{
	const struct nrf_rpc_tr *transport = priv;
	struct nrf_rpc_ipc *ipc_config = transport->ctx;

	LOG_DBG("nRF RPC endpoint %s connected", ipc_config->endpoint.ept_cfg.name);

	k_event_set(&ipc_config->endpoint.ept_bond, 0x01);
}

static void packet_handle(const struct nrf_rpc_tr *transport, const void *data, size_t len)
{
	struct nrf_rpc_ipc *ipc_config = transport->ctx;

	__ASSERT_NO_MSG(ipc_config->receive_cb != NULL);

	ipc_config->receive_cb(transport, data, len, ipc_config->context);
}

#if defined(CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD)
/* Defer processing of the received packet to the transport Rx thread. The IPC Service Rx
 * buffer holding feature is used, so the packet does not need to be copied. The thread
 * releases the buffer once the packet is processed.
 */
static void packet_defer(const struct nrf_rpc_tr *transport, const void *data, size_t len)
{
	struct nrf_rpc_ipc *ipc_config = transport->ctx;
	struct nrf_rpc_ipc_rx_packet packet = { .data = data, .len = len };
	int err;

	err = ipc_service_hold_rx_buffer(&ipc_config->endpoint.ept, (void *)data);
	if (err < 0) {
		__ASSERT_NO_MSG(!k_is_in_isr());
		/* The backend does not support holding the Rx buffer, so the packet cannot
		 * outlive this callback. Such backends invoke the callback from a thread
		 * context, so the packet can be processed here.
		 */
		LOG_DBG("Failed to hold Rx buffer: %d, processing packet in place", err);
		packet_handle(transport, data, len);
		return;
	}

	err = k_msgq_put(ipc_config->rx_msgq, &packet, K_NO_WAIT);
	if (err < 0) {
		LOG_ERR("Rx queue full, dropping packet");

		err = ipc_service_release_rx_buffer(&ipc_config->endpoint.ept, (void *)data);
		if (err < 0) {
			LOG_ERR("Failed to release Rx buffer: %d", err);
		}
	}
}

static void rx_thread_entry(void *p1, void *p2, void *p3)
{
	const struct nrf_rpc_tr *transport = p1;
	struct nrf_rpc_ipc *ipc_config = transport->ctx;
	struct nrf_rpc_ipc_rx_packet packet;
	int err;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		err = k_msgq_get(ipc_config->rx_msgq, &packet, K_FOREVER);
		if (err < 0) {
			LOG_ERR("Failed to get packet from Rx queue: %d", err);
			continue;
		}

		packet_handle(transport, packet.data, packet.len);

		err = ipc_service_release_rx_buffer(&ipc_config->endpoint.ept, (void *)packet.data);
		if (err < 0) {
			LOG_ERR("Failed to release Rx buffer: %d", err);
		}
	}
}

static void rx_thread_start(const struct nrf_rpc_tr *transport)
{
	struct nrf_rpc_ipc *ipc_config = transport->ctx;
	k_tid_t tid;

	tid = k_thread_create(ipc_config->rx_thread, ipc_config->rx_stack,
			      CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD_STACK_SIZE, rx_thread_entry,
			      (void *)transport, NULL, NULL,
			      CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(tid, ipc_config->endpoint.ept_cfg.name);
}
#else
static void rx_thread_start(const struct nrf_rpc_tr *transport)
{
	ARG_UNUSED(transport);
}
#endif /* CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD */

static void ept_received(const void *data, size_t len, void *priv)
{
	const struct nrf_rpc_tr *transport = priv;

	__ASSERT_NO_MSG(data != NULL);

	DUMP_LIMITED_DBG(data, len, "Received");

#if defined(CONFIG_NRF_RPC_IPC_SERVICE_RX_THREAD)
	/* The endpoint receive callback may be called from an interrupt context. */
	packet_defer(transport, data, len);
#else
	packet_handle(transport, data, len);
#endif
}

static void ept_error(const char *message, void *priv)
{
	LOG_ERR("Endpoint error: \"%s\"", message);
	__ASSERT_NO_MSG(false);
}

static int init(const struct nrf_rpc_tr *transport, nrf_rpc_tr_receive_handler_t receive_cb,
		void *context)
{
	int err;
	struct nrf_rpc_ipc *ipc_config = transport->ctx;
	struct nrf_rpc_ipc_endpoint *endpoint = &ipc_config->endpoint;
	struct ipc_ept_cfg *cfg = &ipc_config->endpoint.ept_cfg;

	if (ipc_config->state != NRF_RPC_IPC_STATE_UNINITIALIZED) {
		return 0;
	}

	if (receive_cb == NULL) {
		LOG_ERR("No transport receive callback");
		return -NRF_EINVAL;
	}

	err = ipc_service_open_instance(ipc_config->ipc);
	if (err && err != -EALREADY) {
		LOG_ERR("IPC service instance initialization failed: %d\n", err);
		ipc_config->state = NRF_RPC_IPC_STATE_ERROR;
		return translate_error(err);
	}

	ipc_config->receive_cb = receive_cb;
	ipc_config->context = context;

	cfg->cb.bound = ept_bound;
	cfg->cb.received = ept_received;
	cfg->cb.error = ept_error;
	cfg->priv = (void *)transport;

	k_event_init(&endpoint->ept_bond);

	/* Start the Rx thread before the endpoint is registered, so that it is ready
	 * to process the packets as soon as they can be received.
	 */
	rx_thread_start(transport);

	err = ipc_service_register_endpoint(ipc_config->ipc, &endpoint->ept, cfg);
	if (err) {
		LOG_ERR("Registering endpoint failed with %d", err);
		ipc_config->state = NRF_RPC_IPC_STATE_ERROR;
		return translate_error(err);
	}

	ipc_config->endpoint.timeout = K_TIMEOUT_ABS_MS(k_uptime_get() + EPT_BIND_TIMEOUT_MS);
	ipc_config->state = NRF_RPC_IPC_STATE_WAITING;

	return 0;
}

static int send_packet(struct nrf_rpc_ipc_endpoint *endpoint, const uint8_t *data, size_t length)
{
	int err;

	if (IS_ENABLED(CONFIG_NRF_RPC_IPC_SERVICE_TX_ZERO_COPY)) {
		err = ipc_service_send_nocopy(&endpoint->ept, data, length);
	} else {
		err = ipc_service_send(&endpoint->ept, data, length);
		k_free((void *)data);
	}
	if (err < 0) {
		LOG_ERR("ipc_service_send_nocopy returned err: %d", err);
		if (IS_ENABLED(CONFIG_NRF_RPC_IPC_SERVICE_TX_ZERO_COPY)) {
			(void)ipc_service_drop_tx_buffer(&endpoint->ept, data);
		}
	} else if (err > 0) {
		LOG_DBG("Sent %u bytes", err);
		err = 0;
	}

	return err;
}

static int send(const struct nrf_rpc_tr *transport, const uint8_t *data, size_t length)
{
	int err;
	struct nrf_rpc_ipc *ipc_config = transport->ctx;
	struct nrf_rpc_ipc_endpoint *endpoint = &ipc_config->endpoint;

	switch (ipc_config->state) {
	case NRF_RPC_IPC_STATE_UNINITIALIZED:
		LOG_ERR("nRF RPC transport is not initialized");
		return -NRF_EFAULT;
	case NRF_RPC_IPC_STATE_WAITING:
		if (!k_event_wait(&endpoint->ept_bond, 0x01, false,
				ipc_config->endpoint.timeout)) {
			LOG_ERR("IPC endpoint bond timeout");
			return -NRF_EPIPE;
		}
		ipc_config->state = NRF_RPC_IPC_STATE_READY;
		break;
	case NRF_RPC_IPC_STATE_READY:
		break;
	case NRF_RPC_IPC_STATE_ERROR:
		LOG_ERR("IPC endpoint error");
		return -NRF_EPIPE;
	}

	LOG_DBG("Sending %u bytes", length);
	DUMP_LIMITED_DBG(data, length, "Data: ");

	err = send_packet(endpoint, data, length);

	return translate_error(err);
}

static void *tx_buf_alloc(const struct nrf_rpc_tr *transport, size_t *size)
{
	void *data = NULL;
	struct nrf_rpc_ipc *ipc_config = transport->ctx;
	struct nrf_rpc_ipc_endpoint *endpoint = &ipc_config->endpoint;
	uint32_t buf_size = *size;
	int err;

	if (ipc_config->state == NRF_RPC_IPC_STATE_UNINITIALIZED) {
		LOG_ERR("nRF RPC transport is not initialized");
		goto error;
	}

	if (!IS_ENABLED(CONFIG_NRF_RPC_IPC_SERVICE_TX_ZERO_COPY)) {
		data = k_malloc(*size);
		if (!data) {
			LOG_ERR("Failed to allocate Tx buffer.");
			goto error;
		}
		return data;
	}

	/* Ensure the endpoint is bonded before getting the TX buffer. */
	err = k_event_wait(&endpoint->ept_bond, 0x01, false, endpoint->timeout);
	if (err < 0) {
		LOG_ERR("IPC endpoint bond timeout");
		err = -NRF_EPIPE;
		goto error;
	}

	err = ipc_service_get_tx_buffer(&endpoint->ept, &data, &buf_size, K_FOREVER);
	if (err < 0) {
		LOG_ERR("Failed to get Tx buffer: %d", err);
		goto error;
	}

	*size = buf_size;

	return data;

error:
	/* It should fail to avoid writing to NULL buffer. */
	k_oops();
	*size = 0;
	return NULL;
}

static void tx_buf_free(const struct nrf_rpc_tr *transport, void *buf)
{
	struct nrf_rpc_ipc *ipc_config = transport->ctx;

	if (ipc_config->state == NRF_RPC_IPC_STATE_UNINITIALIZED) {
		LOG_ERR("nRF RPC transport is not initialized");
		return;
	}

	if (IS_ENABLED(CONFIG_NRF_RPC_IPC_SERVICE_TX_ZERO_COPY)) {
		(void)ipc_service_drop_tx_buffer(&ipc_config->endpoint.ept, buf);
	} else {
		k_free(buf);
	}
}

const struct nrf_rpc_tr_api nrf_rpc_ipc_service_api = {
	.init = init,
	.send = send,
	.tx_buf_alloc = tx_buf_alloc,
	.tx_buf_free = tx_buf_free
};
