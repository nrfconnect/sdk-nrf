/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_ncs.h>
#include <net/ready_resolve.h>

#if defined(CONFIG_READY_RESOLVE_TRACK_MODEM)
#include <nrf_modem_at.h>
#endif

LOG_MODULE_REGISTER(ready_resolve, CONFIG_READY_RESOLVE_LOG_LEVEL);

#define STATE_IPV4 BIT(0)
#define STATE_IPV6 BIT(1)
#define STATE_DNS  BIT(2)
#define STATE_IP   (STATE_IPV4 | STATE_IPV6)
#define STATE_BITS 3

/* Connectivity state shared by all contexts */
static uint32_t state;
/* Number of times each state bit came up */
static uint32_t gain_count[STATE_BITS];
static K_MUTEX_DEFINE(state_lock);
static K_CONDVAR_DEFINE(state_changed);

static uint32_t family_state(int family)
{
	if (IS_ENABLED(CONFIG_NET_IPV4) && family == NET_AF_INET) {
		return STATE_IPV4;
	}
	if (IS_ENABLED(CONFIG_NET_IPV6) && family == NET_AF_INET6) {
		return STATE_IPV6;
	}

	return 0;
}

/* Called with state_lock held */
static uint32_t gain_sum_locked(uint32_t mask)
{
	uint32_t sum = 0;

	for (int i = 0; i < STATE_BITS; i++) {
		if (mask & BIT(i)) {
			sum += gain_count[i];
		}
	}

	return sum;
}

/* Called with state_lock held */
static void state_set_locked(uint32_t new_state)
{
	uint32_t gained = new_state & ~state;

	if (new_state == state) {
		return;
	}

	for (int i = 0; i < STATE_BITS; i++) {
		if (gained & BIT(i)) {
			gain_count[i]++;
		}
	}

	LOG_DBG("IPv4 %d, IPv6 %d, DNS %d", !!(new_state & STATE_IPV4),
		!!(new_state & STATE_IPV6), !!(new_state & STATE_DNS));

	state = new_state;
	k_condvar_broadcast(&state_changed);
}

#if defined(CONFIG_READY_RESOLVE_TRACK_CONN_MGR)
#define EVENT_MASK (NET_EVENT_L4_IPV4_CONNECTED | NET_EVENT_L4_IPV4_DISCONNECTED | \
		    NET_EVENT_L4_IPV6_CONNECTED | NET_EVENT_L4_IPV6_DISCONNECTED | \
		    NET_EVENT_L4_DNS_READY | NET_EVENT_L4_DNS_LOST)

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

	state_set_locked(new_state);
	k_mutex_unlock(&state_lock);
}

/* Registered statically to not miss events raised before the first call */
NET_MGMT_REGISTER_EVENT_HANDLER(ready_resolve_events, EVENT_MASK, event_handler, NULL);
#endif /* CONFIG_READY_RESOLVE_TRACK_CONN_MGR */

#if defined(CONFIG_READY_RESOLVE_TRACK_MODEM)
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
#endif /* CONFIG_READY_RESOLVE_TRACK_MODEM */

/* Gets the state bits of mask that are up, and the sum of their gain counters */
static uint32_t snapshot(uint32_t mask, uint32_t *gains)
{
	uint32_t up;
#if defined(CONFIG_READY_RESOLVE_TRACK_MODEM)
	uint32_t polled = modem_state();
#endif

	k_mutex_lock(&state_lock, K_FOREVER);
#if defined(CONFIG_READY_RESOLVE_TRACK_MODEM)
	state_set_locked(polled);
#endif
	up = state & mask;
	*gains = gain_sum_locked(mask);
	k_mutex_unlock(&state_lock);

	return up;
}

/* True if a wanted family is up and, if needed, DNS is available */
static bool is_ready(uint32_t up, uint32_t mask)
{
	return (up & STATE_IP) && ((up & STATE_DNS) || !(mask & STATE_DNS));
}

static bool is_numeric(const char *host)
{
	uint8_t buf[sizeof(struct net_in6_addr)];

	return zsock_inet_pton(NET_AF_INET, host, buf) == 1 ||
	       zsock_inet_pton(NET_AF_INET6, host, buf) == 1;
}

static int dns_err(int err)
{
	switch (err) {
	case DNS_EAI_AGAIN:
		return -EAGAIN;
	case DNS_EAI_MEMORY:
		return -ENOMEM;
	default:
		return -EHOSTUNREACH;
	}
}

