/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

/**
 * @file
 * @brief nrf9x socket offload provider
 */

#include <nrf_modem.h>
#include <nrf_modem_os.h>
#include <errno.h>
#include <fcntl.h>
#include <zephyr/init.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/net_ip.h>
#include <nrf_socket.h>
#include <nrf_errno.h>
#include <nrf_gai_errors.h>
#include <sockets_internal.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/kernel.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/util_macro.h>

#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/sys/socket.h>
#endif

#if defined(CONFIG_NET_SOCKETS_OFFLOAD)

/* Macro used to define a private Connection Manager connectivity context type.
 * Required but not implemented.
 */
#define NRF_MODEM_LIB_NET_IF_CTX_TYPE void *

#define OBJ_TO_SD(obj) (((struct nrf_sock_ctx *)obj)->nrf_fd)
#define OBJ_TO_CTX(obj) ((struct nrf_sock_ctx *)obj)

/* Offloading context related to nRF socket. */
static struct nrf_sock_ctx {
	int nrf_fd; /* nRF socket descriptior. */
	struct k_mutex *lock; /* Mutex associated with the socket. */
	struct k_poll_signal poll; /* poll() signal. */
} offload_ctx[NRF_MODEM_MAX_SOCKET_COUNT];

static K_MUTEX_DEFINE(ctx_lock);

static const struct socket_op_vtable nrf9x_socket_fd_op_vtable;

