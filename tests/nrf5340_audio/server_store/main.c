/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/tc_util.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <../subsys/bluetooth/audio/bap_endpoint.h>
#include <../subsys/bluetooth/audio/bap_iso.h>
#include <../subsys/bluetooth/host/conn_internal.h>
#include "server_store.h"

#define TEST_CAP_STREAM(val)                                                                       \
	struct bt_cap_stream test_##val##_cap_stream;                                              \
	struct bt_bap_ep test_##val##_ep_var = {0};                                                \
	struct bt_iso_chan test_##val##_bap_iso = {0};                                             \
	struct bt_bap_qos_cfg test_##val##_qos = {0};                                              \
	test_##val##_qos.pd = val;                                                                 \
	test_##val##_ep_var.dir = 1;                                                               \
	test_##val##_cap_stream.bap_stream.ep = &test_##val##_ep_var;                              \
	test_##val##_cap_stream.bap_stream.iso = &test_##val##_bap_iso;                            \
	test_##val##_cap_stream.bap_stream.group = (void *)0;                                      \
	test_##val##_cap_stream.bap_stream.qos = &test_##val##_qos;

#define TEST_CONN(val)                                                                             \
	struct bt_conn test_##val##_conn = {                                                       \
		.handle = val,                                                                     \
		.type = BT_CONN_TYPE_LE,                                                           \
		.id = val,                                                                         \
		.state = BT_CONN_STATE_CONNECTED,                                                  \
		.le = {.dst = {.type = BT_ADDR_LE_PUBLIC,                                          \
			       .a = {{val, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}}}}};

static struct bt_bap_lc3_preset lc3_preset_48_4_1 = BT_BAP_LC3_UNICAST_PRESET_48_4_1(
	BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED));
static struct bt_bap_lc3_preset lc3_preset_24_2_1 = BT_BAP_LC3_UNICAST_PRESET_24_2_1(
	BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED));
static struct bt_bap_lc3_preset lc3_preset_16_2_1 = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED));

ZTEST(suite_server_store, test_srv_store_init)
{
	int ret;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_num_get(true);
	zassert_equal(ret, 0);

	TEST_CONN(1);

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	ret = srv_store_num_get(true);
	zassert_equal(ret, 1, "Number of servers should be one after adding a server");

	bt_addr_le_t addr = {.type = BT_ADDR_LE_PUBLIC,
			     .a = {.val = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}}};

	bool exists = srv_store_server_exists(&addr);

	zassert_false(exists, "Server should not exist for non-added address");

	ret = srv_store_add_by_addr(&addr);
	zassert_equal(ret, 0);

	exists = srv_store_server_exists(&addr);
	zassert_true(exists, "Server should not exist for non-added address");

	ret = srv_store_num_get(true);
	zassert_equal(ret, 2, "Number of servers should be two after adding a server");

	ret = srv_store_remove_all();
	zassert_equal(ret, 0);

	ret = srv_store_num_get(true);
	zassert_equal(ret, 0);

	srv_store_unlock();
}

ZTEST(suite_server_store, test_srv_store_multiple)
{
	int ret;

	TEST_CONN(1);
	TEST_CONN(2);
	TEST_CONN(3);
	TEST_CONN(4);

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_2_conn);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_3_conn);
	zassert_equal(ret, 0);

	ret = srv_store_num_get(true);
	zassert_equal(ret, 3, "Number of servers should be three after adding three servers");

	struct server_store *retr_server = NULL;

	ret = srv_store_from_conn_get(&test_2_conn, &retr_server);
	zassert_equal(ret, 0);
	zassert_not_null(retr_server, "Retrieved server should not be NULL");
	zassert_equal(retr_server->conn, &test_2_conn,
		      "Retrieved server connection does not match expected");

	const bt_addr_le_t *peer_addr = bt_conn_get_dst(&test_2_conn);

	ret = srv_store_from_addr_get(peer_addr, &retr_server);
	zassert_equal(ret, 0);

	ret = srv_store_from_conn_get(&test_4_conn, &retr_server);
	zassert_equal(ret, -ENOENT, "Retrieving non-existing server should return -ENOENT");
	zassert_is_null(retr_server, "Retrieved server should be NULL for non-existing entry");

	srv_store_unlock();
}

