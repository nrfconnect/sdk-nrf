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
#include <net/connect_by_name.h>

#define HOST "example.com"
#define PORT 5683
#define MAX_TRIES 16

/* Fake DNS records, IPv6 first as returned by the resolver */
static const char *const records[] = { "2001:db8::1", "2001:db8::2", "192.0.2.1" };

static struct {
	int getaddrinfo_calls;
	int last_family;
	int last_flags;
	const char *last_service;
	bool ignore_hints;
	bool fail_resolve;
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
	fake.last_flags = hints->ai_flags;
	fake.last_service = service;

	if (fake.fail_resolve) {
		return DNS_EAI_FAIL;
	}

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

static const struct connect_by_name_cfg cfg = {
	.host = HOST,
	.port = PORT,
	.socktype = NET_SOCK_DGRAM,
	.connect_cb = connect_cb,
};

static struct connect_by_name ctx;

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
	const struct net_sockaddr *sa = connect_by_name_get_peer(&ctx, &len);

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
	zassert_ok(connect_by_name_init(&ctx, &cfg));
}

static void after(void *f)
{
	connect_by_name_deinit(&ctx);
}

ZTEST(connect_by_name, test_dual_stack)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, true, true);

	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.last_family, NET_AF_UNSPEC);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(peer_str(buf), "2001:db8::1");
}

ZTEST(connect_by_name, test_ipv4_only)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, false, true);

	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.last_family, NET_AF_INET);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(peer_str(buf), "192.0.2.1");
}

ZTEST(connect_by_name, test_ipv6_only)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(false, true, true);

	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.last_family, NET_AF_INET6);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(peer_str(buf), "2001:db8::1");

	/* IPv4 address never tried while IPv4 is down */
	fake.good_addr = "192.0.2.1";
	fake.tries = 0;
	connect_by_name_failed(&ctx);
	zassert_equal(connect_by_name(&ctx), -ECONNREFUSED);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(fake.tried[0], "2001:db8::2");
}

/* Resolver returns both families, only addresses of ready families are tried */
ZTEST(connect_by_name, test_skip_not_ready)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, false, true);
	fake.ignore_hints = true;

	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(fake.tried[0], "192.0.2.1");
	zassert_str_equal(peer_str(buf), "192.0.2.1");
}

ZTEST(connect_by_name, test_try_next_on_failure)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, true, true);
	fake.good_addr = "192.0.2.1";

	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.tries, 3);
	zassert_str_equal(fake.tried[0], "2001:db8::1");
	zassert_str_equal(fake.tried[1], "2001:db8::2");
	zassert_str_equal(peer_str(buf), "192.0.2.1");
}

ZTEST(connect_by_name, test_no_dns)
{
	set_state(true, true, false);

	zassert_equal(connect_by_name(&ctx), -ENETDOWN);
	zassert_equal(fake.getaddrinfo_calls, 0);
}

ZTEST(connect_by_name, test_numeric_host_without_dns)
{
	static const struct connect_by_name_cfg numeric_cfg = {
		.host = "192.0.2.7",
		.port = PORT,
		.socktype = NET_SOCK_DGRAM,
		.connect_cb = connect_cb,
	};
	char buf[NET_IPV6_ADDR_LEN];

	zassert_ok(connect_by_name_init(&ctx, &numeric_cfg));
	set_state(true, false, false);

	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_str_equal(peer_str(buf), "192.0.2.7");
}

/* IPv6 comes up first and doesn't work, IPv4 comes up later and works */
ZTEST(connect_by_name, test_recover_when_second_family_comes_up)
{
	char buf[NET_IPV6_ADDR_LEN];
	int64_t start;

	set_state(false, true, true);
	fake.good_addr = "192.0.2.1";
	zassert_equal(connect_by_name(&ctx), -ECONNREFUSED);

	notify_later(NET_EVENT_L4_IPV4_CONNECTED, K_MSEC(200));
	start = k_uptime_get();
	connect_by_name_wait_change(&ctx, K_SECONDS(10));
	zassert_true(k_uptime_get() - start < 1000, "did not react to IPv4 coming up");

	zassert_true(connect_by_name(&ctx) >= 0);

	zassert_equal(fake.getaddrinfo_calls, 2);
	zassert_equal(fake.last_family, NET_AF_UNSPEC);
	zassert_str_equal(peer_str(buf), "192.0.2.1");
}

