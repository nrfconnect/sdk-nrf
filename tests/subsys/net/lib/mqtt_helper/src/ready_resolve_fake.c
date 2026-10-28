/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Minimal ready_resolve: always ready, resolves through the mocked getaddrinfo() */

#include <errno.h>
#include <zephyr/net/socket.h>
#include <net/ready_resolve.h>

#define FAMILY_V4 BIT(0)
#define FAMILY_V6 BIT(1)

static uint8_t family_bit(int family)
{
	return family == NET_AF_INET ? FAMILY_V4 : family == NET_AF_INET6 ? FAMILY_V6 : 0;
}

int ready_resolve(struct ready_resolve *ctx, const char *host, const char *service,
		  const struct zsock_addrinfo *hints, uint16_t port)
{
	ready_resolve_free(ctx);

	if (zsock_getaddrinfo(host, service, hints, &ctx->res)) {
		ctx->res = NULL;
		return -EHOSTUNREACH;
	}

	ctx->next = ctx->res;
	ctx->port = port;
	/* Wanted families that are enabled */
	ctx->mask = (IS_ENABLED(CONFIG_NET_IPV4) ? FAMILY_V4 : 0) |
		    (IS_ENABLED(CONFIG_NET_IPV6) ? FAMILY_V6 : 0);
	if (hints != NULL && hints->ai_family != NET_AF_UNSPEC) {
		ctx->mask &= family_bit(hints->ai_family);
	}

	return 0;
}

int ready_resolve_next(struct ready_resolve *ctx, const struct net_sockaddr **addr,
		       net_socklen_t *addrlen)
{
	struct zsock_addrinfo *ai = ctx->next;

	while (ai != NULL && !(ctx->mask & family_bit(ai->ai_family))) {
		ai = ai->ai_next;
	}

	if (ai == NULL) {
		ctx->next = NULL;
		return -ENOENT;
	}

	ctx->next = ai->ai_next;
	if (ai->ai_family == NET_AF_INET6) {
		net_sin6(ai->ai_addr)->sin6_port = net_htons(ctx->port);
	} else {
		net_sin(ai->ai_addr)->sin_port = net_htons(ctx->port);
	}

	*addr = ai->ai_addr;
	*addrlen = ai->ai_addrlen;

	return 0;
}

void ready_resolve_free(struct ready_resolve *ctx)
{
	if (ctx->res != NULL) {
		zsock_freeaddrinfo(ctx->res);
	}

	ctx->res = NULL;
	ctx->next = NULL;
}

int ready_resolve_wait_change(const struct ready_resolve *ctx, k_timeout_t timeout)
{
	k_sleep(timeout);

	return -EAGAIN;
}
