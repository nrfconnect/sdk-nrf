/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <net/ready_resolve.h>

#include "nrf_cloud_dns.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, ready_resolve, struct ready_resolve *, const char *, const char *,
		const struct zsock_addrinfo *, uint16_t);
FAKE_VALUE_FUNC(int, ready_resolve_next, struct ready_resolve *, const struct net_sockaddr **,
		net_socklen_t *);
FAKE_VOID_FUNC(ready_resolve_free, struct ready_resolve *);
FAKE_VALUE_FUNC(int, connect_cb, const struct net_sockaddr *, net_socklen_t);

#define HOST "mqtt.nrfcloud.com"
#define PORT 8883
#define SOCK 3
#define ADDRS 2

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

static struct net_sockaddr addrs[ADDRS];
static int next_idx;
static char resolved_host[32];
static struct zsock_addrinfo resolved_hints;

static int resolve_fake(struct ready_resolve *ctx, const char *host, const char *service,
			const struct zsock_addrinfo *hints, uint16_t port)
{
	strncpy(resolved_host, host, sizeof(resolved_host) - 1);
	resolved_hints = *hints;
	return 0;
}

static int next_fake(struct ready_resolve *ctx, const struct net_sockaddr **addr,
		     net_socklen_t *addrlen)
{
	if (next_idx == ADDRS) {
		return -ENOENT;
	}

	*addr = &addrs[next_idx++];
	*addrlen = sizeof(struct net_sockaddr);
	return 0;
}

static void before(void *f)
{
	RESET_FAKE(ready_resolve);
	RESET_FAKE(ready_resolve_next);
	RESET_FAKE(ready_resolve_free);
	RESET_FAKE(connect_cb);
	ready_resolve_fake.custom_fake = resolve_fake;
	ready_resolve_next_fake.custom_fake = next_fake;
	next_idx = 0;
	memset(resolved_host, 0, sizeof(resolved_host));
}

/* The configured host and family are resolved, the first address that connects is used */
ZTEST(nrf_cloud_dns, test_connect_ok)
{
	connect_cb_fake.return_val = SOCK;

	zassert_equal(nrf_cloud_connect_host(HOST, PORT, NET_SOCK_STREAM, connect_cb), SOCK);
	zassert_str_equal(resolved_host, EXPECTED_HOST);
	zassert_equal(resolved_hints.ai_family, EXPECTED_FAMILY);
	zassert_equal(resolved_hints.ai_socktype, NET_SOCK_STREAM);
	zassert_equal(ready_resolve_fake.arg4_val, PORT);
	zassert_equal(connect_cb_fake.call_count, 1);
	zassert_equal_ptr(connect_cb_fake.arg0_val, &addrs[0]);
	zassert_equal(ready_resolve_free_fake.call_count, 1);
}

ZTEST(nrf_cloud_dns, test_try_next)
{
	static int ret[] = { -ECONNREFUSED, SOCK };

	SET_RETURN_SEQ(connect_cb, ret, ARRAY_SIZE(ret));

	zassert_equal(nrf_cloud_connect_host(HOST, PORT, NET_SOCK_STREAM, connect_cb), SOCK);
	zassert_equal(connect_cb_fake.call_count, 2);
	zassert_equal_ptr(connect_cb_fake.arg0_val, &addrs[1]);
	zassert_equal(ready_resolve_free_fake.call_count, 1);
}

/* The last connection error is returned */
ZTEST(nrf_cloud_dns, test_all_fail)
{
	static int ret[] = { -ECONNREFUSED, -ETIMEDOUT };

	SET_RETURN_SEQ(connect_cb, ret, ARRAY_SIZE(ret));

	zassert_equal(nrf_cloud_connect_host(HOST, PORT, NET_SOCK_STREAM, connect_cb),
		      -ETIMEDOUT);
	zassert_equal(connect_cb_fake.call_count, ADDRS);
	zassert_equal(ready_resolve_free_fake.call_count, 1);
}

ZTEST(nrf_cloud_dns, test_resolve_fails)
{
	ready_resolve_fake.custom_fake = NULL;
	ready_resolve_fake.return_val = -ENETDOWN;

	zassert_equal(nrf_cloud_connect_host(HOST, PORT, NET_SOCK_STREAM, connect_cb), -ENETDOWN);
	zassert_equal(connect_cb_fake.call_count, 0);
	zassert_equal(ready_resolve_free_fake.call_count, 1);
}

ZTEST_SUITE(nrf_cloud_dns, NULL, NULL, before, NULL, NULL);
