/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "slm_cmux.h"
#include "slm_at_host.h"
#include "slm_uart_handler.h"
#include <zephyr/logging/log.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <assert.h>

LOG_MODULE_REGISTER(slm_cmux, CONFIG_SLM_LOG_LEVEL);

/* This makes use of some of the Zephyr modem subsystem that has a CMUX module implementation.
 */

static struct {
	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[512];
	uint8_t uart_backend_transmit_buf[512];

	/* CMUX */
	struct modem_cmux instance;
	uint8_t cmux_receive_buf[128];
	uint8_t cmux_transmit_buf[256];

	/* CMUX channels (Data Link Connection Identifier); index = address - 1 */
	struct cmux_dlci {
		struct modem_cmux_dlci instance;
		struct modem_pipe *pipe;
		uint8_t *recv_buf;
		uint16_t recv_buf_size;
		uint8_t address;
	} dlcis[2];
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
		static uint8_t recv_buf[256];
		const int recv_len = modem_pipe_receive(pipe, recv_buf, sizeof(recv_buf));

		if (recv_len > 0) {
			LOG_DBG("Received %u bytes on DLCI %u.", recv_len, dlci->address);
			modem_pipe_transmit(pipe, recv_buf, recv_len);
		} else {
			LOG_ERR("Failed to receive data from DLCI %u (%d).",
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

static void init_dlci(size_t address, uint16_t recv_buf_size,
		      uint8_t recv_buf[static recv_buf_size])
{
	assert(ARRAY_SIZE(cmux.dlcis) >= address);

	struct cmux_dlci *const dlci = &cmux.dlcis[address - 1];
	const struct modem_cmux_dlci_config dlci_config = {
		.dlci_address = address,
		.receive_buf = recv_buf,
		.receive_buf_size = recv_buf_size
	};

	dlci->pipe = modem_cmux_dlci_init(&cmux.instance, &dlci->instance, &dlci_config);
	dlci->recv_buf = recv_buf;
	dlci->recv_buf_size = recv_buf_size;
	dlci->address = address;

	modem_pipe_attach(dlci->pipe, dlci_pipe_event_handler, dlci);
}

static int cmux_start(void)
{
	int ret;

	/* Disable SLM UART so that CMUX can take over it. */
	ret = slm_uart_rx_disable();
	if (ret) {
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
		static uint8_t dlci1_recv_buf[128];
		static uint8_t dlci2_recv_buf[256];

		init_dlci(1, sizeof(dlci1_recv_buf), dlci1_recv_buf);
		init_dlci(2, sizeof(dlci2_recv_buf), dlci2_recv_buf);
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
	const int ret = cmux_start();

	if (ret) {
		LOG_ERR("Failed to start CMUX (%d).", ret);
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
	}

	k_work_init_delayable(&cmux_start_work, cmux_starter);
	ret = k_work_schedule(&cmux_start_work, K_MSEC(100));
	if (ret == 1) {
		ret = 0;
	} else if (ret == 0) {
		ret = -EAGAIN;
	}
	return ret;
}
