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
#include <modem_info.h>
#include <net/tls_credentials.h>
#include "slm_at_host.h"
#include "slm_at_tcpip.h"

LOG_MODULE_REGISTER(tcpip, CONFIG_SLM_LOG_LEVEL);

#define INVALID_SOCKET	-1
#define INVALID_SEC_TAG	-1
#define TCPIP_MAX_URL	128

/*
 * Known limitation in this version
 * - Multiple concurrent sockets
 * - Socket type other than SOCK_STREAM(1) and SOCK_DGRAM(2)
 * - IP Protocol other than TCP(6) and UDP(17)
 * - TCP server accept one connection only
 * - Receive more than IPv4 MTU one-time
 * - IPv6 support
 * - does not support proxy
 */

/**@brief Socket operations. */
enum slm_socket_operation {
	AT_SOCKET_CLOSE,
	AT_SOCKET_OPEN,
};

/**@brief Socketopt operations. */
enum slm_socketopt_operation {
	AT_SOCKETOPT_GET,
	AT_SOCKETOPT_SET
};

/**@brief Socket roles. */
enum slm_socket_role {
	AT_SOCKET_ROLE_CLIENT,
	AT_SOCKET_ROLE_SERVER
};

/**@brief List of supported AT commands. */
enum slm_tcpip_at_cmd_type {
	AT_SOCKET,
	AT_SOCKETOPT,
	AT_BIND,
	AT_CONNECT,
	AT_LISTEN,
	AT_ACCEPT,
	AT_SEND,
	AT_RECV,
	AT_SENDTO,
	AT_RECVFROM,
	AT_GETADDRINFO,
	AT_TCPIP_MAX
};

/** forward declaration of cmd handlers **/
static int handle_at_socket(enum at_cmd_type cmd_type);
static int handle_at_socketopt(enum at_cmd_type cmd_type);
static int handle_at_bind(enum at_cmd_type cmd_type);
static int handle_at_connect(enum at_cmd_type cmd_type);
static int handle_at_listen(enum at_cmd_type cmd_type);
static int handle_at_accept(enum at_cmd_type cmd_type);
static int handle_at_send(enum at_cmd_type cmd_type);
static int handle_at_recv(enum at_cmd_type cmd_type);
static int handle_at_sendto(enum at_cmd_type cmd_type);
static int handle_at_recvfrom(enum at_cmd_type cmd_type);
static int handle_at_getaddrinfo(enum at_cmd_type cmd_type);

#if defined(CONFIG_SLM_TEST_MODE)
static int do_recv(u16_t length);
static int do_recvfrom(const char *url, u16_t port, u16_t length);
#endif

/**@brief SLM AT Command list type. */
static slm_at_cmd_list_t m_tcpip_at_list[AT_TCPIP_MAX] = {
	{AT_SOCKET, "AT#XSOCKET", handle_at_socket},
	{AT_SOCKETOPT, "AT#XSOCKETOPT", handle_at_socketopt},
	{AT_BIND, "AT#XBIND", handle_at_bind},
	{AT_CONNECT, "AT#XCONNECT", handle_at_connect},
	{AT_LISTEN, "AT#XLISTEN", handle_at_listen},
	{AT_ACCEPT, "AT#XACCEPT", handle_at_accept},
	{AT_SEND, "AT#XSEND", handle_at_send},
	{AT_RECV, "AT#XRECV", handle_at_recv},
	{AT_SENDTO, "AT#XSENDTO", handle_at_sendto},
	{AT_RECVFROM, "AT#XRECVFROM", handle_at_recvfrom},
	{AT_GETADDRINFO, "AT#XGETADDRINFO", handle_at_getaddrinfo},
};

static struct sockaddr_in remote;

static struct tcpip_client {
	int sock; /* Socket descriptor. */
	int role; /* Client or Server role */
	int sock_peer; /* Socket descriptor for peer. */
	int ip_proto; /* IP protocol */
	bool connected; /* TCP connected flag */
	at_cmd_handler_t callback;
} client;

static char buf[64];

/* global variable defined in different files */
extern struct at_param_list m_param_list;
extern struct modem_param_info modem_param;

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
	LOG_DBG("IPv4 Address found %s\n", log_strdup(ipv4_addr));

	/* Free the address. */
	freeaddrinfo(result);

	return 0;
}

