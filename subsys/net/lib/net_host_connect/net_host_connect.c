/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <net/net_host_connect.h>

LOG_MODULE_REGISTER(net_host_connect, CONFIG_NET_HOST_CONNECT_LOG_LEVEL);

#define STATE_IPV4 BIT(0)
#define STATE_IPV6 BIT(1)
#define STATE_DNS  BIT(2)
#define STATE_IP   (STATE_IPV4 | STATE_IPV6)

#define EVENT_MASK (NET_EVENT_L4_IPV4_CONNECTED | NET_EVENT_L4_IPV4_DISCONNECTED | \
		    NET_EVENT_L4_IPV6_CONNECTED | NET_EVENT_L4_IPV6_DISCONNECTED | \
		    NET_EVENT_L4_DNS_READY | NET_EVENT_L4_DNS_LOST)

/* Connectivity state shared by all contexts */
static uint32_t state;
/* A generation we bump on every state change */
static uint32_t generation;
static K_MUTEX_DEFINE(state_lock);
static K_CONDVAR_DEFINE(state_changed);
/* Contexts with a lost callback, protected by state_lock */
static sys_slist_t contexts = SYS_SLIST_STATIC_INIT(&contexts);

static uint32_t family_state(int family)
{
	switch (family) {
	case NET_AF_INET:
		return STATE_IPV4;
	case NET_AF_INET6:
		return STATE_IPV6;
	default:
		return 0;
	}
}

/* Called with state_lock held */
static void notify_lost_locked(uint32_t lost)
{
	struct net_host_connect *ctx;

	SYS_SLIST_FOR_EACH_CONTAINER(&contexts, ctx, node) {
		if (ctx->peer >= 0 && (family_state(ctx->addrs[ctx->peer].ss_family) & lost)) {
			ctx->cfg->lost_cb(ctx, ctx->cfg->user_data);
		}
	}
}

static void event_handler(uint64_t event, struct net_if *iface, void *info, size_t info_length,
			  void *user_data)
{
	uint32_t new_state;

	k_mutex_lock(&state_lock, K_FOREVER);
	new_state = state;

	switch (event) {
	case NET_EVENT_L4_IPV4_CONNECTED:
		new_state |= STATE_IPV4;
		break;
	case NET_EVENT_L4_IPV4_DISCONNECTED:
		new_state &= ~STATE_IPV4;
		break;
	case NET_EVENT_L4_IPV6_CONNECTED:
		new_state |= STATE_IPV6;
		break;
	case NET_EVENT_L4_IPV6_DISCONNECTED:
		new_state &= ~STATE_IPV6;
		break;
	case NET_EVENT_L4_DNS_READY:
		new_state |= STATE_DNS;
		break;
	case NET_EVENT_L4_DNS_LOST:
		new_state &= ~STATE_DNS;
		break;
	default:
		break;
	}

	if (new_state != state) {
		LOG_DBG("IPv4 %d, IPv6 %d, DNS %d", !!(new_state & STATE_IPV4),
			!!(new_state & STATE_IPV6), !!(new_state & STATE_DNS));
		notify_lost_locked(state & ~new_state);
		state = new_state;
		generation++;
		k_condvar_broadcast(&state_changed);
	}

	k_mutex_unlock(&state_lock);
}

NET_MGMT_REGISTER_EVENT_HANDLER(net_host_connect_events, EVENT_MASK, event_handler, NULL);

static bool is_numeric(const char *host)
{
	uint8_t buf[sizeof(struct net_in6_addr)];

	return zsock_inet_pton(NET_AF_INET, host, buf) == 1 ||
	       zsock_inet_pton(NET_AF_INET6, host, buf) == 1;
}

/* True if is IP ready + for non-numeric hosts, DNS is also available */
static bool is_ready(uint32_t s, bool numeric)
{
	return (s & STATE_IP) && (numeric || (s & STATE_DNS));
}

/* Returns when the host (IP + DNS if needed) is ready or deadline is reached.
 * Called with state_lock held. Returns -EAGAIN on timeout.
 */
static int wait_ready_locked(bool numeric, k_timepoint_t deadline)
{
	while (!is_ready(state, numeric)) {
		if (k_condvar_wait(&state_changed, &state_lock, sys_timepoint_timeout(deadline))) {
			return -EAGAIN;
		}
	}

	return 0;
}

