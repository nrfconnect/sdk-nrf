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

enum sms_command {
	SMS_CMD_REGISTER = 0,
	SMS_CMD_UNREGISTER,
	SMS_CMD_SEND,
	SMS_CMD_RECV
};

static const char sms_usage_str[] =
	"Usage: sms <command> [options]\n"
	"\n"
	"<command> is one of the following:\n"
	"  send:    Send SMS message. Also registers SMS service if required.\n"
	"           Mandatory options: -m, -n\n"
	"  recv:    Record number of received SMS messages.\n"
	"  reg:     Register SMS service to be able to receive messages.\n"
	"  unreg:   Unregister SMS service after which messages cannot be received.\n"
	"\n"
	"Options for 'send' command:\n"
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
	"\n"
	"Options for 'recv' command:\n"
	"  -r, --start,         Reset SMS counter to zero\n";

/* Specifying the expected options (both long and short): */
static struct option long_options[] = {
	{ "message",    required_argument, 0, 'm' },
	{ "number",     required_argument, 0, 'n' },
	{ "start",      no_argument,       0, 'r' },
	{ "type",       no_argument,       0, 't' },
	{ "help",       no_argument,       0, 'h' },
	{ 0,            0,                 0, 0   }
};

static void sms_print_usage(void)
{
	mosh_print_no_format(sms_usage_str);
}

static int sms_shell(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;
	enum sms_command command;

	if (argc < 2) {
		sms_print_usage();
		return 0;
	}

	/* Command = argv[1] */
	if (!strcmp(argv[1], "reg")) {
		command = SMS_CMD_REGISTER;
	} else if (!strcmp(argv[1], "unreg")) {
		command = SMS_CMD_UNREGISTER;
	} else if (!strcmp(argv[1], "send")) {
		command = SMS_CMD_SEND;
	} else if (!strcmp(argv[1], "recv")) {
		command = SMS_CMD_RECV;
	} else {
		if (strcmp(argv[1], "-h") != 0 && strcmp(argv[1], "--help") != 0) {
			mosh_error("Unsupported command=%s\n", argv[1]);
		}
		sms_print_usage();
		return -EINVAL;
	}
	/* Set getopt command line parsing index to skip commands */
	optind = 2;

	/* Variables for command line arguments */
	char arg_number[SMS_MAX_MESSAGE_LEN + 1];
	char arg_message[SMS_MAX_MESSAGE_LEN + 1];
	bool arg_receive_start = false;
	enum sms_data_type arg_message_type = SMS_DATA_TYPE_ASCII;

	memset(arg_number, 0, SMS_MAX_MESSAGE_LEN + 1);
	memset(arg_message, 0, SMS_MAX_MESSAGE_LEN + 1);

	/* Parse command line */
	int flag = 0;

	while ((flag = getopt_long(argc, argv, "m:n:rt:h", long_options, NULL)) != -1) {
		int send_data_len = 0;

		switch (flag) {
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
		case 'r': /* Start monitoring received messages */
			arg_receive_start = true;
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

	/* Run given command with it's arguments */
	switch (command) {
	case SMS_CMD_REGISTER:
		err = sms_register();
		break;
	case SMS_CMD_UNREGISTER:
		err = sms_unregister();
		break;
	case SMS_CMD_SEND:
		err = sms_send_msg(arg_number, arg_message, arg_message_type);
		break;
	case SMS_CMD_RECV:
		err = sms_recv(arg_receive_start);
		break;
	default:
		mosh_error("Internal error. Unknown sms command=%d", command);
		err = -EINVAL;
		break;
	}

	return err;

show_usage:
	/* Reset getopt for another users */
	optreset = 1;

	sms_print_usage();
	return err;
}

SHELL_CMD_REGISTER(sms, NULL, "Commands for sending and receiving SMS.", sms_shell);
