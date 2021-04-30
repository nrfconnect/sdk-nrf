/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

/**
 * @file
 * @brief nrf91 socket offload provider
 */

#include <nrf_modem_limits.h>
#include <nrf_modem_os.h>
#include <errno.h>
#include <fcntl.h>
#include <init.h>
#include <net/socket_offload.h>
#include <nrf_socket.h>
#include <nrf_errno.h>
#include <sockets_internal.h>
#include <sys/fdtable.h>
#include <zephyr.h>

#if defined(CONFIG_POSIX_API)
#include <posix/poll.h>
#include <posix/sys/time.h>
#include <posix/sys/socket.h>
#endif

#if defined(CONFIG_NET_SOCKETS_OFFLOAD)

#if defined(CONFIG_NRF91_SOCKET_ENABLE_DEBUG_LOGS)
/* TODO: Add debug */
#endif

#define PROTO_WILDCARD 0

#define OBJ_TO_SD(obj) (((struct nrf_sock_ctx *)obj)->nrf_fd)
#define OBJ_TO_CTX(obj) ((struct nrf_sock_ctx *)obj)

/* Offloading context related to nRF socket. */
static struct nrf_sock_ctx {
	int nrf_fd; /* nRF socket descriptior. */
	struct k_mutex *lock; /* Mutex associated with the socket. */
} offload_ctx[NRF_MODEM_MAX_SOCKET_COUNT];

static K_MUTEX_DEFINE(ctx_lock);

static const struct socket_op_vtable nrf91_socket_fd_op_vtable;

static struct nrf_sock_ctx *allocate_ctx(int nrf_fd)
{
	struct nrf_sock_ctx *ctx = NULL;

	k_mutex_lock(&ctx_lock, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(offload_ctx); i++) {
		if (offload_ctx[i].nrf_fd == -1) {
			ctx = &offload_ctx[i];
			ctx->nrf_fd = nrf_fd;
			break;
		}
	}

	k_mutex_unlock(&ctx_lock);

	return ctx;
}

static void release_ctx(struct nrf_sock_ctx *ctx)
{
	k_mutex_lock(&ctx_lock, K_FOREVER);

	ctx->nrf_fd = -1;
	ctx->lock = NULL;

	k_mutex_unlock(&ctx_lock);
}

static void z_to_nrf_ipv4(const struct sockaddr *z_in,
			  struct nrf_sockaddr_in *nrf_out)
{
	const struct sockaddr_in *ptr = (const struct sockaddr_in *)z_in;

	nrf_out->sin_len = sizeof(struct nrf_sockaddr_in);
	nrf_out->sin_port = ptr->sin_port;
	nrf_out->sin_family = NRF_AF_INET;
	nrf_out->sin_addr.s_addr = ptr->sin_addr.s_addr;
}

static void nrf_to_z_ipv4(struct sockaddr *z_out,
			  const struct nrf_sockaddr_in *nrf_in)
{
	struct sockaddr_in *ptr = (struct sockaddr_in *)z_out;

	ptr->sin_port = nrf_in->sin_port;
	ptr->sin_family = AF_INET;
	ptr->sin_addr.s_addr = nrf_in->sin_addr.s_addr;
}

static void z_to_nrf_ipv6(const struct sockaddr *z_in,
			  struct nrf_sockaddr_in6 *nrf_out)
{
	const struct sockaddr_in6 *ptr = (const struct sockaddr_in6 *)z_in;

	/* nrf_out->sin6_flowinfo field not used */
	nrf_out->sin6_len = sizeof(struct nrf_sockaddr_in6);
	nrf_out->sin6_port = ptr->sin6_port;
	nrf_out->sin6_family = NRF_AF_INET6;
	memcpy(nrf_out->sin6_addr.s6_addr, ptr->sin6_addr.s6_addr,
		sizeof(struct in6_addr));
	nrf_out->sin6_scope_id = (uint32_t)ptr->sin6_scope_id;
}

static void nrf_to_z_ipv6(struct sockaddr *z_out,
			  const struct nrf_sockaddr_in6 *nrf_in)
{
	struct sockaddr_in6 *ptr = (struct sockaddr_in6 *)z_out;

	/* nrf_sockaddr_in6 fields .sin6_flowinfo and .sin6_len not used */
	ptr->sin6_port = nrf_in->sin6_port;
	ptr->sin6_family = AF_INET6;
	memcpy(ptr->sin6_addr.s6_addr, nrf_in->sin6_addr.s6_addr,
		sizeof(struct nrf_in6_addr));
	ptr->sin6_scope_id = (uint8_t)nrf_in->sin6_scope_id;
}

