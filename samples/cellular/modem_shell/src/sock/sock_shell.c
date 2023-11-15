/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <assert.h>
#include <stdio.h>
#if defined(CONFIG_POSIX_API)
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <fcntl.h>
#include <getopt.h>

#include "sock.h"
#include "mosh_defines.h"
#include "mosh_print.h"
#include "link_api.h"
#include "utils/net_utils.h"

/* Maximum length of the address */
#define SOCK_MAX_ADDR_LEN 100

enum sock_shell_command {
	SOCK_CMD_CONNECT = 0,
	SOCK_CMD_CLOSE,
	SOCK_CMD_SEND,
	SOCK_CMD_RECV,
	SOCK_CMD_RAI
};

static const char sock_connect_usage_str[] =
	"Usage: sock connect -a <address> -p <port>\n"
	"       [-f <family>] [-t <type>] [-b <port>] [-I <cid>]\n"
	"       [-S] [-T <sec_tag>] [-c] [-V <level>] [-H <hostname>]\n"
	"Options:\n"
	"  -a, --address, [str]      Address as ip address or hostname\n"
	"  -p, --port,  [int]        Port\n"
	"  -f, --family, [str]       Address family: 'inet' (ipv4, default),\n"
	"                            'inet6' (ipv6) or 'packet'\n"
	"  -t, --type, [str]         Address type: 'stream' (tcp, default),\n"
	"                            'dgram' (udp) or 'raw'\n"
	"  -b, --bind_port, [int]    Local port to bind the socket to\n"
	"  -I, --cid, [int]          Use this option to bind socket to specific\n"
	"                            PDN CID. See link command for available CIDs.\n"
	"  -S, --secure,             Enable secure connection (TLS 1.2/DTLS 1.2).\n"
	"  -T, --sec_tag, [int]      Security tag for TLS certificate(s).\n"
	"  -c, --cache,              Enable TLS session cache.\n"
	"  -V, --peer_verify, [int]  TLS peer verification level. None (0),\n"
	"                            optional (1) or required (2). Default value is 2.\n"
	"  -H, --hostname, [str]     Hostname for TLS peer verification.\n"
	"  -h, --help,               Shows this help information";

static const char sock_close_usage_str[] =
	"Usage: sock close -i <socket id>\n"
	"Options:\n"
	"  -i, --id, [int]           Socket id. Use 'sock list' command to see open\n"
	"                            sockets.\n"
	"  -h, --help,               Shows this help information";

static const char sock_send_usage_str[] =
	"Usage: sock send -i <socket id> [-d <data> | -l <data length>] [-e <interval>]\n"
	"       [--packet_number_prefix] [-B <blocking>] [-s <size>] [-x]\n"
	"Options:\n"
	"  -i, --id, [int]           Socket id. Use 'sock list' command to see open\n"
	"                            sockets.\n"
	"  -d, --data [str]          Data to be sent. Cannot be used with -l option.\n"
	"  -l, --length, [int]       Length of undefined data in bytes. This can be used\n"
	"                            when testing with bigger data amounts. Cannot be\n"
	"                            used with -d or -e option.\n"
	"  -e, --period, [int]       Data sending interval in seconds. You must also\n"
	"                            specify -d.\n"
	"      --packet_number_prefix\n"
	"                            Add a 4 digit packet number into the beginning of each\n"
	"                            packet.\n"
	"                            The number is between 0001..9999 and it can roll over.\n"
	"                            Must be used with -d and -e options and without\n"
	"                            -l and -x options.\n"
	"  -B, --blocking, [int]     Blocking (1) or non-blocking (0) mode. This is only\n"
	"                            valid when -l is given. Default value is 1.\n"
	"  -s, --buffer_size, [int]  Send buffer size. This is only valid when -l is\n"
	"                            given. Default value for 'stream' socket is 3540\n"
	"                            and for 'dgram' socket 1200.\n"
	"  -x, --hex,                Indicates that given data (-d) is in hexadecimal\n"
	"                            format. By default, the format is string.\n"
	"                            When this flag is set, given data (-d) is a string\n"
	"                            of hexadecimal characters and each pair of two\n"
	"                            characters form a single byte. Any spaces will be\n"
	"                            removed before processing.\n"
	"                            Examples of equivalent hexadecimal data strings:\n"
	"                                010203040506070809101112\n"
	"                                01 02 03 04 05 06 07 08 09 10 11 12\n"
	"                                01020304 05060708 09101112\n"
	"  -h, --help,               Shows this help information";