ZTEST(suite_server_store, test_srv_store_pointer_check)
{
	int ret;

	TEST_CONN(1);
	TEST_CONN(2);
	TEST_CONN(3);

	struct server_store *retr_server1 = NULL;
	struct server_store *retr_server2 = NULL;
	struct server_store *retr_server3 = NULL;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_2_conn);
	zassert_equal(ret, 0);
	ret = srv_store_from_conn_get(&test_2_conn, &retr_server2);
	zassert_equal(ret, 0);
	retr_server2->snk.num_codec_caps = 2;

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);
	ret = srv_store_from_conn_get(&test_2_conn, &retr_server2);
	zassert_equal(ret, 0);

	ret = srv_store_from_conn_get(&test_1_conn, &retr_server1);
	zassert_equal(ret, 0);
	retr_server1->snk.num_codec_caps = 1;

	ret = srv_store_add_by_conn(&test_3_conn);
	zassert_equal(ret, 0);
	ret = srv_store_from_conn_get(&test_3_conn, &retr_server3);
	zassert_equal(ret, 0);
	retr_server3->snk.num_codec_caps = 3;

	ret = srv_store_from_conn_get(&test_1_conn, &retr_server1);
	zassert_equal(ret, 0);

	ret = srv_store_from_conn_get(&test_3_conn, &retr_server3);
	zassert_equal(ret, 0);

	srv_store_unlock();
}

ZTEST(suite_server_store, test_srv_remove)
{
	int ret;

	TEST_CONN(0);
	TEST_CONN(1);
	TEST_CONN(2);

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_0_conn);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_2_conn);
	zassert_equal(ret, 0);

	ret = srv_store_num_get(true);
	zassert_equal(ret, 3, "Number of servers should be three after adding three servers");

	ret = srv_store_remove_by_conn(&test_2_conn);
	zassert_equal(ret, 0);

	ret = srv_store_num_get(true);
	zassert_equal(ret, 2, "Number of servers should be two after removing one");

	ret = srv_store_remove_by_conn(&test_0_conn);
	zassert_equal(ret, 0);

	/* Test with creating a gap in server store. */
	ret = srv_store_num_get(true);
	zassert_equal(ret, -EINVAL, "Number of servers should be two after removing one");

	ret = srv_store_num_get(false);
	zassert_equal(ret, 1, "Number of servers should be two after removing one");

	srv_store_unlock();
}

ZTEST(suite_server_store, test_find_srv_from_stream)
{
	int ret;

	TEST_CONN(1);
	TEST_CONN(2);

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_2_conn);
	zassert_equal(ret, 0);

	struct server_store *retr_server = NULL;

	ret = srv_store_from_conn_get(&test_1_conn, &retr_server);
	zassert_equal(ret, 0);

	retr_server->name = "Test Server 1";
	TEST_CAP_STREAM(1);
	memcpy(&retr_server->snk.cap_streams[0], &test_1_cap_stream, sizeof(test_1_cap_stream));
	TEST_CAP_STREAM(2);
	memcpy(&retr_server->snk.cap_streams[1], &test_2_cap_stream, sizeof(test_2_cap_stream));
	TEST_CAP_STREAM(3);
	memcpy(&retr_server->snk.cap_streams[2], &test_3_cap_stream, sizeof(test_3_cap_stream));

	ret = srv_store_from_conn_get(&test_2_conn, &retr_server);
	zassert_equal(ret, 0);

	retr_server->name = "Test Server 2";
	TEST_CAP_STREAM(4);
	memcpy(&retr_server->snk.cap_streams[0], &test_4_cap_stream, sizeof(test_4_cap_stream));
	TEST_CAP_STREAM(5);
	memcpy(&retr_server->snk.cap_streams[1], &test_5_cap_stream, sizeof(test_5_cap_stream));
	TEST_CAP_STREAM(6);
	memcpy(&retr_server->snk.cap_streams[2], &test_6_cap_stream, sizeof(test_6_cap_stream));

	struct bt_bap_stream *stream_pointer = &retr_server->snk.cap_streams[1].bap_stream;

	ret = srv_store_from_stream_get((struct bt_bap_stream *)0xDEADBEEF, &retr_server);
	zassert_equal(ret, -ENOENT, "Retrieving from non existing stream should return -ENOENT");
	zassert_is_null(retr_server, "Retrieved server should be NULL");

	TC_PRINT("test bap ptr %p\n", stream_pointer);

	struct server_store *found_server = NULL;

	ret = srv_store_from_stream_get(stream_pointer, &found_server);
	zassert_equal(ret, 0, "Retrieving from stream should return zero %d", ret);
	zassert_not_null(found_server, "Retrieved server should not be NULL");
	zassert_equal(found_server->name, "Test Server 2");
	zassert_equal_ptr(found_server->conn, &test_2_conn,
			  "Retrieved server connection does not match expected");

	srv_store_unlock();
}

