/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <net/socket.h>
#include <net/tls_credentials.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_udp_proxy.h"

LOG_MODULE_REGISTER(slm_udp, CONFIG_SLM_LOG_LEVEL);

#define THREAD_STACK_SIZE	(KB(1) + NET_IPV4_MTU)
#define THREAD_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO

/*
 * Known limitation in this version
 * - Multiple concurrent
 * - Receive more than IPv4 MTU one-time
 * - IPv6 support
 * - does not support proxy
 */

/**@brief Proxy operations. */
enum slm_udp_proxy_operation {
	AT_SERVER_STOP,
	AT_CLIENT_DISCONNECT = AT_SERVER_STOP,
	AT_SERVER_START,
	AT_CLIENT_CONNECT = AT_SERVER_START,
	AT_SERVER_START_WITH_DATAMODE,
	AT_CLIENT_CONNECT_WITH_DATAMODE = AT_SERVER_START_WITH_DATAMODE
};

static struct k_thread udp_thread;
static K_THREAD_STACK_DEFINE(udp_thread_stack, THREAD_STACK_SIZE);
static k_tid_t udp_thread_id;

static struct sockaddr_in remote;
static int udp_sock;
static bool udp_server_role;
static bool udp_datamode;

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2];
extern uint8_t rx_data[CONFIG_SLM_SOCKET_RX_MAX];

/** forward declaration of thread function **/
static void udp_thread_func(void *p1, void *p2, void *p3);

static int do_udp_server_start(uint16_t port)
{
	int ret = 0;
	struct sockaddr_in local;
	int addr_len;
	char ipv4_addr[NET_IPV4_ADDR_LEN] = {0};

	/* Open socket */
	udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_sock < 0) {
		LOG_ERR("socket() failed: %d", -errno);
		sprintf(rsp_buf, "\r\n#XUDPSVR: %d\r\n", -errno);
		rsp_send(rsp_buf, strlen(rsp_buf));
		return -errno;
	}

	/* Bind to local port */
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	util_get_ip_addr(ipv4_addr, NULL);
	if (strlen(ipv4_addr) == 0) {
		LOG_ERR("Unable to obtain local IPv4 address");
		close(udp_sock);
		return ret;
	}
	addr_len = strlen(ipv4_addr);
	if (addr_len == 0) {
		LOG_ERR("LTE not connected yet");
		close(udp_sock);
		return -EINVAL;
	}
	if (inet_pton(AF_INET, ipv4_addr, &local.sin_addr) != 1) {
		LOG_ERR("Parse local IP address failed: %d", -errno);
		close(udp_sock);
		return -EINVAL;
	}

	ret = bind(udp_sock, (struct sockaddr *)&local,
		 sizeof(struct sockaddr_in));
	if (ret) {
		LOG_ERR("bind() failed: %d", -errno);
		sprintf(rsp_buf, "\r\n#XUDPSVR: %d\r\n", -errno);
		rsp_send(rsp_buf, strlen(rsp_buf));
		close(udp_sock);
		return -errno;
	}

	udp_thread_id = k_thread_create(&udp_thread, udp_thread_stack,
			K_THREAD_STACK_SIZEOF(udp_thread_stack),
			udp_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);

	udp_server_role = true;
	sprintf(rsp_buf, "\r\n#XUDPSVR: %d,\"started\"\r\n", udp_sock);
	rsp_send(rsp_buf, strlen(rsp_buf));
	LOG_DBG("UDP server started");

	return ret;
}

static int do_udp_server_stop(int error)
{
	int ret = 0;

	if (udp_sock != INVALID_SOCKET) {
		ret = close(udp_sock);
		if (ret < 0) {
			LOG_WRN("close() failed: %d", -errno);
			ret = -errno;
		}
		(void)slm_at_udp_proxy_init();
		sprintf(rsp_buf, "\r\n#XUDPSVR: %d,\"stopped\"\r\n", error);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}

	return ret;
}

static int do_udp_client_connect(const char *url, uint16_t port, int sec_tag)
{
	int ret;

	/* Open socket */
	if (sec_tag == INVALID_SEC_TAG) {
		udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	} else {
		udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_DTLS_1_2);

	}
	if (udp_sock < 0) {
		LOG_ERR("socket() failed: %d", -errno);
		sprintf(rsp_buf, "\r\n#XUDPCLI: %d\r\n", -errno);
		rsp_send(rsp_buf, strlen(rsp_buf));
		return -errno;
	}
	if (sec_tag != INVALID_SEC_TAG) {
		sec_tag_t sec_tag_list[1] = { sec_tag };

		ret = setsockopt(udp_sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("set tag list failed: %d", -errno);
			sprintf(rsp_buf, "\r\n#XUDPCLI: %d\r\n", -errno);
			rsp_send(rsp_buf, strlen(rsp_buf));
			close(udp_sock);
			return -errno;
		}
	}

	/* Connect to remote host */
	struct addrinfo *result;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_DGRAM
	};

	ret = getaddrinfo(url, NULL, &hints, &result);
	if (ret || result == NULL) {
		LOG_ERR("getaddrinfo() failed: %d", ret);
		close(udp_sock);
		return -EINVAL;
	}

	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);
	remote.sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	/* Free the address. */
	freeaddrinfo(result);

	ret = connect(udp_sock, (struct sockaddr *)&remote,
		sizeof(struct sockaddr_in));
	if (ret < 0) {
		LOG_ERR("connect() failed: %d", -errno);
		sprintf(rsp_buf, "\r\n#XUDPCLI: %d\r\n", -errno);
		rsp_send(rsp_buf, strlen(rsp_buf));
		close(udp_sock);
		return -errno;
	}

	udp_thread_id = k_thread_create(&udp_thread, udp_thread_stack,
			K_THREAD_STACK_SIZEOF(udp_thread_stack),
			udp_thread_func, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);

	udp_server_role = false;
	sprintf(rsp_buf, "\r\n#XUDPCLI: %d,\"connected\"\r\n", udp_sock);
	rsp_send(rsp_buf, strlen(rsp_buf));

	return ret;
}

