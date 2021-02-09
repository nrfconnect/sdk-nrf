/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <net/socket.h>
#include <net/tls_credentials.h>
#include <net/http_client.h>
#include <nrf_socket.h>
#include "slm_at_httpc.h"
#include "slm_util.h"

LOG_MODULE_REGISTER(httpc, CONFIG_SLM_LOG_LEVEL);

#define HTTPC_HOST_LEN		64
#define HTTPC_METHOD_LEN	20
#define HTTPC_RES_LEN		256
#define HTTPC_HEADER_LEN	512
#define HTTPC_REQ_LEN		(HTTPC_METHOD_LEN + HTTPC_RES_LEN \
				+ HTTPC_HEADER_LEN + 3)
#define HTTPC_FRAG_SIZE		NET_IPV4_MTU
#define HTTPC_BUF_LEN		1024
#if HTTPC_REQ_LEN > HTTPC_BUF_LEN
# error "Please specify larger HTTPC_BUF_LEN"
#endif
#define HTTPC_REQ_TO_S		10

/* Buffers for HTTP client. */
static uint8_t data_buf[HTTPC_BUF_LEN];

/**@brief List of supported AT commands. */
enum slm_httpc_at_cmd_type {
	AT_HTTPC_CONNECT,
	AT_HTTPC_REQUEST,
	AT_HTTPC_MAX
};

/**@brief HTTP connect operations. */
enum slm_httpccon_operation {
	AT_HTTPCCON_DISCONNECT,
	AT_HTTPCCON_CONNECT
};

/** forward declaration of cmd handlers **/
static int handle_AT_HTTPC_CONNECT(enum at_cmd_type cmd_type);
static int handle_AT_HTTPC_REQUEST(enum at_cmd_type cmd_type);

/**@brief SLM AT Command list type. */
static slm_at_cmd_list_t http_at_list[AT_HTTPC_MAX] = {
	{AT_HTTPC_CONNECT, "AT#XHTTPCCON", handle_AT_HTTPC_CONNECT},
	{AT_HTTPC_REQUEST, "AT#XHTTPCREQ", handle_AT_HTTPC_REQUEST},
};

static struct slm_httpc_ctx {
	int fd;				/* HTTPC socket */
	bool sec_transport;		/* secure session flag */
	uint32_t sec_tag;		/* security tag to be used */
	char host[HTTPC_HOST_LEN + 1];	/* HTTP server address */
	uint32_t port;			/* HTTP server port */
	char *method_str;		/* request method */
	char *resource;			/* resource */
	char *headers;			/* headers */
	char *payload;			/* payload chunk */
	size_t pl_len;			/* payload length */
	size_t pl_to_send;		/* payload to send to server */
	ssize_t pl_sent;		/* payload sent to server */
	bool rsp_completed;		/* inticator of completed response */
} httpc;

/* global functions defined in different resources */
void rsp_send(const uint8_t *str, size_t len);

/* global variable defined in different resources */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2];

#define THREAD_STACK_SIZE       KB(2)
#define THREAD_PRIORITY         K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(httpc_thread_stack, THREAD_STACK_SIZE);

static K_SEM_DEFINE(http_req_sem, 0, 1);
static K_SEM_DEFINE(http_data_sem, 0, 1);

static int socket_sectag_set(int fd, int sec_tag)
{
	int err;
	int verify;
	sec_tag_t sec_tag_list[] = { sec_tag };

	verify = TLS_PEER_VERIFY_REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, errno %d", errno);
		return -1;
	}

	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			 sizeof(sec_tag_t) * ARRAY_SIZE(sec_tag_list));
	if (err) {
		LOG_ERR("Failed to set socket security tag, errno %d", errno);
		return -1;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME,
			 httpc.host, sizeof(httpc.host));
	if (err < 0) {
		LOG_ERR("Failed to set TLS_HOSTNAME option: %d", errno);
		return -errno;
	}

	return 0;
}