ZTEST(connect_by_name, test_try_next_after_failed_then_resolve_again)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, true, true);

	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_str_equal(peer_str(buf), "2001:db8::1");

	connect_by_name_failed(&ctx);
	zassert_is_null(connect_by_name_get_peer(&ctx, NULL));
	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_str_equal(peer_str(buf), "2001:db8::2");

	connect_by_name_failed(&ctx);
	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_str_equal(peer_str(buf), "192.0.2.1");
	zassert_equal(fake.getaddrinfo_calls, 1);

	/* All tried: resolve again and start over */
	connect_by_name_failed(&ctx);
	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.getaddrinfo_calls, 2);
	zassert_str_equal(peer_str(buf), "2001:db8::1");
}

ZTEST(connect_by_name, test_reconnect_without_failed_reuses_address)
{
	char buf[NET_IPV6_ADDR_LEN];

	set_state(true, true, true);

	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.getaddrinfo_calls, 1);
	zassert_str_equal(peer_str(buf), "2001:db8::1");
}

ZTEST(connect_by_name, test_all_fail)
{
	set_state(true, true, true);
	fake.good_addr = "none";

	zassert_equal(connect_by_name(&ctx), -ECONNREFUSED);
	zassert_equal(fake.tries, 3);
}

ZTEST(connect_by_name, test_wait_timeout)
{
	int64_t start;

	set_state(true, true, true);
	zassert_true(connect_by_name(&ctx) >= 0);

	/* No change since the last call: full timeout */
	start = k_uptime_get();
	connect_by_name_wait_change(&ctx, K_MSEC(200));
	zassert_true(k_uptime_get() - start >= 200);
}

ZTEST(connect_by_name, test_wait_change)
{
	int64_t start;

	zassert_equal(connect_by_name(&ctx), -ENETDOWN);

	/* Change already happened: returns immediately */
	set_state(true, true, true);
	start = k_uptime_get();
	connect_by_name_wait_change(&ctx, K_SECONDS(5));
	zassert_true(k_uptime_get() - start < 100);

	/* Change while waiting */
	zassert_true(connect_by_name(&ctx) >= 0);
	notify_later(NET_EVENT_L4_IPV4_DISCONNECTED, K_MSEC(100));
	start = k_uptime_get();
	connect_by_name_wait_change(&ctx, K_SECONDS(5));
	zassert_true(k_uptime_get() - start < 1000);
}

ZTEST(connect_by_name, test_one_round)
{
	int64_t start;

	/* Not ready: nothing attempted */
	zassert_equal(connect_by_name(&ctx), -ENETDOWN);
	zassert_equal(fake.getaddrinfo_calls, 0);

	set_state(true, true, true);
	start = k_uptime_get();

	/* One round, last connection error returned */
	fake.good_addr = "none";
	zassert_equal(connect_by_name(&ctx), -ECONNREFUSED);
	zassert_equal(fake.tries, 3);

	/* All addresses exhausted: resolves again */
	fake.good_addr = NULL;
	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.getaddrinfo_calls, 2);

	fake.fail_resolve = true;
	connect_by_name_failed(&ctx);
	zassert_true(connect_by_name(&ctx) >= 0);
	connect_by_name_failed(&ctx);
	zassert_true(connect_by_name(&ctx) >= 0);
	connect_by_name_failed(&ctx);
	zassert_equal(connect_by_name(&ctx), -EHOSTUNREACH);

	zassert_true(k_uptime_get() - start < 100, "should not wait");
}

