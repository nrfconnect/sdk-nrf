/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/tc_util.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/lc3.h>

#include <../subsys/bluetooth/audio/bap_endpoint.h>
#include <../subsys/bluetooth/audio/bap_iso.h>
#include <../subsys/bluetooth/host/conn_internal.h>
#include <../subsys/bluetooth/audio/cap_internal.h>
#include "server_store.h"
#include "bt_fakes/bt_fakes.h"

/* TODO: Should populate the pointer on a real cap stream */
#define TEST_CAP_STREAM(name, dir_in, pd_in, group_in)                                             \
	struct bt_cap_stream name = {0};                                                           \
	struct bt_bap_ep name##_ep_var = {0};                                                      \
	struct bt_iso_chan name##_bap_iso = {0};                                                   \
	struct bt_bap_qos_cfg name##_qos = {0};                                                    \
	name##_qos.pd = pd_in;                                                                     \
	name##_ep_var.dir = dir_in;                                                                \
	name.bap_stream.ep = &name##_ep_var;                                                       \
	name.bap_stream.iso = &name##_bap_iso;                                                     \
	name.bap_stream.group = (void *)group_in;                                                  \
	name.bap_stream.qos = &name##_qos;

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

	ret = srv_store_num_get();
	zassert_equal(ret, 0);

	TEST_CONN(1);

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	ret = srv_store_num_get();
	zassert_equal(ret, 1, "Number of servers should be one after adding a server");

	bt_addr_le_t addr = {.type = BT_ADDR_LE_PUBLIC,
			     .a = {.val = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}}};

	bool exists = srv_store_server_exists(&addr);

	zassert_false(exists, "Server should not exist for non-added address");

	ret = srv_store_add_by_addr(&addr);
	zassert_equal(ret, 0);

	exists = srv_store_server_exists(&addr);
	zassert_true(exists, "Server should not exist for non-added address");

	ret = srv_store_num_get();
	zassert_equal(ret, 2, "Number of servers should be two after adding a server");

	ret = srv_store_remove_all(false);
	zassert_equal(ret, -EACCES, "Should not be able to remove with active conn");

	/* Force removal of servers */
	ret = srv_store_remove_all(true);
	zassert_equal(ret, 0);

	ret = srv_store_num_get();
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

	ret = srv_store_num_get();
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

	struct server_store *retr_server1A = NULL;
	struct server_store *retr_server1B = NULL;
	struct server_store *retr_server2A = NULL;
	struct server_store *retr_server2B = NULL;
	struct server_store *retr_server3A = NULL;
	struct server_store *retr_server3B = NULL;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_2_conn);
	zassert_equal(ret, 0);
	ret = srv_store_from_conn_get(&test_2_conn, &retr_server2A);
	zassert_equal(ret, 0);
	retr_server2A->snk.num_codec_caps = 2;

	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);
	ret = srv_store_from_conn_get(&test_2_conn, &retr_server2B);
	zassert_equal(ret, 0);
	zassert_equal(retr_server2A, retr_server2B, "Pointers to do not match");
	zassert_equal(retr_server2B->snk.num_codec_caps, 2,
		      "Server 2 codec cap count does not match expected");

	ret = srv_store_from_conn_get(&test_1_conn, &retr_server1A);
	zassert_equal(ret, 0);

	ret = srv_store_add_by_conn(&test_3_conn);
	zassert_equal(ret, 0);
	ret = srv_store_from_conn_get(&test_3_conn, &retr_server3A);
	zassert_equal(ret, 0);

	ret = srv_store_from_conn_get(&test_1_conn, &retr_server1B);
	zassert_equal(ret, 0);
	zassert_equal(retr_server1A, retr_server1B, "Pointers to do not match");

	ret = srv_store_from_conn_get(&test_3_conn, &retr_server3B);
	zassert_equal(ret, 0);
	zassert_equal(retr_server3A, retr_server3B, "Pointers to do not match");

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

	ret = srv_store_num_get();
	zassert_equal(ret, 3, "Number of servers should be three after adding three servers");

	ret = srv_store_remove_by_conn(&test_2_conn);
	zassert_equal(ret, 0);

	ret = srv_store_num_get();
	zassert_equal(ret, 2, "Number of servers should be two after removing one");

	ret = srv_store_remove_by_conn(&test_0_conn);
	zassert_equal(ret, 0);

	ret = srv_store_num_get();
	zassert_equal(ret, 1, "Number of servers should be one after removing one");

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
	TEST_CAP_STREAM(TCS_1_existing, BT_AUDIO_DIR_SINK, 40000, 0x1111);
	memcpy(&retr_server->snk.cap_streams[0], &TCS_1_existing, sizeof(TCS_1_existing));
	TEST_CAP_STREAM(TCS_2_existing, BT_AUDIO_DIR_SINK, 40000, 0x1111);
	memcpy(&retr_server->snk.cap_streams[1], &TCS_2_existing, sizeof(TCS_2_existing));
	TEST_CAP_STREAM(TCS_3_existing, BT_AUDIO_DIR_SINK, 40000, 0x1111);
	memcpy(&retr_server->snk.cap_streams[2], &TCS_3_existing, sizeof(TCS_3_existing));

	ret = srv_store_from_conn_get(&test_2_conn, &retr_server);
	zassert_equal(ret, 0);

	retr_server->name = "Test Server 2";
	TEST_CAP_STREAM(TCS_4_existing, BT_AUDIO_DIR_SINK, 40000, 0x1111);
	memcpy(&retr_server->snk.cap_streams[0], &TCS_4_existing, sizeof(TCS_4_existing));
	TEST_CAP_STREAM(TCS_5_existing, BT_AUDIO_DIR_SINK, 40000, 0x1111);
	memcpy(&retr_server->snk.cap_streams[1], &TCS_5_existing, sizeof(TCS_5_existing));
	TEST_CAP_STREAM(TCS_6_existing, BT_AUDIO_DIR_SINK, 40000, 0x1111);
	memcpy(&retr_server->snk.cap_streams[2], &TCS_6_existing, sizeof(TCS_6_existing));

	struct bt_bap_stream *stream_pointer = &retr_server->snk.cap_streams[1].bap_stream;

	ret = srv_store_from_stream_get((struct bt_bap_stream *)0xDEADBEEF, &retr_server);
	zassert_equal(ret, -ENOENT, "Retrieving from non existing stream should return -ENOENT");
	zassert_is_null(retr_server, "Retrieved server should be NULL");

	struct server_store *found_server = NULL;

	ret = srv_store_from_stream_get(stream_pointer, &found_server);
	zassert_equal(ret, 0, "Retrieving from stream should return zero %d", ret);
	zassert_not_null(found_server, "Retrieved server should not be NULL");
	zassert_equal(found_server->name, "Test Server 2");
	zassert_equal_ptr(found_server->conn, &test_2_conn,
			  "Retrieved server connection does not match expected");

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

	ret = srv_store_from_conn_get(&test_1_conn, &server);
	zassert_equal(ret, 0);

	valid = srv_store_preset_validated(&lc3_preset_16_2_1.codec_cfg,
					   &server->snk.lc3_preset[0].codec_cfg,
					   preferred_sample_rate_value);
	zassert_equal(valid, true, "the existing LC3 preset is empty. Will accept the new one");
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