ZTEST(suite_server_store, test_pres_dly_simple)
{
	int ret;

	TEST_CAP_STREAM(1);
	test_1_cap_stream.bap_stream.group = (void *)0xaaaa;

	struct bt_bap_qos_cfg_pref qos_cfg_pref_in;

	qos_cfg_pref_in.pd_min = 1000;
	qos_cfg_pref_in.pd_max = 4000;
	qos_cfg_pref_in.pref_pd_min = 2000;
	qos_cfg_pref_in.pref_pd_max = 3000;

	/* For this test, we have no other endpoints or streams stored
	 * This simulates getting the first call to find the presentation delay
	 */

	uint32_t computed_pres_dly_us = 0;
	uint32_t existing_pres_dly_us = 0;
	bool group_reconfig_needed = false;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_pres_dly_find(&test_1_cap_stream.bap_stream, &computed_pres_dly_us,
				      &existing_pres_dly_us, &qos_cfg_pref_in,
				      &group_reconfig_needed);

	zassert_equal(ret, 0);
	zassert_equal(computed_pres_dly_us, 2000,
		      "Computed presentation delay should be equal to preferred min");

	/* Removing preferred min. Result should return min */
	qos_cfg_pref_in.pref_pd_min = 0;

	ret = srv_store_pres_dly_find(&test_1_cap_stream.bap_stream, &computed_pres_dly_us,
				      &existing_pres_dly_us, &qos_cfg_pref_in,
				      &group_reconfig_needed);
	zassert_equal(ret, 0);
	zassert_equal(computed_pres_dly_us, 1000,
		      "Computed presentation delay should be equal to preferred min");

	/* Removing min, should return error*/
	qos_cfg_pref_in.pd_min = 0;
	ret = srv_store_pres_dly_find(&test_1_cap_stream.bap_stream, &computed_pres_dly_us,
				      &existing_pres_dly_us, &qos_cfg_pref_in,
				      &group_reconfig_needed);
	zassert_equal(ret, -EINVAL, "Finding presentation delay should return -EINVAL %d ", ret);

	/*TODO: Add test for existing_pres_dly_us*/
	srv_store_unlock();
}

