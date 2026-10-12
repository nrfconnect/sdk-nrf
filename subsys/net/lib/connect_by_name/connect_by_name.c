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
#include <net/connect_by_name.h>

#if defined(CONFIG_CONNECT_BY_NAME_MODEM_READINESS)
#include <nrf_modem_at.h>
#endif

LOG_MODULE_REGISTER(connect_by_name, CONFIG_CONNECT_BY_NAME_LOG_LEVEL);

#define STATE_IPV4 BIT(0)
#define STATE_IPV6 BIT(1)
#define STATE_DNS  BIT(2)
#define STATE_IP   (STATE_IPV4 | STATE_IPV6)

#define EVENT_MASK (NET_EVENT_L4_IPV4_CONNECTED | NET_EVENT_L4_IPV4_DISCONNECTED | \
		    NET_EVENT_L4_IPV6_CONNECTED | NET_EVENT_L4_IPV6_DISCONNECTED | \
		    NET_EVENT_L4_DNS_READY | NET_EVENT_L4_DNS_LOST)

/* Connectivity state shared by all contexts. Without readiness tracking, always ready. */
static uint32_t state = IS_ENABLED(CONFIG_CONNECT_BY_NAME_READINESS) ? 0 :
			(STATE_IP | STATE_DNS);
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

#if defined(CONFIG_CONNECT_BY_NAME_READINESS)
/* Called with state_lock held */
static void notify_lost_locked(uint32_t lost)
{
	struct connect_by_name *ctx;

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
		k_condvar_broadcast(&state_changed);
	}

	k_mutex_unlock(&state_lock);
}

NET_MGMT_REGISTER_EVENT_HANDLER(connect_by_name_events, EVENT_MASK, event_handler, NULL);
#endif /* CONFIG_CONNECT_BY_NAME_READINESS */

#if defined(CONFIG_CONNECT_BY_NAME_MODEM_READINESS)
/* The modem resolves names itself, so DNS is ready when an IP family is up */
static uint32_t modem_state(void)
{
	char addr1[NET_IPV6_ADDR_LEN] = {0};
	char addr2[NET_IPV6_ADDR_LEN] = {0};
	uint8_t buf[sizeof(struct net_in6_addr)];
	uint32_t up = 0;

	/* The IPv4 address, if any, comes first */
	if (nrf_modem_at_scanf("AT+CGPADDR=0", "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"",
			       addr1, addr2) <= 0) {
		return 0;
	}

	if (zsock_inet_pton(NET_AF_INET, addr1, buf) == 1) {
		up |= STATE_IPV4;
	}
	if (zsock_inet_pton(NET_AF_INET6, addr1, buf) == 1 ||
	    zsock_inet_pton(NET_AF_INET6, addr2, buf) == 1) {
		up |= STATE_IPV6;
	}

	return up ? (up | STATE_DNS) : 0;
}

static void update_state(void)
{
	uint32_t new_state = modem_state();

	k_mutex_lock(&state_lock, K_FOREVER);
	state = new_state;
	k_mutex_unlock(&state_lock);
}
#else
static void update_state(void)
{
}
#endif /* CONFIG_CONNECT_BY_NAME_MODEM_READINESS */

/* IP state bits of the families the context may use */
static uint32_t wanted_ip(const struct connect_by_name_cfg *cfg)
{
	return cfg->family == NET_AF_UNSPEC ? STATE_IP : family_state(cfg->family);
}

static bool is_numeric(const char *host)
{
	uint8_t buf[sizeof(struct net_in6_addr)];

	return zsock_inet_pton(NET_AF_INET, host, buf) == 1 ||
	       zsock_inet_pton(NET_AF_INET6, host, buf) == 1;
}

