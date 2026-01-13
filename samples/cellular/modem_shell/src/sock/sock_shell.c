/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <fcntl.h>
#include <zephyr/sys/sys_getopt.h>

#include "sock.h"
#include "mosh_defines.h"
#include "mosh_print.h"
#include "link_api.h"

/* Maximum length of the address */
#define SOCK_MAX_ADDR_LEN 100

enum sock_shell_command {
	SOCK_CMD_CONNECT = 0,
	SOCK_CMD_CLOSE,
	SOCK_CMD_GETADDRINFO,
	SOCK_CMD_SEND,
	SOCK_CMD_RECV,
	SOCK_CMD_RAI,
	SOCK_CMD_OPTION_SET,
	SOCK_CMD_OPTION_GET,
};

enum sockopt_cmd_type {
	SOCKOPT_CMD_TYPE_SET,
	SOCKOPT_CMD_TYPE_GET,
};

static const char sock_connect_usage_str[] =
	"Usage: sock connect -a <address> -p <port>\n"
	"       [-f <family>] [-t <type>] [-b <port>] [-I <cid>] [-K]\n"
	"       [-S] [-T <sec_tag>] [-c] [-V <level>] [-H <hostname>]\n"
	"       [-C <dtls_cid>] [-F <dtls_frag_ext>]\n"
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
	"  -K, --keep_open           Keep socket open when its PDN connection is lost.\n"
	"  -S, --secure,             Enable secure connection (TLS 1.2/DTLS 1.2).\n"
	"  -T, --sec_tag, [int]      Security tag for TLS certificate(s).\n"
	"  -c, --cache,              Enable TLS session cache.\n"
	"  -V, --peer_verify, [int]  TLS peer verification level: 0 (none), 1 (optional) or\n"
	"                            2 (required, default).\n"
	"  -H, --hostname, [str]     Hostname for TLS peer verification.\n"
	"  -C, --dtls_cid, [int]     DTLS CID setting: 0 (disabled, default), 1 (supported) or\n"
	"                            2 (enabled).\n"
	"  -F, --dtls_frag_ext, [int]\n"
	"                            DTLS fragmentation extension setting:\n"
	"                            0 (disabled, default), 1 (512 bytes) or 2 (1024 bytes).\n"
	"  -h, --help,               Shows this help information";

static const char sock_close_usage_str[] =
	"Usage: sock close -i <socket id>\n"
	"Options:\n"
	"  -i, --id, [int]           Socket id. Use 'sock list' command to see open\n"
	"                            sockets.\n"
	"  -h, --help,               Shows this help information";

static const char sock_getaddrinfo_usage_str[] =
	"Usage: sock getaddrinfo -H <hostname>\n"
	"       [-f <family>] [-t <type>] [-I <cid>]\n"
	"Options:\n"
	"  -H, --hostname, [str]     Hostname to lookup\n"
	"  -f, --family, [str]       Address family: 'unspec' (default),\n"
	"                            'inet' (ipv4) or 'inet6' (ipv6)\n"
	"  -t, --type, [str]         Address type: 'any' (default),\n"
	"                            'stream (tcp)' or 'dgram' (udp)\n"
	"  -I, --cid, [int]          Use this option to lookup using a specific\n"
	"                            PDN CID. See link command for available CIDs.\n"
	"  -h, --help,               Shows this help information";

static const char sock_send_usage_str[] =
	"Usage: sock send -i <socket id> [-d <data> | -l <data length>] [-e <interval>]\n"
	"       [--packet_number_prefix] [-B <blocking>] [-s <size>] [-x] [-W]\n"
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
	"  -B, --blocking, [int]     Blocking (1) or non-blocking (0) mode. This is not\n"
	"                            valid when -e is given. Default value is 1.\n"
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
	"  -W, --wait_ack            Request a blocking send operation until the data\n"
	"                            has been sent on-air and acknowledged by the peer,\n"
	"                            if required by the network protocol. This is not valid\n"
	"                            when -B0 (non-blocking mode) is used.\n"
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
	"      --rai_last,           Sets SO_RAI option with value RAI_LAST.\n"
	"                            Indicates that the next call to send/sendto will be\n"
	"                            the last one for some time, which means that the\n"
	"                            modem can get out of connected mode quicker when\n"
	"                            this data is sent.\n"
	"      --rai_no_data,        Sets SO_RAI option with value RAI_NO_DATA.\n"
	"                            Indicates that the application will not send any\n"
	"                            more data. This socket option will apply\n"
	"                            immediately, and does not require a call to send\n"
	"                            afterwards.\n"
	"      --rai_one_resp,       Sets SO_RAI option with value RAI_ONE_RESP.\n"
	"                            Indicates that after the next call to send/sendto,\n"
	"                            the application is expecting to receive one more\n"
	"                            data packet before this socket will not be used\n"
	"                            again for some time.\n"
	"      --rai_ongoing,        Sets SO_RAI option with value RAI_ONGOING.\n"
	"                            If a client application expects to use the socket\n"
	"                            more it can indicate that by setting this socket\n"
	"                            option before the next send call which will keep\n"
	"                            the modem in connected mode longer.\n"
	"      --rai_wait_more,      Sets SO_RAI option with value RAI_WAIT_MORE.\n"
	"                            If a server application expects to use the socket\n"
	"                            more it can indicate that by setting this socket\n"
	"                            option before the next send call.\n"
	"  -h, --help,               Shows this help information";

