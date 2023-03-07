/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_socket.h"
#include "slm_native_tls.h"

LOG_MODULE_REGISTER(slm_sock, CONFIG_SLM_LOG_LEVEL);

/*
 * Known limitation in this version
 * - Multiple concurrent sockets
 * - TCP server accept one connection only
 * - Receive more than IPv4 MTU one-time
 */

/**@brief Socket operations. */
enum slm_socket_operation {
	AT_SOCKET_CLOSE,
	AT_SOCKET_OPEN,
	AT_SOCKET_OPEN6
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

static char udp_url[SLM_MAX_URL];
static uint16_t udp_port;

static struct slm_socket {
	uint16_t type;     /* SOCK_STREAM or SOCK_DGRAM */
	uint16_t role;     /* Client or Server */
	sec_tag_t sec_tag; /* Security tag of the credential */
	int family;        /* Socket address family */
	int fd;            /* Socket descriptor. */
	int fd_peer;       /* Socket descriptor for peer. */
	int ranking;       /* Ranking of socket */
	uint16_t cid;      /* PDP Context ID, 0: primary; 1~10: secondary */
} socks[SLM_MAX_SOCKET_COUNT];

static struct pollfd fds[SLM_MAX_SOCKET_COUNT];
static struct slm_socket sock;

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern uint8_t data_buf[SLM_MAX_MESSAGE_SIZE];

/* forward declarations */
#define SOCKET_SEND_TMO_SEC      30
static int socket_poll(int sock_fd, int event, int timeout);

static int socket_ranking;

#define INIT_SOCKET(socket)			\
	socket.family  = AF_UNSPEC;		\
	socket.sec_tag = INVALID_SEC_TAG;	\
	socket.role    = AT_SOCKET_ROLE_CLIENT;	\
	socket.fd      = INVALID_SOCKET;	\
	socket.fd_peer = INVALID_SOCKET;	\
	socket.ranking = 0;			\
	socket.cid     = 0;

static bool is_opened_socket(int fd)
{
	if (fd == INVALID_SOCKET) {
		return false;
	}

	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		if (socks[i].fd == fd) {
			return true;
		}
	}

	return false;
}

static int find_avail_socket(void)
{
	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		if (socks[i].fd == INVALID_SOCKET) {
			return i;
		}
	}

	return -ENOENT;
}

static int bind_to_device(uint16_t cid)
{
	int ret = 0;

	if (cid > 0) {
		int cid_int = cid;

		ret = setsockopt(sock.fd, SOL_SOCKET, SO_BINDTODEVICE, &cid_int, sizeof(int));
		if (ret < 0) {
			LOG_ERR("SO_BINDTODEVICE error: %d", -errno);
		}
	}

	return ret;
}

static int do_socket_open(void)
{
	int ret = 0;
	int proto = IPPROTO_TCP;

	if (sock.type == SOCK_STREAM) {
		ret = socket(sock.family, SOCK_STREAM, IPPROTO_TCP);
	} else if (sock.type == SOCK_DGRAM) {
		ret = socket(sock.family, SOCK_DGRAM, IPPROTO_UDP);
		proto = IPPROTO_UDP;
	} else if (sock.type == SOCK_RAW) {
		sock.family = AF_PACKET;
		sock.role = AT_SOCKET_ROLE_CLIENT;
		ret = socket(sock.family, SOCK_RAW, IPPROTO_IP);
		proto = IPPROTO_IP;
	} else {
		LOG_ERR("socket type %d not supported", sock.type);
		return -ENOTSUP;
	}
	if (ret < 0) {
		LOG_ERR("socket() error: %d", -errno);
		return -errno;
	}

	sock.fd = ret;
	/* Explicitly bind to secondary PDP context if required */
	ret = bind_to_device(sock.cid);
	if (ret) {
		close(sock.fd);
		return ret;
	}

	sock.ranking = socket_ranking++;
	ret = find_avail_socket();
	if (ret < 0) {
		return ret;
	}
	socks[ret] = sock;
	rsp_send("\r\n#XSOCKET: %d,%d,%d\r\n", sock.fd, sock.type, proto);

	return 0;
}

static int do_secure_socket_open(int peer_verify)
{
	int ret = 0;
	int proto = IPPROTO_TLS_1_2;

	if (sock.type == SOCK_STREAM) {
		ret = socket(sock.family, SOCK_STREAM, IPPROTO_TLS_1_2);
	} else if (sock.type == SOCK_DGRAM) {
		ret = socket(sock.family, SOCK_DGRAM, IPPROTO_DTLS_1_2);
		proto = IPPROTO_DTLS_1_2;
	} else {
		LOG_ERR("socket type %d not supported", sock.type);
		return -ENOTSUP;
	}
	if (ret < 0) {
		LOG_ERR("socket() error: %d", -errno);
		return -errno;
	}
	sock.fd = ret;
	/* Explicitly bind to secondary PDP context if required */
	ret = bind_to_device(sock.cid);
	if (ret) {
		close(sock.fd);
		return ret;
	}

	sec_tag_t sec_tag_list[1] = { sock.sec_tag };
#if defined(CONFIG_SLM_NATIVE_TLS)
	ret = slm_tls_loadcrdl(sock.sec_tag);
	if (ret < 0) {
		LOG_ERR("Fail to load credential: %d", ret);
		goto error_exit;
	}
#endif
	ret = setsockopt(sock.fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_t));
	if (ret) {
		LOG_ERR("setsockopt(TLS_SEC_TAG_LIST) error: %d", -errno);
		ret = -errno;
		goto error_exit;
	}

	/* Set up (D)TLS peer verification */
	ret = setsockopt(sock.fd, SOL_TLS, TLS_PEER_VERIFY, &peer_verify, sizeof(peer_verify));
	if (ret) {
		LOG_ERR("setsockopt(TLS_PEER_VERIFY) error: %d", errno);
		ret = -errno;
		goto error_exit;
	}
	/* Set up (D)TLS server role if applicable */
	if (sock.role == AT_SOCKET_ROLE_SERVER) {
		int tls_role = TLS_DTLS_ROLE_SERVER;

		ret = setsockopt(sock.fd, SOL_TLS, TLS_DTLS_ROLE, &tls_role, sizeof(int));
		if (ret) {
			LOG_ERR("setsockopt(TLS_DTLS_ROLE) error: %d", -errno);
			ret = -errno;
			goto error_exit;
		}
	}

	sock.ranking = socket_ranking++;
	ret = find_avail_socket();
	if (ret < 0) {
		return ret;
	}
	socks[ret] = sock;
	rsp_send("\r\n#XSSOCKET: %d,%d,%d\r\n", sock.fd, sock.type, proto);

	return 0;