/* Offloading disabled in general. */
static bool offload_disabled;
/* TLS offloading disabled only. */
static bool tls_offload_disabled;

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

	/* nrf_sockaddr_in6 field .sin6_flowinfo not used */
	ptr->sin6_port = nrf_in->sin6_port;
	ptr->sin6_family = AF_INET6;
	memcpy(ptr->sin6_addr.s6_addr, nrf_in->sin6_addr.s6_addr,
		sizeof(struct nrf_in6_addr));
	ptr->sin6_scope_id = (uint8_t)nrf_in->sin6_scope_id;
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
			*nrf_out_optname = NRF_SO_SEC_HOSTNAME;
			break;
		case TLS_CIPHERSUITE_LIST:
			*nrf_out_optname = NRF_SO_SEC_CIPHERSUITE_LIST;
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
		case TLS_SESSION_CACHE_PURGE:
			*nrf_out_optname = NRF_SO_SEC_SESSION_CACHE_PURGE;
			break;
		case TLS_DTLS_HANDSHAKE_TIMEO:
			*nrf_out_optname = NRF_SO_SEC_DTLS_HANDSHAKE_TIMEO;
			break;
		case TLS_CIPHERSUITE_USED:
			*nrf_out_optname = NRF_SO_SEC_CIPHERSUITE_USED;
			break;
		case TLS_DTLS_CID:
			*nrf_out_optname = NRF_SO_SEC_DTLS_CID;
			break;
		case TLS_DTLS_CID_STATUS:
			*nrf_out_optname = NRF_SO_SEC_DTLS_CID_STATUS;
			break;
		case TLS_DTLS_CONN_SAVE:
			*nrf_out_optname = NRF_SO_SEC_DTLS_CONN_SAVE;
			break;
		case TLS_DTLS_CONN_LOAD:
			*nrf_out_optname = NRF_SO_SEC_DTLS_CONN_LOAD;
			break;
		case TLS_DTLS_HANDSHAKE_STATUS:
			*nrf_out_optname = NRF_SO_SEC_HANDSHAKE_STATUS;
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
		case SO_KEEPOPEN:
			*nrf_out_optname = NRF_SO_KEEPOPEN;
			break;
		case SO_EXCEPTIONAL_DATA:
			*nrf_out_optname = NRF_SO_EXCEPTIONAL_DATA;
			break;
		case SO_RCVTIMEO:
			*nrf_out_optname = NRF_SO_RCVTIMEO;
			break;
		case SO_SNDTIMEO:
			*nrf_out_optname = NRF_SO_SNDTIMEO;
			break;
		case SO_BINDTOPDN:
			*nrf_out_optname = NRF_SO_BINDTOPDN;
			break;
		case SO_REUSEADDR:
			*nrf_out_optname = NRF_SO_REUSEADDR;
			break;
		case SO_RAI:
			*nrf_out_optname = NRF_SO_RAI;
			break;
		default:
			retval = -1;
			break;
		}
		break;

	case IPPROTO_IP:
		switch (z_in_optname) {
		case SO_IP_ECHO_REPLY:
			*nrf_out_optname = NRF_SO_IP_ECHO_REPLY;
			break;
		default:
			retval = -1;
			break;
		}
		break;

	case IPPROTO_IPV6:
		switch (z_in_optname) {
		case SO_IPV6_ECHO_REPLY:
			*nrf_out_optname = NRF_SO_IPV6_ECHO_REPLY;
			break;
		default:
			retval = -1;
			break;
		}
		break;

	case IPPROTO_TCP:
		switch (z_in_optname) {
		case SO_TCP_SRV_SESSTIMEO:
			*nrf_out_optname = NRF_SO_TCP_SRV_SESSTIMEO;
			break;
		default:
			retval = -1;
			break;
		}
		break;

	case IPPROTO_ALL:
		switch (z_in_optname) {
		case SO_SILENCE_ALL:
			*nrf_out_optname = NRF_SO_SILENCE_ALL;
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

static void z_to_nrf_addrinfo_hints(const struct zsock_addrinfo *z_in,
				   struct nrf_addrinfo *nrf_out)
{
	memset(nrf_out, 0, sizeof(struct nrf_addrinfo));

	nrf_out->ai_flags = z_in->ai_flags;
	nrf_out->ai_socktype = z_in->ai_socktype;
	nrf_out->ai_family = z_in->ai_family;
	nrf_out->ai_protocol = z_in->ai_protocol;

	if (z_in->ai_canonname != NULL) {
		nrf_out->ai_canonname = z_in->ai_canonname;
	}
}

static int nrf_to_z_addrinfo(struct zsock_addrinfo *z_out,
			     const struct nrf_addrinfo *nrf_in)
{
	z_out->ai_next = NULL;
	z_out->ai_canonname = NULL; /* TODO Do proper content copy. */
	z_out->ai_flags = nrf_in->ai_flags;
	z_out->ai_socktype = nrf_in->ai_socktype;

	z_out->ai_family = nrf_in->ai_family;
	z_out->ai_protocol = nrf_in->ai_protocol;

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
		return -EAFNOSUPPORT;
	}

	return 0;
}

static int nrf9x_socket_offload_socket(int family, int type, int proto)
{
	int retval;

	retval = nrf_socket(family, type, proto);

	return retval;
}

static int nrf9x_socket_offload_accept(void *obj, struct sockaddr *addr,
				       socklen_t *addrlen)
{
	int fd = zvfs_reserve_fd();
	int sd = OBJ_TO_SD(obj);
	int new_sd = -1;
	struct nrf_sock_ctx *ctx = NULL;
	struct nrf_sockaddr *nrf_addr_ptr = NULL;
	nrf_socklen_t *nrf_addrlen_ptr = NULL;
	/* Use `struct nrf_sockaddr_in6` to fit both, IPv4 and IPv6 */
	struct nrf_sockaddr_in6 nrf_addr;
	nrf_socklen_t nrf_addrlen;

	/* `zvfs_reserve_fd()` can fail */
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
		goto error;
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
			errno = ENOTSUP;
			goto error;
		}
	}

	zvfs_finalize_fd(fd, ctx,
		      (const struct fd_op_vtable *)&nrf9x_socket_fd_op_vtable);

	return fd;

error:
	if (new_sd >= 0) {
		nrf_close(new_sd);
	}

	if (ctx != NULL) {
		release_ctx(ctx);
	}

	zvfs_free_fd(fd);
	return -1;
}