static int resolve_and_connect(int family, const char *host, int sec_tag)
{
	int fd;
	int err;
	int proto;
	uint16_t port;
	struct addrinfo *addr;
	struct addrinfo *info;

	__ASSERT_NO_MSG(host);

	/* Set up port and protocol */
	if (httpc.sec_transport == false) {
		/* HTTP, port 80 */
		proto = IPPROTO_TCP;
	} else {
		/* HTTPS, port 443 */
		proto = IPPROTO_TLS_1_2;
	}

	port = htons(httpc.port);

	/* Lookup host */
	struct addrinfo hints = {
		.ai_family = family,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = proto,
	};

	err = getaddrinfo(host, NULL, &hints, &info);
	if (err) {
		LOG_ERR("Failed to resolve hostname %s on %s",
			log_strdup(host),
			family == AF_INET ? "IPv4" : "IPv6");
		return -1;
	}
	LOG_INF("Attempting to connect over %s",
		family == AF_INET ? log_strdup("IPv4") : log_strdup("IPv6"));
	fd = socket(family, SOCK_STREAM, proto);
	if (fd < 0) {
		LOG_ERR("Failed to create socket, errno %d", errno);
		goto cleanup;
	}

	if (proto == IPPROTO_TLS_1_2) {
		LOG_INF("Setting up TLS credentials");
		err = socket_sectag_set(fd, sec_tag);
		if (err) {
			LOG_ERR("Fail to set up TLS credentials: %d", err);
			goto cleanup;
		}
	}

	/* Not connected */
	err = -1;

	for (addr = info; addr != NULL; addr = addr->ai_next) {
		struct sockaddr *const sa = addr->ai_addr;

		switch (sa->sa_family) {
		case AF_INET6:
			((struct sockaddr_in6 *)sa)->sin6_port = port;
			break;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = port;
			break;
		}

		err = connect(fd, sa, addr->ai_addrlen);
		if (err) {
			/* Try next address */
			LOG_ERR("Unable to connect, errno %d", errno);
		} else {
			/* Connected */
			break;
		}
	}

cleanup:
	freeaddrinfo(info);

	if (err) {
		/* Unable to connect, close socket */
		close(fd);
		fd = -1;
	}

	return fd;
}

static int socket_timeout_set(int fd)
{
	int err;
	const uint32_t timeout_ms = HTTPC_REQ_TO_S * MSEC_PER_SEC;

	struct timeval timeo = {
		.tv_sec = (timeout_ms / 1000),
		.tv_usec = (timeout_ms % 1000) * 1000,
	};
	LOG_DBG("Configuring socket timeout (%ld s)", timeo.tv_sec);
	err = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket TX timeout, errno %d", errno);
		return -errno;
	}
	err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket RX timeout, errno %d", errno);
		return -errno;
	}

	return 0;
}

static int server_connect(const char *host, int sec_tag)
{
	int fd;
	int err;

	if (host == NULL) {
		LOG_ERR("Empty remote host.");
		return INVALID_SOCKET;
	}

	if ((httpc.sec_transport == true) && (sec_tag == -1)) {
		LOG_ERR("Empty secure tag.");
		return INVALID_SOCKET;
	}

	/* Attempt IPv6 connection if configured, fallback to IPv4 */
	fd = resolve_and_connect(AF_INET6, host, sec_tag);
	if (fd < 0) {
		fd = resolve_and_connect(AF_INET, host, sec_tag);
	}

	if (fd < 0) {
		LOG_ERR("Fail to resolve and connect");
		return INVALID_SOCKET;
	}
	LOG_INF("Connected to %s", log_strdup(host));

	/* Set socket timeout, if configured */
	err = socket_timeout_set(fd);
	if (err) {
		close(fd);
		return INVALID_SOCKET;
	}

	return fd;
}

static void response_cb(struct http_response *rsp,
			enum http_final_call final_data,
			void *user_data)
{
	static size_t data_received;

	if (rsp->data_len > HTTPC_BUF_LEN) {
		/* Increase HTTPC_BUF_LEN in case of overflow */
		LOG_WRN("HTTP parser buffer overflow!");
		return;
	}

	data_received += rsp->data_len;
	if (final_data == HTTP_DATA_MORE) {
		LOG_DBG("Partial data received (%zd bytes)", rsp->data_len);
		if (data_received == HTTPC_BUF_LEN) {
			sprintf(rsp_buf, "#XHTTPCRSP: %d,1\r\n",
				HTTPC_BUF_LEN);
			rsp_send(rsp_buf, strlen(rsp_buf));
			rsp_send(data_buf, HTTPC_BUF_LEN);
			data_received = 0;
		}
	} else if (final_data == HTTP_DATA_FINAL) {
		LOG_DBG("All the data received (%zd bytes)", data_received);
		sprintf(rsp_buf, "#XHTTPCRSP: %d,0\r\n", data_received);
		rsp_send(rsp_buf, strlen(rsp_buf));
		rsp_send(data_buf, data_received);
		httpc.rsp_completed = true;
		data_received = 0;
	}
}

