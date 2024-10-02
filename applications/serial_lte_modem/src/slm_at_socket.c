/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_ip.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_socket.h"
#include "slm_sockopt.h"
#if defined(CONFIG_SLM_NATIVE_TLS)
#include "slm_native_tls.h"
#endif

LOG_MODULE_REGISTER(slm_sock, CONFIG_SLM_LOG_LEVEL);

#define SLM_MAX_SOCKET_COUNT CONFIG_POSIX_OPEN_MAX

/*
 * Known limitation in this version
 * - Multiple concurrent sockets
 * - TCP server accept one connection only
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
	int type;          /* SOCK_STREAM or SOCK_DGRAM */
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

/* forward declarations */
#define SOCKET_SEND_TMO_SEC 30
static int socket_poll(int sock_fd, int event, int timeout);
static int handle_at_sendto(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			    uint32_t param_count);

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

static int bind_to_pdn(uint16_t cid)
{
	int ret = 0;

	if (cid > 0) {
		int cid_int = cid;

		ret = setsockopt(sock.fd, SOL_SOCKET, SO_BINDTOPDN, &cid_int, sizeof(int));
		if (ret < 0) {
			LOG_ERR("SO_BINDTOPDN error: %d", -errno);
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
	struct timeval tmo = {.tv_sec = SOCKET_SEND_TMO_SEC};

	ret = setsockopt(sock.fd, SOL_SOCKET, SO_SNDTIMEO, &tmo, sizeof(tmo));
	if (ret) {
		LOG_ERR("setsockopt(%d) error: %d", SO_SNDTIMEO, -errno);
		ret = -errno;
		goto error;
	}

	/* Explicitly bind to secondary PDP context if required */
	ret = bind_to_pdn(sock.cid);
	if (ret) {
		goto error;
	}

	sock.ranking = socket_ranking++;
	ret = find_avail_socket();
	if (ret < 0) {
		goto error;
	}
	socks[ret] = sock;
	rsp_send("\r\n#XSOCKET: %d,%d,%d\r\n", sock.fd, sock.type, proto);

	return 0;

error:
	close(sock.fd);
	sock.fd = INVALID_SOCKET;
	return ret;
}

static int do_secure_socket_open(int peer_verify)
{
	int ret = 0;
	int proto = sock.type == SOCK_STREAM ? IPPROTO_TLS_1_2 : IPPROTO_DTLS_1_2;

	if (sock.type != SOCK_STREAM && sock.type != SOCK_DGRAM) {
		LOG_ERR("socket type %d not supported", sock.type);
		return -ENOTSUP;
	}

	ret = socket(sock.family, sock.type, proto);
	if (ret < 0) {
		LOG_ERR("socket() error: %d", -errno);
		return -errno;
	}
	sock.fd = ret;

#if defined(CONFIG_SLM_NATIVE_TLS)
	ret = slm_native_tls_load_credentials(sock.sec_tag);
	if (ret < 0) {
		LOG_ERR("Failed to load sec tag: %d (%d)", sock.sec_tag, ret);
		goto error;
	}
	int tls_native = 1;

	/* Must be the first socket option to set. */
	ret = setsockopt(sock.fd, SOL_TLS, TLS_NATIVE, &tls_native, sizeof(tls_native));
	if (ret) {
		goto error;
	}
#endif
	struct timeval tmo = {.tv_sec = SOCKET_SEND_TMO_SEC};

	ret = setsockopt(sock.fd, SOL_SOCKET, SO_SNDTIMEO, &tmo, sizeof(tmo));
	if (ret) {
		LOG_ERR("setsockopt(%d) error: %d", SO_SNDTIMEO, -errno);
		ret = -errno;
		goto error;
	}

	/* Explicitly bind to secondary PDP context if required */
	ret = bind_to_pdn(sock.cid);
	if (ret) {
		goto error;
	}
	sec_tag_t sec_tag_list[1] = { sock.sec_tag };

	ret = setsockopt(sock.fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_t));
	if (ret) {
		LOG_ERR("setsockopt(TLS_SEC_TAG_LIST) error: %d", -errno);
		ret = -errno;
		goto error;
	}

	/* Set up (D)TLS peer verification */
	ret = setsockopt(sock.fd, SOL_TLS, TLS_PEER_VERIFY, &peer_verify, sizeof(peer_verify));
	if (ret) {
		LOG_ERR("setsockopt(TLS_PEER_VERIFY) error: %d", errno);
		ret = -errno;
		goto error;
	}
	/* Set up (D)TLS server role if applicable */
	if (sock.role == AT_SOCKET_ROLE_SERVER) {
		int tls_role = TLS_DTLS_ROLE_SERVER;

		ret = setsockopt(sock.fd, SOL_TLS, TLS_DTLS_ROLE, &tls_role, sizeof(int));
		if (ret) {
			LOG_ERR("setsockopt(TLS_DTLS_ROLE) error: %d", -errno);
			ret = -errno;
			goto error;
		}
	}

	sock.ranking = socket_ranking++;
	ret = find_avail_socket();
	if (ret < 0) {
		goto error;
	}
	socks[ret] = sock;
	rsp_send("\r\n#XSSOCKET: %d,%d,%d\r\n", sock.fd, sock.type, proto);

	return 0;

error:
	close(sock.fd);
	sock.fd = INVALID_SOCKET;
	return ret;
}

static int do_socket_close(void)
{
	int ret;

	if (sock.fd == INVALID_SOCKET) {
		return 0;
	}

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

static int at_sockopt_to_sockopt(enum at_sockopt at_option, int *level, int *option)
{
	switch (at_option) {
	case AT_SO_REUSEADDR:
		*level = SOL_SOCKET;
		*option = SO_REUSEADDR;
		break;
	case AT_SO_RCVTIMEO:
		*level = SOL_SOCKET;
		*option = SO_RCVTIMEO;
		break;
	case AT_SO_SNDTIMEO:
		*level = SOL_SOCKET;
		*option = SO_SNDTIMEO;
		break;
	case AT_SO_SILENCE_ALL:
		*level = IPPROTO_ALL;
		*option = SO_SILENCE_ALL;
		break;
	case AT_SO_IP_ECHO_REPLY:
		*level = IPPROTO_IP;
		*option = SO_IP_ECHO_REPLY;
		break;
	case AT_SO_IPV6_ECHO_REPLY:
		*level = IPPROTO_IPV6;
		*option = SO_IPV6_ECHO_REPLY;
		break;
	case AT_SO_BINDTOPDN:
		*level = SOL_SOCKET;
		*option = SO_BINDTOPDN;
		break;
	case AT_SO_RAI:
		*level = SOL_SOCKET;
		*option = SO_RAI;
		break;
	case AT_SO_TCP_SRV_SESSTIMEO:
		*level = IPPROTO_TCP;
		*option = SO_TCP_SRV_SESSTIMEO;
		break;

	default:
		LOG_WRN("Unsupported option: %d", at_option);
		return -ENOTSUP;
	}

	return 0;
}

static int sockopt_set(enum at_sockopt at_option, int at_value)
{
	int ret, level, option;
	void *value = &at_value;
	socklen_t len = sizeof(at_value);
	struct timeval tmo;

	ret = at_sockopt_to_sockopt(at_option, &level, &option);
	if (ret) {
		return ret;
	}

	/* Options with special handling. */
	if (level == SOL_SOCKET && (option == SO_RCVTIMEO || option == SO_SNDTIMEO)) {
		tmo.tv_sec = at_value;
		value = &tmo;
		len = sizeof(tmo);
	}

	ret = setsockopt(sock.fd, level, option, value, len);
	if (ret) {
		LOG_ERR("setsockopt(%d,%d) error: %d", level, option, -errno);
	}

	return ret;
}

static int sockopt_get(enum at_sockopt at_option)
{
	int ret, value, level, option;
	socklen_t len = sizeof(int);

	ret = at_sockopt_to_sockopt(at_option, &level, &option);
	if (ret) {
		return ret;
	}

	/* Options with special handling. */
	if (level == SOL_SOCKET && (option == SO_RCVTIMEO || option == SO_SNDTIMEO)) {
		struct timeval tmo;

		len = sizeof(struct timeval);
		ret = getsockopt(sock.fd, level, option, &tmo, &len);
		if (ret == 0) {
			rsp_send("\r\n#XSOCKETOPT: %ld\r\n", (long)tmo.tv_sec);
		}
	} else {
		/* Default */
		ret = getsockopt(sock.fd, level, option, &value, &len);
		if (ret == 0) {
			rsp_send("\r\n#XSOCKETOPT: %d\r\n", value);
		}
	}

	if (ret) {
		LOG_ERR("getsockopt(%d,%d) error: %d", level, option, -errno);
	}

	return ret;
}

static int at_sec_sockopt_to_sockopt(enum at_sec_sockopt at_option, int *level, int *option)
{
	*level = SOL_TLS;

	switch (at_option) {
	case AT_TLS_HOSTNAME:
		*option = TLS_HOSTNAME;
		break;
	case AT_TLS_CIPHERSUITE_USED:
		*option = TLS_CIPHERSUITE_USED;
		break;
	case AT_TLS_PEER_VERIFY:
		*option = TLS_PEER_VERIFY;
		break;
	case AT_TLS_SESSION_CACHE:
		*option = TLS_SESSION_CACHE;
		break;
	case AT_TLS_SESSION_CACHE_PURGE:
		*option = TLS_SESSION_CACHE_PURGE;
		break;
	case AT_TLS_DTLS_CID:
		*option = TLS_DTLS_CID;
		break;
	case AT_TLS_DTLS_CID_STATUS:
		*option = TLS_DTLS_CID_STATUS;
		break;
	case AT_TLS_DTLS_HANDSHAKE_TIMEO:
		*option = TLS_DTLS_HANDSHAKE_TIMEO;
		break;
	default:
		LOG_WRN("Unsupported option: %d", at_option);
		return -ENOTSUP;
	}

	return 0;
}

static int sec_sockopt_set(enum at_sec_sockopt at_option, void *value, socklen_t len)
{
	int ret, level, option;

	ret = at_sec_sockopt_to_sockopt(at_option, &level, &option);
	if (ret) {
		return ret;
	}

	/* Options with special handling. */
	if (level == SOL_TLS && option == TLS_HOSTNAME) {
		if (slm_util_casecmp(value, "NULL")) {
			value = NULL;
			len = 0;
		}
	} else if (len != sizeof(int)) {
		return -EINVAL;
	}

	ret = setsockopt(sock.fd, level, option, value, len);
	if (ret) {
		LOG_ERR("setsockopt(%d,%d) error: %d", level, option, -errno);
	}

	return ret;
}


static int sec_sockopt_get(enum at_sec_sockopt at_option)
{
	int ret, value, level, option;
	socklen_t len = sizeof(int);

	ret = at_sec_sockopt_to_sockopt(at_option, &level, &option);
	if (ret) {
		return ret;
	}

	/* Options with special handling. */
	if (level == SOL_TLS && option == TLS_CIPHERSUITE_USED) {
		ret = getsockopt(sock.fd, level, option, &value, &len);
		if (ret == 0) {
			rsp_send("\r\n#XSSOCKETOPT: 0x%x\r\n", value);
		}
	} else if (level == SOL_TLS && option == TLS_HOSTNAME) {
		char hostname[SLM_MAX_URL] = {0};

		len = sizeof(hostname);
		ret = getsockopt(sock.fd, level, option, &hostname, &len);
		if (ret == 0) {
			rsp_send("\r\n#XSSOCKETOPT: %s\r\n", hostname);
		}
	} else {
		/* Default */
		ret = getsockopt(sock.fd, level, option, &value, &len);
		if (ret == 0) {
			rsp_send("\r\n#XSSOCKETOPT: %d\r\n", value);
		}
	}

	if (ret) {
		LOG_ERR("getsockopt(%d,%d) error: %d", level, option, -errno);
	}

	return ret;
}

int slm_bind_to_local_addr(int socket, int family, uint16_t port)
{
	int ret;

	if (family == AF_INET) {
		char ipv4_addr[INET_ADDRSTRLEN];

		util_get_ip_addr(0, ipv4_addr, NULL);
		if (!*ipv4_addr) {
			LOG_ERR("Get local IPv4 address failed");
			return -ENETDOWN;
		}

		struct sockaddr_in local = {
			.sin_family = AF_INET,
			.sin_port = htons(port)
		};

		if (inet_pton(AF_INET, ipv4_addr, &local.sin_addr) != 1) {
			LOG_ERR("Parse local IPv4 address failed: %d", -errno);
			return -EINVAL;
		}

		ret = bind(socket, (struct sockaddr *)&local, sizeof(struct sockaddr_in));
		if (ret) {
			LOG_ERR("bind() sock %d failed: %d", socket, -errno);
			return -errno;
		}
		LOG_DBG("bind sock %d to %s", socket, ipv4_addr);
	} else if (family == AF_INET6) {
		char ipv6_addr[INET6_ADDRSTRLEN];

		util_get_ip_addr(0, NULL, ipv6_addr);
		if (!*ipv6_addr) {
			LOG_ERR("Get local IPv6 address failed");
			return -ENETDOWN;
		}

		struct sockaddr_in6 local = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(port)
		};

		if (inet_pton(AF_INET6, ipv6_addr, &local.sin6_addr) != 1) {
			LOG_ERR("Parse local IPv6 address failed: %d", -errno);
			return -EINVAL;
		}
		ret = bind(socket, (struct sockaddr *)&local, sizeof(struct sockaddr_in6));
		if (ret) {
			LOG_ERR("bind() sock %d failed: %d", socket, -errno);
			return -errno;
		}
		LOG_DBG("bind sock %d to %s", socket, ipv6_addr);
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
	struct timeval tmo = {.tv_sec = timeout};

	ret = setsockopt(sock.fd, SOL_SOCKET, SO_RCVTIMEO, &tmo, sizeof(tmo));
	if (ret) {
		LOG_ERR("setsockopt(%d) error: %d", SO_RCVTIMEO, -errno);
		return -errno;
	}
	ret = recv(sockfd, (void *)slm_data_buf, sizeof(slm_data_buf), flags);
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
		data_send(slm_data_buf, ret);
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
		return -EAGAIN;
	}

	while (offset < datalen) {
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
		return -EAGAIN;
	}

	uint32_t offset = 0;

	while (offset < datalen) {
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
	struct timeval tmo = {.tv_sec = timeout};

	ret = setsockopt(sock.fd, SOL_SOCKET, SO_RCVTIMEO, &tmo, sizeof(tmo));
	if (ret) {
		LOG_ERR("setsockopt(%d) error: %d", SO_RCVTIMEO, -errno);
		return -errno;
	}
	ret = recvfrom(
		sock.fd, (void *)slm_data_buf, sizeof(slm_data_buf), flags, &remote, &addrlen);
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
		char peer_addr[INET6_ADDRSTRLEN] = {0};
		uint16_t peer_port = 0;

		util_get_peer_addr(&remote, peer_addr, &peer_port);
		rsp_send("\r\n#XRECVFROM: %d,\"%s\",%d\r\n", ret, peer_addr, peer_port);
		data_send(slm_data_buf, ret);
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

static int socket_datamode_callback(uint8_t op, const uint8_t *data, int len, uint8_t flags)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		if (sock.type == SOCK_DGRAM && (flags & SLM_DATAMODE_FLAGS_MORE_DATA) != 0) {
			LOG_ERR("Datamode buffer overflow");
			exit_datamode_handler(-EOVERFLOW);
			return -EOVERFLOW;
		} else {
			if (strlen(udp_url) > 0) {
				ret = do_sendto_datamode(data, len);
			} else {
				ret = do_send_datamode(data, len);
			}
			LOG_INF("datamode send: %d", ret);
		}
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("datamode exit");
		memset(udp_url, 0, sizeof(udp_url));
	}

	return ret;
}

SLM_AT_CMD_CUSTOM(xsocket_set, "AT#XSOCKET=", handle_at_socket);
SLM_AT_CMD_CUSTOM(xsocket_read, "AT#XSOCKET?", handle_at_socket);
static int handle_at_socket(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			    uint32_t param_count)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &op);
		if (err) {
			return err;
		}
		if (op == AT_SOCKET_OPEN || op == AT_SOCKET_OPEN6) {
			if (find_avail_socket() < 0) {
				LOG_ERR("Max socket count reached");
				return -EINVAL;
			}
			INIT_SOCKET(sock);
			err = at_parser_num_get(parser, 2, &sock.type);
			if (err) {
				return err;
			}
			err = at_parser_num_get(parser, 3, &sock.role);
			if (err) {
				return err;
			}
			sock.family = (op == AT_SOCKET_OPEN) ? AF_INET : AF_INET6;
			if (param_count > 4) {
				err = at_parser_num_get(parser, 4, &sock.cid);
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

	case AT_PARSER_CMD_TYPE_READ:
		if (sock.fd != INVALID_SOCKET) {
			rsp_send("\r\n#XSOCKET: %d,%d,%d,%d,%d\r\n", sock.fd,
				sock.family, sock.role, sock.type, sock.cid);
		}
		err = 0;
		break;

	case AT_PARSER_CMD_TYPE_TEST:
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

SLM_AT_CMD_CUSTOM(xssocket_set, "AT#XSSOCKET=", handle_at_secure_socket);
SLM_AT_CMD_CUSTOM(xssocket_read, "AT#XSSOCKET?", handle_at_secure_socket);
static int handle_at_secure_socket(enum at_parser_cmd_type cmd_type,
				   struct at_parser *parser, uint32_t param_count)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &op);
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
			err = at_parser_num_get(parser, 2, &sock.type);
			if (err) {
				return err;
			}
			err = at_parser_num_get(parser, 3, &sock.role);
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
			sock.sec_tag = INVALID_SEC_TAG;
			err = at_parser_num_get(parser, 4, &sock.sec_tag);
			if (err) {
				return err;
			}
			if (param_count > 5) {
				err = at_parser_num_get(parser, 5, &peer_verify);
				if (err) {
					return err;
				}
			}
			sock.family = (op == AT_SOCKET_OPEN) ? AF_INET : AF_INET6;
			if (param_count > 6) {
				err = at_parser_num_get(parser, 6, &sock.cid);
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

	case AT_PARSER_CMD_TYPE_READ:
		if (sock.fd != INVALID_SOCKET) {
			rsp_send("\r\n#XSSOCKET: %d,%d,%d,%d,%d,%d\r\n", sock.fd,
				sock.family, sock.role, sock.type, sock.sec_tag, sock.cid);
		}
		err = 0;
		break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XSSOCKET: (%d,%d,%d),(%d,%d),(%d,%d),"
			 "<sec_tag>,<peer_verify>,<cid>\r\n",
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

SLM_AT_CMD_CUSTOM(xsocketselect, "AT#XSOCKETSELECT", handle_at_socket_select);
static int handle_at_socket_select(enum at_parser_cmd_type cmd_type,
				   struct at_parser *parser, uint32_t)
{
	int err = 0;
	int fd;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &fd);
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

	case AT_PARSER_CMD_TYPE_READ:
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

SLM_AT_CMD_CUSTOM(xsocketopt, "AT#XSOCKETOPT", handle_at_socketopt);
static int handle_at_socketopt(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			       uint32_t param_count)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t name;
	int value = 0;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &op);
		if (err) {
			return err;
		}
		err = at_parser_num_get(parser, 2, &name);
		if (err) {
			return err;
		}
		if (op == AT_SOCKETOPT_SET) {
			/* some options don't require a value */
			if (param_count > 3) {
				err = at_parser_num_get(parser, 3, &value);
				if (err) {
					return err;
				}
			}

			err = sockopt_set(name, value);
		} else if (op == AT_SOCKETOPT_GET) {
			err = sockopt_get(name);
		} break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XSOCKETOPT: (%d,%d),<name>,<value>\r\n",
			AT_SOCKETOPT_GET, AT_SOCKETOPT_SET);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xssocketopt, "AT#XSSOCKETOPT", handle_at_secure_socketopt);
