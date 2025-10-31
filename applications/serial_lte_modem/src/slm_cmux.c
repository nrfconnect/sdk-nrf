/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "slm_cmux.h"
#include "slm_at_host.h"
#if defined(CONFIG_SLM_PPP)
#include "slm_ppp.h"
#endif
#include "slm_util.h"
#include <zephyr/logging/log.h>
#include <zephyr/modem/backend/uart_slm.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <assert.h>

/* This makes use of part of the Zephyr modem subsystem which has a CMUX module. */
LOG_MODULE_REGISTER(slm_cmux, CONFIG_SLM_LOG_LEVEL);

#define CHANNEL_COUNT (1 + CMUX_EXT_CHANNEL_COUNT)

#define RECV_BUF_LEN SLM_AT_MAX_CMD_LEN
/* The CMUX module reserves some spare buffer bytes. To achieve a maximum
 * response length of SLM_AT_MAX_RSP_LEN (comprising the "OK" or "ERROR"
 * that is sent separately), the transmit buffer must be made a bit bigger.
 * 49 extra bytes was manually found to allow SLM_AT_MAX_RSP_LEN long responses.
 */
#define TRANSMIT_BUF_LEN (49 + SLM_AT_MAX_RSP_LEN)

#define DLCI_TO_INDEX(dlci) ((dlci) - 1)
#define INDEX_TO_DLCI(index) ((index) + 1)

/* Workaround before upmerge */
#ifndef MODEM_CMUX_WORK_BUFFER_SIZE
#define MODEM_CMUX_WORK_BUFFER_SIZE CONFIG_MODEM_CMUX_WORK_BUFFER_SIZE
#endif

static struct {
	/* UART backend */
	struct modem_pipe *uart_pipe;
	bool uart_pipe_open;
	struct modem_backend_uart_slm uart_backend;
	uint8_t uart_backend_receive_buf[CONFIG_SLM_CMUX_UART_BUFFER_SIZE]
		__aligned(sizeof(void *));
	uint8_t uart_backend_transmit_buf[CONFIG_SLM_CMUX_UART_BUFFER_SIZE];

	/* CMUX */
	struct modem_cmux instance;
	uint8_t cmux_receive_buf[MODEM_CMUX_WORK_BUFFER_SIZE];
	uint8_t cmux_transmit_buf[MODEM_CMUX_WORK_BUFFER_SIZE];

	/* CMUX channels (Data Link Connection Identifier); index = address - 1 */
	struct cmux_dlci {
		struct modem_cmux_dlci instance;
		struct modem_pipe *pipe;
		uint8_t address;
		uint8_t receive_buf[RECV_BUF_LEN];
	} dlcis[CHANNEL_COUNT];
	/* Index of the DLCI used for AT communication; defaults to 0. */
	unsigned int at_channel;
	unsigned int requested_at_channel;

	/* Incoming data for DLCI's. */
	atomic_t dlci_channel_rx;
	struct k_work rx_work;

	/* Outgoing data for AT DLCI. */
	struct ring_buf tx_rb;
	uint8_t tx_buffer[CONFIG_SLM_CMUX_TX_BUFFER_SIZE];
	struct k_mutex tx_rb_mutex;
	struct k_work tx_work;

} cmux;

static void rx_work_fn(struct k_work *work)
{
	static uint8_t recv_buf[RECV_BUF_LEN];
	int ret;
	bool is_at;

	for (int i = 0; i < ARRAY_SIZE(cmux.dlcis); ++i) {
		if (atomic_test_and_clear_bit(&cmux.dlci_channel_rx, i)) {
			/* Incoming data for DLCI. */
			is_at = (i == cmux.at_channel);

			ret = modem_pipe_receive(cmux.dlcis[i].pipe, recv_buf, sizeof(recv_buf));
			if (ret < 0) {
				LOG_ERR("DLCI %u%s failed modem_pipe_receive. (%d)",
					INDEX_TO_DLCI(i), is_at ? " (AT)" : "", ret);
				continue;
			}

			if (!is_at) {
				LOG_INF("DLCI %u discarding %u bytes of data.", INDEX_TO_DLCI(i),
					ret);
				continue;
			}

			LOG_DBG("DLCI %u (AT) received %u bytes of data.", INDEX_TO_DLCI(i), ret);
			slm_at_receive(recv_buf, ret);
		}
	}
}

static void dlci_pipe_event_handler(struct modem_pipe *pipe,
				    enum modem_pipe_event event, void *user_data)
{
	const struct cmux_dlci *const dlci = user_data;
	bool is_at = (ARRAY_INDEX(cmux.dlcis, dlci) == cmux.at_channel);

