/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_offload.h>
#include <net/net_host_connect.h>

#define HOST "example.com"
#define PORT 5683
#define MAX_TRIES 16

/* Fake DNS records, IPv6 first as returned by the resolver */
static const char *const records[] = { "2001:db8::1", "2001:db8::2", "192.0.2.1" };

static struct {
	int getaddrinfo_calls;
	int last_family;
	bool ignore_hints;
	const char *good_addr;
	char tried[MAX_TRIES][NET_IPV6_ADDR_LEN];
	int tries;
} fake;

static struct zsock_addrinfo ai_buf[ARRAY_SIZE(records) + 1];
static struct net_sockaddr_storage sa_buf[ARRAY_SIZE(records) + 1];

static bool fill_ai(int idx, const char *addr, int family)
{
	struct net_sockaddr *sa = (struct net_sockaddr *)&sa_buf[idx];
	void *dst = (family == NET_AF_INET6) ? (void *)&net_sin6(sa)->sin6_addr :
					       (void *)&net_sin(sa)->sin_addr;

	memset(sa, 0, sizeof(sa_buf[idx]));
	if (zsock_inet_pton(family, addr, dst) != 1) {
		return false;
	}

	sa->sa_family = family;
	ai_buf[idx] = (struct zsock_addrinfo) {
		.ai_family = family,
		.ai_addr = sa,
		.ai_addrlen = (family == NET_AF_INET6) ? sizeof(struct net_sockaddr_in6) :
							 sizeof(struct net_sockaddr_in),
	};

	return true;
}

static int fake_getaddrinfo(const char *node, const char *service,
			    const struct zsock_addrinfo *hints, struct zsock_addrinfo **res)
{
	int count = 0;

	fake.getaddrinfo_calls++;
	fake.last_family = hints->ai_family;

	if (fill_ai(0, node, NET_AF_INET) || fill_ai(0, node, NET_AF_INET6)) {
		count = 1;
	} else {
		ARRAY_FOR_EACH(records, i) {
			int family = strchr(records[i], ':') ? NET_AF_INET6 : NET_AF_INET;

			if (fake.ignore_hints || hints->ai_family == NET_AF_UNSPEC ||
			    hints->ai_family == family) {
				fill_ai(count++, records[i], family);
			}
		}
	}

	if (count == 0) {
		return DNS_EAI_NONAME;
	}

	for (int i = 0; i < count - 1; i++) {
		ai_buf[i].ai_next = &ai_buf[i + 1];
	}
	ai_buf[count - 1].ai_next = NULL;
	*res = &ai_buf[0];

	return 0;
}

static void fake_freeaddrinfo(struct zsock_addrinfo *res)
{
}

static const struct socket_dns_offload fake_dns = {
	.getaddrinfo = fake_getaddrinfo,
	.freeaddrinfo = fake_freeaddrinfo,
};

static int connect_cb(const struct net_sockaddr *addr, net_socklen_t addrlen, void *user_data)
{
	const void *src = (addr->sa_family == NET_AF_INET6) ?
			  (const void *)&net_sin6(addr)->sin6_addr :
			  (const void *)&net_sin(addr)->sin_addr;
	uint16_t port = (addr->sa_family == NET_AF_INET6) ? net_sin6(addr)->sin6_port :
							    net_sin(addr)->sin_port;
	char *ip = fake.tried[fake.tries];

	zassert_true(fake.tries < MAX_TRIES);
	zassert_equal(net_ntohs(port), PORT);
	zsock_inet_ntop(addr->sa_family, src, ip, NET_IPV6_ADDR_LEN);
	fake.tries++;

	if (fake.good_addr == NULL || strcmp(ip, fake.good_addr) == 0) {
		return 100 + fake.tries;
	}

	return -ECONNREFUSED;
}

static const struct net_host_connect_cfg cfg = {
	.host = HOST,
	.port = PORT,
	.socktype = NET_SOCK_DGRAM,
	.connect_cb = connect_cb,
	.retry_delay = K_MSEC(500),
};

static struct net_host_connect ctx;

static void notify(uint64_t event)
{
	net_mgmt_event_notify(event, NULL);
	k_msleep(10);
}

