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
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/posix/sys/eventfd.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_socket.h"
#include "slm_sockopt.h"
#if defined(CONFIG_SLM_NATIVE_TLS)
#include "slm_native_tls.h"
#endif

LOG_MODULE_REGISTER(slm_sock, CONFIG_SLM_LOG_LEVEL);

#define SLM_FDS_COUNT CONFIG_POSIX_OPEN_MAX
#define SLM_MAX_SOCKET_COUNT (SLM_FDS_COUNT - 1)

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

struct slm_async_poll {
	atomic_t update_events; /* Update events to poll for this socket. */
	bool specific: 1;	/* Specific socket to poll. */
	bool disable: 1;	/* Poll needs to stay disabled for this socket. */
};

static struct slm_socket {
	int type;          /* SOCK_STREAM or SOCK_DGRAM */
	uint16_t role;     /* Client or Server */
	sec_tag_t sec_tag; /* Security tag of the credential */
	int family;        /* Socket address family */
	int fd;            /* Socket descriptor. */
	int fd_peer;       /* Socket descriptor for peer. */
	int ranking;       /* Ranking of socket */
	uint16_t cid;      /* PDP Context ID, 0: primary; 1~10: secondary */
	int send_flags;    /* Send flags */
	struct slm_async_poll async_poll; /* Async poll info */
} socks[SLM_MAX_SOCKET_COUNT];

static struct zsock_pollfd fds[SLM_MAX_SOCKET_COUNT];
static struct slm_socket *sock = &socks[0]; /* Current socket in use */

enum EFD_COMMAND {
	EFD_POLL = 0x1,
	EFD_CLOSE = 0x8000
};

struct async_poll_ctx {
	struct k_work update_work; /* Work to update async poll */
	int efd;		   /* Event file descriptor for async poll */
	int poll_events;	   /* Events to poll for async poll. */
	bool poll_all: 1;	   /* Poll all the sockets. */
	bool poll_running: 1;	   /* Async poll is running. */
};
static struct async_poll_ctx poll_ctx = {
	.efd = -1,
};

static struct k_thread apoll_thread_id;
static K_THREAD_STACK_DEFINE(apoll_thread_stack, KB(2));

static struct nw_wait_ctx {
	struct slm_socket *socket;
	uint8_t sock_buf[SLM_MAX_MESSAGE_SIZE];
	size_t len;
	int32_t flags;
} nw_wait_ctx;

static struct k_thread nw_wait_thread_id;
static K_THREAD_STACK_DEFINE(nw_wait_thread_stack, KB(2));

/* forward declarations */
#define SOCKET_SEND_TMO_SEC 30
static int socket_poll(int sock_fd, int event, int timeout);
static int handle_at_sendto(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			    uint32_t param_count);

static int socket_ranking;

static void init_socket(struct slm_socket *socket)
{
	if (socket == NULL) {
		return;
	}

	socket->family = AF_UNSPEC;
	socket->sec_tag = SEC_TAG_TLS_INVALID;
	socket->role = AT_SOCKET_ROLE_CLIENT;
	socket->fd = INVALID_SOCKET;
	socket->fd_peer = INVALID_SOCKET;
	socket->ranking = 0;
	socket->cid = 0;
	socket->async_poll = (struct slm_async_poll){0};
}

static bool is_opened_socket(int fd)
{
	if (fd != INVALID_SOCKET) {
		for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
			if (socks[i].fd == fd) {
				return true;
			}
		}
	}

	return false;
}

static struct slm_socket *find_avail_socket(void)
{
	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		if (socks[i].fd == INVALID_SOCKET) {
			return &socks[i];
		}
	}

	return NULL;
}

static struct slm_socket *get_active_socket(void)
{
	int ranking = 0;
	struct slm_socket *latest_sock = NULL;

	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		if (socks[i].fd == INVALID_SOCKET) {
			continue;
		}
		if (socks[i].ranking > ranking) {
			ranking = socks[i].ranking;
			latest_sock = &socks[i];
		}
	}

	if (latest_sock) {
		LOG_INF("Swap to socket %d", latest_sock->fd);
	} else {
		latest_sock = &socks[0];
	}

	return latest_sock;
}

static int bind_to_pdn(uint16_t cid)
{
	int ret = 0;

	if (cid > 0) {
		int cid_int = cid;

		ret = zsock_setsockopt(sock->fd, SOL_SOCKET, SO_BINDTOPDN, &cid_int, sizeof(int));
		if (ret < 0) {
			LOG_ERR("SO_BINDTOPDN error: %d", -errno);
		}
	}

	return ret;
}

static void update_work_fn(struct k_work *work)
{
	/* Update async poll events after AT-operation has completed. */
	if (poll_ctx.poll_running) {
		LOG_DBG("Update poll events");
		if (eventfd_write(poll_ctx.efd, EFD_POLL)) {
			LOG_ERR("eventfd_write() failed: %d", -errno);
		}
	}
}

static void delegate_poll_event(struct slm_socket *s, int events)
{
	if (poll_ctx.poll_running && (poll_ctx.poll_all || s->async_poll.specific)) {
		LOG_DBG("Delegate poll events %d for socket %d", events, s->fd);
		atomic_or(&s->async_poll.update_events, (poll_ctx.poll_events & events));

		k_work_submit(&poll_ctx.update_work);
	}

}

void slm_at_socket_notify_datamode_exit(void)
{
	if (poll_ctx.poll_running) {
		for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
			if (socks[i].fd == INVALID_SOCKET) {
				continue;
			}
			/* When we exit data mode, we will refresh
			 * POLLIN, POLLERR, POLLHUP, and POLLNVAL for all monitored sockets.
			 */
			socks[i].async_poll.disable = false;
			delegate_poll_event(&socks[i], ZSOCK_POLLIN);
		}
	}
}

