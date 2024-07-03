/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <zephyr/net/net_ip.h>

#include <openthread/cli.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/thread.h>

#include <string.h>

static int ot_cli_output_cb(void *context, const char *format, va_list arg)
{
	const struct shell *sh = context;

	shell_vfprintf(sh, SHELL_NORMAL, format, arg);

	return 0;
}

static void ot_cli_lazy_init(const struct shell *sh)
{
	static bool is_initialized;

	if (is_initialized) {
		return;
	}

	otCliInit(NULL, ot_cli_output_cb, (void *)sh);
	is_initialized = true;
}

typedef otError(*ot_cli_command_handler_t)(const struct shell *, size_t argc, char *argv[]);

/*
 * Adapter function for processing OpenThread shell commands, which is convenient to call from
 * a Zephyr shell command handler. It is meant to duplicate the behavior of the internal OT CLI
 * component with regards to printing the command's result.
 */
static int ot_cli_command_invoke(ot_cli_command_handler_t handler, const struct shell *sh,
				 size_t argc, char *argv[])
{
	otError error;

	ot_cli_lazy_init(sh);
	error = handler(sh, argc, argv);

	if (error != OT_ERROR_NONE) {
		shell_print(sh, "Error %d", error);
		return -EINVAL;
	}

	shell_print(sh, "Done");
	return 0;
}

static otError ot_cli_command_ifconfig(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc <= 1) {
		shell_print(sh, "%s", otIp6IsEnabled(NULL) ? "up" : "down");
		return OT_ERROR_NONE;
	}

	if (strcmp(argv[1], "up") == 0) {
		return otIp6SetEnabled(NULL, true);
	}

	if (strcmp(argv[1], "down") == 0) {
		return otIp6SetEnabled(NULL, false);
	}

	return OT_ERROR_INVALID_COMMAND;
}

static otError ot_cli_command_ipmaddr(const struct shell *sh, size_t argc, char *argv[])
{
	struct in6_addr in6_addr;

	if (argc <= 1) {
		const otNetifMulticastAddress *addr;
		char addr_string[NET_IPV6_ADDR_LEN];

		for (addr = otIp6GetMulticastAddresses(NULL); addr != NULL; addr = addr->mNext) {
			memcpy(in6_addr.s6_addr, addr->mAddress.mFields.m8, OT_IP6_ADDRESS_SIZE);

			if (!net_addr_ntop(AF_INET6, &in6_addr, addr_string, sizeof(addr_string))) {
				return OT_ERROR_FAILED;
			}

			shell_print(sh, "%s", addr_string);
		}

		return OT_ERROR_NONE;
	}

	if (argc == 3 && strcmp(argv[1], "add") == 0) {
		otIp6Address addr;

		if (net_addr_pton(AF_INET6, argv[2], &in6_addr)) {
			return OT_ERROR_INVALID_ARGS;
		}

		memcpy(addr.mFields.m8, in6_addr.s6_addr, OT_IP6_ADDRESS_SIZE);

		return otIp6SubscribeMulticastAddress(NULL, &addr);
	}

	if (argc == 3 && strcmp(argv[1], "del") == 0) {
		otIp6Address addr;

		if (net_addr_pton(AF_INET6, argv[2], &in6_addr)) {
			return OT_ERROR_INVALID_ARGS;
		}

		memcpy(addr.mFields.m8, in6_addr.s6_addr, OT_IP6_ADDRESS_SIZE);

		return otIp6UnsubscribeMulticastAddress(NULL, &addr);
	}

	return OT_ERROR_INVALID_ARGS;
}

