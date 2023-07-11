/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/__assert.h>

#include <nfc_t4t_lib.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_NFC_THREAD_CALLBACK),
	     "NFC callback cannot be called from ISR context");

#if (IS_ENABLED(CONFIG_LOG) && (CONFIG_SHELL_NFC_LOG_LEVEL != 0) && \
	!IS_ENABLED(CONFIG_LOG_MODE_DEFERRED))
#warning NFC shell backend might be unstable when logger is enabled and it is not in the deferred \
	 mode. Synchronous logs processing can add to big delay in handling NFC events.
#endif

#define NFC_SHELL_HEADER_SIZE 1U
#define VT100_ASCII_CTRL_C   0x03

LOG_MODULE_REGISTER(shell_nfc, CONFIG_SHELL_NFC_LOG_LEVEL);

RING_BUF_DECLARE(shell_nfc_tx_buff, CONFIG_SHELL_NFC_BACKEND_TX_RING_BUFFER_SIZE);
RING_BUF_DECLARE(shell_nfc_rx_buff, CONFIG_SHELL_NFC_BACKEND_RX_RING_BUFFER_SIZE);

static K_MUTEX_DEFINE(nfc_data_mutex);
static K_SEM_DEFINE(shell_nfc_ready, 0, 1);

static const struct shell_transport_api shell_nfc_transport_api;

static struct shell_nfc {
	shell_transport_handler_t handler;
	void *context;
	struct ring_buf *tx_buf;
	struct ring_buf *rx_buf;
	size_t data_len;
	bool session_active;
	bool first_exchange;
	bool initialized;
} shell_nfc_transport = {
	.rx_buf = &shell_nfc_rx_buff,
	.tx_buf = &shell_nfc_tx_buff
};

static const struct shell_transport shell_transport_nfc = {
	.api = &shell_nfc_transport_api,
	.ctx = &shell_nfc_transport
};

SHELL_DEFINE(shell_nfc, "nfc:~$", &shell_transport_nfc, CONFIG_SHELL_NFC_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_NFC_LOG_MESSAGE_QUEUE_TIMEOUT, SHELL_FLAG_OLF_CRLF);

static void on_field_on_evt(struct shell_nfc *transport)
{
	transport->session_active = true;
	LOG_INF("NFC T4T shell transport session active");
}

static void on_data_transmitted_evt(struct shell_nfc *transport, size_t data_length)
{
	ARG_UNUSED(transport);

	LOG_DBG("NFC T4T data transmitted, length: %u", data_length);
}

static void on_field_off_evt(struct shell_nfc *transport)
{
	if (!transport->session_active) {
		return;
	}

	transport->first_exchange = true;

	int err = k_mutex_lock(&nfc_data_mutex, K_FOREVER);

	if (err) {
		LOG_ERR("Failed to lock mutex, (err: %d)", err);
		__ASSERT(false, "Cannot lock mutex");
	}

	transport->session_active = false;

	/* Reset Tx buffer here to avoid sending leftover data if transport interface will be
	 * restored.
	 */
	ring_buf_reset(transport->tx_buf);

	err = k_mutex_unlock(&nfc_data_mutex);
	if (err) {
		LOG_ERR("Failed to unlock mutex, (err: %d)", err);
		__ASSERT(false, "Cannot unlock mutex");
	}

	LOG_INF("NFC T4T transport session ended. Shell data will be discarded");
}

static void nfc_response_send(struct shell_nfc *transport, uint8_t header_token)
{
	static uint8_t tx_buffer[CONFIG_SHELL_NFC_BACKEND_TX_BUFFER];
	int err;
	size_t length = NFC_SHELL_HEADER_SIZE;

	tx_buffer[0] = header_token;

	if (!ring_buf_is_empty(transport->tx_buf)) {
		length += ring_buf_get(transport->tx_buf, (tx_buffer + NFC_SHELL_HEADER_SIZE),
			  (sizeof(tx_buffer) - NFC_SHELL_HEADER_SIZE));
	}

	__ASSERT_NO_MSG(length <= sizeof(tx_buffer));

	err = nfc_t4t_response_pdu_send(tx_buffer, length);
	if (err) {
		LOG_ERR("Failed to transmit NFC T4T response, (err: %d)", err);
		__ASSERT_NO_MSG(false);
	}

	transport->handler(SHELL_TRANSPORT_EVT_TX_RDY, transport->context);
}

static void transport_connection_process(struct shell_nfc *transport)
{
	/* Notify shell on initialization that transport is ready. */
	if (!transport->initialized) {
		transport->initialized = true;
		k_sem_give(&shell_nfc_ready);
	} else if (transport->first_exchange) {
		/* First data exchange after transport being disconnected. Shell can contain
		 * uncompleted command in the command buffer. Send Ctrl + C to the terminal to
		 * discard previously received partial command and print the shell prompt.
		 */
		static const uint8_t etx[] = {VT100_ASCII_CTRL_C};

		transport->data_len = 0;

		ring_buf_put(transport->rx_buf, etx, sizeof(etx));
		transport->handler(SHELL_TRANSPORT_EVT_RX_RDY, transport->context);

		transport->first_exchange = false;
	} else {
		/* Do nothing. */
	}
}

