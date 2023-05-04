/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <modem/nrf_modem_lib_trace.h>

#ifdef CONFIG_NRF_MODEM_LIB_SHELL_TRACE_UART

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#define UART_DEVICE_NODE DT_CHOSEN(nordic_modem_trace_uart)
#define READ_BUF_SIZE 256

#endif /* CONFIG_NRF_MODEM_LIB_SHELL_TRACE_UART */

static int modem_trace_start(const struct shell *sh, enum nrf_modem_lib_trace_level trace_level)
{
	int err;

	shell_print(sh, "Starting modem traces with trace level: %d", trace_level);

	err = nrf_modem_lib_trace_level_set(trace_level);
	if (err) {
		shell_error(sh, "Failed to enable modem traces, error: %d", err);
		return err;
	}

	return 0;
}

static int modem_trace_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int err;

	err = nrf_modem_lib_trace_level_set(NRF_MODEM_LIB_TRACE_LEVEL_OFF);
	if (err) {
		shell_error(sh, "Failed to turn off modem traces, error: %d", err);
		return err;
	}

	shell_print(sh, "Modem trace stop command issued. This may produce a few more traces. "
			"Please wait at least 1 second before attempting to read out trace data.");

	return 0;
}

static void modem_trace_clear(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int err;

	shell_print(sh, "Clearing modem traces");

	err = nrf_modem_lib_trace_clear();
	if (err == -ENOTSUP) {
		shell_error(sh, "The current modem trace backend does not support this operation");
	} else if (err) {
		shell_error(sh, "Failed to clear modem traces: %d", err);
	}
}

static void modem_trace_size(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret;

	ret = nrf_modem_lib_trace_data_size();
	if (ret == -ENOTSUP) {
		shell_error(sh, "The current modem trace backend does not support this operation");
	} else if (ret < 0) {
		shell_error(sh, "Failed to get modem traces size: %d", ret);
	} else {
		shell_print(sh, "Modem trace data size: %d bytes", ret);
	}
}

#ifdef CONFIG_NRF_MODEM_LIB_SHELL_TRACE_UART

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static void print_uart(const struct shell *sh, char *buf, int len)
{
	if (!device_is_ready(uart_dev)) {
		shell_error(sh, "uart device not found/ready!");
		return;
	}

	for (int i = 0; i < len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

void modem_trace_uart_send(const struct shell *sh, size_t size)
{
	static uint8_t read_buf[READ_BUF_SIZE];
	int ret = 1;
	size_t read_offset = 0;

	shell_print(sh, "Reading out %d bytes of trace data", size);
	/* Read out the trace data from flash */
	while (ret > 0) {
		ret = nrf_modem_lib_trace_read(read_buf, READ_BUF_SIZE);
		if (ret == 0 || ret == -ENODATA) {
			break;
		} else if (ret < 0) {
			shell_error(sh, "Error reading modem traces: %d", ret);
			break;
		}
		read_offset += ret;
		print_uart(sh, read_buf, ret);
	}
	shell_print(sh, "Total trace bytes read from flash: %d", read_offset);
}

#endif /* CONFIG_NRF_MODEM_LIB_SHELL_TRACE_UART */

static void modem_trace_dump_uart(const struct shell *sh, size_t argc, char **argv, void *data)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	ARG_UNUSED(data);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_SHELL_TRACE_UART)) {
		size_t size = nrf_modem_lib_trace_data_size();

		if (size == -ENOTSUP) {
			shell_error(sh,
				"The current modem trace backend does not support this operation");
			return;
		} else if (size < 0) {
			shell_error(sh, "Failed to get modem traces size: %d", size);
			return;
		} else if (size == 0) {
			shell_error(sh, "No data to send");
			return;
		}

		modem_trace_uart_send(sh, size);
	} else {
		shell_error(sh, "Missing chosen node for nordic,modem-trace-uart. "
			"Please configure which uart to use.");
	}
}

static int modem_trace_shell_start(const struct shell *sh, size_t argc, char **argv, void *data)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	enum nrf_modem_lib_trace_level level = (enum nrf_modem_lib_trace_level)data;

	return modem_trace_start(sh, level);
}

SHELL_SUBCMD_DICT_SET_CREATE(start_cmd, modem_trace_shell_start,
	(full, 2, "Full"),
	(coredump_only, 1, "Coredump only"),
	(ip_only, 4, "IP only"),
	(lte_and_ip, 5, "LTE and IP")
);

SHELL_STATIC_SUBCMD_SET_CREATE(modem_trace_cmd,
	SHELL_CMD(start, &start_cmd,
		"Start modem tracing.", NULL),
	SHELL_CMD(stop, NULL,
		"Stop modem tracing.", modem_trace_stop),
	SHELL_CMD(clear, NULL,
		"Clear captured trace data and prepare the backend for capturing new traces.\n"
		"This operation is only supported with some trace backends.", modem_trace_clear),
	SHELL_CMD(size, NULL,
		"Read out the size of stored modem traces.\n"
		"This operation is only supported with some trace backends.", modem_trace_size),
	SHELL_CMD(dump_uart, NULL,
		"Dump stored traces to UART.", modem_trace_dump_uart),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(modem_trace, &modem_trace_cmd,
	"Commands for controlling modem trace functionality.", NULL);