static const char sock_recv_usage_str[] =
	"Usage: sock recv -i <socket id> [-r] [-l <data_len>] [-B <blocking>] [-P <format>]\n"
	"Options:\n"
	"  -i, --id, [int]           Socket id. Use 'sock list' command to see open\n"
	"                            sockets.\n"
	"  -r, --start,              Initialize variables for receive throughput\n"
	"                            calculation\n"
	"  -l, --length, [int]       Length of expected data in bytes. After receiving\n"
	"                            data with given length, summary of data throughput\n"
	"                            is printed. Should be used with -r option.\n"
	"  -B, --blocking, [int]     Blocking (1) or non-blocking (0) mode. This only\n"
	"                            accounts when -r is given. Default value is 0.\n"
	"  -P, --print_format, [str] Set receive data print format: 'str' (default) or\n"
	"                            'hex'\n"
	"  -h, --help,               Shows this help information";

static const char sock_rai_usage_str[] =
	"Usage: sock rai -i <socket id> [--rai_last] [--rai_no_data] [--rai_one_resp]\n"
	"       [--rai_ongoing] [--rai_wait_more]\n"
	"Options:\n"
	"  -i, --id, [int]           Socket id. Use 'sock list' command to see open\n"
	"                            sockets.\n"
	"      --rai_last,           Sets NRF_SO_RAI_LAST option.\n"
	"                            Indicates that the next call to send/sendto will be\n"
	"                            the last one for some time, which means that the\n"
	"                            modem can get out of connected mode quicker when\n"
	"                            this data is sent.\n"
	"      --rai_no_data,        Sets NRF_SO_RAI_NO_DATA option.\n"
	"                            Indicates that the application will not send any\n"
	"                            more data. This socket option will apply\n"
	"                            immediately, and does not require a call to send\n"
	"                            afterwards.\n"
	"      --rai_one_resp,       Sets NRF_SO_RAI_ONE_RESP option.\n"
	"                            Indicates that after the next call to send/sendto,\n"
	"                            the application is expecting to receive one more\n"
	"                            data packet before this socket will not be used\n"
	"                            again for some time.\n"
	"      --rai_ongoing,        Sets NRF_SO_RAI_ONGOING option.\n"
	"                            If a client application expects to use the socket\n"
	"                            more it can indicate that by setting this socket\n"
	"                            option before the next send call which will keep\n"
	"                            the modem in connected mode longer.\n"
	"      --rai_wait_more,      Sets NRF_SO_RAI_WAIT_MORE option.\n"
	"                            If a server application expects to use the socket\n"
	"                            more it can indicate that by setting this socket\n"
	"                            option before the next send call.\n"
	"  -h, --help,               Shows this help information";

/* The following do not have short options */
#define SOCK_SHELL_OPT_RAI_LAST 200
#define SOCK_SHELL_OPT_RAI_NO_DATA 201
#define SOCK_SHELL_OPT_RAI_ONE_RESP 202
#define SOCK_SHELL_OPT_RAI_ONGOING 203
#define SOCK_SHELL_OPT_RAI_WAIT_MORE 204
#define SOCK_SHELL_OPT_PACKET_NUMBER_PREFIX 205