error_exit:
#if defined(CONFIG_SLM_NATIVE_TLS)
	if (sock.sec_tag != INVALID_SEC_TAG) {
		(void)slm_tls_unloadcrdl(sock.sec_tag);
		sock.sec_tag = INVALID_SEC_TAG;
	}
#endif
	close(sock.fd);
	INIT_SOCKET(sock);
	return ret;
}

static int do_socket_close(void)
{
	int ret;

	if (sock.fd == INVALID_SOCKET) {
		return 0;
	}

#if defined(CONFIG_SLM_NATIVE_TLS)
	if (sock.sec_tag != INVALID_SEC_TAG) {
		ret = slm_tls_unloadcrdl(sock.sec_tag);
		if (ret < 0) {
			LOG_WRN("Fail to unload credential: %d", ret);
		}
		sock.sec_tag = INVALID_SEC_TAG;
	}
#endif
	if (sock.fd_peer != INVALID_SOCKET) {
		ret = close(sock.fd_peer);
		if (ret) {
			LOG_WRN("peer close() error: %d", -errno);
		}
		sock.fd_peer = INVALID_SOCKET;
	}
	ret = close(sock.fd);
	if (ret) {
		LOG_WRN("close() error: %d", -errno);
		ret = -errno;
	}

	rsp_send("\r\n#XSOCKET: %d,\"closed\"\r\n", ret);

	/* Select most recent socket as current active */
	int ranking = 0, index = -1;

	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		if (socks[i].fd == INVALID_SOCKET) {
			continue;
		}
		if (socks[i].fd == sock.fd) {
			LOG_DBG("Set socket %d null", sock.fd);
			INIT_SOCKET(socks[i]);
		} else {
			if (ranking < socks[i].ranking) {
				ranking = socks[i].ranking;
				index = i;
			}
		}
	}
	if (index >= 0) {
		LOG_INF("Swap to socket %d", socks[index].fd);
		sock = socks[index];
	} else {
		INIT_SOCKET(sock);
	}

	return ret;
}

static int do_socketopt_set(int option, int value)
{
	int ret = -ENOTSUP;

	switch (option) {
	case SO_BINDTODEVICE:
	case SO_REUSEADDR:
		ret = setsockopt(sock.fd, SOL_SOCKET, option, &value, sizeof(int));
		if (ret < 0) {
			LOG_ERR("setsockopt(%d) error: %d", option, -errno);
		}
		break;

	case SO_RCVTIMEO:
	case SO_SNDTIMEO: {
		struct timeval tmo = { .tv_sec = value };
		socklen_t len = sizeof(struct timeval);

		ret = setsockopt(sock.fd, SOL_SOCKET, option, &tmo, len);
		if (ret < 0) {
			LOG_ERR("setsockopt(%d) error: %d", option, -errno);
		}
	} break;

	/** NCS extended socket options */
	case SO_SILENCE_ALL:
	case SO_IP_ECHO_REPLY:
	case SO_IPV6_ECHO_REPLY:
	case SO_TCP_SRV_SESSTIMEO:
		ret = setsockopt(sock.fd, SOL_SOCKET, option, &value, sizeof(int));
		if (ret < 0) {
			LOG_ERR("setsockopt(%d) error: %d", option, -errno);
		}
		break;

	/* RAI-related */
	case SO_RAI_LAST:
	case SO_RAI_NO_DATA:
	case SO_RAI_ONE_RESP:
	case SO_RAI_ONGOING:
	case SO_RAI_WAIT_MORE:
		ret = setsockopt(sock.fd, SOL_SOCKET, option, NULL, 0);
		if (ret < 0) {
			LOG_ERR("setsockopt(%d) error: %d", option, -errno);
		}
		break;

	case SO_PRIORITY:
	case SO_TIMESTAMPING:
		rsp_send("\r\n#XSOCKETOPT: \"not supported\"\r\n");
		break;

	default:
		LOG_WRN("Unknown option %d", option);
		break;
	}

	return ret;
}