static int handle_at_secure_socketopt(enum at_parser_cmd_type cmd_type,
				      struct at_parser *parser, uint32_t)
{
	int err = -EINVAL;
	uint16_t op;
	uint16_t name;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (sock.sec_tag == INVALID_SEC_TAG) {
			LOG_ERR("Not secure socket");
			return err;
		}
		err = at_parser_num_get(parser, 1, &op);
		if (err) {
			return err;
		}
		err = at_parser_num_get(parser, 2, &name);
		if (err) {
			return err;
		}
		if (op == AT_SOCKETOPT_SET) {
			int value_int = 0;
			char value_str[SLM_MAX_URL] = {0};
			int size = SLM_MAX_URL;

			err = at_parser_num_get(parser, 3, &value_int);
			if (err == -EOPNOTSUPP) {
				err = util_string_get(parser, 3, value_str, &size);
				if (err) {
					return err;
				}
				err = sec_sockopt_set(name, value_str, strlen(value_str));
			} else if (err == 0) {
				err = sec_sockopt_set(name, &value_int, sizeof(value_int));
			} else {
				return -EINVAL;
			}
		}  else if (op == AT_SOCKETOPT_GET) {
			err = sec_sockopt_get(name);
		} break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XSSOCKETOPT: (%d,%d),<name>,<value>\r\n",
			AT_SOCKETOPT_GET, AT_SOCKETOPT_SET);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xbind, "AT#XBIND", handle_at_bind);