static int do_socket_open(u8_t type, u8_t role, int sec_tag)
{
	int ret = 0;

	if (type == SOCK_STREAM) {
		if (sec_tag == INVALID_SEC_TAG) {
			client.sock = socket(AF_INET, SOCK_STREAM,
					IPPROTO_TCP);
			client.ip_proto = IPPROTO_TCP;
		} else {
			client.sock = socket(AF_INET, SOCK_STREAM,
					IPPROTO_TLS_1_2);
			client.ip_proto = IPPROTO_TLS_1_2;
		}
	} else if (type == SOCK_DGRAM) {
		if (sec_tag == INVALID_SEC_TAG) {
			client.sock = socket(AF_INET, SOCK_DGRAM,
					IPPROTO_UDP);
			client.ip_proto = IPPROTO_UDP;
		} else {
			client.sock = socket(AF_INET, SOCK_DGRAM,
					IPPROTO_DTLS_1_2);
			client.ip_proto = IPPROTO_DTLS_1_2;
		}
	} else {
		LOG_ERR("socket type %d not supported", type);
		return -ENOTSUP;
	}
	if (client.sock < 0) {
		LOG_ERR("socket() failed: %d", -errno);
		sprintf(buf, "#XSOCKET: %d\r\n", -errno);
		client.callback(buf);
		client.ip_proto = IPPROTO_IP;
		ret = -errno;
	}

	if (sec_tag != INVALID_SEC_TAG) {
		sec_tag_t sec_tag_list[1] = { sec_tag };

		if (role == AT_SOCKET_ROLE_SERVER) {
			client.callback("#XSOCKET: (D)TLS Server not ");
			client.callback("supported\r\n");
			close(client.sock);
			client.sock = INVALID_SOCKET;
			return -ENOTSUP;
		}

		ret = setsockopt(client.sock, SOL_TLS, TLS_SEC_TAG_LIST,
				sec_tag_list, sizeof(sec_tag_t));
		if (ret) {
			LOG_ERR("set tag list failed: %d", -errno);
			close(client.sock);
			return -errno;
		}

	}

	client.role = role;
	sprintf(buf, "#XSOCKET: %d, %d, %d, %d\r\n", client.sock,
		type, role, client.ip_proto);
	client.callback(buf);

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
		if (client.sock_peer > 0) {
			close(client.sock_peer);
		}
		client.role = AT_SOCKET_ROLE_CLIENT;
		client.sock_peer = INVALID_SOCKET;
		client.connected = false;

		sprintf(buf, "#XSOCKET: %d, closed\r\n", error);
		client.callback(buf);
		LOG_DBG("Socket closed");
	}

	return ret;
}

static int do_socketopt_set(int name, int value)
{
	int ret = -ENOTSUP;

	switch (name) {
	case SO_REUSEADDR:	/* Ignored by Zephyr */
	case SO_ERROR:		/* Ignored by Zephyr */
		sprintf(buf, "#XSOCKETOPT: ignored\r\n");
		client.callback(buf);
		ret = 0;
		break;

	case SO_RCVTIMEO: {
		struct timeval tmo = { .tv_sec = value };

		ret = setsockopt(client.sock, SOL_SOCKET, SO_RCVTIMEO,
				&tmo, sizeof(struct timeval));
		if (ret < 0) {
			LOG_ERR("setsockopt() error: %d", -errno);
		}
	} break;

	case SO_BINDTODEVICE:	/* Not supported by SLM for now */
	case SO_TIMESTAMPING:	/* Not supported by SLM for now */
	case SO_TXTIME:		/* Not supported by SLM for now */
	case SO_SOCKS5:		/* Not supported by SLM for now */
	default:
		sprintf(buf, "#XSOCKETOPT: not supported\r\n");
		client.callback(buf);
		ret = 0;
		break;
	}

	return ret;
}