int ready_resolve(struct ready_resolve *ctx, const char *host, const char *service,
		  const struct zsock_addrinfo *hints, uint16_t port)
{
	struct zsock_addrinfo h = {0};
	struct zsock_addrinfo *res;
	uint32_t mask;
	uint32_t up;
	int count = 0;
	int err;

	if (ctx == NULL || host == NULL) {
		return -EINVAL;
	}

	/* Copy only the fields getaddrinfo() accepts as hints */
	if (hints != NULL) {
		h.ai_family = hints->ai_family;
		h.ai_socktype = hints->ai_socktype;
		h.ai_protocol = hints->ai_protocol;
		h.ai_flags = hints->ai_flags;
	}

	ready_resolve_free(ctx);

	/* State bits this lookup needs: the wanted families, plus DNS for host names */
	mask = (h.ai_family == NET_AF_UNSPEC) ?
	       (family_state(NET_AF_INET) | family_state(NET_AF_INET6)) :
	       family_state(h.ai_family);
	if (mask == 0) {
		ctx->mask = 0;
		return -EINVAL;
	}

	if (!is_numeric(host)) {
		mask |= STATE_DNS;
	}

	ctx->mask = mask;
	ctx->port = port;
	/* Only the default PDN is tracked, leave other PDNs to getaddrinfo() and connect() */
	ctx->untracked = (h.ai_flags & AI_PDNSERV) != 0;

	if (!ctx->untracked) {
		/* Also records the gain counters, the baseline for ready_resolve_wait_change() */
		up = snapshot(mask, &ctx->gains);
		if (!is_ready(up, mask)) {
			return -ENETDOWN;
		}

		/* If only one family is up, only query that one: avoids returning unreachable
		 * addresses and saves a DNS round trip.
		 */
		if ((up & STATE_IP) == STATE_IPV4) {
			h.ai_family = NET_AF_INET;
		} else if ((up & STATE_IP) == STATE_IPV6) {
			h.ai_family = NET_AF_INET6;
		}
	}

	err = zsock_getaddrinfo(host, service, &h, &res);
	if (err) {
		LOG_WRN("Failed to resolve %s: %d", host, err);
		return dns_err(err);
	}

	/* Count addresses of enabled families only */
	for (struct zsock_addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
		if (family_state(ai->ai_family)) {
			count++;
		}
	}

	if (count == 0) {
		LOG_WRN("No usable address for %s", host);
		zsock_freeaddrinfo(res);
		return -EHOSTUNREACH;
	}

	LOG_DBG("Resolved %s: %d address(es)", host, count);

	ctx->res = res;
	ctx->next = res;

	return 0;
}

int ready_resolve_next(struct ready_resolve *ctx, const struct net_sockaddr **addr,
		       net_socklen_t *addrlen)
{
	uint32_t up;

	if (ctx == NULL || addr == NULL || addrlen == NULL) {
		return -EINVAL;
	}

	/* Last known state, the modem is not polled for each address */
	k_mutex_lock(&state_lock, K_FOREVER);
	up = ctx->untracked ? STATE_IP : state;
	k_mutex_unlock(&state_lock);

	while (ctx->next != NULL) {
		struct zsock_addrinfo *ai = ctx->next;
		struct net_sockaddr *sa = ai->ai_addr;
		char ip[NET_IPV6_ADDR_LEN] = "";
		uint16_t *port;

		ctx->next = ai->ai_next;

		/* Not wanted, or down */
		if (!(up & ctx->mask & family_state(ai->ai_family))) {
			continue;
		}

		/* Overrides the port from the service, if any */
		port = (sa->sa_family == NET_AF_INET6) ? &net_sin6(sa)->sin6_port :
							 &net_sin(sa)->sin_port;
		if (ctx->port != 0) {
			*port = net_htons(ctx->port);
		}

		/* Log each attempt, so connection failures can be traced to an address */
		zsock_inet_ntop(sa->sa_family,
				(sa->sa_family == NET_AF_INET6) ? (void *)&net_sin6(sa)->sin6_addr :
								  (void *)&net_sin(sa)->sin_addr,
				ip, sizeof(ip));
		LOG_INF("Trying %s, port %u", ip, net_ntohs(*port));

		*addr = sa;
		*addrlen = (sa->sa_family == NET_AF_INET6) ? sizeof(struct net_sockaddr_in6) :
							     sizeof(struct net_sockaddr_in);

		return 0;
	}

	/* All addresses tried, or skipped */
	return -ENOENT;
}

void ready_resolve_free(struct ready_resolve *ctx)
{
	if (ctx == NULL) {
		return;
	}

	if (ctx->res != NULL) {
		zsock_freeaddrinfo(ctx->res);
	}

	ctx->res = NULL;
	ctx->next = NULL;
}

int ready_resolve_wait_change(const struct ready_resolve *ctx, k_timeout_t timeout)
{
	uint32_t mask = ctx->mask;

	/* Never resolved, or untracked: nothing to compare with */
	if (mask == 0 || ctx->untracked) {
		k_sleep(timeout);
		return -EAGAIN;
	}

#if defined(CONFIG_READY_RESOLVE_TRACK_CONN_MGR)
	k_timepoint_t deadline = sys_timepoint_calc(timeout);
	int err = 0;

	/* Wait until a wanted bit came up since ready_resolve(), events signal the condvar */
	k_mutex_lock(&state_lock, K_FOREVER);
	while (gain_sum_locked(mask) == ctx->gains) {
		if (k_condvar_wait(&state_changed, &state_lock, sys_timepoint_timeout(deadline))) {
			err = -EAGAIN;
			break;
		}
	}
	k_mutex_unlock(&state_lock);

	return err;
#elif defined(CONFIG_READY_RESOLVE_TRACK_MODEM)
	k_timepoint_t deadline = sys_timepoint_calc(timeout);
	uint32_t gains;

	/* No events from the modem, poll it */
	while (true) {
		(void)snapshot(mask, &gains);
		if (gains != ctx->gains) {
			return 0;
		}

		if (sys_timepoint_expired(deadline)) {
			return -EAGAIN;
		}

		/* Sleep one poll interval, or less if the deadline is closer */
		k_timeout_t left = sys_timepoint_timeout(deadline);
		k_timeout_t poll = K_MSEC(CONFIG_READY_RESOLVE_MODEM_POLL_INTERVAL_MS);

		k_sleep(!K_TIMEOUT_EQ(left, K_FOREVER) && left.ticks < poll.ticks ? left : poll);
	}
#else
#error "No connectivity tracking"
#endif
}
