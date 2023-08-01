/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <caf/events/button_event.h>

static const char button_event_cmd_help_str[] =
	"Submit a button_event with user-defined key ID and pressed state\n"
	"  Key ID           Decimal numeric ID of the button\n"
	"  Pressed state    'y' or 'n'\n";

static int button_event_handler(const struct shell *shell, size_t argc, char **argv)
{
	/* The string part of button_id param, for invalidation check */
	char *str_part;

	/* Convert button_id from string to decimal number */
	long button_id = strtol(argv[1], &str_part, 10);

	if (strlen(str_part) > 0 || button_id < 0 || button_id > UINT16_MAX) {
		shell_error(shell, "Invalid key ID");
		return -EINVAL;
	}

	if ((strlen(argv[2]) > 1) || (*argv[2] != 'y' && *argv[2] != 'n')) {
		shell_error(shell, "Invalid button press state");
		return -EINVAL;
	}

	struct button_event *event = new_button_event();

	event->key_id = (uint16_t)button_id;
	event->pressed = *argv[2] != 'n';
	APP_EVENT_SUBMIT(event);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_caf_shell_command,
	SHELL_CMD_ARG(button_event, NULL, button_event_cmd_help_str, button_event_handler, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(caf_events, &sub_caf_shell_command,
	"Submit a CAF event with user-defined parameters", NULL);
