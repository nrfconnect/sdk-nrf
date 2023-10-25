/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/shell/shell.h>
#include <modem/at_monitor.h>
#include <nrf_modem_at.h>

#include "mosh_defines.h"
#include "mosh_print.h"

#if defined(CONFIG_MOSH_AT_CMD_MODE)
#include "at_cmd_mode.h"
#include "at_cmd_mode_sett.h"
#endif

extern char mosh_at_resp_buf[MOSH_AT_CMD_RESPONSE_MAX_LEN];
extern struct k_mutex mosh_at_resp_buf_mutex;

static const char at_usage_str[] =
	"Usage: at <subcommand>\n"
	"\n"
	"Subcommands:\n"
	"  events_enable     Enable AT event handler which prints AT notifications\n"
	"  events_disable    Disable AT event handler\n"
#if defined(CONFIG_MOSH_AT_CMD_MODE)
	"  at_cmd_mode       AT command mode.\n"
#endif
	"\n"
	"Any other subcommand is interpreted as AT command and sent to the modem.\n";

AT_MONITOR(mosh_at_handler, ANY, at_cmd_handler, PAUSED);

#if defined(CONFIG_MOSH_AT_CMD_MODE)

static int at_shell_cmd_mode_start(const struct shell *shell, size_t argc, char **argv)
{
	at_cmd_mode_start(shell);
	return 0;
}

static int at_shell_cmd_mode_enable_autostart(const struct shell *shell, size_t argc, char **argv)
{
	at_cmd_mode_sett_autostart_enabled(true);
	return 0;
}
static int at_shell_cmd_mode_disable_autostart(const struct shell *shell, size_t argc, char **argv)
{
	at_cmd_mode_sett_autostart_enabled(false);
	return 0;
}
static int at_shell_cmd_mode_term_cr_lf(const struct shell *shell, size_t argc, char **argv)
{
	at_cmd_mode_line_termination_set(CR_LF_TERM);
	return 0;
}
static int at_shell_cmd_mode_term_lf(const struct shell *shell, size_t argc, char **argv)
{
	at_cmd_mode_line_termination_set(LF_TERM);
	return 0;
}
static int at_shell_cmd_mode_term_cr(const struct shell *shell, size_t argc, char **argv)
{
	at_cmd_mode_line_termination_set(CR_TERM);
	return 0;
}
#endif

static void at_cmd_handler(const char *response)
{
	mosh_print("AT event handler: %s", response);
}

static int at_shell(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	if (argc < 2) {
		mosh_print_no_format(at_usage_str);
		return 0;
	}

	char *command = argv[1];

	if (!strcmp(command, "events_enable")) {
		at_monitor_resume(&mosh_at_handler);
		mosh_print("AT command events enabled");
	} else if (!strcmp(command, "events_disable")) {
		at_monitor_pause(&mosh_at_handler);
		mosh_print("AT command events disabled");
	} else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		mosh_print_no_format(at_usage_str);
	} else {
		k_mutex_lock(&mosh_at_resp_buf_mutex, K_FOREVER);
		err = nrf_modem_at_cmd(mosh_at_resp_buf, sizeof(mosh_at_resp_buf), "%s", command);
		if (err == 0) {
			mosh_print("%s", mosh_at_resp_buf);
		} else if (err > 0) {
			mosh_error("%s", mosh_at_resp_buf);
			err = -EINVAL;
		} else {
			/* Negative values are error codes */
			mosh_error("Failed to send AT command, err %d", err);
		}
		k_mutex_unlock(&mosh_at_resp_buf_mutex);
	}

	return 0;
}

#if defined(CONFIG_MOSH_AT_CMD_MODE)
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_at_cmd_mode,
	SHELL_CMD(start, NULL,
		  "Start AT command mode.",
		  at_shell_cmd_mode_start),
	SHELL_CMD(enable_autostart, NULL,
		  "Enable AT command autostart on bootup.",
		  at_shell_cmd_mode_enable_autostart),
	SHELL_CMD(disable_autostart, NULL,
		  "Disable AT command mode autostart on bootup.",
		  at_shell_cmd_mode_disable_autostart),
	SHELL_CMD(term_cr_lf, NULL,
		  "Receive CR+LF as command line termination.",
		  at_shell_cmd_mode_term_cr_lf),
	SHELL_CMD(term_lf, NULL,
		  "Receive LF as command line termination.",
		  at_shell_cmd_mode_term_lf),
	SHELL_CMD(term_cr, NULL,
		  "Receive CR as command line termination.",
		  at_shell_cmd_mode_term_cr),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_at_shell,
	SHELL_CMD(at_cmd_mode, &sub_at_cmd_mode,
		  "Enable/disable AT command mode.",
		  mosh_print_help_shell),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(at, &sub_at_shell, "Execute an AT command.", at_shell);
#else
SHELL_CMD_REGISTER(at, NULL, "Execute an AT command.", at_shell);
#endif