static int handle_at_bind(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			  uint32_t)
{
	int err = -EINVAL;
	uint16_t port;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &port);
		if (err < 0) {
			return err;
		}
		err = slm_bind_to_local_addr(sock.fd, sock.family, port);
		break;

	default:
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xconnect, "AT#XCONNECT", handle_at_connect);
static int handle_at_connect(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			     uint32_t)
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
	case AT_PARSER_CMD_TYPE_SET:
		err = util_string_get(parser, 1, url, &size);
		if (err) {
			return err;
		}
		err = at_parser_num_get(parser, 2, &port);
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

SLM_AT_CMD_CUSTOM(xlisten, "AT#XLISTEN", handle_at_listen);
static int handle_at_listen(enum at_parser_cmd_type cmd_type, struct at_parser *, uint32_t)
{
	int err = -EINVAL;

	if (sock.role != AT_SOCKET_ROLE_SERVER) {
		LOG_ERR("Invalid role");
		return err;
	}

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = do_listen();
		break;

	default:
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xaccept, "AT#XACCEPT", handle_at_accept);
static int handle_at_accept(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			    uint32_t)
{
	int err = -EINVAL;
	int timeout;

	if (sock.role != AT_SOCKET_ROLE_SERVER) {
		LOG_ERR("Invalid role");
		return err;
	}

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &timeout);
		if (err) {
			return err;
		}
		err = do_accept(timeout);
		break;

	case AT_PARSER_CMD_TYPE_READ:
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