ZTEST(suite_server_store, test_pres_delay_advanced)
{
	int ret;

	TEST_CONN(100);

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_100_conn);
	zassert_equal(ret, 0);

	struct server_store *retr_server = NULL;

	ret = srv_store_from_conn_get(&test_100_conn, &retr_server);
	zassert_equal(ret, 0);
	zassert_not_null(retr_server, "Retrieved server should not be NULL");
	zassert_equal(retr_server->conn, &test_100_conn,
		      "Retrieved server connection does not match expected");

	TEST_CAP_STREAM(1);
	test_1_cap_stream.bap_stream.ep->qos_pref.pd_min = 1000;
	test_1_cap_stream.bap_stream.ep->qos_pref.pd_max = 4000;
	test_1_cap_stream.bap_stream.ep->qos_pref.pref_pd_min = 2000;
	test_1_cap_stream.bap_stream.ep->qos_pref.pref_pd_max = 3000;
	test_1_cap_stream.bap_stream.group = (void *)0xaaaa;
	test_1_cap_stream.bap_stream.qos->pd = 2500;

	memcpy(&retr_server->snk.cap_streams[0], &test_1_cap_stream, sizeof(test_1_cap_stream));

	TEST_CAP_STREAM(2);

	struct bt_bap_qos_cfg_pref qos_cfg_pref_in;

	qos_cfg_pref_in.pd_min = 1100;
	qos_cfg_pref_in.pd_max = 4000;
	qos_cfg_pref_in.pref_pd_min = 2100;
	qos_cfg_pref_in.pref_pd_max = 3000;

	uint32_t computed_pres_dly_us = 0;
	uint32_t existing_pres_dly_us = 0;
	bool group_reconfig_needed = false;

	ret = srv_store_pres_dly_find(&test_1_cap_stream.bap_stream, &computed_pres_dly_us,
				      &existing_pres_dly_us, &qos_cfg_pref_in,
				      &group_reconfig_needed);
	zassert_equal(ret, 0, "Finding presentation delay did not return zero %d", ret);
	zassert_equal(computed_pres_dly_us, 2500, "Presentation delay should be unchanged %d",
		      computed_pres_dly_us);
	zassert_equal(group_reconfig_needed, false, "Group reconfiguration should not be needed");

	/* Testing with pref outside existing PD. Should not change existing streams*/

	qos_cfg_pref_in.pref_pd_min = 2600;
	ret = srv_store_pres_dly_find(&test_1_cap_stream.bap_stream, &computed_pres_dly_us,
				      &existing_pres_dly_us, &qos_cfg_pref_in,
				      &group_reconfig_needed);
	zassert_equal(ret, 0, "Finding presentation delay did not return zero %d", ret);
	zassert_equal(computed_pres_dly_us, 2500, "Presentation delay should be unchanged %d",
		      computed_pres_dly_us);
	zassert_equal(group_reconfig_needed, false, "Group reconfiguration should not be needed");

	/* Now check with min outside range. Shall trigger a reconfig*/
	qos_cfg_pref_in.pd_min = 2600;
	ret = srv_store_pres_dly_find(&test_1_cap_stream.bap_stream, &computed_pres_dly_us,
				      &existing_pres_dly_us, &qos_cfg_pref_in,
				      &group_reconfig_needed);
	zassert_equal(ret, 0, "Finding presentation delay did not return zero %d", ret);
	zassert_equal(computed_pres_dly_us, 2600, "Presentation delay should be unchanged %d",
		      computed_pres_dly_us);
	zassert_equal(group_reconfig_needed, true, "Group reconfiguration should not be needed");
	srv_store_unlock();
}

ZTEST(suite_server_store, test_pres_delay_multi_group)
{
	int ret;

	TEST_CONN(100);
	TEST_CONN(1);

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_100_conn);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	struct server_store *retr_server = NULL;

	ret = srv_store_from_conn_get(&test_100_conn, &retr_server);
	zassert_equal(ret, 0);
	zassert_not_null(retr_server, "Retrieved server should not be NULL");
	zassert_equal(retr_server->conn, &test_100_conn,
		      "Retrieved server connection does not match expected");

	TEST_CAP_STREAM(1);
	test_1_cap_stream.bap_stream.group = (void *)0xaaaa;
	test_1_cap_stream.bap_stream.qos->pd = 2000;

	memcpy(&retr_server->snk.cap_streams[0], &test_1_cap_stream, sizeof(test_1_cap_stream));

	/* Add stream in another group. Should be ignored */
	TEST_CAP_STREAM(2);
	test_2_cap_stream.bap_stream.group = (void *)0xbbbb;
	test_2_cap_stream.bap_stream.qos->pd = 500;

	memcpy(&retr_server->snk.cap_streams[1], &test_2_cap_stream, sizeof(test_2_cap_stream));

	struct bt_bap_qos_cfg_pref qos_cfg_pref_in;

	qos_cfg_pref_in.pd_min = 1100;
	qos_cfg_pref_in.pd_max = 4000;
	qos_cfg_pref_in.pref_pd_min = 2100;
	qos_cfg_pref_in.pref_pd_max = 3000;

	uint32_t computed_pres_dly_us = 0;
	uint32_t existing_pres_dly_us = 0;
	bool group_reconfig_needed = false;

	ret = srv_store_pres_dly_find(&test_1_cap_stream.bap_stream, &computed_pres_dly_us,
				      &existing_pres_dly_us, &qos_cfg_pref_in,
				      &group_reconfig_needed);
	zassert_equal(ret, 0, "Finding presentation delay did not return zero %d", ret);
	zassert_equal(computed_pres_dly_us, 2000, "Presentation delay should be unchanged %d",
		      computed_pres_dly_us);
	zassert_equal(group_reconfig_needed, false, "Group reconfiguration should not be needed");
	srv_store_unlock();
}