	switch (event) {
		/* The events of the DLCIs other than that of the AT channel
		 * are received here when they haven't been attached to
		 * by their respective implementations.
		 */
	case MODEM_PIPE_EVENT_OPENED:
		LOG_INF("DLCI %u%s opened.", dlci->address, is_at ? " (AT)" : "");
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		LOG_INF("DLCI %u%s closed.", dlci->address, is_at ? " (AT)" : "");
		break;

	case MODEM_PIPE_EVENT_RECEIVE_READY:
		LOG_DBG("DLCI %u%s receive ready.", dlci->address, is_at ? " (AT)" : "");
		atomic_or(&cmux.dlci_channel_rx, BIT(DLCI_TO_INDEX(dlci->address)));
		k_work_submit_to_queue(&slm_work_q, &cmux.rx_work);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		if (is_at &&
		    cmux.dlcis[cmux.at_channel].instance.state == MODEM_CMUX_DLCI_STATE_OPEN &&
		    !ring_buf_is_empty(&cmux.tx_rb)) {
			k_work_submit_to_queue(&slm_work_q, &cmux.tx_work);
		}
		break;
	}
}

static void cmux_event_handler(struct modem_cmux *, enum modem_cmux_event event, void *)
{
	if (event == MODEM_CMUX_EVENT_CONNECTED || event == MODEM_CMUX_EVENT_DISCONNECTED) {
		LOG_INF("CMUX %sconnected.", (event == MODEM_CMUX_EVENT_CONNECTED) ? "" : "dis");
	}
}

static void init_dlci(size_t dlci_idx, uint16_t recv_buf_size,
		      uint8_t recv_buf[static recv_buf_size])
{
	assert(ARRAY_SIZE(cmux.dlcis) > dlci_idx);

	struct cmux_dlci *const dlci = &cmux.dlcis[dlci_idx];
	const struct modem_cmux_dlci_config dlci_config = {
		.dlci_address = dlci_idx + 1,
		.receive_buf = recv_buf,
		.receive_buf_size = recv_buf_size
	};

	dlci->pipe = modem_cmux_dlci_init(&cmux.instance, &dlci->instance, &dlci_config);
	dlci->address = dlci_config.dlci_address;

	modem_pipe_attach(dlci->pipe, dlci_pipe_event_handler, dlci);
}

static size_t cmux_write(struct modem_pipe *pipe, const uint8_t *data, size_t len)
{
	size_t sent_len = 0;
	int ret = 0;

	while (sent_len < len) {
		/* Push data to CMUX TX buffer.  */
		ret = modem_pipe_transmit(pipe, data, len - sent_len);
		if (ret <= 0) {
			break;
		}
		sent_len += ret;
		data += ret;
	}

	if (ret < 0) {
		LOG_DBG("DLCI %u (AT). Sent %u out of %u bytes. (%d)",
			INDEX_TO_DLCI(cmux.at_channel), sent_len, len, ret);
	}

	return sent_len;
}

static void tx_work_fn(struct k_work *work)
{
	uint8_t *data;
	size_t len;

	k_mutex_lock(&cmux.tx_rb_mutex, K_FOREVER);

	do {
		len = ring_buf_get_claim(&cmux.tx_rb, &data, ring_buf_capacity_get(&cmux.tx_rb));
		len = cmux_write(cmux.dlcis[cmux.at_channel].pipe, data, len);
		ring_buf_get_finish(&cmux.tx_rb, len);

	} while (!ring_buf_is_empty(&cmux.tx_rb) && len != 0);

	k_mutex_unlock(&cmux.tx_rb_mutex);

	if (!ring_buf_is_empty(&cmux.tx_rb)) {
		LOG_DBG("Remaining bytes in TX buffer: %u.", ring_buf_size_get(&cmux.tx_rb));
	}

	if (cmux.requested_at_channel != UINT_MAX) {
		cmux.at_channel = cmux.requested_at_channel;
		cmux.requested_at_channel = UINT_MAX;
		LOG_INF("DLCI %u (AT) updated.", INDEX_TO_DLCI(cmux.at_channel));
	}
}

static int cmux_write_at_channel_nonblock(const uint8_t *data, size_t len)
{
	int ret = 0;

	k_mutex_lock(&cmux.tx_rb_mutex, K_FOREVER);

	if (ring_buf_space_get(&cmux.tx_rb) >= len) {
		ring_buf_put(&cmux.tx_rb, data, len);
	} else {
		LOG_WRN("TX buf overflow, dropping %u bytes.", len);
		ret = -ENOBUFS;
	}

	k_mutex_unlock(&cmux.tx_rb_mutex);

	return ret;
}