SLM_AT_CMD_CUSTOM(xsend, "AT#XSEND", handle_at_send);
static int handle_at_send(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			  uint32_t param_count)
{
	char at_cmd[32] = {0};
	size_t at_cmd_size = sizeof(at_cmd);

	if (util_string_get(parser, 0, at_cmd, &at_cmd_size)) {
		return -EINVAL;
	}
	if (!strncasecmp(at_cmd, "AT#XSENDTO", strlen("AT#XSENDTO"))) {
		return handle_at_sendto(cmd_type, parser, param_count);
	}

	int err = -EINVAL;
	char data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};
	int size;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (param_count > 1) {
			size = sizeof(data);
			err = util_string_get(parser, 1, data, &size);
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

SLM_AT_CMD_CUSTOM(xrecv_set, "AT#XRECV=", handle_at_recv);
SLM_AT_CMD_CUSTOM(xrecv_read, "AT#XRECV?", handle_at_recv);
static int handle_at_recv(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			  uint32_t param_count)
{
	int err = -EINVAL;
	int timeout;
	int flags = 0;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &timeout);
		if (err) {
			return err;
		}
		if (param_count > 2) {
			err = at_parser_num_get(parser, 2, &flags);
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

SLM_AT_CMD_CUSTOM(xsendto, "AT#XSENDTO", handle_at_sendto);
static int handle_at_sendto(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			    uint32_t param_count)
{

	int err = -EINVAL;
	int size;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		size = sizeof(udp_url);
		err = util_string_get(parser, 1, udp_url, &size);
		if (err) {
			return err;
		}
		err = at_parser_num_get(parser, 2, &udp_port);
		if (err) {
			return err;
		}
		if (param_count > 3) {
			char data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};

			size = sizeof(data);
			err = util_string_get(parser, 3, data, &size);
			if (err) {
				return err;
			}
			err = do_sendto(udp_url, udp_port, data, size);
			memset(udp_url, 0, sizeof(udp_url));
		} else {
			err = enter_datamode(socket_datamode_callback);
		}
		break;

	default:
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xrecvfrom, "AT#XRECVFROM", handle_at_recvfrom);
static int handle_at_recvfrom(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			      uint32_t param_count)
{
	int err = -EINVAL;
	int timeout;
	int flags = 0;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &timeout);
		if (err) {
			return err;
		}
		if (param_count > 2) {
			err = at_parser_num_get(parser, 2, &flags);
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

SLM_AT_CMD_CUSTOM(xgetaddrinfo, "AT#XGETADDRINFO", handle_at_getaddrinfo);
static int handle_at_getaddrinfo(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
				 uint32_t param_count)
{
	int err = -EINVAL;
	char hostname[NI_MAXHOST];
	char host[SLM_MAX_URL];
	int size = SLM_MAX_URL;
	struct addrinfo *result;
	struct addrinfo *res;
	char rsp_buf[256];

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = util_string_get(parser, 1, host, &size);
		if (err) {
			return err;
		}
		if (param_count == 3) {
			/* DNS query with designated address family */
			struct addrinfo hints = {
				.ai_family = AF_UNSPEC
			};
			err = at_parser_num_get(parser, 2, &hints.ai_family);
			if (err) {
				return err;
			}
			if (hints.ai_family < 0  || hints.ai_family > AF_INET6) {
				return -EINVAL;
			}
			err = getaddrinfo(host, NULL, &hints, &result);
		} else if (param_count == 2) {
			err = getaddrinfo(host, NULL, NULL, &result);
		} else {
			return -EINVAL;
		}
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

SLM_AT_CMD_CUSTOM(xpoll, "AT#XPOLL", handle_at_poll);
static int handle_at_poll(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			  uint32_t param_count)
{
	int err = -EINVAL;
	int timeout, handle;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = at_parser_num_get(parser, 1, &timeout);
		if (err) {
			return err;
		}
		if (param_count == 2) {
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
				if (param_count > 2 + i) {
					err = at_parser_num_get(parser, 2 + i, &handle);
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
