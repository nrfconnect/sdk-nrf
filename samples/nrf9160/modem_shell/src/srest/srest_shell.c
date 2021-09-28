/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr.h>
#include <assert.h>

#include <shell/shell.h>
#include <getopt.h>

#include <net/http_parser.h>
#include <net/rest_client.h>

#include "srest_shell.h"

static const char srest_shell_cmd_usage_str[] =
	"Simple REST shell client\n"
	"Usage:\n"
	"srest -d host [-p port] [-m method] [-u url] [-b body] [-H header1] [-H header2 ... header10]\n"
	"              [-s sec_tag] [-v verify_level] [-t timeout_in_ms] [-l length of the response buffer]\n"
	"\n"
	"  -d, --host,         Destination host/domain of the URL\n"
	"Optionals:\n"
	"  -m, --method,       HTTP method (one of the: \"get\" (default),\"head\",\n"
	"                      \"post\",\"put\",\"delete\" or \"patch\")\n"
	"  -p, --port,         Destination port (default: 80)\n"
	"  -u, --url,          URL beyond host/domain (default: \"/index.html\")\n"
	"  -l, --resp_len,     Length of the buffer reserved for the REST response\n"
	"                      (default: 1024 bytes)\n"
	"  -H, --header,       Header including CRLF, for example:\n"
	"                      -H \"Content-Type: application/json\\x0D\\x0A\"\n"
	"  -b, --body,         Payload body, example: -b '{\"foo\":bar}'\n"
	"  -s, --sec_tag,      Used security tag for TLS connection\n"
	"  -t, --timeout,      Request timeout in seconds\n"
	"                      (default: CONFIG_REST_CLIENT_REST_REQUEST_TIMEOUT)\n"
	"  -v, --peer_verify,  TLS peer verification level. None (0),\n"
	"                      optional (1) or required (2). Default value is 2.\n";

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
	{ 0, 0, 0, 0 }
};

static void srest_shell_print_usage(const struct shell *shell)
{
	shell_print(shell, "%s", srest_shell_cmd_usage_str);
}

/*****************************************************************************/

#define SREST_REQUEST_MAX_HEADERS 10
#define SREST_RESPONSE_BUFF_SIZE 1024
#define SREST_DEFAULT_DESTINATION_PORT 80

struct rip_header_list_item {
	bool in_use;
	char *header_str;
};