ZTEST(suite_server_store, test_cap_set)
{
	int ret;

	TEST_CONN(1);

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	struct bt_audio_codec_cap codec;

	codec.id = 0xaa;
	codec.data_len = 10;

	ret = srv_store_codec_cap_set(&test_1_conn, BT_AUDIO_DIR_SINK, &codec);
	zassert_equal(ret, 0, "Setting codec capabilities did not return zero %d", ret);

	srv_store_unlock();
}

ZTEST(suite_server_store, test_srv_get)
{
	int ret;

	TEST_CONN(100);
	TEST_CONN(1);

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_100_conn);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	struct server_store *server;

	ret = srv_store_server_get(&server, 0);
	zassert_equal(ret, 0);
	zassert_not_null(server, "Retrieved server should not be NULL");
	zassert_equal_ptr(server->conn, &test_100_conn,
			  "Retrieved server connection does not match expected");

	ret = srv_store_server_get(&server, 1);
	zassert_equal(ret, 0);
	zassert_not_null(server, "Retrieved server should not be NULL");
	zassert_equal_ptr(server->conn, &test_1_conn,
			  "Retrieved server connection does not match expected");

	ret = srv_store_server_get(&server, 2);
	zassert_equal(ret, -ENOENT, "Adding server did not return zero %d", ret);

	srv_store_unlock();
}

ZTEST(suite_server_store, test_preset_pref)
{
	int ret;

	int preferred_sample_rate_value = BT_AUDIO_CODEC_CFG_FREQ_48KHZ;
	bool valid = false;

	TEST_CONN(1);

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	struct server_store *server;

	ret = srv_store_server_get(&server, 0);
	zassert_equal(ret, 0);

	valid = srv_store_preset_validated(&lc3_preset_16_2_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);
	zassert_equal(valid, true, "16kHz preset should be valid even when pref is 48kHz");
	memcpy(&server->snk.lc3_preset[0], &lc3_preset_16_2_1, sizeof(server->snk.lc3_preset[0]));

	valid = srv_store_preset_validated(&lc3_preset_48_4_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);

	zassert_equal(valid, true, "48kHz preset should be valid when pref is 48kHz");
	memcpy(&server->snk.lc3_preset[0], &lc3_preset_48_4_1, sizeof(server->snk.lc3_preset[0]));

	valid = srv_store_preset_validated(&lc3_preset_24_2_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);

	zassert_equal(valid, false, "24kHz preset should be invalid when pref is 48kHz");

	/* Change preferred sample rate to 16kHz */
	preferred_sample_rate_value = BT_AUDIO_CODEC_CFG_FREQ_16KHZ;

	valid = srv_store_preset_validated(&lc3_preset_48_4_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);
	zassert_equal(valid, true, "48kHz preset should be valid when pref is 16kHz");
	memcpy(&server->snk.lc3_preset[0], &lc3_preset_48_4_1, sizeof(server->snk.lc3_preset[0]));

	valid = srv_store_preset_validated(&lc3_preset_24_2_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);

	zassert_equal(valid, false, "24kHz preset should be invalid when pref is 16kHz");

	valid = srv_store_preset_validated(&lc3_preset_16_2_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);
	zassert_equal(valid, true, "16kHz preset should be valid when pref is 16kHz");
	memcpy(&server->snk.lc3_preset[0], &lc3_preset_16_2_1, sizeof(server->snk.lc3_preset[0]));

	valid = srv_store_preset_validated(&lc3_preset_24_2_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);

	zassert_equal(valid, false, "24kHz preset should be invalid when pref is 16kHz");

	/* Change preferred sample rate to 24kHz */
	preferred_sample_rate_value = BT_AUDIO_CODEC_CFG_FREQ_24KHZ;
	valid = srv_store_preset_validated(&lc3_preset_48_4_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);
	zassert_equal(valid, true, "48kHz preset should be valid when pref is 24kHz");
	memcpy(&server->snk.lc3_preset[0], &lc3_preset_48_4_1, sizeof(server->snk.lc3_preset[0]));

	valid = srv_store_preset_validated(&lc3_preset_24_2_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);
	zassert_equal(valid, true, "24kHz preset should be valid when pref is 24kHz");
	memcpy(&server->snk.lc3_preset[0], &lc3_preset_24_2_1, sizeof(server->snk.lc3_preset[0]));

	valid = srv_store_preset_validated(&lc3_preset_16_2_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);
	zassert_equal(valid, false, "16kHz preset should be invalid when pref is 24kHz");

	valid = srv_store_preset_validated(&lc3_preset_48_4_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);
	zassert_equal(valid, false, "48kHz preset should be invalid when pref is 24kHz");

	srv_store_unlock();
}

