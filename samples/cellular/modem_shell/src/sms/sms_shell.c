/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#ifdef CONFIG_GETOPT
#include <zephyr/posix/unistd.h>
#endif
#include <getopt.h>
#include <modem/sms.h>

#include "sms.h"
#include "mosh_defines.h"
#include "mosh_print.h"

/* Maximum length of the message data that can be specified with -m option */
#define SMS_MAX_MESSAGE_LEN CONFIG_SHELL_CMD_BUFF_SIZE

enum sms_shell_command {
	SMS_CMD_SEND = 0,
	SMS_CMD_RECV
};

static const char sms_send_usage_str[] =
	"Usage: sms send -n <number> -m <message> [-t <type>]\n"
	"\n"
	"Options:\n"
	"  -m, --message, <str> Text to be sent.\n"
	"  -n, --number, <str>  Number in international format including country number.\n"
	"                       Leading '+' can be present or left out.\n"
	"  -t, --type, <str>    The type, format or character set of given message (-m):\n"
	"                       'ascii' or 'gsm7bit'. By default, the type is 'ascii'.\n"
	"                       'ascii':\n"
	"                         Each character is treated as ASCII string with\n"
	"                         ISO-8859-15 extension.\n"
	"                       'gsm7bit':\n"
	"                         String of hexadecimal characters where each pair of two\n"
	"                         characters form a single byte. Those bytes are treated as\n"
	"                         GSM 7 bit Default Alphabet as specified in 3GPP TS 23.038\n"
	"                         Section 6.2. Any spaces will be removed before processing.\n"
	"                         Examples of equivalent hexadecimal data strings:\n"
	"                             010203040506070809101112\n"
	"                             01 02 03 04 05 06 07 08 09 10 11 12\n"
	"                             01020304 05060708 09101112\n"
	"  -h, --help,          Shows this help information";

static const char sms_recv_usage_str[] =
	"Usage: sms recv [-r]\n"
	"\n"
	"Options for 'recv' command:\n"
	"  -r, --start,         Reset SMS counter to zero\n"
	"  -h, --help,          Shows this help information";

/* Specifying the expected options (both long and short) */
static struct option long_options[] = {
	{ "message",    required_argument, 0, 'm' },
	{ "number",     required_argument, 0, 'n' },
	{ "start",      no_argument,       0, 'r' },
	{ "type",       no_argument,       0, 't' },
	{ "help",       no_argument,       0, 'h' },
	{ 0,            0,                 0, 0   }
};

static void sms_print_usage(enum sms_shell_command command)
{
	switch (command) {
	case SMS_CMD_SEND:
		mosh_print_no_format(sms_send_usage_str);
		break;
	case SMS_CMD_RECV:
		mosh_print_no_format(sms_recv_usage_str);
		break;
	default:
		break;
	}
}

static int cmd_sms_send(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;

	if (argc < 2) {
		goto show_usage;
	}

	/* Variables for command line arguments */
	char arg_number[SMS_MAX_MESSAGE_LEN + 1];
	char arg_message[SMS_MAX_MESSAGE_LEN + 1];
	enum sms_data_type arg_message_type = SMS_DATA_TYPE_ASCII;

	memset(arg_number, 0, SMS_MAX_MESSAGE_LEN + 1);
	memset(arg_message, 0, SMS_MAX_MESSAGE_LEN + 1);

	optreset = 1;
	optind = 1;
	int opt;

	while ((opt = getopt_long(argc, argv, "m:n:t:h", long_options, NULL)) != -1) {
		int send_data_len = 0;

		switch (opt) {
		case 'n': /* Phone number */
			strcpy(arg_number, optarg);
			break;
		case 'm': /* Message text */
			send_data_len = strlen(optarg);
			if (send_data_len > SMS_MAX_MESSAGE_LEN) {
				mosh_error(
					"Data length %d exceeded. Maximum is %d. Given data: %s",
					send_data_len,
					SMS_MAX_MESSAGE_LEN,
					optarg);
				return -EINVAL;
			}
			strcpy(arg_message, optarg);
			break;
		case 't': /* Message data type */
			if (!strcmp(optarg, "ascii")) {
				arg_message_type = SMS_DATA_TYPE_ASCII;
			} else if (!strcmp(optarg, "gsm7bit")) {
				arg_message_type = SMS_DATA_TYPE_GSM7BIT;
			} else {
				mosh_error(
					"Unsupported message type=%s. Supported values are: "
					"'ascii' or 'gsm7bit'",
					optarg);
				return -EINVAL;
			}
			break;

		case '?':
			mosh_error("Unknown option. See usage:");
			goto show_usage;
		case 'h':
		default:
			goto show_usage;
		}
	}

	err = sms_send_msg(arg_number, arg_message, arg_message_type);

	return err;

show_usage:
	sms_print_usage(SMS_CMD_SEND);
	return err;
}

static int cmd_sms_recv(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;

	/* Variables for command line arguments */
	bool arg_receive_start = false;

	optreset = 1;
	optind = 1;
	int opt;

	while ((opt = getopt_long(argc, argv, "rh", long_options, NULL)) != -1) {

		switch (opt) {
		case 'r': /* Start monitoring received messages */
			arg_receive_start = true;
			break;
		case '?':
			mosh_error("Unknown option. See usage:");
			goto show_usage;
		case 'h':
		default:
			goto show_usage;
		}
	}

	err = sms_recv(arg_receive_start);

	return err;

show_usage:
	sms_print_usage(SMS_CMD_RECV);
	return err;
}

static int cmd_sms_reg(const struct shell *shell, size_t argc, char **argv)
{
	return sms_register();
}

static int cmd_sms_unreg(const struct shell *shell, size_t argc, char **argv)
{
	return sms_unregister();
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sms,
	SHELL_CMD_ARG(
		send, NULL,
		"Send SMS message. Also registers SMS service if required.",
		cmd_sms_send, 0, 20),
	SHELL_CMD_ARG(
		recv, NULL,
		"Record number of received SMS messages.",
		cmd_sms_recv, 0, 10),
	SHELL_CMD_ARG(
		reg, NULL,
		"Register SMS service to be able to receive messages.",
		cmd_sms_reg, 1, 0),
	SHELL_CMD_ARG(
		unreg, NULL,
		"Unregister SMS service after which messages cannot be received.",
		cmd_sms_unreg, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sms, &sub_sms, "Commands for sending and receiving SMS.", mosh_print_help_shell);
