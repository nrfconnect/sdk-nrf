/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/shell/shell.h>
#include <unistd.h>
#include <getopt.h>

#include <zephyr/net/promiscuous.h>
#include <zephyr/drivers/uart.h>

#include "ppp_ctrl.h"
#include "ppp_settings.h"

#include "mosh_print.h"

static const char ppp_uartconf_usage_str[] =
	"Usage: ppp uartconf [-r] [-b <number>] [-f <method>]\n"
	"       [-d <bits>] [-s <bits>] [-p <parity>]\n"
	"Options:\n"
	"  -r, --read,            Read currrent PPP uart configuration\n"
	"  -b, --baudrate,        Set baudrate (e.g. 115200, 230400, 460800 or 921600)\n"
	"  -f, --flowctrl,        Set flow control method (\"none\" or \"rts_cts\")\n"
	"  -d, --databits,        Set databits ('7' or '8')\n"
	"  -s, --stopbits,        Set stopbits ('1' or '2')\n"
	"  -p, --parity,          Set parity (\"none\" or \"odd\" or \"even\")\n"
	"  -h, --help,            Shows this help information";

/* Specifying the expected options (both long and short) */
static struct option long_options[] = { { "baudrate", required_argument, 0, 'b' },
					{ "databits", required_argument, 0, 'd' },
					{ "stopbits", required_argument, 0, 's' },
					{ "parity", required_argument, 0, 'p' },
					{ "flowctrl", required_argument, 0, 'f' },
					{ "read", no_argument, 0, 'r' },
					{ "help", no_argument, 0, 'h' },
					{ 0, 0, 0, 0 } };

static void ppp_shell_uart_conf_print(struct uart_config *ppp_uart_config)
{
	char tmp_str[64];

	mosh_print("PPP uart configuration:");
	mosh_print("  baudrate:     %d", ppp_uart_config->baudrate);

	if (ppp_uart_config->flow_ctrl == UART_CFG_FLOW_CTRL_NONE) {
		sprintf(tmp_str, "%s", "none");
	} else if (ppp_uart_config->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		sprintf(tmp_str, "%s", "RTS_CTS");
	} else if (ppp_uart_config->flow_ctrl == UART_CFG_FLOW_CTRL_DTR_DSR) {
		sprintf(tmp_str, "%s", "DTR_DSR");
	} else {
		sprintf(tmp_str, "%s", "unknown");
	}
	mosh_print("  flow control: %s", tmp_str);

	if (ppp_uart_config->parity == UART_CFG_PARITY_NONE) {
		sprintf(tmp_str, "%s", "none");
	} else if (ppp_uart_config->parity == UART_CFG_PARITY_ODD) {
		sprintf(tmp_str, "%s", "odd");
	} else if (ppp_uart_config->parity == UART_CFG_PARITY_EVEN) {
		sprintf(tmp_str, "%s", "even");
	} else if (ppp_uart_config->parity == UART_CFG_PARITY_MARK) {
		sprintf(tmp_str, "%s", "mark");
	} else if (ppp_uart_config->parity == UART_CFG_PARITY_SPACE) {
		sprintf(tmp_str, "%s", "space");
	} else {
		sprintf(tmp_str, "%s", "unknown");
	}
	mosh_print("  parity:       %s", tmp_str);

	if (ppp_uart_config->data_bits == UART_CFG_DATA_BITS_5) {
		sprintf(tmp_str, "%s", "bits5");
	} else if (ppp_uart_config->data_bits == UART_CFG_DATA_BITS_6) {
		sprintf(tmp_str, "%s", "bits6");
	} else if (ppp_uart_config->data_bits == UART_CFG_DATA_BITS_7) {
		sprintf(tmp_str, "%s", "bits7");
	} else if (ppp_uart_config->data_bits == UART_CFG_DATA_BITS_8) {
		sprintf(tmp_str, "%s", "bits8");
	} else if (ppp_uart_config->data_bits == UART_CFG_DATA_BITS_9) {
		sprintf(tmp_str, "%s", "bits9");
	} else {
		sprintf(tmp_str, "%s", "unknown");
	}
	mosh_print("  data bits:    %s", tmp_str);

	if (ppp_uart_config->stop_bits == UART_CFG_STOP_BITS_0_5) {
		sprintf(tmp_str, "%s", "bits0_5");
	} else if (ppp_uart_config->stop_bits == UART_CFG_STOP_BITS_1) {
		sprintf(tmp_str, "%s", "bits1");
	} else if (ppp_uart_config->stop_bits == UART_CFG_STOP_BITS_1_5) {
		sprintf(tmp_str, "%s", "bits1_5");
	} else if (ppp_uart_config->stop_bits == UART_CFG_STOP_BITS_2) {
		sprintf(tmp_str, "%s", "bits2");
	} else {
		sprintf(tmp_str, "%s", "unknown");
	}
	mosh_print("  stop bits:    %s", tmp_str);
}

static int cmd_ppp_up(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;

	ret = ppp_ctrl_start();
	if (ret >= 0) {
		mosh_print("PPP up.");
	} else {
		mosh_error("PPP cannot be started");
	}

	return 0;
}

static int cmd_ppp_down(const struct shell *shell, size_t argc, char **argv)
{
	ppp_ctrl_stop();

	return 0;
}