void before_fn(void *dummy)
{
	int ret;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_init();
	zassert_equal(ret, 0);
	srv_store_unlock();
}

void after_fn(void *dummy)
{
	int ret;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_remove_all();
	zassert_equal(ret, 0);

	srv_store_unlock();
}

/* Test that calling functions without lock triggers assertions */
ZTEST(suite_server_store, test_assert_no_lock)
{
	int ret;

	/* Enable assert catching to prevent actual crash */
	ztest_set_assert_valid(true);

	/* This should trigger the assertion in valid_entry_check() */
	(void)srv_store_num_get(true);

	/* Disable assert catching */
	ztest_set_assert_valid(false);

	/* Re-acquire lock for cleanup */
	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	srv_store_unlock();
}

ZTEST(suite_server_store, test_conn_ptr_update)
{
	int ret;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	const bt_addr_le_t addr = {.type = BT_ADDR_LE_PUBLIC,
				   .a = {.val = {0x01, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}}};

	ret = srv_store_add_by_addr(&addr);
	zassert_equal(ret, 0);

	TEST_CONN(1);

	ret = srv_store_conn_update(&test_1_conn, &addr);
	zassert_equal(ret, 0);

	struct server_store *server;

	TEST_CONN(2);

	/* Test conn 2 has a different address and shall fail*/
	ret = srv_store_from_conn_get(&test_2_conn, &server);
	zassert_equal(ret, -ENOENT);

	ret = srv_store_from_addr_get(&addr, &server);
	zassert_equal(ret, 0);

	zassert_true(bt_addr_le_eq(&server->addr, &addr), NULL);

	/* Try to update again*/
	ret = srv_store_conn_update(&test_1_conn, &addr);
	zassert_equal(ret, -EACCES);

	srv_store_unlock();
}

ZTEST(suite_server_store, test_store_location_set)
{
	int ret;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	TEST_CONN(1);
	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	ret = srv_store_location_set(&test_1_conn, BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
	zassert_equal(ret, 0);

	ret = srv_store_location_set(&test_1_conn, BT_AUDIO_DIR_SOURCE,
				     BT_AUDIO_LOCATION_FRONT_RIGHT);

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(&test_1_conn, &server);
	zassert_equal(ret, 0);
	zassert_not_null(server);
	zassert_equal(server->snk.locations, BT_AUDIO_LOCATION_FRONT_LEFT);
	zassert_equal(server->src.locations, BT_AUDIO_LOCATION_FRONT_RIGHT);

	srv_store_unlock();
}

ZTEST(suite_server_store, test_clear)
{
	int ret;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	TEST_CONN(1);
	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	struct server_store *server_in;
	struct server_store *server_out;

	ret = srv_store_from_conn_get(&test_1_conn, &server_in);
	zassert_equal(ret, 0);

	server_in->name = "Test Server 1";
	server_in->member = (struct bt_csip_set_coordinator_set_member *)0xDEADBEEF;
	memset(&server_in->snk, 0xAA, sizeof(server_in->snk));
	memset(&server_in->src, 0xBB, sizeof(server_in->src));

	ret = srv_store_clear_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	ret = srv_store_from_conn_get(&test_1_conn, &server_out);
	zassert_equal(ret, 0);

	const bt_addr_le_t *addr = bt_conn_get_dst(&test_1_conn);

	ret = srv_store_from_addr_get(addr, &server_out);
	zassert_equal(ret, 0);

	ret = srv_store_remove_by_addr(addr);
	zassert_equal(ret, 0);

	ret = srv_store_from_addr_get(addr, &server_out);
	zassert_equal(ret, -ENOENT, "Should not be found after removal");

	srv_store_unlock();
}

ZTEST(suite_server_store, test_cap_check)
{
	/* This test is to be expanded after OCT-3480 is implemented, for
	 * more comprehensive client <-> server(s) capability check.
	 */
	int ret;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	TEST_CONN(1);
	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	static struct bt_audio_codec_cap codec_cap_1 = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_48KHZ,
		(BT_AUDIO_CODEC_CAP_DURATION_10 | BT_AUDIO_CODEC_CAP_DURATION_PREFER_10),
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 20, 180, 1u, BT_AUDIO_CONTEXT_TYPE_ANY);

	static struct bt_audio_codec_cap codec_cap_2 = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_24KHZ,
		(BT_AUDIO_CODEC_CAP_DURATION_10 | BT_AUDIO_CODEC_CAP_DURATION_PREFER_10),
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 20, 180, 1u, BT_AUDIO_CONTEXT_TYPE_ANY);

	ret = srv_store_codec_cap_set(&test_1_conn, BT_AUDIO_DIR_SINK, &codec_cap_1);
	zassert_equal(ret, 0, "Setting codec capabilities did not return zero %d", ret);

	ret = srv_store_codec_cap_set(&test_1_conn, BT_AUDIO_DIR_SINK, &codec_cap_2);
	zassert_equal(ret, 0, "Setting codec capabilities did not return zero %d", ret);

	uint32_t valid_codec_caps;

	ret = srv_store_valid_codec_cap_check(&test_1_conn, BT_AUDIO_DIR_SINK, &valid_codec_caps,
					      NULL, 0);
	zassert_equal(ret, 0);
	TC_PRINT("codec caps: 0x%x", valid_codec_caps);
	zassert_equal(valid_codec_caps, (1 << 0) | (1 << 1), "Two first caps are valid");

	srv_store_unlock();
}