int srest_shell(const struct shell *shell, size_t argc, char **argv)
{
	int opt, i, j, len;
	int ret = 0;
	int long_index = 0;
	struct rest_client_req_resp_context rest_ctx = { 0 };
	/* Collect headers to: */
	struct rip_header_list_item headers[SREST_REQUEST_MAX_HEADERS] = { 0 };
	/* Headers to actual request + 1 for NULL: */
	char *req_headers[SREST_REQUEST_MAX_HEADERS + 1];
	bool headers_set = false;
	bool method_set = false;
	bool host_set = false;
	int response_buf_len = SREST_RESPONSE_BUFF_SIZE;

	/* Set the defaults: */
	rest_client_request_defaults_set(&rest_ctx);
	rest_ctx.url = "/index.html";
	rest_ctx.port = SREST_DEFAULT_DESTINATION_PORT;
	rest_ctx.body = NULL;
	memset(req_headers, 0, (SREST_REQUEST_MAX_HEADERS + 1) * sizeof(char *));

	if (argc < 3) {
		goto show_usage;
	}

	/* Start from the 1st argument */
	optind = 1;

	while ((opt = getopt_long(argc, argv, "d:p:b:H:m:s:u:v:t:l:", long_options, &long_index)) !=
	       -1) {
		switch (opt) {
		case 'm':
			if (strcmp(optarg, "get") == 0) {
				rest_ctx.http_method = HTTP_GET;
			} else if (strcmp(optarg, "head") == 0) {
				rest_ctx.http_method = HTTP_HEAD;
			} else if (strcmp(optarg, "post") == 0) {
				rest_ctx.http_method = HTTP_POST;
			} else if (strcmp(optarg, "put") == 0) {
				rest_ctx.http_method = HTTP_PUT;
			} else if (strcmp(optarg, "delete") == 0) {
				rest_ctx.http_method = HTTP_DELETE;
			} else if (strcmp(optarg, "patch") == 0) {
				rest_ctx.http_method = HTTP_PATCH;
			} else {
				shell_error(shell, "Unsupported HTTP request method");
				return -EINVAL;
			}
			method_set = true;
			break;
		case 'd':
			rest_ctx.host = optarg;
			host_set = true;
			break;
		case 'u':
			rest_ctx.url = optarg;
			break;
		case 's':
			rest_ctx.sec_tag = atoi(optarg);
			if (rest_ctx.sec_tag == 0) {
				shell_warn(shell, "sec_tag not an integer (> 0)");
				return -EINVAL;
			}
			break;
		case 'l':
			response_buf_len = atoi(optarg);
			if (response_buf_len == 0) {
				shell_warn(shell, "response buffer length not an integer (> 0)");
				return -EINVAL;
			}
			break;
		case 't':
			rest_ctx.timeout_ms = atoi(optarg);
			if (rest_ctx.timeout_ms == 0) {
				shell_warn(shell, "timeout not an integer (> 0)");
				return -EINVAL;
			}
			rest_ctx.timeout_ms *= 1000;
			break;
		case 'p':
			rest_ctx.port = atoi(optarg);
			if (rest_ctx.port == 0) {
				shell_warn(shell, "port not an integer (> 0)");
				return -EINVAL;
			}
			break;
		case 'b':
			rest_ctx.body = optarg;
			len = strlen(optarg);
			if (len > 0) {
				rest_ctx.body = k_calloc(len + 1, 1);
				if (rest_ctx.body == NULL) {
					shell_error(shell, "Cannot allocate memory for given body");
					return -ENOMEM;
				}
				strcpy(rest_ctx.body, optarg);
			}
			break;
		case 'H': {
			bool room_available = false;

			len = strlen(optarg);
			if (len <= 0) {
				shell_error(shell, "No header given");
				return -EINVAL;
			}

			/* Check if there are still room for additional header? */
			for (i = 0; i < SREST_REQUEST_MAX_HEADERS; i++) {
				if (!headers[i].in_use) {
					room_available = true;
					break;
				}
			}

			if (!room_available) {
				shell_error(shell, "There are already max number (%d) of headers",
					    SREST_REQUEST_MAX_HEADERS);
				return -EINVAL;
			}

			for (i = 0; i < SREST_REQUEST_MAX_HEADERS; i++) {
				if (!headers[i].in_use) {
					headers[i].header_str = k_malloc(strlen(optarg) + 1);
					if (headers[i].header_str == NULL) {
						shell_error(shell,
							    "Cannot allocate memory for header %s",
							    optarg);
						break;
					}
					headers[i].in_use = true;
					headers_set = true;
					strcpy(headers[i].header_str, optarg);
					break;
				}
			}
			break;
		}
		case 'v':
			rest_ctx.tls_peer_verify = atoi(optarg);
			if (rest_ctx.tls_peer_verify  < 0 || rest_ctx.tls_peer_verify  > 2) {
				shell_error(
					shell,
					"Valid values for peer verify (%d) are 0, 1 and 2.",
					rest_ctx.tls_peer_verify);
				return -EINVAL;
			}
			break;
		case '?':
			goto show_usage;
		default:
			shell_error(shell, "Unknown option. See usage:");
			goto show_usage;
		}
	}

	/* Check that all mandatory args were given: */
	if (!host_set) {
		shell_error(shell, "Please, give all mandatory options");
		goto show_usage;
	}

	rest_ctx.header_fields = NULL;
	if (headers_set) {
		/* Fill given headers to req_headers[] from headers[]: */
		for (i = 0; i < SREST_REQUEST_MAX_HEADERS; i++) {
			for (j = 0; j < SREST_REQUEST_MAX_HEADERS; j++) {
				headers_set = false;
				if (headers[j].in_use) {
					headers_set = true;
					headers[j].in_use = false;
					break;
				}
			}
			if (headers_set) {
				req_headers[i] = headers[j].header_str;
				req_headers[i + 1] = NULL;
				//shell_print(shell, "header %i set: %s", i, req_headers[i]);
			}
		}
		rest_ctx.header_fields = (const char **)req_headers;
	}

	rest_ctx.resp_buff = k_calloc(response_buf_len, 1);
	if (rest_ctx.resp_buff == NULL) {
		shell_error(shell, "No memory available for response buffer of length %d",
			    response_buf_len);
		return -ENOMEM;
	}
	rest_ctx.resp_buff_len = response_buf_len;

	ret = rest_client_request(&rest_ctx);
	if (ret) {
		shell_error(shell, "Error %d from rest_lib", ret);
	} else {
		shell_print(shell,
			    "Response:\n"
			    "  HTTP status: %d\n"
			    "  Body/Total length: %d/%d\n\n"
			    "  %s\n",
			    rest_ctx.http_status_code,
			    rest_ctx.response_len,
			    rest_ctx.total_response_len,
			    rest_ctx.response);
	}
	k_free(rest_ctx.body);
	for (i = 0; i < SREST_REQUEST_MAX_HEADERS; i++) {
		k_free(headers[i].header_str);
	}
	k_free(rest_ctx.resp_buff);

	return ret;

show_usage:
	/* Reset getopt for another users */
	optreset = 1;

	srest_shell_print_usage(shell);
	return ret;
}
