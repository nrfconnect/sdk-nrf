/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef READY_RESOLVE_H_
#define READY_RESOLVE_H_

/**
 * @file ready_resolve.h
 * @defgroup ready_resolve Ready resolver
 * @{
 * @brief Dual-stack, connectivity-aware host name resolution.
 *
 * Resolves a host name for the address families that are up and with DNS ready.
 * Returns an iterator over the resolved addresses. The caller is expected to
 * attempt connection over the resolved addresses, and between rounds of attempts
 * waits for connectivity with @ref ready_resolve_wait_change, as follows:
 *
 * @code
 * struct ready_resolve host = {0};
 * const struct zsock_addrinfo hints = { .ai_socktype = NET_SOCK_STREAM };
 * int fd = -1;
 *
 * while (fd < 0 && !stop) {
 *	int err = ready_resolve(&host, "example.com", NULL, &hints, 443);
 *
 *	while (err == 0 && fd < 0 && ready_resolve_next(&host, &addr, &addrlen) == 0) {
 *		fd = server_connect(addr, addrlen);
 *	}
 *	if (fd < 0) {
 *		ready_resolve_wait_change(&host, K_SECONDS(10));
 *	}
 * }
 * ready_resolve_free(&host);
 * @endcode
 *
 * Connectivity is tracked through connection manager events, or on nRF91 Series devices without
 * a modem network interface by querying the modem.
 *
 * A context is zero-initialized before first use and used from a single thread. It holds the
 * resolved addresses until @ref ready_resolve_free is called.
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Ready resolver context. Zero-initialize before first use. Fields are internal. */
struct ready_resolve {
	/** @cond INTERNAL_HIDDEN */
	struct zsock_addrinfo *res;
	struct zsock_addrinfo *next;
	/* Sum of the gain counters of mask at the last resolve */
	uint32_t gains;
	uint16_t port;
	/* State bits the context depends on */
	uint8_t mask;
	/* Readiness not tracked, for a non-default PDN */
	bool untracked;
	/** @endcond */
};

/**
 * @brief Resolve a host for the address families that are up.
 *
 * Frees the previous result, if any. Only the wanted address families that are up are queried.
 * Returns -ENETDOWN immediately if none is up, or if no DNS server is available for a host
 * name. Can block in getaddrinfo() up to the DNS timeout.
 *
 * Only the default PDN is tracked. With @c AI_PDNSERV in @p hints, readiness is not checked
 * and errors come from getaddrinfo() or connect().
 *
 * @param ctx Context.
 * @param host Host name or numeric address. Not used after the call returns.
 * @param service Service passed to getaddrinfo(), or NULL.
 * @param hints Hints passed to getaddrinfo(), or NULL. @c ai_family selects the wanted family,
 *		@c NET_AF_UNSPEC for any.
 * @param port Port set on the returned addresses, in host byte order. 0 to keep the port
 *	       resolved from @p service.
 *
 * @retval 0 Success. Call @ref ready_resolve_free when done.
 * @retval -ENETDOWN Network not ready.
 * @retval -EAGAIN Temporary DNS failure, for example no response from the server.
 * @retval -EHOSTUNREACH The host could not be resolved to a usable address.
 * @retval -ENOMEM Out of memory.
 * @retval -EINVAL Invalid argument.
 */
int ready_resolve(struct ready_resolve *ctx, const char *host, const char *service,
		  const struct zsock_addrinfo *hints, uint16_t port);

/**
 * @brief Get the next resolved address.
 *
 * Addresses are returned in getaddrinfo() order, skipping addresses of families that are not
 * wanted or are down. The address stays valid until @ref ready_resolve_free or the next
 * @ref ready_resolve.
 *
 * @param ctx Context.
 * @param[out] addr Next address, including the port.
 * @param[out] addrlen Length of @p addr.
 *
 * @retval 0 Success.
 * @retval -ENOENT No more addresses. Resolve again to start over.
 * @retval -EINVAL Invalid argument.
 */
int ready_resolve_next(struct ready_resolve *ctx, const struct net_sockaddr **addr,
		       net_socklen_t *addrlen);

/**
 * @brief Free the resolved addresses.
 *
 * Can be called on a zero-initialized or already freed context.
 *
 * @param ctx Context.
 */
void ready_resolve_free(struct ready_resolve *ctx);

/**
 * @brief Wait for connectivity to be gained since the last @ref ready_resolve.
 *
 * Returns when an address family or DNS relevant to the context comes up, including one that
 * went down and came back. A loss alone does not wake the call. Returns immediately if this
 * already happened. With @kconfig{CONFIG_READY_RESOLVE_TRACK_MODEM}, the modem is polled.
 * If @ref ready_resolve was never called or failed with -EINVAL, waits for the full timeout.
 *
 * @param ctx Context.
 * @param timeout Maximum time to wait, typically the retry delay.
 *
 * @retval 0 Connectivity was gained.
 * @retval -EAGAIN Timeout.
 */
int ready_resolve_wait_change(const struct ready_resolve *ctx, k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* READY_RESOLVE_H_ */