static int nrf9x_socket_offload_bind(void *obj, const struct sockaddr *addr,
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
		errno = EAFNOSUPPORT;
		retval = -1;
	}

	return retval;
}

static int nrf9x_socket_offload_listen(void *obj, int backlog)
{
	int sd = OBJ_TO_SD(obj);

	return nrf_listen(sd, backlog);
}

static int nrf9x_socket_offload_connect(void *obj, const struct sockaddr *addr,
					socklen_t addrlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;

	if (addr->sa_family == AF_INET) {
		struct nrf_sockaddr_in ipv4;

		z_to_nrf_ipv4(addr, &ipv4);
		retval = nrf_connect(sd, (struct nrf_sockaddr *)&ipv4,
				     sizeof(struct nrf_sockaddr_in));
	} else if (addr->sa_family == AF_INET6) {
		struct nrf_sockaddr_in6 ipv6;

		z_to_nrf_ipv6(addr, &ipv6);
		retval = nrf_connect(sd, (struct nrf_sockaddr *)&ipv6,
				  sizeof(struct nrf_sockaddr_in6));
	} else {
		/* Pass in raw to library as it is non-IP address. */
		retval = nrf_connect(sd, (void *)addr, addrlen);
	}

	return retval;
}

static int nrf9x_socket_offload_setsockopt(void *obj, int level, int optname,
					   const void *optval, socklen_t optlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;
	int nrf_level = level;
	int nrf_optname;
	struct nrf_timeval nrf_timeo = { 0 };
	void *nrf_optval = (void *)optval;
	nrf_socklen_t nrf_optlen = optlen;

	if ((level == SOL_SOCKET) && (optname == SO_BINDTODEVICE)) {
		if (IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD_DISPATCHER)) {
			/* This is used by socket dispatcher in zephyr and is forwarded to the
			 * offloading layer afterwards. We don't have to do anything in this case.
			 */
			return 0;
		}

		errno = EOPNOTSUPP;
		return -1;
	}

	if (z_to_nrf_optname(level, optname, &nrf_optname) < 0) {
		errno = ENOPROTOOPT;
		return -1;
	}

	if ((level == SOL_SOCKET) && ((optname == SO_RCVTIMEO) ||
				      (optname == SO_SNDTIMEO))) {
		if (optval != NULL) {
			nrf_timeo.tv_sec = ((struct timeval *)optval)->tv_sec;
			nrf_timeo.tv_usec = ((struct timeval *)optval)->tv_usec;
			nrf_optval = &nrf_timeo;
			nrf_optlen = sizeof(struct nrf_timeval);
		}
	} else if ((level == SOL_TLS) && (optname == TLS_SESSION_CACHE)) {
		nrf_optlen = sizeof(int);
	}

	retval = nrf_setsockopt(sd, nrf_level, nrf_optname, nrf_optval,
				nrf_optlen);

	return retval;
}