/* Called with state_lock held. Waits for a generation change, delay, or deadline. */
static void wait_change_locked(uint32_t gen, k_timeout_t delay, k_timepoint_t deadline)
{
	k_timepoint_t until = sys_timepoint_calc(delay);

	if (sys_timepoint_cmp(deadline, until) < 0) {
		until = deadline;
	}

	while (generation == gen) {
		if (k_condvar_wait(&state_changed, &state_lock, sys_timepoint_timeout(until))) {
			return;
		}
	}
}

static net_socklen_t addr_len(const struct net_sockaddr *sa)
{
	return (sa->sa_family == NET_AF_INET6) ? sizeof(struct net_sockaddr_in6) :
						 sizeof(struct net_sockaddr_in);
}

static const char *addr_str(const struct net_sockaddr *sa, char *buf, size_t len)
{
	const void *addr = (sa->sa_family == NET_AF_INET6) ?
			   (const void *)&net_sin6(sa)->sin6_addr :
			   (const void *)&net_sin(sa)->sin_addr;

	return zsock_inet_ntop(sa->sa_family, addr, buf, len);
}

static int resolve(struct net_host_connect *ctx, uint32_t s)
{
	const struct net_host_connect_cfg *cfg = ctx->cfg;
	struct zsock_addrinfo *res;
	struct zsock_addrinfo hints = {
		.ai_family = NET_AF_UNSPEC,
		.ai_socktype = cfg->socktype,
	};
	int err;

	/* If only one family is up, only query that one: avoids filling addrs with
	 * unreachable addresses and saves a DNS round trip.
	 */
	if ((s & STATE_IP) == STATE_IPV4) {
		hints.ai_family = NET_AF_INET;
	} else if ((s & STATE_IP) == STATE_IPV6) {
		hints.ai_family = NET_AF_INET6;
	}

	ctx->addr_count = 0;
	ctx->addr_next = 0;

	err = zsock_getaddrinfo(cfg->host, NULL, &hints, &res);
	if (err) {
		LOG_WRN("Failed to resolve %s: %d", cfg->host, err);
		return -EHOSTUNREACH;
	}

	/* Copy supported results, in the order we got from getaddrinfo() */
	for (struct zsock_addrinfo *ai = res;
	     ai != NULL && ctx->addr_count < ARRAY_SIZE(ctx->addrs); ai = ai->ai_next) {
		/* Next free slot */
		struct net_sockaddr *sa = (struct net_sockaddr *)&ctx->addrs[ctx->addr_count];

		if (IS_ENABLED(CONFIG_NET_IPV6) && ai->ai_family == NET_AF_INET6) {
			memcpy(sa, ai->ai_addr, sizeof(struct net_sockaddr_in6));
			net_sin6(sa)->sin6_port = net_htons(cfg->port);
		} else if (IS_ENABLED(CONFIG_NET_IPV4) && ai->ai_family == NET_AF_INET) {
			memcpy(sa, ai->ai_addr, sizeof(struct net_sockaddr_in));
			net_sin(sa)->sin_port = net_htons(cfg->port);
		} else {
			/* Unsupported or disabled family: skip, reuse the slot. */
			continue;
		}

		ctx->addr_count++;
	}

	zsock_freeaddrinfo(res);

	LOG_DBG("Resolved %s: %u address(es)", cfg->host, ctx->addr_count);

	return ctx->addr_count > 0 ? 0 : -EHOSTUNREACH;
}

/* Tries addresses from addr_next on. Returns a socket, or -ENOTCONN if all failed. */
static int try_addresses(struct net_host_connect *ctx)
{
	const struct net_host_connect_cfg *cfg = ctx->cfg;

	for (; ctx->addr_next < ctx->addr_count; ctx->addr_next++) {
		const struct net_sockaddr *sa =
			(const struct net_sockaddr *)&ctx->addrs[ctx->addr_next];
		char ip[NET_IPV6_ADDR_LEN] = "";
		uint32_t s;
		int fd;

		k_mutex_lock(&state_lock, K_FOREVER);
		s = state;
		k_mutex_unlock(&state_lock);

		if (!(s & family_state(sa->sa_family))) {
			continue;
		}

		addr_str(sa, ip, sizeof(ip));
		LOG_INF("Connecting to %s (%s), port %u", cfg->host, ip, cfg->port);

		fd = cfg->connect_cb(sa, addr_len(sa), cfg->user_data);
		if (fd >= 0) {
			bool lost;

			k_mutex_lock(&state_lock, K_FOREVER);
			ctx->peer = ctx->addr_next;
			lost = !(state & family_state(sa->sa_family));
			k_mutex_unlock(&state_lock);

			/* Family went down while connecting */
			if (lost && cfg->lost_cb != NULL) {
				cfg->lost_cb(ctx, cfg->user_data);
			}

			return fd;
		}

		LOG_WRN("Failed to connect to %s: %d", ip, fd);
	}

	return -ENOTCONN;
}