static void set_state(bool ipv4, bool ipv6, bool dns)
{
	notify(ipv4 ? NET_EVENT_L4_IPV4_CONNECTED : NET_EVENT_L4_IPV4_DISCONNECTED);
	notify(ipv6 ? NET_EVENT_L4_IPV6_CONNECTED : NET_EVENT_L4_IPV6_DISCONNECTED);
	notify(dns ? NET_EVENT_L4_DNS_READY : NET_EVENT_L4_DNS_LOST);
}

static uint64_t delayed_event;

static void delayed_event_fn(struct k_work *work)
{
	net_mgmt_event_notify(delayed_event, NULL);
}

static K_WORK_DELAYABLE_DEFINE(delayed_work, delayed_event_fn);

static void notify_later(uint64_t event, k_timeout_t delay)
{
	delayed_event = event;
	k_work_schedule(&delayed_work, delay);
}

static const char *peer_str(char *buf)
{
	net_socklen_t len;
	const struct net_sockaddr *sa = net_host_connect_get_peer(&ctx, &len);

	zassert_not_null(sa);
	zassert_equal(len, sa->sa_family == NET_AF_INET6 ? sizeof(struct net_sockaddr_in6) :
							   sizeof(struct net_sockaddr_in));
	return zsock_inet_ntop(sa->sa_family,
			       sa->sa_family == NET_AF_INET6 ?
				       (const void *)&net_sin6(sa)->sin6_addr :
				       (const void *)&net_sin(sa)->sin_addr,
			       buf, NET_IPV6_ADDR_LEN);
}

static void *suite_setup(void)
{
	socket_offload_dns_register(&fake_dns);
	return NULL;
}

static void before(void *f)
{
	k_work_cancel_delayable(&delayed_work);
	set_state(false, false, false);
	memset(&fake, 0, sizeof(fake));
	net_host_connect_init(&ctx, &cfg);
}

static void after(void *f)
{
	net_host_connect_deinit(&ctx);
}

ZTEST(net_host_connect, test_dual_stack)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, true, true);

	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_equal(fake.last_family, NET_AF_UNSPEC);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(peer_str(buf), "2001:db8::1");
}

ZTEST(net_host_connect, test_ipv4_only)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, false, true);

	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_equal(fake.last_family, NET_AF_INET);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(peer_str(buf), "192.0.2.1");
}

ZTEST(net_host_connect, test_ipv6_only)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(false, true, true);

	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_equal(fake.last_family, NET_AF_INET6);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(peer_str(buf), "2001:db8::1");

	/* IPv4 address never tried while IPv4 is down */
	fake.good_addr = "192.0.2.1";
	fake.tries = 0;
	net_host_connect_failed(&ctx);
	zassert_equal(net_host_connect(&ctx, K_MSEC(300)), -EAGAIN);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(fake.tried[0], "2001:db8::2");
}

/* Resolver returns both families, only addresses of ready families are tried */
ZTEST(net_host_connect, test_skip_not_ready)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, false, true);
	fake.ignore_hints = true;

	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(fake.tried[0], "192.0.2.1");
	zassert_str_equal(peer_str(buf), "192.0.2.1");
}

ZTEST(net_host_connect, test_try_next_on_failure)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, true, true);
	fake.good_addr = "192.0.2.1";

	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_equal(fake.tries, 3);
	zassert_str_equal(fake.tried[0], "2001:db8::1");
	zassert_str_equal(fake.tried[1], "2001:db8::2");
	zassert_str_equal(peer_str(buf), "192.0.2.1");
}

ZTEST(net_host_connect, test_timeout_without_dns)
{
	set_state(true, true, false);

	zassert_equal(net_host_connect(&ctx, K_MSEC(100)), -EAGAIN);
	zassert_equal(fake.getaddrinfo_calls, 0);
}

ZTEST(net_host_connect, test_numeric_host_without_dns)
{
	static const struct net_host_connect_cfg numeric_cfg = {
		.host = "192.0.2.7",
		.port = PORT,
		.socktype = NET_SOCK_DGRAM,
		.connect_cb = connect_cb,
		.retry_delay = K_MSEC(500),
	};
	char buf[NET_IPV6_ADDR_LEN];

	net_host_connect_init(&ctx, &numeric_cfg);
	set_state(true, false, false);

	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_str_equal(peer_str(buf), "192.0.2.7");
}