static int do_socketopt_get(int option)
{
	int ret = 0;

	switch (option) {
	case SO_SILENCE_ALL:
	case SO_IP_ECHO_REPLY:
	case SO_IPV6_ECHO_REPLY:
	case SO_TCP_SRV_SESSTIMEO:
	case SO_ERROR: {
		int value;
		socklen_t len = sizeof(int);

		ret = getsockopt(sock.fd, SOL_SOCKET, option, &value, &len);
		if (ret) {
			LOG_ERR("getsockopt(%d) error: %d", option, -errno);
		} else {
			rsp_send("\r\n#XSOCKETOPT: %d\r\n", value);
		}
	} break;

	case SO_RCVTIMEO:
	case SO_SNDTIMEO: {
		struct timeval tmo;
		socklen_t len = sizeof(struct timeval);

		ret = getsockopt(sock.fd, SOL_SOCKET, option, &tmo, &len);
		if (ret) {
			LOG_ERR("getsockopt(%d) error: %d", option, -errno);
		} else {
			rsp_send("\r\n#XSOCKETOPT: \"%d sec\"\r\n", (int)tmo.tv_sec);
		}
	} break;

	case SO_TYPE:
	case SO_PRIORITY:
	case SO_PROTOCOL:
		rsp_send("\r\n#XSOCKETOPT: \"not supported\"\r\n");
		break;

	default:
		break;
	}

	return ret;
}

static int do_secure_socketopt_set_str(int option, const char *value)
{
	int ret = -ENOTSUP;

	switch (option) {
	case TLS_HOSTNAME:
		/** Write-only socket option to set hostname. It accepts a string containing
		 *  the hostname (may be NULL to disable hostname verification).
		 */
		if (slm_util_cmd_casecmp(value, "NULL")) {
			ret = setsockopt(sock.fd, SOL_TLS, option, NULL, 0);
		} else {
			ret = setsockopt(sock.fd, SOL_TLS, option, value, strlen(value));
		}
		if (ret < 0) {
			LOG_ERR("setsockopt(%d) error: %d", option, -errno);
		}
		break;

	default:
		LOG_WRN("Unknown option %d", option);
		break;
	}

	return ret;
}

static int do_secure_socketopt_set_int(int option, int value)
{
	int ret = -ENOTSUP;

	switch (option) {
	case TLS_PEER_VERIFY:
	case TLS_SESSION_CACHE:
	case TLS_SESSION_CACHE_PURGE:
	case TLS_DTLS_HANDSHAKE_TIMEO:
		ret = setsockopt(sock.fd, SOL_TLS, option, &value, sizeof(int));
		if (ret < 0) {
			LOG_ERR("setsockopt(%d) error: %d", option, -errno);
		}
		break;

	case TLS_DTLS_HANDSHAKE_TIMEOUT_MIN:
	case TLS_DTLS_HANDSHAKE_TIMEOUT_MAX:
	case TLS_CIPHERSUITE_LIST:
	case TLS_ALPN_LIST:
		rsp_send("\r\n#XSSOCKETOPT: \"not supported\"\r\n");
		break;

	default:
		LOG_WRN("Unknown option %d", option);
		break;
	}

	return ret;
}

static int do_secure_socketopt_get(int option)
{
	int ret = 0;

	switch (option) {
	case TLS_CIPHERSUITE_USED: {
		int value;
		socklen_t len = sizeof(int);

		ret = getsockopt(sock.fd, SOL_SOCKET, option, &value, &len);
		if (ret) {
			LOG_ERR("getsockopt(%d) error: %d", option, -errno);
		} else {
			rsp_send("\r\n#XSSOCKETOPT: %d\r\n", value);
		}
	} break;

	default:
		break;
	}

	return ret;
}

