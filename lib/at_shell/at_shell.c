/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <nrf_modem_at.h>
#include <modem/at_monitor.h>
#include <modem/at_shell.h>

#include "at_shell_internal.h"

#define CREDENTIALS_CMD "AT%CMNG=0"

static struct shell *global_shell;
static const char at_usage_str[] =
	"Usage: at <subcommand>\n"
	"\n"
	"Subcommands:\n"
	"  events_enable     Enable AT event handler which prints AT notifications\n"
	"  events_disable    Disable AT event handler\n"
#if defined(CONFIG_AT_SHELL_CMD_MODE)
	"  at_cmd_mode       AT command mode subcommands\n"
#endif
	"\n"
	"Any other subcommand is interpreted as AT command and sent to the modem.\n";

AT_MONITOR(at_shell_monitor, ANY, at_notif_handler, PAUSED);

static void at_notif_handler(const char *response)
{
	/* The AT monitor is initialized in PAUSED state, which means that `at events_enable`
	 * must be run first, at which point `global_shell` will be set to `shell`.
	 * The assert below is added to catch the case where the monitor is initialized to
	 * ACTIVE and no shell command has been invoked before the first AT notification arrives.
	 */
	__ASSERT(global_shell != NULL, "global_shell has not been set to a valid shell");
	shell_print(global_shell, "AT event handler:\n%s", response);
}

static int sub_at_cmd_events_enable(const struct shell *shell, size_t argc, char **argv)
{
	(void)argc;
	(void)argv;

	global_shell = (struct shell *)shell;

	at_monitor_resume(&at_shell_monitor);
	shell_print(shell, "AT command event handler enabled");

	return 0;
}

static int sub_at_cmd_events_disable(const struct shell *shell, size_t argc, char **argv)
{
	(void)argc;
	(void)argv;

	global_shell = (struct shell *)shell;

	at_monitor_pause(&at_shell_monitor);
	shell_print(shell, "AT command event handler disabled");

	return 0;
}

#if defined(CONFIG_AT_SHELL_CMD_MODE)
static int at_shell_cmd_mode_shell(const struct shell *shell, size_t argc, char **argv)
{
	(void)argc;
	(void)argv;

	global_shell = (struct shell *)shell;

	shell_print(shell, "Usage: at at_cmd_mode <subcommand>");
	shell_print(shell, "Subcommands: start");
	shell_print(shell,
		    "  start [term_null|term_cr|term_lf|term_cr_lf] [echo_on|echo_off]");

	return 0;
}

static int at_shell_cmd_mode_start_shell(const struct shell *shell, size_t argc, char **argv)
{
	struct at_shell_cmd_mode_config cfg = {
		.termination =
			(enum at_shell_cmd_mode_termination)CONFIG_AT_SHELL_CMD_MODE_TERMINATION,
		.echo = IS_ENABLED(CONFIG_AT_SHELL_CMD_MODE_ECHO_DEFAULT),
	};

	for (size_t i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "term_null")) {
			cfg.termination = AT_SHELL_CMD_MODE_TERM_NULL;
		} else if (!strcmp(argv[i], "term_cr")) {
			cfg.termination = AT_SHELL_CMD_MODE_TERM_CR;
		} else if (!strcmp(argv[i], "term_lf")) {
			cfg.termination = AT_SHELL_CMD_MODE_TERM_LF;
		} else if (!strcmp(argv[i], "term_cr_lf")) {
			cfg.termination = AT_SHELL_CMD_MODE_TERM_CR_LF;
		} else if (!strcmp(argv[i], "echo_on")) {
			cfg.echo = true;
		} else if (!strcmp(argv[i], "echo_off")) {
			cfg.echo = false;
		} else {
			shell_error(shell, "Unknown option: %s", argv[i]);
			return -EINVAL;
		}
	}

	if (at_shell_work_q_get() == NULL) {
		shell_error(shell, "AT command mode work queue not initialized");
		return -EINVAL;
	}

	return at_shell_cmd_mode_start(shell, &cfg);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_cmd_at_cmd_mode,
	SHELL_CMD_ARG(start, NULL, NULL, at_shell_cmd_mode_start_shell, 1, 2),
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_AT_SHELL_CMD_MODE */

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_at_shell,
	SHELL_CMD_ARG(events_enable, NULL, NULL, sub_at_cmd_events_enable, 1, 0),
	SHELL_CMD_ARG(events_disable, NULL, NULL, sub_at_cmd_events_disable, 1, 0),
#if defined(CONFIG_AT_SHELL_CMD_MODE)
	SHELL_CMD(at_cmd_mode, &sub_cmd_at_cmd_mode, NULL, at_shell_cmd_mode_shell),
#endif
	SHELL_SUBCMD_SET_END);

int at_shell(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	static char response[CONFIG_AT_SHELL_CMD_RESPONSE_MAX_LEN + 1];

	global_shell = (struct shell *)shell;

	if (argc < 2) {
		shell_print(shell, "%s", at_usage_str);
		return 0;
	}

	char *command = argv[1];

	if (IS_ENABLED(CONFIG_AT_SHELL_UNESCAPE_LF)) {
		/* Replace the two character sequence "\n" with <CR><LF> so AT%CMNG=0,
		 * which accepts multiline input, will work with a properly
		 * pre-processed cert. Without this, the second and subsequent lines of the
		 * cert are treated as new shell commands, which of course fail.
		 */
		if (strncmp(command, CREDENTIALS_CMD, strlen(CREDENTIALS_CMD)) == 0) {
			char *c = command;

			while ((c = strstr(c, "\\n")) != NULL) {
				c[0] = '\r';
				c[1] = '\n';
			}
		}
	}

	err = nrf_modem_at_cmd(response, sizeof(response), "%s", command);
	if (err < 0) {
		shell_print(shell, "Sending AT command failed with error code %d", err);
		return err;
	} else if (err) {
		shell_print(shell, "%s", response);
		return -EINVAL;
	}

	shell_print(shell, "%s", response);
	return 0;
}

SHELL_CMD_REGISTER(at, &sub_at_shell,
	"Execute an AT command. Known subcommands are tab-completable; any other "
	"subcommand is interpreted as an AT command and sent to the modem.",
	at_shell);
