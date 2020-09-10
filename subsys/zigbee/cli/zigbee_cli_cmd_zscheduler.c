/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>

#include <zboss_api.h>
#include <zb_nrf_platform.h>
#include "zigbee_cli.h"


#ifdef CONFIG_ZIGBEE_SHELL_DEBUG_CMD
/**@brief Suspend Zigbee scheduler processing
 *
 * @code
 * zscheduler suspend
 * @endcode
 *
 */
static int cmd_zb_suspend(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	zigbee_debug_suspend_zboss_thread();
	zb_cli_print_done(shell, ZB_FALSE);

	return 0;
}

/**@brief Resume Zigbee scheduler processing
 *
 * @code
 * zscheduler resume
 * @endcode
 *
 */
static int cmd_zb_resume(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	zigbee_debug_resume_zboss_thread();
	zb_cli_print_done(shell, ZB_FALSE);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_zigbee,
	SHELL_CMD_ARG(resume, NULL, "Suspend Zigbee scheduler processing",
		      cmd_zb_resume, 1, 0),
	SHELL_CMD_ARG(suspend, NULL, "Suspend Zigbee scheduler processing",
		      cmd_zb_suspend, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(zscheduler, &sub_zigbee, "Zigbee scheduler manipulation",
		   NULL);
#endif