static int do_bind(uint16_t port)
{
	int ret;

	if (sock.family == AF_INET) {
		char ipv4_addr[INET_ADDRSTRLEN] = {0};

		util_get_ip_addr(0, ipv4_addr, NULL);
		if (strlen(ipv4_addr) == 0) {
			LOG_ERR("Get local IPv4 address failed");
			return -EINVAL;
		}

		struct sockaddr_in local = {
			.sin_family = AF_INET,
			.sin_port = htons(port)
		};

		if (inet_pton(AF_INET, ipv4_addr, &local.sin_addr) != 1) {
			LOG_ERR("Parse local IPv4 address failed: %d", -errno);
			return -EAGAIN;
		}

		ret = bind(sock.fd, (struct sockaddr *)&local, sizeof(struct sockaddr_in));
		if (ret) {
			LOG_ERR("bind() failed: %d", -errno);
			return -errno;
		}
		LOG_DBG("bind to %s", ipv4_addr);
	} else if (sock.family == AF_INET6) {
		char ipv6_addr[INET6_ADDRSTRLEN] = {0};

		util_get_ip_addr(0, NULL, ipv6_addr);
		if (strlen(ipv6_addr) == 0) {
			LOG_ERR("Get local IPv6 address failed");
			return -EINVAL;
		}

		struct sockaddr_in6 local = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(port)
		};

		if (inet_pton(AF_INET6, ipv6_addr, &local.sin6_addr) != 1) {
			LOG_ERR("Parse local IPv6 address failed: %d", -errno);
			return -EAGAIN;
		}
		ret = bind(sock.fd, (struct sockaddr *)&local, sizeof(struct sockaddr_in6));
		if (ret) {
			LOG_ERR("bind() failed: %d", -errno);
			return -errno;
		}
		LOG_DBG("bind to %s", ipv6_addr);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int do_connect(const char *url, uint16_t port)
{
	int ret = 0;
	struct sockaddr sa = {
		.sa_family = AF_UNSPEC
	};

	LOG_DBG("connect %s:%d", url, port);
	ret = util_resolve_host(sock.cid, url, port, sock.family, &sa);
	if (ret) {
		LOG_ERR("getaddrinfo() error: %s", gai_strerror(ret));
		return -EAGAIN;
	}
	if (sa.sa_family == AF_INET) {
		ret = connect(sock.fd, &sa, sizeof(struct sockaddr_in));
	} else {
		ret = connect(sock.fd, &sa, sizeof(struct sockaddr_in6));
	}
	if (ret) {
		LOG_ERR("connect() error: %d", -errno);
		return -errno;
	}

	rsp_send("\r\n#XCONNECT: 1\r\n");

	return ret;
}

static int do_listen(void)
{
	int ret;

	/* hardcode backlog to be 1 for now */
	ret = listen(sock.fd, 1);
	if (ret < 0) {
		LOG_ERR("listen() error: %d", -errno);
		return -errno;
	}

	return 0;
}

static int do_accept(int timeout)
{
	int ret;
	char peer_addr[INET6_ADDRSTRLEN] = {0};

	ret = socket_poll(sock.fd, POLLIN, timeout);
	if (ret) {
		return ret;
	}

	if (sock.family == AF_INET) {
		struct sockaddr_in client;
		socklen_t len = sizeof(struct sockaddr_in);

		ret = accept(sock.fd, (struct sockaddr *)&client, &len);
		if (ret == -1) {
			LOG_ERR("accept() error: %d", -errno);
			sock.fd_peer = INVALID_SOCKET;
			return -errno;
		}
		sock.fd_peer = ret;
		(void)inet_ntop(AF_INET, &client.sin_addr, peer_addr, sizeof(peer_addr));
	} else if (sock.family == AF_INET6) {
		struct sockaddr_in6 client;
		socklen_t len = sizeof(struct sockaddr_in6);

		ret = accept(sock.fd, (struct sockaddr *)&client, &len);
		if (ret == -1) {
			LOG_ERR("accept() error: %d", -errno);
			sock.fd_peer = INVALID_SOCKET;
			return -errno;
		}
		sock.fd_peer = ret;
		(void)inet_ntop(AF_INET6, &client.sin6_addr, peer_addr, sizeof(peer_addr));
	} else {
		return -EINVAL;
	}
	rsp_send("\r\n#XACCEPT: %d,\"%s\"\r\n", sock.fd_peer, peer_addr);

	return 0;
}

static int do_send(const uint8_t *data, int datalen)
{
	int ret = 0;
	int sockfd = sock.fd;

	/* For TCP/TLS Server, send to incoming socket */
	if (sock.type == SOCK_STREAM && sock.role == AT_SOCKET_ROLE_SERVER) {
		if (sock.fd_peer != INVALID_SOCKET) {
			sockfd = sock.fd_peer;
		} else {
			LOG_ERR("No connection");
			return -EINVAL;
		}
	}

	uint32_t offset = 0;

	while (offset < datalen) {
		ret = socket_poll(sockfd, POLLOUT, SOCKET_SEND_TMO_SEC);
		if (ret) {
			break;
		}
		ret = send(sockfd, data + offset, datalen - offset, 0);
		if (ret < 0) {
			LOG_ERR("send() failed: %d, sent: %d", -errno, offset);
			ret = -errno;
			break;
		}
		offset += ret;
	}

	rsp_send("\r\n#XSEND: %d\r\n", offset);

	if (ret >= 0) {
		return 0;
	}

	return ret;
}

static int do_send_datamode(const uint8_t *data, int datalen)
{
	int ret = 0;
	int sockfd = sock.fd;

	/* For TCP/TLS Server, send to incoming socket */
	if (sock.type == SOCK_STREAM && sock.role == AT_SOCKET_ROLE_SERVER) {
		if (sock.fd_peer != INVALID_SOCKET) {
			sockfd = sock.fd_peer;
		} else {
			LOG_ERR("No connection");
			return -EINVAL;
		}
	}

	uint32_t offset = 0;

	while (offset < datalen) {
		ret = socket_poll(sockfd, POLLOUT, SOCKET_SEND_TMO_SEC);
		if (ret) {
			break;
		}
		ret = send(sockfd, data + offset, datalen - offset, 0);
		if (ret < 0) {
			LOG_ERR("send() failed: %d, sent: %d", -errno, offset);
			break;
		}
		offset += ret;
	}

	return (offset > 0) ? offset : -1;
}

static int do_recv(int timeout, int flags)
{
	int ret;
	int sockfd = sock.fd;

	/* For TCP/TLS Server, receive from incoming socket */
	if (sock.type == SOCK_STREAM && sock.role == AT_SOCKET_ROLE_SERVER) {
		if (sock.fd_peer != INVALID_SOCKET) {
			sockfd = sock.fd_peer;
		} else {
			LOG_ERR("No remote connection");
			return -EINVAL;
		}
	}

	ret = socket_poll(sockfd, POLLIN, timeout);
	if (ret) {
		return ret;
	}
	ret = recv(sockfd, (void *)data_buf, sizeof(data_buf), flags);
	if (ret < 0) {
		LOG_WRN("recv() error: %d", -errno);
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
	} else {
		rsp_send("\r\n#XRECV: %d\r\n", ret);
		data_send(data_buf, ret);
		ret = 0;
	}

	return ret;
}

static int do_sendto(const char *url, uint16_t port, const uint8_t *data, int datalen)
{
	int ret = 0;
	uint32_t offset = 0;
	struct sockaddr sa = {
		.sa_family = AF_UNSPEC
	};

	LOG_DBG("sendto %s:%d", url, port);
	ret = util_resolve_host(sock.cid, url, port, sock.family, &sa);
	if (ret) {
		LOG_ERR("getaddrinfo() error: %s", gai_strerror(ret));
		return -EAGAIN;
	}

	while (offset < datalen) {
		ret = socket_poll(sock.fd, POLLOUT, SOCKET_SEND_TMO_SEC);
		if (ret) {
			break;
		}
		if (sa.sa_family == AF_INET) {
			ret = sendto(sock.fd, data + offset, datalen - offset, 0,
				&sa, sizeof(struct sockaddr_in));
		} else {
			ret = sendto(sock.fd, data + offset, datalen - offset, 0,
				&sa, sizeof(struct sockaddr_in6));
		}
		if (ret <= 0) {
			LOG_ERR("sendto() failed: %d, sent: %d", -errno, offset);
			ret = -errno;
			break;
		}
		offset += ret;
	}

	rsp_send("\r\n#XSENDTO: %d\r\n", offset);

	if (ret >= 0) {
		return 0;
	}

	return ret;
}

static int do_sendto_datamode(const uint8_t *data, int datalen)
{
	int ret = 0;
	struct sockaddr sa = {
		.sa_family = AF_UNSPEC
	};

	LOG_DBG("sendto %s:%d", udp_url, udp_port);
	ret = util_resolve_host(sock.cid, udp_url, udp_port, sock.family, &sa);
	if (ret) {
		LOG_ERR("getaddrinfo() error: %s", gai_strerror(ret));
		return -EAGAIN;
	}

	uint32_t offset = 0;

	while (offset < datalen) {
		ret = socket_poll(sock.fd, POLLOUT, SOCKET_SEND_TMO_SEC);
		if (ret) {
			break;
		}
		if (sa.sa_family == AF_INET) {
			ret = sendto(sock.fd, data + offset, datalen - offset, 0,
				&sa, sizeof(struct sockaddr_in));
		} else {
			ret = sendto(sock.fd, data + offset, datalen - offset, 0,
				&sa, sizeof(struct sockaddr_in6));
		}
		if (ret <= 0) {
			LOG_ERR("sendto() failed: %d, sent: %d", -errno, offset);
			break;
		}
		offset += ret;
	}

	return (offset > 0) ? offset : -1;
}

static int do_recvfrom(int timeout, int flags)
{
	int ret;
	struct sockaddr remote;
	socklen_t addrlen = sizeof(struct sockaddr);

	ret = socket_poll(sock.fd, POLLIN, timeout);
	if (ret) {
		return ret;
	}
	ret = recvfrom(sock.fd, (void *)data_buf, sizeof(data_buf), flags, &remote, &addrlen);
	if (ret < 0) {
		LOG_ERR("recvfrom() error: %d", -errno);
		return -errno;
	}
	/**
	 * Datagram sockets in various domains permit zero-length
	 * datagrams. When such a datagram is received, the return
	 * value is 0. Treat as normal case
	 */
	if (ret == 0) {
		LOG_WRN("recvfrom() return 0");
	} else {
		char peer_addr[NET_IPV6_ADDR_LEN] = {0};

		if (remote.sa_family == AF_INET) {
			(void)inet_ntop(AF_INET, &((struct sockaddr_in *)&remote)->sin_addr,
			    peer_addr, sizeof(peer_addr));
		} else if (remote.sa_family == AF_INET6) {
			(void)inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&remote)->sin6_addr,
			    peer_addr, sizeof(peer_addr));
		}
		rsp_send("\r\n#XRECVFROM: %d,\"%s\"\r\n", ret, peer_addr);
		data_send(data_buf, ret);
	}

	return 0;
}

