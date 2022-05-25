/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <version.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zb_version.h>
#include <zigbee/zigbee_shell.h>
#include "zigbee_shell_utils.h"

#define DEBUG_HELP \
	"Return state of debug mode."

#define DEBUG_ON_HELP \
	"Turn on debug mode."

#define DEBUG_OFF_HELP \
	"Turn off debug mode."

#define DEBUG_WARN_MSG \
	"You are about to turn the debug mode on. This unblocks several\n" \
	"additional commands in the Shell. They can render the device unstable.\n" \
	"It is implied that you know what you are doing."


/**@brief Print Shell, ZBOSS and Zephyr kernel version
 *
 * @code
 * version
 * @endcode
 *
 * @code
 * > version
 * Shell: Jul 2 2020 16:14:18
 * ZBOSS: 3.1.0.59
 * Done
 * @endcode
 */
static int cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Shell: " __DATE__ " " __TIME__);
	shell_print(shell, "ZBOSS: %d.%d.0.%d", ZBOSS_MAJOR, ZBOSS_MINOR,
		    ZBOSS_SDK_REVISION);
	shell_print(shell, "Zephyr kernel version: %s", KERNEL_VERSION_STRING);

	zb_shell_print_done(shell, false);
	return 0;
}

/**@brief Get state of debug mode in the shell.
 *
 * @code
 * debug
 * @endcode
 */
static int cmd_debug(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Debug mode is %s.",
		    zb_shell_debug_get() ? "on" : "off");

	zb_shell_print_done(shell, false);
	return 0;
}

/**@brief Enable debug mode in the shell.
 *
 * @code
 * debug on
 * @endcode
 *
 * This command unblocks several additional commands in the shell.
 * They can render the device unstable. It is implied that you know
 * what you are doing.
 */
static int cmd_debug_on(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_warn(shell, DEBUG_WARN_MSG);
	zb_shell_debug_set(ZB_TRUE);
	shell_print(shell, "Debug mode is on.");

	zb_shell_print_done(shell, false);
	return 0;
}

/**@brief Disable debug mode in the shell.
 *
 * @code
 * debug off
 * @endcode
 *
 * This command blocks several additional commands in the shell.
 * They can render the device unstable. It is implied that you know
 * what you are doing.
 */
static int cmd_debug_off(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	zb_shell_debug_set(ZB_FALSE);
	shell_print(shell, "Debug mode is off.");

	zb_shell_print_done(shell, false);
	return 0;
}

SHELL_CMD_REGISTER(version, NULL, "Print firmware version", cmd_version);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_debug,
	SHELL_COND_CMD(CONFIG_ZIGBEE_SHELL_DEBUG_CMD, off, NULL,
		       DEBUG_OFF_HELP, cmd_debug_off),
	SHELL_COND_CMD(CONFIG_ZIGBEE_SHELL_DEBUG_CMD, on, NULL,
		       DEBUG_ON_HELP, cmd_debug_on),
	SHELL_SUBCMD_SET_END);

SHELL_COND_CMD_REGISTER(CONFIG_ZIGBEE_SHELL_DEBUG_CMD, debug, &sub_debug,
			DEBUG_HELP, cmd_debug);