static int do_socketopt_get(int name)
{
	int ret = 0;

	switch (name) {
	case SO_REUSEADDR:	/* Ignored by Zephyr */
	case SO_ERROR:		/* Ignored by Zephyr */
		sprintf(buf, "#XSOCKETOPT: ignored\r\n");
		client.callback(buf);
		break;

	case SO_RCVTIMEO: {
		struct timeval tmo;
		socklen_t len = sizeof(struct timeval);

		ret = getsockopt(client.sock, SOL_SOCKET, SO_RCVTIMEO,
				&tmo, &len);
		if (ret) {
			LOG_ERR("getsockopt() error: %d", -errno);
		} else {
			sprintf(buf, "#XSOCKETOPT: %d sec\r\n",
				(int)tmo.tv_sec);
			client.callback(buf);
		}
	} break;

	case SO_BINDTODEVICE:	/* Not supported by SLM for now */
	case SO_TIMESTAMPING:	/* Not supported by SLM for now */
	case SO_TXTIME:		/* Not supported by SLM for now */
	case SO_SOCKS5:		/* Not supported by SLM for now */
	default:
		sprintf(buf, "#XSOCKETOPT: not supported\r\n");
		client.callback(buf);
		break;
	}

	return ret;
}

static int do_bind(u16_t port)
{
	int ret;
	struct sockaddr_in local;
	int addr_len;

	local.sin_family = AF_INET;
	local.sin_port = htons(port);

	ret = modem_info_params_get(&modem_param);
	if (ret) {
		LOG_ERR("Unable to obtain modem parameters (%d)", ret);
		return -1;
	}
	/* Check network connection status by checking local IP address */
	addr_len = strlen(modem_param.network.ip_address.value_string);
	if (addr_len == 0) {
		LOG_ERR("LTE not connected yet");
		return -1;
	}
	if (!check_for_ipv4(modem_param.network.ip_address.value_string,
			addr_len)) {
		LOG_ERR("Invalid local address");
		return -1;
	}

	/* NOTE inet_pton() returns 1 as success */
	if (inet_pton(AF_INET, modem_param.network.ip_address.value_string,
		&local.sin_addr) != 1) {
		LOG_ERR("Parse local IP address failed: %d", -errno);
		return -EINVAL;
	}

	ret = bind(client.sock, (struct sockaddr *)&local,
		 sizeof(struct sockaddr_in));
	if (ret) {
		LOG_ERR("bind() failed: %d", -errno);
		do_socket_close(-errno);
		return -errno;
	}

	return 0;
}

static int do_connect(const char *url, u16_t port)
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
	client.callback("#XCONNECT: 1\r\n");
	return 0;
}

static int do_listen(void)
{
	int ret;

	/* hardcode backlog to be 1 for now */
	ret = listen(client.sock, 1);
	if (ret < 0) {
		LOG_ERR("listen() failed: %d", -errno);
		do_socket_close(-errno);
		return -errno;
	}

	client.sock_peer = INVALID_SOCKET;
	return 0;
}

static int do_accept(void)
{
	int ret;
	char peer_addr[16];
	socklen_t len = sizeof(struct sockaddr_in);

	ret = accept(client.sock, (struct sockaddr *)&remote, &len);
	if (ret < 0) {
		LOG_ERR("accept() failed: %d/%d", -errno, ret);
		do_socket_close(-errno);
		return -errno;
	}
	if (inet_ntop(AF_INET, &remote.sin_addr, peer_addr, len) == NULL) {
		LOG_WRN("Parse peer IP address failed: %d", -errno);
		return -EINVAL;
	}
	sprintf(buf, "#XACCEPT: connected with %s\r\n",
		peer_addr);
	client.callback(buf);
	client.sock_peer = ret;
	sprintf(buf, "#XACCEPT: %d\r\n", client.sock_peer);
	client.callback(buf);

#if defined(CONFIG_SLM_TEST_MODE)
	do_recv(NET_IPV4_MTU);
#endif

	return 0;
}

