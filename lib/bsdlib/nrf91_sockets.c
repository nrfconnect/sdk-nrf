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

static void z_to_nrf_ipv6(const struct sockaddr *z_in,
			  struct nrf_sockaddr_in6 *nrf_out)
{
	const struct sockaddr_in6 *ptr = (const struct sockaddr_in6 *)z_in;

	/* nrf_out->sin6_flowinfo field not used */
	nrf_out->sin6_len = sizeof(struct nrf_sockaddr_in6);
	nrf_out->sin6_port = ptr->sin6_port;
	nrf_out->sin6_family = ptr->sin6_family;
	memcpy(nrf_out->sin6_addr.s6_addr, ptr->sin6_addr.s6_addr,
		sizeof(struct in6_addr));
	nrf_out->sin6_scope_id = (u32_t)ptr->sin6_scope_id;
}

static void nrf_to_z_ipv6(struct sockaddr *z_out,
			  const struct nrf_sockaddr_in6 *nrf_in)
{
	struct sockaddr_in6 *ptr = (struct sockaddr_in6 *)z_out;

	/* nrf_sockaddr_in6 fields .sin6_flowinfo and .sin6_len not used */
	ptr->sin6_port = nrf_in->sin6_port;
	ptr->sin6_family = nrf_in->sin6_family;
	memcpy(ptr->sin6_addr.s6_addr, nrf_in->sin6_addr.s6_addr,
		sizeof(struct nrf_in6_addr));
	ptr->sin6_scope_id = (u8_t)nrf_in->sin6_scope_id;
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

static int z_to_nrf_addrinfo_flags(int flags)
{
	/* Flags not implemented.*/
	ARG_UNUSED(flags);
	return 0;
}

static int nrf_to_z_addrinfo_flags(int flags)
{
	/* Flags not implemented.*/
	ARG_UNUSED(flags);
	return 0;
}

static int nrf_to_z_protocol(int proto)
{
	switch (proto) {
	case NRF_IPPROTO_TCP:
		return IPPROTO_TCP;
	case NRF_IPPROTO_UDP:
		return IPPROTO_UDP;
	case NRF_SPROTO_TLS1v2:
		return IPPROTO_TLS_1_2;
	case NRF_PROTO_AT:
		return NPROTO_AT;
	case 0:
		return PROTO_WILDCARD;
	/*
	 * TODO: handle missing TLS v1.3 define
	 * case IPPROTO_TLS_1_3:
	 *      return NRF_SPROTO_TLS1v3;
	 */
	case NRF_SPROTO_DTLS1v2:
		return IPPROTO_DTLS_1_2;
	default:
		return -EPROTONOSUPPORT;
	}
}



static int z_to_nrf_protocol(int proto)
{
	switch (proto) {
	case IPPROTO_TCP:
		return NRF_IPPROTO_TCP;
	case IPPROTO_UDP:
		return NRF_IPPROTO_UDP;
	case IPPROTO_TLS_1_2:
		return NRF_SPROTO_TLS1v2;
	case NPROTO_AT:
		return NRF_PROTO_AT;
	case PROTO_WILDCARD:
		return 0;
	/*
	 * TODO: handle missing TLS v1.3 define
	 * case IPPROTO_TLS_1_3:
	 *      return NRF_SPROTO_TLS1v3;
	 */
	case IPPROTO_DTLS_1_2:
		return NRF_SPROTO_DTLS1v2;
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
		return -EPROTONOSUPPORT;
	}
}

static int z_to_nrf_addrinfo_hints(const struct addrinfo *z_in,
			      struct nrf_addrinfo *nrf_out)
{
	memset(nrf_out, 0, sizeof(struct nrf_addrinfo));
	nrf_out->ai_flags     = z_to_nrf_addrinfo_flags(z_in->ai_flags);
	nrf_out->ai_family    = z_in->ai_family;
	nrf_out->ai_socktype  = z_in->ai_socktype;
	nrf_out->ai_protocol  = z_to_nrf_protocol(z_in->ai_protocol);

	if (nrf_out->ai_protocol == -EPROTONOSUPPORT) {
		return -EPROTONOSUPPORT;
	}
	return 0;
}

static int nrf_to_z_addrinfo(struct addrinfo *z_out,
			      const struct nrf_addrinfo *nrf_in)
{
	z_out->ai_next      = NULL;
	z_out->ai_canonname = NULL; /* TODO Do proper content copy. */
	z_out->ai_flags     = nrf_to_z_addrinfo_flags(nrf_in->ai_flags);
	z_out->ai_family    = nrf_in->ai_family;
	z_out->ai_socktype  = nrf_in->ai_socktype;
	z_out->ai_protocol  = nrf_to_z_protocol(nrf_in->ai_protocol);

	if (z_out->ai_protocol == -EPROTONOSUPPORT) {
		z_out->ai_addr = NULL;
		return -EPROTONOSUPPORT;
	}
	if (nrf_in->ai_family == AF_INET) {
		z_out->ai_addr = k_malloc(sizeof(struct sockaddr_in));
		if (z_out->ai_addr == NULL) {
			return -ENOMEM;
		}
		z_out->ai_addrlen  = sizeof(struct sockaddr_in);
		nrf_to_z_ipv4(z_out->ai_addr,
			(const struct nrf_sockaddr_in *)nrf_in->ai_addr);
	} else if (nrf_in->ai_family == AF_INET6) {
		z_out->ai_addr = k_malloc(sizeof(struct sockaddr_in6));
		if (z_out->ai_addr == NULL) {
			return -ENOMEM;
		}
		z_out->ai_addrlen  = sizeof(struct sockaddr_in6);
		nrf_to_z_ipv6(z_out->ai_addr,
			(const struct nrf_sockaddr_in6 *)nrf_in->ai_addr);
	} else {
		return -EPROTONOSUPPORT;
	}
	return 0;
}

static int nrf91_socket_offload_socket(int family, int type, int proto)
{
	int retval;

	proto = z_to_nrf_protocol(proto);
	if (proto == -EPROTONOSUPPORT) {
		errno = -EPROTONOSUPPORT;
		return -1;
	}

	retval = nrf_socket(family, type, proto);

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
		*addrlen = sizeof(struct sockaddr_in6);
		nrf_to_z_ipv6(addr, (const struct nrf_sockaddr_in6 *)&nrf_addr);
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
		struct nrf_sockaddr_in6 ipv6;

		z_to_nrf_ipv6(addr, &ipv6);
		retval = nrf_bind(sd, (const struct nrf_sockaddr *)&ipv6,
				  sizeof(struct nrf_sockaddr_in6));
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
		struct nrf_sockaddr_in6 ipv6;

		z_to_nrf_ipv6(addr, &ipv6);
		retval = nrf_connect(sd, (const struct nrf_sockaddr *)&ipv6,
				  sizeof(struct nrf_sockaddr_in6));
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
	ssize_t retval;

	if (from == NULL) {
		retval = nrf_recvfrom(sd, buf, len, z_to_nrf_flags(flags), NULL,
				      NULL);
	} else {
		struct nrf_sockaddr cliaddr;
		nrf_socklen_t sock_len = sizeof(struct nrf_sockaddr);

		retval = nrf_recvfrom(sd, buf, len, z_to_nrf_flags(flags),
				      &cliaddr, &sock_len);
		if (cliaddr.sa_family == AF_INET) {
			nrf_to_z_ipv4(from, (struct nrf_sockaddr_in *)&cliaddr);
			*fromlen = sizeof(struct sockaddr_in);
		} else if (cliaddr.sa_family == AF_INET6) {
			nrf_to_z_ipv6(from, (struct nrf_sockaddr_in6 *)
					  &cliaddr);
			*fromlen = sizeof(struct sockaddr_in6);
		}
	}

	return retval;
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
	ssize_t retval;

	if (to == NULL) {
		retval = nrf_sendto(sd, buf, len, z_to_nrf_flags(flags), NULL,
				    0);
	} else if (to->sa_family == AF_INET) {
		struct nrf_sockaddr_in ipv4;
		nrf_socklen_t sock_len = sizeof(struct nrf_sockaddr_in);

		z_to_nrf_ipv4(to, &ipv4);
		retval = nrf_sendto(sd, buf, len, z_to_nrf_flags(flags), &ipv4,
				    sock_len);
	} else if (to->sa_family == AF_INET6) {
		struct nrf_sockaddr_in6 ipv6;
		nrf_socklen_t sock_len = sizeof(struct nrf_sockaddr_in6);

		z_to_nrf_ipv6(to, &ipv6);
		retval = nrf_sendto(sd, buf, len, z_to_nrf_flags(flags), &ipv6,
				    sock_len);
	} else {
		goto error;
	}
	return retval;

error:
	retval = -1;
	errno = -ENOTSUP;
	return retval;
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

static void nrf91_socket_offload_freeaddrinfo(struct addrinfo *root)
{
	struct addrinfo *next = root;

	while (next != NULL) {
		struct addrinfo *this = next;

		next = next->ai_next;
		k_free(this->ai_addr);
		k_free(this);
	}
}

static int nrf91_socket_offload_getaddrinfo(const char *node,
					    const char *service,
					    const struct addrinfo *hints,
					    struct addrinfo **res)
{
	int error;
	struct nrf_addrinfo nrf_hints;
	struct nrf_addrinfo *nrf_hints_ptr = NULL;
	struct nrf_addrinfo *nrf_res = NULL;

	memset(&nrf_hints, 0, sizeof(struct nrf_addrinfo));

	if (hints != NULL) {
		nrf_hints_ptr = &nrf_hints;
		error = z_to_nrf_addrinfo_hints(hints, &nrf_hints);
		if (error == -EPROTONOSUPPORT) {
			return DNS_EAI_SOCKTYPE;
		}
	}
	int retval = nrf_getaddrinfo(node, service, nrf_hints_ptr, &nrf_res);

	struct nrf_addrinfo *next_nrf_res = nrf_res;
	struct addrinfo *latest_z_res = NULL;

	*res = NULL;

	while ((retval == 0) && (next_nrf_res != NULL)) {
		struct addrinfo *next_z_res = k_malloc(sizeof(struct addrinfo));

		if (next_z_res == NULL) {
			retval = DNS_EAI_MEMORY;
			break;
		}
		error = nrf_to_z_addrinfo(next_z_res, next_nrf_res);
		if (error == -ENOMEM) {
			retval = DNS_EAI_MEMORY;
			k_free(next_z_res);
			break;
		} else if (error == -EPROTONOSUPPORT) {
			retval = DNS_EAI_SOCKTYPE;
			k_free(next_z_res);
			break;
		}
		if (latest_z_res == NULL) {
			/* This is the first node processed. */
			*res = next_z_res;
		} else {
			latest_z_res->ai_next = next_z_res;
		}
		latest_z_res = next_z_res;
		next_nrf_res = next_nrf_res->ai_next;
	}
	if (retval != 0) {
		/* Release any already allocated list nodes. */
		nrf91_socket_offload_freeaddrinfo(*res);
		*res = NULL;
	}
	nrf_freeaddrinfo(nrf_res);

	return retval;
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