static int do_socket_open(void)
{
	int ret = 0;
	int proto = IPPROTO_TCP;

	if (sock->type == SOCK_STREAM) {
		ret = zsock_socket(sock->family, SOCK_STREAM, IPPROTO_TCP);
	} else if (sock->type == SOCK_DGRAM) {
		ret = zsock_socket(sock->family, SOCK_DGRAM, IPPROTO_UDP);
		proto = IPPROTO_UDP;
	} else if (sock->type == SOCK_RAW) {
		sock->family = AF_PACKET;
		sock->role = AT_SOCKET_ROLE_CLIENT;
		ret = zsock_socket(sock->family, SOCK_RAW, IPPROTO_IP);
		proto = IPPROTO_IP;
	} else {
		LOG_ERR("socket type %d not supported", sock->type);
		return -ENOTSUP;
	}
	if (ret < 0) {
		LOG_ERR("zsock_socket() error: %d", -errno);
		return -errno;
	}

	sock->fd = ret;
	struct timeval tmo = {.tv_sec = SOCKET_SEND_TMO_SEC};

	ret = zsock_setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, &tmo, sizeof(tmo));
	if (ret) {
		LOG_ERR("zsock_setsockopt(%d) error: %d", SO_SNDTIMEO, -errno);
		ret = -errno;
		goto error;
	}

	/* Explicitly bind to secondary PDP context if required */
	ret = bind_to_pdn(sock->cid);
	if (ret) {
		goto error;
	}

	sock->ranking = socket_ranking++;
	rsp_send("\r\n#XSOCKET: %d,%d,%d\r\n", sock->fd, sock->type, proto);

	delegate_poll_event(sock, ZSOCK_POLLIN | ZSOCK_POLLOUT);

	return 0;

error:
	zsock_close(sock->fd);
	sock->fd = INVALID_SOCKET;
	return ret;
}

static int do_secure_socket_open(int peer_verify)
{
	int ret = 0;
	int proto = sock->type == SOCK_STREAM ? IPPROTO_TLS_1_2 : IPPROTO_DTLS_1_2;

	if (sock->type != SOCK_STREAM && sock->type != SOCK_DGRAM) {
		LOG_ERR("socket type %d not supported", sock->type);
		return -ENOTSUP;
	}

	ret = zsock_socket(sock->family, sock->type, proto);
	if (ret < 0) {
		LOG_ERR("zsock_socket() error: %d", -errno);
		return -errno;
	}
	sock->fd = ret;

#if defined(CONFIG_SLM_NATIVE_TLS)
	ret = slm_native_tls_load_credentials(sock->sec_tag);
	if (ret < 0) {
		LOG_ERR("Failed to load sec tag: %d (%d)", sock->sec_tag, ret);
		goto error;
	}
	int tls_native = 1;

	/* Must be the first socket option to set. */
	ret = zsock_setsockopt(sock->fd, SOL_TLS, TLS_NATIVE, &tls_native, sizeof(tls_native));
	if (ret) {
		goto error;
	}
#endif
	struct timeval tmo = {.tv_sec = SOCKET_SEND_TMO_SEC};

	ret = zsock_setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, &tmo, sizeof(tmo));
	if (ret) {
		LOG_ERR("zsock_setsockopt(%d) error: %d", SO_SNDTIMEO, -errno);
		ret = -errno;
		goto error;
	}

	/* Explicitly bind to secondary PDP context if required */
	ret = bind_to_pdn(sock->cid);
	if (ret) {
		goto error;
	}
	sec_tag_t sec_tag_list[1] = { sock->sec_tag };

	ret = zsock_setsockopt(sock->fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			       sizeof(sec_tag_t));
	if (ret) {
		LOG_ERR("zsock_setsockopt(TLS_SEC_TAG_LIST) error: %d", -errno);
		ret = -errno;
		goto error;
	}

	/* Set up (D)TLS peer verification */
	ret = zsock_setsockopt(sock->fd, SOL_TLS, TLS_PEER_VERIFY, &peer_verify,
			       sizeof(peer_verify));
	if (ret) {
		LOG_ERR("zsock_setsockopt(TLS_PEER_VERIFY) error: %d", errno);
		ret = -errno;
		goto error;
	}
	/* Set up (D)TLS server role if applicable */
	if (sock->role == AT_SOCKET_ROLE_SERVER) {
		int tls_role = TLS_DTLS_ROLE_SERVER;

		ret = zsock_setsockopt(sock->fd, SOL_TLS, TLS_DTLS_ROLE, &tls_role, sizeof(int));
		if (ret) {
			LOG_ERR("zsock_setsockopt(TLS_DTLS_ROLE) error: %d", -errno);
			ret = -errno;
			goto error;
		}
	}

	sock->ranking = socket_ranking++;
	rsp_send("\r\n#XSSOCKET: %d,%d,%d\r\n", sock->fd, sock->type, proto);

	delegate_poll_event(sock, ZSOCK_POLLIN | ZSOCK_POLLOUT);

	return 0;

error:
	zsock_close(sock->fd);
	sock->fd = INVALID_SOCKET;
	return ret;
}