static int do_send(const char *data)
{
	u32_t offset = 0;
	u32_t datalen = strlen(data);
	int ret = 0;
	int sock = client.sock;

	/* For TCP/TLS Server, send to imcoming socket */
	if ((client.ip_proto == IPPROTO_TCP ||
		client.ip_proto == IPPROTO_TLS_1_2) &&
		client.role == AT_SOCKET_ROLE_SERVER) {
		if (client.sock_peer != INVALID_SOCKET) {
			sock = client.sock_peer;
		} else {
			LOG_ERR("No remote connection");
			return -EINVAL;
		}
	}

	while (offset < datalen) {
		ret = send(sock, data + offset, datalen - offset, 0);
		if (ret < 0) {
			LOG_ERR("send() failed: %d", -errno);
			if (errno != EAGAIN && errno != ETIMEDOUT) {
				do_socket_close(-errno);
			} else {
				sprintf(buf, "#XSOCKET: %d\r\n", -errno);
				client.callback(buf);
			}
			ret = -errno;
			break;
		}
		offset += ret;
	}

	sprintf(buf, "#XSEND: %d\r\n", offset);
	client.callback(buf);

#if defined(CONFIG_SLM_TEST_MODE)
	if (client.role == AT_SOCKET_ROLE_CLIENT) {
		do_recv(NET_IPV4_MTU);
	}
#endif

	LOG_DBG("Sent");
	if (ret >= 0) {
		return 0;
	} else {
		return ret;
	}
}