static int nrf9x_socket_offload_getsockopt(void *obj, int level, int optname,
					   void *optval, socklen_t *optlen)
{
	int sd = OBJ_TO_SD(obj);
	int retval;
	int nrf_level = level;
	int nrf_optname;
	struct nrf_timeval nrf_timeo = {0, 0};
	nrf_socklen_t nrf_timeo_size = sizeof(struct nrf_timeval);
	void *nrf_optval = optval;
	nrf_socklen_t *nrf_optlen = (nrf_socklen_t *)optlen;

	if (z_to_nrf_optname(level, optname, &nrf_optname) < 0) {
		errno = ENOPROTOOPT;
		return -1;
	}

	if ((level == SOL_SOCKET) && ((optname == SO_RCVTIMEO) ||
		(optname == SO_SNDTIMEO))) {
		nrf_optval = &nrf_timeo;
		nrf_optlen = &nrf_timeo_size;
	}

	retval = nrf_getsockopt(sd, nrf_level, nrf_optname, nrf_optval,
				nrf_optlen);

	/* The call was successful so we can assume that both
	 * `nrf_optval` (`optval`) and `nrf_optlen` (`optlen`)
	 * are not NULL, so we can dereference them.
	 */
	if (retval == 0) {
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
}

static ssize_t nrf9x_socket_offload_recvfrom(void *obj, void *buf, size_t len,
					     int flags, struct sockaddr *from,
					     socklen_t *fromlen)
{
	struct nrf_sock_ctx *ctx = OBJ_TO_CTX(obj);
	ssize_t retval;

	if (ctx->lock) {
		(void) k_mutex_unlock(ctx->lock);
	}

	if (from == NULL || fromlen == NULL) {
		retval = nrf_recvfrom(ctx->nrf_fd, buf, len, flags, NULL, NULL);
	} else {
		/* Allocate space for maximum of IPv4 and IPv6 family type. */
		struct nrf_sockaddr_in6 cliaddr_storage = {0};
		nrf_socklen_t sock_len = sizeof(struct nrf_sockaddr_in6);
		struct nrf_sockaddr *cliaddr = (struct nrf_sockaddr *)&cliaddr_storage;

		retval = nrf_recvfrom(ctx->nrf_fd, buf, len, flags, cliaddr, &sock_len);
		if (retval < 0) {
			goto exit;
		}

		if (cliaddr->sa_family == NRF_AF_INET &&
		    sock_len == sizeof(struct nrf_sockaddr_in)) {
			nrf_to_z_ipv4(from, (struct nrf_sockaddr_in *)cliaddr);
			*fromlen = sizeof(struct sockaddr_in);
		} else if (cliaddr->sa_family == NRF_AF_INET6 &&
			   sock_len == sizeof(struct nrf_sockaddr_in6)) {
			nrf_to_z_ipv6(from, (struct nrf_sockaddr_in6 *)cliaddr);
			*fromlen = sizeof(struct sockaddr_in6);
		}
	}

exit:
	/* Context might have been freed during this call.
	 * Check again before accessing.
	 */
	if (ctx->lock) {
		(void) k_mutex_lock(ctx->lock, K_FOREVER);
	}

	return retval;
}

static ssize_t nrf9x_socket_offload_sendto(void *obj, const void *buf,
					   size_t len, int flags,
					   const struct sockaddr *to,
					   socklen_t tolen)
{
	int sd = OBJ_TO_SD(obj);
	struct nrf_sock_ctx *ctx = OBJ_TO_CTX(obj);
	ssize_t retval;

	if (ctx->lock) {
		(void)k_mutex_unlock(ctx->lock);
	}

	if (to == NULL) {
		retval = nrf_sendto(sd, buf, len, flags, NULL, 0);
	} else if (to->sa_family == AF_INET) {
		struct nrf_sockaddr_in ipv4;
		nrf_socklen_t sock_len = sizeof(struct nrf_sockaddr_in);

		z_to_nrf_ipv4(to, &ipv4);
		retval = nrf_sendto(sd, buf, len, flags, (struct nrf_sockaddr *)&ipv4, sock_len);
	} else if (to->sa_family == AF_INET6) {
		struct nrf_sockaddr_in6 ipv6;
		nrf_socklen_t sock_len = sizeof(struct nrf_sockaddr_in6);

		z_to_nrf_ipv6(to, &ipv6);
		retval = nrf_sendto(sd, buf, len, flags, (struct nrf_sockaddr *)&ipv6, sock_len);
	} else {
		errno = EAFNOSUPPORT;
		retval = -1;
	}

	/* Context might have been freed during this call.
	 * Check again before accessing.
	 */
	if (ctx->lock) {
		(void) k_mutex_lock(ctx->lock, K_FOREVER);
	}

	return retval;
}

static ssize_t nrf9x_socket_offload_sendmsg(void *obj, const struct msghdr *msg,
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
			ret = nrf9x_socket_offload_sendto(obj,
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
			ret = nrf9x_socket_offload_sendto(obj,
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

static void nrf9x_socket_offload_freeaddrinfo(struct zsock_addrinfo *root)
{
	struct zsock_addrinfo *next = root;

	while (next != NULL) {
		struct zsock_addrinfo *this = next;

		next = next->ai_next;
		k_free(this->ai_addr);
		k_free(this);
	}
}

static int nrf9x_socket_offload_getaddrinfo(const char *node,
					    const char *service,
					    const struct zsock_addrinfo *hints,
					    struct zsock_addrinfo **res)
{
	int err;
	struct nrf_addrinfo nrf_hints;
	struct nrf_addrinfo *nrf_res = NULL;
	struct nrf_addrinfo *nrf_hints_ptr = NULL;
	static K_MUTEX_DEFINE(getaddrinfo_lock);

	memset(&nrf_hints, 0, sizeof(struct nrf_addrinfo));

	if (hints != NULL) {
		z_to_nrf_addrinfo_hints(hints, &nrf_hints);
		nrf_hints_ptr = &nrf_hints;
	}

	k_mutex_lock(&getaddrinfo_lock, K_FOREVER);
	int retval = nrf_getaddrinfo(node, service, nrf_hints_ptr, &nrf_res);

	if (retval != 0) {
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

		err = nrf_to_z_addrinfo(next_z_res, next_nrf_res);
		if (err == -ENOMEM) {
			retval = DNS_EAI_MEMORY;
			k_free(next_z_res);
			break;
		} else if (err == -EAFNOSUPPORT) {
			retval = DNS_EAI_FAMILY;
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
		nrf9x_socket_offload_freeaddrinfo(*res);
		*res = NULL;
	}
	nrf_freeaddrinfo(nrf_res);

error:
	k_mutex_unlock(&getaddrinfo_lock);
	return retval;
}

static int nrf9x_socket_offload_fcntl(int fd, int cmd, va_list args)
{
	int retval;
	int flags;

	switch (cmd) {
	case F_SETFL:
		flags = va_arg(args, int);
		if (flags != 0 && flags != O_NONBLOCK) {
			errno = EINVAL;
			retval = -1;
		}

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
		errno = EINVAL;
		retval = -1;
	}

	return retval;
}

static struct nrf_sock_ctx *find_ctx(int fd)
{
	for (size_t i = 0; i < ARRAY_SIZE(offload_ctx); i++) {
		if (offload_ctx[i].nrf_fd == fd) {
			return &offload_ctx[i];
		}
	}

	return NULL;
}

static void pollcb(struct nrf_pollfd *pollfd)
{
	struct nrf_sock_ctx *ctx;

	ctx = find_ctx(pollfd->fd);
	if (!ctx) {
		return;
	}

	k_poll_signal_raise(&ctx->poll, pollfd->revents);
}

static int nrf9x_poll_prepare(struct nrf_sock_ctx *ctx, struct zsock_pollfd *pfd,
			      struct k_poll_event **pev, struct k_poll_event *pev_end)
{
	int err;
	int flags;
	unsigned int signaled;
	int fd = OBJ_TO_SD(ctx);
	struct nrf_modem_pollcb pcb = {
		.callback = pollcb,
		.events = pfd->events,
		.oneshot = true, /* callback invoked only once */
	};

	if (*pev == pev_end) {
		errno = ENOMEM;
		return -1;
	}

	k_poll_signal_init(&ctx->poll);
	k_poll_event_init(*pev, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &ctx->poll);

	err = nrf_setsockopt(fd, NRF_SOL_SOCKET, NRF_SO_POLLCB, &pcb, sizeof(pcb));
	if (err) {
		return -1;
	}

	/* Let other sockets use another k_poll_event */
	(*pev)++;

	signaled = 0;
	flags = 0;

	k_poll_signal_check(&ctx->poll, &signaled, &flags);
	if (!signaled) {
		return 0;
	}

	/* Events are ready, don't wait */
	return -EALREADY;
}

static int nrf9x_poll_update(struct nrf_sock_ctx *ctx, struct zsock_pollfd *pfd,
			     struct k_poll_event **pev)
{
	int flags;
	unsigned int signaled;

	(*pev)++;

	signaled = 0;
	flags = 0;

	k_poll_signal_check(&ctx->poll, &signaled, &flags);
	if (!signaled) {
		return 0;
	}

	pfd->revents = flags;

	return 0;
}

static int nrf9x_socket_offload_ioctl(void *obj, unsigned int request,
				      va_list args)
{
	int sd = OBJ_TO_SD(obj);

	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return nrf9x_poll_prepare(obj, pfd, pev, pev_end);
	}

	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return nrf9x_poll_update(obj, pfd, pev);
	}

	case ZFD_IOCTL_POLL_OFFLOAD:
		return -EOPNOTSUPP;

	case ZFD_IOCTL_SET_LOCK: {
		struct nrf_sock_ctx *ctx = OBJ_TO_CTX(obj);

		ctx->lock = va_arg(args, struct k_mutex *);

		return 0;
	}

	/* Otherwise, just forward to offloaded fcntl()
	 * In Zephyr, fcntl() is just an alias of ioctl().
	 */
	default:
		return nrf9x_socket_offload_fcntl(sd, request, args);
	}
}

static ssize_t nrf9x_socket_offload_read(void *obj, void *buffer, size_t count)
{
	return nrf9x_socket_offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t nrf9x_socket_offload_write(void *obj, const void *buffer,
					  size_t count)
{
	return nrf9x_socket_offload_sendto(obj, buffer, count, 0, NULL, 0);
}

static int nrf9x_socket_offload_close(void *obj)
{
	struct nrf_sock_ctx *ctx = OBJ_TO_CTX(obj);
	int retval;

	retval = nrf_close(ctx->nrf_fd);
	if (retval == 0) {
		release_ctx(ctx);
	}

	return retval;
}

static const struct socket_op_vtable nrf9x_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = nrf9x_socket_offload_read,
		.write = nrf9x_socket_offload_write,
		.close = nrf9x_socket_offload_close,
		.ioctl = nrf9x_socket_offload_ioctl,
	},
	.bind = nrf9x_socket_offload_bind,
	.connect = nrf9x_socket_offload_connect,
	.listen = nrf9x_socket_offload_listen,
	.accept = nrf9x_socket_offload_accept,
	.sendto = nrf9x_socket_offload_sendto,
	.sendmsg = nrf9x_socket_offload_sendmsg,
	.recvfrom = nrf9x_socket_offload_recvfrom,
	.getsockopt = nrf9x_socket_offload_getsockopt,
	.setsockopt = nrf9x_socket_offload_setsockopt,
};

static inline bool proto_is_secure(int proto)
{
	return (proto >= IPPROTO_TLS_1_0 && proto <= IPPROTO_TLS_1_2) ||
	       (proto >= IPPROTO_DTLS_1_0 && proto <= IPPROTO_DTLS_1_2);
}

static inline bool af_is_supported(int family)
{
	return (family == AF_PACKET) || (family == AF_INET) || (family == AF_INET6);
}

static bool nrf9x_socket_is_supported(int family, int type, int proto)
{
	if (offload_disabled) {
		return false;
	}

	if (tls_offload_disabled && proto_is_secure(proto)) {
		return false;
	}

	return af_is_supported(family);
}

static int native_socket(int family, int type, int proto, bool *offload_lock)
{
	int sock;

	type = type & ~(SOCK_NATIVE | SOCK_NATIVE_TLS);

	/* Lock the scheduler for the time of a socket creation to prevent
	 * race condition when offloading is disabled.
	 */
	k_sched_lock();
	*offload_lock = true;
	sock = zsock_socket(family, type, proto);
	*offload_lock = false;
	k_sched_unlock();

	return sock;
}

static int nrf9x_socket_create(int family, int type, int proto)
{
	int fd, sd;
	struct nrf_sock_ctx *ctx;

	if (type & SOCK_NATIVE) {
		return native_socket(family, type, proto, &offload_disabled);
	} else if (type & SOCK_NATIVE_TLS) {
		return native_socket(family, type, proto, &tls_offload_disabled);
	}

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		return -1;
	}

	sd = nrf9x_socket_offload_socket(family, type, proto);
	if (sd < 0) {
		zvfs_free_fd(fd);
		return -1;
	}

	ctx = allocate_ctx(sd);
	if (ctx == NULL) {
		errno = ENOMEM;
		nrf_close(sd);
		zvfs_free_fd(fd);
		return -1;
	}

	zvfs_finalize_fd(fd, ctx,
		      (const struct fd_op_vtable *)&nrf9x_socket_fd_op_vtable);

	return fd;
}

