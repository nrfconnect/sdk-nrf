/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/net/socket_offload.h>
#include <sockets_internal.h>
#include <zephyr/ztest.h>

#include "mock/socket.h"

void mock_socket_iface_init(struct net_if *iface);

struct mock_socket_iface_data {
	struct net_if *iface;
} mock_socket_iface_data;

struct offloaded_if_api mock_if_api = {
	.iface_api.init = mock_socket_iface_init,
};

/* All the functions contains a delay of 50 msec to avoid endless loops in
 * application code and simulate a bit the socket api as it would block
 * for a short moment anyway
 */

static ssize_t mock_socket_offload_recvfrom(void *obj, void *buf, size_t len, int flags,
					    struct sockaddr *from, socklen_t *fromlen)
{
	k_sleep(K_MSEC(50));
	return ztest_get_return_value();
}

static ssize_t mock_socket_offload_read(void *obj, void *buffer, size_t count)
{
	return mock_socket_offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t mock_socket_offload_sendto(void *obj, const void *buf, size_t len, int flags,
					  const struct sockaddr *to, socklen_t tolen)
{
	k_sleep(K_MSEC(50));
	return ztest_get_return_value();
}

static ssize_t mock_socket_offload_write(void *obj, const void *buffer, size_t count)
{
	return mock_socket_offload_sendto(obj, buffer, count, 0, NULL, 0);
}

static int mock_socket_offload_close(void *obj)
{
	k_sleep(K_MSEC(50));
	return zsock_close_ctx(obj);
}

static int mock_socket_offload_ioctl(void *obj, unsigned int request, va_list args)
{
	k_sleep(K_MSEC(50));

	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		return -EXDEV;

	case ZFD_IOCTL_POLL_UPDATE:
		return -EOPNOTSUPP;

	case ZFD_IOCTL_POLL_OFFLOAD: {
		return 0;
	}

	case ZFD_IOCTL_SET_LOCK: {
		return 0;
	}

	/* Otherwise, just forward to offloaded fcntl()
	 * In Zephyr, fcntl() is just an alias of ioctl().
	 */
	default:
		return 0;
	}

	return 0;
}

static int mock_socket_offload_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	k_sleep(K_MSEC(50));
	return 0;
}

static int mock_socket_offload_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	ztest_check_expected_data(addr, addrlen);
	k_sleep(K_MSEC(50));
	errno = ztest_get_return_value();
	return errno ? -1 : 0;
}

static int mock_socket_offload_listen(void *obj, int backlog)
{
	k_sleep(K_MSEC(50));
	return 0;
}

static int mock_socket_offload_accept(void *obj, struct sockaddr *addr, socklen_t *addrlen)
{
	k_sleep(K_MSEC(50));
	return 0;
}

static ssize_t mock_socket_offload_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	k_sleep(K_MSEC(50));
	return 0;
}

static int mock_socket_offload_setsockopt(void *obj, int level, int optname, const void *optval,
					  socklen_t optlen)
{
	struct timeval time;

	k_sleep(K_MSEC(50));

	if ((level == SOL_SOCKET) && ((optname == SO_RCVTIMEO) || (optname == SO_SNDTIMEO))) {
		time.tv_sec = ((struct timeval *)optval)->tv_sec;
		time.tv_usec = ((struct timeval *)optval)->tv_usec;
	}

	return 0;
}

static int mock_socket_offload_getsockopt(void *obj, int level, int optname, void *optval,
					  socklen_t *optlen)
{
	k_sleep(K_MSEC(50));
	return 0;
}

static const struct socket_op_vtable mock_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = mock_socket_offload_read,
		.write = mock_socket_offload_write,
		.close = mock_socket_offload_close,
		.ioctl = mock_socket_offload_ioctl,
	},
	.bind = mock_socket_offload_bind,
	.connect = mock_socket_offload_connect,
	.listen = mock_socket_offload_listen,
	.accept = mock_socket_offload_accept,
	.sendto = mock_socket_offload_sendto,
	.sendmsg = mock_socket_offload_sendmsg,
	.recvfrom = mock_socket_offload_recvfrom,
	.getsockopt = mock_socket_offload_getsockopt,
	.setsockopt = mock_socket_offload_setsockopt,
};

/**
 * There is no support for dns lookup, node has to be a valid ip address
 * that is parseable via net_ipaddr_parse
 */