static int do_recv(u16_t length)
{
	int ret;
	char data[NET_IPV4_MTU];
	int sock = client.sock;

	/* For TCP/TLS Server, receive from imcoming socket */
	if ((client.ip_proto == IPPROTO_TCP ||
		client.ip_proto == IPPROTO_TLS_1_2) &&
		client.role == AT_SOCKET_ROLE_SERVER) {
		if (client.sock_peer != INVALID_SOCKET) {
			sock = client.sock_peer;
		} else {
			LOG_ERR("No remote connection");
			return -EINVAL;
		}
	}

	ret = recv(sock, data, length, 0);
	if (ret < 0) {
		LOG_WRN("recv() error: %d", -errno);
		if (errno != EAGAIN && errno != ETIMEDOUT) {
			do_socket_close(-errno);
		} else {
			sprintf(buf, "#XSOCKET: %d\r\n", -errno);
			client.callback(buf);
		}
		return -errno;
	}
	/**
	 * When a stream socket peer has performed an orderly shutdown,
	 * the return value will be 0 (the traditional "end-of-file")
	 * The value 0 may also be returned if the requested number of
	 * bytes to receive from a stream socket was 0
	 * In both cases, treat as normal shutdown by remote
	 */
	if (ret == 0) {
		LOG_WRN("recv() return 0");
	}
	data[ret] = '\0';
	client.callback("#XRECV: ");
	client.callback(data);
	client.callback("\r\n");
	sprintf(buf, "#XRECV: %d\r\n", ret);
	client.callback(buf);
	ret = 0;

#if defined(CONFIG_SLM_TEST_MODE)
	if (client.role == AT_SOCKET_ROLE_SERVER) {
		do_send(data);
	}
#endif
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

static int do_sendto(const char *url, u16_t port, const char *data)
{
	u32_t offset = 0;
	u32_t datalen = strlen(data);
	int ret;

	ret = do_udp_init(url, port);
	if (ret < 0) {
		return ret;
	}

	while (offset < datalen) {
		ret = sendto(client.sock, data + offset,
			datalen - offset, 0,
			(struct sockaddr *)&remote,
			sizeof(struct sockaddr_in));
		if (ret <= 0) {
			LOG_ERR("sendto() failed: %d", -errno);
			if (errno != EAGAIN && errno != ETIMEDOUT) {
				do_socket_close(-errno);
			} else {
				sprintf(buf, "#XSOCKET: %d\r\n", -errno);
				client.callback(buf);
			}
			ret = -errno;
			break;
		}
		offset += ret;
	}

	sprintf(buf, "#XSENDTO: %d\r\n", offset);
	client.callback(buf);

#if defined(CONFIG_SLM_TEST_MODE)
	if (client.role == AT_SOCKET_ROLE_CLIENT) {
		do_recvfrom(url, port, NET_IPV4_MTU);
	}
#endif

	LOG_DBG("UDP sent");
	if (ret >= 0) {
		return 0;
	} else {
		return ret;
	}
}

static int do_recvfrom(const char *url, u16_t port, u16_t length)
{
	int ret;
	char data[NET_IPV4_MTU];
	int sockaddr_len = sizeof(struct sockaddr_in);

	ret = do_udp_init(url, port);
	if (ret < 0) {
		return ret;
	}

	ret = recvfrom(client.sock, data, length, 0,
		(struct sockaddr *)&remote, &sockaddr_len);
	if (ret < 0) {
		LOG_ERR("recvfrom() error: %d", -errno);
		if (errno != EAGAIN && errno != ETIMEDOUT) {
			do_socket_close(-errno);
		} else {
			sprintf(buf, "#XSOCKET: %d\r\n", -errno);
			client.callback(buf);
		}
		return -errno;
	}
	/**
	 * Datagram sockets in various domains permit zero-length
	 * datagrams. When such a datagram is received, the return
	 * value is 0. Treat as normal case
	 */
	data[ret] = '\0';
	client.callback("#XRECV: ");
	client.callback(data);
	client.callback("\r\n");
	sprintf(buf, "#XRECV: %d\r\n", ret);
	client.callback(buf);

#if defined(CONFIG_SLM_TEST_MODE)
	if (client.role == AT_SOCKET_ROLE_SERVER) {
		do_sendto(url, port, data);
	}
#endif

	LOG_DBG("UDP received");
	return 0;
}

/**@brief handle AT#XSOCKET commands
 *  AT#XSOCKET=<op>[,<type>,<role>,[sec_tag]]
 *  AT#XSOCKET?
 *  AT#XSOCKET=? TEST command not supported
 */
static int handle_at_socket(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	u16_t op;
	u16_t role;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 2) {
			return -EINVAL;
		}
		err = at_params_short_get(&m_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == AT_SOCKET_OPEN) {
			u16_t type;
			sec_tag_t sec_tag = INVALID_SEC_TAG;

			if (at_params_valid_count_get(&m_param_list) < 4) {
				return -EINVAL;
			}
			err = at_params_short_get(&m_param_list, 2, &type);
			if (err) {
				return err;
			}
			err = at_params_short_get(&m_param_list, 3, &role);
			if (err) {
				return err;
			}
			if (at_params_valid_count_get(&m_param_list) > 4) {
				at_params_int_get(&m_param_list, 4, &sec_tag);
			}
			if (client.sock > 0) {
				LOG_WRN("Socket is already opened");
				return -EINVAL;
			} else {
				err = do_socket_open(type, role, sec_tag);
			}
		} else if (op == AT_SOCKET_CLOSE) {
			if (client.sock < 0) {
				LOG_WRN("Socket is not opened yet");
				return -EINVAL;
			} else {
				err = do_socket_close(0);
			}
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (client.sock != INVALID_SOCKET) {
			sprintf(buf, "#XSOCKET: %d, %d, %d\r\n", client.sock,
				client.ip_proto, client.role);
		} else {
			sprintf(buf, "#XSOCKET: 0\r\n");
		}
		client.callback(buf);
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(buf, "#XSOCKET: (%d, %d), (%d, %d), (%d, %d)",
			AT_SOCKET_CLOSE, AT_SOCKET_OPEN,
			SOCK_STREAM, SOCK_DGRAM,
			AT_SOCKET_ROLE_CLIENT, AT_SOCKET_ROLE_SERVER);
		client.callback(buf);
		client.callback(", <sec-tag>\r\n");
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XSOCKETOPT commands
 *  AT#XSOCKETOPT=<op>,<name>[,<value>]
 *  AT#XSOCKETOPT? READ command not supported
 *  AT#XSOCKETOPT=? TEST command not supported
 */
static int handle_at_socketopt(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	u16_t op;
	u16_t name;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (client.sock < 0) {
			LOG_ERR("Socket not opened yet");
			return err;
		}
		if (client.role != AT_SOCKET_ROLE_CLIENT) {
			LOG_ERR("Invalid role");
			return err;
		}
		if (at_params_valid_count_get(&m_param_list) < 3) {
			return -EINVAL;
		}
		err = at_params_short_get(&m_param_list, 1, &op);
		if (err) {
			return err;
		}
		err = at_params_short_get(&m_param_list, 2, &name);
		if (err) {
			return err;
		}
		if (op == AT_SOCKETOPT_SET) {
			int value;

			if (at_params_valid_count_get(&m_param_list) < 4) {
				return -EINVAL;
			}
			err = at_params_int_get(&m_param_list, 2, &value);
			if (err) {
				return err;
			}
			err = do_socketopt_set(name, value);
		} else if (op == AT_SOCKETOPT_GET) {
			err = do_socketopt_get(name);
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(buf, "#XSOCKETOPT: (%d, %d), <name>, <value>\r\n",
			AT_SOCKETOPT_GET, AT_SOCKETOPT_SET);
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
		}
		err = do_bind(port);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XCONNECT commands
 *  AT#XCONNECT=<url>,<port>
 *  AT#XCONNECT?
 *  AT#XCONNECT=? TEST command not supported
 */
static int handle_at_connect(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char url[TCPIP_MAX_URL];
	int size = TCPIP_MAX_URL;
	u16_t port;

	if (client.sock < 0) {
		LOG_ERR("Socket not opened yet");
		return err;
	}
	if (client.role != AT_SOCKET_ROLE_CLIENT) {
		LOG_ERR("Invalid role");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 3) {
			return -EINVAL;
		}
		err = at_params_string_get(&m_param_list, 1, url, &size);
		if (err) {
			return err;
		}
		url[size] = '\0';
		err = at_params_short_get(&m_param_list, 2, &port);
		if (err) {
			return err;
		}
		err = do_connect(url, port);
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (client.connected) {
			client.callback("+XCONNECT: 1\r\n");
		} else {
			client.callback("+XCONNECT: 0\r\n");
		}
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XLISTEN commands
 *  AT#XLISTEN
 *  AT#XLISTEN? READ command not supported
 *  AT#XLISTEN=? TEST command not supported
 */
static int handle_at_listen(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;

	if (client.sock < 0) {
		LOG_ERR("Socket not opened yet");
		return err;
	}
	if (client.role != AT_SOCKET_ROLE_SERVER) {
		LOG_ERR("Invalid role");
		return err;
	}
	if (client.ip_proto != IPPROTO_TCP &&
		client.ip_proto != IPPROTO_TLS_1_2) {
		LOG_ERR("Invalid protocol");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = do_listen();
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XACCEPT commands
 *  AT#XACCEPT
 *  AT#XACCEPT? READ command not supported
 *  AT#XACCEPT=? TEST command not supported
 */
static int handle_at_accept(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;

	if (client.sock < 0) {
		LOG_ERR("Socket not opened yet");
		return err;
	}
	if (client.role != AT_SOCKET_ROLE_SERVER) {
		LOG_ERR("Invalid role");
		return err;
	}
	if (client.ip_proto != IPPROTO_TCP &&
		client.ip_proto != IPPROTO_TLS_1_2) {
		LOG_ERR("Invalid protocol");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = do_accept();
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (client.sock_peer != INVALID_SOCKET) {
			sprintf(buf, "#XTCPACCEPT: %d\r\n", client.sock_peer);
		} else {
			sprintf(buf, "#XTCPACCEPT: 0\r\n");
		}
		client.callback(buf);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XSEND commands
 *  AT#XSEND=<data>
 *  AT#XSEND? READ command not supported
 *  AT#XSEND=? TEST command not supported
 */
static int handle_at_send(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char data[NET_IPV4_MTU];
	int size = NET_IPV4_MTU;

	if (!client.connected) {
		LOG_ERR("Not connected yet");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 2) {
			return -EINVAL;
		}
		err = at_params_string_get(&m_param_list, 1, data, &size);
		if (err) {
			return err;
		}
		data[size] = '\0';
		err = do_send(data);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XRECV commands
 *  AT#XRECV[=<length>]
 *  AT#XRECV? READ command not supported
 *  AT#XRECV=? TEST command not supported
 */
static int handle_at_recv(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	u16_t length = NET_IPV4_MTU;

	if (!client.connected) {
		LOG_ERR("Not connected yet");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) > 1) {
			err = at_params_short_get(&m_param_list, 1, &length);
			if (err) {
				return err;
			}
		}
		err = do_recv(length);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XSENDTO commands
 *  AT#XSENDTO=<url>,<port>,<data>
 *  AT#XSENDTO? READ command not supported
 *  AT#XSENDTO=? TEST command not supported
 */
static int handle_at_sendto(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char url[TCPIP_MAX_URL];
	u16_t port;
	char data[NET_IPV4_MTU];
	int size;

	if (client.sock < 0) {
		LOG_ERR("Socket not opened yet");
		return err;
	}
	if (client.ip_proto != IPPROTO_UDP &&
		client.ip_proto != IPPROTO_DTLS_1_2) {
		LOG_ERR("Invalid protocol");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 4) {
			return -EINVAL;
		}
		size = TCPIP_MAX_URL;
		err = at_params_string_get(&m_param_list, 1, url, &size);
		if (err) {
			return err;
		}
		url[size] = '\0';
		err = at_params_short_get(&m_param_list, 2, &port);
		if (err) {
			return err;
		}
		size = NET_IPV4_MTU;
		err = at_params_string_get(&m_param_list, 3, data, &size);
		if (err) {
			return err;
		}
		data[size] = '\0';
		err = do_sendto(url, port, data);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XRECVFROM commands
 *  AT#XRECVFROM=<url>,<port>[,<length>]
 *  AT#XRECVFROM? READ command not supported
 *  AT#XRECVFROM=? TEST command not supported
 */
static int handle_at_recvfrom(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char url[TCPIP_MAX_URL];
	int size = TCPIP_MAX_URL;
	u16_t port;
	u16_t length = NET_IPV4_MTU;

	if (client.sock < 0) {
		LOG_ERR("Socket not opened yet");
		return err;
	}
	if (client.ip_proto != IPPROTO_UDP &&
		client.ip_proto != IPPROTO_DTLS_1_2) {
		LOG_ERR("Invalid protocol");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 3) {
			return -EINVAL;
		}
		err = at_params_string_get(&m_param_list, 1, url, &size);
		if (err) {
			return err;
		}
		url[size] = '\0';
		err = at_params_short_get(&m_param_list, 2, &port);
		if (err) {
			return err;
		}
		if (at_params_valid_count_get(&m_param_list) > 3) {
			err = at_params_short_get(&m_param_list, 3, &length);
			if (err) {
				return err;
			}
		}
		err = do_recvfrom(url, port, length);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#GETADDRINFO commands
 *  AT#XGETADDRINFO=<url>
 *  AT#XGETADDRINFO? READ command not supported
 *  AT#XGETADDRINFO=? TEST command not supported
 */
static int handle_at_getaddrinfo(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char url[TCPIP_MAX_URL];
	int size = TCPIP_MAX_URL;
	struct addrinfo *result;
	struct addrinfo hints = {
		.ai_family = AF_INET
	};
	struct sockaddr_in *host;
	char ipv4addr[NET_IPV4_ADDR_LEN];

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&m_param_list) < 2) {
			return -EINVAL;
		}
		err = at_params_string_get(&m_param_list, 1, url, &size);
		if (err) {
			return err;
		}
		url[size] = '\0';
		if (check_for_ipv4(url, strlen(url))) {
			LOG_ERR("already IPv4 address");
			return -EINVAL;
		}
		err = getaddrinfo(url, NULL, &hints, &result);
		if (err) {
			LOG_ERR("getaddrinfo() failed %d", err);
			sprintf(buf, "#XGETADDRINFO: %d\r\n", -err);
			client.callback(buf);
			return err;
		} else if (result == NULL) {
			LOG_ERR("Address not found\n");
			sprintf(buf, "#XGETADDRINFO: not found\r\n");
			client.callback(buf);
			return -ENOENT;
		}

		host = (struct sockaddr_in *)result->ai_addr;
		inet_ntop(AF_INET, &(host->sin_addr.s_addr),
			ipv4addr, sizeof(ipv4addr));
		sprintf(buf, "#XGETADDRINFO: %s\r\n", ipv4addr);
		client.callback(buf);
		freeaddrinfo(result);
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
			if (ret) {
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

/**@brief API to list TCP/IP AT commands
 */
void slm_at_tcpip_clac(void)
{
	for (int i = 0; i < AT_TCPIP_MAX; i++) {
		client.callback(m_tcpip_at_list[i].string);
		client.callback("\r\n");
	}
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
	client.role = AT_SOCKET_ROLE_CLIENT;
	client.sock_peer = INVALID_SOCKET;
	client.connected = false;
	client.ip_proto = IPPROTO_IP;
	client.callback = callback;
	return 0;
}

/**@brief API to uninitialize TCP/IP AT commands handler
 */
int slm_at_tcpip_uninit(void)
{
	return do_socket_close(0);
}