ZTEST(connect_by_name, test_family)
{
	static const struct connect_by_name_cfg ipv4_cfg = {
		.host = HOST,
		.port = PORT,
		.socktype = NET_SOCK_DGRAM,
		.family = NET_AF_INET,
		.connect_cb = connect_cb,
	};
	char buf[NET_IPV6_ADDR_LEN];
	int64_t start;

	zassert_ok(connect_by_name_init(&ctx, &ipv4_cfg));

	/* Wanted family down */
	set_state(false, true, true);
	zassert_equal(connect_by_name(&ctx), -ENETDOWN);

	/* Other family addresses are skipped even if returned */
	set_state(true, true, true);
	fake.ignore_hints = true;
	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.last_family, NET_AF_INET);
	zassert_equal(fake.tries, 1);
	zassert_str_equal(peer_str(buf), "192.0.2.1");

	/* Other family changes are ignored: no wakeup, no new resolution */
	notify_later(NET_EVENT_L4_IPV6_DISCONNECTED, K_MSEC(50));
	start = k_uptime_get();
	connect_by_name_wait_change(&ctx, K_MSEC(300));
	zassert_true(k_uptime_get() - start >= 300);
	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.getaddrinfo_calls, 1);
}

ZTEST(connect_by_name, test_init_invalid)
{
	struct connect_by_name_cfg bad = cfg;
	struct connect_by_name other;

	bad.host = NULL;
	zassert_equal(connect_by_name_init(&other, &bad), -EINVAL);

	bad = cfg;
	bad.connect_cb = NULL;
	zassert_equal(connect_by_name_init(&other, &bad), -EINVAL);

	bad = cfg;
	bad.family = NET_AF_PACKET;
	zassert_equal(connect_by_name_init(&other, &bad), -EINVAL);
}

ZTEST(connect_by_name, test_service_and_flags)
{
	static const struct connect_by_name_cfg svc_cfg = {
		.host = HOST,
		.port = PORT,
		.socktype = NET_SOCK_DGRAM,
		.service = "1",
		.ai_flags = 0x1000,
		.connect_cb = connect_cb,
	};

	zassert_ok(connect_by_name_init(&ctx, &svc_cfg));
	set_state(true, true, true);

	/* Port comes from the configuration, checked by connect_cb */
	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_str_equal(fake.last_service, "1");
	zassert_equal(fake.last_flags, 0x1000);
}

static K_SEM_DEFINE(lost_sem, 0, 1);

static void lost_cb(struct connect_by_name *c, void *user_data)
{
	zassert_equal_ptr(c, &ctx);
	k_sem_give(&lost_sem);
}

ZTEST(connect_by_name, test_lost_cb)
{
	static const struct connect_by_name_cfg lost_cfg = {
		.host = HOST,
		.port = PORT,
		.socktype = NET_SOCK_DGRAM,
		.connect_cb = connect_cb,
		.lost_cb = lost_cb,
	};

	zassert_ok(connect_by_name_init(&ctx, &lost_cfg));
	k_sem_reset(&lost_sem);
	set_state(true, true, true);

	/* Not connected: no notification */
	notify(NET_EVENT_L4_IPV4_DISCONNECTED);
	notify(NET_EVENT_L4_IPV4_CONNECTED);
	zassert_equal(k_sem_take(&lost_sem, K_MSEC(50)), -EAGAIN);

	zassert_true(connect_by_name(&ctx) >= 0);

	/* IPv4 loss does not affect an IPv6 connection */
	notify(NET_EVENT_L4_IPV4_DISCONNECTED);
	zassert_equal(k_sem_take(&lost_sem, K_MSEC(50)), -EAGAIN);

	notify_later(NET_EVENT_L4_IPV6_DISCONNECTED, K_MSEC(50));
	zassert_ok(k_sem_take(&lost_sem, K_SECONDS(5)));

	/* No notification after deinit */
	connect_by_name_deinit(&ctx);
	set_state(true, true, true);
	zassert_true(connect_by_name(&ctx) >= 0);
	notify(NET_EVENT_L4_IPV6_DISCONNECTED);
	zassert_equal(k_sem_take(&lost_sem, K_MSEC(50)), -EAGAIN);
}

static bool readiness(const void *state)
{
	return IS_ENABLED(CONFIG_CONNECT_BY_NAME_READINESS);
}

static bool no_readiness(const void *state)
{
	return !readiness(state) && !IS_ENABLED(CONFIG_CONNECT_BY_NAME_MODEM_READINESS);
}

ZTEST_SUITE(connect_by_name, readiness, suite_setup, before, after, NULL);

