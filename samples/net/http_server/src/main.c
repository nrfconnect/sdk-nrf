/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dns_sd.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/net/net_config.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>

#include <zephyr/net/http/client.h>
#include <zephyr/net/http/parser.h>

#if defined(CONFIG_DK_LIBRARY)
#include <dk_buttons_and_leds.h>
#endif /* defined(CONFIG_DK_LIBRARY) */

#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#endif

#include "credentials_provision.h"

LOG_MODULE_REGISTER(http_server, CONFIG_HTTP_SERVER_SAMPLE_LOG_LEVEL);

#define SERVER_PORT			CONFIG_HTTP_SERVER_SAMPLE_PORT
#define MAX_CLIENT_QUEUE		CONFIG_HTTP_SERVER_SAMPLE_CLIENTS_MAX
#define STACK_SIZE			CONFIG_HTTP_SERVER_SAMPLE_STACK_SIZE
#define THREAD_PRIORITY			K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)

/* Macro used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()								\
	LOG_ERR("Fatal error!%s", IS_ENABLED(CONFIG_RESET_ON_FATAL_ERROR) ?	\
				  " Rebooting the device" : "");		\
	LOG_PANIC();								\
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

#if defined(CONFIG_NET_HOSTNAME)
/* Register service */
DNS_SD_REGISTER_TCP_SERVICE(http_server_sd, CONFIG_NET_HOSTNAME, "_http", "local",
			    DNS_SD_EMPTY_TXT, SERVER_PORT);
#endif /* CONFIG_NET_HOSTNAME */

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

struct http_req {
	struct http_parser parser;
	int socket;
	bool received_all;
	enum http_method method;
	const char *url;
	size_t url_len;
	const char *body;
	size_t body_len;
};

/* Forward declarations */
static void process_tcp4(void);
static void process_tcp6(void);

/* Keep track of the current LED states. 0 = LED OFF, 1 = LED ON.
 * Index 0 corresponds to LED1, index 1 to LED2.
 */
static uint8_t led_states[2];

/* HTTP responses for demonstration */
#define RESPONSE_200 "HTTP/1.1 200 OK\r\n"
#define RESPONSE_400 "HTTP/1.1 400 Bad Request\r\n\r\n"
#define RESPONSE_403 "HTTP/1.1 403 Forbidden\r\n\r\n"
#define RESPONSE_404 "HTTP/1.1 404 Not Found\r\n\r\n"
#define RESPONSE_405 "HTTP/1.1 405 Method Not Allowed\r\n\r\n"
#define RESPONSE_500 "HTTP/1.1 500 Internal Server Error\r\n\r\n"

/* Processing threads for incoming connections */
K_THREAD_STACK_ARRAY_DEFINE(tcp4_handler_stack, MAX_CLIENT_QUEUE, STACK_SIZE);
static struct k_thread tcp4_handler_thread[MAX_CLIENT_QUEUE];
static k_tid_t tcp4_handler_tid[MAX_CLIENT_QUEUE];
K_THREAD_DEFINE(tcp4_thread_id, STACK_SIZE,
		process_tcp4, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

K_THREAD_STACK_ARRAY_DEFINE(tcp6_handler_stack, MAX_CLIENT_QUEUE, STACK_SIZE);
static struct k_thread tcp6_handler_thread[MAX_CLIENT_QUEUE];
static k_tid_t tcp6_handler_tid[MAX_CLIENT_QUEUE];
K_THREAD_DEFINE(tcp6_thread_id, STACK_SIZE,
		process_tcp6, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

static K_SEM_DEFINE(network_connected_sem, 0, 1);
static K_SEM_DEFINE(ipv6_setup_sem, 0, 1);

static int tcp4_sock;
static int tcp4_accepted[MAX_CLIENT_QUEUE];
static int tcp6_sock;
static int tcp6_accepted[MAX_CLIENT_QUEUE];

static struct http_parser_settings parser_settings;

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connected");

		k_sem_give(&network_connected_sem);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network disconnected");
		break;
	default:
		/* Don't care */
		return;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint32_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		FATAL_ERROR();
		return;
	}
}