void net_host_connect_init(struct net_host_connect *ctx, const struct net_host_connect_cfg *cfg)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->cfg = cfg;
	ctx->peer = -1;

	if (cfg->lost_cb != NULL) {
		k_mutex_lock(&state_lock, K_FOREVER);
		sys_slist_append(&contexts, &ctx->node);
		k_mutex_unlock(&state_lock);
	}
}

void net_host_connect_deinit(struct net_host_connect *ctx)
{
	k_mutex_lock(&state_lock, K_FOREVER);
	sys_slist_find_and_remove(&contexts, &ctx->node);
	k_mutex_unlock(&state_lock);
}

static void set_peer(struct net_host_connect *ctx, int peer)
{
	k_mutex_lock(&state_lock, K_FOREVER);
	ctx->peer = peer;
	k_mutex_unlock(&state_lock);
}

int net_host_connect(struct net_host_connect *ctx, k_timeout_t timeout)
{
	k_timepoint_t deadline = sys_timepoint_calc(timeout);
	bool backoff = false;
	bool exhausted;
	bool numeric;
	uint32_t gen;
	uint32_t s;
	int ret;

	if (ctx == NULL || ctx->cfg == NULL || ctx->cfg->host == NULL ||
	    ctx->cfg->connect_cb == NULL) {
		return -EINVAL;
	}

	numeric = is_numeric(ctx->cfg->host);
	set_peer(ctx, -1);

	while (true) {
		k_mutex_lock(&state_lock, K_FOREVER);

		/* Wait for IP, and DNS if host is a name */
		ret = wait_ready_locked(numeric, deadline);
		if (ret) {
			k_mutex_unlock(&state_lock);
			return ret;
		}

		gen = generation;
		s = state;
		/* All resolved addresses already tried */
		exhausted = ctx->addr_count > 0 && ctx->addr_next >= ctx->addr_count;

		/* Nothing left to try and connectivity unchanged: back off */
		if ((backoff || exhausted) && gen == ctx->generation) {
			LOG_INF("No usable address for %s, retrying", ctx->cfg->host);
			wait_change_locked(gen, ctx->cfg->retry_delay, deadline);
			k_mutex_unlock(&state_lock);

			/* Force a fresh resolve on the next round */
			ctx->addr_count = 0;
			backoff = false;

			if (sys_timepoint_expired(deadline)) {
				return -EAGAIN;
			}

			continue;
		}

		k_mutex_unlock(&state_lock);

		/* (Re)resolve if no addresses, all tried, or connectivity changed */
		if (ctx->addr_count == 0 || exhausted || gen != ctx->generation) {
			ctx->generation = gen;
			if (resolve(ctx, s)) {
				backoff = true;
				continue;
			}
		}

		ret = try_addresses(ctx);
		if (ret >= 0) {
			return ret;
		}
		/* All failed: next round backs off, or re-resolves if connectivity changed */
	}
}

void net_host_connect_failed(struct net_host_connect *ctx)
{
	if (ctx->peer >= 0) {
		ctx->addr_next = ctx->peer + 1;
		set_peer(ctx, -1);
	}
}

const struct net_sockaddr *net_host_connect_get_peer(const struct net_host_connect *ctx,
						     net_socklen_t *addrlen)
{
	const struct net_sockaddr *sa;

	if (ctx->peer < 0) {
		return NULL;
	}

	sa = (const struct net_sockaddr *)&ctx->addrs[ctx->peer];
	if (addrlen != NULL) {
		*addrlen = addr_len(sa);
	}

	return sa;
}