ZTEST(suite_server_store, test_ep_count)
{
	int ret;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	TEST_CONN(0);
	TEST_CONN(1);

	ret = srv_store_add_by_conn(&test_0_conn);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	struct bt_bap_iso iso_chan_0A;
	struct bt_bap_ep ep_0A;

	ep_0A.state = BT_BAP_EP_STATE_IDLE;
	ep_0A.iso = &iso_chan_0A;

	struct bt_bap_iso iso_chan_0B;
	struct bt_bap_ep ep_0B;

	ep_0B.state = BT_BAP_EP_STATE_IDLE;
	ep_0B.iso = &iso_chan_0B;

	struct bt_bap_iso iso_chan_1A;
	struct bt_bap_ep ep_1A;

	ep_1A.state = BT_BAP_EP_STATE_IDLE;
	ep_1A.iso = &iso_chan_1A;

	struct bt_bap_iso iso_chan_1B;
	struct bt_bap_ep ep_1B;

	ep_1B.state = BT_BAP_EP_STATE_IDLE;
	ep_1B.iso = &iso_chan_1B;

	struct server_store *server_0 = NULL;
	struct server_store *server_1 = NULL;

	ret = srv_store_from_conn_get(&test_0_conn, &server_0);
	zassert_equal(ret, 0);

	ret = srv_store_from_conn_get(&test_1_conn, &server_1);
	zassert_equal(ret, 0);

	server_0->snk.cap_streams[0].bap_stream.ep = &ep_0A;
	server_0->snk.cap_streams[1].bap_stream.ep = &ep_0B;
	server_1->snk.cap_streams[0].bap_stream.ep = &ep_1A;
	server_1->snk.cap_streams[1].bap_stream.ep = &ep_1B;

	ret = srv_store_all_ep_state_count(BT_BAP_EP_STATE_IDLE, BT_AUDIO_DIR_SINK);
	zassert_equal(ret, 4);

	ep_0B.state = BT_BAP_EP_STATE_QOS_CONFIGURED;

	ret = srv_store_all_ep_state_count(BT_BAP_EP_STATE_IDLE, BT_AUDIO_DIR_SINK);
	zassert_equal(ret, 3);

	ret = srv_store_all_ep_state_count(BT_BAP_EP_STATE_QOS_CONFIGURED, BT_AUDIO_DIR_SINK);
	zassert_equal(ret, 1);

	srv_store_unlock();
}

ZTEST_SUITE(suite_server_store, NULL, NULL, before_fn, after_fn, NULL);