/* Update the LED states. Returns 0 if it was updated, otherwise -1. */
static int led_update(uint8_t index, uint8_t state)
{
	if (state <= 1) {
		led_states[index] = state;
	} else {
		LOG_WRN("Illegal value %d written to LED %d", state, index);
		return -EBADMSG;
	}

#if defined(CONFIG_DK_LIBRARY)
	int ret;

	ret = dk_set_led(index, led_states[index]);
	if (ret) {
		LOG_ERR("Failed to update LED %d state to %d", index, led_states[index]);
		FATAL_ERROR();
		return -EIO;
	}
#endif /* defined(CONFIG_DK_LIBRARY) */

	LOG_INF("LED %d state updated to %d", index, led_states[index]);

	return 0;
}

static int handle_put(struct http_req *request, char *response, size_t response_size)
{
	int ret;
	uint8_t led_id, led_index;

	if (request->body_len < 1) {
		LOG_ERR("No request body");
		return -EBADMSG;
	}

	/* Get LED ID */
	ret = sscanf(request->url, "/led/%hhu", &led_id);
	if (ret != 1) {
		LOG_WRN("Invalid URL: %s", request->url);
		return -EINVAL;
	}

	/* Get LED Index */
	if (led_id < 1 || led_id > 2) {
		LOG_ERR("Invalid LED ID: %d, valid values are 1 and 2", led_id);
		return -EINVAL;
	}

	led_index = led_id - 1;

	ret = led_update(led_index, (uint8_t)atoi(request->body));
	if (ret) {
		LOG_WRN("Update failed for LED %d", led_index);
		return ret;
	}

	ret = snprintk(response, response_size, "%sContent-Length: %d\r\n\r\n",
		       RESPONSE_200, 0);
	if ((ret < 0) || (ret >= response_size)) {
		return -ENOBUFS;
	}

	return 0;
}

static int handle_get(struct http_req *request, char *response, size_t response_size)
{
	int ret;
	char body[2];
	uint8_t led_id, led_index;

	/* Get LED ID */
	ret = sscanf(request->url, "/led/%hhu", &led_id);
	if (ret != 1) {
		LOG_WRN("Invalid URL: %s", request->url);
		return -EINVAL;
	}

	/* Get LED Index */
	if (led_id < 1 || led_id > 2) {
		LOG_ERR("Invalid LED ID: %d, valid values are 1 and 2", led_id);
		return -EINVAL;
	}

	led_index = led_id - 1;

	ret = snprintk(body, sizeof(body), "%d", led_states[led_index]);
	if ((ret < 0) || (ret >= sizeof(body))) {
		return -ENOBUFS;
	}

	ret = snprintk(response, response_size,
		       "%sContent-Type: text/plain\r\n\r\nContent-Length: %d\r\n\r\n%s",
		       RESPONSE_200, strlen(body), body);
	if ((ret < 0) || (ret >= response_size)) {
		return -ENOBUFS;
	}

	return 0;
}

static int send_response(struct http_req *request, char *response)
{
	ssize_t out_len;
	size_t len = strlen(response);

	while (len) {
		out_len = send(request->socket, response, len, 0);
		if (out_len < 0) {
			LOG_ERR("send, error: %d", -errno);
			return -errno;
		}

		len -= out_len;
	}

	return 0;
}