/* Specifying the expected options (both long and short) */
static struct option long_options[] = {
	{ "id",             required_argument, 0, 'i' },
	{ "cid",            required_argument, 0, 'I' },
	{ "address",        required_argument, 0, 'a' },
	{ "port",           required_argument, 0, 'p' },
	{ "family",         required_argument, 0, 'f' },
	{ "type",           required_argument, 0, 't' },
	{ "bind_port",      required_argument, 0, 'b' },
	{ "secure",         no_argument,       0, 'S' },
	{ "sec_tag",        required_argument, 0, 'T' },
	{ "cache",          no_argument,       0, 'c' },
	{ "peer_verify",    required_argument, 0, 'V' },
	{ "hostname",       required_argument, 0, 'H' },
	{ "data",           required_argument, 0, 'd' },
	{ "length",         required_argument, 0, 'l' },
	{ "period",         required_argument, 0, 'e' },
	{ "buffer_size",    required_argument, 0, 's' },
	{ "hex",            no_argument,       0, 'x' },
	{ "start",          no_argument,       0, 'r' },
	{ "blocking",       required_argument, 0, 'B' },
	{ "print_format",   required_argument, 0, 'P' },
	{ "packet_number_prefix", no_argument, 0, SOCK_SHELL_OPT_PACKET_NUMBER_PREFIX },
	{ "rai_last",       no_argument,       0, SOCK_SHELL_OPT_RAI_LAST },
	{ "rai_no_data",    no_argument,       0, SOCK_SHELL_OPT_RAI_NO_DATA },
	{ "rai_one_resp",   no_argument,       0, SOCK_SHELL_OPT_RAI_ONE_RESP },
	{ "rai_ongoing",    no_argument,       0, SOCK_SHELL_OPT_RAI_ONGOING },
	{ "rai_wait_more",  no_argument,       0, SOCK_SHELL_OPT_RAI_WAIT_MORE },
	{ "help",           no_argument,       0, 'h' },
	{ 0,                0,                 0, 0   }
};

static const char short_options[] = "i:I:a:p:f:t:b:ST:cV:H:d:l:e:s:xrB:P:h";

static void sock_print_usage(enum sock_shell_command command)
{
	switch (command) {
	case SOCK_CMD_CONNECT:
		mosh_print_no_format(sock_connect_usage_str);
		break;
	case SOCK_CMD_CLOSE:
		mosh_print_no_format(sock_close_usage_str);
		break;
	case SOCK_CMD_SEND:
		mosh_print_no_format(sock_send_usage_str);
		break;
	case SOCK_CMD_RECV:
		mosh_print_no_format(sock_recv_usage_str);
		break;
	case SOCK_CMD_RAI:
		mosh_print_no_format(sock_rai_usage_str);
		break;
	default:
		break;
	}
}

