/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <../subsys/bluetooth/audio/bap_endpoint.h>
#include <../subsys/bluetooth/audio/bap_iso.h>
#include "server_store.h"

#define TEST_CAP_STREAM(name)                                                                      \
	struct bt_cap_stream test_##name##_cap_stream;                                             \
	struct bt_bap_ep test_##name##_ep_var = {0};                                               \
	struct bt_bap_iso test_##name##_bap_iso = {0};                                             \
	struct bt_bap_qos_cfg test_##name##_qos = {0};                                             \
	test_##name##_ep_var.dir = 1;                                                              \
	test_##name##_cap_stream.bap_stream.ep = &test_##name##_ep_var;                            \
	test_##name##_cap_stream.bap_stream.bap_iso = &test_##name##_bap_iso;                      \
	test_##name##_cap_stream.bap_stream.group = (void *)0;                                     \
	test_##name##_cap_stream.bap_stream.qos = &test_##name##_qos;

ZTEST(suite_server_store, test_1_srv_store_init)
{
	int ret;

	ret = srv_store_init();
	zassert_equal(ret, 0, "Init did not return zero");

	ret = srv_store_num_get();
	zassert_equal(ret, 0, "Number of servers should be zero after init");

	uint32_t conn = 0x12345678;

	ret = srv_store_add((struct bt_conn *)conn);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_num_get();
	zassert_equal(ret, 1, "Number of servers should be one after adding a server");

	ret = srv_store_remove_all();
	zassert_equal(ret, 0, "Clearing all servers did not return zero");

	ret = srv_store_num_get();
	zassert_equal(ret, 0, "Number of servers should be zero after clearing");
}

ZTEST(suite_server_store, test_2_srv_store_multiple)
{
	int ret;

	ret = srv_store_init();
	zassert_equal(ret, 0, "Init did not return zero");

	uint32_t conn1 = 0x1000;
	uint32_t conn2 = 0x2000;
	uint32_t conn3 = 0x3000;
	uint32_t conn4 = 0x4000;

	ret = srv_store_add((struct bt_conn *)conn1);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_add((struct bt_conn *)conn2);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_add((struct bt_conn *)conn3);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_num_get();
	zassert_equal(ret, 3, "Number of servers should be three after adding three servers");

	struct server_store *retrieved_server = NULL;

	ret = srv_store_from_conn_get((struct bt_conn *)conn2, &retrieved_server);
	zassert_equal(ret, 0, "Retrieving server by connection did not return zero");
	zassert_not_null(retrieved_server, "Retrieved server should not be NULL");
	zassert_equal(retrieved_server->conn, (struct bt_conn *)conn2,
		      "Retrieved server connection does not match expected");

	ret = srv_store_from_conn_get((struct bt_conn *)conn4, &retrieved_server);
	zassert_equal(ret, -ENOENT, "Retrieving non-existing server should return -ENOENT");
	zassert_is_null(retrieved_server, "Retrieved server should be NULL for non-existing entry");
}

ZTEST(suite_server_store, test_3_srv_remove)
{
	Z_TEST_SKIP_IFNDEF(true);
	int ret;

	ret = srv_store_init();
	zassert_equal(ret, 0, "Init did not return zero");

	uint32_t conn1 = 0x1000;
	uint32_t conn2 = 0x2000;
	uint32_t conn3 = 0x3000;

	ret = srv_store_add((struct bt_conn *)conn1);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_add((struct bt_conn *)conn2);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_add((struct bt_conn *)conn3);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_num_get();
	zassert_equal(ret, 3, "Number of servers should be three after adding three servers");

	ret = srv_store_remove((struct bt_conn *)conn2);
	zassert_equal(ret, 0, "Removing server by connection did not return zero");

	ret = srv_store_num_get();
	zassert_equal(ret, 2, "Number of servers should be two after removing one");
}

ZTEST(suite_server_store, test_4_find_srv_from_stream)
{
	int ret;

	ret = srv_store_init();
	zassert_equal(ret, 0, "Init did not return zero");

	uint32_t conn0 = 0x1000;
	uint32_t conn1 = 0x2000;

	ret = srv_store_add((struct bt_conn *)conn0);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_add((struct bt_conn *)conn1);
	zassert_equal(ret, 0, "Adding server did not return zero");

	struct server_store *retrieved_server = NULL;

	ret = srv_store_from_conn_get((struct bt_conn *)conn0, &retrieved_server);
	zassert_equal(ret, 0, "Retrieving OK server by connection did not return zero");

	retrieved_server->name = "Test Server 0";
	retrieved_server->snk.cap_streams[0] = (struct bt_cap_stream *)0x0000;
	retrieved_server->snk.cap_streams[1] = (struct bt_cap_stream *)0x0111;
	retrieved_server->snk.cap_streams[2] = (struct bt_cap_stream *)0x0222;

	ret = srv_store_from_conn_get((struct bt_conn *)conn1, &retrieved_server);
	zassert_equal(ret, 0, "Retrieving OK server by connection did not return zero");

	retrieved_server->name = "Test Server 1";
	retrieved_server->snk.cap_streams[0] = (struct bt_cap_stream *)0x1000;
	retrieved_server->snk.cap_streams[1] = (struct bt_cap_stream *)0x1111;
	retrieved_server->snk.cap_streams[2] = (struct bt_cap_stream *)0x1222;

	ret = srv_store_from_stream_get((struct bt_cap_stream *)0x9999, &retrieved_server);
	zassert_equal(ret, -ENOENT, "Retrieving from NULL stream should return -ENOENT");
	zassert_is_null(retrieved_server, "Retrieved server should be NULL");

	ret = srv_store_from_stream_get((struct bt_cap_stream *)0x1111, &retrieved_server);
	zassert_equal(ret, 0, "Retrieving from stream should return zero");
	zassert_not_null(retrieved_server, "Retrieved server should not be NULL");
	zassert_equal(retrieved_server->name, "Test Server 1");
	zassert_equal_ptr(retrieved_server->conn, conn1,
			  "Retrieved server connection does not match expected");
	/* Testing illegal operation with idential cap_stream pointers */
	retrieved_server->snk.cap_streams[3] = (struct bt_cap_stream *)0x0111;

	ret = srv_store_from_stream_get((struct bt_cap_stream *)0x0111, &retrieved_server);
	zassert_equal(ret, -ESPIPE, "Retrieving from stream should return -ESPIPE");
}