/* Handle HTTP request */
static void handle_http_request(struct http_req *request)
{
	int ret;
	char *resp_ptr = RESPONSE_200;
	char get_response_buffer[100] = { 0 };

	/* Handle the request method */
	switch (request->method) {
	case HTTP_PUT:
		ret = handle_put(request, get_response_buffer, sizeof(get_response_buffer));
		if (ret) {
			LOG_WRN("Incoming HTTP GET request, error: %d", ret);
			resp_ptr = (ret == -EINVAL) ? RESPONSE_404 :
				   (ret == -EBADMSG) ? RESPONSE_400 : RESPONSE_500;
			break;
		}

		resp_ptr = get_response_buffer;
		break;
	case HTTP_GET:
		ret = handle_get(request, get_response_buffer, sizeof(get_response_buffer));
		if (ret) {
			LOG_WRN("Incoming HTTP GET request, error: %d", ret);
			resp_ptr = (ret == -EINVAL) ? RESPONSE_404 : RESPONSE_500;
			break;
		}

		resp_ptr = get_response_buffer;
		break;
	default:
		LOG_WRN("Unsupported request method");
		resp_ptr = RESPONSE_405;
		break;
	}

	/* Send response */
	ret = send_response(request, resp_ptr);
	if (ret) {
		LOG_ERR("send_response, error: %d", ret);
		FATAL_ERROR();
		return;
	}
}

/* HTTP parser callbacks */
static int on_body(struct http_parser *parser, const char *at, size_t length)
{
	struct http_req *req = CONTAINER_OF(parser, struct http_req, parser);

	req->body = at;
	req->body_len = length;

	LOG_DBG("on_body: %d", parser->method);
	LOG_DBG("> %.*s", length, at);

	return 0;
}

static int on_headers_complete(struct http_parser *parser)
{
	struct http_req *req = CONTAINER_OF(parser, struct http_req, parser);

	req->method = parser->method;

	LOG_DBG("on_headers_complete, method: %s", http_method_str(parser->method));

	return 0;
}

static int on_message_begin(struct http_parser *parser)
{
	struct http_req *req = CONTAINER_OF(parser, struct http_req, parser);

	req->received_all = false;

	LOG_DBG("on_message_begin, method: %d", parser->method);

	return 0;
}

static int on_message_complete(struct http_parser *parser)
{
	struct http_req *req = CONTAINER_OF(parser, struct http_req, parser);

	req->received_all = true;

	LOG_DBG("on_message_complete, method: %d", parser->method);

	return 0;
}

static int on_url(struct http_parser *parser, const char *at, size_t length)
{
	struct http_req *req = CONTAINER_OF(parser, struct http_req, parser);

	req->url = at;
	req->url_len = length;

	LOG_DBG("on_url, method: %d", parser->method);
	LOG_DBG("> %.*s", length, at);

	return 0;
}

/* Initialize HTTP parser. */
static void parser_init(void)
{
	http_parser_settings_init(&parser_settings);

	parser_settings.on_body = on_body;
	parser_settings.on_headers_complete = on_headers_complete;
	parser_settings.on_message_begin = on_message_begin;
	parser_settings.on_message_complete = on_message_complete;
	parser_settings.on_url = on_url;
}

static int setup_server(int *sock, struct sockaddr *bind_addr, socklen_t bind_addrlen)
{
	int ret;
	int enable = 1;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		/* Run the Zephyr Native TLS stack (MBed TLS) in the application core instead of
		 * using the TLS stack on the modem.
		 * This is needed because the modem does not support TLS in server mode.
		 *
		 * This is done by using (SOCK_STREAM | SOCK_NATIVE_TLS) as socket type when
		 * building for a nRF91 Series board.
		 */
		int type = IS_ENABLED(CONFIG_NRF_MODEM_LIB) ? SOCK_STREAM | SOCK_NATIVE_TLS :
							      SOCK_STREAM;

		*sock = socket(bind_addr->sa_family, type, IPPROTO_TLS_1_2);
	} else {
		*sock = socket(bind_addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
	}

	if (*sock < 0) {
		LOG_ERR("Failed to create socket: %d", -errno);
		return -errno;
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		sec_tag_t sec_tag_list[] = {
			CONFIG_HTTP_SERVER_SAMPLE_SERVER_CERTIFICATE_SEC_TAG,
		};

		ret = setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST,
				 sec_tag_list, sizeof(sec_tag_list));
		if (ret < 0) {
			LOG_ERR("Failed to set security tag list %d", -errno);
			return -errno;
		}

		if (IS_ENABLED(CONFIG_HTTP_SERVER_SAMPLE_PEER_VERIFICATION_REQUIRE)) {
			int require = 2;

			ret = zsock_setsockopt(*sock, SOL_TLS, TLS_PEER_VERIFY,
					       &require,
					       sizeof(require));
			if (ret < 0) {
				LOG_ERR("Failed to set peer verification option %d", -errno);
				return -errno;
			}
		}
	}

	ret = setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	if (ret) {
		LOG_ERR("Failed to set SO_REUSEADDR %d", -errno);
		return -errno;
	}

	ret = bind(*sock, bind_addr, bind_addrlen);
	if (ret < 0) {
		LOG_ERR("Failed to bind socket %d", -errno);
		return -errno;
	}

	ret = listen(*sock, MAX_CLIENT_QUEUE);
	if (ret < 0) {
		LOG_ERR("Failed to listen on socket %d", -errno);
		return -errno;
	}

	return ret;
}