static int z_to_nrf_level(int z_in_level, int *nrf_out_level)
{
	int retval = 0;

	switch (z_in_level) {
	case SOL_TLS:
		*nrf_out_level = NRF_SOL_SECURE;
		break;
	case SOL_SOCKET:
		*nrf_out_level = NRF_SOL_SOCKET;
		break;
	case SOL_DFU:
		*nrf_out_level = NRF_SOL_DFU;
		break;
	case SOL_PDN:
		*nrf_out_level = NRF_SOL_PDN;
		break;
	default:
		retval = -1;
		break;
	}

	return retval;
}

static int z_to_nrf_optname(int z_in_level, int z_in_optname,
			    int *nrf_out_optname)
{
	int retval = 0;

	switch (z_in_level) {
	case SOL_TLS:
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
		case TLS_SESSION_CACHE:
			*nrf_out_optname = NRF_SO_SEC_SESSION_CACHE;
			break;
		default:
			retval = -1;
			break;
		}
		break;

	case SOL_SOCKET:
		switch (z_in_optname) {
		case SO_ERROR:
			*nrf_out_optname = NRF_SO_ERROR;
			break;
		case SO_RCVTIMEO:
			*nrf_out_optname = NRF_SO_RCVTIMEO;
			break;
		case SO_SNDTIMEO:
			*nrf_out_optname = NRF_SO_SNDTIMEO;
			break;
		case SO_BINDTODEVICE:
			*nrf_out_optname = NRF_SO_BINDTODEVICE;
			break;
		case SO_REUSEADDR:
			*nrf_out_optname = NRF_SO_REUSEADDR;
			break;
		case SO_SILENCE_ALL:
			*nrf_out_optname = NRF_SO_SILENCE_ALL;
			break;
		case SO_IP_ECHO_REPLY:
			*nrf_out_optname = NRF_SO_SILENCE_IP_ECHO_REPLY;
			break;
		case SO_IPV6_ECHO_REPLY:
			*nrf_out_optname = NRF_SO_SILENCE_IPV6_ECHO_REPLY;
			break;
		default:
			retval = -1;
			break;
		}
		break;

	case SOL_DFU:
		switch (z_in_optname) {
		case SO_DFU_FW_VERSION:
			*nrf_out_optname = NRF_SO_DFU_FW_VERSION;
			break;
		case SO_DFU_RESOURCES:
			*nrf_out_optname = NRF_SO_DFU_RESOURCES;
			break;
		case SO_DFU_TIMEO:
			*nrf_out_optname = NRF_SO_DFU_TIMEO;
			break;
		case SO_DFU_APPLY:
			*nrf_out_optname = NRF_SO_DFU_APPLY;
			break;
		case SO_DFU_REVERT:
			*nrf_out_optname = NRF_SO_DFU_REVERT;
			break;
		case SO_DFU_BACKUP_DELETE:
			*nrf_out_optname = NRF_SO_DFU_BACKUP_DELETE;
			break;
		case SO_DFU_OFFSET:
			*nrf_out_optname = NRF_SO_DFU_OFFSET;
			break;
		case SO_DFU_ERROR:
			*nrf_out_optname = NRF_SO_DFU_ERROR;
			break;
		default:
			retval = -1;
			break;
		}
		break;

	case SOL_PDN:
		switch (z_in_optname) {
		case SO_PDN_AF:
			*nrf_out_optname = NRF_SO_PDN_AF;
			break;
		case SO_PDN_CONTEXT_ID:
			*nrf_out_optname = NRF_SO_PDN_CONTEXT_ID;
			break;
		case SO_PDN_STATE:
			*nrf_out_optname = NRF_SO_PDN_STATE;
			break;
		default:
			retval = -1;
			break;
		}
		break;

	default:
		retval = -1;
		break;
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

	if (z_flags & MSG_TRUNC) {
		nrf_flags |= NRF_MSG_TRUNC;
	}

	if (z_flags & MSG_WAITALL) {
		nrf_flags |= NRF_MSG_WAITALL;
	}

	/* TODO: Handle missing flags, missing from zephyr,
	 * may also be missing from nrf_modem_lib.
	 * Missing flags from "man recv" or "man recvfrom":
	 *	MSG_CMSG_CLOEXEC
	 *	MSG_ERRQUEUE
	 *	MSG_OOB
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
	 *	NRF_MSG_WAITALL (covered)
	 *	NRF_MSG_TRUNC (covered)
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

static int z_to_nrf_family(sa_family_t z_family)
{
	switch (z_family) {
	case AF_INET:
		return NRF_AF_INET;
	case AF_INET6:
		return NRF_AF_INET6;
	case AF_LTE:
		return NRF_AF_LTE;
	case AF_LOCAL:
		return NRF_AF_LOCAL;
	case AF_PACKET:
		return NRF_AF_PACKET;
	case AF_UNSPEC:
	/* No NRF_AF_UNSPEC defined. */
	default:
		return -EAFNOSUPPORT;
	}
}