static int do_udp_client_disconnect(void)
{
	int ret = 0;

	if (udp_sock != INVALID_SOCKET) {
		ret = close(udp_sock);
		if (ret < 0) {
			LOG_WRN("close() failed: %d", -errno);
			ret = -errno;
		}
		(void)slm_at_udp_proxy_init();
		sprintf(rsp_buf, "\r\n#XUDPCLI: \"disconnected\"\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
	}

	return ret;
}

static int do_udp_send(const uint8_t *data, int datalen)
{
	int ret = 0;
	uint32_t offset = 0;

	if (udp_sock == INVALID_SOCKET) {
		LOG_ERR("Not connected yet");
		return -EINVAL;
	}

	while (offset < datalen) {
		if (udp_server_role) {
			ret = sendto(udp_sock, data + offset, datalen - offset,
				0, (struct sockaddr *)&remote, sizeof(remote));
		} else {
			ret = send(udp_sock, data + offset, datalen - offset,
				0);
		}
		if (ret < 0) {
			LOG_ERR("send() failed: %d", -errno);
			if (errno != EAGAIN && errno != ETIMEDOUT) {
				sprintf(rsp_buf, "\r\n#XUDPSEND: %d\r\n", -errno);
				rsp_send(rsp_buf, strlen(rsp_buf));
				if (udp_server_role) {
					do_udp_server_stop(-errno);
				} else {
					do_udp_client_disconnect();
				}
			}
			ret = -errno;
			break;
		}
		offset += ret;
	}

	if (ret >= 0) {
		sprintf(rsp_buf, "\r\n#XUDPSEND: %d\r\n", offset);
		rsp_send(rsp_buf, strlen(rsp_buf));
		return 0;
	} else {
		return ret;
	}
}

static int do_udp_send_datamode(const uint8_t *data, int datalen)
{
	int ret = 0;
	uint32_t offset = 0;

	while (offset < datalen) {
		if (udp_server_role) {
			ret = sendto(udp_sock, data + offset, datalen - offset,
				0, (struct sockaddr *)&remote, sizeof(remote));
		} else {
			ret = send(udp_sock, data + offset, datalen - offset,
				0);
		}
		if (ret < 0) {
			LOG_ERR("send() failed: %d", -errno);
			if (errno != EAGAIN && errno != ETIMEDOUT) {
				(void)exit_datamode();
				if (udp_server_role) {
					do_udp_server_stop(-errno);
				} else {
					do_udp_client_disconnect();
				}
			}
			break;
		}
		offset += ret;
	}

	return offset;
}

static void udp_thread_func(void *p1, void *p2, void *p3)
{
	int ret;
	int size = sizeof(struct sockaddr_in);
	struct pollfd fds;
	char rx_data[SLM_MAX_PAYLOAD];

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fds.fd = udp_sock;
	fds.events = POLLIN;

	do {
		ret = poll(&fds, 1, MSEC_PER_SEC * CONFIG_SLM_UDP_POLL_TIME);
		if (ret < 0) {  /* IO error */
			LOG_WRN("poll() error: %d", ret);
			continue;
		}
		if (ret == 0) {  /* timeout */
			continue;
		}
		LOG_DBG("Poll events 0x%08x", fds.revents);
		if ((fds.revents & POLLERR) == POLLERR) {
			LOG_DBG("Socket error");
			return;
		}
		if ((fds.revents & POLLNVAL) == POLLNVAL) {
			LOG_DBG("Socket closed");
			return;
		}
		if ((fds.revents & POLLIN) != POLLIN) {
			continue;
		} else {
			if (udp_server_role) {
				ret = recvfrom(udp_sock, (void *)rx_data, sizeof(rx_data), 0,
					(struct sockaddr *)&remote, &size);
			} else {
				ret = recv(udp_sock, (void *)rx_data, sizeof(rx_data), 0);
			}
		}
		if (ret < 0) {
			LOG_WRN("recv() error: %d", -errno);
			continue;
		}
		if (ret == 0) {
			continue;
		}
		if (udp_datamode) {
			rsp_send(rx_data, ret);
		} else {
			sprintf(rsp_buf, "\r\n#XUDPDATA: %d\r\n", ret);
			rsp_send(rsp_buf, strlen(rsp_buf));
			rsp_send(rx_data, ret);
			rsp_send("\r\n", 2);
		}
	} while (true);

	LOG_DBG("Quit receive thread");
}

static int udp_datamode_callback(uint8_t op, const uint8_t *data, int len)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		ret = do_udp_send_datamode(data, len);
		LOG_INF("datamode send: %d", ret);
	} else if (op == DATAMODE_EXIT) {
		udp_datamode = false;
	}

	return ret;
}

