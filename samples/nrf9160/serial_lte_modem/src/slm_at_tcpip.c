/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <net/socket.h>
#include "slm_at_tcpip.h"

LOG_MODULE_REGISTER(tcpip, CONFIG_SLM_LOG_LEVEL);

#define INVALID_SOCKET	-1
#define TCPIP_MAX_URL	128

/*
 * Known limitation in this version
 * - Multiple concurrent sockets
 * - Socket type other than SOCK_STREAM(1) and SOCK_DGRAM(2)
 * - IP Protocol other than TCP(6) and UDP(17)
 * - TCP/UDP server mode
 * - Receive more than IPv4 MTU one-time
 * - IPv6 support
 */

/**@brief Socket operations. */
enum slm_socket_operation {
	AT_SOCKET_CLOSE,
	AT_SOCKET_OPEN
};

/**@brief List of supported AT commands. */
enum slm_tcpip_at_cmd_type {
	AT_SOCKET,
	AT_BIND,
	AT_TCP_CONNECT,
	AT_TCP_SEND,
	AT_TCP_RECV,
	AT_UDP_SENDTO,
	AT_UDP_RECVFROM,
	AT_TCPIP_MAX
};

/** forward declaration of cmd handlers **/
static int handle_at_socket(enum at_cmd_type cmd_type);
static int handle_at_bind(enum at_cmd_type cmd_type);
static int handle_at_tcp_conn(enum at_cmd_type cmd_type);
static int handle_at_tcp_send(enum at_cmd_type cmd_type);
static int handle_at_tcp_recv(enum at_cmd_type cmd_type);
static int handle_at_udp_sendto(enum at_cmd_type cmd_type);
static int handle_at_udp_recvfrom(enum at_cmd_type cmd_type);

/**@brief SLM AT Command list type. */
static slm_at_cmd_list_t m_tcpip_at_list[AT_TCPIP_MAX] = {
	{AT_SOCKET, "AT#XSOCKET", handle_at_socket},
	{AT_BIND, "AT#XBIND", handle_at_bind},
	{AT_TCP_CONNECT, "AT#XTCPCONN", handle_at_tcp_conn},
	{AT_TCP_SEND, "AT#XTCPSEND", handle_at_tcp_send},
	{AT_TCP_RECV, "AT#XTCPRECV", handle_at_tcp_recv},
	{AT_UDP_SENDTO, "AT#XUDPSENDTO", handle_at_udp_sendto},
	{AT_UDP_RECVFROM, "AT#XUDPRECVFROM", handle_at_udp_recvfrom},
};

static struct sockaddr_storage remote;

static struct tcpip_client {
	int sock; /* Socket descriptor. */
	enum net_ip_protocol ip_proto; /* IP protocol */
	bool connected; /* TCP connected flag */
	at_cmd_handler_t callback;
} client;

static char buf[64];

/* global variable defined in different files */
extern struct at_param_list m_param_list;

/**@brief Check whether a string has valid IPv4 address or not
 */
static bool check_for_ipv4(const char *address, u8_t length)
{
	int index;

	for (index = 0; index < length; index++) {
		char ch = *(address + index);

		if ((ch == '.') || (ch >= '0' && ch <= '9')) {
			continue;
		} else {
			return false;
		}
	}

	return true;
}

/**@brief Resolves host IPv4 address and port
 */
static int parse_host_by_ipv4(const char *ip, u16_t port)
{
	struct sockaddr_in *address4 = ((struct sockaddr_in *)&remote);

	address4->sin_family = AF_INET;
	address4->sin_port = htons(port);
	LOG_DBG("IPv4 Address %s", log_strdup(ip));
	/* NOTE inet_pton() returns 1 as success */
	if (inet_pton(AF_INET, ip, &address4->sin_addr) == 1) {
		return 0;
	} else {
		return -EINVAL;
	}
}


