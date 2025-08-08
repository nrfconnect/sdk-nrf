/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#include "server_store.h"

ZTEST(suite_server_store, test_srv_store_init)
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

	ret = srv_store_clear_all();
	zassert_equal(ret, 0, "Clearing all servers did not return zero");

	ret = srv_store_num_get();
	zassert_equal(ret, 0, "Number of servers should be zero after clearing");
}

ZTEST(suite_server_store, test_srv_store_multiple)
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
	TC_PRINT("ret = %d\n", ret);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_add((struct bt_conn *)conn3);
	TC_PRINT("ret = %d\n", ret);
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

ZTEST(suite_server_store, test_srv_remove)
{
	int ret;

	ret = srv_store_init();
	zassert_equal(ret, 0, "Init did not return zero");

	uint32_t conn1 = 0x1000;
	uint32_t conn2 = 0x2000;
	uint32_t conn3 = 0x3000;

	ret = srv_store_add((struct bt_conn *)conn1);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_add((struct bt_conn *)conn2);
	TC_PRINT("ret = %d\n", ret);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_add((struct bt_conn *)conn3);
	TC_PRINT("ret = %d\n", ret);
	zassert_equal(ret, 0, "Adding server did not return zero");

	ret = srv_store_num_get();
	zassert_equal(ret, 3, "Number of servers should be three after adding three servers");

	ret = srv_store_remove((struct bt_conn *)conn2);
	zassert_equal(ret, 0, "Removing server by connection did not return zero");

	ret = srv_store_num_get();
	zassert_equal(ret, 2, "Number of servers should be two after removing one");
}

ZTEST_SUITE(suite_server_store, NULL, NULL, NULL, NULL, NULL);