static const char sock_setopt_usage_str[] =
	"Usage: sock option set -i <socket id> -o <socket option> -v <value>\n"
	"       [--level <level>]\n"
	"Options:\n"
	"  -i, --id, [int]           Socket id. Use 'sock list' command to see open\n"
	"                            sockets.\n"
	"  -o, --option, [str|int]   String name or integer ID of the socket option\n"
	"      --level, [str|int]    String name or integer ID of the socket option level.\n"
	"                            This is optional. If a valid option string is given\n"
	"                            with '-o' option, the level is set automatically.\n"
	"  -v, --value, [str|int]    Value of the socket option\n"
	"  -h, --help,               Shows this help information\n"
	"\nSee Zephyr net/socket.h and net/socket_ncs.h for more information about the options.\n";

static const char sock_getopt_usage_str[] =
	"Usage: sock option get -i <socket id> -o <socket option> [--level <level>]\n"
	"Options:\n"
	"  -i, --id, [int]           Socket id. Use 'sock list' command to see open\n"
	"                            sockets.\n"
	"  -o, --option, [str|int]   String name or integer ID of the socket option\n"
	"      --level, [str|int]    String name or integer ID of the socket option level.\n"
	"                            This is optional. If a valid option string is given\n"
	"                            with '-o' option, the level is set automatically.\n"
	"  -h, --help,               Shows this help information\n"
	"\nSee Zephyr net/socket.h and net/socket_ncs.h for more information about the options.\n";

static const char sock_option_id_list_str[] =
	"List of supported socket options which you can set as string for '-o' option:\n"
	" - TLS_SEC_TAG_LIST, [str] (for example \"42,43,44\")\n"
	" - TLS_HOSTNAME, [str]\n"
	" - TLS_CIPHERSUITE_LIST, [str] (for example \"0xC024,0xC023\")\n"
	" - TLS_PEER_VERIFY, [int]\n"
	" - TLS_DTLS_ROLE, [int]\n"
	" - TLS_SESSION_CACHE, [int]\n"
	" - TLS_SESSION_CACHE_PURGE, (set only)\n"
	" - TLS_DTLS_HANDSHAKE_TIMEO, [int]\n"
	" - TLS_CIPHERSUITE_USED, [int] (get only)\n"
	" - TLS_DTLS_CID, [int]\n"
	" - TLS_DTLS_CID_STATUS [int] (get only)\n"
	" - TLS_DTLS_CONN_SAVE [int], (set only)\n"
	" - TLS_DTLS_CONN_LOAD [int], (set only)\n"
	" - TLS_DTLS_HANDSHAKE_STATUS, [int] (get only)\n"
	" - SO_ERROR, [int] (get only)\n"
	" - SO_KEEPOPEN, [int]\n"
	" - SO_EXCEPTIONAL_DATA, [int]\n"
	" - SO_RCVTIMEO, [int] (time in milliseconds)\n"
	" - SO_SNDTIMEO, [int] (time in milliseconds)\n"
	" - SO_BINDTOPDN, [int] (set only)\n"
	" - SO_REUSEADDR, [int] (set only)\n"
	" - SO_RAI, [int] (set only)\n"
	" - SO_IP_ECHO_REPLY, [int]\n"
	" - SO_IPV6_ECHO_REPLY, [int]\n"
	" - SO_IPV6_DELAYED_ADDR_REFRESH, [int]\n"
	" - SO_TCP_SRV_SESSTIMEO, [int]\n"
	" - SO_SILENCE_ALL, [int]\n";

