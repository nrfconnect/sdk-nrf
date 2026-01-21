/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief WiFi pre-silicon testing shell
 */

#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(pocap_mode, CONFIG_LOG_DEFAULT_LEVEL);

static int cmd_wifi_pocap(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t rx_mode = 0, tx_mode = 0, rx_holdoff = 0, rx_wrap = 0, b2b_mode = 0;
	int opt;

	optind = 1;

	while ((opt = getopt(argc, argv, "r:t:h:w:b:")) != -1) {
		switch (opt) {
		case 'r':
			rx_mode = (uint32_t)atoi(optarg);
			break;
		case 't':
			tx_mode = (uint32_t)atoi(optarg);
			break;
		case 'h':
			rx_holdoff = (uint32_t)atoi(optarg);
			break;
		case 'w':
			rx_wrap = (uint32_t)atoi(optarg);
			break;
		case 'b':
			b2b_mode = (uint32_t)atoi(optarg);
			break;
		default:
			shell_print(
				sh,
				"Usage: wifi_pocap [-b b2b_mode] [other opts]\n"
				"b2b_mode=0: -r rx_mode -t tx_mode -h rx_holdoff_len -w "
				"rx_wrap_len\n"
				"Defaults: rx_mode=0 tx_mode=0 rx_holdoff=0 rx_wrap=0 b2b_mode=0");
			return -EINVAL;
		}
	}

	if (b2b_mode) {
		shell_print(sh, "Config: back_to_back_mode=%u (others ignored in B2B mode)",
			    b2b_mode);
		configure_playout_capture(0, 0, 0, 0, b2b_mode);
	} else {
		shell_print(sh, "Config: rx=%u tx=%u holdoff=%u wrap=%u b2b=%u", rx_mode, tx_mode,
			    rx_holdoff, rx_wrap, b2b_mode);
		configure_playout_capture(rx_mode, tx_mode, rx_holdoff, rx_wrap, 0);
	}

	return 0;
}

SHELL_CMD_REGISTER(wifi_pocap, NULL, "Configure RF Playout Capture", cmd_wifi_pocap);