static int headers_cb(int sock, struct http_request *req, void *user_data)
{
	size_t len;
	int ret = 0;

	len = strlen(httpc.headers);
	while (len > 0) {
		ret = send(sock, httpc.headers + ret, len, 0);
		if (ret < 0) {
			LOG_ERR("send header fail: %d", ret);
			return ret;
		}
		LOG_DBG("send header: %d bytes", ret);
		len -= ret;
	}

	return len;
}

static int payload_cb(int sock, struct http_request *req, void *user_data)
{
	size_t total_sent = 0;

	if (httpc.pl_len > 0) {
		sprintf(rsp_buf, "#XHTTPCREQ: 1\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		do {
			/* Wait until payload is ready */
			LOG_DBG("wait until payload is ready");
			k_sem_take(&http_req_sem, K_FOREVER);
			if (httpc.pl_len == 0) {
				LOG_INF("abort sending payload");
				httpc.pl_sent = 0;
				return -ECONNABORTED;
			}
			if (total_sent + httpc.pl_to_send > httpc.pl_len) {
				LOG_WRN("send unexpected payload");
				httpc.pl_to_send = httpc.pl_len - total_sent;
			}
			while (httpc.pl_sent < httpc.pl_to_send) {
				ssize_t ret;

				ret = send(sock, httpc.payload + httpc.pl_sent,
					   MIN(httpc.pl_to_send - httpc.pl_sent,
					   HTTPC_FRAG_SIZE), 0);
				if (ret < 0) {
					LOG_ERR("send fail: %d", ret);
					httpc.pl_len = 0;
					httpc.pl_sent = 0;
					k_sem_give(&http_data_sem);
					return -errno;
				}
				LOG_DBG("send %d bytes payload", ret);
				httpc.pl_sent += ret;
				total_sent += ret;
			}
			if (total_sent == httpc.pl_len) {
				httpc.pl_len = 0;
			}
			k_sem_give(&http_data_sem);
		} while (total_sent < httpc.pl_len);
		sprintf(rsp_buf, "#XHTTPCREQ: 0\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
	} else {
		sprintf(rsp_buf, "#XHTTPCREQ: 0\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
	}

	return total_sent;
}

static int do_http_connect(void)
{
	/* Connect to server if it is not connected yet. */
	if (httpc.fd == INVALID_SOCKET) {
		httpc.fd = server_connect(httpc.host, httpc.sec_tag);
		if (httpc.fd == INVALID_SOCKET) {
			LOG_ERR("server_connect fail.");
			sprintf(rsp_buf, "#XHTTPCCON: 0\r\n");
			rsp_send(rsp_buf, strlen(rsp_buf));
		} else {
			sprintf(rsp_buf, "#XHTTPCCON: 1\r\n");
			rsp_send(rsp_buf, strlen(rsp_buf));
		}
	} else {
		LOG_ERR("Already connected to server.");
		return -EINVAL;
	}

	return httpc.fd;
}

static int do_http_disconnect(void)
{
	int err = -EINVAL;

	/* Close socket if it is connected. */
	if (httpc.fd != INVALID_SOCKET) {
		close(httpc.fd);
		httpc.fd = INVALID_SOCKET;
		err = 0;
	} else {
		return -ENOTCONN;
	}
	if (httpc.pl_len > 0) {
		LOG_ERR("Exit request");
		httpc.pl_len = 0;
		k_sem_give(&http_req_sem);
	}
	sprintf(rsp_buf, "#XHTTPCCON: 0\r\n");
	rsp_send(rsp_buf, strlen(rsp_buf));

	return err;
}

static int http_method_str_enum(uint8_t *method_str)
{
	static const char * const method_strings[] = {
		"DELETE", "GET", "HEAD", "POST", "PUT", "CONNECT", "OPTIONS",
		"TRACE", "COPY", "LOCK", "MKCOL", "MOVE", "PROPFIND",
		"PROPPATCH", "SEARCH", "UNLOCK", "BIND", "REBIND", "UNBIND",
		"ACL", "REPORT", "MKACTIVITY", "CHECKOUT", "MERGE", "M-SEARCH",
		"NOTIFY", "SUBSCRIBE", "UNSUBSCRIBE", "PATCH", "PURGE",
		"MKCALENDAR", "LINK", "UNLINK"};

	for (int i = HTTP_DELETE; i <= HTTP_UNLINK; i++) {
		if (!strncmp(method_str, method_strings[i],
			HTTPC_METHOD_LEN)) {
			return i;
		}
	}
	return -1;
}

static int do_http_request(void)
{
	int err = -EINVAL;
	struct http_request req;
	int method;
	int32_t timeout = SYS_FOREVER_MS;

	if (httpc.fd == INVALID_SOCKET) {
		LOG_ERR("Remote host is not connected.");
		return -EINVAL;
	}

	method = http_method_str_enum(httpc.method_str);
	if (method < 0) {
		LOG_ERR("Request method is not allowed.");
		return -EINVAL;
	}

	memset(&req, 0, sizeof(req));
	req.method = (enum http_method)method;
	req.url = httpc.resource;
	req.host = httpc.host;
	req.protocol = "HTTP/1.1";
	req.response = response_cb;
	req.recv_buf = data_buf;
	req.recv_buf_len = HTTPC_BUF_LEN;
	req.payload_cb =  payload_cb;
	req.optional_headers_cb = headers_cb;
	err = http_client_req(httpc.fd, &req, timeout, "");
	if (err < 0) {
		/* Socket send/recv error */
		sprintf(rsp_buf, "#XHTTPCREQ: %d\r\n", err);
		rsp_send(rsp_buf, strlen(rsp_buf));
	} else if (httpc.rsp_completed == false) {
		/* Socket was closed by remote */
		err = -ECONNRESET;
		sprintf(rsp_buf, "#XHTTPCRSP: 0,%d\r\n", err);
		rsp_send(rsp_buf, strlen(rsp_buf));
	} else {
		err = 0;
	}

	return err;
}

/**@brief handle AT#XHTTPCCON commands
 *  AT#XHTTPCCON=<op>[,<host>,<port>[,<sec_tag>]]
 *  AT#XHTTPCCON? READ command not supported
 *  AT#XHTTPCCON=?
 */
static int handle_AT_HTTPC_CONNECT(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;

	uint16_t op;
	size_t host_sz = HTTPC_HOST_LEN;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&at_param_list) == 0) {
			return -EINVAL;
		}
		err = at_params_short_get(&at_param_list, 1, &op);
		if (err < 0) {
			LOG_ERR("Fail to get op: %d", err);
			return err;
		}

		if (op == AT_HTTPCCON_CONNECT) {
			if (at_params_valid_count_get(&at_param_list) <= 3) {
				return -EINVAL;
			}
			if (httpc.fd != INVALID_SOCKET) {
				return -EINPROGRESS;
			}
			err = util_string_get(&at_param_list, 2,
							httpc.host, &host_sz);
			if (err < 0) {
				LOG_ERR("Fail to get host: %d", err);
				return err;
			}

			err = at_params_int_get(&at_param_list, 3, &httpc.port);
			if (err < 0) {
				LOG_ERR("Fail to get port: %d", err);
				return err;
			}

			if (at_params_valid_count_get(&at_param_list) == 5) {
				err = at_params_int_get(&at_param_list, 4,
							&httpc.sec_tag);
				if (err < 0) {
					LOG_ERR("Fail to get sec_tag: %d", err);
					return err;
				}
				httpc.sec_transport = true;
			}
			LOG_DBG("Connect from http server");
			if (do_http_connect() >= 0) {
				err = 0;
			} else {
				err = -EINVAL;
			}
			break;
		} else if (op == AT_HTTPCCON_DISCONNECT) {
			LOG_DBG("Disconnect from http server");
			err = do_http_disconnect();
			if (err) {
				LOG_ERR("Fail to disconnect. Error: %d", err);
			}
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (httpc.sec_transport) {
			sprintf(rsp_buf, "#XHTTPCCON: %d,\"%s\",%d,%d\r\n",
				(httpc.fd == INVALID_SOCKET)?0:1, httpc.host,
				httpc.port, httpc.sec_tag);
		} else {
			sprintf(rsp_buf, "#XHTTPCCON: %d,\"%s\",%d\r\n",
				(httpc.fd == INVALID_SOCKET)?0:1, httpc.host,
				httpc.port);
		}
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf,
			"#XHTTPCCON: (%d,%d),<host>,<port>,<sec_tag>\r\n",
			AT_HTTPCCON_DISCONNECT, AT_HTTPCCON_CONNECT);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}


