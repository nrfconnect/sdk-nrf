/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <net/lwm2m_client_utils_location.h>
#include "location_events.h"
#include "gnss_module.h"

#define HELP_CMD  "LwM2M client application commands"
#define HELP_GNSS "Trigger GNSS search"
#define HELP_GFIX "Trigger Ground Fix location request"

static int cmd_gnss(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS) ||                                    \
	defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
	shell_print(sh, "Starting GNSS\n");
	start_gnss();
	return 0;
#else
	shell_print(sh, "A-GNSS not enabled\n");
	return -ENOEXEC;
#endif
}

static int cmd_gfix(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT)
	shell_print(sh, "Send cell location request event\n");
	struct ground_fix_location_request_event *ground_fix_event =
		new_ground_fix_location_request_event();

	APP_EVENT_SUBMIT(ground_fix_event);
	return 0;
#else
	shell_print(sh, "Ground Fix not enabled\n");
	return -ENOEXEC;
#endif
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_app,
			       SHELL_COND_CMD_ARG(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSISTANCE,
						  gnss, NULL, HELP_GNSS, cmd_gnss, 1, 0),
			       SHELL_COND_CMD_ARG(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT,
						  gfix, NULL, HELP_GFIX, cmd_gfix, 1, 0),
			       SHELL_SUBCMD_SET_END);
SHELL_CMD_ARG_REGISTER(app, &sub_app, HELP_CMD, NULL, 1, 0);
