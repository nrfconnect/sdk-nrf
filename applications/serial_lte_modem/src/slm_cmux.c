/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "slm_cmux.h"
#include "slm_at_host.h"
#include <zephyr/logging/log.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <assert.h>

/* This makes use of part of the Zephyr modem subsystem which has a CMUX module. */
LOG_MODULE_REGISTER(slm_cmux, CONFIG_SLM_LOG_LEVEL);

enum cmux_channel {
	AT_CHANNEL,
	CHANNEL_COUNT
};

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
} cmux;

static void dlci_pipe_event_handler(struct modem_pipe *pipe,
				    enum modem_pipe_event event, void *user_data)
{
	const struct cmux_dlci *const dlci = user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
	case MODEM_PIPE_EVENT_CLOSED:
		LOG_INF("DLCI %u %s.", dlci->address,
			(event == MODEM_PIPE_EVENT_OPENED) ? "opened" : "closed");
		break;
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		static uint8_t recv_buf[RECV_BUF_LEN];
		const int recv_len = modem_pipe_receive(pipe, recv_buf, sizeof(recv_buf));

		if (recv_len > 0) {
			if (ARRAY_INDEX(cmux.dlcis, dlci) == AT_CHANNEL) {
				slm_at_receive(recv_buf, recv_len);
			}
		} else {
			LOG_ERR("Failed to receive data from DLCI %u. (%d)",
				dlci->address, recv_len);
		}
		break;
	}
}

static void cmux_event_handler(struct modem_cmux *, enum modem_cmux_event event, void *)
{
	assert(event == MODEM_CMUX_EVENT_CONNECTED || event == MODEM_CMUX_EVENT_DISCONNECTED);
	LOG_INF("CMUX %sconnected.", (event == MODEM_CMUX_EVENT_CONNECTED) ? "" : "dis");
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
	struct modem_pipe *const at_dlci_pipe = cmux.dlcis[AT_CHANNEL].pipe;
	int ret = modem_pipe_transmit(at_dlci_pipe, data, len);

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

static bool cmux_is_initialized(void)
{
	return (cmux.uart_pipe != NULL);
}

static int cmux_start(void)
{
	int ret;

	if (cmux_is_initialized()) {
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
	{
		const struct modem_cmux_config cmux_config = {
			.callback = cmux_event_handler,
			.receive_buf = cmux.cmux_receive_buf,
			.receive_buf_size = sizeof(cmux.cmux_receive_buf),
			.transmit_buf = cmux.cmux_transmit_buf,
			.transmit_buf_size = sizeof(cmux.cmux_transmit_buf),
		};

		modem_cmux_init(&cmux.instance, &cmux_config);
	}
	{
		for (size_t i = 0; i != ARRAY_SIZE(cmux.dlcis); ++i) {
			init_dlci(i, sizeof(cmux.dlcis[i].receive_buf), cmux.dlcis[i].receive_buf);
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

/* Handles AT#CMUX commands. */
int handle_at_cmux(enum at_cmd_type cmd_type)
{
	static struct k_work_delayable cmux_start_work;
	int ret;
	unsigned int op;
	enum {
		OP_START,
	};

	if (cmd_type != AT_CMD_TYPE_SET_COMMAND
	 || at_params_valid_count_get(&slm_at_param_list) != 2) {
		return -EINVAL;
	}

	ret = at_params_unsigned_int_get(&slm_at_param_list, 1, &op);
	if (ret) {
		return ret;
	} else if (op != OP_START) {
		return -EINVAL;
	} else if (cmux_is_initialized()) {
		return -EALREADY;
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
