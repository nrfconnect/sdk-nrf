/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <modem/at_cmd.h>

#include "mosh_print.h"

int gnss_configure_lna(void)
{
	int err;
	const char *xmagpio_command;
	const char *xcoex0_command;

	xmagpio_command = CONFIG_MOSH_AT_MAGPIO;
	xcoex0_command = CONFIG_MOSH_AT_COEX0;

	/* Make sure the AT command is not empty */
	if (xmagpio_command[0] != '\0') {
		err = at_cmd_write(xmagpio_command, NULL, 0, NULL);
		if (err) {
			mosh_error("Failed to send XMAGPIO command");
			return err;
		}
	}

	if (xcoex0_command[0] != '\0') {
		err = at_cmd_write(xcoex0_command, NULL, 0, NULL);
		if (err) {
			mosh_error("Failed to send XCOEX0 command");
			return err;
		}
	}

	return 0;
}
