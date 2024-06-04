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
#include <zephyr/logging/log.h>
#include <zephyr/modem/backend/uart.h>
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

static struct {
	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[RECV_BUF_LEN];
	uint8_t uart_backend_transmit_buf[TRANSMIT_BUF_LEN];

	/* CMUX */
	struct modem_cmux instance;
	uint8_t cmux_receive_buf[RECV_BUF_LEN];
	uint8_t cmux_transmit_buf[TRANSMIT_BUF_LEN];

	/* CMUX channels (Data Link Connection Identifier); index = address - 1 */
	struct cmux_dlci {
		struct modem_cmux_dlci instance;
		struct modem_pipe *pipe;
		uint8_t address;
		uint8_t receive_buf[RECV_BUF_LEN];
	} dlcis[CHANNEL_COUNT];
	/* Index of the DLCI used for AT communication; defaults to 0. */
	unsigned int at_channel;
} cmux;

static void dlci_pipe_event_handler(struct modem_pipe *pipe,
				    enum modem_pipe_event event, void *user_data)
{
	const struct cmux_dlci *const dlci = user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
	case MODEM_PIPE_EVENT_CLOSED:
		/* The events of the DLCIs other than that of the AT channel
		 * are received here when they haven't been attached to
		 * by their respective implementations.
		 */
		LOG_INF("DLCI %u %s.", dlci->address,
			(event == MODEM_PIPE_EVENT_OPENED) ? "opened" : "closed");
		break;
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		static uint8_t recv_buf[RECV_BUF_LEN];
		const int recv_len = modem_pipe_receive(pipe, recv_buf, sizeof(recv_buf));

		if (recv_len > 0) {
			if (ARRAY_INDEX(cmux.dlcis, dlci) == cmux.at_channel) {
				slm_at_receive(recv_buf, recv_len);
			} else {
				LOG_WRN("Discarding %u byte%s received on non-AT DLCI %u.",
					recv_len, (recv_len > 1) ? "s" : "", dlci->address);
			}
		} else {
			LOG_ERR("Failed to receive data on DLCI %u. (%d)",
				dlci->address, recv_len);
		}
		break;
	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
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

static int cmux_write_at_channel(const uint8_t *data, size_t len)
{
	int ret = modem_pipe_transmit(cmux.dlcis[cmux.at_channel].pipe, data, len);

	if (ret != len) {
		const int sent_len = MAX(0, ret);

		if (ret >= 0) {
			ret = 1;
		}
		LOG_ERR("Sent %u out of %u AT data bytes. (%d)", sent_len, len, ret);
		return ret;
	}
	return 0;
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
	if (cmux.uart_pipe && cmux.uart_pipe->state == MODEM_PIPE_STATE_OPEN) {
		/* CMUX is running. Just close the UART pipe to pause and be able to resume.
		 * This allows AT data to be cached by the CMUX module while the UART is off.
		 */
		modem_pipe_close_async(cmux.uart_pipe);
		return 0;
	}

	modem_cmux_release(&cmux.instance);

	close_pipe(&cmux.uart_pipe);

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

void slm_cmux_release(enum cmux_channel channel)
{
	struct cmux_dlci *dlci = cmux_get_dlci(channel);

#if defined(SLM_CMUX_AUTOMATIC_FALLBACK_ON_PPP_STOPPAGE)
	if (channel == CMUX_PPP_CHANNEL) {
		cmux.at_channel = 0;
	}
#endif
	modem_pipe_attach(dlci->pipe, dlci_pipe_event_handler, dlci);
}

static int cmux_start(void)
{
	int ret;

	if (cmux_is_started()) {
		ret = modem_pipe_open(cmux.uart_pipe);
		if (!ret) {
			LOG_INF("CMUX resumed.");
		}
		return ret;
	}

	{
		const struct modem_backend_uart_config uart_backend_config = {
			.uart = DEVICE_DT_GET(DT_CHOSEN(ncs_slm_uart)),
			.receive_buf = cmux.uart_backend_receive_buf,
			.receive_buf_size = sizeof(cmux.uart_backend_receive_buf),
			.transmit_buf = cmux.uart_backend_transmit_buf,
			.transmit_buf_size = sizeof(cmux.uart_backend_transmit_buf),
		};

		cmux.uart_pipe = modem_backend_uart_init(&cmux.uart_backend, &uart_backend_config);
		if (!cmux.uart_pipe) {
			return -ENODEV;
		}
	}

	ret = modem_cmux_attach(&cmux.instance, cmux.uart_pipe);
	if (ret) {
		return ret;
	}

	ret = modem_pipe_open(cmux.uart_pipe);
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
static int handle_at_cmux(enum at_cmd_type cmd_type, const struct at_param_list *param_list,
			  uint32_t param_count)
{
	static struct k_work_delayable cmux_start_work;
	unsigned int at_dlci;
	int ret;

	if (cmd_type == AT_CMD_TYPE_READ_COMMAND) {
		rsp_send("\r\n#XCMUX: %u,%u\r\n", cmux.at_channel + 1, CHANNEL_COUNT);
		return 0;
	}
	if (cmd_type != AT_CMD_TYPE_SET_COMMAND || param_count > 2) {
		return -EINVAL;
	}

	if (param_count == 1 && cmux_is_started()) {
		return -EALREADY;
	}

	if (param_count == 2) {
		ret = at_params_unsigned_int_get(param_list, 1, &at_dlci);
		if (ret || (at_dlci != 1 && (!IS_ENABLED(CONFIG_SLM_PPP) || at_dlci != 2))) {
			return -EINVAL;
		}
		const unsigned int at_channel = at_dlci - 1;

#if defined(CONFIG_SLM_PPP)
		if (slm_ppp_is_running() && at_channel != cmux.at_channel) {
			/* The AT channel cannot be changed when PPP is running. */
			return -ENOTSUP;
		}
#endif
		if (cmux_is_started()) {
			/* Just update the AT channel, first answering "OK" on the current DLCI. */
			rsp_send_ok();
			cmux.at_channel = at_channel;
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
