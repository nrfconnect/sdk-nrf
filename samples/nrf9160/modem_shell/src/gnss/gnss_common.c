/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <shell/shell.h>
#include <modem/at_cmd.h>

#include "gnss.h"

#ifdef CONFIG_BOARD_THINGY91_NRF9160_NS
#define GNSS_LNA_ENABLE_XMAGPIO "AT\%XMAGPIO=1,1,1,7,1,746,803,2,698,748," \
"2,1710,2200,3,824,894,4,880,960,5,791,849," \
"7,1565,1586"
#else
#define GNSS_LNA_ENABLE_XMAGPIO "AT\%XMAGPIO=1,0,0,1,1,1574,1577"
#endif

#define GNSS_LNA_DISABLE_XMAGPIO "AT\%XMAGPIO"
#define GNSS_LNA_ENABLE_XCOEX0 "AT\%XCOEX0=1,1,1565,1586"
#define GNSS_LNA_DISABLE_XCOEX0 "AT\%XCOEX0"

extern const struct shell *shell_global;

int gnss_set_lna_enabled(bool enabled)
{
	int err;
	const char *xmagpio_command;
	const char *xcoex0_command;

	if (enabled) {
		xmagpio_command = GNSS_LNA_ENABLE_XMAGPIO;
		xcoex0_command = GNSS_LNA_ENABLE_XCOEX0;
	} else {
		xmagpio_command = GNSS_LNA_DISABLE_XMAGPIO;
		xcoex0_command = GNSS_LNA_DISABLE_XCOEX0;
	}

	err = at_cmd_write(xmagpio_command, NULL, 0, NULL);
	if (err) {
		shell_error(shell_global, "Failed to send XMAGPIO command");
		return err;
	}

	err = at_cmd_write(xcoex0_command, NULL, 0, NULL);
	if (err) {
		shell_error(shell_global, "Failed to send XCOEX0 command");
		return err;
	}

	return 0;
}