/**@brief handle AT#XUDPSVR commands
 *  AT#XUDPSVR=<op>[,<port>]
 *  AT#XUDPSVR? READ command not supported
 *  AT#XUDPSVR=?
 */
int handle_at_udp_server(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t port;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == AT_SERVER_START || op == AT_SERVER_START_WITH_DATAMODE) {
			err = at_params_unsigned_short_get(&at_param_list, 2, &port);
			if (err) {
				return err;
			}
			if (udp_sock > 0) {
				LOG_WRN("Server is running");
				return -EINVAL;
			}
			err = do_udp_server_start((uint16_t)port);
			if (err == 0 && op == AT_SERVER_START_WITH_DATAMODE) {
				udp_datamode = true;
				enter_datamode(udp_datamode_callback);
			}
		} else if (op == AT_SERVER_STOP) {
			if (udp_sock < 0) {
				LOG_WRN("Server is not running");
				return -EINVAL;
			}
			err = do_udp_server_stop(0);
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		sprintf(rsp_buf, "\r\n#XUDPSVR: %d,%d\r\n", udp_sock, udp_datamode);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "\r\n#XUDPSVR: (%d,%d,%d),<port>,<sec_tag>\r\n",
			AT_SERVER_STOP, AT_SERVER_START, AT_SERVER_START_WITH_DATAMODE);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XUDPCLI commands
 *  AT#XUDPCLI=<op>[,<url>,<port>[,<sec_tag>]
 *  AT#XUDPCLI? READ command not supported
 *  AT#XUDPCLI=?
 */
int handle_at_udp_client(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == AT_CLIENT_CONNECT || op == AT_CLIENT_CONNECT_WITH_DATAMODE) {
			uint16_t port;
			char url[SLM_MAX_URL];
			int size = SLM_MAX_URL;
			sec_tag_t sec_tag = INVALID_SEC_TAG;

			err = util_string_get(&at_param_list, 2, url, &size);
			if (err) {
				return err;
			}
			err = at_params_unsigned_short_get(&at_param_list, 3, &port);
			if (err) {
				return err;
			}
			if (at_params_valid_count_get(&at_param_list) > 4) {
				at_params_unsigned_int_get(&at_param_list, 4, &sec_tag);
			}
			err = do_udp_client_connect(url, (uint16_t)port, sec_tag);
			if (err == 0 && op == AT_CLIENT_CONNECT_WITH_DATAMODE) {
				udp_datamode = true;
				enter_datamode(udp_datamode_callback);
			}
		} else if (op == AT_CLIENT_DISCONNECT) {
			if (udp_sock < 0) {
				LOG_WRN("Client is not connected");
				return -EINVAL;
			}
			err = do_udp_client_disconnect();
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		sprintf(rsp_buf, "\r\n#XUDPCLI: %d,%d\r\n", udp_sock, udp_datamode);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "\r\n#XUDPCLI: (%d, %d, %d),<url>,<port>,<sec_tag>\r\n",
			AT_CLIENT_DISCONNECT, AT_CLIENT_CONNECT, AT_CLIENT_CONNECT_WITH_DATAMODE);
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XUDPSEND commands
 *  AT#XUDPSEND=<data>
 *  AT#XUDPSEND? READ command not supported
 *  AT#XUDPSEND=? TEST command not supported
 */
int handle_at_udp_send(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char data[NET_IPV4_MTU];
	int size = NET_IPV4_MTU;

	if (remote.sin_family == AF_UNSPEC) {
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = util_string_get(&at_param_list, 1, data, &size);
		if (err) {
			return err;
		}
		err = do_udp_send(data, size);
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to initialize UDP Proxy AT commands handler
 */
int slm_at_udp_proxy_init(void)
{
	udp_sock = INVALID_SOCKET;
	udp_datamode = false;
	udp_server_role = false;
	remote.sin_family = AF_UNSPEC;

	return 0;
}

/**@brief API to uninitialize UDP Proxy AT commands handler
 */
int slm_at_udp_proxy_uninit(void)
{
	int ret = 0;

	if (udp_sock != INVALID_SOCKET) {
		k_thread_abort(udp_thread_id);
		ret = close(udp_sock);
		if (ret < 0) {
			LOG_WRN("close() failed: %d", -errno);
			ret = -errno;
		}
		udp_sock = INVALID_SOCKET;
	}

	return ret;
}
