/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/net/socket_offload.h>

#include "nrf_cloud_dns.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, dns_getaddrinfo, const char *, const char *, const struct zsock_addrinfo *,
		struct zsock_addrinfo **);
FAKE_VOID_FUNC(dns_freeaddrinfo, struct zsock_addrinfo *);
FAKE_VALUE_FUNC(int, connect_cb, const struct net_sockaddr *, net_socklen_t);

/* Resolves through the offloaded DNS API, so results can be injected */
static const struct socket_dns_offload dns_ops = {
	.getaddrinfo = dns_getaddrinfo,
	.freeaddrinfo = dns_freeaddrinfo,
};

#define HOST "mqtt.nrfcloud.com"
#define PORT 8883
#define SOCK 3

#if defined(CONFIG_TEST_NRF_CLOUD_STATIC_IPV4)
#define EXPECTED_HOST   "192.0.2.1"
#define EXPECTED_FAMILY NET_AF_INET
#elif defined(CONFIG_TEST_NRF_CLOUD_IPV6)
#define EXPECTED_HOST   HOST
#define EXPECTED_FAMILY NET_AF_INET6
#else
#define EXPECTED_HOST   HOST
#define EXPECTED_FAMILY NET_AF_UNSPEC
#endif

static struct net_sockaddr_in6 addr6 = { .sin6_family = NET_AF_INET6 };
static struct net_sockaddr_in addr4 = { .sin_family = NET_AF_INET };
static struct zsock_addrinfo ai4 = {
	.ai_family = NET_AF_INET,
	.ai_addr = (struct net_sockaddr *)&addr4,
	.ai_addrlen = sizeof(addr4),
};
static struct zsock_addrinfo ai6 = {
	.ai_family = NET_AF_INET6,
	.ai_addr = (struct net_sockaddr *)&addr6,
	.ai_addrlen = sizeof(addr6),
	.ai_next = &ai4,
};
static char resolved_host[32];
static struct zsock_addrinfo resolved_hints;

/* Returns an IPv6 then an IPv4 address */
static int getaddrinfo_fake(const char *host, const char *service,
			    const struct zsock_addrinfo *hints, struct zsock_addrinfo **res)
{
	strncpy(resolved_host, host, sizeof(resolved_host) - 1);
	resolved_hints = *hints;
	*res = &ai6;
	return 0;
}

static void *setup(void)
{
	socket_offload_dns_register(&dns_ops);
	return NULL;
}

static void before(void *f)
{
	RESET_FAKE(dns_getaddrinfo);
	RESET_FAKE(dns_freeaddrinfo);
	RESET_FAKE(connect_cb);
	dns_getaddrinfo_fake.custom_fake = getaddrinfo_fake;
	memset(resolved_host, 0, sizeof(resolved_host));
	addr4.sin_port = 0;
	addr6.sin6_port = 0;
}

/* The configured host and family are resolved, the first address that connects is used */
ZTEST(nrf_cloud_dns, test_connect_ok)
{
	connect_cb_fake.return_val = SOCK;

	zassert_equal(nrf_cloud_connect_host(HOST, PORT, NET_SOCK_STREAM, connect_cb), SOCK);
	zassert_str_equal(resolved_host, EXPECTED_HOST);
	zassert_equal(resolved_hints.ai_family, EXPECTED_FAMILY);
	zassert_equal(resolved_hints.ai_socktype, NET_SOCK_STREAM);
	zassert_equal(connect_cb_fake.call_count, 1);
	zassert_equal_ptr(connect_cb_fake.arg0_val, &addr6);
	zassert_equal(connect_cb_fake.arg1_val, sizeof(addr6));
	zassert_equal(net_ntohs(addr6.sin6_port), PORT);
	zassert_equal(dns_freeaddrinfo_fake.call_count, 1);
}

ZTEST(nrf_cloud_dns, test_try_next)
{
	static int ret[] = { -ECONNREFUSED, SOCK };

	SET_RETURN_SEQ(connect_cb, ret, ARRAY_SIZE(ret));

	zassert_equal(nrf_cloud_connect_host(HOST, PORT, NET_SOCK_STREAM, connect_cb), SOCK);
	zassert_equal(connect_cb_fake.call_count, 2);
	zassert_equal_ptr(connect_cb_fake.arg0_val, &addr4);
	zassert_equal(net_ntohs(addr4.sin_port), PORT);
	zassert_equal(dns_freeaddrinfo_fake.call_count, 1);
}

/* The last connection error is returned */
ZTEST(nrf_cloud_dns, test_all_fail)
{
	static int ret[] = { -ECONNREFUSED, -ETIMEDOUT };

	SET_RETURN_SEQ(connect_cb, ret, ARRAY_SIZE(ret));

	zassert_equal(nrf_cloud_connect_host(HOST, PORT, NET_SOCK_STREAM, connect_cb),
		      -ETIMEDOUT);
	zassert_equal(connect_cb_fake.call_count, 2);
	zassert_equal(dns_freeaddrinfo_fake.call_count, 1);
}

ZTEST(nrf_cloud_dns, test_resolve_fails)
{
	dns_getaddrinfo_fake.custom_fake = NULL;
	dns_getaddrinfo_fake.return_val = DNS_EAI_AGAIN;

	zassert_equal(nrf_cloud_connect_host(HOST, PORT, NET_SOCK_STREAM, connect_cb), -EAGAIN);
	zassert_equal(connect_cb_fake.call_count, 0);
	zassert_equal(dns_freeaddrinfo_fake.call_count, 0);
}

ZTEST_SUITE(nrf_cloud_dns, NULL, setup, before, NULL, NULL);