static void client_conn_handler(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr1);
	int *sock = ptr2;
	k_tid_t *in_use = ptr3;
	int received;
	int ret;
	char buf[CONFIG_HTTP_SERVER_SAMPLE_RECEIVE_BUFFER_SIZE];
	size_t offset = 0;
	size_t total_received = 0;
	struct http_req request = {
		.socket = *sock,
	};

	http_parser_init(&request.parser, HTTP_REQUEST);

	while (true) {
		received = recv(request.socket, buf + offset, sizeof(buf) - offset, 0);
		if (received == 0) {
			/* Connection closed */
			LOG_DBG("[%d] Connection closed by peer", request.socket);
			break;
		} else if (received < 0) {
			/* Socket error */
			ret = -errno;
			LOG_ERR("[%d] Connection error %d", request.socket, ret);
			break;
		}

		/* Parse the received data as HTTP request */
		(void)http_parser_execute(&request.parser,
					  &parser_settings, buf + offset, received);

		total_received += received;
		offset += received;

		if (offset >= sizeof(buf)) {
			offset = 0;
		}

		/* If the HTTP request has been completely received, stop receiving data and
		 * proceed to process the request.
		 */
		if (request.received_all) {
			handle_http_request(&request);
			break;
		}
	};

	(void)close(request.socket);

	*sock = -1;
	*in_use = NULL;
}

static int get_free_slot(int *accepted)
{
	for (int i = 0; i < MAX_CLIENT_QUEUE; i++) {
		if (accepted[i] < 0) {
			return i;
		}
	}

	return -1;
}

static void process_tcp(sa_family_t family, int *sock, int *accepted)
{
	int client;
	int slot;
	struct sockaddr_in6 client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	char addr_str[INET6_ADDRSTRLEN];

	client = accept(*sock, (struct sockaddr *)&client_addr,
			&client_addr_len);
	if (client < 0) {
		LOG_ERR("Error in accept %d, try again", -errno);
		return;
	}

	slot = get_free_slot(accepted);
	if (slot < 0 || slot >= MAX_CLIENT_QUEUE) {
		LOG_ERR("Cannot accept more connections");
		close(client);
		return;
	}

	accepted[slot] = client;

	if (family == AF_INET6) {
		tcp6_handler_tid[slot] = k_thread_create(
			&tcp6_handler_thread[slot],
			tcp6_handler_stack[slot],
			K_THREAD_STACK_SIZEOF(tcp6_handler_stack[slot]),
			(k_thread_entry_t)client_conn_handler,
			INT_TO_POINTER(slot),
			&accepted[slot],
			&tcp6_handler_tid[slot],
			THREAD_PRIORITY,
			0, K_NO_WAIT);
	} else if (family == AF_INET) {
		tcp4_handler_tid[slot] = k_thread_create(
			&tcp4_handler_thread[slot],
			tcp4_handler_stack[slot],
			K_THREAD_STACK_SIZEOF(tcp4_handler_stack[slot]),
			(k_thread_entry_t)client_conn_handler,
			INT_TO_POINTER(slot),
			&accepted[slot],
			&tcp4_handler_tid[slot],
			THREAD_PRIORITY,
			0, K_NO_WAIT);
	}

	net_addr_ntop(client_addr.sin6_family, &client_addr.sin6_addr, addr_str, sizeof(addr_str));

	LOG_INF("[%d] Connection from %s accepted", client, addr_str);
}

