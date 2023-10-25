/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>

#include <zephyr/shell/shell.h>
#ifdef CONFIG_GETOPT
#include <zephyr/posix/unistd.h>
#endif
#include <getopt.h>

#include "link_api.h"
#include "mosh_print.h"
#include "icmp_ping.h"
#include "icmp_ping_print.h"
#include "icmp_ping_shell.h"

static const char icmp_ping_shell_cmd_usage_str[] =
	"Usage: ping [optional options] -d destination\n"
	"\n"
	"mandatory options:\n"
	"  -d, --destination, [str] Name or IP address\n"
	"optional options:\n"
	"  -t, --timeout, [int]     Ping timeout in milliseconds\n"
	"  -c, --count, [int]       The number of times to send the ping request\n"
	"  -i, --interval, [int]    Interval between successive packet transmissions\n"
	"                           in milliseconds\n"
	"  -l, --length, [int]      Payload length to be sent\n"
	"  -I, --cid, [int]         Use this option to bind pinging to specific CID.\n"
	"                           See link cmd for interfaces\n"
	"  -6, --ipv6,              Force IPv6 usage with the dual stack interfaces\n"
	"  -r, --rai                Set RAI options for ping socket. In order to use RAI,\n"
	"                           it must be enabled with 'link rai' command.\n"
	"  -h, --help,              Shows this help information\n";

/* Specifying the expected options (both long and short): */
static struct option long_options[] = {
	{ "destination", required_argument, 0, 'd' },
	{ "timeout", required_argument, 0, 't' },
	{ "count", required_argument, 0, 'c' },
	{ "interval", required_argument, 0, 'i' },
	{ "length", required_argument, 0, 'l' },
	{ "cid", required_argument, 0, 'I' },
	{ "ipv6", no_argument, 0, '6' },
	{ "rai", no_argument, 0, 'r' },
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0 }
};

static void icmp_ping_shell_usage_print(void)
{
	mosh_print_no_format(icmp_ping_shell_cmd_usage_str);
}

/*****************************************************************************/

static int icmp_ping_shell(const struct shell *shell, size_t argc, char **argv)
{
	return icmp_ping_shell_th(shell, argc, argv, NULL, 0, NULL);
}

int icmp_ping_shell_th(const struct shell *shell, size_t argc, char **argv,
	char *print_buf, int print_buf_len, struct k_poll_signal *kill_signal)
{
	struct icmp_ping_shell_cmd_argv ping_args;
	int opt, dest_len;

	icmp_ping_cmd_defaults_set(&ping_args);

#if defined(CONFIG_MOSH_WORKER_THREADS)
	ping_args.print_buf = print_buf;
	ping_args.print_buf_len = print_buf_len;

	if (kill_signal != NULL) {
		ping_args.kill_signal = kill_signal;
	}
#endif

	/* Start from the 1st argument */
	optind = 1;

	while ((opt = getopt_long(argc, argv, "d:t:c:i:I:l:h6r", long_options, NULL)) != -1) {

		switch (opt) {
		case 'd': /* destination */
			dest_len = strlen(optarg);
			if (dest_len > ICMP_MAX_URL) {
				ping_error(&ping_args, "too long destination name");
				goto show_usage;
			}
			strcpy(ping_args.target_name, optarg);
			break;
		case 't': /* timeout */
			ping_args.timeout = atoi(optarg);
			if (ping_args.timeout == 0) {
				ping_warn(
					&ping_args,
					"timeout not an integer (> 0), defaulting to %d msecs",
					ICMP_PARAM_TIMEOUT_DEFAULT);
				ping_args.timeout = ICMP_PARAM_TIMEOUT_DEFAULT;
			}
			break;
		case 'I': /* PDN CID */
			ping_args.cid = atoi(optarg);
			if (ping_args.cid == 0) {
				ping_warn(&ping_args, "CID not an integer (> 0), default context used");
				ping_args.cid = MOSH_ARG_NOT_SET;
			}
			break;
		case 'c': /* count */
			ping_args.count = atoi(optarg);
			if (ping_args.count == 0) {
				ping_warn(
					&ping_args,
					"count not an integer (> 0), defaulting to %d",
					ICMP_PARAM_COUNT_DEFAULT);
				ping_args.timeout = ICMP_PARAM_COUNT_DEFAULT;
			}
			break;
		case 'i': /* interval */
			ping_args.interval = atoi(optarg);
			if (ping_args.interval == 0) {
				ping_warn(
					&ping_args,
					"interval not an integer (> 0), defaulting to %d",
					ICMP_PARAM_INTERVAL_DEFAULT);
				ping_args.interval = ICMP_PARAM_INTERVAL_DEFAULT;
			}
			break;
		case 'l': /* payload length */
			ping_args.len = atoi(optarg);
			if (ping_args.len > ICMP_IPV4_MAX_LEN) {
				ping_error(
					&ping_args,
					"Payload size exceeds the ultimate max limit %d",
					ICMP_IPV4_MAX_LEN);
				goto show_usage;
			}
			break;
		case '6': /* force ipv6 */
			ping_args.force_ipv6 = true;
			break;
		case 'r': /* RAI */
			{
			bool rai_status = false;

			ping_args.rai = true;
			(void)link_api_rai_status(&rai_status);
			/* If RAI is disabled or reading it fails, show warning. It's only
			 * warning because RAI status may be out of sync if device hadn't gone
			 * to normal mode since changing it.
			 */
			if (!rai_status) {
				ping_warn(
					&ping_args,
					"RAI is requested but RAI is disabled.\n"
					"Use 'link rai' command to enable it for ping usage.");
			}
			break;
			}

		case '?':
			mosh_error("Unknown option. See usage:");
			goto show_usage;
		case 'h':
		default:
			goto show_usage;
		}
	}

	/* Check that all mandatory args were given: */
	if (strlen(ping_args.target_name) == 0) {
		ping_error(&ping_args, "-d destination, MUST be given. See usage:");
		goto show_usage;
	}

	return icmp_ping_start(&ping_args);

show_usage:
	icmp_ping_shell_usage_print();
	return -1;
}

SHELL_CMD_REGISTER(ping, NULL, "For ping usage, just type \"ping\"", icmp_ping_shell);
