/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/coap.h>
#include <net/coap_utils.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(coap_utils, CONFIG_COAP_UTILS_LOG_LEVEL);

#define MAX_COAP_MSG_LEN 256
#define COAP_VER 1
#define COAP_TOKEN_LEN 8
#define COAP_MAX_REPLIES 1
#define COAP_POOL_SLEEP 500
#define COAP_OPEN_SOCKET_SLEEP 200
#if defined(CONFIG_NRF_MODEM_LIB)
#define COAP_RECEIVE_STACK_SIZE 1096
#else
#define COAP_RECEIVE_STACK_SIZE 996
#endif

const static int nfds = 1;
static struct pollfd fds;
static struct coap_reply replies[COAP_MAX_REPLIES];
static int proto_family;
static struct sockaddr *bind_addr;

static K_THREAD_STACK_DEFINE(receive_stack_area, COAP_RECEIVE_STACK_SIZE);
static struct k_thread receive_thread_data;

static int coap_open_socket(void)
{
	int sock;

	while (1) {
		sock = socket(proto_family, SOCK_DGRAM, IPPROTO_UDP);
		if (sock < 0) {
			LOG_ERR("Failed to create socket %d", errno);
			k_sleep(K_MSEC(COAP_OPEN_SOCKET_SLEEP));
			continue;
		}
		break;
	}

	if (bind_addr) {
		if (bind(sock, bind_addr, sizeof(*bind_addr))) {
			LOG_ERR("Failed to bind socket, errno: %d", errno);
		}
	}

	return sock;
}

static void coap_close_socket(int socket)
{
	(void)close(socket);
}

static void coap_receive(void)
{
	static uint8_t buf[MAX_COAP_MSG_LEN + 1];
	struct coap_packet response;
	struct coap_reply *reply = NULL;
	static struct sockaddr from_addr;
	socklen_t from_addr_len;
	int len;
	int ret;

	while (1) {
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

			continue;
		}

		if (!(fds.revents & POLLIN)) {
			LOG_ERR("Unknown poll error");
			continue;
		}

		from_addr_len = sizeof(from_addr);
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
			     const char *const *uri_path_options, uint8_t *payload,
			     uint16_t payload_size, struct coap_packet *request,
			     uint8_t *buf)
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
						*(const uint8_t *const *)opt, strlen(*opt));
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

void coap_init(int ip_family, struct sockaddr *addr)
{
	proto_family = ip_family;
	if (addr) {
		bind_addr = addr;
	}

	fds.events = POLLIN;
	fds.revents = 0;
	fds.fd = coap_open_socket();

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

int coap_send_request(enum coap_method method, const struct sockaddr *addr,
		      const char *const *uri_path_options, uint8_t *payload,
		      uint16_t payload_size, coap_reply_t reply_cb)
{
	int ret;
	struct coap_packet request;
	uint8_t buf[MAX_COAP_MSG_LEN];

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
