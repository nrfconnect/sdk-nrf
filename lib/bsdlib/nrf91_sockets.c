/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 *
 */

/**
 * @file
 * @brief nrf91 socket offload provider
 */

#include <bsd_limits.h>
#include <bsd_os.h>
#include <errno.h>
#include <init.h>
#include <net/socket_offload.h>
#include <nrf_socket.h>
#include <zephyr.h>

#if defined(CONFIG_NET_SOCKETS_OFFLOAD)

#if defined(CONFIG_NRF91_SOCKET_ENABLE_DEBUG_LOGS)
/* TODO: Add debug */
#endif

#define PROTO_WILDCARD 0

static void z_to_nrf_ipv4(const struct sockaddr *z_in,
			  struct nrf_sockaddr_in *nrf_out)
{
	const struct sockaddr_in *ptr = (const struct sockaddr_in *)z_in;

	nrf_out->sin_len = sizeof(struct nrf_sockaddr_in);
	nrf_out->sin_port = ptr->sin_port;
	nrf_out->sin_family = ptr->sin_family;
	nrf_out->sin_addr.s_addr = ptr->sin_addr.s_addr;
}

static void nrf_to_z_ipv4(struct sockaddr *z_out,
			  const struct nrf_sockaddr_in *nrf_in)
{
	struct sockaddr_in *ptr = (struct sockaddr_in *)z_out;

	ptr->sin_port = nrf_in->sin_port;
	ptr->sin_family = nrf_in->sin_family;
	ptr->sin_addr.s_addr = nrf_in->sin_addr.s_addr;
}

static int z_to_nrf_level(int z_in_level, int *nrf_out_level)
{
	int retval = 0;

	switch (z_in_level) {
	case SOL_TLS:
		*nrf_out_level = NRF_SOL_SECURE;
		break;
	default:
		retval = -1;
		break;
	}

	return retval;
}

static int z_to_nrf_optname(int z_in_optname, int *nrf_out_optname)
{
	int retval = 0;

	switch (z_in_optname) {
	case TLS_SEC_TAG_LIST:
		*nrf_out_optname = NRF_SO_SEC_TAG_LIST;
		break;
	case TLS_HOSTNAME:
		*nrf_out_optname = NRF_SO_HOSTNAME;
		break;
	case TLS_CIPHERSUITE_LIST:
		*nrf_out_optname = NRF_SO_CIPHERSUITE_LIST;
		break;
	case TLS_CIPHERSUITE_USED:
		*nrf_out_optname = NRF_SO_CIPHER_IN_USE;
		break;
	case TLS_PEER_VERIFY:
		*nrf_out_optname = NRF_SO_SEC_PEER_VERIFY;
		break;
	case TLS_DTLS_ROLE:
		*nrf_out_optname = NRF_SO_SEC_ROLE;
		break;
	default:
		retval = -1;
	}

	return retval;
}

static int z_to_nrf_flags(int z_flags)
{
	int nrf_flags = 0;

	if (z_flags & MSG_DONTWAIT) {
		nrf_flags |= NRF_MSG_DONTWAIT;
	}

	if (z_flags & MSG_PEEK) {
		nrf_flags |= NRF_MSG_PEEK;
	}
	/* TODO: Handle missing flags, missing from zephyr,
	 * may also be missing from bsd socket library.
	 * Missing flags from "man recv" or "man recvfrom":
	 *	MSG_CMSG_CLOEXEC
	 *	MSG_ERRQUEUE
	 *	MSG_OOB
	 *	MSG_TRUNC
	 *	MSG_WAITALL
	 * Missing flags from "man send" or "man sendto":
	 *	MSG_CONFIRM
	 *	MSG_DONTROUTE
	 *	MSG_EOR
	 *	MSG_MORE
	 *	MSG_NOSIGNAL
	 * Flags defined in nrf_socket.h:
	 *	NRF_MSG_DONTROUTE
	 *	NRF_MSG_DONTWAIT (covered)
	 *	NRF_MSG_OOB
	 *	NRF_MSG_PEEK (covered)
	 *	NRF_MSG_WAITALL
	 */
	return nrf_flags;
}

