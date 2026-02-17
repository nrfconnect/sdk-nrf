/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/shell/shell.h>
#ifdef CONFIG_POSIX_C_LIB_EXT
#include <zephyr/posix/unistd.h>
#endif
#include <zephyr/sys/sys_getopt.h>
#include <zephyr/net/net_if.h>

#include "icmp_ping.h"

#include "desh_print.h"

static const char icmp_ping_shell_cmd_usage_str[] =
	"Usage: ping [options] -d destination\n"
	"\n"
	"  -d, --destination, [str] Name or IP address\n"
	"Options:\n"
	"  -t, --timeout, [int]     Ping timeout in milliseconds\n"
	"  -c, --count, [int]       The number of times to send the ping request\n"
	"  -i, --interval, [int]    Interval between successive packet transmissions\n"
	"                           in milliseconds\n"
	"  -l, --length, [int]      Payload length to be sent\n"
	"  -6, --ipv6,              Force IPv6 usage with the dual stack interfaces\n"
	"  -I, --interface, [int]   Interface index to be used. Default: DECT NR+ interface.\n"
	"  -h, --help,              Shows this help information";

/* Specifying the expected options (both long and short) */
static struct sys_getopt_option long_options[] = {
	{ "destination", sys_getopt_required_argument, 0, 'd' },
	{ "timeout", sys_getopt_required_argument, 0, 't' },
	{ "count", sys_getopt_required_argument, 0, 'c' },
	{ "interval", sys_getopt_required_argument, 0, 'i' },
	{ "length", sys_getopt_required_argument, 0, 'l' },
	{ "ipv6", sys_getopt_no_argument, 0, '6' },
	{ "interface", sys_getopt_required_argument, 0, 'I' },
	{ "help", sys_getopt_no_argument, 0, 'h' },
	{ 0, 0, 0, 0 }
};

static int icmp_ping_shell_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct icmp_ping_shell_cmd_argv ping_args;
	int dest_len, tmp_opt;

	if (argc < 2) {
		goto show_usage;
	}

	icmp_ping_cmd_defaults_set(&ping_args);

	sys_getopt_init();
	int opt;

	while ((opt = sys_getopt_long(argc, argv, "d:t:c:i:l:I:h6", long_options, NULL)) != -1) {
		switch (opt) {
		case 'I': /* interface */
			tmp_opt = atoi(sys_getopt_optarg);
			if (tmp_opt < 0) {
				desh_error("Interface index not an integer (>= 0)");
				return -1;
			}
			ping_args.ping_iface = net_if_get_by_index(tmp_opt);
			if (!ping_args.ping_iface) {
				desh_error("%s: Interface with index %d not found", (__func__),
					   tmp_opt);
				return -1;
			}
			break;
		case 'd': /* destination */
			dest_len = strlen(sys_getopt_optarg);
			if (dest_len > ICMP_MAX_ADDR) {
				desh_error("too long destination name");
				goto show_usage;
			}
			strcpy(ping_args.target_name, sys_getopt_optarg);
			break;
		case 't': /* timeout */
			ping_args.timeout = atoi(sys_getopt_optarg);
			if (ping_args.timeout == 0) {
				desh_warn("timeout not an integer (> 0), defaulting to %d msecs",
					  ICMP_PARAM_TIMEOUT_DEFAULT);
				ping_args.timeout = ICMP_PARAM_TIMEOUT_DEFAULT;
			}
			break;
		case 'c': /* count */
			ping_args.count = atoi(sys_getopt_optarg);
			if (ping_args.count == 0) {
				desh_warn("count not an integer (> 0), defaulting to %d",
					  ICMP_PARAM_COUNT_DEFAULT);
				ping_args.timeout = ICMP_PARAM_COUNT_DEFAULT;
			}
			break;
		case 'i': /* interval */
			ping_args.interval = atoi(sys_getopt_optarg);
			if (ping_args.interval == 0) {
				desh_warn("interval not an integer (> 0), defaulting to %d",
					  ICMP_PARAM_INTERVAL_DEFAULT);
				ping_args.interval = ICMP_PARAM_INTERVAL_DEFAULT;
			}
			break;
		case 'l': /* payload length */
			ping_args.len = atoi(sys_getopt_optarg);
			if (ping_args.len > ICMP_IPV4_MAX_LEN) {
				desh_error("Payload size exceeds the ultimate max limit %d",
					   ICMP_IPV4_MAX_LEN);
				goto show_usage;
			}
			break;
		case '6': /* force ipv6 */
			ping_args.force_ipv6 = true;
			break;
		case 'h':
			goto show_usage;
		case '?':
		default:
			desh_error("Unknown option (%s). See usage:",
				   argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		desh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	/* Check that all mandatory args were given */
	if (strlen(ping_args.target_name) == 0) {
		desh_error("-d destination, MUST be given. See usage:");
		goto show_usage;
	}
	ping_args.shell = (struct shell *)shell;

	return icmp_ping_start(&ping_args);

show_usage:
	shell_print(shell, "%s", icmp_ping_shell_cmd_usage_str);
	return -1;
}

SHELL_CMD_REGISTER(ping, NULL, "For ping usage, just type \"ping\"", icmp_ping_shell_cmd);
