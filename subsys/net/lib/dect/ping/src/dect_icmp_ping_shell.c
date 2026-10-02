/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/sys_getopt.h>
#include <zephyr/net/net_if.h>

#include <net/dect/dect_icmp_ping.h>

#include "dect_icmp_ping_internal.h"
#include "dect_icmp_ping_shell_ctx.h"

struct dect_icmp_ping_shell_ctx dect_icmp_ping_shell_ctx = {
	.shell = NULL,
	.print_fns = {0},
	.custom_print_enabled = false,
};

int dect_icmp_ping_init(const struct dect_net_lib_shell_print_fns *print_fns)
{
	if (print_fns == NULL) {
		dect_icmp_ping_shell_ctx.custom_print_enabled = false;
		memset(&dect_icmp_ping_shell_ctx.print_fns, 0,
		       sizeof(dect_icmp_ping_shell_ctx.print_fns));
	} else {
		dect_icmp_ping_shell_ctx.custom_print_enabled = true;
		dect_icmp_ping_shell_ctx.print_fns = *print_fns;
	}

	return 0;
}

static const char dect_icmp_ping_shell_usage[] =
	"Usage: ping [options] -d destination\n"
	"\n"
	"  -d, --destination, [str] Name or IP address\n"
	"Options:\n"
	"  -t, --timeout, [int]     Ping timeout in milliseconds\n"
	"  -c, --count, [int]       The number of times to send the ping request\n"
	"  -i, --interval, [int]    Interval between successive packet transmissions\n"
	"                           in milliseconds\n"
	"  -l, --length, [int]      Payload length to be sent\n"
	"  -I, --interface, [int]   Interface index to be used. Default: DECT NR+ interface.\n"
	"  -h, --help,              Shows this help information";

static struct sys_getopt_option long_options[] = {
	{ "destination", sys_getopt_required_argument, 0, 'd' },
	{ "timeout", sys_getopt_required_argument, 0, 't' },
	{ "count", sys_getopt_required_argument, 0, 'c' },
	{ "interval", sys_getopt_required_argument, 0, 'i' },
	{ "length", sys_getopt_required_argument, 0, 'l' },
	{ "interface", sys_getopt_required_argument, 0, 'I' },
	{ "help", sys_getopt_no_argument, 0, 'h' },
	{ 0, 0, 0, 0 }
};

static int dect_icmp_ping_shell_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_icmp_ping_argv ping_args;
	int dest_len, tmp_opt;

	dect_icmp_ping_set_shell(shell);

	if (argc < 2) {
		goto show_usage;
	}

	dect_icmp_ping_cmd_defaults_set(&ping_args);

	sys_getopt_init();
	int opt;

	while ((opt = sys_getopt_long(argc, argv, "d:t:c:i:l:I:h", long_options, NULL)) != -1) {
		switch (opt) {
		case 'I':
			tmp_opt = atoi(sys_getopt_optarg);
			if (tmp_opt < 0) {
				dect_icmp_ping_error("Interface index not an integer (>= 0)");
				return -1;
			}
			ping_args.ping_iface = net_if_get_by_index(tmp_opt);
			if (!ping_args.ping_iface) {
				dect_icmp_ping_error("%s: Interface with index %d not found",
						     __func__, tmp_opt);
				return -1;
			}
			break;
		case 'd':
			dest_len = strlen(sys_getopt_optarg);
			if (dest_len > DECT_ICMP_MAX_ADDR) {
				dect_icmp_ping_error("too long destination name");
				goto show_usage;
			}
			strcpy(ping_args.target_name, sys_getopt_optarg);
			break;
		case 't':
			ping_args.timeout = atoi(sys_getopt_optarg);
			if (ping_args.timeout == 0) {
				dect_icmp_ping_warn(
					"timeout not an integer (> 0), defaulting to %d msecs",
					DECT_ICMP_PARAM_TIMEOUT_DEFAULT);
				ping_args.timeout = DECT_ICMP_PARAM_TIMEOUT_DEFAULT;
			}
			break;
		case 'c':
			ping_args.count = atoi(sys_getopt_optarg);
			if (ping_args.count == 0) {
				dect_icmp_ping_warn("count not an integer (> 0), defaulting to %d",
						    DECT_ICMP_PARAM_COUNT_DEFAULT);
				ping_args.count = DECT_ICMP_PARAM_COUNT_DEFAULT;
			}
			break;
		case 'i':
			ping_args.interval = atoi(sys_getopt_optarg);
			if (ping_args.interval == 0) {
				dect_icmp_ping_warn(
					"interval not an integer (> 0), defaulting to %d",
					DECT_ICMP_PARAM_INTERVAL_DEFAULT);
				ping_args.interval = DECT_ICMP_PARAM_INTERVAL_DEFAULT;
			}
			break;
		case 'l':
			ping_args.len = atoi(sys_getopt_optarg);
			if (ping_args.len > DECT_ICMP_IPV6_MAX_LEN) {
				dect_icmp_ping_error(
					"Payload size exceeds the ultimate max limit %d",
					DECT_ICMP_IPV6_MAX_LEN);
				goto show_usage;
			}
			break;
		case 'h':
			goto show_usage;
		case '?':
		default:
			dect_icmp_ping_error("Unknown option (%s). See usage:",
					     argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		dect_icmp_ping_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	if (strlen(ping_args.target_name) == 0) {
		dect_icmp_ping_error("-d destination, MUST be given. See usage:");
		goto show_usage;
	}

	ping_args.shell = (struct shell *)shell;

	return dect_icmp_ping_start(&ping_args);

show_usage:
	dect_icmp_ping_print("%s", dect_icmp_ping_shell_usage);
	return -1;
}

SHELL_CMD_REGISTER(ping, NULL, "IPv6 ICMP ping; use ping -h for usage", dect_icmp_ping_shell_cmd);