static int nrf_to_z_family(nrf_socket_family_t nrf_family)
{
	switch (nrf_family) {
	case NRF_AF_INET:
		return AF_INET;
	case NRF_AF_INET6:
		return AF_INET6;
	case NRF_AF_LTE:
		return AF_LTE;
	case NRF_AF_LOCAL:
		return AF_LOCAL;
	case NRF_AF_PACKET:
		return AF_PACKET;
	default:
		return -EAFNOSUPPORT;
	}
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
	case NRF_PROTO_PDN:
		return NPROTO_PDN;
	case NRF_PROTO_DFU:
		return NPROTO_DFU;
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

static int z_to_nrf_socktype(int socktype)
{
	switch (socktype) {
	case SOCK_MGMT:
		return NRF_SOCK_MGMT;
	case SOCK_RAW:
		return NRF_SOCK_RAW;
	default:
		return socktype;
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
	case NPROTO_DFU:
		return NRF_PROTO_DFU;
	case NPROTO_PDN:
		return NRF_PROTO_PDN;
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

static int nrf_to_z_dns_error_code(int nrf_error)
{
	switch (nrf_error) {
	case NRF_ENOMEM:
		return DNS_EAI_MEMORY;
	case NRF_EAGAIN:
		return DNS_EAI_AGAIN;
	case NRF_EAFNOSUPPORT:
		return DNS_EAI_NONAME;
	case NRF_EINPROGRESS:
		return DNS_EAI_INPROGRESS;
	case NRF_ENETUNREACH:
		errno = ENETUNREACH;
	default:
		return DNS_EAI_SYSTEM;
	}
}

static int z_to_nrf_addrinfo_hints(const struct zsock_addrinfo *z_in,
				   struct nrf_addrinfo *nrf_out)
{
	int family;

	memset(nrf_out, 0, sizeof(struct nrf_addrinfo));
	nrf_out->ai_flags = z_to_nrf_addrinfo_flags(z_in->ai_flags);
	nrf_out->ai_socktype = z_to_nrf_socktype(z_in->ai_socktype);

	family = z_to_nrf_family(z_in->ai_family);
	if (family == -EAFNOSUPPORT) {
		return -EAFNOSUPPORT;
	}
	nrf_out->ai_family = family;

	nrf_out->ai_protocol = z_to_nrf_protocol(z_in->ai_protocol);
	if (nrf_out->ai_protocol == -EPROTONOSUPPORT) {
		return -EPROTONOSUPPORT;
	}

	if (z_in->ai_canonname != NULL) {
		nrf_out->ai_canonname = z_in->ai_canonname;
	}

	return 0;
}

static int nrf_to_z_addrinfo(struct zsock_addrinfo *z_out,
			     const struct nrf_addrinfo *nrf_in)
{
	int family;

	z_out->ai_next = NULL;
	z_out->ai_canonname = NULL; /* TODO Do proper content copy. */
	z_out->ai_flags = nrf_to_z_addrinfo_flags(nrf_in->ai_flags);
	z_out->ai_socktype = nrf_in->ai_socktype;

	family = nrf_to_z_family(nrf_in->ai_family);
	if (family == -EAFNOSUPPORT) {
		return -EAFNOSUPPORT;
	}
	z_out->ai_family = family;

	z_out->ai_protocol = nrf_to_z_protocol(nrf_in->ai_protocol);
	if (z_out->ai_protocol == -EPROTONOSUPPORT) {
		z_out->ai_addr = NULL;
		return -EPROTONOSUPPORT;
	}

	if (nrf_in->ai_family == NRF_AF_INET) {
		z_out->ai_addr = k_malloc(sizeof(struct sockaddr_in));
		if (z_out->ai_addr == NULL) {
			return -ENOMEM;
		}
		z_out->ai_addrlen  = sizeof(struct sockaddr_in);
		nrf_to_z_ipv4(z_out->ai_addr,
			(const struct nrf_sockaddr_in *)nrf_in->ai_addr);
	} else if (nrf_in->ai_family == NRF_AF_INET6) {
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

	family = z_to_nrf_family(family);
	if (family == -EAFNOSUPPORT) {
		errno = EAFNOSUPPORT;
		return -1;
	}

	type = z_to_nrf_socktype(type);

	proto = z_to_nrf_protocol(proto);
	if (proto == -EPROTONOSUPPORT) {
		errno = EPROTONOSUPPORT;
		return -1;
	}

	retval = nrf_socket(family, type, proto);

	return retval;
}

static int nrf91_socket_offload_accept(void *obj, struct sockaddr *addr,
				       socklen_t *addrlen)
{
	int fd = z_reserve_fd();
	int sd = OBJ_TO_SD(obj);
	int new_sd = -1;
	struct nrf_sock_ctx *ctx = NULL;
	struct nrf_sockaddr *nrf_addr_ptr = NULL;
	nrf_socklen_t *nrf_addrlen_ptr = NULL;
	/* Use `struct nrf_sockaddr_in6` to fit both, IPv4 and IPv6 */
	struct nrf_sockaddr_in6 nrf_addr;
	nrf_socklen_t nrf_addrlen;

	if (fd < 0) {
		return -1;
	}

	if ((addr != NULL) && (addrlen != NULL)) {
		nrf_addr_ptr = (struct nrf_sockaddr *)&nrf_addr;
		nrf_addrlen_ptr = &nrf_addrlen;

		/* Workaround for the nrf_modem_lib issue, making `nrf_accept` to
		 * expect `nrf_addrlen` to be exactly of
		 * sizeof(struct nrf_sockaddr_in) size for IPv4
		 */
		if (*addrlen == sizeof(struct sockaddr_in)) {
			nrf_addrlen = sizeof(struct nrf_sockaddr_in);
		} else {
			nrf_addrlen = sizeof(struct nrf_sockaddr_in6);
		}
	}

	new_sd = nrf_accept(sd, nrf_addr_ptr, nrf_addrlen_ptr);
	if (new_sd < 0) {
		/* nrf_accept sets errno appropriately */
		return -1;
	}

	ctx = allocate_ctx(new_sd);
	if (ctx == NULL) {
		errno = ENOMEM;
		goto error;
	}

	if ((addr != NULL) && (addrlen != NULL)) {
		if (nrf_addr_ptr->sa_family == NRF_AF_INET) {
			*addrlen = sizeof(struct sockaddr_in);
			nrf_to_z_ipv4(
			    addr, (const struct nrf_sockaddr_in *)&nrf_addr);
		} else if (nrf_addr_ptr->sa_family == NRF_AF_INET6) {
			*addrlen = sizeof(struct sockaddr_in6);
			nrf_to_z_ipv6(
			    addr, (const struct nrf_sockaddr_in6 *)&nrf_addr);
		} else {
			goto error;
		}
	}

	z_finalize_fd(fd, ctx,
		      (const struct fd_op_vtable *)&nrf91_socket_fd_op_vtable);

	return fd;

error:
	if (new_sd != -1) {
		nrf_close(new_sd);
	}

	if (ctx != NULL) {
		release_ctx(ctx);
	}

	z_free_fd(fd);
	return -1;
}

static int nrf91_socket_offload_bind(void *obj, const struct sockaddr *addr,
				     socklen_t addrlen)
{
	int sd = OBJ_TO_SD(obj);
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
	errno = ENOTSUP;
	return retval;
}

static int nrf91_socket_offload_listen(void *obj, int backlog)
{
	int sd = OBJ_TO_SD(obj);

	return nrf_listen(sd, backlog);
}

static int nrf91_socket_offload_connect(void *obj, const struct sockaddr *addr,
					socklen_t addrlen)
{
	int sd = OBJ_TO_SD(obj);
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
		/* Pass in raw to library as it is non-IP address. */
		retval = nrf_connect(sd, (void *)addr, addrlen);
		if (retval < 0) {
			/* Not supported by library. */
			goto error;
		}
	}

	return retval;

error:
	retval = -1;
	errno = ENOTSUP;
	return retval;
}

static int nrf91_socket_offload_setsockopt(void *obj, int level, int optname,
					   const void *optval, socklen_t optlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;
	int nrf_level;
	int nrf_optname;
	struct nrf_timeval nrf_timeo;
	void *nrf_optval = (void *)optval;
	nrf_socklen_t nrf_optlen = optlen;

	if (z_to_nrf_level(level, &nrf_level) < 0)
		goto error;
	if (z_to_nrf_optname(level, optname, &nrf_optname) < 0)
		goto error;

	if ((level == SOL_SOCKET) && ((optname == SO_RCVTIMEO) ||
		(optname == SO_SNDTIMEO))) {
		nrf_timeo.tv_sec = ((struct timeval *)optval)->tv_sec;
		nrf_timeo.tv_usec = ((struct timeval *)optval)->tv_usec;
		nrf_optval = &nrf_timeo;
		nrf_optlen = sizeof(struct nrf_timeval);
	} else if ((level == SOL_TLS) && (optname == TLS_SESSION_CACHE)) {
		nrf_optlen = sizeof(nrf_sec_session_cache_t);
	}

	retval = nrf_setsockopt(sd, nrf_level, nrf_optname, nrf_optval,
				nrf_optlen);

	return retval;

error:
	retval = -1;
	errno = ENOPROTOOPT;
	return retval;
}

static int nrf91_socket_offload_getsockopt(void *obj, int level, int optname,
					   void *optval, socklen_t *optlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;
	int nrf_level;
	int nrf_optname;
	struct nrf_timeval nrf_timeo = {0, 0};
	void *nrf_optval = optval;
	nrf_socklen_t nrf_optlen = (nrf_socklen_t)*optlen;

	if (z_to_nrf_level(level, &nrf_level) < 0)
		goto error;
	if (z_to_nrf_optname(level, optname, &nrf_optname) < 0)
		goto error;

	if ((level == SOL_SOCKET) && ((optname == SO_RCVTIMEO) ||
		(optname == SO_SNDTIMEO))) {
		nrf_optval = &nrf_timeo;
		nrf_optlen = sizeof(struct nrf_timeval);
	}

	retval = nrf_getsockopt(sd, nrf_level, nrf_optname, nrf_optval,
				&nrf_optlen);


	if ((retval == 0) && (optval != NULL)) {
		*optlen = nrf_optlen;

		if (level == SOL_SOCKET) {
			if (optname == SO_ERROR) {
				/* Use nrf_modem_os_errno_set() to translate from nRF
				 * error to native error. If there is no error,
				 * don't translate it.
				 */
				if (*(int *)optval != 0) {
					nrf_modem_os_errno_set(*(int *)optval);
					*(int *)optval = errno;
				}
			} else if ((optname == SO_RCVTIMEO) ||
				(optname == SO_SNDTIMEO)) {
				((struct timeval *)optval)->tv_sec =
					nrf_timeo.tv_sec;
				((struct timeval *)optval)->tv_usec =
					nrf_timeo.tv_usec;
				*optlen = sizeof(struct timeval);
			}
		}
	}

	return retval;

error:
	retval = -1;
	errno = ENOPROTOOPT;
	return retval;
}

static ssize_t nrf91_socket_offload_recvfrom(void *obj, void *buf, size_t len,
					     int flags, struct sockaddr *from,
					     socklen_t *fromlen)
{
	struct nrf_sock_ctx *ctx = OBJ_TO_CTX(obj);
	ssize_t retval;

	k_mutex_unlock(ctx->lock);

	if (from == NULL) {
		retval = nrf_recvfrom(ctx->nrf_fd, buf, len, z_to_nrf_flags(flags),
				      NULL, NULL);
	} else {
		/* Allocate space for maximum of IPv4 and IPv6 family type. */
		struct nrf_sockaddr_in6 cliaddr_storage;
		nrf_socklen_t sock_len = sizeof(struct nrf_sockaddr_in6);
		struct nrf_sockaddr *cliaddr = (struct nrf_sockaddr *)&cliaddr_storage;

		retval = nrf_recvfrom(ctx->nrf_fd, buf, len, z_to_nrf_flags(flags),
				      cliaddr, &sock_len);
		if (cliaddr->sa_family == NRF_AF_INET) {
			nrf_to_z_ipv4(from, (struct nrf_sockaddr_in *)cliaddr);
			*fromlen = sizeof(struct sockaddr_in);
		} else if (cliaddr->sa_family == NRF_AF_INET6) {
			nrf_to_z_ipv6(from, (struct nrf_sockaddr_in6 *)
					  cliaddr);
			*fromlen = sizeof(struct sockaddr_in6);
		}
	}

	k_mutex_lock(ctx->lock, K_FOREVER);

	return retval;
}

static ssize_t nrf91_socket_offload_sendto(void *obj, const void *buf,
					   size_t len, int flags,
					   const struct sockaddr *to,
					   socklen_t tolen)
{
	int sd = OBJ_TO_SD(obj);
	ssize_t retval;

	if (IS_ENABLED(CONFIG_NRF91_SOCKET_SEND_SPLIT_LARGE_BLOCKS)) {
		len = MIN(len, CONFIG_NRF91_SOCKET_BLOCK_LIMIT);
	}

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
	errno = ENOTSUP;
	return retval;
}

static ssize_t nrf91_socket_offload_sendmsg(void *obj, const struct msghdr *msg,
					    int flags)
{
	ssize_t len = 0;
	ssize_t ret;
	ssize_t offset;
	int i;
	static K_MUTEX_DEFINE(sendmsg_lock);
	static uint8_t buf[CONFIG_NRF_MODEM_LIB_SENDMSG_BUF_SIZE];

	if (msg == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* The standard POSIX definition of sendmsg says it should return
	 * EWOULDBLOCK if the socket is in NONBLOCK mode and handed too much data.
	 * This implementation doesn't meet that requirement and will always
	 * block, even in NONBLOCK mode.  See POSIX.1-2017:
	 * <http://pubs.opengroup.org/onlinepubs/9699919799/functions/sendmsg.html>
	 */

	/* Try to reduce number of `sendto` calls - copy data if they fit into
	 * a single buffer
	 */

	for (i = 0; i < msg->msg_iovlen; i++) {
		len += msg->msg_iov[i].iov_len;
	}

	if (len <= sizeof(buf)) {
		/* Protect `buf` access with a mutex. */
		k_mutex_lock(&sendmsg_lock, K_FOREVER);
		len = 0;

		for (i = 0; i < msg->msg_iovlen; i++) {
			memcpy(buf + len, msg->msg_iov[i].iov_base,
			       msg->msg_iov[i].iov_len);
			len += msg->msg_iov[i].iov_len;
		}

		offset = 0;
		ret = 0;
		while ((offset < len) && (ret >= 0)) {
			ret = nrf91_socket_offload_sendto(obj,
				(buf + offset), (len - offset), flags,
				msg->msg_name, msg->msg_namelen);
			if (ret > 0) {
				offset += ret;
			}
		}

		k_mutex_unlock(&sendmsg_lock);
		return ret;
	}

	/* If the data won't fit into intermediate buffer, send the buffers
	 * separately
	 */

	len = 0;

	for (i = 0; i < msg->msg_iovlen; i++) {
		if (msg->msg_iov[i].iov_len == 0) {
			continue;
		}

		offset = 0;
		while (offset < msg->msg_iov[i].iov_len) {
			ret = nrf91_socket_offload_sendto(obj,
				(((uint8_t *) msg->msg_iov[i].iov_base) + offset),
				(msg->msg_iov[i].iov_len - offset), flags,
				msg->msg_name, msg->msg_namelen);
			if (ret < 0) {
				return ret;
			}
			offset += ret;
			len += ret;
		}
	}

	return len;
}

static inline int nrf91_socket_offload_poll(struct pollfd *fds, int nfds,
					    int timeout)
{
	int retval = 0;
	struct nrf_pollfd tmp[NRF_MODEM_MAX_SOCKET_COUNT] = { 0 };
	void *obj;

	for (int i = 0; i < nfds; i++) {
		tmp[i].events = 0;
		fds[i].revents = 0;

		if (fds[i].fd < 0) {
			/* Per POSIX, negative fd's are just ignored */
			tmp[i].fd = fds[i].fd;
			continue;
		} else {
			obj = z_get_fd_obj(fds[i].fd,
					   (const struct fd_op_vtable *)
						     &nrf91_socket_fd_op_vtable,
					   ENOTSUP);
			if (obj != NULL) {
				/* Offloaded socket found. */
				tmp[i].fd = OBJ_TO_SD(obj);
			} else {
				/* Non-offloaded socket, return an error. */
				fds[i].revents = POLLNVAL;
				retval++;
			}
		}

		/* Translate the API from native to nRF */
		if (fds[i].events & POLLIN) {
			tmp[i].events |= NRF_POLLIN;
		}
		if (fds[i].events & POLLOUT) {
			tmp[i].events |= NRF_POLLOUT;
		}
	}

	if (retval > 0) {
		return retval;
	}

	retval = nrf_poll((struct nrf_pollfd *)&tmp, nfds, timeout);

	/* Translate the API from nRF to native. */
	/* No need to translate .events, shall be untouched by poll() */
	for (int i = 0; i < nfds; i++) {
		if (fds[i].fd < 0) {
			continue;
		}

		if (tmp[i].revents & NRF_POLLIN) {
			fds[i].revents |= POLLIN;
		}
		if (tmp[i].revents & NRF_POLLOUT) {
			fds[i].revents |= POLLOUT;
		}
		if (tmp[i].revents & NRF_POLLERR) {
			fds[i].revents |= POLLERR;
		}
		if (tmp[i].revents & NRF_POLLNVAL) {
			fds[i].revents |= POLLNVAL;
		}
		if (tmp[i].revents & NRF_POLLHUP) {
			fds[i].revents |= POLLHUP;
		}
	}

	return retval;
}

static void nrf91_socket_offload_freeaddrinfo(struct zsock_addrinfo *root)
{
	struct zsock_addrinfo *next = root;

	while (next != NULL) {
		struct zsock_addrinfo *this = next;

		next = next->ai_next;
		k_free(this->ai_addr);
		k_free(this);
	}
}

static int nrf91_socket_offload_getaddrinfo(const char *node,
					    const char *service,
					    const struct zsock_addrinfo *hints,
					    struct zsock_addrinfo **res)
{
	int error;
	struct nrf_addrinfo nrf_hints;
	struct nrf_addrinfo nrf_hints_pdn;
	struct nrf_addrinfo *nrf_res = NULL;
	struct nrf_addrinfo *nrf_hints_ptr = NULL;
	static K_MUTEX_DEFINE(getaddrinfo_lock);

	memset(&nrf_hints, 0, sizeof(struct nrf_addrinfo));

	if (hints != NULL) {
		error = z_to_nrf_addrinfo_hints(hints, &nrf_hints);
		if (error == -EPROTONOSUPPORT) {
			return DNS_EAI_SOCKTYPE;
		} else if (error == -EAFNOSUPPORT) {
			return DNS_EAI_ADDRFAMILY;
		}

		if (hints->ai_next != NULL) {
			z_to_nrf_addrinfo_hints(hints->ai_next, &nrf_hints_pdn);
			if (error == -EPROTONOSUPPORT) {
				return DNS_EAI_SOCKTYPE;
			} else if (error == -EAFNOSUPPORT) {
				return DNS_EAI_ADDRFAMILY;
			}
			nrf_hints.ai_next = &nrf_hints_pdn;
		}
		nrf_hints_ptr = &nrf_hints;
	}

	k_mutex_lock(&getaddrinfo_lock, K_FOREVER);
	int retval = nrf_getaddrinfo(node, service, nrf_hints_ptr, &nrf_res);

	if (retval != 0) {
		error = nrf_to_z_dns_error_code(retval);
		retval = error;
		goto error;
	}

	struct nrf_addrinfo *next_nrf_res = nrf_res;
	struct zsock_addrinfo *latest_z_res = NULL;

	*res = NULL;

	while ((retval == 0) && (next_nrf_res != NULL)) {
		struct zsock_addrinfo *next_z_res =
					k_malloc(sizeof(struct zsock_addrinfo));

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
		} else if (error == -EAFNOSUPPORT) {
			retval = DNS_EAI_ADDRFAMILY;
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

error:
	k_mutex_unlock(&getaddrinfo_lock);
	return retval;
}

static int nrf91_socket_offload_fcntl(int fd, int cmd, va_list args)
{
	int retval;
	int flags;

	switch (cmd) {
	case F_SETFL:
		flags = va_arg(args, int);
		if (flags != 0 && flags != O_NONBLOCK)
			goto error;

		/* Translate flags from native to nRF */
		flags = (flags & O_NONBLOCK) ? NRF_O_NONBLOCK : 0;

		retval = nrf_fcntl(fd, NRF_F_SETFL, flags);
		break;

	case F_GETFL:
		flags = nrf_fcntl(fd, NRF_F_GETFL, 0);

		/* Translate flags from nRF to native */
		retval = (flags & NRF_O_NONBLOCK) ? O_NONBLOCK : 0;
		break;

	default:
		goto error;
	}

	return retval;

error:
	retval = -1;
	errno = EINVAL;
	return retval;
}

static int nrf91_socket_offload_ioctl(void *obj, unsigned int request,
				      va_list args)
{
	int sd = OBJ_TO_SD(obj);

	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		return -EXDEV;

	case ZFD_IOCTL_POLL_UPDATE:
		return -EOPNOTSUPP;

	case ZFD_IOCTL_POLL_OFFLOAD: {
		struct zsock_pollfd *fds;
		int nfds;
		int timeout;

		fds = va_arg(args, struct zsock_pollfd *);
		nfds = va_arg(args, int);
		timeout = va_arg(args, int);

		return nrf91_socket_offload_poll(fds, nfds, timeout);
	}

	case ZFD_IOCTL_SET_LOCK: {
		struct nrf_sock_ctx *ctx = OBJ_TO_CTX(obj);

		ctx->lock = va_arg(args, struct k_mutex *);

		return 0;
	}

	/* Otherwise, just forward to offloaded fcntl()
	 * In Zephyr, fcntl() is just an alias of ioctl().
	 */
	default:
		return nrf91_socket_offload_fcntl(sd, request, args);
	}
}

static ssize_t nrf91_socket_offload_read(void *obj, void *buffer, size_t count)
{
	return nrf91_socket_offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t nrf91_socket_offload_write(void *obj, const void *buffer,
					  size_t count)
{
	return nrf91_socket_offload_sendto(obj, buffer, count, 0, NULL, 0);
}

static int nrf91_socket_offload_close(void *obj)
{
	struct nrf_sock_ctx *ctx = OBJ_TO_CTX(obj);
	int retval;

	retval = nrf_close(ctx->nrf_fd);
	if (retval == 0) {
		release_ctx(ctx);
	}

	return retval;
}

static const struct socket_op_vtable nrf91_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = nrf91_socket_offload_read,
		.write = nrf91_socket_offload_write,
		.close = nrf91_socket_offload_close,
		.ioctl = nrf91_socket_offload_ioctl,
	},
	.bind = nrf91_socket_offload_bind,
	.connect = nrf91_socket_offload_connect,
	.listen = nrf91_socket_offload_listen,
	.accept = nrf91_socket_offload_accept,
	.sendto = nrf91_socket_offload_sendto,
	.sendmsg = nrf91_socket_offload_sendmsg,
	.recvfrom = nrf91_socket_offload_recvfrom,
	.getsockopt = nrf91_socket_offload_getsockopt,
	.setsockopt = nrf91_socket_offload_setsockopt,
};

static bool nrf91_socket_is_supported(int family, int type, int proto)
{
	if (IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD_TLS)) {
		return true;
	}

	if ((proto >= IPPROTO_TLS_1_0 && proto <= IPPROTO_TLS_1_2) ||
	    (proto >= IPPROTO_DTLS_1_0 && proto <= IPPROTO_DTLS_1_2)) {
		return false;
	}

	return true;
}

static int nrf91_socket_create(int family, int type, int proto)
{
	int fd = z_reserve_fd();
	int sd;
	struct nrf_sock_ctx *ctx;

	if (fd < 0) {
		return -1;
	}

	sd = nrf91_socket_offload_socket(family, type, proto);
	if (sd < 0) {
		z_free_fd(fd);
		return -1;
	}

	ctx = allocate_ctx(sd);
	if (ctx == NULL) {
		errno = ENOMEM;
		nrf_close(sd);
		z_free_fd(fd);
		return -1;
	}

	z_finalize_fd(fd, ctx,
		      (const struct fd_op_vtable *)&nrf91_socket_fd_op_vtable);

	return fd;
}

NET_SOCKET_REGISTER(nrf91_socket, AF_UNSPEC, nrf91_socket_is_supported,
		    nrf91_socket_create);

/* Create a network interface for nRF91 */

static int nrf91_nrf_modem_lib_socket_offload_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	for (int i = 0; i < ARRAY_SIZE(offload_ctx); i++) {
		offload_ctx[i].nrf_fd = -1;
	}

	return 0;
}

static const struct socket_dns_offload nrf91_socket_dns_offload_ops = {
	.getaddrinfo = nrf91_socket_offload_getaddrinfo,
	.freeaddrinfo = nrf91_socket_offload_freeaddrinfo,
};

static struct nrf91_socket_iface_data {
	struct net_if *iface;
} nrf91_socket_iface_data;

static void nrf91_socket_iface_init(struct net_if *iface)
{
	nrf91_socket_iface_data.iface = iface;

	iface->if_dev->offloaded = true;

	socket_offload_dns_register(&nrf91_socket_dns_offload_ops);
}

static struct net_if_api nrf91_if_api = {
	.init = nrf91_socket_iface_init,
};

/* TODO Get the actual MTU for the nRF91 LTE link. */
NET_DEVICE_OFFLOAD_INIT(nrf91_socket, "nrf91_socket",
			nrf91_nrf_modem_lib_socket_offload_init,
			device_pm_control_nop,
			&nrf91_socket_iface_data, NULL,
			0, &nrf91_if_api, 1280);

#endif