/* True if a wanted family is up and, if needed, DNS is available */
static bool is_ready(uint32_t up, uint32_t mask)
{
	return (up & STATE_IP) && ((up & STATE_DNS) || !(mask & STATE_DNS));
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

/* Resolves the host for the families that are up */
static int resolve(struct connect_by_name *ctx, uint32_t up)
{
	const struct connect_by_name_cfg *cfg = ctx->cfg;
	struct zsock_addrinfo *res;
	struct zsock_addrinfo hints = {
		.ai_family = NET_AF_UNSPEC,
		.ai_socktype = cfg->socktype,
		.ai_flags = cfg->ai_flags,
	};
	int err;

	/* If only one family is up, only query that one: avoids filling addrs with
	 * unreachable addresses and saves a DNS round trip.
	 */
	if ((up & STATE_IP) == STATE_IPV4) {
		hints.ai_family = NET_AF_INET;
	} else if ((up & STATE_IP) == STATE_IPV6) {
		hints.ai_family = NET_AF_INET6;
	}

	ctx->addr_count = 0;
	ctx->addr_next = 0;
	ctx->resolved = up & STATE_IP;

	err = zsock_getaddrinfo(cfg->host, cfg->service, &hints, &res);
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

/* Tries addresses from addr_next on. Returns a socket, the last connection error, or
 * -EHOSTUNREACH if no address could be tried.
 */
static int try_addresses(struct connect_by_name *ctx)
{
	const struct connect_by_name_cfg *cfg = ctx->cfg;
	int ret = -EHOSTUNREACH;

	for (; ctx->addr_next < ctx->addr_count; ctx->addr_next++) {
		const struct net_sockaddr *sa =
			(const struct net_sockaddr *)&ctx->addrs[ctx->addr_next];
		char ip[NET_IPV6_ADDR_LEN] = "";
		uint32_t up;
		int fd;

		k_mutex_lock(&state_lock, K_FOREVER);
		up = state;
		k_mutex_unlock(&state_lock);

		if (!(up & ctx->mask & family_state(sa->sa_family))) {
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
		ret = fd;
	}

	return ret;
}

int connect_by_name_init(struct connect_by_name *ctx, const struct connect_by_name_cfg *cfg)
{
	if (ctx == NULL || cfg == NULL || cfg->host == NULL || cfg->connect_cb == NULL ||
	    wanted_ip(cfg) == 0) {
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(*ctx));
	ctx->cfg = cfg;
	ctx->peer = -1;
	ctx->mask = wanted_ip(cfg) | (is_numeric(cfg->host) ? 0 : STATE_DNS);

	k_mutex_lock(&state_lock, K_FOREVER);
	ctx->seen = state & ctx->mask;
	if (cfg->lost_cb != NULL) {
		sys_slist_append(&contexts, &ctx->node);
	}
	k_mutex_unlock(&state_lock);

	return 0;
}

void connect_by_name_deinit(struct connect_by_name *ctx)
{
	k_mutex_lock(&state_lock, K_FOREVER);
	sys_slist_find_and_remove(&contexts, &ctx->node);
	k_mutex_unlock(&state_lock);
}

int connect_by_name(struct connect_by_name *ctx)
{
	uint32_t up;
	int err;

	update_state();

	k_mutex_lock(&state_lock, K_FOREVER);
	/* peer is read by the event handler */
	ctx->peer = -1;
	up = state & ctx->mask;
	ctx->seen = up;
	k_mutex_unlock(&state_lock);

	if (!is_ready(up, ctx->mask)) {
		return -ENETDOWN;
	}

	/* (Re)resolve if no addresses, all tried, or the wanted families up changed */
	if (ctx->addr_count == 0 || ctx->addr_next >= ctx->addr_count ||
	    (up & STATE_IP) != ctx->resolved) {
		err = resolve(ctx, up);
		if (err) {
			return err;
		}
	}

	return try_addresses(ctx);
}

void connect_by_name_wait_change(struct connect_by_name *ctx, k_timeout_t timeout)
{
	k_timepoint_t deadline = sys_timepoint_calc(timeout);

#if defined(CONFIG_CONNECT_BY_NAME_MODEM_READINESS)
	/* No events from the modem, poll it */
	while ((modem_state() & ctx->mask) == ctx->seen && !sys_timepoint_expired(deadline)) {
		k_timeout_t left = sys_timepoint_timeout(deadline);
		k_timeout_t poll = K_MSEC(CONFIG_CONNECT_BY_NAME_MODEM_POLL_INTERVAL_MS);

		k_sleep(!K_TIMEOUT_EQ(left, K_FOREVER) && left.ticks < poll.ticks ? left : poll);
	}
#else
	k_mutex_lock(&state_lock, K_FOREVER);
	while ((state & ctx->mask) == ctx->seen) {
		if (k_condvar_wait(&state_changed, &state_lock, sys_timepoint_timeout(deadline))) {
			break;
		}
	}
	k_mutex_unlock(&state_lock);
#endif
}

void connect_by_name_failed(struct connect_by_name *ctx)
{
	if (ctx->peer >= 0) {
		ctx->addr_next = ctx->peer + 1;
		k_mutex_lock(&state_lock, K_FOREVER);
		ctx->peer = -1;
		k_mutex_unlock(&state_lock);
	}
}

const struct net_sockaddr *connect_by_name_get_peer(const struct connect_by_name *ctx,
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