static int cmux_write_at_channel_block(const uint8_t *data, size_t len)
{
	size_t sent = 0;
	size_t ret;
	uint8_t *buf;

	k_mutex_lock(&cmux.tx_rb_mutex, K_FOREVER);

	while (sent < len) {
		ret = ring_buf_put(&cmux.tx_rb, data + sent, len - sent);
		sent += ret;
		if (!ret) {
			/* Buffer full, send partial data. */
			ret = ring_buf_get_claim(&cmux.tx_rb, &buf,
						 ring_buf_capacity_get(&cmux.tx_rb));
			ret = cmux_write(cmux.dlcis[cmux.at_channel].pipe, buf, ret);
			ring_buf_get_finish(&cmux.tx_rb, ret);

			if (ret == 0) {
				/* Cannot send and buffers are full.
				 * Data will be dropped.
				 */
				break;
			}
		}
	}

	k_mutex_unlock(&cmux.tx_rb_mutex);

	if (sent < len) {
		LOG_WRN("TX buf overflow, dropping %u bytes.", len - sent);
		return -ENOBUFS;
	}

	return 0;
}

static int cmux_write_at_channel(const uint8_t *data, size_t len)
{
	size_t ret;

	/* CMUX work queue needs to be able to run.
	 * So, we will send only from SLM work queue.
	 */
	if (k_current_get() != &slm_work_q.thread) {
		ret = cmux_write_at_channel_nonblock(data, len);
	} else {
		ret = cmux_write_at_channel_block(data, len);
	}

	k_work_submit_to_queue(&slm_work_q, &cmux.tx_work);

	return ret;
}

static void close_pipe(struct modem_pipe **pipe)
{
	if (*pipe) {
		modem_pipe_close_async(*pipe);
		modem_pipe_release(*pipe);
		*pipe = NULL;
	}
}

static int cmux_stop(void)
{
	if (cmux.uart_pipe && cmux.uart_pipe_open) {
		/* CMUX is running. Just close the UART pipe to pause and be able to resume.
		 * This allows AT data to be cached by the CMUX module while the UART is off.
		 */
		modem_pipe_close_async(cmux.uart_pipe);
		cmux.uart_pipe_open = false;
		return 0;
	}

	modem_cmux_release(&cmux.instance);

	close_pipe(&cmux.uart_pipe);
	cmux.uart_pipe_open = false;

	for (size_t i = 0; i != ARRAY_SIZE(cmux.dlcis); ++i) {
		close_pipe(&cmux.dlcis[i].pipe);
	}

	return 0;
}

static bool cmux_is_started(void)
{
	return (cmux.uart_pipe != NULL);
}

void slm_cmux_init(void)
{
	const struct modem_cmux_config cmux_config = {
		.callback = cmux_event_handler,
		.receive_buf = cmux.cmux_receive_buf,
		.receive_buf_size = sizeof(cmux.cmux_receive_buf),
		.transmit_buf = cmux.cmux_transmit_buf,
		.transmit_buf_size = sizeof(cmux.cmux_transmit_buf),
	};

	modem_cmux_init(&cmux.instance, &cmux_config);

	for (size_t i = 0; i != ARRAY_SIZE(cmux.dlcis); ++i) {
		init_dlci(i, sizeof(cmux.dlcis[i].receive_buf), cmux.dlcis[i].receive_buf);
	}

	cmux.dlci_channel_rx = ATOMIC_INIT(0);
	k_work_init(&cmux.rx_work, rx_work_fn);

	ring_buf_init(&cmux.tx_rb, sizeof(cmux.tx_buffer), cmux.tx_buffer);
	k_mutex_init(&cmux.tx_rb_mutex);
	k_work_init(&cmux.tx_work, tx_work_fn);

	cmux.requested_at_channel = UINT_MAX;
}

static struct cmux_dlci *cmux_get_dlci(enum cmux_channel channel)
{
#if defined(CONFIG_SLM_PPP)
	if (channel == CMUX_PPP_CHANNEL) {
		/* The first DLCI that is not the AT channel's is PPP's. */
		return &cmux.dlcis[!cmux.at_channel];
	}
#endif
#if defined(CONFIG_SLM_GNSS_OUTPUT_NMEA_ON_CMUX_CHANNEL)
	if (channel == CMUX_GNSS_CHANNEL) {
		/* The last DLCI. */
		return &cmux.dlcis[CHANNEL_COUNT - 1];
	}
#endif
	assert(false);
}