static const char sock_level_list_str[] =
	"List of supported socket levels which you can set as string for '--level' option:\n"
	" - SOL_SOCKET\n"
	" - SOL_TLS\n"
	" - IPPROTO_IP\n"
	" - IPPROTO_IPV6\n"
	" - IPPROTO_TCP\n"
	" - IPPROTO_ALL";

/* The following do not have short options */
#define SOCK_SHELL_OPT_RAI_LAST 200
#define SOCK_SHELL_OPT_RAI_NO_DATA 201
#define SOCK_SHELL_OPT_RAI_ONE_RESP 202
#define SOCK_SHELL_OPT_RAI_ONGOING 203
#define SOCK_SHELL_OPT_RAI_WAIT_MORE 204
#define SOCK_SHELL_OPT_PACKET_NUMBER_PREFIX 205
#define SOCK_SHELL_OPT_SOCKET_OPTION_LEVEL 206

/* Specifying the expected options (both long and short) */
static struct sys_getopt_option long_options[] = {
	{ "id",             sys_getopt_required_argument, 0, 'i' },
	{ "cid",            sys_getopt_required_argument, 0, 'I' },
	{ "address",        sys_getopt_required_argument, 0, 'a' },
	{ "port",           sys_getopt_required_argument, 0, 'p' },
	{ "family",         sys_getopt_required_argument, 0, 'f' },
	{ "type",           sys_getopt_required_argument, 0, 't' },
	{ "bind_port",      sys_getopt_required_argument, 0, 'b' },
	{ "secure",         sys_getopt_no_argument,       0, 'S' },
	{ "sec_tag",        sys_getopt_required_argument, 0, 'T' },
	{ "cache",          sys_getopt_no_argument,       0, 'c' },
	{ "peer_verify",    sys_getopt_required_argument, 0, 'V' },
	{ "hostname",       sys_getopt_required_argument, 0, 'H' },
	{ "dtls_cid",       sys_getopt_required_argument, 0, 'C' },
	{ "dtls_frag_ext",  sys_getopt_required_argument, 0, 'F' },
	{ "data",           sys_getopt_required_argument, 0, 'd' },
	{ "length",         sys_getopt_required_argument, 0, 'l' },
	{ "period",         sys_getopt_required_argument, 0, 'e' },
	{ "buffer_size",    sys_getopt_required_argument, 0, 's' },
	{ "hex",            sys_getopt_no_argument,       0, 'x' },
	{ "start",          sys_getopt_no_argument,       0, 'r' },
	{ "blocking",       sys_getopt_required_argument, 0, 'B' },
	{ "wait_ack",       sys_getopt_no_argument,       0, 'W' },
	{ "keep_open",      sys_getopt_no_argument,       0, 'K' },
	{ "print_format",   sys_getopt_required_argument, 0, 'P' },
	{ "option",         sys_getopt_required_argument, 0, 'o' },
	{ "value",          sys_getopt_required_argument, 0, 'v' },
	{ "level",          sys_getopt_required_argument, 0, SOCK_SHELL_OPT_SOCKET_OPTION_LEVEL },
	{ "packet_number_prefix", sys_getopt_no_argument, 0, SOCK_SHELL_OPT_PACKET_NUMBER_PREFIX },
	{ "rai_last",       sys_getopt_no_argument,       0, SOCK_SHELL_OPT_RAI_LAST },
	{ "rai_no_data",    sys_getopt_no_argument,       0, SOCK_SHELL_OPT_RAI_NO_DATA },
	{ "rai_one_resp",   sys_getopt_no_argument,       0, SOCK_SHELL_OPT_RAI_ONE_RESP },
	{ "rai_ongoing",    sys_getopt_no_argument,       0, SOCK_SHELL_OPT_RAI_ONGOING },
	{ "rai_wait_more",  sys_getopt_no_argument,       0, SOCK_SHELL_OPT_RAI_WAIT_MORE },
	{ "help",           sys_getopt_no_argument,       0, 'h' },
	{ 0,                0,                            0, 0   }
};

static const char short_options[] = "i:I:a:p:f:t:b:ST:cV:H:C:F:d:l:e:s:xrB:WKP:o:v:h";

