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
#include "ppp_shell.h"

#include "mosh_print.h"

enum ppp_shell_command { PPP_CMD_UP = 0, PPP_CMD_DOWN, PPP_CMD_UARTCONF };

enum ppp_shell_common_options { PPP_COMMON_NONE = 0, PPP_COMMON_READ };

struct ppp_shell_cmd_args_t {
	enum ppp_shell_command command;
	enum ppp_shell_common_options common_option;
	uint32_t baudrate;

	bool parity_set;
	enum uart_config_parity parity;

	bool stop_bits_set;
	enum uart_config_stop_bits stop_bits;

	bool data_bits_set;
	enum uart_config_data_bits data_bits;

	bool flow_ctrl_set;
	enum uart_config_flow_control flow_ctrl;
};

static struct ppp_shell_cmd_args_t ppp_cmd_args;

static const char ppp_cmd_usage_str[] =
	"Usage: ppp <sub-command>\n"
	"\n"
	"where <command> is one of the following:\n"
	"  help:                    Show this message\n"
	"  uartconf:                Configure or read PPP UART configuration\n"
	"  up:                      Set PPP net_if up (no options)\n"
	"  down:                    Set PPP net_if down (no options)\n"
	"\n";

static const char ppp_uartconf_usage_str[] =
	"Usage: ppp uartconf <options>\n"
	"Options:\n"
	"  -r, --read,            Read currrent PPP uart configuration\n"
	"  -b, --baudrate,        Set baudrate (e.g. 115200, 230400, 460800 or 921600)\n"
	"  -f, --flowctrl,        Set flow control method (\"none\" or \"rts_cts\")\n"
	"  -d, --databits,        Set databits ('7' or '8')\n"
	"  -s, --stopbits,        Set stopbits ('1' or '2')\n"
	"  -p, --parity,          Set parity (\"none\" or \"odd\" or \"even\")\n";

/* Specifying the expected options (both long and short): */
static struct option long_options[] = { { "baudrate", required_argument, 0, 'b' },
					{ "databits", required_argument, 0, 'd' },
					{ "stopbits", required_argument, 0, 's' },
					{ "parity", required_argument, 0, 'p' },
					{ "flowctrl", required_argument, 0, 'f' },
					{ "read", no_argument, 0, 'r' },
					{ 0, 0, 0, 0 } };

static void ppp_shell_print_usage(struct ppp_shell_cmd_args_t *ppp_cmd_args)
{
	switch (ppp_cmd_args->command) {
	case PPP_CMD_UARTCONF:
		mosh_print_no_format(ppp_uartconf_usage_str);
		break;
	default:
		mosh_print_no_format(ppp_cmd_usage_str);
		break;
	}
}

static void ppp_shell_cmd_defaults_set(struct ppp_shell_cmd_args_t *ppp_cmd_args)
{
	memset(ppp_cmd_args, 0, sizeof(struct ppp_shell_cmd_args_t));
}

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