static int nrf91_socket_offload_socket(int family, int type, int proto)
{
	int retval;

	switch (proto) {
	case IPPROTO_TCP:
		proto = NRF_IPPROTO_TCP;
		break;
	case IPPROTO_UDP:
		proto = NRF_IPPROTO_UDP;
		break;
	case IPPROTO_TLS_1_2:
		proto = NRF_SPROTO_TLS1v2;
		break;
	case NPROTO_AT:
		/* fall through */
	case PROTO_WILDCARD:
		/* No need to set, will match */
		break;
	/*
	 * TODO: handle missing TLS v1.3 define
	 * case IPPROTO_TLS_1_3:
	 *	proto = NRF_SPROTO_TLS1v3;
	 *	break;
	 */
	case IPPROTO_DTLS_1_2:
		proto = NRF_SPROTO_DTLS1v2;
		break;
	/* Let non-implemented cases fall through */
	case IPPROTO_DTLS_1_0:
		/* fall through */
	case IPPROTO_ICMP:
		/* fall through */
	case IPPROTO_ICMPV6:
		/* fall through */
	case IPPROTO_TLS_1_0:
		/* fall through */
	case IPPROTO_TLS_1_1:
		/* fall through */
	default:
		goto error;
		break;
	}

	retval = nrf_socket(family, type, proto);

	return retval;

error:
	retval = -1;
	errno = -EPROTONOSUPPORT;
	return retval;
}

static int nrf91_socket_offload_close(int sd)
{
	return nrf_close(sd);
}

static int nrf91_socket_offload_accept(int sd, struct sockaddr *addr,
				       socklen_t *addrlen)
{
	int retval;

	struct nrf_sockaddr nrf_addr;
	nrf_socklen_t nrf_addrlen = sizeof(nrf_addr);

	retval = nrf_accept(sd, &nrf_addr, &nrf_addrlen);
	if (retval < 0) {
		/* nrf_accept sets errno appropriately */
		return -1;
	}

	if (nrf_addr.sa_family == AF_INET) {
		*addrlen = sizeof(struct sockaddr_in);
		nrf_to_z_ipv4(addr, (const struct nrf_sockaddr_in *)&nrf_addr);
	} else if (nrf_addr.sa_family == AF_INET6) {
		/* TODO: Add ipv6 */
		goto error;
	} else {
		goto error;
	}

	return retval;

error:
	retval = -1;
	errno = -ENOTSUP;
	return retval;
}

static int nrf91_socket_offload_bind(int sd, const struct sockaddr *addr,
				     socklen_t addrlen)
{
	int retval;

	if (addr->sa_family == AF_INET) {
		struct nrf_sockaddr_in ipv4;

		z_to_nrf_ipv4(addr, &ipv4);
		retval = nrf_bind(sd, (const struct nrf_sockaddr *)&ipv4,
				  sizeof(struct nrf_sockaddr_in));
	} else if (addr->sa_family == AF_INET6) {
		/* TODO: Add ipv6 */
		goto error;
	} else {
		goto error;
	}

	return retval;

error:
	retval = -1;
	errno = -ENOTSUP;
	return retval;
}

static int nrf91_socket_offload_listen(int sd, int backlog)
{
	return nrf_listen(sd, backlog);
}

static int nrf91_socket_offload_connect(int sd, const struct sockaddr *addr,
					socklen_t addrlen)
{
	int retval;

	if (addr->sa_family == AF_INET) {
		struct nrf_sockaddr_in ipv4;

		z_to_nrf_ipv4(addr, &ipv4);
		retval = nrf_connect(sd, (const struct nrf_sockaddr_in *)&ipv4,
				     sizeof(struct nrf_sockaddr_in));
	} else if (addr->sa_family == AF_INET6) {
		/* TODO: Add ipv6 */
		goto error;
	} else {
		goto error;
	}

	return retval;

error:
	retval = -1;
	errno = -ENOTSUP;
	return retval;
}

static int nrf91_socket_offload_setsockopt(int sd, int level, int optname,
					   const void *optval, socklen_t optlen)
{
	int retval;
	int nrf_level;
	int nrf_optname;

	if (z_to_nrf_level(level, &nrf_level) < 0)
		goto error;
	if (z_to_nrf_optname(optname, &nrf_optname) < 0)
		goto error;

	retval = nrf_setsockopt(sd, nrf_level, nrf_optname, optval,
				(nrf_socklen_t)optlen);

	return retval;

error:
	retval = -1;
	errno = -ENOPROTOOPT;
	return retval;
}