static int cmd_ppp_uartconf(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	bool uartconf_option_given = false;
	bool uart_config_read = false;
	struct uart_config ppp_uart_config;

	uint32_t baudrate = 0;
	bool parity_set = false;
	enum uart_config_parity parity;
	bool stop_bits_set = false;
	enum uart_config_stop_bits stop_bits;
	bool data_bits_set = false;
	enum uart_config_data_bits data_bits;
	bool flow_ctrl_set = false;
	enum uart_config_flow_control flow_ctrl;

	if (argc < 2) {
		goto show_usage;
	}

	optreset = 1;
	optind = 1;
	int opt;

	while ((opt = getopt_long(argc, argv, "b:d:s:p:f:rh", long_options, NULL)) != -1) {
		switch (opt) {
		case 'b':
			ret = atoi(optarg);
			switch (ret) {
			case 1200:
			case 2400:
			case 4800:
			case 9600:
			case 14400:
			case 19200:
			case 38400:
			case 57600:
			case 115200:
			case 230400:
			case 460800:
			case 921600:
			case 1000000:
				baudrate = ret;
				uartconf_option_given = true;
				break;
			default:
				mosh_error("Unsupported baudrate");
				return -EINVAL;
			}
			break;
		case 'd':
			ret = atoi(optarg);
			data_bits_set = true;
			uartconf_option_given = true;
			switch (ret) {
			case 7:
				data_bits = UART_CFG_DATA_BITS_7;
				break;
			case 8:
				data_bits = UART_CFG_DATA_BITS_8;
				break;
			default:
				mosh_error("Unsupported data bits config");
				data_bits_set = false;
				return -EINVAL;
			}
			break;
		case 's':
			ret = atoi(optarg);
			stop_bits_set = true;
			uartconf_option_given = true;
			switch (ret) {
			case 1:
				stop_bits = UART_CFG_STOP_BITS_1;
				uartconf_option_given = true;
				break;
			case 2:
				stop_bits = UART_CFG_STOP_BITS_2;
				uartconf_option_given = true;
				break;
			default:
				mosh_error("Unsupported stop bits config");
				stop_bits_set = false;
				return -EINVAL;
			}
			break;
		case 'p':
			ret = atoi(optarg);
			parity_set = true;
			uartconf_option_given = true;
			if (strcmp(optarg, "none") == 0) {
				parity = UART_CFG_PARITY_NONE;
			} else if (strcmp(optarg, "odd") == 0) {
				parity = UART_CFG_PARITY_ODD;
			} else if (strcmp(optarg, "even") == 0) {
				parity = UART_CFG_PARITY_EVEN;
			} else {
				mosh_error("Unsupported parity config");
				parity_set = false;
				return -EINVAL;
			}
			break;
		case 'f':
			ret = atoi(optarg);
			flow_ctrl_set = true;
			uartconf_option_given = true;
			if (strcmp(optarg, "none") == 0) {
				flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
			} else if (strcmp(optarg, "rts_cts") == 0 ||
				   strcmp(optarg, "RTS_CTS") == 0) {
				flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
			} else {
				mosh_error("Unsupported flow ctrl config");
				flow_ctrl_set = false;
				return -EINVAL;
			}
			break;
		case 'r':
			uart_config_read = true;
			break;

		case '?':
			mosh_error("Unknown option. See usage:");
			goto show_usage;
		case 'h':
		default:
			goto show_usage;
		}
	}

	/* Read current config */
	ret = ppp_uart_settings_read(&ppp_uart_config);
	if (ret) {
		mosh_error("Cannot get current PPP uart config, ret %d", ret);
		return ret;
	}
	if (uart_config_read) {
		ppp_shell_uart_conf_print(&ppp_uart_config);
	} else if (!uartconf_option_given) {
		mosh_error("No option given. See usage:");
		goto show_usage;
	} else {
		if (baudrate > 0) {
			ppp_uart_config.baudrate = baudrate;
		}
		if (flow_ctrl_set) {
			ppp_uart_config.flow_ctrl = flow_ctrl;
		}
		if (data_bits_set) {
			ppp_uart_config.data_bits = data_bits;
		}
		if (stop_bits_set) {
			ppp_uart_config.stop_bits = stop_bits;
		}
		if (parity_set) {
			ppp_uart_config.parity = parity;
		}

		/* Save UART configuration to settings and to device */
		ret = ppp_uart_settings_write(&ppp_uart_config);
		if (ret != 0) {
			mosh_error("Cannot write PPP uart config: %d", ret);
			return ret;
		}
		mosh_print("UART config updated");
	}

	return 0;

show_usage:
	mosh_print_no_format(ppp_uartconf_usage_str);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_ppp,
	SHELL_CMD_ARG(
		up, NULL,
		"Set PPP net_if up (no options).",
		cmd_ppp_up, 1, 0),
	SHELL_CMD_ARG(
		down, NULL,
		"Set PPP net_if down (no options).",
		cmd_ppp_down, 1, 0),
	SHELL_CMD_ARG(
		uartconf, NULL,
		"Configure or read PPP UART configuration.",
		cmd_ppp_uartconf, 1, 20),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ppp, &sub_ppp, "Commands for controlling PPP.", mosh_print_help_shell);