/**@brief handle AT#XHTTPCREQ commands
 *  AT#XHTTPCREQ=<method>,<resource>,<header>[,<payload_length>]
 *  AT#XHTTPCREQ? READ command not supported
 *  AT#XHTTPCREQ=?
 */
static int handle_AT_HTTPC_REQUEST(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	int param_count;
	size_t method_sz = HTTPC_METHOD_LEN;
	size_t resource_sz = HTTPC_RES_LEN;
	size_t headers_sz = HTTPC_HEADER_LEN;
	size_t offset;

	if (httpc.fd == INVALID_SOCKET) {
		LOG_ERR("Remote host is not connected.");
		return err;
	}

	if (httpc.pl_len > 0) {
		LOG_ERR("Another request is not finished.");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		param_count = at_params_valid_count_get(&at_param_list);
		err = util_string_get(&at_param_list, 1,
					   data_buf, &method_sz);
		if (err < 0) {
			LOG_ERR("Fail to get method string: %d", err);
			return err;
		}
		httpc.method_str = data_buf;
		offset = method_sz + 1;
		/* Get resource path string */
		err = util_string_get(&at_param_list, 2,
					   data_buf + offset, &resource_sz);
		if (err < 0) {
			LOG_ERR("Fail to get resource string: %d", err);
			return err;
		}
		httpc.resource = data_buf + offset;
		offset = offset + resource_sz + 1;
		/* Get header string */
		err = util_string_get(&at_param_list, 3,
					   data_buf + offset, &headers_sz);
		if (err < 0) {
			LOG_ERR("Fail to get option string: %d", err);
			return err;
		}
		httpc.headers = data_buf + offset;
		if (param_count >= 5) {
			err = at_params_int_get(&at_param_list, 4,
						  &httpc.pl_len);
			if (err != 0) {
				return err;
			}
		}
		/* start sending request */
		k_sem_give(&http_req_sem);
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to handle HTTP AT commands
 */
int slm_at_httpc_parse(const char *at_cmd, size_t length)
{
	int ret = -ENOENT;
	enum at_cmd_type type;

	for (int i = 0; i < AT_HTTPC_MAX; i++) {
		if (slm_util_cmd_casecmp(at_cmd, http_at_list[i].string)) {
			ret = at_parser_params_from_str(at_cmd, NULL,
						&at_param_list);
			if (ret) {
				LOG_ERR("Failed to parse AT command %d", ret);
				return -EINVAL;
			}
			type = at_parser_cmd_type_get(at_cmd);
			ret = http_at_list[i].handler(type);
			break;
		}
	}

	/* Return if no payload to send */
	if (ret != -ENOENT || (httpc.pl_len == 0)) {
		return ret;
	}
	/* Process input data as payload */
	httpc.payload = (char *)at_cmd;
	httpc.pl_to_send = length;
	httpc.pl_sent = 0;
	/* start sending payload */
	k_sem_give(&http_req_sem);
	/* Don't go any further until send returns */
	k_sem_take(&http_data_sem, K_FOREVER);
	if (httpc.pl_sent <= 0) {
		return -EINVAL;
	}
	return 0;
}

static void httpc_thread_fn(void *arg1, void *arg2, void *arg3)
{
	int err;

	while (1) {
		/* Don't go any further until sending HTTP request */
		k_sem_take(&http_req_sem, K_FOREVER);
		err = do_http_request();
		if (err < 0) {
			LOG_ERR("do_http_request fail:%d", err);
			/* Disconnect from server */
			err = do_http_disconnect();
			if (err) {
				LOG_ERR("Fail to disconnect. Error: %d", err);
			}
		}
	}
}

int slm_at_httpc_init(void)
{
	httpc.fd = INVALID_SOCKET;

	return 0;
}

int slm_at_httpc_uninit(void)
{
	int err = 0;

	if (httpc.fd != INVALID_SOCKET) {
		err = do_http_disconnect();
		if (err != 0) {
			LOG_ERR("Fail to disconnect. Error: %d", err);
		}
	}

	return err;
}

/**@brief API to list HTTP AT commands
 */
void slm_at_httpc_clac(void)
{
	for (int i = 0; i < AT_HTTPC_MAX; i++) {
		sprintf(rsp_buf, "%s\r\n", http_at_list[i].string);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}
}

K_THREAD_DEFINE(httpc_thread, K_THREAD_STACK_SIZEOF(httpc_thread_stack),
		httpc_thread_fn, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, 0);