/* Processing incoming IPv4 clients */
static void process_tcp4(void)
{
	int ret;
	struct sockaddr_in addr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(SERVER_PORT),
	};

	ret = setup_server(&tcp4_sock, (struct sockaddr *)&addr4, sizeof(addr4));
	if (ret < 0) {
		LOG_ERR("Failed to create IPv4 socket %d", ret);
		FATAL_ERROR();
		return;
	}

	LOG_INF("Waiting for IPv4 HTTP connections on port %d, sock %d", SERVER_PORT, tcp4_sock);

	while (true) {
		process_tcp(AF_INET, &tcp4_sock, tcp4_accepted);
	}
}

/* Processing incoming IPv6 clients */
static void process_tcp6(void)
{
	int ret;
	struct sockaddr_in6 addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(SERVER_PORT),
	};

	ret = setup_server(&tcp6_sock, (struct sockaddr *)&addr6, sizeof(addr6));
	if (ret < 0) {
		LOG_ERR("Failed to create IPv6 socket %d", ret);
		FATAL_ERROR();
		return;
	}

	LOG_INF("Waiting for IPv6 HTTP connections on port %d, sock %d",
		SERVER_PORT, tcp6_sock);

	k_sem_give(&ipv6_setup_sem);

	while (true) {
		process_tcp(AF_INET6, &tcp6_sock, tcp6_accepted);
	}
}

void start_listener(void)
{
	for (size_t i = 0; i < MAX_CLIENT_QUEUE; i++) {
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			tcp4_accepted[i] = -1;
			tcp4_sock = -1;
		}

		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			tcp6_accepted[i] = -1;
			tcp6_sock = -1;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		k_thread_start(tcp6_thread_id);

		/* Wait for the thread that sets up the IPv6 sockets to complete
		 * before starting the IPv4 thread.
		 * This is to avoid an error when secure sockets
		 * for both IP families are created at the same time in two different threads.
		 */
		k_sem_take(&ipv6_setup_sem, K_FOREVER);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		k_thread_start(tcp4_thread_id);
	}
}

int main(void)
{
	int ret;

	LOG_INF("HTTP Server sample started");

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		ret = credentials_provision();
		if (ret) {
			LOG_ERR("credentials_provision, error: %d", ret);
			FATAL_ERROR();
			return ret;
		}
	}

	parser_init();

#if defined(CONFIG_DK_LIBRARY)
	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("dk_leds_init, error: %d", ret);
		FATAL_ERROR();
		return ret;
	}
#endif /* defined(CONFIG_DK_LIBRARY) */

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	ret = conn_mgr_all_if_up(true);
	if (ret) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", ret);
		FATAL_ERROR();
		return ret;
	}

	LOG_INF("Network interface brought up");

	ret = conn_mgr_all_if_connect(true);
	if (ret) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", ret);
		FATAL_ERROR();
		return ret;
	}

	/* Resend connection status if the sample is built for NATIVE_SIM.
	 * This is necessary because the network interface is automatically brought up
	 * at SYS_INIT() before main() is called.
	 * This means that NET_EVENT_L4_CONNECTED fires before the
	 * appropriate handler l4_event_handler() is registered.
	 */
	if (IS_ENABLED(CONFIG_BOARD_NATIVE_SIM)) {
		conn_mgr_mon_resend_status();
	}

	k_sem_take(&network_connected_sem, K_FOREVER);

	start_listener();

	return 0;
}