/* Test that calling functions without lock triggers assertions */
ZTEST(suite_server_store, test_assert_no_lock)
{
	int ret;

	/* Enable assert catching to prevent actual crash */
	ztest_set_assert_valid(true);

	/* This should trigger the assertion in valid_entry_check() */
	(void)srv_store_num_get();

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

	const bt_addr_le_t *addr = bt_conn_get_dst(&test_1_conn);

	ret = srv_store_remove_by_addr(addr);
	zassert_equal(ret, -EACCES, " Should not be able to remove with active conn");

	ret = srv_store_clear_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	ret = srv_store_from_conn_get(&test_1_conn, &server_out);
	zassert_equal(ret, -ENOTCONN);

	ret = srv_store_from_addr_get(addr, &server_out);
	zassert_equal(ret, 0);

	uint8_t zeros[sizeof(struct server_store)] = {0};

	/* Address is not cleared */
	zassert_str_equal(server_out->name, "NOT_SET");
	zassert_is_null(server_out->conn);
	zassert_is_null(server_out->member);
	zassert_false(server_out->snk.waiting_for_disc);
	zassert_false(server_out->src.waiting_for_disc);
	zassert_equal(server_out->snk.locations, 0);
	zassert_equal(server_out->src.locations, 0);
	zassert_mem_equal(&server_out->snk.lc3_preset, zeros, sizeof(server_out->snk.lc3_preset));
	zassert_mem_equal(&server_out->src.lc3_preset, zeros, sizeof(server_out->src.lc3_preset));
	zassert_mem_equal(&server_out->snk.codec_caps, zeros, sizeof(server_out->snk.codec_caps));
	zassert_mem_equal(&server_out->src.codec_caps, zeros, sizeof(server_out->src.codec_caps));
	zassert_equal(server_out->snk.num_codec_caps, 0);
	zassert_equal(server_out->src.num_codec_caps, 0);
	zassert_mem_equal((&server_out->snk.eps), zeros, sizeof(server_out->snk.eps));
	zassert_mem_equal((&server_out->src.eps), zeros, sizeof(server_out->src.eps));
	zassert_equal(server_out->snk.num_eps, 0);
	zassert_equal(server_out->src.num_eps, 0);
	zassert_equal(server_out->snk.supported_ctx, 0);
	zassert_equal(server_out->src.supported_ctx, 0);
	zassert_equal(server_out->snk.available_ctx, 0);
	zassert_equal(server_out->src.available_ctx, 0);

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
	uint32_t valid_codec_caps = 0;

	ret = srv_store_lock(K_NO_WAIT);
	zassert_equal(ret, 0);

	TEST_CONN(1);
	ret = srv_store_add_by_conn(&test_1_conn);
	zassert_equal(ret, 0);

	/* Unsupported codec capability */
	static struct bt_audio_codec_cap codec_cap_1 = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_384KHZ,
		(BT_AUDIO_CODEC_CAP_DURATION_10 | BT_AUDIO_CODEC_CAP_DURATION_PREFER_10),
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 20, 180, 1u, BT_AUDIO_CONTEXT_TYPE_ANY);

	static struct bt_audio_codec_cap codec_cap_2 = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_48KHZ,
		(BT_AUDIO_CODEC_CAP_DURATION_10 | BT_AUDIO_CODEC_CAP_DURATION_PREFER_10),
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 20, 180, 1u, BT_AUDIO_CONTEXT_TYPE_ANY);

	static struct bt_audio_codec_cap codec_cap_3 = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_24KHZ,
		(BT_AUDIO_CODEC_CAP_DURATION_10 | BT_AUDIO_CODEC_CAP_DURATION_PREFER_10),
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 20, 180, 1u, BT_AUDIO_CONTEXT_TYPE_ANY);

	ret = srv_store_codec_cap_set(&test_1_conn, BT_AUDIO_DIR_SINK, &codec_cap_1);
	zassert_equal(ret, 0, "Setting codec capabilities did not return zero %d", ret);

	ret = srv_store_valid_codec_cap_check(&test_1_conn, BT_AUDIO_DIR_SINK, &valid_codec_caps,
					      NULL, 0);
	zassert_equal(ret, 0);
	zassert_equal(valid_codec_caps, 0, "No valid codec caps should be found");

	ret = srv_store_codec_cap_set(&test_1_conn, BT_AUDIO_DIR_SINK, &codec_cap_2);
	zassert_equal(ret, 0, "Setting codec capabilities did not return zero %d", ret);

	ret = srv_store_codec_cap_set(&test_1_conn, BT_AUDIO_DIR_SINK, &codec_cap_3);
	zassert_equal(ret, 0, "Setting codec capabilities did not return zero %d", ret);

	valid_codec_caps = 0;
	ret = srv_store_valid_codec_cap_check(&test_1_conn, BT_AUDIO_DIR_SINK, &valid_codec_caps,
					      NULL, 0);
	zassert_equal(ret, 0);
	TC_PRINT("codec caps: 0x%x", valid_codec_caps);
	zassert_equal(valid_codec_caps, (1 << 1) | (1 << 2), "cap 1 and 2 are valid, not 0");

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

	struct bt_bap_ep ep_0A = {0};
	struct bt_bap_iso iso_chan_0A = {0};

	ep_0A.state = BT_BAP_EP_STATE_IDLE;
	ep_0A.iso = &iso_chan_0A;

	struct bt_bap_ep ep_0B = {0};
	struct bt_bap_iso iso_chan_0B = {0};

	ep_0B.state = BT_BAP_EP_STATE_IDLE;
	ep_0B.iso = &iso_chan_0B;

	struct bt_bap_ep ep_1A = {0};
	struct bt_bap_iso iso_chan_1A = {0};

	ep_1A.state = BT_BAP_EP_STATE_IDLE;
	ep_1A.iso = &iso_chan_1A;

	struct bt_bap_ep ep_1B = {0};
	struct bt_bap_iso iso_chan_1B = {0};

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

void before_fn(void *dummy)
{
	int ret;

	RESET_FAKE(bt_cap_unicast_group_foreach_stream);
	FFF_RESET_HISTORY();
	bt_cap_unicast_group_foreach_stream_fake.custom_fake =
		bt_cap_unicast_group_foreach_stream_custom_fake;

	bt_conn_get_dst_fake.custom_fake = bt_conn_get_dst_custom_fake;
	bt_bap_ep_get_info_fake.custom_fake = bt_bap_ep_get_info_custom_fake;

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

	ret = srv_store_remove_all(true);
	zassert_equal(ret, 0);

	srv_store_unlock();
}

ZTEST_SUITE(suite_server_store, NULL, NULL, before_fn, after_fn, NULL);