/* Without readiness tracking: all families queried and tried, wait is a plain delay */
ZTEST(connect_by_name_no_readiness, test_no_readiness)
{
	char buf[NET_IPV6_ADDR_LEN];
	int64_t start = k_uptime_get();

	fake.good_addr = "192.0.2.1";
	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.last_family, NET_AF_UNSPEC);
	zassert_equal(fake.tries, 3);
	zassert_str_equal(peer_str(buf), "192.0.2.1");

	fake.good_addr = "none";
	fake.tries = 0;
	connect_by_name_failed(&ctx);
	zassert_equal(connect_by_name(&ctx), -ECONNREFUSED);
	zassert_equal(fake.tries, 3);
	zassert_true(k_uptime_get() - start < 100, "should not wait");

	start = k_uptime_get();
	connect_by_name_wait_change(&ctx, K_MSEC(200));
	zassert_true(k_uptime_get() - start >= 200);
}

ZTEST_SUITE(connect_by_name_no_readiness, no_readiness, suite_setup, before, after, NULL);

#if defined(CONFIG_CONNECT_BY_NAME_MODEM_READINESS)
#include <stdarg.h>
#include <stdio.h>

static const char *volatile cgpaddr;
static int scanf_calls;

int nrf_modem_at_scanf(const char *cmd, const char *fmt, ...)
{
	va_list args;
	int ret;

	scanf_calls++;
	zassert_str_equal(cmd, "AT+CGPADDR=0");
	if (cgpaddr == NULL) {
		return -1;
	}

	va_start(args, fmt);
	ret = vsscanf(cgpaddr, fmt, args);
	va_end(args);

	return ret;
}

#define CGPADDR_NONE "+CGPADDR: 0"
#define CGPADDR_V4   "+CGPADDR: 0,\"10.0.0.1\""
#define CGPADDR_V6   "+CGPADDR: 0,\"2001:DB8::10\""
#define CGPADDR_BOTH "+CGPADDR: 0,\"10.0.0.1\",\"2001:DB8::10\""

static void modem_before(void *f)
{
	cgpaddr = NULL;
	scanf_calls = 0;
	before(f);
}

static void modem_connect(const char *addrs, int family)
{
	cgpaddr = addrs;
	fake.getaddrinfo_calls = 0;
	fake.tries = 0;
	connect_by_name_failed(&ctx);
	zassert_true(connect_by_name(&ctx) >= 0);
	zassert_equal(fake.getaddrinfo_calls, 1);
	zassert_equal(fake.last_family, family);
}

/* The families up are read from the modem's addresses */
ZTEST(connect_by_name_modem, test_modem_state)
{
	cgpaddr = NULL;
	zassert_equal(connect_by_name(&ctx), -ENETDOWN);
	cgpaddr = CGPADDR_NONE;
	zassert_equal(connect_by_name(&ctx), -ENETDOWN);
	zassert_equal(fake.getaddrinfo_calls, 0);

	modem_connect(CGPADDR_V4, NET_AF_INET);
	modem_connect(CGPADDR_V6, NET_AF_INET6);
	modem_connect(CGPADDR_BOTH, NET_AF_UNSPEC);
}

static void modem_up_fn(struct k_work *work)
{
	cgpaddr = CGPADDR_BOTH;
}

static K_WORK_DELAYABLE_DEFINE(modem_up_work, modem_up_fn);

/* Waiting polls the modem until the families up change, or timeout */
ZTEST(connect_by_name_modem, test_modem_wait_change)
{
	int64_t start;

	cgpaddr = CGPADDR_V4;
	fake.good_addr = "none";
	zassert_equal(connect_by_name(&ctx), -ECONNREFUSED);

	start = k_uptime_get();
	connect_by_name_wait_change(&ctx, K_MSEC(100));
	zassert_true(k_uptime_get() - start >= 100);
	zassert_true(scanf_calls > 2, "should poll");

	k_work_schedule(&modem_up_work, K_MSEC(50));
	start = k_uptime_get();
	connect_by_name_wait_change(&ctx, K_SECONDS(1));
	zassert_true(k_uptime_get() - start < 500, "should wake on change");
}

static bool modem_readiness(const void *state)
{
	return true;
}

ZTEST_SUITE(connect_by_name_modem, modem_readiness, suite_setup, modem_before, after, NULL);
#endif /* CONFIG_CONNECT_BY_NAME_MODEM_READINESS */