/* IPv6 comes up first and doesn't work, IPv4 comes up later and works */
ZTEST(net_host_connect, test_recover_when_second_family_comes_up)
{
	static const struct net_host_connect_cfg slow_retry_cfg = {
		.host = HOST,
		.port = PORT,
		.socktype = NET_SOCK_DGRAM,
		.connect_cb = connect_cb,
		.retry_delay = K_SECONDS(60),
	};
	char buf[NET_IPV6_ADDR_LEN];
	int64_t start;

	net_host_connect_init(&ctx, &slow_retry_cfg);
	set_state(false, true, true);
	fake.good_addr = "192.0.2.1";
	notify_later(NET_EVENT_L4_IPV4_CONNECTED, K_MSEC(200));

	start = k_uptime_get();
	zassert_true(net_host_connect(&ctx, K_SECONDS(10)) >= 0);
	zassert_true(k_uptime_get() - start < 1000, "did not react to IPv4 coming up");

	zassert_equal(fake.getaddrinfo_calls, 2);
	zassert_equal(fake.last_family, NET_AF_UNSPEC);
	zassert_str_equal(peer_str(buf), "192.0.2.1");
}

ZTEST(net_host_connect, test_try_next_after_failed_then_backoff)
{
	char buf[NET_IPV6_ADDR_LEN];
	int64_t start;

	set_state(true, true, true);

	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_str_equal(peer_str(buf), "2001:db8::1");

	net_host_connect_failed(&ctx);
	zassert_is_null(net_host_connect_get_peer(&ctx, NULL));
	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_str_equal(peer_str(buf), "2001:db8::2");

	net_host_connect_failed(&ctx);
	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_str_equal(peer_str(buf), "192.0.2.1");
	zassert_equal(fake.getaddrinfo_calls, 1);

	/* All tried: back off, then resolve again and start over */
	net_host_connect_failed(&ctx);
	start = k_uptime_get();
	zassert_true(net_host_connect(&ctx, K_SECONDS(2)) >= 0);
	zassert_true(k_uptime_get() - start >= 500);
	zassert_equal(fake.getaddrinfo_calls, 2);
	zassert_str_equal(peer_str(buf), "2001:db8::1");
}

ZTEST(net_host_connect, test_reconnect_without_failed_reuses_address)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, true, true);

	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	zassert_equal(fake.getaddrinfo_calls, 1);
	zassert_str_equal(peer_str(buf), "2001:db8::1");
}

ZTEST(net_host_connect, test_all_fail_timeout)
{
	set_state(true, true, true);
	fake.good_addr = "none";

	zassert_equal(net_host_connect(&ctx, K_MSEC(300)), -EAGAIN);
	zassert_equal(fake.tries, 3);
}

static K_SEM_DEFINE(lost_sem, 0, 1);

static void lost_cb(struct net_host_connect *c, void *user_data)
{
	zassert_equal_ptr(c, &ctx);
	k_sem_give(&lost_sem);
}

ZTEST(net_host_connect, test_lost_cb)
{
	static const struct net_host_connect_cfg lost_cfg = {
		.host = HOST,
		.port = PORT,
		.socktype = NET_SOCK_DGRAM,
		.connect_cb = connect_cb,
		.lost_cb = lost_cb,
		.retry_delay = K_MSEC(500),
	};

	net_host_connect_init(&ctx, &lost_cfg);
	k_sem_reset(&lost_sem);
	set_state(true, true, true);

	/* Not connected: no notification */
	notify(NET_EVENT_L4_IPV4_DISCONNECTED);
	notify(NET_EVENT_L4_IPV4_CONNECTED);
	zassert_equal(k_sem_take(&lost_sem, K_MSEC(50)), -EAGAIN);

	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);

	/* IPv4 loss does not affect an IPv6 connection */
	notify(NET_EVENT_L4_IPV4_DISCONNECTED);
	zassert_equal(k_sem_take(&lost_sem, K_MSEC(50)), -EAGAIN);

	notify_later(NET_EVENT_L4_IPV6_DISCONNECTED, K_MSEC(50));
	zassert_ok(k_sem_take(&lost_sem, K_SECONDS(5)));

	/* No notification after deinit */
	net_host_connect_deinit(&ctx);
	set_state(true, true, true);
	zassert_true(net_host_connect(&ctx, K_SECONDS(1)) >= 0);
	notify(NET_EVENT_L4_IPV6_DISCONNECTED);
	zassert_equal(k_sem_take(&lost_sem, K_MSEC(50)), -EAGAIN);
}

ZTEST_SUITE(net_host_connect, NULL, suite_setup, before, after, NULL);