static void on_data_indication_evt(struct shell_nfc *transport,
				   const uint8_t *data,
				   size_t data_length, uint32_t flags)
{
	uint8_t header_token = 0;

	transport_connection_process(transport);

	/* The beginning of the payload, skip header. */
	if ((transport->data_len == 0) && (data_length >= NFC_SHELL_HEADER_SIZE)) {
		header_token = data[0];
		data += NFC_SHELL_HEADER_SIZE;
		data_length -= NFC_SHELL_HEADER_SIZE;
	}

	transport->data_len += data_length;

	/* Add new data to the buffer. Data in buffer does not contain payload header. */
	if (data_length > 0) {
		ring_buf_put(transport->rx_buf, data, data_length);
		LOG_DBG("NFC Rx data received, data length: %u", data_length);
	}

	if (flags != NFC_T4T_DI_FLAG_MORE) {
		if (transport->data_len > 0) {
			transport->handler(SHELL_TRANSPORT_EVT_RX_RDY, transport->context);

			LOG_DBG("NFC complete data payload received, data length: %u",
				transport->data_len);

			transport->data_len = 0;
		}

		nfc_response_send(transport, header_token);
	}
}

static void nfc_t4t_handler(void *context, nfc_t4t_event_t event, const uint8_t *data,
			    size_t data_length, uint32_t flags)
{
	__ASSERT((!k_is_in_isr()), "NFC T4T callback called from ISR context");

	struct shell_nfc *nfc_transport = context;

	switch (event) {
	case NFC_T4T_EVENT_FIELD_ON:
		on_field_on_evt(nfc_transport);
		break;

	case NFC_T4T_EVENT_FIELD_OFF:
		on_field_off_evt(nfc_transport);
		break;

	case NFC_T4T_EVENT_DATA_IND:
		on_data_indication_evt(nfc_transport, data, data_length, flags);
		break;

	case NFC_T4T_EVENT_DATA_TRANSMITTED:
		on_data_transmitted_evt(nfc_transport, data_length);
		break;

	default:
		__ASSERT(false, "Unknown event");
		break;
	}
}

static int init(const struct shell_transport *transport, const void *config,
		shell_transport_handler_t evt_handler, void *context)
{
	int err;
	struct shell_nfc *nfc_transport = transport->ctx;

	nfc_transport->session_active = false;
	nfc_transport->initialized = false;
	nfc_transport->first_exchange = false;
	nfc_transport->data_len = 0;
	nfc_transport->handler = evt_handler;
	nfc_transport->context = context;

	ring_buf_reset(nfc_transport->rx_buf);
	ring_buf_reset(nfc_transport->tx_buf);

	k_sem_reset(&shell_nfc_ready);

	err = nfc_t4t_setup(nfc_t4t_handler, nfc_transport);
	if (err) {
		LOG_ERR("Failed to setup NFC T4T library, (err: %d)", err);
		return err;
	}

	err = nfc_t4t_emulation_start();
	if (err) {
		LOG_ERR("Failed to start NFC T4T emulation, (err: %d)", err);
	}

	return err;
}

static int uninit(const struct shell_transport *transport)
{
	int err;

	err = nfc_t4t_emulation_stop();
	if (err) {
		LOG_ERR("Failed to stop NFC T4T emulation, (err: %d)", err);
		return err;
	}

	err = nfc_t4t_done();
	if (err) {
		LOG_ERR("Failed to un-initialize NFC T4T, (err: %d)", err);
	}

	return err;
}

static int enable(const struct shell_transport *transport, bool blocking_tx)
{
	ARG_UNUSED(transport);

	if (blocking_tx) {
		LOG_WRN("Blocking Tx is not supported");
		return -ENOTSUP;
	}

	LOG_DBG("Waiting for NFC T4T transport being enabled");
	k_sem_take(&shell_nfc_ready, K_FOREVER);

	return 0;
}

static int read(const struct shell_transport *transport, void *data, size_t length, size_t *cnt)
{
	struct shell_nfc *nfc_transport = transport->ctx;

	*cnt = ring_buf_get(nfc_transport->rx_buf, data, length);

	return 0;
}

static int write(const struct shell_transport *transport, const void *data, size_t length,
		 size_t *cnt)
{
	bool accepted = false;
	struct shell_nfc *nfc_transport = transport->ctx;

	/* The NFC transport can be disconnected asynchronously at any time. The NFC callback
	 * function informs about it and performs clean up. The Tx ring buffer need to be
	 * protected here because on transport disconnection its state need to be cleaned and data
	 * is discarded. We do not want to put new data into buffer after transport disconnection.
	 */
	int err = k_mutex_lock(&nfc_data_mutex, K_FOREVER);

	if (err) {
		LOG_ERR("Failed to lock mutex, (err: %d)", err);
		__ASSERT(false, "Cannot lock mutex");
	}

	if (nfc_transport->session_active) {
		*cnt = ring_buf_put(nfc_transport->tx_buf, data, length);
		accepted = true;
	} else {
		*cnt = length;
	}

	err = k_mutex_unlock(&nfc_data_mutex);
	if (err) {
		LOG_ERR("Failed to unlock mutex, (err: %d)", err);
		__ASSERT_NO_MSG(false);
	}

	if (accepted) {
		LOG_DBG("Write req: %u buffered:%u", length, *cnt);
	} else {
		LOG_INF("Write req: %u, transport session inactive, discarding: %u", length, *cnt);
	}

	return 0;
}

static const struct shell_transport_api shell_nfc_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.read = read,
	.write = write
};

int shell_nfc_init(void)
{
	bool log_backend = (CONFIG_SHELL_NFC_INIT_LOG_LEVEL > 0);
	uint32_t init_log_level = (CONFIG_SHELL_NFC_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		      CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_NFC_INIT_LOG_LEVEL;

	const struct shell_backend_config_flags cfg_flags = SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

	return shell_init(&shell_nfc, NULL, cfg_flags, log_backend, init_log_level);
}

SYS_INIT(shell_nfc_init, APPLICATION, CONFIG_SHELL_NFC_INIT_PRIORITY);

const struct shell *shell_backend_nfc_get_ptr(void)
{
	return &shell_nfc;
}
