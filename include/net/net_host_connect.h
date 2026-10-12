/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NET_HOST_CONNECT_H_
#define NET_HOST_CONNECT_H_

/**
 * @file net_host_connect.h
 * @defgroup net_host_connect Host connection helper
 * @{
 * @brief Resolve a host name and connect to it, robust to dual-stack connectivity changes.
 *
 * The library waits until IP connectivity and name resolution are available, resolves the
 * host, and tries the returned addresses in order until one connects. When all addresses
 * fail, it waits for a connectivity change or a retry delay, then starts over.
 * Connectivity is tracked through connection manager events.
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
typedef int (*net_host_connect_cb_t)(const struct net_sockaddr *addr, net_socklen_t addrlen,
				     void *user_data);

struct net_host_connect;

/**
 * @brief Callback notifying that connectivity of the current connection is lost.
 *
 * Called when the address family of the current connection goes down, typically from the
 * network management thread. Must not block.
 *
 * @param ctx Context.
 * @param user_data User data from the configuration.
 */
typedef void (*net_host_connect_lost_cb_t)(struct net_host_connect *ctx, void *user_data);

/** @brief Host connection configuration. */
struct net_host_connect_cfg {
	/** Host name or numeric address. */
	const char *host;
	/** Port, in host byte order. */
	uint16_t port;
	/** Socket type passed to getaddrinfo(), for example NET_SOCK_DGRAM. */
	int socktype;
	/** Connection callback, called for each address tried. */
	net_host_connect_cb_t connect_cb;
	/** Optional connectivity loss callback. */
	net_host_connect_lost_cb_t lost_cb;
	/** User data passed to the callbacks. */
	void *user_data;
	/** Delay before resolving again when all addresses failed. */
	k_timeout_t retry_delay;
};

/** @brief Host connection context. Fields are internal. */
struct net_host_connect {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	const struct net_host_connect_cfg *cfg;
	struct net_sockaddr_storage addrs[CONFIG_NET_HOST_CONNECT_MAX_ADDRS];
	uint8_t addr_count;
	uint8_t addr_next;
	int peer;
	uint32_t generation;
	/** @endcond */
};

/**
 * @brief Initialize a host connection context.
 *
 * @param ctx Context to initialize.
 * @param cfg Configuration. Must remain valid for the lifetime of the context.
 */
void net_host_connect_init(struct net_host_connect *ctx, const struct net_host_connect_cfg *cfg);

/**
 * @brief Deinitialize a host connection context.
 *
 * Call when done with the context, before its memory goes out of scope or is reused, and
 * before initializing it again.
 *
 * @param ctx Context to deinitialize.
 */
void net_host_connect_deinit(struct net_host_connect *ctx);

/**
 * @brief Connect to the host.
 *
 * Blocks until connectivity is available, resolves the host if needed, and tries addresses
 * until the connection callback succeeds. If @ref net_host_connect_failed was called, the
 * next call starts from the following address. The host is resolved again when
 * connectivity changes or all addresses have been tried.
 *
 * @param ctx Context.
 * @param timeout Maximum time to wait.
 *
 * @return Socket descriptor returned by the connection callback on success.
 * @retval -EAGAIN Timeout expired.
 * @retval -EINVAL Invalid context.
 */
int net_host_connect(struct net_host_connect *ctx, k_timeout_t timeout);

/**
 * @brief Report that the current connection is not usable.
 *
 * Call after an application-level failure, for example a request timeout. The next call to
 * @ref net_host_connect tries the next address. The caller closes the socket.
 *
 * @param ctx Context.
 */
void net_host_connect_failed(struct net_host_connect *ctx);

/**
 * @brief Get the address of the current connection.
 *
 * @param ctx Context.
 * @param addrlen Set to the address length if not NULL.
 *
 * @return Peer address, or NULL if not connected.
 */
const struct net_sockaddr *net_host_connect_get_peer(const struct net_host_connect *ctx,
						     net_socklen_t *addrlen);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NET_HOST_CONNECT_H_ */
