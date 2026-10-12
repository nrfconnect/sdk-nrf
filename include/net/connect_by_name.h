/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CONNECT_BY_NAME_H_
#define CONNECT_BY_NAME_H_

/**
 * @file connect_by_name.h
 * @defgroup connect_by_name Host connection helper
 * @{
 * @brief Resolve a host name and connect to it, robust to dual-stack connectivity changes.
 *
 * The library resolves the host for the address families that are up and tries the returned
 * addresses in order until one connects. It does not block on connectivity: the caller
 * retries, using @ref connect_by_name_wait_change to react to connectivity changes:
 *
 * @code
 * while ((fd = connect_by_name(&ctx)) < 0 && !stop) {
 *	connect_by_name_wait_change(&ctx, K_SECONDS(10));
 * }
 * @endcode
 *
 * Connectivity is tracked through connection manager events, see
 * @kconfig{CONFIG_CONNECT_BY_NAME_READINESS}. On nRF91 Series devices without a modem network
 * interface, the modem is queried instead, see @kconfig{CONFIG_CONNECT_BY_NAME_MODEM_READINESS}.
 * Otherwise, the network is assumed ready.
 *
 * A context must be used from a single thread. Contexts are independent of each other.
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback that creates, configures and connects a socket to the given address.
 *
 * @param addr Address to connect to, including the port.
 * @param addrlen Length of @p addr.
 * @param user_data User data from the configuration.
 *
 * @return Connected socket descriptor on success, negative error code otherwise.
 *	   On failure, the callback must close any socket it created.
 */
typedef int (*connect_by_name_cb_t)(const struct net_sockaddr *addr, net_socklen_t addrlen,
				     void *user_data);

struct connect_by_name;

/**
 * @brief Callback notifying that connectivity of the current connection is lost.
 *
 * Called when the address family of the current connection goes down, from the network
 * management thread, or from @ref connect_by_name if it went down while connecting.
 * Must not block.
 *
 * @param ctx Context.
 * @param user_data User data from the configuration.
 */
typedef void (*connect_by_name_lost_cb_t)(struct connect_by_name *ctx, void *user_data);

/** @brief Host connection configuration. */
struct connect_by_name_cfg {
	/** Host name or numeric address. */
	const char *host;
	/** Port, in host byte order. */
	uint16_t port;
	/** Socket type passed to getaddrinfo(), for example NET_SOCK_DGRAM. */
	int socktype;
	/** Address family to use, @c NET_AF_INET or @c NET_AF_INET6. @c NET_AF_UNSPEC for any. */
	int family;
	/** Optional service passed to getaddrinfo(). The port is always set from @c port. */
	const char *service;
	/** Flags passed to getaddrinfo() hints. */
	int ai_flags;
	/** Connection callback, called for each address tried. */
	connect_by_name_cb_t connect_cb;
	/** Optional connectivity loss callback. */
	connect_by_name_lost_cb_t lost_cb;
	/** User data passed to the callbacks. */
	void *user_data;
};

/** @brief Host connection context. Fields are internal. */
struct connect_by_name {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	const struct connect_by_name_cfg *cfg;
	struct net_sockaddr_storage addrs[CONFIG_CONNECT_BY_NAME_MAX_ADDRS];
	uint8_t addr_count;
	uint8_t addr_next;
	int peer;
	/* State bits relevant to this context, last seen, and last resolved for */
	uint8_t mask;
	uint8_t seen;
	uint8_t resolved;
	/** @endcond */
};

/**
 * @brief Initialize a host connection context.
 *
 * @param ctx Context to initialize.
 * @param cfg Configuration. Must remain valid for the lifetime of the context.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid configuration.
 */
int connect_by_name_init(struct connect_by_name *ctx, const struct connect_by_name_cfg *cfg);

/**
 * @brief Deinitialize a host connection context.
 *
 * Call when done with the context, before its memory goes out of scope or is reused, and
 * before initializing it again.
 *
 * @param ctx Context to deinitialize.
 */
void connect_by_name_deinit(struct connect_by_name *ctx);

/**
 * @brief Connect to the host.
 *
 * Makes one round of attempts, without waiting for connectivity: resolves the host if
 * needed, then tries addresses until the connection callback succeeds. If
 * @ref connect_by_name_failed was called, starts from the following address. The host is
 * resolved again when the address families up changed or all addresses have been tried.
 *
 * @param ctx Context.
 *
 * @return Socket descriptor returned by the connection callback on success.
 * @retval -ENETDOWN Network not ready: no wanted address family up, or no DNS for a name.
 * @retval -EHOSTUNREACH The host could not be resolved to a usable address.
 * @return Other negative error code: last error from the connection callback.
 */
int connect_by_name(struct connect_by_name *ctx);

/**
 * @brief Wait for a connectivity change since the last call to @ref connect_by_name.
 *
 * Only changes relevant to the context count: its address families, and DNS for host names.
 * Returns immediately if connectivity already changed. With
 * @kconfig{CONFIG_CONNECT_BY_NAME_MODEM_READINESS}, the modem is polled. Without any readiness
 * tracking, waits for the full timeout.
 *
 * @param ctx Context.
 * @param timeout Maximum time to wait, typically the retry delay.
 */
void connect_by_name_wait_change(struct connect_by_name *ctx, k_timeout_t timeout);

/**
 * @brief Report that the current connection is not usable.
 *
 * Call after an application-level failure, for example a request timeout. The next call to
 * @ref connect_by_name tries the next address. The caller closes the socket.
 *
 * @param ctx Context.
 */
void connect_by_name_failed(struct connect_by_name *ctx);

/**
 * @brief Get the address of the current connection.
 *
 * @param ctx Context.
 * @param addrlen Set to the address length if not NULL.
 *
 * @return Peer address, or NULL if not connected.
 */
const struct net_sockaddr *connect_by_name_get_peer(const struct connect_by_name *ctx,
						     net_socklen_t *addrlen);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* CONNECT_BY_NAME_H_ */