static int mock_socket_offload_getaddrinfo(const char *node, const char *service,
					   const struct zsock_addrinfo *hints,
					   struct zsock_addrinfo **res)
{
	struct sockaddr_in *ai_addr;
	struct zsock_addrinfo *ai;
	unsigned long port = 0;

	if (!node) {
		return -1;
	}

	if (service) {
		port = strtol(service, NULL, 10);
		if (port < 1 || port > USHRT_MAX) {
			return -1;
		}
	}

	if (!res) {
		return -1;
	}

	if (hints && hints->ai_family != AF_INET) {
		return -1;
	}

	*res = calloc(1, sizeof(struct zsock_addrinfo));
	ai = *res;
	if (!ai) {
		return -1;
	}

	ai_addr = calloc(1, sizeof(*ai_addr));
	if (!ai_addr) {
		free(*res);
		return -1;
	}

	ai->ai_family = AF_INET;
	ai->ai_socktype = hints ? hints->ai_socktype : SOCK_STREAM;
	ai->ai_protocol = ai->ai_socktype == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP;

	ai_addr->sin_family = ai->ai_family;
	ai_addr->sin_port = htons(port);
	ai->ai_addrlen = sizeof(*ai_addr);
	ai->ai_addr = (struct sockaddr *)ai_addr;

	if (net_ipaddr_parse(node, strlen(node), (struct sockaddr *)ai_addr)) {
		return 0;
	}
	if (strncmp(node, "example.com", 11) == 0) {
		ai_addr->sin_addr = (struct in_addr){.s4_addr = {93, 184, 216, 34}};
		return 0;
	}
	if (strncmp(node, "coap.me", 7) == 0) {
		ai_addr->sin_addr = (struct in_addr){.s4_addr = {134, 102, 218, 18}};
		return 0;
	}


	free(ai_addr);
	free(*res);
	return -1;
}

static void mock_socket_offload_freeaddrinfo(struct zsock_addrinfo *res)
{
	__ASSERT_NO_MSG(res);

	free(res->ai_addr);
	free(res);
}

bool mock_socket_is_supported(int family, int type, int proto)
{
	return true;
}

int mock_socket_create(int family, int type, int proto)
{
	int fd = zvfs_reserve_fd();
	struct net_context *ctx;
	int res;

	if (fd < 0) {
		return -1;
	}

	if (proto == 0) {
		if (family == AF_INET || family == AF_INET6) {
			if (type == SOCK_DGRAM) {
				proto = IPPROTO_UDP;
			} else if (type == SOCK_STREAM) {
				proto = IPPROTO_TCP;
			}
		}
	}

	res = net_context_get(family, type, proto, &ctx);
	if (res < 0) {
		zvfs_free_fd(fd);
		errno = -res;
		return -1;
	}

	/* Initialize user_data, all other calls will preserve it */
	ctx->user_data = NULL;

	/* The socket flags are stored here */
	ctx->socket_data = NULL;

	/* recv_q and accept_q are in union */
	k_fifo_init(&ctx->recv_q);

	/* Condition variable is used to avoid keeping lock for a long time
	 * when waiting data to be received
	 */
	k_condvar_init(&ctx->cond.recv);

	/* TCP context is effectively owned by both application
	 * and the stack: stack may detect that peer closed/aborted
	 * connection, but it must not dispose of the context behind
	 * the application back. Likewise, when application "closes"
	 * context, it's not disposed of immediately - there's yet
	 * closing handshake for stack to perform.
	 */
	if (proto == IPPROTO_TCP) {
		net_context_ref(ctx);
	}

	zvfs_finalize_fd(fd, ctx, (const struct fd_op_vtable *)&mock_socket_fd_op_vtable);

	return fd;
}

int mock_nrf_modem_lib_socket_offload_init(const struct device *arg)
{
	return 0;
}

static const struct socket_dns_offload mock_socket_dns_offload_ops = {
	.getaddrinfo = mock_socket_offload_getaddrinfo,
	.freeaddrinfo = mock_socket_offload_freeaddrinfo,
};

void mock_socket_iface_init(struct net_if *iface)
{
	mock_socket_iface_data.iface = iface;

	iface->if_dev->socket_offload = mock_socket_create;

	socket_offload_dns_register(&mock_socket_dns_offload_ops);
}