static int do_poll(int timeout)
{
	int ret = poll(fds, SLM_MAX_SOCKET_COUNT, timeout);

	if (ret < 0) {
		rsp_send("\r\n#XPOLL: %d\r\n", ret);
		return ret;
	}
	/* ret == 0 means timeout */
	if (ret > 0) {
		for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
			/* If fd is equal to -1	then revents is cleared (set to zero) */
			if (fds[i].revents != 0) {
				rsp_send("\r\n#XPOLL: %d,\"0x%04x\"\r\n",
					fds[i].fd, fds[i].revents);
			}
		}
	}

	return 0;
}

static int socket_poll(int sock_fd, int event, int timeout)
{
	int ret;
	struct pollfd fd = {
		.fd = sock_fd,
		.events = event
	};

	if (timeout <= 0) {
		return 0;
	}

	ret = poll(&fd, 1, MSEC_PER_SEC * timeout);
	if (ret < 0) {
		LOG_WRN("poll() error: %d", -errno);
		return -errno;
	} else if (ret == 0) {
		LOG_WRN("poll() timeout");
		return -EAGAIN;
	}

	LOG_DBG("poll() events 0x%08x", fd.revents);
	if ((fd.revents & event) != event) {
		return -EAGAIN;
	}

	return 0;
}