static int do_socket_close(void)
{
	int ret;

	if (sock->fd == INVALID_SOCKET) {
		return 0;
	}

	if (sock->fd_peer != INVALID_SOCKET) {
		ret = zsock_close(sock->fd_peer);
		if (ret) {
			LOG_WRN("peer zsock_close() error: %d", -errno);
		}
		sock->fd_peer = INVALID_SOCKET;
	}
	ret = zsock_close(sock->fd);
	if (ret) {
		LOG_WRN("zsock_close() error: %d", -errno);
		ret = -errno;
	}

	rsp_send("\r\n#XSOCKET: %d,\"closed\"\r\n", ret);

	init_socket(sock);
	sock = get_active_socket();

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
	case AT_SO_IPV6_DELAYED_ADDR_REFRESH:
		*level = IPPROTO_IPV6;
		*option = SO_IPV6_DELAYED_ADDR_REFRESH;
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

	ret = zsock_setsockopt(sock->fd, level, option, value, len);
	if (ret) {
		LOG_ERR("zsock_setsockopt(%d,%d) error: %d", level, option, -errno);
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
		ret = zsock_getsockopt(sock->fd, level, option, &tmo, &len);
		if (ret == 0) {
			rsp_send("\r\n#XSOCKETOPT: %ld\r\n", (long)tmo.tv_sec);
		}
	} else {
		/* Default */
		ret = zsock_getsockopt(sock->fd, level, option, &value, &len);
		if (ret == 0) {
			rsp_send("\r\n#XSOCKETOPT: %d\r\n", value);
		}
	}

	if (ret) {
		LOG_ERR("zsock_getsockopt(%d,%d) error: %d", level, option, -errno);
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

	ret = zsock_setsockopt(sock->fd, level, option, value, len);
	if (ret) {
		LOG_ERR("zsock_setsockopt(%d,%d) error: %d", level, option, -errno);
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
		ret = zsock_getsockopt(sock->fd, level, option, &value, &len);
		if (ret == 0) {
			rsp_send("\r\n#XSSOCKETOPT: 0x%x\r\n", value);
		}
	} else if (level == SOL_TLS && option == TLS_HOSTNAME) {
		char hostname[SLM_MAX_URL] = {0};

		len = sizeof(hostname);
		ret = zsock_getsockopt(sock->fd, level, option, &hostname, &len);
		if (ret == 0) {
			rsp_send("\r\n#XSSOCKETOPT: %s\r\n", hostname);
		}
	} else {
		/* Default */
		ret = zsock_getsockopt(sock->fd, level, option, &value, &len);
		if (ret == 0) {
			rsp_send("\r\n#XSSOCKETOPT: %d\r\n", value);
		}
	}

	if (ret) {
		LOG_ERR("zsock_getsockopt(%d,%d) error: %d", level, option, -errno);
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

		if (zsock_inet_pton(AF_INET, ipv4_addr, &local.sin_addr) != 1) {
			LOG_ERR("Parse local IPv4 address failed: %d", -errno);
			return -EINVAL;
		}

		ret = zsock_bind(socket, (struct sockaddr *)&local, sizeof(struct sockaddr_in));
		if (ret) {
			LOG_ERR("zsock_bind() sock %d failed: %d", socket, -errno);
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

		if (zsock_inet_pton(AF_INET6, ipv6_addr, &local.sin6_addr) != 1) {
			LOG_ERR("Parse local IPv6 address failed: %d", -errno);
			return -EINVAL;
		}
		ret = zsock_bind(socket, (struct sockaddr *)&local, sizeof(struct sockaddr_in6));
		if (ret) {
			LOG_ERR("zsock_bind() sock %d failed: %d", socket, -errno);
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

	if (sock == nw_wait_ctx.socket)	{
		LOG_ERR("nw_wait: Socket busy.");
		return -EBUSY;
	}

	LOG_DBG("connect %s:%d", url, port);
	ret = util_resolve_host(sock->cid, url, port, sock->family, &sa);
	if (ret) {
		return -EAGAIN;
	}
	if (sa.sa_family == AF_INET) {
		ret = zsock_connect(sock->fd, &sa, sizeof(struct sockaddr_in));
	} else {
		ret = zsock_connect(sock->fd, &sa, sizeof(struct sockaddr_in6));
	}
	if (ret) {
		LOG_ERR("zsock_connect() error: %d", -errno);
		return -errno;
	}

	rsp_send("\r\n#XCONNECT: 1\r\n");

	return ret;
}

static int do_listen(void)
{
	int ret;

	/* hardcode backlog to be 1 for now */
	ret = zsock_listen(sock->fd, 1);
	if (ret < 0) {
		LOG_ERR("zsock_listen() error: %d", -errno);
		return -errno;
	}

	return 0;
}

static int do_accept(int timeout)
{
	int ret;
	char peer_addr[INET6_ADDRSTRLEN] = {0};

	ret = socket_poll(sock->fd, ZSOCK_POLLIN, timeout);
	if (ret) {
		return ret;
	}

	if (sock->family == AF_INET) {
		struct sockaddr_in client;
		socklen_t len = sizeof(struct sockaddr_in);

		ret = zsock_accept(sock->fd, (struct sockaddr *)&client, &len);
		if (ret == -1) {
			LOG_ERR("zsock_accept() error: %d", -errno);
			sock->fd_peer = INVALID_SOCKET;
			return -errno;
		}
		sock->fd_peer = ret;
		(void)zsock_inet_ntop(AF_INET, &client.sin_addr, peer_addr, sizeof(peer_addr));
	} else if (sock->family == AF_INET6) {
		struct sockaddr_in6 client;
		socklen_t len = sizeof(struct sockaddr_in6);

		ret = zsock_accept(sock->fd, (struct sockaddr *)&client, &len);
		if (ret == -1) {
			LOG_ERR("zsock_accept() error: %d", -errno);
			sock->fd_peer = INVALID_SOCKET;
			return -errno;
		}
		sock->fd_peer = ret;
		(void)zsock_inet_ntop(AF_INET6, &client.sin6_addr, peer_addr, sizeof(peer_addr));
	} else {
		return -EINVAL;
	}
	rsp_send("\r\n#XACCEPT: %d,\"%s\"\r\n", sock->fd_peer, peer_addr);

	return 0;
}

static void nw_wait_thread(void*, void*, void*)
{
	size_t sent = 0;
	int err = 0;

	LOG_DBG("nw_wait_thread started");

	if (nw_wait_ctx.socket == NULL) {
		LOG_ERR("nw_wait: No socket");
		return;
	}

	while (sent < nw_wait_ctx.len) {
		err = zsock_send(nw_wait_ctx.socket->fd, nw_wait_ctx.sock_buf + sent,
				 nw_wait_ctx.len - sent, nw_wait_ctx.flags);
		if (err < 0) {
			LOG_ERR("nw_wait: Sent %u out of %u bytes. (%d)", sent, nw_wait_ctx.len,
				-errno);
			delegate_poll_event(nw_wait_ctx.socket, ZSOCK_POLLERR);

			/* TODO:
			 * There is an argument on sending the #XSEND: 0 from the do_send() and
			 * sending the #XSEND: sent bytes with possible failure from here.
			 * This will work better when the handles are included in the response.
			 */
			goto exit;
		}
		sent += err;
	}
	LOG_DBG("nw_wait: Sent %u out of %u bytes. (%d)", sent, nw_wait_ctx.len, 0);

exit:
	delegate_poll_event(nw_wait_ctx.socket, ZSOCK_POLLOUT);

	LOG_DBG("nw_wait_thread stopped");
	nw_wait_ctx.socket = NULL;
}


static int do_send(const uint8_t *data, int len, int flags)
{
	int ret = 0;
	int sockfd = sock->fd;

	LOG_DBG("send flags=%d", flags);

	if (sock == nw_wait_ctx.socket) {
		LOG_ERR("nw_wait: Socket busy.");
		return -EBUSY;
	}

	/* For TCP/TLS Server, send to incoming socket */
	if (sock->type == SOCK_STREAM && sock->role == AT_SOCKET_ROLE_SERVER) {
		if (sock->fd_peer != INVALID_SOCKET) {
			sockfd = sock->fd_peer;
		} else {
			LOG_ERR("No connection");
			return -EINVAL;
		}
	}

	uint32_t sent = 0;

	if (flags & NRF_MSG_WAITACK) {
		if (nw_wait_ctx.socket != NULL) {
			LOG_ERR("Another socket is waiting for NW ACK");
			return -EBUSY;
		}
		if (len > sizeof(nw_wait_ctx.sock_buf)) {
			LOG_ERR("Data length %d exceeds max %d", len, sizeof(nw_wait_ctx.sock_buf));
			return -EMSGSIZE;
		}

		/* Send data with a thread. */
		memcpy(nw_wait_ctx.sock_buf, data, len);
		nw_wait_ctx.len = len;
		nw_wait_ctx.flags = flags;
		nw_wait_ctx.socket = sock;
		k_thread_create(&nw_wait_thread_id, nw_wait_thread_stack,
			K_THREAD_STACK_SIZEOF(nw_wait_thread_stack),
			nw_wait_thread, NULL, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

		if (!in_datamode()) {
			rsp_send("\r\n#XSEND: %d\r\n", len);
		}
		return len;
	}

	while (sent < len) {
		ret = zsock_send(sockfd, data + sent, len - sent, flags);
		if (ret < 0) {
			LOG_ERR("Sent %u out of %u bytes. (%d)", sent, len, -errno);
			ret = -errno;
			break;
		}
		sent += ret;
	}

	if (!in_datamode()) {
		rsp_send("\r\n#XSEND: %d\r\n", sent);
		delegate_poll_event(sock, ZSOCK_POLLOUT);
	}

	return sent > 0 ? sent : ret;
}

static int do_recv(int timeout, int flags)
{
	int ret;
	int sockfd = sock->fd;

	/* For TCP/TLS Server, receive from incoming socket */
	if (sock->type == SOCK_STREAM && sock->role == AT_SOCKET_ROLE_SERVER) {
		if (sock->fd_peer != INVALID_SOCKET) {
			sockfd = sock->fd_peer;
		} else {
			LOG_ERR("No remote connection");
			return -EINVAL;
		}
	}
	struct timeval tmo = {.tv_sec = timeout};

	ret = zsock_setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, &tmo, sizeof(tmo));
	if (ret) {
		LOG_ERR("zsock_setsockopt(%d) error: %d", SO_RCVTIMEO, -errno);
		return -errno;
	}
	ret = zsock_recv(sockfd, (void *)slm_data_buf, sizeof(slm_data_buf), flags);
	if (ret < 0) {
		LOG_WRN("zsock_recv() error: %d", -errno);
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
		LOG_WRN("zsock_recv() return 0");
	} else {
		rsp_send("\r\n#XRECV: %d\r\n", ret);
		data_send(slm_data_buf, ret);
		ret = 0;

		delegate_poll_event(sock, ZSOCK_POLLIN);
	}

	return ret;
}

static int do_sendto(const char *url, uint16_t port, const uint8_t *data, int len, int flags)
{
	int ret = 0;
	uint32_t sent = 0;
	struct sockaddr sa = {.sa_family = AF_UNSPEC};

	if (sock == nw_wait_ctx.socket) {
		LOG_ERR("nw_wait: Socket busy.");
		return -EBUSY;
	}

	if (flags & NRF_MSG_WAITACK) {
		LOG_ERR("nw_wait: Not supported with sendto.");
		return -EOPNOTSUPP;
	}

	LOG_DBG("sendto %s:%d, flags=%d", url, port, flags);
	ret = util_resolve_host(sock->cid, url, port, sock->family, &sa);
	if (ret) {
		return -EAGAIN;
	}

	do {
		ret = zsock_sendto(sock->fd, data + sent, len - sent, flags, &sa,
				   sa.sa_family == AF_INET ? sizeof(struct sockaddr_in)
							   : sizeof(struct sockaddr_in6));
		if (ret <= 0) {
			ret = -errno;
			break;
		}
		sent += ret;

	} while (sock->type != SOCK_DGRAM && sent < len);

	if (ret >= 0 && sock->type == SOCK_DGRAM && sent != len) {
		/* Partial send of datagram. */
		ret = -EAGAIN;
		sent = 0;
	}

	if (ret < 0) {
		LOG_ERR("Sent %u out of %u bytes. (%d)", sent, len, ret);
	}

	if (!in_datamode()) {
		rsp_send("\r\n#XSENDTO: %d\r\n", sent);
		delegate_poll_event(sock, ZSOCK_POLLOUT);
	}


	return sent > 0 ? sent : ret;
}

static int do_recvfrom(int timeout, int flags)
{
	int ret;
	struct sockaddr remote;
	socklen_t addrlen = sizeof(struct sockaddr);
	struct timeval tmo = {.tv_sec = timeout};

	ret = zsock_setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, &tmo, sizeof(tmo));
	if (ret) {
		LOG_ERR("zsock_setsockopt(%d) error: %d", SO_RCVTIMEO, -errno);
		return -errno;
	}
	ret = zsock_recvfrom(
		sock->fd, (void *)slm_data_buf, sizeof(slm_data_buf), flags, &remote, &addrlen);
	if (ret < 0) {
		LOG_ERR("zsock_recvfrom() error: %d", -errno);
		return -errno;
	}
	/**
	 * Datagram sockets in various domains permit zero-length
	 * datagrams. When such a datagram is received, the return
	 * value is 0. Treat as normal case
	 */
	if (ret == 0) {
		LOG_WRN("zsock_recvfrom() return 0");
	} else {
		char peer_addr[INET6_ADDRSTRLEN] = {0};
		uint16_t peer_port = 0;

		util_get_peer_addr(&remote, peer_addr, &peer_port);
		rsp_send("\r\n#XRECVFROM: %d,\"%s\",%d\r\n", ret, peer_addr, peer_port);
		data_send(slm_data_buf, ret);

		delegate_poll_event(sock, ZSOCK_POLLIN);
	}

	return 0;
}

static int do_poll(int timeout)
{
	int ret = zsock_poll(fds, SLM_MAX_SOCKET_COUNT, timeout);

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
	struct zsock_pollfd fd = {
		.fd = sock_fd,
		.events = event
	};

	if (timeout <= 0) {
		return 0;
	}

	ret = zsock_poll(&fd, 1, MSEC_PER_SEC * timeout);
	if (ret < 0) {
		LOG_WRN("zsock_poll() error: %d", -errno);
		return -errno;
	} else if (ret == 0) {
		LOG_WRN("zsock_poll() timeout");
		return -EAGAIN;
	}

	LOG_DBG("zsock_poll() events 0x%08x", fd.revents);
	if ((fd.revents & event) != event) {
		return -EAGAIN;
	}

	return 0;
}

static int socket_datamode_callback(uint8_t op, const uint8_t *data, int len, uint8_t flags)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		if (sock->type == SOCK_DGRAM && (flags & SLM_DATAMODE_FLAGS_MORE_DATA) != 0) {
			LOG_ERR("Datamode buffer overflow");
			exit_datamode_handler(-EOVERFLOW);
			return -EOVERFLOW;
		} else {
			if (strlen(udp_url) > 0) {
				ret = do_sendto(udp_url, udp_port, data, len, sock->send_flags);
			} else {
				ret = do_send(data, len, sock->send_flags);
			}
			LOG_INF("datamode send: %d", ret);
		}
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("datamode exit");
		memset(udp_url, 0, sizeof(udp_url));
		if (nw_wait_ctx.socket == NULL) {
			delegate_poll_event(sock, ZSOCK_POLLOUT);
		}
	}

	return ret;
}

SLM_AT_CMD_CUSTOM(xsocket, "AT#XSOCKET", handle_at_socket);
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
			sock = find_avail_socket();
			if (sock == NULL) {
				LOG_ERR("Max socket count reached");
				err = -EINVAL;
				goto error;
			}
			init_socket(sock);
			err = at_parser_num_get(parser, 2, &sock->type);
			if (err) {
				goto error;
			}
			err = at_parser_num_get(parser, 3, &sock->role);
			if (err) {
				goto error;
			}
			sock->family = (op == AT_SOCKET_OPEN) ? AF_INET : AF_INET6;
			if (param_count > 4) {
				err = at_parser_num_get(parser, 4, &sock->cid);
				if (err) {
					goto error;
				}
				if (sock->cid > 10) {
					err = -EINVAL;
					goto error;
				}
			}
			err = do_socket_open();
			if (err) {
				LOG_ERR("do_socket_open() failed: %d", err);
				goto error;
			}
		} else if (op == AT_SOCKET_CLOSE) {
			err = do_socket_close();
		} else {
			err = -EINVAL;
		} break;

	case AT_PARSER_CMD_TYPE_READ:
		if (sock->fd != INVALID_SOCKET) {
			rsp_send("\r\n#XSOCKET: %d,%d,%d,%d,%d\r\n", sock->fd,
				sock->family, sock->role, sock->type, sock->cid);
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

error:
	init_socket(sock);
	sock = get_active_socket();

	return err;
}