static void sock_print_usage(enum sock_shell_command command)
{
	switch (command) {
	case SOCK_CMD_CONNECT:
		mosh_print_no_format(sock_connect_usage_str);
		break;
	case SOCK_CMD_CLOSE:
		mosh_print_no_format(sock_close_usage_str);
		break;
	case SOCK_CMD_GETADDRINFO:
		mosh_print_no_format(sock_getaddrinfo_usage_str);
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
	case SOCK_CMD_OPTION_SET:
		mosh_print_no_format(sock_setopt_usage_str);
		mosh_print_no_format(sock_option_id_list_str);
		mosh_print_no_format(sock_level_list_str);
		break;
	case SOCK_CMD_OPTION_GET:
		mosh_print_no_format(sock_getopt_usage_str);
		mosh_print_no_format(sock_option_id_list_str);
		mosh_print_no_format(sock_level_list_str);
		break;
	default:
		break;
	}
}

static int cmd_sock_getaddrinfo(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;

	if (argc < 2) {
		goto show_usage;
	}

	/* Variables for command line arguments */
	int arg_family = AF_UNSPEC;
	int arg_type = 0;
	char arg_hostname[SOCK_MAX_ADDR_LEN + 1];
	int arg_pdn_cid = 0;

	memset(arg_hostname, 0, SOCK_MAX_ADDR_LEN + 1);

	sys_getopt_init();

	int opt;

	while ((opt = sys_getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		int addr_len = 0;

		switch (opt) {
		case 'H': /* Hostname */
			addr_len = strlen(sys_getopt_optarg);
			if (addr_len > SOCK_MAX_ADDR_LEN) {
				mosh_error(
					"Hostname length %d exceeded. Maximum is %d.",
					addr_len, SOCK_MAX_ADDR_LEN);
				return -EINVAL;
			}
			memcpy(arg_hostname, sys_getopt_optarg, addr_len);
			break;
		case 'f': /* Address family */
			if (strcmp(sys_getopt_optarg, "unspec") == 0) {
				arg_family = AF_UNSPEC;
			} else if (strcmp(sys_getopt_optarg, "inet") == 0) {
				arg_family = AF_INET;
			} else if (!strcmp(sys_getopt_optarg, "inet6")) {
				arg_family = AF_INET6;
			} else {
				mosh_error(
					"Unsupported address family=%s. Supported values are: "
					"'unspec', 'inet' (ipv4) or 'inet6' (ipv6)",
					sys_getopt_optarg);
				return -EINVAL;
			}
			break;
		case 't': /* Socket type */
			if (strcmp(sys_getopt_optarg, "any") == 0) {
				arg_type = 0;
			} else if (strcmp(sys_getopt_optarg, "stream") == 0) {
				arg_type = SOCK_STREAM;
			} else if (strcmp(sys_getopt_optarg, "dgram") == 0) {
				arg_type = SOCK_DGRAM;
			} else {
				mosh_error(
					"Unsupported address type=%s. Supported values are: "
					"'any', 'stream' (tcp) or 'dgram' (udp)",
					sys_getopt_optarg);
				return -EINVAL;
			}
			break;
		case 'I': /* PDN CID */
			arg_pdn_cid = atoi(sys_getopt_optarg);
			if (arg_pdn_cid < 0) {
				mosh_error(
					"PDN CID (%d) must be non-negative integer.",
					arg_pdn_cid);
				return -EINVAL;
			}
			break;
		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		mosh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	err = sock_getaddrinfo(arg_family, arg_type, arg_hostname, arg_pdn_cid);

	return err;

show_usage:
	sock_print_usage(SOCK_CMD_GETADDRINFO);
	return err;
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
	uint32_t arg_sec_tag = SEC_TAG_TLS_INVALID;
	bool arg_session_cache = false;
	bool arg_keep_open = false;
	int arg_peer_verify = 2;
	char arg_peer_hostname[SOCK_MAX_ADDR_LEN + 1];
	int arg_dtls_cid = 0;
	int arg_dtls_frag_ext = 0;

	memset(arg_address, 0, SOCK_MAX_ADDR_LEN + 1);
	memset(arg_peer_hostname, 0, SOCK_MAX_ADDR_LEN + 1);

	sys_getopt_init();

	int opt;

	while ((opt = sys_getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		int addr_len = 0;

		switch (opt) {
		case 'I': /* PDN CID */
			arg_pdn_cid = atoi(sys_getopt_optarg);
			if (arg_pdn_cid < 0) {
				mosh_error(
					"PDN CID (%d) must be non-negative integer.",
					arg_pdn_cid);
				return -EINVAL;
			}
			break;
		case 'a': /* IP address, or hostname */
			addr_len = strlen(sys_getopt_optarg);
			if (addr_len > SOCK_MAX_ADDR_LEN) {
				mosh_error(
					"Address length %d exceeded. Maximum is %d.",
					addr_len, SOCK_MAX_ADDR_LEN);
				return -EINVAL;
			}
			memcpy(arg_address, sys_getopt_optarg, addr_len);
			break;
		case 'p': /* Port */
			arg_port = atoi(sys_getopt_optarg);
			if (arg_port <= 0 || arg_port > 65535) {
				mosh_error(
					"Port (%d) must be bigger than 0 and smaller than 65536.",
					arg_port);
				return -EINVAL;
			}
			break;
		case 'f': /* Address family */
			if (!strcmp(sys_getopt_optarg, "inet")) {
				arg_family = AF_INET;
			} else if (!strcmp(sys_getopt_optarg, "inet6")) {
				arg_family = AF_INET6;
			} else if (!strcmp(sys_getopt_optarg, "packet")) {
				arg_family = AF_PACKET;
			} else {
				mosh_error(
					"Unsupported address family=%s. Supported values are: "
					"'inet' (ipv4, default), 'inet6' (ipv6) or 'packet'",
					sys_getopt_optarg);
				return -EINVAL;
			}
			break;
		case 't': /* Socket type */
			if (!strcmp(sys_getopt_optarg, "stream")) {
				arg_type = SOCK_STREAM;
			} else if (!strcmp(sys_getopt_optarg, "dgram")) {
				arg_type = SOCK_DGRAM;
			} else if (!strcmp(sys_getopt_optarg, "raw")) {
				arg_type = SOCK_RAW;
			} else {
				mosh_error(
					"Unsupported address type=%s. Supported values are: "
					"'stream' (tcp, default), 'dgram' (udp) or 'raw'",
					sys_getopt_optarg);
				return -EINVAL;
			}
			break;
		case 'b': /* Bind port */
			arg_bind_port = atoi(sys_getopt_optarg);
			if (arg_bind_port <= 0 || arg_bind_port > 65535) {
				mosh_error(
					"Bind port (%d) must be bigger than 0 and "
					"smaller than 65536.",
					arg_bind_port);
				return -EINVAL;
			}
			break;
		case 'K':
			arg_keep_open = true;
			break;
		case 'S': /* Security */
			arg_secure = true;
			break;
		case 'T': /* Security tag */
			err = 0;
			arg_sec_tag = shell_strtoul(sys_getopt_optarg, 10, &err);
			if (err) {
				mosh_error(
					"Valid range for security tag (%s) is 0 .. %u.",
					sys_getopt_optarg, UINT32_MAX);
				return -EINVAL;
			}
			break;
		case 'c': /* TLS session cache */
			arg_session_cache = true;
			break;
		case 'V': /* TLS peer verify */
			arg_peer_verify = atoi(sys_getopt_optarg);
			if (arg_peer_verify < 0 || arg_peer_verify > 2) {
				mosh_error(
					"Valid values for peer verify (%d) are 0, 1 and 2.",
					arg_peer_verify);
				return -EINVAL;
			}
			break;
		case 'H': /* TLS peer hostname */
			addr_len = strlen(sys_getopt_optarg);
			if (addr_len > SOCK_MAX_ADDR_LEN) {
				mosh_error(
					"Peer hostname length %d exceeded. Maximum is %d.",
					addr_len, SOCK_MAX_ADDR_LEN);
				return -EINVAL;
			}
			strcpy(arg_peer_hostname, sys_getopt_optarg);
			break;
		case 'C': /* DTLS CID setting */
			arg_dtls_cid = atoi(sys_getopt_optarg);
			if (arg_dtls_cid < 0 || arg_dtls_cid > 2) {
				mosh_error(
					"Valid values for DTLS CID setting (%d) are 0, 1 and 2.",
					arg_dtls_cid);
				return -EINVAL;
			}
			break;
		case 'F': /* DTLS fragmentation extension */
			arg_dtls_frag_ext = atoi(sys_getopt_optarg);
			if (arg_dtls_frag_ext < 0 || arg_dtls_frag_ext > 2) {
				mosh_error(
					"Valid values for DTLS fragmentation extension (%d) are "
					"0, 1 and 2.", arg_dtls_frag_ext);
				return -EINVAL;
			}
			break;
		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
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
		arg_keep_open,
		arg_peer_verify,
		arg_peer_hostname,
		arg_dtls_cid,
		arg_dtls_frag_ext);

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

	sys_getopt_init();

	int opt;

	while ((opt = sys_getopt_long(argc, argv, "i:h", long_options, NULL)) != -1) {

		switch (opt) {
		case 'i': /* Socket ID */
			arg_socket_id = atoi(sys_getopt_optarg);
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
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
	int arg_flags = 0;

	memset(arg_send_data, 0, SOCK_MAX_SEND_DATA_LEN + 1);

	sys_getopt_init();

	int opt;

	while ((opt = sys_getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		int send_data_len = 0;

		switch (opt) {
		case 'i': /* Socket ID */
			arg_socket_id = atoi(sys_getopt_optarg);
			break;
		case 'd': /* Data to be sent is available in send buffer */
			send_data_len = strlen(sys_getopt_optarg);
			if (send_data_len > SOCK_MAX_SEND_DATA_LEN) {
				mosh_error(
					"Data length %d exceeded. Maximum is %d. Given data: %s",
					send_data_len, SOCK_MAX_SEND_DATA_LEN,
					sys_getopt_optarg);
				return -EINVAL;
			}
			strcpy(arg_send_data, sys_getopt_optarg);
			break;
		case 'x': /* Send data string is defined in hex format */
			arg_data_format_hex = true;
			break;
		case 'l': /* Length of data */
			arg_data_length = atoi(sys_getopt_optarg);
			break;
		case 'e': /* Interval in which data will be sent */
			arg_data_interval = atoi(sys_getopt_optarg);
			break;
		case 's': /* Buffer size */
			arg_buffer_size = atoi(sys_getopt_optarg);
			if (arg_buffer_size <= 0) {
				mosh_error(
					"Buffer size %d must be a positive number",
					arg_buffer_size);
				return -EINVAL;
			}
			break;
		case 'B': /* Blocking/non-blocking send or receive */
		{
			int blocking = atoi(sys_getopt_optarg);

			if (blocking != 0 && blocking != 1) {
				mosh_error(
					"Blocking (%s) must be either '0' (false) or '1' (true)",
					sys_getopt_optarg);
				return -EINVAL;
			}
			arg_blocking_recv = blocking;
			arg_blocking_send = blocking;
			break;
		}
		case 'W':
			arg_flags |= MSG_WAITACK;
			break;
		case SOCK_SHELL_OPT_PACKET_NUMBER_PREFIX:
			arg_packet_number_prefix = true;
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
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
		arg_flags,
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

	sys_getopt_init();

	int opt;

	while ((opt = sys_getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {

		switch (opt) {
		case 'i': /* Socket ID */
			arg_socket_id = atoi(sys_getopt_optarg);
			break;
		case 'l': /* Length of data */
			arg_data_length = atoi(sys_getopt_optarg);
			break;
		case 'r': /* Start monitoring received data */
			arg_receive_start = true;
			break;
		case 'B': /* Blocking/non-blocking send or receive */
		{
			int blocking = atoi(sys_getopt_optarg);

			if (blocking != 0 && blocking != 1) {
				mosh_error(
					"Blocking (%s) must be either '0' (false) or '1' (true)",
					sys_getopt_optarg);
				return -EINVAL;
			}
			arg_blocking_recv = blocking;
			arg_blocking_send = blocking;
			break;
		}
		case 'P': /* Receive data print format: "str" or "hex" */
			if (!strcmp(sys_getopt_optarg, "str")) {
				arg_recv_print_format =
					SOCK_RECV_PRINT_FORMAT_STR;
			} else if (!strcmp(sys_getopt_optarg, "hex")) {
				arg_recv_print_format =
					SOCK_RECV_PRINT_FORMAT_HEX;
			} else {
				mosh_error(
					"Receive data print format (%s) must be 'str' or 'hex'",
					sys_getopt_optarg);
				return -EINVAL;
			}
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
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
	int rai_option_count = 0;

	sys_getopt_init();

	int opt;

	while ((opt = sys_getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {

		switch (opt) {
		case 'i': /* Socket ID */
			arg_socket_id = atoi(sys_getopt_optarg);
			break;

		case SOCK_SHELL_OPT_RAI_LAST:
			arg_rai_last = true;
			rai_option_count++;
			break;
		case SOCK_SHELL_OPT_RAI_NO_DATA:
			arg_rai_no_data = true;
			rai_option_count++;
			break;
		case SOCK_SHELL_OPT_RAI_ONE_RESP:
			arg_rai_one_resp = true;
			rai_option_count++;
			break;
		case SOCK_SHELL_OPT_RAI_ONGOING:
			arg_rai_ongoing = true;
			rai_option_count++;
			break;
		case SOCK_SHELL_OPT_RAI_WAIT_MORE:
			arg_rai_wait_more = true;
			rai_option_count++;
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
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

	if (rai_option_count > 1) {
		mosh_warn(
			"%d RAI options set while normally just one should be used at a time. "
			"Hope you know what you are doing.", rai_option_count);
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

struct sock_opt_entry {
	char name[30];
	int opt;
	int level;
};

static struct sock_opt_entry sock_opt_table[] = {
	{ "TLS_SEC_TAG_LIST", TLS_SEC_TAG_LIST, SOL_TLS },
	{ "TLS_HOSTNAME", TLS_HOSTNAME, SOL_TLS },
	{ "TLS_CIPHERSUITE_LIST", TLS_CIPHERSUITE_LIST, SOL_TLS },
	{ "TLS_PEER_VERIFY", TLS_PEER_VERIFY, SOL_TLS },
	{ "TLS_DTLS_ROLE", TLS_DTLS_ROLE, SOL_TLS },
	{ "TLS_SESSION_CACHE", TLS_SESSION_CACHE, SOL_TLS },
	{ "TLS_SESSION_CACHE_PURGE", TLS_SESSION_CACHE_PURGE, SOL_TLS },
	{ "TLS_DTLS_HANDSHAKE_TIMEO", TLS_DTLS_HANDSHAKE_TIMEO, SOL_TLS },
	{ "TLS_CIPHERSUITE_USED", TLS_CIPHERSUITE_USED, SOL_TLS },
	{ "TLS_DTLS_CID", TLS_DTLS_CID, SOL_TLS },
	{ "TLS_DTLS_CID_STATUS", TLS_DTLS_CID_STATUS, SOL_TLS },
	{ "TLS_DTLS_CONN_SAVE", TLS_DTLS_CONN_SAVE, SOL_TLS },
	{ "TLS_DTLS_CONN_LOAD", TLS_DTLS_CONN_LOAD, SOL_TLS },
	{ "TLS_DTLS_HANDSHAKE_STATUS", TLS_DTLS_HANDSHAKE_STATUS, SOL_TLS },

	{ "SO_ERROR", SO_ERROR, SOL_SOCKET },
	{ "SO_KEEPOPEN", SO_KEEPOPEN, SOL_SOCKET },
	{ "SO_EXCEPTIONAL_DATA", SO_EXCEPTIONAL_DATA, SOL_SOCKET },
	{ "SO_RCVTIMEO", SO_RCVTIMEO, SOL_SOCKET },
	{ "SO_SNDTIMEO", SO_SNDTIMEO, SOL_SOCKET },
	{ "SO_BINDTOPDN", SO_BINDTOPDN, SOL_SOCKET },
	{ "SO_REUSEADDR", SO_REUSEADDR, SOL_SOCKET },
	{ "SO_RAI", SO_RAI, SOL_SOCKET },

	{ "SO_IP_ECHO_REPLY", SO_IP_ECHO_REPLY, IPPROTO_IP },

	{ "SO_IPV6_ECHO_REPLY", SO_IPV6_ECHO_REPLY, IPPROTO_IPV6 },
	{ "SO_IPV6_DELAYED_ADDR_REFRESH", SO_IPV6_DELAYED_ADDR_REFRESH, IPPROTO_IPV6 },

	{ "SO_TCP_SRV_SESSTIMEO", SO_TCP_SRV_SESSTIMEO, IPPROTO_TCP },

	{ "SO_SILENCE_ALL", SO_SILENCE_ALL, IPPROTO_ALL },
};

static struct sock_opt_entry *sock_opt_map(const char *opt)
{
	for (int i = 0; i < ARRAY_SIZE(sock_opt_table); i++) {
		if (!strcmp(opt, sock_opt_table[i].name)) {
			return &sock_opt_table[i];
		}
	}

	return NULL;
}

struct sock_level_entry {
	char name[20];
	int level;
};

static struct sock_level_entry sock_level_table[] = {
	{ "SOL_SOCKET", SOL_SOCKET },
	{ "SOL_TLS", SOL_TLS },
	{ "IPPROTO_IP", IPPROTO_IP },
	{ "IPPROTO_IPV6", IPPROTO_IPV6 },
	{ "IPPROTO_TCP", IPPROTO_TCP },
	{ "IPPROTO_ALL", IPPROTO_ALL },
};

static struct sock_level_entry *sock_level_map(const char *level)
{
	for (int i = 0; i < ARRAY_SIZE(sock_level_table); i++) {
		if (!strcmp(level, sock_level_table[i].name)) {
			return &sock_level_table[i];
		}
	}

	return NULL;
}

static int cmd_sock_setgetopt(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;
	enum sockopt_cmd_type cmd;

	if (strcmp(argv[0], "set") == 0) {
		cmd = SOCKOPT_CMD_TYPE_SET;
	} else {
		cmd = SOCKOPT_CMD_TYPE_GET;
	}

	if (argc < 2) {
		goto show_usage;
	}

	/* Variables for command line arguments */
	int arg_socket_id = SOCK_ID_NONE;
	int arg_sock_opt_level = SOCK_OPT_SOL_NONE;
	int arg_sock_opt_id = 0;
	char *arg_sock_opt_value = NULL;
	struct sock_opt_entry *sock_opt;
	struct sock_level_entry *sock_level;

	sys_getopt_init();

	int opt;

	while ((opt = sys_getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {

		switch (opt) {
		case 'i': /* Socket ID */
			arg_socket_id = atoi(sys_getopt_optarg);
			break;

		case SOCK_SHELL_OPT_SOCKET_OPTION_LEVEL: /* Level */
			arg_sock_opt_level = atoi(sys_getopt_optarg);
			if (arg_sock_opt_level == 0 && strcmp(sys_getopt_optarg, "0") != 0) {
				sock_level = sock_level_map(sys_getopt_optarg);
				if (sock_level == NULL) {
					mosh_error("Unknown socket level: %s", sys_getopt_optarg);
					goto show_usage;
				}
				arg_sock_opt_level = sock_level->level;
			}
			break;

		case 'o': /* Option */
			arg_sock_opt_id = atoi(sys_getopt_optarg);
			if (arg_sock_opt_id == 0) {
				sock_opt = sock_opt_map(sys_getopt_optarg);
				if (sock_opt == NULL) {
					mosh_error("Unknown socket option: %s", sys_getopt_optarg);
					goto show_usage;
				}
				arg_sock_opt_id = sock_opt->opt;
				if (arg_sock_opt_level == SOCK_OPT_SOL_NONE) {
					arg_sock_opt_level = sock_opt->level;
				}
			}
			break;

		case 'v': /* Value */
			if (cmd == SOCKOPT_CMD_TYPE_GET) {
				mosh_error("Unknown option (%s). See usage:", argv[sys_getopt_optind - 1]);
				goto show_usage;
			}
			arg_sock_opt_value = sys_getopt_optarg;
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		mosh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	if (cmd == SOCKOPT_CMD_TYPE_SET) {
		err = sock_setopt(arg_socket_id, arg_sock_opt_level, arg_sock_opt_id,
				  arg_sock_opt_value);
	} else {
		err = sock_getopt(arg_socket_id, arg_sock_opt_level, arg_sock_opt_id);
	}

	return err;

show_usage:
	if (cmd == SOCKOPT_CMD_TYPE_SET) {
		sock_print_usage(SOCK_CMD_OPTION_SET);
	} else {
		sock_print_usage(SOCK_CMD_OPTION_GET);
	}
	return err;
}

static int cmd_sock_list(const struct shell *shell, size_t argc, char **argv)
{
	return sock_list();
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sock_option,
	SHELL_CMD_ARG(set, NULL, "Set socket option.", cmd_sock_setgetopt, 0, 10),
	SHELL_CMD_ARG(get, NULL, "Get socket option.", cmd_sock_setgetopt, 0, 10),
	SHELL_SUBCMD_SET_END
);

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
		getaddrinfo, NULL,
		"DNS query using getaddrinfo.",
		cmd_sock_getaddrinfo, 0, 10),
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
	SHELL_CMD(
		option, &sub_sock_option,
		"Socket options.",
		mosh_print_help_shell),
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