static int socket_datamode_callback(uint8_t op, const uint8_t *data, int len)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		if (strlen(udp_url) > 0) {
			ret = do_sendto_datamode(data, len);
		} else {
			ret = do_send_datamode(data, len);
		}
		LOG_INF("datamode send: %d", ret);
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("datamode exit");
		memset(udp_url, 0x00, sizeof(udp_url));
	}

	return ret;
}

/**@brief handle AT#XSOCKET commands
 *  AT#XSOCKET=<op>[,<type>,<role>[,<cid>]]
 *  AT#XSOCKET?
 *  AT#XSOCKET=?
 */
int handle_at_socket(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == AT_SOCKET_OPEN || op == AT_SOCKET_OPEN6) {
			if (find_avail_socket() < 0) {
				LOG_ERR("Max socket count reached");
				return -EINVAL;
			}
			INIT_SOCKET(sock);
			err = at_params_unsigned_short_get(&at_param_list, 2, &sock.type);
			if (err) {
				return err;
			}
			err = at_params_unsigned_short_get(&at_param_list, 3, &sock.role);
			if (err) {
				return err;
			}
			sock.family = (op == AT_SOCKET_OPEN) ? AF_INET : AF_INET6;
			if (at_params_valid_count_get(&at_param_list) > 4) {
				err = at_params_unsigned_short_get(&at_param_list, 4, &sock.cid);
				if (err) {
					return err;
				}
				if (sock.cid > 10) {
					return -EINVAL;
				}
			}
			err = do_socket_open();
		} else if (op == AT_SOCKET_CLOSE) {
			err = do_socket_close();
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (sock.fd != INVALID_SOCKET) {
			rsp_send("\r\n#XSOCKET: %d,%d,%d,%d,%d\r\n", sock.fd,
				sock.family, sock.role, sock.type, sock.cid);
		}
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XSOCKET: (%d,%d,%d),(%d,%d,%d),(%d,%d),<cid>",
			AT_SOCKET_CLOSE, AT_SOCKET_OPEN, AT_SOCKET_OPEN6,
			SOCK_STREAM, SOCK_DGRAM, SOCK_RAW,
			AT_SOCKET_ROLE_CLIENT, AT_SOCKET_ROLE_SERVER);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XSOCKET commands
 *  AT#XSSOCKET=<op>[,<type>,<role>,<sec_tag>[,<peer_verify>[,<cid>]]]
 *  AT#XSSOCKET?
 *  AT#XSSOCKET=?
 */
int handle_at_secure_socket(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == AT_SOCKET_OPEN || op == AT_SOCKET_OPEN6) {
			/** Peer verification level for TLS connection.
			 *    - 0 - none
			 *    - 1 - optional
			 *    - 2 - required
			 * If not set, socket will use defaults (none for servers,
			 * required for clients)
			 */
			uint16_t peer_verify;

			if (find_avail_socket() < 0) {
				LOG_ERR("Max socket count reached");
				return -EINVAL;
			}
			INIT_SOCKET(sock);
			err = at_params_unsigned_short_get(&at_param_list, 2, &sock.type);
			if (err) {
				return err;
			}
			err = at_params_unsigned_short_get(&at_param_list, 3, &sock.role);
			if (err) {
				return err;
			}
			if (sock.role == AT_SOCKET_ROLE_SERVER) {
				peer_verify = TLS_PEER_VERIFY_NONE;
			} else if (sock.role == AT_SOCKET_ROLE_CLIENT) {
				peer_verify = TLS_PEER_VERIFY_REQUIRED;
			} else {
				return -EINVAL;
			}
			err = at_params_unsigned_int_get(&at_param_list, 4, &sock.sec_tag);
			if (err) {
				return err;
			}
			if (at_params_valid_count_get(&at_param_list) > 5) {
				err = at_params_unsigned_short_get(&at_param_list, 5,
								   &peer_verify);
				if (err) {
					return err;
				}
			}
			sock.family = (op == AT_SOCKET_OPEN) ? AF_INET : AF_INET6;
			if (at_params_valid_count_get(&at_param_list) > 6) {
				err = at_params_unsigned_short_get(&at_param_list, 6, &sock.cid);
				if (err) {
					return err;
				}
				if (sock.cid > 10) {
					return -EINVAL;
				}
			}
			err = do_secure_socket_open(peer_verify);
		} else if (op == AT_SOCKET_CLOSE) {
			err = do_socket_close();
		} else {
			err = -EINVAL;
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (sock.fd != INVALID_SOCKET) {
			rsp_send("\r\n#XSSOCKET: %d,%d,%d,%d,%d,%d\r\n", sock.fd,
				sock.family, sock.role, sock.type, sock.sec_tag, sock.cid);
		}
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XSSOCKET: (%d,%d,%d),(%d,%d),(%d,%d),"
			 "<sec-tag>,<peer_verify>,<cid>\r\n",
			AT_SOCKET_CLOSE, AT_SOCKET_OPEN, AT_SOCKET_OPEN6,
			SOCK_STREAM, SOCK_DGRAM,
			AT_SOCKET_ROLE_CLIENT, AT_SOCKET_ROLE_SERVER);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XSOCKETSELECT commands
 *  AT#XSOCKETSELECT=<fd>
 *  AT#XSOCKETSELECT?
 *  AT#XSOCKETSELECT=?
 */