SLM_AT_CMD_CUSTOM(xssocket, "AT#XSSOCKET", handle_at_secure_socket);
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

			sock = find_avail_socket();
			if (sock == NULL) {
				LOG_ERR("Max socket count reached");
				err = -EINVAL;
				goto error;
			}
			init_socket(sock);
			err = at_parser_num_get(parser, 2, &sock->type);
			if (err) {
				goto error;
			}
			err = at_parser_num_get(parser, 3, &sock->role);
			if (err) {
				goto error;
			}
			if (sock->role == AT_SOCKET_ROLE_SERVER) {
				peer_verify = TLS_PEER_VERIFY_NONE;
			} else if (sock->role == AT_SOCKET_ROLE_CLIENT) {
				peer_verify = TLS_PEER_VERIFY_REQUIRED;
			} else {
				err = -EINVAL;
				goto error;
			}
			sock->sec_tag = SEC_TAG_TLS_INVALID;
			err = at_parser_num_get(parser, 4, &sock->sec_tag);
			if (err) {
				goto error;
			}
			if (param_count > 5) {
				err = at_parser_num_get(parser, 5, &peer_verify);
				if (err) {
					goto error;
				}
			}
			sock->family = (op == AT_SOCKET_OPEN) ? AF_INET : AF_INET6;
			if (param_count > 6) {
				err = at_parser_num_get(parser, 6, &sock->cid);
				if (err) {
					goto error;
				}
				if (sock->cid > 10) {
					err = -EINVAL;
					goto error;
				}
			}
			err = do_secure_socket_open(peer_verify);
			if (err) {
				LOG_ERR("do_secure_socket_open() failed: %d", err);
				goto error;
			}
		} else if (op == AT_SOCKET_CLOSE) {
			err = do_socket_close();
		} else {
			err = -EINVAL;
		} break;

	case AT_PARSER_CMD_TYPE_READ:
		if (sock->fd != INVALID_SOCKET) {
			rsp_send("\r\n#XSSOCKET: %d,%d,%d,%d,%d,%d\r\n", sock->fd,
				sock->family, sock->role, sock->type, sock->sec_tag, sock->cid);
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

error:
	init_socket(sock);
	sock = get_active_socket();

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
				sock = &socks[i];
				rsp_send("\r\n#XSOCKETSELECT: %d\r\n", sock->fd);
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
		if (sock->fd != INVALID_SOCKET) {
			rsp_send("\r\n#XSOCKETSELECT: %d\r\n", sock->fd);
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
		if (sock->sec_tag == SEC_TAG_TLS_INVALID) {
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
		err = slm_bind_to_local_addr(sock->fd, sock->family, port);
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

	if (sock->role != AT_SOCKET_ROLE_CLIENT) {
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

	if (sock->role != AT_SOCKET_ROLE_SERVER) {
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

	if (sock->role != AT_SOCKET_ROLE_SERVER) {
		LOG_ERR("Invalid role");
		return err;
	}

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (poll_ctx.poll_running) {
			LOG_ERR("%s cannot be used with AT#XAPOLL", "AT#XACCEPT");
			return -EBUSY;
		}
		err = at_parser_num_get(parser, 1, &timeout);
		if (err) {
			return err;
		}
		err = do_accept(timeout);
		break;

	case AT_PARSER_CMD_TYPE_READ:
		if (sock->fd_peer != INVALID_SOCKET) {
			rsp_send("\r\n#XTCPACCEPT: %d\r\n", sock->fd_peer);
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
	int err = -EINVAL;
	char data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};
	int size;
	bool datamode = false;

	sock->send_flags = 0;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (param_count > 1) {
			size = sizeof(data);
			err = util_string_get(parser, 1, data, &size);
			if (err == -ENODATA) {
				/* -ENODATA means data is empty so we go into datamode */
				datamode = true;
			} else if (err != 0) {
				return err;
			}
			if (param_count > 2) {
				err = at_parser_num_get(parser, 2, &sock->send_flags);
				if (err) {
					return err;
				}
			}
		} else {
			datamode = true;
		}
		if (datamode) {
			err = enter_datamode(socket_datamode_callback);
		} else {
			err = do_send(data, size, sock->send_flags);
			if (err == size) {
				err = 0;
			} else {
				err = err < 0 ? err : -EAGAIN;
			}
		}
		break;

	default:
		break;
	}

	return err;
}

SLM_AT_CMD_CUSTOM(xrecv, "AT#XRECV", handle_at_recv);
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
	char data[SLM_MAX_PAYLOAD_SIZE + 1] = {0};
	int size;
	bool datamode = false;

	sock->send_flags = 0;

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
			size = sizeof(data);
			err = util_string_get(parser, 3, data, &size);
			if (err == -ENODATA) {
				/* -ENODATA means data is empty so we go into datamode */
				datamode = true;
			} else if (err != 0) {
				return err;
			}
			if (param_count > 4) {
				err = at_parser_num_get(parser, 4, &sock->send_flags);
				if (err) {
					return err;
				}
			}
		} else {
			datamode = true;
		}
		if (datamode) {
			err = enter_datamode(socket_datamode_callback);
		} else {
			err = do_sendto(udp_url, udp_port, data, size, sock->send_flags);
			if (err == size) {
				err = 0;
			} else {
				err = err < 0 ? err : -EAGAIN;
			}
			memset(udp_url, 0, sizeof(udp_url));
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
	struct zsock_addrinfo *result;
	struct zsock_addrinfo *res;
	char rsp_buf[256];

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		err = util_string_get(parser, 1, host, &size);
		if (err) {
			return err;
		}
		if (param_count == 3) {
			/* DNS query with designated address family */
			struct zsock_addrinfo hints = {
				.ai_family = AF_UNSPEC
			};
			err = at_parser_num_get(parser, 2, &hints.ai_family);
			if (err) {
				return err;
			}
			if (hints.ai_family < 0  || hints.ai_family > AF_INET6) {
				return -EINVAL;
			}
			err = zsock_getaddrinfo(host, NULL, &hints, &result);
		} else if (param_count == 2) {
			err = zsock_getaddrinfo(host, NULL, NULL, &result);
		} else {
			return -EINVAL;
		}
		if (err) {
			rsp_send("\r\n#XGETADDRINFO: \"%s\"\r\n", zsock_gai_strerror(err));
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

				zsock_inet_ntop(AF_INET, &host->sin_addr, hostname,
						sizeof(hostname));
			} else if (res->ai_family == AF_INET6) {
				struct sockaddr_in6 *host = (struct sockaddr_in6 *)result->ai_addr;

				zsock_inet_ntop(AF_INET6, &host->sin6_addr, hostname,
						sizeof(hostname));
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
		zsock_freeaddrinfo(result);
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
		if (poll_ctx.poll_running) {
			LOG_ERR("%s cannot be used with AT#XAPOLL", "AT#XPOLL");
			return -EBUSY;
		}
		err = at_parser_num_get(parser, 1, &timeout);
		if (err) {
			return err;
		}
		if (param_count == 2) {
			/* poll all opened socket */
			for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
				fds[i].fd = socks[i].fd;
				if (fds[i].fd != INVALID_SOCKET) {
					fds[i].events = ZSOCK_POLLIN;
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
					fds[i].events = ZSOCK_POLLIN;
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

static void update_apoll_fds(struct zsock_pollfd *afds, size_t count)
{
	for (size_t i = 0; i < count && i < ARRAY_SIZE(socks); ++i) {
		/* Skip sockets that are not opened or are disabled for async poll. */
		if (socks[i].fd == INVALID_SOCKET || socks[i].async_poll.disable) {
			afds[i].fd = INVALID_SOCKET;
			afds[i].events = 0;
			continue;
		}
		/* Skip sockets that are not in async poll mode. */
		if (!poll_ctx.poll_all && !socks[i].async_poll.specific) {
			afds[i].fd = INVALID_SOCKET;
			continue;
		}
		afds[i].fd = socks[i].fd;

		/* atomic_clear returns the value before clear. */
		afds[i].events |= atomic_clear(&socks[i].async_poll.update_events);
		LOG_DBG("fd: %d, events: 0x%04x", afds[i].fd, afds[i].events);
	}
}

static eventfd_t get_efd_event(struct zsock_pollfd *efd)
{
	eventfd_t value = 0;

	if (efd->revents & ZSOCK_POLLIN) {
		efd->revents &= ~ZSOCK_POLLIN;
		eventfd_read(efd->fd, &value);
	}
	if (efd->revents) {
		LOG_ERR("efd revents: 0x%04x", efd->revents);
	}

	return value;
}

static inline void handle_apoll_socket_event(struct slm_socket *s, struct zsock_pollfd *fd)
{
	if (fd->revents) {
		if (fd->revents & (ZSOCK_POLLERR | ZSOCK_POLLNVAL | ZSOCK_POLLHUP)) {
			/* Remove socket from poll until closed. */
			s->async_poll.disable = true;
		} else {
			/* Remove POLLIN/POLLOUT from poll, until AT-operation. */
			fd->events &= ~fd->revents;
		}
		if (!in_datamode()) {
			rsp_send_indicate("\r\n#XAPOLL: %d,%d\r\n", fd->fd, fd->revents);
		}
	}
}

static void apoll_thread(void*, void*, void*)
{
	int ret;
	struct zsock_pollfd afds[SLM_FDS_COUNT] = {
		[0 ... SLM_FDS_COUNT - 1] = {
			.fd = INVALID_SOCKET
		}
	};

	LOG_DBG("Asynchronous polling thread started");

	/* Always poll the EFD socket. */
	afds[ARRAY_SIZE(afds) - 1].fd = poll_ctx.efd;
	afds[ARRAY_SIZE(afds) - 1].events = ZSOCK_POLLIN;

	while (poll_ctx.poll_running) {
		update_apoll_fds(afds, ARRAY_SIZE(afds) - 1);

		ret = zsock_poll(afds, ARRAY_SIZE(afds), -1);
		if (ret < 0) {
			LOG_ERR("zsock_poll() failed: %d, exit thread...", -errno);
			poll_ctx.poll_running = false;
			break;
		}
		for (int i = 0; i < ARRAY_SIZE(afds); ++i) {
			if (afds[i].fd == INVALID_SOCKET) {
				continue;
			}

			if (afds[i].fd == poll_ctx.efd) {
				if (get_efd_event(&afds[i]) & EFD_CLOSE) {
					/* Exit thread. */
					poll_ctx.poll_running = false;
				}
			} else {
				handle_apoll_socket_event(&socks[i], &afds[i]);
			}
		}
	}
	LOG_DBG("Asynchronous polling thread stopped");
}

static int apoll_start(void)
{
	if (poll_ctx.poll_running) {
		LOG_DBG("Restart asynchronous polling");
		return eventfd_write(poll_ctx.efd, EFD_POLL);
	}

	LOG_DBG("Start asynchronous polling");
	if (poll_ctx.efd != INVALID_SOCKET) {
		zsock_close(poll_ctx.efd);
	}
	poll_ctx.efd = eventfd(0, 0);
	if (poll_ctx.efd < 0) {
		LOG_ERR("eventfd() failed: %d", -errno);
		return -errno;
	}
	poll_ctx.poll_running = true;
	k_thread_create(&apoll_thread_id, apoll_thread_stack,
			K_THREAD_STACK_SIZEOF(apoll_thread_stack),
			apoll_thread, NULL, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&apoll_thread_id, "Asynchronous polling");

	return 0;
}

static int apoll_stop(void)
{
	int err = 0;

	if (poll_ctx.poll_running) {
		LOG_DBG("Stop asynchronous polling");
		err = eventfd_write(poll_ctx.efd, EFD_CLOSE);
		if (err < 0) {
			LOG_ERR("eventfd_write() failed: %d", -errno);
		}
		err = k_thread_join(&apoll_thread_id, K_SECONDS(1));
		if (err) {
			LOG_WRN("k_thread_join() failed: %d", err);
		}
	}

	if (poll_ctx.efd != INVALID_SOCKET) {
		zsock_close(poll_ctx.efd);
		poll_ctx.efd = INVALID_SOCKET;
	}
	poll_ctx.poll_events = 0;

	return 0;
}

static void format_apoll_read_response(char *response, size_t size)
{
	int offset = 0;

	offset = snprintf(response, size, "\r\n#XAPOLL: %d,%d", poll_ctx.poll_running,
			  poll_ctx.poll_events);

	for (int i = 0; i < SLM_MAX_SOCKET_COUNT && offset < size; i++) {
		if (socks[i].fd != INVALID_SOCKET &&
		    (poll_ctx.poll_all || socks[i].async_poll.specific)) {
			offset += snprintf(&response[offset], size - offset, ",%d", socks[i].fd);
		}
	}

	if (offset > size - sizeof("\r\n")) {
		offset = size - sizeof("\r\n");
	}
	snprintf(&response[offset], size - offset, "\r\n");
}

static void set_apoll_all_sockets(void)
{
	poll_ctx.poll_all = true;
	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		if (socks[i].fd != INVALID_SOCKET) {
			atomic_set(&socks[i].async_poll.update_events, poll_ctx.poll_events);
		}
	}
}

static int set_apoll_specific_sockets(struct at_parser *parser, uint32_t param_count)
{
	int handle;
	int err;
	bool socket_found;

	/* Clear previously set values. */
	poll_ctx.poll_all = false;
	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		socks[i].async_poll.specific = false;
	}

	/* Go through all the given handles. */
	for (int i = 3; i < param_count; i++) {
		err = at_parser_num_get(parser, i, &handle);
		if (err) {
			return err;
		}
		socket_found = false;

		/* Match given handles to socket handles. */
		for (int j = 0; j < SLM_MAX_SOCKET_COUNT; j++) {
			if (socks[j].fd == handle) {
				socks[j].async_poll.specific = true;
				atomic_set(&socks[j].async_poll.update_events,
					   poll_ctx.poll_events);
				socket_found = true;
				break;
			}
		}
		if (!socket_found) {
			LOG_ERR("Socket %d not found", handle);
			return -EINVAL;
		}
	}

	return 0;
}

SLM_AT_CMD_CUSTOM(xapoll, "AT#XAPOLL", handle_at_apoll);
static int handle_at_apoll(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			  uint32_t param_count)
{
	int err = -EINVAL;
	int op;

	enum async_poll_operation {
		AT_ASYNCPOLL_STOP = 0,
		AT_ASYNCPOLL_START = 1,
	};

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (at_parser_num_get(parser, 1, &op) ||
		    (op != AT_ASYNCPOLL_START && op != AT_ASYNCPOLL_STOP)) {
			return -EINVAL;
		}
		if (op == AT_ASYNCPOLL_STOP) {
			return apoll_stop();
		}

		/* APOLL START */
		err = at_parser_num_get(parser, 2, &poll_ctx.poll_events);
		if (err) {
			return err;
		}
		if (poll_ctx.poll_events & ~(ZSOCK_POLLIN | ZSOCK_POLLOUT)) {
			LOG_ERR("Invalid poll events: 0x%02x", poll_ctx.poll_events);
			return -EINVAL;
		}
		if (param_count == 3) {
			set_apoll_all_sockets();
		} else {
			err = set_apoll_specific_sockets(parser, param_count);
			if (err) {
				return err;
			}
		}
		err = apoll_start();
		break;

	case AT_PARSER_CMD_TYPE_READ:
		char response[64];

		format_apoll_read_response(response, sizeof(response));
		err = slm_at_send_str(response);
		break;

	case AT_PARSER_CMD_TYPE_TEST:
		rsp_send("\r\n#XAPOLL: (%d,%d),<0,%d,%d,%d>,<handle1>,<handle2>,...\r\n",
			 AT_ASYNCPOLL_STOP, AT_ASYNCPOLL_START, ZSOCK_POLLIN, ZSOCK_POLLOUT,
			 ZSOCK_POLLIN | ZSOCK_POLLOUT);
		err = 0;
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
	init_socket(sock);
	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		init_socket(&socks[i]);
	}
	socket_ranking = 1;

	k_work_init(&poll_ctx.update_work, update_work_fn);

	return 0;
}

/**@brief API to uninitialize Socket AT commands handler
 */
int slm_at_socket_uninit(void)
{
	apoll_stop();

	(void)do_socket_close();
	for (int i = 0; i < SLM_MAX_SOCKET_COUNT; i++) {
		if (socks[i].fd_peer != INVALID_SOCKET) {
			zsock_close(socks[i].fd_peer);
		}
		if (socks[i].fd != INVALID_SOCKET) {
			zsock_close(socks[i].fd);
		}
	}

	return 0;
}
