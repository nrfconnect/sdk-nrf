/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>

#include <openthread/cli.h>

static bool is_initialized;

static int ot_console_cb(void *context, const char *format, va_list arg)
{
	const struct shell *sh = context;

	shell_vfprintf(sh, SHELL_NORMAL, format, arg);

	return 0;
}

static int ot_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	char buffer[128];
	char *ptr = buffer;
	char *end = buffer + sizeof(buffer);

	if (!is_initialized) {
		otCliInit(NULL, ot_console_cb, (void *)sh);
		is_initialized = true;
	}

	for (size_t i = 1; i < argc; i++) {
		int arg_len = snprintk(ptr, end - ptr, "%s", argv[i]);

		if (arg_len < 0) {
			return arg_len;
		}

		ptr += arg_len;

		if (ptr >= end) {
			shell_fprintf(sh, SHELL_WARNING, "OT shell buffer full\n");
			return -ENOEXEC;
		}

		if (i + 1 < argc) {
			*ptr++ = ' ';
		}
	}

	*ptr = '\0';
	otCliInputLine(buffer);

	return 0;
}

SHELL_CMD_ARG_REGISTER(ot, NULL,
		       "OpenThread subcommands\n"
		       "Use \"ot help\" to get the list of subcommands",
		       ot_cmd, 2, 255);
