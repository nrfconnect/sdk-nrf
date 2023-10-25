/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <assert.h>

#include <zephyr/shell/shell.h>
#ifdef CONFIG_GETOPT
#include <zephyr/posix/unistd.h>
#endif
#include <getopt.h>

#include <zephyr/net/http/parser.h>
#include <net/rest_client.h>

#include "mosh_print.h"

static const char rest_shell_cmd_usage_str[] =
	"REST client\n"
	"Usage:\n"
	"rest -d host [-m method] [-p port] [-s sec_tag] [-v verify_level] [-u url] [-t timeout_in_ms]\n"
	"              [-H header1] [-H header2]...[-H header10] [-b body] [-l response_buffer_length]\n"
	"              [-k] [-i sckt_id] [--print_headers]\n"
	"\n"
	"  -d, --host,          Destination host/domain of the URL\n"
	"Optionals:\n"
	"  -m, --method,        HTTP method (one of the: \"get\" (default),\"head\",\n"
	"                       \"post\",\"put\",\"delete\" or \"patch\")\n"
	"  -p, --port,          Destination port (default: 80)\n"
	"  -s, --sec_tag,       Used security tag for TLS connection\n"
	"  -v, --peer_verify,   TLS peer verification level. None (0),\n"
	"                       optional (1) or required (2). Default value is 2.\n"
	"  -u, --url,           URL beyond host/domain (default: \"/index.html\")\n"
	"  -t, --timeout,       Request timeout in seconds. Zero means timeout is disabled.\n"
	"                       (default: CONFIG_REST_CLIENT_REST_REQUEST_TIMEOUT)\n"
	"  -H, --header,        Header including CRLF, for example:\n"
	"                       -H \"Content-Type: application/json\\x0D\\x0A\"\n"
	"  -b, --body,          Payload body, example: -b '{\"foo\":bar}'\n"
	"  -l, --resp_len,      Length of the buffer reserved for the REST response\n"
	"                       (default: 1024 bytes)\n"
	"  -k, --keepalive,     Keep socket connection alive for upcoming requests\n"
	"                       towards the same server (default: false)\n"
	"      --print_headers, Print HTTP headers of the response\n"
	"  -i, --sckt_id,       Use existing socket id (default: create a new connection)\n";

/* Following are not having short options: */
enum {
	REST_SHELL_OPT_PRINT_RESP_HEADERS = 1001,
};

/* Specifying the expected options (both long and short): */
static struct option long_options[] = {
	{ "host", required_argument, 0, 'd' },
	{ "port", required_argument, 0, 'p' },
	{ "url", required_argument, 0, 'u' },
	{ "body", required_argument, 0, 'b' },
	{ "resp_len", required_argument, 0, 'l' },
	{ "header", required_argument, 0, 'H' },
	{ "method", required_argument, 0, 'm' },
	{ "sec_tag", required_argument, 0, 's' },
	{ "peer_verify", required_argument, 0, 'v' },
	{ "timeout", required_argument, 0, 't' },
	{ "keepalive", no_argument, 0, 'k' },
	{ "sckt_id", required_argument, 0, 'i' },
	{ "help", no_argument, 0, 'h' },
	{ "print_headers", no_argument, 0, REST_SHELL_OPT_PRINT_RESP_HEADERS },
	{ 0, 0, 0, 0 }
};

static void rest_shell_print_usage(const struct shell *shell)
{
	mosh_print_no_format(rest_shell_cmd_usage_str);
}

/*****************************************************************************/

#define REST_REQUEST_MAX_HEADERS 10
#define REST_RESPONSE_BUFF_SIZE 1024
#define REST_DEFAULT_DESTINATION_PORT 80

