/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_offload.h>
#include <net/ready_resolve.h>

#define HOST "example.com"
#define PORT 5683

/* Fake DNS records, IPv6 first as returned by the resolver */
static const char *const records[] = { "2001:db8::1", "2001:db8::2", "192.0.2.1" };

static struct {
	int getaddrinfo_calls;
	int freeaddrinfo_calls;
	int last_family;
	int last_socktype;
	int last_flags;
	const char *last_service;
	bool ignore_hints;
	int resolve_err;
} fake;

static struct zsock_addrinfo ai_buf[ARRAY_SIZE(records) + 1];
static struct net_sockaddr_storage sa_buf[ARRAY_SIZE(records) + 1];

static bool fill_ai(int idx, const char *addr, int family, uint16_t port)
{
	struct net_sockaddr *sa = (struct net_sockaddr *)&sa_buf[idx];
	void *dst = (family == NET_AF_INET6) ? (void *)&net_sin6(sa)->sin6_addr :
					       (void *)&net_sin(sa)->sin_addr;

	memset(sa, 0, sizeof(sa_buf[idx]));
	if (zsock_inet_pton(family, addr, dst) != 1) {
		return false;
	}

	sa->sa_family = family;
	if (family == NET_AF_INET6) {
		net_sin6(sa)->sin6_port = net_htons(port);
	} else {
		net_sin(sa)->sin_port = net_htons(port);
	}

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
	uint16_t port = service ? atoi(service) : 0;
	int count = 0;

	fake.getaddrinfo_calls++;
	fake.last_family = hints->ai_family;
	fake.last_socktype = hints->ai_socktype;
	fake.last_flags = hints->ai_flags;
	fake.last_service = service;

	if (fake.resolve_err) {
		return fake.resolve_err;
	}

	if (fill_ai(0, node, NET_AF_INET, port) || fill_ai(0, node, NET_AF_INET6, port)) {
		count = 1;
	} else {
		ARRAY_FOR_EACH(records, i) {
			int family = strchr(records[i], ':') ? NET_AF_INET6 : NET_AF_INET;

			if (fake.ignore_hints || hints->ai_family == NET_AF_UNSPEC ||
			    hints->ai_family == family) {
				fill_ai(count++, records[i], family, port);
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
	fake.freeaddrinfo_calls++;
}

static const struct socket_dns_offload fake_dns = {
	.getaddrinfo = fake_getaddrinfo,
	.freeaddrinfo = fake_freeaddrinfo,
};

static const struct zsock_addrinfo hints = {
	.ai_socktype = NET_SOCK_DGRAM,
};

static struct ready_resolve ctx;

static int resolve(void)
{
	return ready_resolve(&ctx, HOST, NULL, &hints, PORT);
}

/* Gets the next address as a string, checking its length and port */
static const char *next_ip(uint16_t expected_port)
{
	static char ip[NET_IPV6_ADDR_LEN];
	const struct net_sockaddr *sa;
	net_socklen_t len;
	bool v6;

	if (ready_resolve_next(&ctx, &sa, &len) != 0) {
		return "";
	}

	v6 = sa->sa_family == NET_AF_INET6;
	zassert_equal(len, v6 ? sizeof(struct net_sockaddr_in6) : sizeof(struct net_sockaddr_in));
	zassert_equal(net_ntohs(v6 ? net_sin6(sa)->sin6_port : net_sin(sa)->sin_port),
		      expected_port);

	return zsock_inet_ntop(sa->sa_family,
			       v6 ? (const void *)&net_sin6(sa)->sin6_addr :
				    (const void *)&net_sin(sa)->sin_addr,
			       ip, sizeof(ip));
}

static void assert_exhausted(void)
{
	const struct net_sockaddr *sa;
	net_socklen_t len;

	zassert_equal(ready_resolve_next(&ctx, &sa, &len), -ENOENT);
}

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

static uint64_t delayed_events[2];

static void delayed_event_fn(struct k_work *work)
{
	ARRAY_FOR_EACH(delayed_events, i) {
		if (delayed_events[i]) {
			net_mgmt_event_notify(delayed_events[i], NULL);
		}
	}
}

static K_WORK_DELAYABLE_DEFINE(delayed_work, delayed_event_fn);

static void notify_later(uint64_t event1, uint64_t event2, k_timeout_t delay)
{
	delayed_events[0] = event1;
	delayed_events[1] = event2;
	k_work_schedule(&delayed_work, delay);
}

/* Measures how long ready_resolve_wait_change() blocks */
static int64_t wait_ms(k_timeout_t timeout, int expected)
{
	int64_t start = k_uptime_get();

	zassert_equal(ready_resolve_wait_change(&ctx, timeout), expected);

	return k_uptime_get() - start;
}

static void *suite_setup(void)
{
	socket_offload_dns_register(&fake_dns);
	return NULL;
}

static void before(void *f)
{
	struct k_work_sync sync;

	k_work_cancel_delayable_sync(&delayed_work, &sync);
	set_state(false, false, false);
	memset(&fake, 0, sizeof(fake));
	memset(&ctx, 0, sizeof(ctx));
}

static void after(void *f)
{
	ready_resolve_free(&ctx);
}

ZTEST(ready_resolve, test_dual_stack)
{
	set_state(true, true, true);

	zassert_ok(resolve());
	zassert_equal(fake.last_family, NET_AF_UNSPEC);
	zassert_equal(fake.last_socktype, NET_SOCK_DGRAM);
	zassert_str_equal(next_ip(PORT), "2001:db8::1");
	zassert_str_equal(next_ip(PORT), "2001:db8::2");
	zassert_str_equal(next_ip(PORT), "192.0.2.1");
	assert_exhausted();
}

ZTEST(ready_resolve, test_ipv4_only)
{
	set_state(true, false, true);

	zassert_ok(resolve());
	zassert_equal(fake.last_family, NET_AF_INET);
	zassert_str_equal(next_ip(PORT), "192.0.2.1");
	assert_exhausted();
}

ZTEST(ready_resolve, test_ipv6_only)
{
	set_state(false, true, true);

	zassert_ok(resolve());
	zassert_equal(fake.last_family, NET_AF_INET6);
	zassert_str_equal(next_ip(PORT), "2001:db8::1");
	zassert_str_equal(next_ip(PORT), "2001:db8::2");
	assert_exhausted();
}

/* Resolver returns both families, only addresses of families up are returned */
ZTEST(ready_resolve, test_skip_down_family)
{
	set_state(true, false, true);
	fake.ignore_hints = true;

	zassert_ok(resolve());
	zassert_str_equal(next_ip(PORT), "192.0.2.1");
	assert_exhausted();
}

/* A family going down while iterating is skipped from then on */
ZTEST(ready_resolve, test_family_lost_while_iterating)
{
	set_state(true, true, true);

	zassert_ok(resolve());
	zassert_str_equal(next_ip(PORT), "2001:db8::1");
	notify(NET_EVENT_L4_IPV6_DISCONNECTED);
	zassert_str_equal(next_ip(PORT), "192.0.2.1");
	assert_exhausted();
}

ZTEST(ready_resolve, test_not_ready)
{
	zassert_equal(resolve(), -ENETDOWN);

	set_state(true, true, false);
	zassert_equal(resolve(), -ENETDOWN);

	set_state(false, false, true);
	zassert_equal(resolve(), -ENETDOWN);

	zassert_equal(fake.getaddrinfo_calls, 0);
	assert_exhausted();
}

ZTEST(ready_resolve, test_numeric_host_without_dns)
{
	set_state(true, false, false);

	zassert_ok(ready_resolve(&ctx, "192.0.2.7", NULL, &hints, PORT));
	zassert_str_equal(next_ip(PORT), "192.0.2.7");
	assert_exhausted();
}

ZTEST(ready_resolve, test_resolve_errors)
{
	set_state(true, true, true);

	fake.resolve_err = DNS_EAI_AGAIN;
	zassert_equal(resolve(), -EAGAIN);
	fake.resolve_err = DNS_EAI_MEMORY;
	zassert_equal(resolve(), -ENOMEM);
	fake.resolve_err = DNS_EAI_FAIL;
	zassert_equal(resolve(), -EHOSTUNREACH);
	fake.resolve_err = DNS_EAI_NONAME;
	zassert_equal(resolve(), -EHOSTUNREACH);
	assert_exhausted();
}

/* Resolving again starts over, the previous result is freed */
ZTEST(ready_resolve, test_resolve_again)
{
	set_state(true, true, true);

	zassert_ok(resolve());
	zassert_str_equal(next_ip(PORT), "2001:db8::1");
	zassert_ok(resolve());
	zassert_equal(fake.freeaddrinfo_calls, 1);
	zassert_str_equal(next_ip(PORT), "2001:db8::1");

	/* A failed resolve also frees */
	fake.resolve_err = DNS_EAI_FAIL;
	zassert_equal(resolve(), -EHOSTUNREACH);
	zassert_equal(fake.freeaddrinfo_calls, 2);
	assert_exhausted();
}

ZTEST(ready_resolve, test_free)
{
	/* Zeroed context */
	ready_resolve_free(&ctx);
	zassert_equal(fake.freeaddrinfo_calls, 0);
	assert_exhausted();

	set_state(true, true, true);
	zassert_ok(resolve());
	ready_resolve_free(&ctx);
	ready_resolve_free(&ctx);
	zassert_equal(fake.freeaddrinfo_calls, 1);
	assert_exhausted();
}

/* The port is set from the argument, or kept from the service */
ZTEST(ready_resolve, test_service_and_flags)
{
	const struct zsock_addrinfo flag_hints = {
		.ai_socktype = NET_SOCK_STREAM,
		.ai_flags = 0x1000,
	};

	set_state(true, true, true);

	zassert_ok(ready_resolve(&ctx, HOST, "1234", &flag_hints, 0));
	zassert_str_equal(fake.last_service, "1234");
	zassert_equal(fake.last_flags, 0x1000);
	zassert_equal(fake.last_socktype, NET_SOCK_STREAM);
	zassert_str_equal(next_ip(1234), "2001:db8::1");

	zassert_ok(ready_resolve(&ctx, HOST, "1234", NULL, PORT));
	zassert_str_equal(next_ip(PORT), "2001:db8::1");
}

ZTEST(ready_resolve, test_family)
{
	const struct zsock_addrinfo ipv4_hints = {
		.ai_family = NET_AF_INET,
		.ai_socktype = NET_SOCK_DGRAM,
	};

	/* Wanted family down */
	set_state(false, true, true);
	zassert_equal(ready_resolve(&ctx, HOST, NULL, &ipv4_hints, PORT), -ENETDOWN);

	/* Other family addresses are skipped even if returned */
	set_state(true, true, true);
	fake.ignore_hints = true;
	zassert_ok(ready_resolve(&ctx, HOST, NULL, &ipv4_hints, PORT));
	zassert_equal(fake.last_family, NET_AF_INET);
	zassert_str_equal(next_ip(PORT), "192.0.2.1");
	assert_exhausted();

	/* Other family changes do not wake the wait */
	notify_later(NET_EVENT_L4_IPV6_DISCONNECTED, NET_EVENT_L4_IPV6_CONNECTED, K_MSEC(50));
	zassert_true(wait_ms(K_MSEC(300), -EAGAIN) >= 300);
}

ZTEST(ready_resolve, test_invalid)
{
	const struct zsock_addrinfo bad_hints = {
		.ai_family = NET_AF_PACKET,
	};
	const struct net_sockaddr *sa;
	net_socklen_t len;

	set_state(true, true, true);

	zassert_equal(ready_resolve(NULL, HOST, NULL, NULL, PORT), -EINVAL);
	zassert_equal(ready_resolve(&ctx, NULL, NULL, NULL, PORT), -EINVAL);
	zassert_equal(ready_resolve(&ctx, HOST, NULL, &bad_hints, PORT), -EINVAL);
	zassert_equal(ready_resolve_next(&ctx, NULL, &len), -EINVAL);
	zassert_equal(ready_resolve_next(&ctx, &sa, NULL), -EINVAL);
	zassert_equal(fake.getaddrinfo_calls, 0);

	/* No busy loop on a context that was never resolved */
	zassert_true(wait_ms(K_MSEC(100), -EAGAIN) >= 100);
}

ZTEST(ready_resolve, test_wait_timeout)
{
	set_state(true, true, true);
	zassert_ok(resolve());

	zassert_true(wait_ms(K_MSEC(200), -EAGAIN) >= 200);
}

ZTEST(ready_resolve, test_wait_change)
{
	zassert_equal(resolve(), -ENETDOWN);

	/* Gain already happened: returns immediately */
	set_state(true, true, true);
	zassert_true(wait_ms(K_SECONDS(5), 0) < 100);

	/* A loss alone does not wake */
	zassert_ok(resolve());
	notify_later(NET_EVENT_L4_IPV4_DISCONNECTED, 0, K_MSEC(50));
	zassert_true(wait_ms(K_MSEC(300), -EAGAIN) >= 300);

	/* A gain while waiting wakes */
	notify_later(NET_EVENT_L4_IPV4_CONNECTED, 0, K_MSEC(100));
	zassert_true(wait_ms(K_SECONDS(5), 0) < 1000);
}

/* A down/up flap of the family in use wakes the wait */
ZTEST(ready_resolve, test_wait_flap)
{
	set_state(true, true, true);
	zassert_ok(resolve());

	notify(NET_EVENT_L4_IPV6_DISCONNECTED);
	notify(NET_EVENT_L4_IPV6_CONNECTED);
	zassert_true(wait_ms(K_SECONDS(5), 0) < 100);
}

/* IPv6 comes up first and doesn't work, IPv4 comes up later and works */
ZTEST(ready_resolve, test_recover_when_second_family_comes_up)
{
	set_state(false, true, true);
	zassert_ok(resolve());
	zassert_str_equal(next_ip(PORT), "2001:db8::1");
	zassert_str_equal(next_ip(PORT), "2001:db8::2");
	assert_exhausted();

	notify_later(NET_EVENT_L4_IPV4_CONNECTED, 0, K_MSEC(200));
	zassert_true(wait_ms(K_SECONDS(10), 0) < 1000, "did not react to IPv4 coming up");

	zassert_ok(resolve());
	zassert_equal(fake.last_family, NET_AF_UNSPEC);
	zassert_str_equal(next_ip(PORT), "2001:db8::1");
	zassert_str_equal(next_ip(PORT), "2001:db8::2");
	zassert_str_equal(next_ip(PORT), "192.0.2.1");
}

static bool readiness(const void *state)
{
	return IS_ENABLED(CONFIG_READY_RESOLVE_TRACK_CONN_MGR);
}

ZTEST_SUITE(ready_resolve, readiness, suite_setup, before, after, NULL);

#if defined(CONFIG_READY_RESOLVE_TRACK_MODEM)
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

static void modem_resolve(const char *addrs, int family)
{
	cgpaddr = addrs;
	fake.getaddrinfo_calls = 0;
	zassert_ok(resolve());
	zassert_equal(fake.getaddrinfo_calls, 1);
	zassert_equal(fake.last_family, family);
}

/* The families up are read from the modem's addresses */
ZTEST(ready_resolve_modem, test_modem_state)
{
	zassert_equal(resolve(), -ENETDOWN);
	cgpaddr = CGPADDR_NONE;
	zassert_equal(resolve(), -ENETDOWN);
	zassert_equal(fake.getaddrinfo_calls, 0);

	modem_resolve(CGPADDR_V4, NET_AF_INET);
	modem_resolve(CGPADDR_V6, NET_AF_INET6);
	modem_resolve(CGPADDR_BOTH, NET_AF_UNSPEC);
}

static void modem_up_fn(struct k_work *work)
{
	cgpaddr = CGPADDR_BOTH;
}

static K_WORK_DELAYABLE_DEFINE(modem_up_work, modem_up_fn);

/* Waiting polls the modem until a family comes up, or timeout */
ZTEST(ready_resolve_modem, test_modem_wait_change)
{
	cgpaddr = CGPADDR_V4;
	zassert_ok(resolve());

	scanf_calls = 0;
	zassert_true(wait_ms(K_MSEC(100), -EAGAIN) >= 100);
	zassert_true(scanf_calls > 2, "should poll");

	k_work_schedule(&modem_up_work, K_MSEC(50));
	zassert_true(wait_ms(K_SECONDS(1), 0) < 500, "should wake on change");
}

static bool modem_readiness(const void *state)
{
	return true;
}

ZTEST_SUITE(ready_resolve_modem, modem_readiness, suite_setup, modem_before, after, NULL);
#endif /* CONFIG_READY_RESOLVE_TRACK_MODEM */
