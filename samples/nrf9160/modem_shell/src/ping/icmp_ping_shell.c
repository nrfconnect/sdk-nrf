/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr.h>

#include <shell/shell.h>
#include <getopt.h>

#include "link_api.h"
#include "mosh_print.h"
#include "icmp_ping.h"
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

static void
icmp_ping_shell_cmd_defaults_set(struct icmp_ping_shell_cmd_argv *ping_args)
{
	memset(ping_args, 0, sizeof(struct icmp_ping_shell_cmd_argv));
	/* ping_args->dest = NULL; */
	ping_args->count = ICMP_PARAM_COUNT_DEFAULT;
	ping_args->interval = ICMP_PARAM_INTERVAL_DEFAULT;
	ping_args->timeout = ICMP_PARAM_TIMEOUT_DEFAULT;
	ping_args->len = ICMP_PARAM_LENGTH_DEFAULT;
	ping_args->cid = MOSH_ARG_NOT_SET;
	ping_args->pdn_id_for_cid = MOSH_ARG_NOT_SET;
}

static bool icmp_ping_shell_set_ping_args_according_to_pdp_ctx(
	struct icmp_ping_shell_cmd_argv *ping_args,
	struct pdp_context_info_array *ctx_info_tbl)
{
	/* Find PDP context info for requested CID: */
	int i;
	bool found = false;

	for (i = 0; i < ctx_info_tbl->size; i++) {
		if (ctx_info_tbl->array[i].cid == ping_args->cid) {
			ping_args->current_pdp_type = ctx_info_tbl->array[i].pdp_type;
			ping_args->current_addr4 = ctx_info_tbl->array[i].ip_addr4;
			ping_args->current_addr6 = ctx_info_tbl->array[i].ip_addr6;

			if (ctx_info_tbl->array[i].pdn_id_valid) {
				ping_args->pdn_id_for_cid = ctx_info_tbl->array[i].pdn_id;
			}

			strcpy(ping_args->current_apn_str, ctx_info_tbl->array[i].apn_str);

			if (ctx_info_tbl->array[0].ipv4_mtu != 0) {
				ping_args->mtu = ctx_info_tbl->array[0].ipv4_mtu;
			} else {
				ping_args->mtu = ICMP_DEFAULT_LINK_MTU;
			}

			found = true;
		}
	}
	return found;
}
/*****************************************************************************/

int icmp_ping_shell(const struct shell *shell, size_t argc, char **argv)
{
	struct icmp_ping_shell_cmd_argv ping_args;
	int flag, dest_len;

	icmp_ping_shell_cmd_defaults_set(&ping_args);

	if (argc < 3) {
		goto show_usage;
	}

	/* Start from the 1st argument */
	optind = 1;

	while ((flag = getopt_long(argc, argv, "d:t:c:i:I:l:h6r", long_options, NULL)) != -1) {

		switch (flag) {
		case 'd': /* destination */
			dest_len = strlen(optarg);
			if (dest_len > ICMP_MAX_URL) {
				mosh_error("too long destination name");
				goto show_usage;
			}
			strcpy(ping_args.target_name, optarg);
			break;
		case 't': /* timeout */
			ping_args.timeout = atoi(optarg);
			if (ping_args.timeout == 0) {
				mosh_warn(
					"timeout not an integer (> 0), defaulting to %d msecs",
					ICMP_PARAM_TIMEOUT_DEFAULT);
				ping_args.timeout = ICMP_PARAM_TIMEOUT_DEFAULT;
			}
			break;
		case 'I': /* PDN CID */
			ping_args.cid = atoi(optarg);
			if (ping_args.cid == 0) {
				mosh_warn("CID not an integer (> 0), default context used");
				ping_args.cid = MOSH_ARG_NOT_SET;
			}
			break;
		case 'c': /* count */
			ping_args.count = atoi(optarg);
			if (ping_args.count == 0) {
				mosh_warn(
					"count not an integer (> 0), defaulting to %d",
					ICMP_PARAM_COUNT_DEFAULT);
				ping_args.timeout = ICMP_PARAM_COUNT_DEFAULT;
			}
			break;
		case 'i': /* interval */
			ping_args.interval = atoi(optarg);
			if (ping_args.interval == 0) {
				mosh_warn(
					"interval not an integer (> 0), defaulting to %d",
					ICMP_PARAM_INTERVAL_DEFAULT);
				ping_args.interval = ICMP_PARAM_INTERVAL_DEFAULT;
			}
			break;
		case 'l': /* payload length */
			ping_args.len = atoi(optarg);
			if (ping_args.len > ICMP_IPV4_MAX_LEN) {
				mosh_error(
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
				mosh_warn(
					"RAI is requested but RAI is disabled.\n"
					"Use 'link rai' command to enable it for ping usage.");
			}
			break;
			}
		case 'h': /* help */
		default:
			goto show_usage;
		}
	}

	/* Check that all mandatory args were given: */
	if (ping_args.target_name == NULL) {
		mosh_error("-d destination, MUST be given. See usage:");
		goto show_usage;
	}
	/* All good for args, get the current connection info and start the ping: */
	int ret = 0;
	struct pdp_context_info_array pdp_context_info_tbl;

	ret = link_api_pdp_contexts_read(&pdp_context_info_tbl);
	if (ret) {
		mosh_error("cannot read current connection info: %d", ret);
		goto show_usage;
	}

	if (pdp_context_info_tbl.size > 0) {
		/* Default context: */
		if (ping_args.cid == MOSH_ARG_NOT_SET) {
			ping_args.current_pdp_type = pdp_context_info_tbl.array[0].pdp_type;
			ping_args.current_addr4 = pdp_context_info_tbl.array[0].ip_addr4;
			ping_args.current_addr6 = pdp_context_info_tbl.array[0].ip_addr6;
			if (pdp_context_info_tbl.array[0].ipv4_mtu != 0) {
				ping_args.mtu =	pdp_context_info_tbl.array[0].ipv4_mtu;
			} else {
				ping_args.mtu = ICMP_DEFAULT_LINK_MTU;
			}
		} else {
			bool found = icmp_ping_shell_set_ping_args_according_to_pdp_ctx(
				&ping_args, &pdp_context_info_tbl);

			if (!found) {
				mosh_error("cannot find CID: %d", ping_args.cid);
				return -1;
			}
		}
	} else {
		mosh_error("cannot read current connection info");
		return -1;
	}
	if (pdp_context_info_tbl.array != NULL) {
		free(pdp_context_info_tbl.array);
	}

	/* Now we can check the max payload len vs link MTU (IPv6 check later): */
	uint32_t ipv4_max_payload_len = ping_args.mtu - ICMP_IPV4_HDR_LEN - ICMP_HDR_LEN;

	if (!ping_args.force_ipv6 && ping_args.len > ipv4_max_payload_len) {
		mosh_warn(
			"Payload size exceeds the link limits: MTU %d - headers %d = %d ",
			ping_args.mtu,
			(ICMP_IPV4_HDR_LEN - ICMP_HDR_LEN),
			ipv4_max_payload_len);
		/* Execute ping anyway */
	}

	return icmp_ping_start(&ping_args);

show_usage:
	icmp_ping_shell_usage_print();
	return -1;
}
