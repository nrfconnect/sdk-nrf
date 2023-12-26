/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Raw Tx Packet shell
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(raw_tx_pkt, CONFIG_LOG_DEFAULT_LEVEL);

#include "net_private.h"
#include <nrf_wifi/fw_if/umac_if/inc/default/fmac_structs.h>

struct raw_tx_pkt_header raw_tx_pkt;

void fill_raw_tx_pkt_hdr(int rate_flags, int data_rate, int queue_num)
{
	raw_tx_pkt.magic_num = NRF_WIFI_MAGIC_NUM_RAWTX;
	raw_tx_pkt.data_rate = data_rate;
	raw_tx_pkt.packet_length = 0;
	raw_tx_pkt.tx_mode = rate_flags;
	raw_tx_pkt.queue = queue_num;
	raw_tx_pkt.raw_tx_flag = 0;
}

int validate(int value, int min, int max, const char *param)
{
	if (value < min || value > max) {
		LOG_ERR("Please provide %s value between %d and %d", param, min, max);
		return 0;
	}

	return 1;
}

int validate_rate(int data_rate, int flag)
{
	if ((flag == 0) && ((data_rate == 1) ||
			   (data_rate == 2) ||
			   (data_rate == 55) ||
			   (data_rate == 11) ||
			   (data_rate == 6) ||
			   (data_rate == 9) ||
			   (data_rate == 12) ||
			   (data_rate == 18) ||
			   (data_rate == 24) ||
			   (data_rate == 36) ||
			   (data_rate == 48) ||
			   (data_rate == 54))) {

		return 1;
	} else if (((flag >= 1 && flag <= 5)) && ((data_rate >= 0) && (data_rate <= 7))) {
		return 1;
	}

	LOG_ERR("Invalid Data rate");
	return 0;
}

static int parse_raw_tx_configure_args(const struct shell *sh,
				       size_t argc,
				       char *argv[],
				       int *flags, int *rate, int *queue)
{
	struct getopt_state *state;
	int opt;
	static struct option long_options[] = {{"rate-flags", required_argument, 0, 'f'},
					       {"data-rate", required_argument, 0, 'd'},
					       {"queue-number", required_argument, 0, 'q'},
					       {"help", no_argument, 0, 'h'},
					       {0, 0, 0, 0}};
	int opt_index = 0;
	int opt_num = 0;

	while ((opt = getopt_long(argc, argv, "f:d:q:h", long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'f':
			*flags = atoi(optarg);
			if (!validate(*flags, 0, 4, "Rate Flags")) {
				return -ENOEXEC;
			}
			opt_num++;
			break;
		case 'd':
			*rate = atoi(optarg);
			if (!validate_rate(*rate, *flags)) {
				return -ENOEXEC;
			}
			opt_num++;
			break;
		case 'q':
			*queue = atoi(optarg);
			if (!validate(*queue, 0, 4, "Queue Number")) {
				return -ENOEXEC;
			}
			opt_num++;
			break;
		case'h':
			shell_help(sh);
			opt_num++;
			break;
		case '?':
		default:
			LOG_ERR("Invalid option or option usage: %s",
				argv[opt_index + 1]);
			return -ENOEXEC;
		}
	}

	return opt_num;
}

static int cmd_configure_raw_tx_pkt(
		const struct shell *sh,
		size_t argc,
		char *argv[])
{
	int opt_num, rate_flags = -1, data_rate = -1, queue_num = -1;

	if (argc == 7) {
		opt_num = parse_raw_tx_configure_args(sh, argc, argv, &rate_flags,
						      &data_rate, &queue_num);
		if (opt_num < 0) {
			shell_help(sh);
			return -ENOEXEC;
		} else if (!opt_num) {
			LOG_ERR("No valid option(s) found");
			return -ENOEXEC;
		}
	}

	LOG_INF("Rate-flags: %d Data-rate:%d queue-num:%d",
		rate_flags, data_rate, queue_num);

	fill_raw_tx_pkt_hdr(rate_flags, data_rate, queue_num);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	raw_tx_cmds,
	SHELL_CMD_ARG(configure, NULL,
		      "Configure raw TX packet header\n"
		      "This command may be used to configure raw TX packet header\n"
		      "parameters:\n"
		      "[-f, --rate-flags] : Rate flag value.\n"
		      "[-d, --data-rate] : Data rate value.\n"
		      "[-q, --queue-number] : Queue number.\n"
		      "[-h, --help] : Print out the help for the configure command\n"
		      "Usage: raw_tx configure -f 1 -d 9 -q 1\n",
		      cmd_configure_raw_tx_pkt,
		      7, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(raw_tx,
		   &raw_tx_cmds,
		   "raw_tx_cmds (To configure and send raw TX packets)",
		   NULL);