static int parse_host_by_name(const char *name, u16_t port, int socktype)
{
	int err;
	struct addrinfo *result;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = socktype
	};
	char ipv4_addr[NET_IPV4_ADDR_LEN];

	err = getaddrinfo(name, NULL, &hints, &result);
	if (err) {
		LOG_ERR("ERROR: getaddrinfo failed %d", err);
		return err;
	}

	if (result == NULL) {
		LOG_ERR("ERROR: Address not found\n");
		return -ENOENT;
	}

	/* IPv4 Address. */
	struct sockaddr_in *server4 = ((struct sockaddr_in *)&remote);

	server4->sin_addr.s_addr =
		((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	server4->sin_family = AF_INET;
	server4->sin_port = htons(port);

	inet_ntop(AF_INET, &server4->sin_addr.s_addr, ipv4_addr,
		  sizeof(ipv4_addr));
	LOG_DBG("IPv4 Address found %s\n", ipv4_addr);

	/* Free the address. */
	freeaddrinfo(result);

	return 0;
}

static int do_socket_open(u8_t type)
{
	int ret = 0;

	if (type == SOCK_STREAM) {
		client.sock = socket(AF_INET, SOCK_STREAM,
				IPPROTO_TCP);
		client.ip_proto = IPPROTO_TCP;
	} else if (type == SOCK_DGRAM) {
		client.sock = socket(AF_INET, SOCK_DGRAM,
				IPPROTO_UDP);
		client.ip_proto = IPPROTO_UDP;
	}
	if (client.sock < 0) {
		LOG_ERR("socket() failed: %d", -errno);
		sprintf(buf, "#XSOCKET: %d\r\n", -errno);
		client.callback(buf);
		client.ip_proto = IPPROTO_IP;
		ret = -errno;
	} else {
		sprintf(buf, "#XSOCKET: %d, %d\r\n", client.sock,
			client.ip_proto);
		client.callback(buf);
	}

	LOG_DBG("Socket opened");
	return ret;
}

static int do_socket_close(int error)
{
	int ret = 0;

	if (client.sock > 0) {
		ret = close(client.sock);
		if (ret < 0) {
			LOG_WRN("close() failed: %d", -errno);
			ret = -errno;
		}
		client.sock = INVALID_SOCKET;
		client.ip_proto = IPPROTO_IP;
		client.connected = false;

		sprintf(buf, "#XSOCKET: %d\r\n", error);
		client.callback(buf);
		LOG_DBG("Socket closed");
	}

	return ret;
}

static int do_bind(u16_t port)
{
	int ret;
	struct sockaddr_in local;

	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(client.sock, (struct sockaddr *)&local,
		 sizeof(struct sockaddr_in));
	if (ret < 0) {
		LOG_ERR("bind() failed: %d", -errno);
		do_socket_close(-errno);
		return -errno;
	}

	return 0;
}

static int do_tcp_connect(const char *url, u16_t port)
{
	int ret;

	LOG_DBG("%s:%d", log_strdup(url), port);

	if (check_for_ipv4(url, strlen(url))) {
		ret = parse_host_by_ipv4(url, port);
	} else {
		ret = parse_host_by_name(url, port, SOCK_STREAM);
	}
	if (ret) {
		LOG_ERR("Parse failed: %d", ret);
		return ret;
	}

	ret = connect(client.sock, (struct sockaddr *)&remote,
		 sizeof(struct sockaddr_in));
	if (ret < 0) {
		LOG_ERR("connect() failed: %d", -errno);
		do_socket_close(-errno);
		return -errno;
	}

	client.connected = true;
	client.callback("#XTCPCONN: 1\r\n");
	return 0;
}

static int do_tcp_send(const char *data)
{
	u32_t offset = 0;
	u32_t datalen = strlen(data);
	int ret;

	while (offset < datalen) {
		ret = send(client.sock, data + offset,
			   datalen - offset, 0);
		if (ret < 0) {
			do_socket_close(-errno);
			LOG_WRN("send() failed: %d", -errno);
			break;
		}

		offset += ret;
	}

	sprintf(buf, "#XTCPSEND: %d\r\n", offset);
	client.callback(buf);
	LOG_DBG("TCP sent");
	return 0;
}

static int do_tcp_receive(u16_t length, u16_t time)
{
	int ret;
	char data[NET_IPV4_MTU];
	struct timeval tmo = {
		.tv_sec = time
	};

	ret = setsockopt(client.sock, SOL_SOCKET, SO_RCVTIMEO,
			&tmo, sizeof(struct timeval));
	if (ret < 0) {
		do_socket_close(-errno);
		LOG_ERR("setsockopt() error: %d", -errno);
		return ret;
	}

	if (length > NET_IPV4_MTU) {
		ret = recv(client.sock, data, NET_IPV4_MTU, 0);
	} else {
		ret = recv(client.sock, data, length, 0);
	}
	if (ret < 0) {
		LOG_WRN("recv() error: %d", -errno);
		do_socket_close(-errno);
		ret = -errno;
	} else if (ret == 0) {
		/**
		 * When a stream socket peer has performed an orderly shutdown,
		 * the return value will be 0 (the traditional "end-of-file")
		 * The value 0 may also be returned if the requested number of
		 * bytes to receive from a stream socket was 0
		 * In both cases, treat as normal shutdown by remote
		 */
		LOG_WRN("recv() return 0");
		do_socket_close(0);
	} else {
		data[ret] = '\0';
		client.callback("#XTCPRECV: ");
		client.callback(data);
		client.callback("\r\n");
		sprintf(buf, "#XTCPRECV: %d\r\n", ret);
		client.callback(buf);
		ret = 0;
	}

	LOG_DBG("TCP received");
	return ret;
}

static int do_udp_init(const char *url, u16_t port)
{
	int ret;

	if (check_for_ipv4(url, strlen(url))) {
		ret = parse_host_by_ipv4(url, port);
	} else {
		ret = parse_host_by_name(url, port, SOCK_DGRAM);
	}
	if (ret) {
		LOG_ERR("Parse failed: %d", ret);
		return ret;
	}

	LOG_DBG("UDP initialized");
	return 0;
}

static int do_udp_sendto(const char *url, u16_t port, const char *data)
{
	u32_t offset = 0;
	u32_t datalen = strlen(data);
	int ret;

	ret = do_udp_init(url, port);
	if (ret < 0) {
		return ret;
	};

	while (offset < datalen) {
		ret = sendto(client.sock, data + offset,
			   datalen - offset, 0,
			   (struct sockaddr *)&remote,
			   sizeof(struct sockaddr_in));
		if (ret <= 0) {
			LOG_ERR("sendto() failed: %d", -errno);
			do_socket_close(-errno);
			return -errno;
		}

		offset += ret;
	}

	sprintf(buf, "#XUDPSENDTO: %d\r\n", offset);
	client.callback(buf);
	LOG_DBG("UDP sent");
	return 0;
}

static int do_udp_recvfrom(const char *url, u16_t port, u16_t length,
			u16_t time)
{
	int ret;
	char data[NET_IPV4_MTU];
	int sockaddr_len = sizeof(struct sockaddr);
	struct timeval tmo = {
		.tv_sec = time
	};

	ret = do_udp_init(url, port);
	if (ret < 0) {
		return ret;
	};

	ret = setsockopt(client.sock, SOL_SOCKET, SO_RCVTIMEO,
			&tmo, sizeof(struct timeval));
	if (ret < 0) {
		LOG_ERR("setsockopt() error: %d", -errno);
		do_socket_close(-errno);
		return ret;
	}

	if (length > NET_IPV4_MTU) {
		ret = recvfrom(client.sock, data, NET_IPV4_MTU, 0,
			(struct sockaddr *)&remote, &sockaddr_len);
	} else {
		ret = recvfrom(client.sock, data, length, 0,
			(struct sockaddr *)&remote, &sockaddr_len);
	}
	if (ret < 0) {
		LOG_WRN("recvfrom() error: %d", -errno);
		do_socket_close(-errno);
		ret = -errno;
	} else {
		/**
		 * Datagram sockets in various domains permit zero-length
		 * datagrams. When such a datagram is received, the return
		 * value is 0. Treat as normal case
		 */
		data[ret] = '\0';
		client.callback("#XUDPRECV: ");
		client.callback(data);
		client.callback("\r\n");
		sprintf(buf, "#XUDPRECV: %d\r\n", ret);
		client.callback(buf);
		ret = 0;
	}

	LOG_DBG("UDP received");
	return ret;
}

/**@brief handle AT#XSOCKET commands
 *  AT#XSOCKET=<op>[,<type>]
 *  AT#XSOCKET?
 *  AT#XSOCKET=? TEST command not supported
 */
static int handle_at_socket(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	u16_t op;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 2) {
			return -EINVAL;
		}
		err = at_params_short_get(&m_param_list, 1, &op);
		if (err < 0) {
			return err;
		};
		if (op == 1) {
			u16_t type;

			if (at_params_valid_count_get(&m_param_list) < 3) {
				return -EINVAL;
			}
			err = at_params_short_get(&m_param_list, 2, &type);
			if (err < 0) {
				return err;
			};
			if (client.sock > 0) {
				LOG_WRN("Socket is already opened");
			} else {
				err = do_socket_open(type);
			}
		} else if (op == 0) {
			if (client.sock < 0) {
				LOG_WRN("Socket is not opened yet");
			} else {
				err = do_socket_close(0);
			}
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (client.sock != INVALID_SOCKET) {
			sprintf(buf, "#XSOCKET: %d, %d\r\n", client.sock,
				client.ip_proto);
		} else {
			sprintf(buf, "#XSOCKET: 0\r\n");
		}
		client.callback(buf);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XBIND commands
 *  AT#XBIND=<port>
 *  AT#XBIND?
 *  AT#XBIND=? TEST command not supported
 */
static int handle_at_bind(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	u16_t port;

	if (client.sock < 0) {
		LOG_ERR("Socket not opened yet");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 2) {
			return -EINVAL;
		}
		err = at_params_short_get(&m_param_list, 1, &port);
		if (err < 0) {
			return err;
		};
		err = do_bind(port);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XTCPCONN commands
 *  AT#XTCPCONN=<url>,<port>
 *  AT#XTCPCONN?
 *  AT#XTCPCONN=? TEST command not supported
 */
static int handle_at_tcp_conn(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char url[TCPIP_MAX_URL];
	int size = TCPIP_MAX_URL;
	u16_t port;

	if (client.sock < 0) {
		LOG_ERR("Socket not opened yet");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 3) {
			return -EINVAL;
		}
		err = at_params_string_get(&m_param_list, 1, url, &size);
		if (err < 0) {
			return err;
		};
		url[size] = '\0';
		err = at_params_short_get(&m_param_list, 2, &port);
		if (err < 0) {
			return err;
		};
		err = do_tcp_connect(url, port);
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (client.connected) {
			client.callback("+XTCPCONN: 1\r\n");
		} else {
			client.callback("+XTCPCONN: 0\r\n");
		}
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XTCPSEND commands
 *  AT#XTCPSEND=<data>
 *  AT#XTCPSEND? READ command not supported
 *  AT#XTCPSEND=? TEST command not supported
 */
static int handle_at_tcp_send(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char data[NET_IPV4_MTU];
	int size = NET_IPV4_MTU;

	if (!client.connected) {
		LOG_ERR("TCP not connected yet");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 2) {
			return -EINVAL;
		}
		err = at_params_string_get(&m_param_list, 1, data, &size);
		if (err < 0) {
			return err;
		};
		data[size] = '\0';
		err = do_tcp_send(data);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XTCPRECV commands
 *  AT#XTCPRECV=<length>,<timeout>
 *  AT#XTCPRECV? READ command not supported
 *  AT#XTCPRECV=? TEST command not supported
 */
static int handle_at_tcp_recv(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	u16_t length, time;

	if (!client.connected) {
		LOG_ERR("TCP not connected yet");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 3) {
			return -EINVAL;
		}
		err = at_params_short_get(&m_param_list, 1, &length);
		if (err < 0) {
			return err;
		};
		err = at_params_short_get(&m_param_list, 2, &time);
		if (err < 0) {
			return err;
		};
		err = do_tcp_receive(length, time);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XUDPSENDTO commands
 *  AT#XUDPSENDTO=<url>,<port>,<data>
 *  AT#XUDPSENDTO? READ command not supported
 *  AT#XUDPSENDTO=? TEST command not supported
 */
static int handle_at_udp_sendto(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char url[TCPIP_MAX_URL];
	u16_t port;
	char data[NET_IPV4_MTU];
	int size;

	if (client.sock < 0) {
		LOG_ERR("Socket not opened yet");
		return err;
	} else if (client.ip_proto != IPPROTO_UDP) {
		LOG_ERR("Invalid socket");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 4) {
			return -EINVAL;
		}
		size = TCPIP_MAX_URL;
		err = at_params_string_get(&m_param_list, 1, url, &size);
		if (err < 0) {
			return err;
		};
		url[size] = '\0';
		err = at_params_short_get(&m_param_list, 2, &port);
		if (err < 0) {
			return err;
		};
		size = NET_IPV4_MTU;
		err = at_params_string_get(&m_param_list, 3, data, &size);
		if (err < 0) {
			return err;
		};
		data[size] = '\0';
		err = do_udp_sendto(url, port, data);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XUDPRECVFROM commands
 *  AT#XUDPRECVFROM=<url>,<port>,<length>,<timeout>
 *  AT#XUDPRECVFROM? READ command not supported
 *  AT#XUDPRECVFROM=? TEST command not supported
 */
static int handle_at_udp_recvfrom(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char url[TCPIP_MAX_URL];
	int size = TCPIP_MAX_URL;
	u16_t port, length, time;

	if (client.sock < 0) {
		LOG_ERR("Socket not opened yet");
		return err;
	} else if (client.ip_proto != IPPROTO_UDP) {
		LOG_ERR("Invalid socket");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 5) {
			return -EINVAL;
		}
		err = at_params_string_get(&m_param_list, 1, url, &size);
		if (err < 0) {
			return err;
		};
		url[size] = '\0';
		err = at_params_short_get(&m_param_list, 2, &port);
		if (err < 0) {
			return err;
		};
		err = at_params_short_get(&m_param_list, 3, &length);
		if (err < 0) {
			return err;
		};
		err = at_params_short_get(&m_param_list, 4, &time);
		if (err < 0) {
			return err;
		};
		err = do_udp_recvfrom(url, port, length, time);
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to handle TCP/IP AT commands
 */
int slm_at_tcpip_parse(const char *at_cmd)
{
	int ret = -ENOTSUP;
	enum at_cmd_type type;

	for (int i = 0; i < AT_TCPIP_MAX; i++) {
		u8_t cmd_len = strlen(m_tcpip_at_list[i].string);

		if (slm_at_cmd_cmp(at_cmd, m_tcpip_at_list[i].string,
			cmd_len)) {
			ret = at_parser_params_from_str(at_cmd, NULL,
						&m_param_list);
			if (ret < 0) {
				LOG_ERR("Failed to parse AT command %d", ret);
				return -EINVAL;
			}
			type = at_parser_cmd_type_get(at_cmd);
			ret = m_tcpip_at_list[i].handler(type);
			break;
		}
	}

	return ret;
}

/**@brief API to initialize TCP/IP AT commands handler
 */
int slm_at_tcpip_init(at_cmd_handler_t callback)
{
	if (callback == NULL) {
		LOG_ERR("No callback");
		return -EINVAL;
	}
	client.sock = INVALID_SOCKET;
	client.connected = false;
	client.ip_proto = IPPROTO_IP;
	client.callback = callback;
	return 0;
}
