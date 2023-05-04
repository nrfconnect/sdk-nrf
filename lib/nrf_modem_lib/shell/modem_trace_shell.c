/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <modem/nrf_modem_lib_trace.h>

#if defined(CONFIG_MEMFAULT)
#include "modem_trace_memfault.h"
#endif

static uint32_t trace_start_time_ms;
static uint32_t trace_duration_ms;

int modem_traces_start(const struct shell *sh, enum nrf_modem_lib_trace_level trace_level)
{
	int err;

	shell_print(sh, "Starting modem traces: %d", trace_level);

	err = nrf_modem_lib_trace_level_set(trace_level);
	if (err) {
		shell_error(sh, "Failed to enable modem traces");
		return err;
	}
	trace_start_time_ms = k_uptime_get_32();
	trace_duration_ms = 0;

	return 0;
}

int modem_traces_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int err;

	shell_print(sh, "Stopping modem traces");

	err = nrf_modem_lib_trace_level_set(NRF_MODEM_LIB_TRACE_LEVEL_OFF);
	if (err) {
		shell_error(sh, "Failed to turn off modem traces");
		return err;
	}

	trace_duration_ms = k_uptime_get_32() - trace_start_time_ms;
	trace_start_time_ms = 0;

	/* Changing the trace level to off will produce some traces, so sleep long enough to
	 * receive those as well.
	 */
	k_sleep(K_SECONDS(1));

	return 0;
}

void modem_traces_clear(const struct shell *sh, size_t argc, char **argv)
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

void modem_trace_size(const struct shell *sh, size_t argc, char **argv)
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

static int modem_trace_shell_start(const struct shell *sh, size_t argc, char **argv, void *data)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	enum nrf_modem_lib_trace_level level = (enum nrf_modem_lib_trace_level)data;

	return modem_traces_start(sh, level);
}

void send_traces_to_memfault(const struct shell *sh, size_t argc, char **argv, void *data)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	ARG_UNUSED(data);
#ifdef CONFIG_MEMFAULT
	uint32_t duration_ms;
	size_t size = nrf_modem_lib_trace_data_size();

	if (size == -ENOTSUP) {
		shell_error(sh, "The current modem trace backend does not support this operation");
		return;
	} else if (size < 0) {
		shell_error(sh, "Failed to get modem traces size: %d", size);
		return;
	} else if (size == 0) {
		shell_error(sh, "No data to send");
		return;
	}

	if (trace_duration_ms) {
		duration_ms = trace_duration_ms;
	} else if (trace_start_time_ms == 0) { /* modem tracing not started since reboot */
		duration_ms = 0;
	} else { /* modem tracing active */
		duration_ms = k_uptime_get_32() - trace_start_time_ms;
	}

	modem_trace_memfault_send(sh, size, duration_ms);
#else
	shell_error(sh, "CONFIG_MEMFAULT is not enabled. Please configure memfault first.");
#endif /* defined(CONFIG_MEMFAULT) */
}

SHELL_STATIC_SUBCMD_SET_CREATE(send_cmd,
	SHELL_CMD(memfault, NULL,
		"Send stored traces to Memfault as Custom Data Recording.",
		send_traces_to_memfault),
	SHELL_SUBCMD_SET_END);

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
		"Stop modem tracing.", modem_traces_stop),
	SHELL_CMD(clear, NULL,
		"Clear captured trace data and prepare the backend for capturing new traces.\n"
		"This operation is only supported with some trace backends.", modem_traces_clear),
	SHELL_CMD(size, NULL,
		"Read out the size of stored modem traces.\n"
		"This operation is only supported with some trace backends.", modem_trace_size),
	SHELL_CMD(send, &send_cmd,
		"Collection of subcommands for handling stored trace data.\n"
		"The stored traces are automatically deleted after send.", NULL),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(modem_trace, &modem_trace_cmd,
	"Commands for controlling modem trace functionality.", NULL);
