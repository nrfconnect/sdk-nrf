/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/settings/settings.h>
#include <net/nrf_cloud.h>
#include <net/connect_by_name.h>

#include "nrf_cloud_transport.h"
#include "nrf_cloud_dns.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_client_id.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, nrf_cloud_connect_host, struct connect_by_name *,
		struct connect_by_name_cfg *);
FAKE_VOID_FUNC(connect_by_name_deinit, struct connect_by_name *);
FAKE_VOID_FUNC(mqtt_client_init, struct mqtt_client *);
FAKE_VALUE_FUNC(int, mqtt_connect, struct mqtt_client *);
FAKE_VALUE_FUNC(int, mqtt_abort, struct mqtt_client *);

/* Only needed to link */
FAKE_VALUE_FUNC(int, mqtt_publish_qos1_ack, struct mqtt_client *,
		const struct mqtt_puback_param *);
FAKE_VALUE_FUNC(int, mqtt_readall_publish_payload, struct mqtt_client *, uint8_t *, size_t);
FAKE_VALUE_FUNC(int, nct_input, const struct nct_evt *);
FAKE_VALUE_FUNC(int, nrf_cloud_disconnect);
FAKE_VALUE_FUNC(int, settings_save_one, const char *, const void *, size_t);
FAKE_VALUE_FUNC(int, nrf_cloud_client_id_runtime_set, const char *const);
FAKE_VALUE_FUNC(sec_tag_t, nrf_cloud_sec_tag_get);

int nrf_cloud_client_id_ptr_get(const char **client_id)
{
	*client_id = "test-device";
	return 0;
}

void *nrf_cloud_calloc(size_t count, size_t size)
{
	return k_calloc(count, size);
}

void nrf_cloud_free(void *memory)
{
	k_free(memory);
}

static struct connect_by_name *conn_ctx;
static struct connect_by_name_cfg *conn_cfg;

static int connect_host_fake(struct connect_by_name *ctx, struct connect_by_name_cfg *cfg)
{
	struct net_sockaddr_in addr = { .sin_family = NET_AF_INET };

	conn_ctx = ctx;
	conn_cfg = cfg;

	return cfg->connect_cb((struct net_sockaddr *)&addr, sizeof(addr), cfg->user_data);
}

static void before(void *f)
{
	struct nrf_cloud_init_param param = {};

	zassert_ok(nct_initialize(&param));
	RESET_FAKE(nrf_cloud_connect_host);
	RESET_FAKE(connect_by_name_deinit);
	RESET_FAKE(mqtt_client_init);
	RESET_FAKE(mqtt_connect);
	RESET_FAKE(mqtt_abort);
	nrf_cloud_connect_host_fake.custom_fake = connect_host_fake;

	zassert_ok(nct_connect());
	zassert_equal(mqtt_connect_fake.call_count, 1);
	zassert_not_null(conn_cfg->lost_cb);
}

static void after(void *f)
{
	nct_uninit();
}

/* Losing connectivity aborts the MQTT connection */
ZTEST(nrf_cloud_transport, test_lost_aborts)
{
	conn_cfg->lost_cb(conn_ctx, conn_cfg->user_data);
	k_msleep(10);

	zassert_equal(mqtt_abort_fake.call_count, 1);
	zassert_equal_ptr(mqtt_abort_fake.arg0_val, mqtt_connect_fake.arg0_val);
}

/* The connection context is released on uninit, so no more loss is reported */
ZTEST(nrf_cloud_transport, test_uninit_releases)
{
	RESET_FAKE(connect_by_name_deinit);
	nct_uninit();

	zassert_equal(connect_by_name_deinit_fake.call_count, 1);
	zassert_equal_ptr(connect_by_name_deinit_fake.arg0_val, conn_ctx);
}

ZTEST_SUITE(nrf_cloud_transport, NULL, NULL, before, after, NULL);