struct modem_pipe *slm_cmux_reserve(enum cmux_channel channel)
{
	/* Return the channel's pipe. The requesting module may attach to it,
	 * after which this pipe's events and data won't be received here anymore
	 * until the channel is released (below) and we attach back to the pipe.
	 */
	return cmux_get_dlci(channel)->pipe;
}

void slm_cmux_release(enum cmux_channel channel, bool fallback)
{
	struct cmux_dlci *dlci = cmux_get_dlci(channel);

#if defined(CONFIG_SLM_CMUX_AUTOMATIC_FALLBACK_ON_PPP_STOPPAGE)
	if (channel == CMUX_PPP_CHANNEL && fallback) {
		cmux.at_channel = 0;
	}
#endif
	modem_pipe_attach(dlci->pipe, dlci_pipe_event_handler, dlci);
}

static int cmux_start(void)
{
	int ret;

	if (cmux_is_started()) {
		ret = modem_pipe_open(cmux.uart_pipe, K_SECONDS(CONFIG_SLM_MODEM_PIPE_TIMEOUT));
		if (!ret) {
			cmux.uart_pipe_open = true;
			LOG_INF("CMUX resumed.");
		}
		return ret;
	}

	{
		const struct modem_backend_uart_slm_config uart_backend_config = {
			.uart = DEVICE_DT_GET(DT_CHOSEN(ncs_slm_uart)),
			.receive_buf = cmux.uart_backend_receive_buf,
			.receive_buf_size = sizeof(cmux.uart_backend_receive_buf),
			.transmit_buf = cmux.uart_backend_transmit_buf,
			.transmit_buf_size = sizeof(cmux.uart_backend_transmit_buf),
		};

		cmux.uart_pipe =
			modem_backend_uart_slm_init(&cmux.uart_backend, &uart_backend_config);
		if (!cmux.uart_pipe) {
			return -ENODEV;
		}
	}

	ret = modem_cmux_attach(&cmux.instance, cmux.uart_pipe);
	if (ret) {
		return ret;
	}

	ret = modem_pipe_open(cmux.uart_pipe, K_SECONDS(CONFIG_SLM_MODEM_PIPE_TIMEOUT));
	if (!ret) {
		cmux.uart_pipe_open = true;
	}
	return ret;
}

static void cmux_starter(struct k_work *)
{
	const int ret = slm_at_set_backend((struct slm_at_backend) {
		.start = cmux_start,
		.send = cmux_write_at_channel,
		.stop = cmux_stop
	});

	if (ret) {
		LOG_ERR("Failed to start CMUX. (%d)", ret);
	} else {
		LOG_INF("CMUX started.");
	}
}

SLM_AT_CMD_CUSTOM(xcmux, "AT#XCMUX", handle_at_cmux);
static int handle_at_cmux(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			  uint32_t param_count)
{
	static struct k_work_delayable cmux_start_work;
	unsigned int at_dlci;
	int ret;

	if (cmd_type == AT_PARSER_CMD_TYPE_READ) {
		rsp_send("\r\n#XCMUX: %u,%u\r\n", cmux.at_channel + 1, CHANNEL_COUNT);
		return 0;
	}
	if (cmd_type != AT_PARSER_CMD_TYPE_SET || param_count > 2) {
		return -EINVAL;
	}

	if (param_count == 1 && cmux_is_started()) {
		return -EALREADY;
	}

	if (param_count == 2) {
		ret = at_parser_num_get(parser, 1, &at_dlci);
		if (ret || (at_dlci != 1 && (!IS_ENABLED(CONFIG_SLM_PPP) || at_dlci != 2))) {
			return -EINVAL;
		}
		const unsigned int at_channel = DLCI_TO_INDEX(at_dlci);

#if defined(CONFIG_SLM_PPP)
		if (!slm_ppp_is_stopped() && at_channel != cmux.at_channel) {
			/* The AT channel cannot be changed when PPP has a channel reserved. */
			return -ENOTSUP;
		}
#endif
		if (cmux_is_started()) {
			/* Update the AT channel after answering "OK" on the current DLCI. */
			rsp_send_ok();
			cmux.requested_at_channel = at_channel;
			return -SILENT_AT_COMMAND_RET;
		}
		cmux.at_channel = at_channel;
	}

	k_work_init_delayable(&cmux_start_work, cmux_starter);
	ret = k_work_schedule(&cmux_start_work, SLM_UART_RESPONSE_DELAY);
	if (ret == 1) {
		ret = 0;
	} else if (ret == 0) {
		ret = -EAGAIN;
	}
	return ret;
}
