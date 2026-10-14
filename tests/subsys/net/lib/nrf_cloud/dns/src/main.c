/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <net/connect_by_name.h>

#include "nrf_cloud_dns.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, connect_by_name_init, struct connect_by_name *,
		const struct connect_by_name_cfg *);
FAKE_VALUE_FUNC(int, connect_by_name, struct connect_by_name *);
FAKE_VOID_FUNC(connect_by_name_deinit, struct connect_by_name *);

#define HOST "mqtt.nrfcloud.com"
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

static struct connect_by_name ctx;
static struct connect_by_name_cfg cfg;

static void before(void *f)
{
	RESET_FAKE(connect_by_name_init);
	RESET_FAKE(connect_by_name);
	RESET_FAKE(connect_by_name_deinit);
	cfg = (struct connect_by_name_cfg){ .host = HOST, .port = 8883 };
}

/* The configured family and host are used, the context stays initialized on success */
ZTEST(nrf_cloud_dns, test_connect_ok)
{
	connect_by_name_fake.return_val = SOCK;

	zassert_equal(nrf_cloud_connect_host(&ctx, &cfg), SOCK);
	zassert_str_equal(cfg.host, EXPECTED_HOST);
	zassert_equal(cfg.family, EXPECTED_FAMILY);
	zassert_equal(connect_by_name_init_fake.call_count, 1);
	zassert_equal_ptr(connect_by_name_init_fake.arg0_val, &ctx);
	zassert_equal_ptr(connect_by_name_init_fake.arg1_val, &cfg);
	zassert_equal(connect_by_name_fake.call_count, 1);
	zassert_equal(connect_by_name_deinit_fake.call_count, 0);
}

ZTEST(nrf_cloud_dns, test_init_fails)
{
	connect_by_name_init_fake.return_val = -EINVAL;

	zassert_equal(nrf_cloud_connect_host(&ctx, &cfg), -EINVAL);
	zassert_equal(connect_by_name_fake.call_count, 0);
	zassert_equal(connect_by_name_deinit_fake.call_count, 0);
}

/* The context is deinitialized when the connection fails */
ZTEST(nrf_cloud_dns, test_connect_fails)
{
	connect_by_name_fake.return_val = -ENETDOWN;

	zassert_equal(nrf_cloud_connect_host(&ctx, &cfg), -ENETDOWN);
	zassert_equal(connect_by_name_deinit_fake.call_count, 1);
	zassert_equal_ptr(connect_by_name_deinit_fake.arg0_val, &ctx);
}

ZTEST_SUITE(nrf_cloud_dns, NULL, NULL, before, NULL, NULL);