static otError ot_cli_command_mode(const struct shell *sh, size_t argc, char *argv[])
{
	otLinkModeConfig mode;

	if (argc <= 1) {
		char mode_string[3];
		size_t i = 0;

		mode = otThreadGetLinkMode(NULL);

		if (mode.mRxOnWhenIdle) {
			mode_string[i++] = 'r';
		}

		if (mode.mDeviceType) {
			mode_string[i++] = 'd';
		}

		if (mode.mNetworkData) {
			mode_string[i++] = 'n';
		}

		if (i == 0) {
			mode_string[i++] = '-';
		}

		shell_print(sh, "%.*s", i, mode_string);
		return OT_ERROR_NONE;
	}

	memset(&mode, 0, sizeof(mode));

	if (strcmp(argv[1], "-") != 0) {
		for (const char *c = argv[1]; *c; c++) {
			switch (*c) {
			case 'r':
				mode.mRxOnWhenIdle = true;
				break;
			case 'd':
				mode.mDeviceType = true;
				break;
			case 'n':
				mode.mNetworkData = true;
				break;
			default:
				return OT_ERROR_INVALID_ARGS;
			}
		}
	}

	return otThreadSetLinkMode(NULL, mode);
}

static otError ot_cli_command_pollperiod(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc <= 1) {
		shell_print(sh, "%u", (unsigned int)otLinkGetPollPeriod(NULL));
		return OT_ERROR_NONE;
	}

	return otLinkSetPollPeriod(NULL, strtoul(argv[1], NULL, 10));
}

static otError ot_cli_command_state(const struct shell *sh, size_t argc, char *argv[])
{
	static const char *const role_str[] = {"disabled", "detached", "child", "router", "leader"};

	const otDeviceRole role = otThreadGetDeviceRole(NULL);

	switch (role) {
	case OT_DEVICE_ROLE_DISABLED:
	case OT_DEVICE_ROLE_DETACHED:
	case OT_DEVICE_ROLE_CHILD:
	case OT_DEVICE_ROLE_ROUTER:
	case OT_DEVICE_ROLE_LEADER:
		shell_print(sh, "%s", role_str[role]);
		return OT_ERROR_NONE;
	default:
		return OT_ERROR_FAILED;
	}
}

static otError ot_cli_command_thread(const struct shell *sh, size_t argc, char *argv[])
{
	if (strcmp(argv[2], "start")) {
		return otThreadSetEnabled(NULL, true);
	}

	if (strcmp(argv[2], "stop")) {
		return otThreadSetEnabled(NULL, false);
	}

	return OT_ERROR_INVALID_COMMAND;
}

static int cmd_ifconfig(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_ifconfig, sh, argc, argv);
}

static int cmd_ipmaddr(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_ipmaddr, sh, argc, argv);
}

static int cmd_mode(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_mode, sh, argc, argv);
}

static int cmd_pollperiod(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_pollperiod, sh, argc, argv);
}

static int cmd_state(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_state, sh, argc, argv);
}

static int cmd_thread(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_thread, sh, argc, argv);
}

/*
 * Root/fallback command handler for OpenThread shell commands that have not been implemented
 * using the Zephyr shell framework and must be relayed to the internal OpenThread's CLI component.
 */
static int cmd_ot(const struct shell *sh, size_t argc, char *argv[])
{
	char buffer[128];
	char *ptr = buffer;
	char *end = buffer + sizeof(buffer);

	ot_cli_lazy_init(sh);

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

SHELL_STATIC_SUBCMD_SET_CREATE(
	ot_cmds, SHELL_CMD_ARG(ifconfig, NULL, "Interface management", cmd_ifconfig, 1, 1),
	SHELL_CMD_ARG(ipmaddr, NULL, "IPv6 multicast configuration", cmd_ipmaddr, 1, 2),
	SHELL_CMD_ARG(mode, NULL, "Mode configuration", cmd_mode, 1, 1),
	SHELL_CMD_ARG(pollperiod, NULL, "Polling configuration", cmd_pollperiod, 1, 1),
	SHELL_CMD_ARG(state, NULL, "Current role", cmd_state, 1, 0),
	SHELL_CMD_ARG(thread, NULL, "Role management", cmd_thread, 2, 0), SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(ot, &ot_cmds,
		       "OpenThread subcommands\n"
		       "Use \"ot help\" to get the list of subcommands",
		       cmd_ot, 2, 255);