int handle_at_socket_select(enum at_cmd_type cmd_type)
{
	int err = 0;
	int fd;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_int_get(&at_param_list, 1, &fd);
		if (err) {
			return err;
		}
		if (fd < 0) {
			return -EINVAL;
		}
		for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
			if (socks[i].fd == fd) {
				sock = socks[i];
				rsp_send("\r\n#XSOCKETSELECT: %d\r\n", sock.fd);
				return 0;
			}
		}
		err = -EBADF;
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
			if (socks[i].fd != INVALID_SOCKET) {
				rsp_send("\r\n#XSOCKETSELECT: %d,%d,%d,%d,%d,%d,%d\r\n",
					socks[i].fd, socks[i].family, socks[i].role,
					socks[i].type, socks[i].sec_tag, socks[i].ranking,
					socks[i].cid);
			}
		}
		if (sock.fd != INVALID_SOCKET) {
			rsp_send("\r\n#XSOCKETSELECT: %d\r\n", sock.fd);
		}
		break;

	default:
		break;
	}

	return err;

}

/**@brief handle AT#XSOCKETOPT commands
 *  AT#XSOCKETOPT=<op>,<name>[,<value>]
 *  AT#XSOCKETOPT? READ command not supported
 *  AT#XSOCKETOPT=?
 */
int handle_at_socketopt(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t name;
	int value = 0;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err) {
			return err;
		}
		err = at_params_unsigned_short_get(&at_param_list, 2, &name);
		if (err) {
			return err;
		}
		if (op == AT_SOCKETOPT_SET) {
			/* some options don't require a value */
			if (at_params_valid_count_get(&at_param_list) > 3) {
				err = at_params_int_get(&at_param_list, 3, &value);
				if (err) {
					return err;
				}
			}
			err = do_socketopt_set(name, value);
		} else if (op == AT_SOCKETOPT_GET) {
			err = do_socketopt_get(name);
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XSOCKETOPT: (%d,%d),<name>,<value>\r\n",
			AT_SOCKETOPT_GET, AT_SOCKETOPT_SET);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XSSOCKETOPT commands
 *  AT#XSSOCKETOPT=<op>,<name>[,<value>]
 *  AT#XSSOCKETOPT? READ command not supported
 *  AT#XSSOCKETOPT=?
 */