#define NRF9X_SOCKET_PRIORITY 40

NET_SOCKET_OFFLOAD_REGISTER(nrf9x_socket, NRF9X_SOCKET_PRIORITY, AF_UNSPEC,
		    nrf9x_socket_is_supported, nrf9x_socket_create);

/* Create a network interface for nRF9x */

static int nrf9x_socket_offload_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	for (int i = 0; i < ARRAY_SIZE(offload_ctx); i++) {
		offload_ctx[i].nrf_fd = -1;
	}

	return 0;
}

static const struct socket_dns_offload nrf9x_socket_dns_offload_ops = {
	.getaddrinfo = nrf9x_socket_offload_getaddrinfo,
	.freeaddrinfo = nrf9x_socket_offload_freeaddrinfo,
};

static struct nrf9x_iface_data {
	struct net_if *iface;
} nrf9x_iface_data;

static void nrf9x_iface_api_init(struct net_if *iface)
{
	nrf9x_iface_data.iface = iface;

	iface->if_dev->socket_offload = nrf9x_socket_create;

	socket_offload_dns_register(&nrf9x_socket_dns_offload_ops);

	if (!IS_ENABLED(CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_START)) {
		net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	}
}

static int nrf9x_iface_enable(const struct net_if *iface, bool enabled)
{
#if defined(CONFIG_NRF_MODEM_LIB_NET_IF)
	/* Enables or disable the device (in response to admin state change) */
	extern int lte_net_if_enable(void);
	extern int lte_net_if_disable(void);

	return enabled ? lte_net_if_enable() :
			 lte_net_if_disable();
#else
	ARG_UNUSED(iface);
	ARG_UNUSED(enabled);
	return 0;
#endif /* CONFIG_NRF_MODEM_LIB_NET_IF */
}

static struct offloaded_if_api nrf9x_iface_offload_api = {
	.iface_api.init = nrf9x_iface_api_init,
	.enable = nrf9x_iface_enable,
};

/* TODO Get the actual MTU for the nRF9x LTE link. */
NET_DEVICE_OFFLOAD_INIT(nrf9x_socket, "nrf9x_socket",
			nrf9x_socket_offload_init,
			NULL,
			&nrf9x_iface_data, NULL,
			0, &nrf9x_iface_offload_api, 1280);

#if defined(CONFIG_NRF_MODEM_LIB_NET_IF)
extern struct conn_mgr_conn_api lte_net_if_conn_mgr_api;
CONN_MGR_CONN_DEFINE(NRF_MODEM_LIB_NET_IF, &lte_net_if_conn_mgr_api);
CONN_MGR_BIND_CONN(nrf9x_socket, NRF_MODEM_LIB_NET_IF);
#endif /* CONFIG_NRF_MODEM_LIB_NET_IF */

#endif /* CONFIG_NET_SOCKETS_OFFLOAD */