int ppp_shell_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	bool uartconf_option_given = false;

	ppp_shell_cmd_defaults_set(&ppp_cmd_args);

	if (argc < 2) {
		goto show_usage;
	}

	if (strcmp(argv[1], "up") == 0) {
		ppp_cmd_args.command = PPP_CMD_UP;
	} else if (strcmp(argv[1], "down") == 0) {
		ppp_cmd_args.command = PPP_CMD_DOWN;
	} else if (strcmp(argv[1], "uartconf") == 0) {
		ppp_cmd_args.command = PPP_CMD_UARTCONF;
	} else if (strcmp(argv[1], "help") == 0) {
		goto show_usage;
	} else {
		mosh_error("Unsupported command=%s\n", argv[1]);
		ret = -EINVAL;
		goto show_usage;
	}

	/* Reset getopt due to possible previous failures: */
	optreset = 1;

	/* We start from subcmd arguments */
	optind = 2;

	int long_index = 0;
	int opt;
	struct uart_config ppp_uart_config;

	while ((opt = getopt_long(argc, argv, "b:d:s:p:f:r", long_options, &long_index)) != -1) {
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
				ppp_cmd_args.baudrate = ret;
				uartconf_option_given = true;
				break;
			default:
				mosh_error("Unsupported baudrate");
				return -EINVAL;
			}
			break;
		case 'd':
			ret = atoi(optarg);
			ppp_cmd_args.data_bits_set = true;
			uartconf_option_given = true;
			switch (ret) {
			case 7:
				ppp_cmd_args.data_bits = UART_CFG_DATA_BITS_7;
				break;
			case 8:
				ppp_cmd_args.data_bits = UART_CFG_DATA_BITS_8;
				break;
			default:
				mosh_error("Unsupported data bits config");
				ppp_cmd_args.data_bits_set = false;
				return -EINVAL;
			}
			break;
		case 's':
			ret = atoi(optarg);
			ppp_cmd_args.stop_bits_set = true;
			uartconf_option_given = true;
			switch (ret) {
			case 1:
				ppp_cmd_args.stop_bits = UART_CFG_STOP_BITS_1;
				uartconf_option_given = true;
				break;
			case 2:
				ppp_cmd_args.stop_bits = UART_CFG_STOP_BITS_2;
				uartconf_option_given = true;
				break;
			default:
				mosh_error("Unsupported stop bits config");
				ppp_cmd_args.stop_bits_set = false;
				return -EINVAL;
			}
			break;
		case 'p':
			ret = atoi(optarg);
			ppp_cmd_args.parity_set = true;
			uartconf_option_given = true;
			if (strcmp(optarg, "none") == 0) {
				ppp_cmd_args.parity = UART_CFG_PARITY_NONE;
			} else if (strcmp(optarg, "odd") == 0) {
				ppp_cmd_args.parity = UART_CFG_PARITY_ODD;
			} else if (strcmp(optarg, "even") == 0) {
				ppp_cmd_args.parity = UART_CFG_PARITY_EVEN;
			} else {
				mosh_error("Unsupported parity config");
				ppp_cmd_args.parity_set = false;
				return -EINVAL;
			}
			break;
		case 'f':
			ret = atoi(optarg);
			ppp_cmd_args.flow_ctrl_set = true;
			uartconf_option_given = true;
			if (strcmp(optarg, "none") == 0) {
				ppp_cmd_args.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
			} else if (strcmp(optarg, "rts_cts") == 0 ||
				   strcmp(optarg, "RTS_CTS") == 0) {
				ppp_cmd_args.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
			} else {
				mosh_error("Unsupported flow ctrl config");
				ppp_cmd_args.flow_ctrl_set = false;
				return -EINVAL;
			}
			break;
		case 'r':
			ppp_cmd_args.common_option = PPP_COMMON_READ;
			break;

		default:
			mosh_error("Unknown option. See usage:");
			goto show_usage;
		}
	}

	switch (ppp_cmd_args.command) {
	case PPP_CMD_UP:
		ret = ppp_ctrl_start();
		if (ret >= 0) {
			mosh_print("PPP up.");
		} else {
			mosh_error("PPP cannot be started");
		}

		break;
	case PPP_CMD_DOWN:
		ppp_ctrl_stop();
		break;
	case PPP_CMD_UARTCONF:
		/* Read current config: */
		ret = ppp_uart_settings_read(&ppp_uart_config);
		if (ret) {
			mosh_error("Cannot get current PPP uart config, ret %d", ret);
			return ret;
		}
		if (ppp_cmd_args.common_option == PPP_COMMON_READ) {
			ppp_shell_uart_conf_print(&ppp_uart_config);
		} else if (!uartconf_option_given) {
			mosh_error("No option given. See usage:");
			goto show_usage;
		} else {
			if (ppp_cmd_args.baudrate) {
				ppp_uart_config.baudrate = ppp_cmd_args.baudrate;
			}
			if (ppp_cmd_args.flow_ctrl_set) {
				ppp_uart_config.flow_ctrl = ppp_cmd_args.flow_ctrl;
			}
			if (ppp_cmd_args.data_bits_set) {
				ppp_uart_config.data_bits = ppp_cmd_args.data_bits;
			}
			if (ppp_cmd_args.stop_bits_set) {
				ppp_uart_config.stop_bits = ppp_cmd_args.stop_bits;
			}
			if (ppp_cmd_args.parity_set) {
				ppp_uart_config.parity = ppp_cmd_args.parity;
			}

			/* Save UART configuration to settings and to device: */
			ret = ppp_uart_settings_write(&ppp_uart_config);
			if (ret != 0) {
				mosh_error("Cannot write PPP uart config: %d", ret);
				return ret;
			}
			mosh_print("UART config updated");
		}
		break;
	}

	return 0;

show_usage:
	ppp_shell_print_usage(&ppp_cmd_args);
	return 0;
}