static int nrf91_socket_offload_getsockopt(int sd, int level, int optname,
					   void *optval, socklen_t *optlen)
{
	int retval;
	int nrf_level;
	int nrf_optname;

	if (z_to_nrf_level(level, &nrf_level) < 0)
		goto error;
	if (z_to_nrf_optname(optname, &nrf_optname) < 0)
		goto error;

	retval = nrf_getsockopt(sd, level, optname, optval,
				(nrf_socklen_t *)optlen);

	return retval;

error:
	retval = -1;
	errno = -ENOPROTOOPT;
	return retval;
}

static ssize_t nrf91_socket_offload_recv(int sd, void *buf, size_t max_len,
					 int flags)
{
	return nrf_recv(sd, buf, max_len, z_to_nrf_flags(flags));
}

static ssize_t nrf91_socket_offload_recvfrom(int sd, void *buf, short int len,
					     short int flags,
					     struct sockaddr *from,
					     socklen_t *fromlen)
{
	return nrf_recvfrom(sd, buf, len, z_to_nrf_flags(flags), from,
			    (nrf_socklen_t *)fromlen);
}

static ssize_t nrf91_socket_offload_send(int sd, const void *buf, size_t len,
					 int flags)
{
	return nrf_send(sd, buf, len, z_to_nrf_flags(flags));
}

static ssize_t nrf91_socket_offload_sendto(int sd, const void *buf, size_t len,
					   int flags, const struct sockaddr *to,
					   socklen_t tolen)
{
	return nrf_sendto(sd, buf, len, z_to_nrf_flags(flags), to, tolen);
}

static inline int nrf91_socket_offload_poll(struct pollfd *fds, int nfds,
					    int timeout)
{
	int retval;
	struct nrf_pollfd tmp[BSD_MAX_SOCKET_COUNT] = {0};

	for (int i = 0; i < nfds; i++) {
		tmp[i].handle = fds[i].fd;

		/* Translate the API from native to nRF */
		if (fds[i].events & POLLIN) {
			tmp[i].requested |= NRF_POLLIN;
		}
		if (fds[i].events & POLLOUT) {
			tmp[i].requested |= NRF_POLLOUT;
		}
		if (fds[i].events & POLLERR) {
			tmp[i].requested |= NRF_POLLERR;
		}
	}

	retval = nrf_poll((struct nrf_pollfd *)&tmp, nfds, timeout);

	/* Translate the API from nRF to native */
	/* No need to translate .requested, shall be untouched by poll() */
	for (int i = 0; i < nfds; i++) {
		fds[i].revents = 0;
		if (tmp[i].returned & NRF_POLLIN) {
			fds[i].revents |= POLLIN;
		}
		if (tmp[i].returned & NRF_POLLOUT) {
			fds[i].revents |= POLLOUT;
		}
		if (tmp[i].requested & NRF_POLLERR) {
			fds[i].revents |= POLLERR;
		}
	}

	return retval;
}

static int nrf91_socket_offload_getaddrinfo(const char *node,
					    const char *service,
					    const struct addrinfo *hints,
					    struct addrinfo **res)
{
	return nrf_getaddrinfo(node, service, (struct nrf_addrinfo *)hints,
				 (struct nrf_addrinfo **)res);
}

static void nrf91_socket_offload_freeaddrinfo(struct addrinfo *res)
{
	nrf_freeaddrinfo((struct nrf_addrinfo *)res);
}

static const struct socket_offload nrf91_socket_offload_ops = {
	.socket = nrf91_socket_offload_socket,
	.close = nrf91_socket_offload_close,
	.accept = nrf91_socket_offload_accept,
	.bind = nrf91_socket_offload_bind,
	.listen = nrf91_socket_offload_listen,
	.connect = nrf91_socket_offload_connect,
	.setsockopt = nrf91_socket_offload_setsockopt,
	.getsockopt = nrf91_socket_offload_getsockopt,
	.recv = nrf91_socket_offload_recv,
	.recvfrom = nrf91_socket_offload_recvfrom,
	.send = nrf91_socket_offload_send,
	.sendto = nrf91_socket_offload_sendto,
	.poll = nrf91_socket_offload_poll,
	.getaddrinfo = nrf91_socket_offload_getaddrinfo,
	.freeaddrinfo = nrf91_socket_offload_freeaddrinfo,
};

static int nrf91_bsdlib_socket_offload_init(struct device *arg)
{
	ARG_UNUSED(arg);

	socket_offload_register(&nrf91_socket_offload_ops);

	return 0;
}

SYS_INIT(nrf91_bsdlib_socket_offload_init, APPLICATION,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