ZTEST(suite_server_store, test_5_pres_dly_simple)
{
	int ret;

	ret = srv_store_init();
	zassert_equal(ret, 0, "Init did not return zero");

	TEST_CAP_STREAM(one);
	test_one_cap_stream.bap_stream.group = (void *)0xaaaa;

	struct bt_bap_qos_cfg_pref qos_cfg_pref_in;
	qos_cfg_pref_in.pd_min = 1000;
	qos_cfg_pref_in.pd_max = 4000;
	qos_cfg_pref_in.pref_pd_min = 2000;
	qos_cfg_pref_in.pref_pd_max = 3000;

	/* For this test, we have no other endpoints or streams stored
	 * This simulates getting the first call to find the presentation delay
	 */

	uint32_t computed_pres_dly_us = 0;
	bool group_reconfig_needed = false;

	ret = srv_store_pres_dly_find(&test_one_cap_stream.bap_stream, &computed_pres_dly_us,
				      &qos_cfg_pref_in, &group_reconfig_needed);

	zassert_equal(ret, 0, "Finding presentation delay did not return zero");
	zassert_equal(computed_pres_dly_us, 2000,
		      "Computed presentation delay should be equal to preferred min");

	/* Removing preferred min. Result should return min */
	qos_cfg_pref_in.pref_pd_min = 0;

	ret = srv_store_pres_dly_find(&test_one_cap_stream.bap_stream, &computed_pres_dly_us,
				      &qos_cfg_pref_in, &group_reconfig_needed);
	zassert_equal(ret, 0, "Finding presentation delay did not return zero");
	zassert_equal(computed_pres_dly_us, 1000,
		      "Computed presentation delay should be equal to preferred min");

	/* Removing min, should return error*/
	qos_cfg_pref_in.pd_min = 0;
	ret = srv_store_pres_dly_find(&test_one_cap_stream.bap_stream, &computed_pres_dly_us,
				      &qos_cfg_pref_in, &group_reconfig_needed);
	zassert_equal(ret, -EINVAL, "Finding presentation delay should return -EINVAL %d ", ret);
}

ZTEST(suite_server_store, test_6_pres_delay_advanced)
{
	int ret;

	uint32_t conn0 = 0x4000;

	ret = srv_store_init();
	zassert_equal(ret, 0, "Init did not return zero");

	ret = srv_store_add((struct bt_conn *)conn0);
	zassert_equal(ret, 0, "Adding server did not return zero");

	struct server_store *retrieved_server = NULL;

	ret = srv_store_from_conn_get((struct bt_conn *)conn0, &retrieved_server);
	zassert_equal(ret, 0, "Retrieving server by connection did not return zero");
	zassert_not_null(retrieved_server, "Retrieved server should not be NULL");
	zassert_equal(retrieved_server->conn, (struct bt_conn *)conn0,
		      "Retrieved server connection does not match expected");

	TEST_CAP_STREAM(one);
	test_one_cap_stream.bap_stream.ep->qos_pref.pd_min = 1000;
	test_one_cap_stream.bap_stream.ep->qos_pref.pd_max = 4000;
	test_one_cap_stream.bap_stream.ep->qos_pref.pref_pd_min = 2000;
	test_one_cap_stream.bap_stream.ep->qos_pref.pref_pd_max = 3000;
	test_one_cap_stream.bap_stream.group = (void *)0xaaaa;

	retrieved_server->snk.cap_streams[0] = &test_one_cap_stream;

	TEST_CAP_STREAM(two);
	test_one_cap_stream.bap_stream.group = (void *)0xaaaa;

	struct bt_bap_qos_cfg_pref qos_cfg_pref_in_two;

	qos_cfg_pref_in_two.pd_min = 1100;
	qos_cfg_pref_in_two.pd_max = 4000;
	qos_cfg_pref_in_two.pref_pd_min = 2100;
	qos_cfg_pref_in_two.pref_pd_max = 3000;

	uint32_t computed_pres_dly_us = 0;
	bool group_reconfig_needed = false;

	ret = srv_store_pres_dly_find(&test_one_cap_stream.bap_stream, &computed_pres_dly_us,
				      &qos_cfg_pref_in_two, &group_reconfig_needed);
	zassert_equal(ret, 0, "Finding presentation delay did not return zero %d", ret);
	zassert_equal(computed_pres_dly_us, 2100,
		      "Computed presentation delay should be equal to preferred min %d",
		      computed_pres_dly_us);
}

ZTEST_SUITE(suite_server_store, NULL, NULL, NULL, NULL, NULL);