static int cmd_sock_connect(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;

	if (argc < 2) {
		goto show_usage;
	}

	/* Variables for command line arguments */
	int arg_family = AF_INET;
	int arg_type = SOCK_STREAM;
	char arg_address[SOCK_MAX_ADDR_LEN + 1];
	int arg_port = 0;
	int arg_bind_port = 0;
	int arg_pdn_cid = 0;
	bool arg_secure = false;
	int arg_sec_tag = -1;
	bool arg_session_cache = false;
	int arg_peer_verify = 2;
	char arg_peer_hostname[SOCK_MAX_ADDR_LEN + 1];

	memset(arg_address, 0, SOCK_MAX_ADDR_LEN + 1);
	memset(arg_peer_hostname, 0, SOCK_MAX_ADDR_LEN + 1);

	optreset = 1;
	optind = 1;
	int opt;

	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		int addr_len = 0;

		switch (opt) {
		case 'I': /* PDN CID */
			arg_pdn_cid = atoi(optarg);
			if (arg_pdn_cid <= 0) {
				mosh_error("PDN CID (%d) must be positive integer.", arg_pdn_cid);
				return -EINVAL;
			}
			break;
		case 'a': /* IP address, or hostname */
			addr_len = strlen(optarg);
			if (addr_len > SOCK_MAX_ADDR_LEN) {
				mosh_error(
					"Address length %d exceeded. Maximum is %d.",
					addr_len, SOCK_MAX_ADDR_LEN);
				return -EINVAL;
			}
			memcpy(arg_address, optarg, addr_len);
			break;
		case 'p': /* Port */
			arg_port = atoi(optarg);
			if (arg_port <= 0 || arg_port > 65535) {
				mosh_error(
					"Port (%d) must be bigger than 0 and smaller than 65536.",
					arg_port);
				return -EINVAL;
			}
			break;
		case 'f': /* Address family */
			if (!strcmp(optarg, "inet")) {
				arg_family = AF_INET;
			} else if (!strcmp(optarg, "inet6")) {
				arg_family = AF_INET6;
			} else if (!strcmp(optarg, "packet")) {
				arg_family = AF_PACKET;
			} else {
				mosh_error(
					"Unsupported address family=%s. Supported values are: "
					"'inet' (ipv4, default), 'inet6' (ipv6) or 'packet'",
					optarg);
				return -EINVAL;
			}
			break;
		case 't': /* Socket type */
			if (!strcmp(optarg, "stream")) {
				arg_type = SOCK_STREAM;
			} else if (!strcmp(optarg, "dgram")) {
				arg_type = SOCK_DGRAM;
			} else if (!strcmp(optarg, "raw")) {
				arg_type = SOCK_RAW;
			} else {
				mosh_error(
					"Unsupported address type=%s. Supported values are: "
					"'stream' (tcp, default), 'dgram' (udp) or 'raw'",
					optarg);
				return -EINVAL;
			}
			break;
		case 'b': /* Bind port */
			arg_bind_port = atoi(optarg);
			if (arg_bind_port <= 0 || arg_bind_port > 65535) {
				mosh_error(
					"Bind port (%d) must be bigger than 0 and "
					"smaller than 65536.",
					arg_bind_port);
				return -EINVAL;
			}
			break;
		case 'S': /* Security */
			arg_secure = true;
			break;
		case 'T': /* Security tag */
			arg_sec_tag = atoi(optarg);
			if (arg_sec_tag < 0) {
				mosh_error(
					"Valid range for security tag (%d) is 0 ... 2147483647.",
					arg_sec_tag);
				return -EINVAL;
			}
			break;
		case 'c': /* TLS session cache */
			arg_session_cache = true;
			break;
		case 'V': /* TLS peer verify */
			arg_peer_verify = atoi(optarg);
			if (arg_peer_verify < 0 || arg_peer_verify > 2) {
				mosh_error(
					"Valid values for peer verify (%d) are 0, 1 and 2.",
					arg_peer_verify);
				return -EINVAL;
			}
			break;
		case 'H': /* TLS peer hostname */
			addr_len = strlen(optarg);
			if (addr_len > SOCK_MAX_ADDR_LEN) {
				mosh_error(
					"Peer hostname length %d exceeded. Maximum is %d.",
					addr_len, SOCK_MAX_ADDR_LEN);
				return -EINVAL;
			}
			strcpy(arg_peer_hostname, optarg);
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}

	if (optind < argc) {
		mosh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	err = sock_open_and_connect(
		arg_family,
		arg_type,
		arg_address,
		arg_port,
		arg_bind_port,
		arg_pdn_cid,
		arg_secure,
		arg_sec_tag,
		arg_session_cache,
		arg_peer_verify,
		arg_peer_hostname);

	return err;

show_usage:
	sock_print_usage(SOCK_CMD_CONNECT);
	return err;
}

static int cmd_sock_close(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;

	if (argc < 2) {
		goto show_usage;
	}

	/* Variables for command line arguments */
	int arg_socket_id = SOCK_ID_NONE;

	optreset = 1;
	optind = 1;
	int opt;

	while ((opt = getopt_long(argc, argv, "i:h", long_options, NULL)) != -1) {

		switch (opt) {
		case 'i': /* Socket ID */
			arg_socket_id = atoi(optarg);
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}

	if (optind < argc) {
		mosh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	err = sock_close(arg_socket_id);

	return err;

show_usage:
	sock_print_usage(SOCK_CMD_CLOSE);
	return err;
}

static int cmd_sock_send(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;

	if (argc < 2) {
		goto show_usage;
	}

	/* Variables for command line arguments */
	int arg_socket_id = SOCK_ID_NONE;
	char arg_send_data[SOCK_MAX_SEND_DATA_LEN + 1];
	int arg_data_length = 0;
	int arg_data_interval = SOCK_SEND_DATA_INTERVAL_NONE;
	bool arg_packet_number_prefix = false;
	int arg_buffer_size = SOCK_BUFFER_SIZE_NONE;
	bool arg_data_format_hex = false;
	bool arg_blocking_send = true;
	bool arg_blocking_recv = false;

	memset(arg_send_data, 0, SOCK_MAX_SEND_DATA_LEN + 1);

	optreset = 1;
	optind = 1;
	int opt;

	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		int send_data_len = 0;

		switch (opt) {
		case 'i': /* Socket ID */
			arg_socket_id = atoi(optarg);
			break;
		case 'd': /* Data to be sent is available in send buffer */
			send_data_len = strlen(optarg);
			if (send_data_len > SOCK_MAX_SEND_DATA_LEN) {
				mosh_error(
					"Data length %d exceeded. Maximum is %d. Given data: %s",
					send_data_len, SOCK_MAX_SEND_DATA_LEN,
					optarg);
				return -EINVAL;
			}
			strcpy(arg_send_data, optarg);
			break;
		case 'x': /* Send data string is defined in hex format */
			arg_data_format_hex = true;
			break;
		case 'l': /* Length of data */
			arg_data_length = atoi(optarg);
			break;
		case 'e': /* Interval in which data will be sent */
			arg_data_interval = atoi(optarg);
			break;
		case 's': /* Buffer size */
			arg_buffer_size = atoi(optarg);
			if (arg_buffer_size <= 0) {
				mosh_error(
					"Buffer size %d must be a positive number",
					arg_buffer_size);
				return -EINVAL;
			}
			break;
		case 'B': /* Blocking/non-blocking send or receive */
		{
			int blocking = atoi(optarg);

			if (blocking != 0 && blocking != 1) {
				mosh_error(
					"Blocking (%d) must be either '0' (false) or '1' (true)",
					optarg);
				return -EINVAL;
			}
			arg_blocking_recv = blocking;
			arg_blocking_send = blocking;
			break;
		}
		case SOCK_SHELL_OPT_PACKET_NUMBER_PREFIX:
			arg_packet_number_prefix = true;
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}

	if (optind < argc) {
		mosh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	err = sock_send_data(
		arg_socket_id,
		arg_send_data,
		arg_data_length,
		arg_data_interval,
		arg_packet_number_prefix,
		arg_blocking_send,
		arg_buffer_size,
		arg_data_format_hex);

	return err;

show_usage:
	sock_print_usage(SOCK_CMD_SEND);
	return err;
}

static int cmd_sock_recv(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;

	if (argc < 2) {
		goto show_usage;
	}

	/* Variables for command line arguments */
	int arg_socket_id = SOCK_ID_NONE;
	int arg_data_length = 0;
	bool arg_receive_start = false;
	bool arg_blocking_send = true;
	bool arg_blocking_recv = false;
	enum sock_recv_print_format arg_recv_print_format = SOCK_RECV_PRINT_FORMAT_NONE;

	optreset = 1;
	optind = 1;
	int opt;

	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {

		switch (opt) {
		case 'i': /* Socket ID */
			arg_socket_id = atoi(optarg);
			break;
		case 'l': /* Length of data */
			arg_data_length = atoi(optarg);
			break;
		case 'r': /* Start monitoring received data */
			arg_receive_start = true;
			break;
		case 'B': /* Blocking/non-blocking send or receive */
		{
			int blocking = atoi(optarg);

			if (blocking != 0 && blocking != 1) {
				mosh_error(
					"Blocking (%d) must be either '0' (false) or '1' (true)",
					optarg);
				return -EINVAL;
			}
			arg_blocking_recv = blocking;
			arg_blocking_send = blocking;
			break;
		}
		case 'P': /* Receive data print format: "str" or "hex" */
			if (!strcmp(optarg, "str")) {
				arg_recv_print_format =
					SOCK_RECV_PRINT_FORMAT_STR;
			} else if (!strcmp(optarg, "hex")) {
				arg_recv_print_format =
					SOCK_RECV_PRINT_FORMAT_HEX;
			} else {
				mosh_error(
					"Receive data print format (%s) must be 'str' or 'hex'",
					optarg);
				return -EINVAL;
			}
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}

	if (optind < argc) {
		mosh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	err = sock_recv(
		arg_socket_id,
		arg_receive_start,
		arg_data_length,
		arg_blocking_recv,
		arg_recv_print_format);

	return err;

show_usage:
	sock_print_usage(SOCK_CMD_RECV);
	return err;
}

static int cmd_sock_rai(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;

	if (argc < 2) {
		goto show_usage;
	}

	/* Variables for command line arguments */
	int arg_socket_id = SOCK_ID_NONE;
	bool arg_rai_last = false;
	bool arg_rai_no_data = false;
	bool arg_rai_one_resp = false;
	bool arg_rai_ongoing = false;
	bool arg_rai_wait_more = false;

	optreset = 1;
	optind = 1;
	int opt;

	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {

		switch (opt) {
		case 'i': /* Socket ID */
			arg_socket_id = atoi(optarg);
			break;

		case SOCK_SHELL_OPT_RAI_LAST:
			arg_rai_last = true;
			break;
		case SOCK_SHELL_OPT_RAI_NO_DATA:
			arg_rai_no_data = true;
			break;
		case SOCK_SHELL_OPT_RAI_ONE_RESP:
			arg_rai_one_resp = true;
			break;
		case SOCK_SHELL_OPT_RAI_ONGOING:
			arg_rai_ongoing = true;
			break;
		case SOCK_SHELL_OPT_RAI_WAIT_MORE:
			arg_rai_wait_more = true;
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}

	if (optind < argc) {
		mosh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	bool rai_status = false;

	(void)link_api_rai_status(&rai_status);
	/* If RAI is disabled or reading it fails, show warning. It's only
	 * warning because RAI status may be out of sync if device hadn't gone
	 * to normal mode since changing it.
	 */
	if (!rai_status) {
		mosh_warn(
			"RAI is requested but RAI is disabled.\n"
			"Use 'link rai' command to enable it for socket usage.");
	}

	err = sock_rai(
		arg_socket_id,
		arg_rai_last,
		arg_rai_no_data,
		arg_rai_one_resp,
		arg_rai_ongoing,
		arg_rai_wait_more);

	return err;

show_usage:
	sock_print_usage(SOCK_CMD_RAI);
	return err;
}

static int cmd_sock_list(const struct shell *shell, size_t argc, char **argv)
{
	return sock_list();
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sock,
	SHELL_CMD_ARG(
		connect, NULL,
		"Open socket and connect to given host.",
		cmd_sock_connect, 0, 20),
	SHELL_CMD_ARG(
		close, NULL,
		"Close socket connection.",
		cmd_sock_close, 0, 10),
	SHELL_CMD_ARG(
		send, NULL,
		"Send data.",
		cmd_sock_send, 0, 30),
	SHELL_CMD_ARG(
		recv, NULL,
		"Initialize and query receive throughput metrics. Without -r option, "
		"returns current metrics so that can be used both as status request "
		"and final summary for receiving.",
		cmd_sock_recv, 0, 10),
	SHELL_CMD_ARG(
		rai, NULL,
		"Set Release Assistance Indication (RAI) parameters. "
		"In order to use RAI options for a socket, RAI feature must be "
		"globally enabled with 'link rai' command.",
		cmd_sock_rai, 0, 10),
	SHELL_CMD_ARG(
		list, NULL,
		"List open sockets.",
		cmd_sock_list, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(
	sock,
	&sub_sock,
	"Commands for socket operations such as connect and send.",
	mosh_print_help_shell);
