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
#include "unicast_client_internal.h"
#include "server_store.h"
#include "bt_fakes/bt_fakes.h"

#define TEST_UNICAST_GROUP(name)                                                                   \
	struct bt_bap_unicast_group bap_group_##name = {0};                                        \
	struct bt_cap_unicast_group name;                                                          \
	sys_slist_init(&bap_group_##name.streams);                                                 \
	name.bap_unicast_group = &bap_group_##name;

static int test_cap_stream_populate(struct server_store *server, uint8_t idx, enum bt_audio_dir dir,
				    uint32_t pd, struct bt_cap_unicast_group *group,
				    struct bt_bap_ep *ep, uint16_t latency)
{
	if (server == NULL || group == NULL) {
		TC_PRINT("Invalid parameter(s) passed\n");
		return -EINVAL;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		if (idx >= CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT) {
			TC_PRINT("Index out of bounds for sink streams\n");
			return -EINVAL;
		}
		if (server->snk.cap_streams[idx].bap_stream.ep != NULL) {
			TC_PRINT("Stream already populated at index %d\n", idx);
			return -EALREADY;
		}
		server->snk.cap_streams[idx].bap_stream.group = group;
		server->snk.cap_streams[idx].bap_stream.ep = ep;
		server->snk.cap_streams[idx].bap_stream.qos = &server->snk.lc3_preset[idx].qos;
		server->snk.lc3_preset[idx].qos.pd = pd;
		server->snk.lc3_preset[idx].qos.latency = latency;

	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		if (idx >= CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT) {
			TC_PRINT("Index out of bounds for source streams\n");
			return -EINVAL;
		}
		if (server->src.cap_streams[idx].bap_stream.ep != NULL) {
			TC_PRINT("Stream already populated at index %d\n", idx);
			return -EALREADY;
		}
		server->src.cap_streams[idx].bap_stream.group = group;
		server->src.cap_streams[idx].bap_stream.ep = ep;
		server->src.cap_streams[idx].bap_stream.qos = &server->src.lc3_preset[idx].qos;
		server->src.lc3_preset[idx].qos.pd = pd;
		server->src.lc3_preset[idx].qos.latency = latency;
	} else {
		TC_PRINT("Invalid dir parameter passed\n");
		return -EINVAL;
	}

	return 0;
}

static void mock_add_stream_to_group(struct bt_bap_stream *bap_stream,
				     struct bt_cap_unicast_group *group)
{
	if ((bap_stream == NULL) || (group == NULL) || (group->bap_unicast_group == NULL)) {
		ztest_test_fail();
	}

	TC_PRINT("sys list append stream %p to group\n", (void *)bap_stream);
	sys_slist_append(&group->bap_unicast_group->streams, &bap_stream->_node);
}

ZTEST(suite_unicast_client_internal, test_unicast_client_internal_pres_dly_get_none)
{
	int ret;

	TEST_UNICAST_GROUP(cap_group);
	uint32_t pres_dly_snk_us;
	uint32_t pres_dly_src_us;
	struct pd_struct common_pd_snk;
	struct pd_struct common_pd_src;

	ret = unicast_client_internal_pres_dly_get(&cap_group, &pres_dly_snk_us, &pres_dly_src_us,
						   &common_pd_snk, &common_pd_src);
	zassert_equal(ret, 0);
	zassert_equal(pres_dly_snk_us, UINT32_MAX, "no streams");
	zassert_equal(pres_dly_src_us, UINT32_MAX, "no streams");
	zassert_equal(common_pd_snk.pd_min, 0);
	zassert_equal(common_pd_snk.pref_pd_min, 0);
	zassert_equal(common_pd_snk.pd_max, UINT32_MAX);
	zassert_equal(common_pd_snk.pref_pd_max, UINT32_MAX);
	zassert_equal(common_pd_src.pd_min, 0);
	zassert_equal(common_pd_src.pref_pd_min, 0);
	zassert_equal(common_pd_src.pd_max, UINT32_MAX);
	zassert_equal(common_pd_src.pref_pd_max, UINT32_MAX);
}

ZTEST(suite_unicast_client_internal, test_unicast_client_internal_pres_dly_get_one)
{
	int ret;

	TEST_UNICAST_GROUP(cap_group);
	uint32_t pres_dly_snk_us;
	uint32_t pres_dly_src_us;
	struct pd_struct common_pd_snk;
	struct pd_struct common_pd_src;

	/* Need to create endpoints in test as these are owned by the host */
	struct bt_bap_ep ep_1 = {0};

	ep_1.state = BT_BAP_EP_STATE_STREAMING;
	ep_1.dir = BT_AUDIO_DIR_SINK;

	ep_1.qos_pref.pd_min = 1000;
	ep_1.qos_pref.pref_pd_min = 2000;
	ep_1.qos_pref.pref_pd_max = 3000;
	ep_1.qos_pref.pd_max = 4000;

	struct server_store server_0 = {0};
	struct server_store server_1 = {0};

	ret = test_cap_stream_populate(&server_0, 0, BT_AUDIO_DIR_SINK, 40000, &cap_group, &ep_1,
				       0);
	zassert_equal(ret, 0);

	mock_add_stream_to_group(&server_0.snk.cap_streams[0].bap_stream, &cap_group);

	ret = unicast_client_internal_pres_dly_get(&cap_group, &pres_dly_snk_us, &pres_dly_src_us,
						   &common_pd_snk, &common_pd_src);
	zassert_equal(ret, 0);

	zassert_equal(pres_dly_snk_us, 2000, "Pref_min_selected");
	zassert_equal(pres_dly_src_us, UINT32_MAX, "no streams");
	zassert_equal(common_pd_snk.pd_min, 1000);
	zassert_equal(common_pd_snk.pref_pd_min, 2000);
	zassert_equal(common_pd_snk.pref_pd_max, 3000);
	zassert_equal(common_pd_snk.pd_max, 4000);

	ep_1.qos_pref.pref_pd_min = 0; /* Should select pd_min now, no pref */
	ret = unicast_client_internal_pres_dly_get(&cap_group, &pres_dly_snk_us, &pres_dly_src_us,
						   &common_pd_snk, &common_pd_src);
	zassert_equal(ret, 0);

	zassert_equal(pres_dly_snk_us, 1000, "Pref_min_selected");
	zassert_equal(pres_dly_src_us, UINT32_MAX, "no streams");

	struct bt_bap_ep ep_2 = {0};

	ep_2.state = BT_BAP_EP_STATE_STREAMING;
	ep_2.dir = BT_AUDIO_DIR_SINK;

	ep_2.qos_pref.pd_min = 4000;
	ep_2.qos_pref.pref_pd_min = 5000;
	ep_2.qos_pref.pref_pd_max = 6000;
	ep_2.qos_pref.pd_max = 7000;

	ret = test_cap_stream_populate(&server_1, 0, BT_AUDIO_DIR_SINK, 40000, &cap_group, &ep_2,
				       0);
	zassert_equal(ret, 0);

	mock_add_stream_to_group(&server_1.snk.cap_streams[0].bap_stream, &cap_group);

	ret = unicast_client_internal_pres_dly_get(&cap_group, &pres_dly_snk_us, &pres_dly_src_us,
						   &common_pd_snk, &common_pd_src);
	zassert_equal(ret, 0);

	/* Only common ground is 4000*/
	zassert_equal(pres_dly_snk_us, 4000, "Only common denominator is 4000");
	zassert_equal(pres_dly_src_us, UINT32_MAX, "no streams");
	zassert_equal(common_pd_snk.pd_min, 4000);
	zassert_equal(common_pd_snk.pref_pd_min, 5000);
	zassert_equal(common_pd_snk.pref_pd_max, 3000);
	zassert_equal(common_pd_snk.pd_max, 4000);

	/* No common denominator */
	ep_2.qos_pref.pd_min = 4001;
	ret = unicast_client_internal_pres_dly_get(&cap_group, &pres_dly_snk_us, &pres_dly_src_us,
						   &common_pd_snk, &common_pd_src);
	zassert_equal(ret, -ESPIPE, "Should return error when no common denominator");

	zassert_equal(pres_dly_snk_us, UINT32_MAX, "no common denominator");
	zassert_equal(pres_dly_src_us, UINT32_MAX, "no common denominator");
}

ZTEST(suite_unicast_client_internal, test_unicast_client_internal_pres_dly_set)
{
	int ret;

	TEST_UNICAST_GROUP(cap_group);

	struct bt_bap_ep ep_1 = {0};

	ep_1.state = BT_BAP_EP_STATE_STREAMING;
	ep_1.dir = BT_AUDIO_DIR_SINK;

	struct bt_bap_ep ep_2 = {0};

	ep_2.state = BT_BAP_EP_STATE_STREAMING;
	ep_2.dir = BT_AUDIO_DIR_SINK;

	struct bt_bap_ep ep_3 = {0};

	ep_3.state = BT_BAP_EP_STATE_STREAMING;
	ep_3.dir = BT_AUDIO_DIR_SOURCE;

	struct server_store server_0 = {0};

	ret = test_cap_stream_populate(&server_0, 0, BT_AUDIO_DIR_SINK, 5000, &cap_group, &ep_1, 0);
	zassert_equal(ret, 0);

	ret = test_cap_stream_populate(&server_0, 1, BT_AUDIO_DIR_SINK, 5000, &cap_group, &ep_2, 0);
	zassert_equal(ret, 0);

	ret = test_cap_stream_populate(&server_0, 0, BT_AUDIO_DIR_SOURCE, 40000, &cap_group, &ep_3,
				       0);
	zassert_equal(ret, 0);

	mock_add_stream_to_group(&server_0.snk.cap_streams[0].bap_stream, &cap_group);
	mock_add_stream_to_group(&server_0.snk.cap_streams[1].bap_stream, &cap_group);
	mock_add_stream_to_group(&server_0.src.cap_streams[0].bap_stream, &cap_group);

	enum action_req action;

	action = ACTION_REQ_NONE;

	ret = unicast_client_internal_pres_dly_set(&cap_group, 5000, UINT32_MAX, &action);
	zassert_equal(ret, 0);

	zassert_equal(action, ACTION_REQ_NONE);

	zassert_equal(server_0.snk.cap_streams[0].bap_stream.qos->pd, 5000);
	zassert_equal(server_0.snk.cap_streams[1].bap_stream.qos->pd, 5000);
	zassert_equal(server_0.src.cap_streams[0].bap_stream.qos->pd, 40000);

	/* Put one stream in a QoS configured state*/
	ep_1.state = BT_BAP_EP_STATE_QOS_CONFIGURED;

	/* No change in PD*/
	ret = unicast_client_internal_pres_dly_set(&cap_group, 5000, UINT32_MAX, &action);
	zassert_equal(ret, 0);

	zassert_equal(action, ACTION_REQ_NONE);

	/* Change PD with one stream in QoS configured state */
	ret = unicast_client_internal_pres_dly_set(&cap_group, 4000, UINT32_MAX, &action);
	zassert_equal(ret, 0);

	zassert_equal(action, ACTION_REQ_STREAM_QOS_RECONFIG);
	zassert_equal(server_0.snk.cap_streams[0].bap_stream.qos->pd, 4000);
	zassert_equal(server_0.snk.cap_streams[1].bap_stream.qos->pd, 4000);
	zassert_equal(server_0.src.cap_streams[0].bap_stream.qos->pd, 40000);

	/* Put one stream in a streaming state*/
	ep_1.state = BT_BAP_EP_STATE_STREAMING;
	/* Change PD with one stream in QoS configured state */
	ret = unicast_client_internal_pres_dly_set(&cap_group, 3000, UINT32_MAX, &action);
	zassert_equal(ret, 0);

	zassert_equal(action, ACTION_REQ_STREAM_QOS_RECONFIG);
	zassert_equal(server_0.snk.cap_streams[0].bap_stream.qos->pd, 3000);
	zassert_equal(server_0.snk.cap_streams[1].bap_stream.qos->pd, 3000);
	zassert_equal(server_0.src.cap_streams[0].bap_stream.qos->pd, 40000);

	ret = unicast_client_internal_pres_dly_set(&cap_group, UINT32_MAX, 20000, &action);
	zassert_equal(ret, 0);

	zassert_equal(action, ACTION_REQ_STREAM_QOS_RECONFIG);
	zassert_equal(server_0.snk.cap_streams[0].bap_stream.qos->pd, 3000);
	zassert_equal(server_0.snk.cap_streams[1].bap_stream.qos->pd, 3000);
	zassert_equal(server_0.src.cap_streams[0].bap_stream.qos->pd, 20000);
}

ZTEST(suite_unicast_client_internal, test_srv_store_max_transp_lat_get)
{
	int ret;

	TEST_UNICAST_GROUP(cap_group);

	TC_PRINT("the cap group is %p\n", &cap_group);

	struct server_store server_0 = {0};

	/* Need to create endpoints in test as these are owned by the host */
	struct bt_bap_ep ep_1 = {0};

	ep_1.state = BT_BAP_EP_STATE_STREAMING;
	ep_1.dir = BT_AUDIO_DIR_SINK;
	ep_1.qos_pref.latency = 40;

	struct bt_bap_ep ep_2 = {0};

	ep_2.state = BT_BAP_EP_STATE_STREAMING;
	ep_2.dir = BT_AUDIO_DIR_SINK;
	ep_2.qos_pref.latency = 45;

	struct bt_bap_ep ep_3 = {0};

	ep_3.state = BT_BAP_EP_STATE_STREAMING;
	ep_3.dir = BT_AUDIO_DIR_SOURCE;
	ep_3.qos_pref.latency = 50;

	ret = test_cap_stream_populate(&server_0, 0, BT_AUDIO_DIR_SINK, 40000, &cap_group, &ep_1,
				       40);
	zassert_equal(ret, 0);

	ret = test_cap_stream_populate(&server_0, 1, BT_AUDIO_DIR_SINK, 40000, &cap_group, &ep_2,
				       45);
	zassert_equal(ret, 0);

	ret = test_cap_stream_populate(&server_0, 0, BT_AUDIO_DIR_SOURCE, 40000, &cap_group, &ep_3,
				       50);
	zassert_equal(ret, 0);

	mock_add_stream_to_group(&server_0.snk.cap_streams[0].bap_stream, &cap_group);
	mock_add_stream_to_group(&server_0.snk.cap_streams[1].bap_stream, &cap_group);
	mock_add_stream_to_group(&server_0.src.cap_streams[0].bap_stream, &cap_group);

	uint16_t max_transp_lat_snk_ms;
	uint16_t max_transp_lat_src_ms;

	ret = unicast_client_internal_max_transp_latency_get(&cap_group, &max_transp_lat_snk_ms,
							     &max_transp_lat_src_ms);
	zassert_equal(ret, 0);
	TC_PRINT("Max transport latency for sink: %d ms, source: %d ms", max_transp_lat_snk_ms,
		 max_transp_lat_src_ms);
	TC_PRINT("max_transp_lat_snk_ms: %d, max_transp_lat_src_ms: %d", max_transp_lat_snk_ms,
		 max_transp_lat_src_ms);
	zassert_equal(max_transp_lat_snk_ms, 40, "Max transport latency for sink should be 40ms");
	zassert_equal(max_transp_lat_src_ms, 50, "Max transport latency for source should be 50ms");
}

ZTEST(suite_unicast_client_internal, test_srv_store_max_transp_lat_set)
{
	int ret;

	TEST_UNICAST_GROUP(cap_group);

	struct server_store server_0 = {0};

	TC_PRINT("the cap group is %p\n", &cap_group);

	/* Need to create endpoints in test as these are owned by the host */
	struct bt_bap_ep ep_1 = {0};

	ep_1.state = BT_BAP_EP_STATE_STREAMING;
	ep_1.dir = BT_AUDIO_DIR_SINK;
	ep_1.qos_pref.latency = 10;

	struct bt_bap_ep ep_2 = {0};

	ep_2.state = BT_BAP_EP_STATE_STREAMING;
	ep_2.dir = BT_AUDIO_DIR_SINK;
	ep_2.qos_pref.latency = 20;

	struct bt_bap_ep ep_3 = {0};

	ep_3.state = BT_BAP_EP_STATE_STREAMING;
	ep_3.dir = BT_AUDIO_DIR_SOURCE;
	ep_3.qos_pref.latency = 50;

	ret = test_cap_stream_populate(&server_0, 0, BT_AUDIO_DIR_SINK, 40000, &cap_group, &ep_1,
				       10);
	zassert_equal(ret, 0);

	ret = test_cap_stream_populate(&server_0, 1, BT_AUDIO_DIR_SINK, 40000, &cap_group, &ep_2,
				       20);
	zassert_equal(ret, 0);

	ret = test_cap_stream_populate(&server_0, 0, BT_AUDIO_DIR_SOURCE, 40000, &cap_group, &ep_3,
				       50);
	zassert_equal(ret, 0);

	mock_add_stream_to_group(&server_0.snk.cap_streams[0].bap_stream, &cap_group);
	mock_add_stream_to_group(&server_0.snk.cap_streams[1].bap_stream, &cap_group);
	mock_add_stream_to_group(&server_0.src.cap_streams[0].bap_stream, &cap_group);

	enum action_req action;

	action = ACTION_REQ_NONE;

	/* No change */
	ret = unicast_client_internal_max_transp_latency_set(&cap_group, UINT16_MAX, UINT16_MAX,
							     &action);
	zassert_equal(ret, 0);
	zassert_equal(action, ACTION_REQ_NONE);
	zassert_equal(server_0.snk.cap_streams[0].bap_stream.qos->latency, 10);
	zassert_equal(server_0.snk.cap_streams[1].bap_stream.qos->latency, 20);
	zassert_equal(server_0.src.cap_streams[0].bap_stream.qos->latency, 50);

	/* Change sinks */
	ret = unicast_client_internal_max_transp_latency_set(&cap_group, 5, UINT16_MAX, &action);
	zassert_equal(ret, 0);
	zassert_equal(action, ACTION_REQ_GROUP_RESTART);
	zassert_equal(server_0.snk.cap_streams[0].bap_stream.qos->latency, 5);
	zassert_equal(server_0.snk.cap_streams[1].bap_stream.qos->latency, 5);
	zassert_equal(server_0.src.cap_streams[0].bap_stream.qos->latency, 50);

	/* Change source */
	ret = unicast_client_internal_max_transp_latency_set(&cap_group, UINT16_MAX, 33, &action);
	zassert_equal(ret, 0);
	zassert_equal(action, ACTION_REQ_GROUP_RESTART);
	zassert_equal(server_0.snk.cap_streams[0].bap_stream.qos->latency, 5);
	zassert_equal(server_0.snk.cap_streams[1].bap_stream.qos->latency, 5);
	zassert_equal(server_0.src.cap_streams[0].bap_stream.qos->latency, 33);
}

void before_fn(void *dummy)
{

	RESET_FAKE(bt_cap_unicast_group_foreach_stream);
	FFF_RESET_HISTORY();
	bt_cap_unicast_group_foreach_stream_fake.custom_fake =
		bt_cap_unicast_group_foreach_stream_custom_fake;

	bt_conn_get_dst_fake.custom_fake = bt_conn_get_dst_custom_fake;
	bt_bap_ep_get_info_fake.custom_fake = bt_bap_ep_get_info_custom_fake;
}

ZTEST_SUITE(suite_unicast_client_internal, NULL, NULL, before_fn, NULL, NULL);