static int rest_shell(const struct shell *shell, size_t argc, char **argv)
{
	int opt, i, len;
	int ret = 0;
	int long_index = 0;
	bool show_usage = false;
	struct rest_client_req_context req_ctx = { 0 };
	struct rest_client_resp_context resp_ctx = { 0 };
	/* Headers to request + 1 for NULL (for http_client): */
	char *req_headers[REST_REQUEST_MAX_HEADERS + 1] = { NULL };
	bool headers_set = false;
	bool method_set = false;
	bool host_set = false;
	bool print_headers_from_resp = false;
	int response_buf_len = REST_RESPONSE_BUFF_SIZE;

	/* Set the defaults: */
	rest_client_request_defaults_set(&req_ctx);
	req_ctx.url = "/index.html";
	req_ctx.port = REST_DEFAULT_DESTINATION_PORT;
	req_ctx.body = NULL;
	memset(req_headers, 0, (REST_REQUEST_MAX_HEADERS + 1) * sizeof(char *));

	/* Start from the 1st argument */
	optind = 1;

	while ((opt = getopt_long(argc, argv, "d:p:b:H:m:s:u:v:t:l:i:kh", long_options,
				  &long_index)) != -1) {
		switch (opt) {
		case 'm':
			if (strcmp(optarg, "get") == 0) {
				req_ctx.http_method = HTTP_GET;
			} else if (strcmp(optarg, "head") == 0) {
				req_ctx.http_method = HTTP_HEAD;
			} else if (strcmp(optarg, "post") == 0) {
				req_ctx.http_method = HTTP_POST;
			} else if (strcmp(optarg, "put") == 0) {
				req_ctx.http_method = HTTP_PUT;
			} else if (strcmp(optarg, "delete") == 0) {
				req_ctx.http_method = HTTP_DELETE;
			} else if (strcmp(optarg, "patch") == 0) {
				req_ctx.http_method = HTTP_PATCH;
			} else {
				mosh_error("Unsupported HTTP request method");
				ret = -EINVAL;
				goto end;
			}
			method_set = true;
			break;
		case 'k':
			req_ctx.keep_alive = true;
			break;
		case 'd':
			req_ctx.host = optarg;
			host_set = true;
			break;
		case 'u':
			req_ctx.url = optarg;
			break;
		case 'i':
			req_ctx.connect_socket = atoi(optarg);
			break;
		case 's':
			req_ctx.sec_tag = atoi(optarg);
			if (req_ctx.sec_tag == 0) {
				mosh_warn("sec_tag not an integer (> 0)");
				ret = -EINVAL;
				goto end;
			}
			break;
		case 'l':
			response_buf_len = atoi(optarg);
			if (response_buf_len == 0) {
				mosh_warn("response buffer length not an integer (> 0)");
				ret = -EINVAL;
				goto end;
			}
			break;
		case 't':
			req_ctx.timeout_ms = atoi(optarg);
			if (req_ctx.timeout_ms == 0) {
				req_ctx.timeout_ms = SYS_FOREVER_MS;
			}
			break;
		case 'p':
			req_ctx.port = atoi(optarg);
			if (req_ctx.port == 0) {
				mosh_warn("port not an integer (> 0)");
				ret = -EINVAL;
				goto end;
			}
			break;
		case 'b':
			len = strlen(optarg);
			if (len > 0) {
				req_ctx.body = k_malloc(len + 1);
				if (req_ctx.body == NULL) {
					mosh_error("Cannot allocate memory for given body");
					ret = -ENOMEM;
					goto end;
				}
				strcpy((char *)req_ctx.body, optarg);
			}
			break;
		case 'H': {
			bool room_available = false;

			len = strlen(optarg);
			if (len <= 0) {
				mosh_error("No header given");
				ret = -EINVAL;
				goto end;
			}

			/* Check if there are still room for additional header? */
			for (i = 0; i < REST_REQUEST_MAX_HEADERS; i++) {
				if (req_headers[i] == NULL) {
					room_available = true;
					break;
				}
			}

			if (!room_available) {
				mosh_error("There are already max number (%d) of headers",
					    REST_REQUEST_MAX_HEADERS);
				ret = -EINVAL;
				goto end;
			}

			req_headers[i] = k_malloc(strlen(optarg) + 1);
			if (req_headers[i] == NULL) {
				mosh_error("Cannot allocate memory for header %s", optarg);
				ret = -ENOMEM;
				goto end;
			}
			headers_set = true;
			strcpy(req_headers[i], optarg);
			break;
		}
		case 'v':
			req_ctx.tls_peer_verify = atoi(optarg);
			if (req_ctx.tls_peer_verify < 0 || req_ctx.tls_peer_verify > 2) {
				mosh_error(
					"Valid values for peer verify (%d) are 0, 1 and 2.",
					req_ctx.tls_peer_verify);
				ret = -EINVAL;
				goto end;
			}
			break;
		case REST_SHELL_OPT_PRINT_RESP_HEADERS:
			print_headers_from_resp = true;
			break;

		case '?':
			mosh_error("Unknown option. See usage:");
			show_usage = true;
			goto end;
		case 'h':
		default:
			show_usage = true;
			goto end;
		}
	}

	/* Check that all mandatory args were given: */
	if (!host_set) {
		mosh_error("Please, give all mandatory options");
		show_usage = true;
		goto end;
	}

	req_ctx.header_fields = NULL;
	if (headers_set) {
		req_ctx.header_fields = (const char **)req_headers;
	}

	req_ctx.resp_buff = k_calloc(response_buf_len, 1);
	if (req_ctx.resp_buff == NULL) {
		mosh_error(
			"No memory available for response buffer of length %d",
			response_buf_len);
		ret = -ENOMEM;
		goto end;
	}
	req_ctx.resp_buff_len = response_buf_len;

	ret = rest_client_request(&req_ctx, &resp_ctx);
	if (ret) {
		mosh_error("Error %d from rest_lib", ret);
	} else {
		int headers_len = resp_ctx.response - req_ctx.resp_buff;
		char *headers_start = req_ctx.resp_buff;
		int printed_headers_len = 0;

		if (print_headers_from_resp) {
			printed_headers_len = headers_len;
		}

		mosh_print(
			"Response:\n"
			"  Socket ID: %d (sckt closed: %s)\n"
			"  HTTP status: %d (%s)\n"
			"  Headers/Body/Total length: %d/%d/%d\n\n"
			"%.*s"
			"%s",
			resp_ctx.used_socket_id,
			(resp_ctx.used_socket_is_alive) ? "no" : "yes",
			resp_ctx.http_status_code,
			(resp_ctx.http_status_code) ? resp_ctx.http_status_code_str :
				"No response / timeout",
			headers_len,
			resp_ctx.response_len,
			resp_ctx.total_response_len,
			printed_headers_len, headers_start,
			resp_ctx.response);
	}

end:
	k_free((void *)req_ctx.body);
	for (i = 0; i < REST_REQUEST_MAX_HEADERS; i++) {
		k_free(req_headers[i]);
	}
	k_free(req_ctx.resp_buff);

	/* Reset getopt for another users */
	optreset = 1;

	if (show_usage) {
		rest_shell_print_usage(shell);
	}
	return ret;
}

SHELL_CMD_REGISTER(rest, NULL, "REST client.", rest_shell);