int handle_at_secure_socketopt(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t name;
	enum at_param_type type = AT_PARAM_TYPE_NUM_INT;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (sock.sec_tag == INVALID_SEC_TAG) {
			LOG_ERR("Not secure socket");
			return err;
		}
		err = at_params_unsigned_short_get(&at_param_list, 1, &op);
		if (err) {
			return err;
		}
		err = at_params_unsigned_short_get(&at_param_list, 2, &name);
		if (err) {
			return err;
		}
		if (op == AT_SOCKETOPT_SET) {
			int value_int = 0;
			char value_str[SLM_MAX_URL] = {0};
			int size = SLM_MAX_URL;

			type = at_params_type_get(&at_param_list, 3);
			if (type == AT_PARAM_TYPE_NUM_INT) {
				err = at_params_int_get(&at_param_list, 3, &value_int);
				if (err) {
					return err;
				}
				err = do_secure_socketopt_set_int(name, value_int);
			} else if (type == AT_PARAM_TYPE_STRING) {
				err = util_string_get(&at_param_list, 3, value_str, &size);
				if (err) {
					return err;
				}
				err = do_secure_socketopt_set_str(name, value_str);
			} else {
				return -EINVAL;
			}
		}  else if (op == AT_SOCKETOPT_GET) {
			err = do_secure_socketopt_get(name);
		} break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XSSOCKETOPT: (%d),<name>,<value>\r\n", AT_SOCKETOPT_SET);
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
int handle_at_bind(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t port;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &port);
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
int handle_at_connect(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char url[SLM_MAX_URL] = {0};
	int size = SLM_MAX_URL;
	uint16_t port;

	if (sock.role != AT_SOCKET_ROLE_CLIENT) {
		LOG_ERR("Invalid role");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = util_string_get(&at_param_list, 1, url, &size);
		if (err) {
			return err;
		}
		err = at_params_unsigned_short_get(&at_param_list, 2, &port);
		if (err) {
			return err;
		}
		err = do_connect(url, port);
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
int handle_at_listen(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;

	if (sock.role != AT_SOCKET_ROLE_SERVER) {
		LOG_ERR("Invalid role");
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
 *  AT#XACCEPT=<timeout>
 *  AT#XACCEPT? READ command not supported
 *  AT#XACCEPT=? TEST command not supported
 */
int handle_at_accept(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	int timeout;

	if (sock.role != AT_SOCKET_ROLE_SERVER) {
		LOG_ERR("Invalid role");
		return err;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_int_get(&at_param_list, 1, &timeout);
		if (err) {
			return err;
		}
		err = do_accept(timeout);
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (sock.fd_peer != INVALID_SOCKET) {
			rsp_send("\r\n#XTCPACCEPT: %d\r\n", sock.fd_peer);
		} else {
			rsp_send("\r\n#XTCPACCEPT: 0\r\n");
		}
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XSEND commands
 *  AT#XSEND[=<data>]
 *  AT#XSEND? READ command not supported
 *  AT#XSEND=? TEST command not supported
 */
int handle_at_send(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};
	int size;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&at_param_list) > 1) {
			size = sizeof(data);
			err = util_string_get(&at_param_list, 1, data, &size);
			if (err) {
				return err;
			}
			err = do_send(data, size);
		} else {
			err = enter_datamode(socket_datamode_callback);
		}
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XRECV commands
 *  AT#XRECV=<timeout>
 *  AT#XRECV? READ command not supported
 *  AT#XRECV=? TEST command not supported
 */
int handle_at_recv(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	int timeout;
	int flags = 0;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_int_get(&at_param_list, 1, &timeout);
		if (err) {
			return err;
		}
		if (at_params_valid_count_get(&at_param_list) > 2) {
			err = at_params_int_get(&at_param_list, 2, &flags);
			if (err) {
				return err;
			}
		}
		err = do_recv(timeout, flags);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XSENDTO commands
 *  AT#XSENDTO=<url>,<port>[<data>]
 *  AT#XSENDTO? READ command not supported
 *  AT#XSENDTO=? TEST command not supported
 */
int handle_at_sendto(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	int size;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		size = sizeof(udp_url);
		err = util_string_get(&at_param_list, 1, udp_url, &size);
		if (err) {
			return err;
		}
		err = at_params_unsigned_short_get(&at_param_list, 2, &udp_port);
		if (err) {
			return err;
		}
		if (at_params_valid_count_get(&at_param_list) > 3) {
			char data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};

			size = sizeof(data);
			err = util_string_get(&at_param_list, 3, data, &size);
			if (err) {
				return err;
			}
			err = do_sendto(udp_url, udp_port, data, size);
		} else {
			err = enter_datamode(socket_datamode_callback);
		}
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XRECVFROM commands
 *  AT#XRECVFROM=<timeout>[,<flags>]
 *  AT#XRECVFROM? READ command not supported
 *  AT#XRECVFROM=? TEST command not supported
 */
int handle_at_recvfrom(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	int timeout;
	int flags = 0;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_int_get(&at_param_list, 1, &timeout);
		if (err) {
			return err;
		}
		if (at_params_valid_count_get(&at_param_list) > 2) {
			err = at_params_int_get(&at_param_list, 2, &flags);
			if (err) {
				return err;
			}
		}
		err = do_recvfrom(timeout, flags);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XGETADDRINFO commands
 *  AT#XGETADDRINFO=<url>
 *  AT#XGETADDRINFO? READ command not supported
 *  AT#XGETADDRINFO=? TEST command not supported
 */
int handle_at_getaddrinfo(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char hostname[NI_MAXHOST];
	char host[SLM_MAX_URL];
	int size = SLM_MAX_URL;
	struct addrinfo *result;
	struct addrinfo *res;
	char rsp_buf[256];

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = util_string_get(&at_param_list, 1, host, &size);
		if (err) {
			return err;
		}
		err = getaddrinfo(host, NULL, NULL, &result);
		if (err) {
			rsp_send("\r\n#XGETADDRINFO: \"%s\"\r\n", gai_strerror(err));
			return err;
		} else if (result == NULL) {
			rsp_send("\r\n#XGETADDRINFO: \"not found\"\r\n");
			return -ENOENT;
		}

		sprintf(rsp_buf, "\r\n#XGETADDRINFO: \"");
		/* loop over all returned results and do inverse lookup */
		for (res = result; res != NULL; res = res->ai_next) {
			if (res->ai_family == AF_INET) {
				struct sockaddr_in *host = (struct sockaddr_in *)result->ai_addr;

				inet_ntop(AF_INET, &host->sin_addr, hostname, sizeof(hostname));
			} else if (res->ai_family == AF_INET6) {
				struct sockaddr_in6 *host = (struct sockaddr_in6 *)result->ai_addr;

				inet_ntop(AF_INET6, &host->sin6_addr, hostname, sizeof(hostname));
			} else {
				continue;
			}

			strcat(rsp_buf, hostname);
			if (res->ai_next) {
				strcat(rsp_buf, " ");
			}
		}
		strcat(rsp_buf, "\"\r\n");
		rsp_send("%s", rsp_buf);
		freeaddrinfo(result);
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XPOLL commands
 *  AT#XPOLL=<timeout>[,<handle1>[,<handle2> ...<handle8>]
 *  AT#XPOLL? READ command not support
 *  AT#XPOLL=? TEST command not support
 */
int handle_at_poll(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	int timeout, handle;
	int count = at_params_valid_count_get(&at_param_list);

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_int_get(&at_param_list, 1, &timeout);
		if (err) {
			return err;
		}
		if (count == 2) {
			/* poll all opened socket */
			for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
				fds[i].fd = socks[i].fd;
				if (fds[i].fd != INVALID_SOCKET) {
					fds[i].events = POLLIN;
				}
			}
		} else {
			/* poll selected sockets */
			for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
				fds[i].fd = INVALID_SOCKET;
				if (count > 2 + i) {
					err = at_params_int_get(&at_param_list, 2 + i, &handle);
					if (err) {
						return err;
					}
					if (!is_opened_socket(handle)) {
						return -EINVAL;
					}
					fds[i].fd = handle;
					fds[i].events = POLLIN;
				}
			}
		}
		err = do_poll(timeout);
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to initialize Socket AT commands handler
 */
int slm_at_socket_init(void)
{
	INIT_SOCKET(sock);
	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		INIT_SOCKET(socks[i]);
	}
	socket_ranking = 1;

	return 0;
}

/**@brief API to uninitialize Socket AT commands handler
 */
int slm_at_socket_uninit(void)
{
	(void)do_socket_close();
	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		if (socks[i].fd_peer != INVALID_SOCKET) {
			close(socks[i].fd_peer);
		}
		if (socks[i].fd != INVALID_SOCKET) {
			close(socks[i].fd);
		}
	}

	return 0;
}
