/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <logging/log.h>
#include <net/coap.h>
#include <net/coap_utils.h>
#include <net/socket.h>

LOG_MODULE_REGISTER(coap_utils, CONFIG_COAP_UTILS_LOG_LEVEL);

#define MAX_COAP_MSG_LEN 256
#define COAP_VER 1
#define COAP_TOKEN_LEN 8
#define COAP_MAX_REPLIES 1
#define COAP_POOL_SLEEP 500
#define COAP_OPEN_SOCKET_SLEEP 200
#define COAP_RECEIVE_STACK_SIZE 500

const static int nfds = 1;
static struct pollfd fds;
static struct coap_reply replies[COAP_MAX_REPLIES];
const struct sockaddr *socket_addr;

static K_THREAD_STACK_DEFINE(receive_stack_area, COAP_RECEIVE_STACK_SIZE);
static struct k_thread receive_thread_data;

static bool is_active;

static int coap_open_socket(void)
{
	int sock;

	while (1) {
		sock = socket(socket_addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
		if (sock < 0) {
			LOG_ERR("Failed to create socket %d", errno);
			k_sleep(K_MSEC(COAP_OPEN_SOCKET_SLEEP));
			continue;
		}
		break;
	}

	return sock;
}

static void coap_close_socket(int socket)
{
	(void)close(socket);
}

static void coap_bind_socket(int socket, const struct sockaddr *addr)
{
	char addr_str[INET6_ADDRSTRLEN];
	socklen_t socklen;
	size_t addr_str_size;
	const void *addr_src;

	__ASSERT(addr != NULL, "Pointer is null");

	switch (addr->sa_family) {
	case AF_INET: {
		socklen = sizeof(struct sockaddr_in);
		addr_str_size = INET_ADDRSTRLEN;
		addr_src = net_sin(addr)->sin_addr.s4_addr;

	} break;

	case AF_INET6: {
		socklen = sizeof(struct sockaddr_in6);
		addr_str_size = INET6_ADDRSTRLEN;
		addr_src = net_sin6(addr)->sin6_addr.s6_addr;
	} break;

	default:
		__ASSERT(false, "Invalid socket address family");
		return;
	}

	if (bind(socket, addr, socklen)) {
		LOG_ERR("Bind failed %d", errno);
	} else {
		inet_ntop(addr->sa_family, addr_src, addr_str, addr_str_size);
		LOG_INF("Binded: socket: %d : address: %s", socket,
			log_strdup(addr_str));
	}
}

static void coap_receive(void)
{
	static u8_t buf[MAX_COAP_MSG_LEN + 1];
	struct coap_packet response;
	struct coap_reply *reply = NULL;
	static struct sockaddr from_addr;
	socklen_t from_addr_len;
	int len;
	int ret;

	while (is_active) {
		fds.revents = 0;

		if (poll(&fds, nfds, -1) < 0) {
			LOG_ERR("Error in poll:%d", errno);
			errno = 0;
			k_sleep(K_MSEC(COAP_POOL_SLEEP));
			continue;
		}

		if (fds.revents & POLLERR) {
			LOG_ERR("Error in poll.. waiting a moment.");
			k_sleep(K_MSEC(COAP_POOL_SLEEP));
			continue;
		}

		if (fds.revents & POLLHUP) {
			LOG_ERR("Error in poll: POLLHUP");
			continue;
		}

		if (fds.revents & POLLNVAL) {
			LOG_ERR("Error in poll: POLLNVAL - fd not open");

			coap_close_socket(fds.fd);
			fds.fd = coap_open_socket();

			LOG_INF("Socket has been re-open");

			coap_bind_socket(fds.fd, socket_addr);

			continue;
		}

		if (!(fds.revents & POLLIN)) {
			LOG_ERR("Unknown poll error");
			continue;
		}

		len = recvfrom(fds.fd, buf, sizeof(buf) - 1, 0, &from_addr,
			       &from_addr_len);

		if (len < 0) {
			LOG_ERR("Error reading response: %d", errno);
			errno = 0;
			continue;
		}

		if (len == 0) {
			LOG_ERR("Zero length recv");
			continue;
		}

		ret = coap_packet_parse(&response, buf, len, NULL, 0);
		if (ret < 0) {
			LOG_ERR("Invalid data received");
			continue;
		}

		reply = coap_response_received(&response, &from_addr, replies,
					       COAP_MAX_REPLIES);
		if (reply) {
			coap_reply_clear(reply);
		}
	}
}

static int coap_init_request(enum coap_method method,
			     enum coap_msgtype msg_type,
			     const char *const *uri_path_options, u8_t *payload,
			     u16_t payload_size, struct coap_packet *request,
			     u8_t *buf)
{
	const char *const *opt;
	int ret;

	ret = coap_packet_init(request, buf, MAX_COAP_MSG_LEN, COAP_VER,
			       msg_type, COAP_TOKEN_LEN, coap_next_token(),
			       method, coap_next_id());
	if (ret < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	for (opt = uri_path_options; opt && *opt; opt++) {
		ret = coap_packet_append_option(request, COAP_OPTION_URI_PATH,
						*opt, strlen(*opt));
		if (ret < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	if (payload) {
		ret = coap_packet_append_payload_marker(request);
		if (ret < 0) {
			LOG_ERR("Unable to append payload marker");
			goto end;
		}

		ret = coap_packet_append_payload(request, payload,
						 payload_size);
		if (ret < 0) {
			LOG_ERR("Not able to append payload");
			goto end;
		}
	}

end:
	return ret;
}

static int coap_send_message(const struct sockaddr *addr,
			     struct coap_packet *request)
{
	return sendto(fds.fd, request->data, request->offset, 0, addr,
		      sizeof(*addr));
}

static void coap_set_response_callback(struct coap_packet *request,
				       coap_reply_t reply_cb)
{
	struct coap_reply *reply;

	reply = &replies[0];

	coap_reply_clear(reply);
	coap_reply_init(reply, request);
	reply->reply = reply_cb;
}

void coap_start(const struct sockaddr *addr)
{
	socket_addr = addr;

	fds.events = POLLIN;
	fds.revents = 0;
	fds.fd = coap_open_socket();

	coap_bind_socket(fds.fd, socket_addr);

	is_active = true;

	/* start sock receive thread */
	k_thread_create(&receive_thread_data, receive_stack_area,
			K_THREAD_STACK_SIZEOF(receive_stack_area),
			(k_thread_entry_t)coap_receive, NULL, NULL, NULL,
			/* Lowest priority cooperative thread */
			K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1), 0,
			K_NO_WAIT);
	k_thread_name_set(&receive_thread_data, "CoAP-sock-recv");
	LOG_DBG("CoAP socket receive thread started");
}

void coap_stop(void)
{
	is_active = false;
}

int coap_send_request(enum coap_method method, const struct sockaddr *addr,
		      const char *const *uri_path_options, u8_t *payload,
		      u16_t payload_size, coap_reply_t reply_cb)
{
	int ret;
	struct coap_packet request;
	u8_t buf[MAX_COAP_MSG_LEN];

	ret = coap_init_request(method, COAP_TYPE_NON_CON, uri_path_options,
				payload, payload_size, &request, buf);
	if (ret < 0) {
		goto end;
	}

	if (reply_cb != NULL) {
		coap_set_response_callback(&request, reply_cb);
	}

	ret = coap_send_message(addr, &request);
	if (ret < 0) {
		LOG_ERR("Transmission failed: %d", errno);
		goto end;
	}

end:
	return ret;
}
