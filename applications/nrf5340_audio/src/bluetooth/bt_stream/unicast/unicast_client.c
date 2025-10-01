/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "unicast_client.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <../subsys/bluetooth/audio/bap_iso.h>

/* TODO: Remove when a qos_pref_get function has been added in host */
/* https://github.com/zephyrproject-rtos/zephyr/issues/72359 */
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include "macros_common.h"
#include "zbus_common.h"
#include "bt_le_audio_tx.h"
#include "le_audio.h"
#include "server_store.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(unicast_client, CONFIG_UNICAST_CLIENT_LOG_LEVEL);

ZBUS_CHAN_DEFINE(le_audio_chan, struct le_audio_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

#define CAP_PROCED_SEM_WAIT_TIME_MS K_MSEC(500)
K_SEM_DEFINE(sem_cap_procedure_proceed, 1, 1);

enum cap_procedure_type {
	CAP_PROCEDURE_START = 1,
	CAP_PROCEDURE_UPDATE,
	CAP_PROCEDURE_STOP,
};

K_MSGQ_DEFINE(cap_proc_q, sizeof(enum cap_procedure_type), CONFIG_BT_ISO_MAX_CHAN, sizeof(void *));

/* For unicast (as opposed to broadcast) level 2/subgroup is not defined in the specification */
#define LVL2 0

struct discover_dir {
	struct bt_conn *conn;
	bool sink;
	bool source;
};

BUILD_ASSERT(CONFIG_BT_ISO_MAX_CIG == 1, "Only one CIG is supported");

static le_audio_receive_cb receive_cb;

static struct bt_cap_unicast_group *unicast_group;
static bool unicast_group_created;

static bool playing_state = true;

static void le_audio_event_publish(enum le_audio_evt_type event, struct bt_conn *conn,
				   struct bt_bap_stream *stream, enum bt_audio_dir dir)
{
	int ret;
	struct le_audio_msg msg;

	msg.event = event;
	msg.stream = stream;
	msg.conn = conn;
	msg.dir = dir;

	ret = zbus_chan_pub(&le_audio_chan, &msg, LE_AUDIO_ZBUS_EVENT_WAIT_TIME);
	ERR_CHK(ret);
}

static int stream_idx_get(struct bt_bap_stream *stream, struct stream_index *idx)
{
	int ret;
	struct bt_iso_info info;

	ret = bt_iso_chan_get_info(stream->iso, &info);
	if (ret < 0) {
		LOG_ERR("Failed to get ISO channel info: %d", ret);
		return ret;
	}

	idx->lvl1 = info.unicast.cig_id;
	idx->lvl2 = LVL2;
	idx->lvl3 = info.unicast.cis_id;

	return 0;
}

/**
 * @brief	Check if there is any CAP procedure waiting, execute it if so.
 */
static void cap_proc_waiting_check(void)
{
	int ret;
	enum cap_procedure_type proc;

	ret = k_msgq_get(&cap_proc_q, &proc, K_NO_WAIT);
	if (ret == -ENOMSG) {
		/* No procedure waiting */
		return;
	} else if (ret) {
		LOG_ERR("Failed to get message from cap_proc_q: %d", ret);
		return;
	}

	switch (proc) {
	case CAP_PROCEDURE_START:
		unicast_client_start(0);
		break;
	case CAP_PROCEDURE_UPDATE:
		LOG_WRN("Update procedure not implemented");
		break;
	case CAP_PROCEDURE_STOP:
		unicast_client_stop(0);
		break;
	default:
		LOG_ERR("Unknown procedure: %d", proc);
		break;
	}
}

/**
 * @brief	Create a unicast group with all connected servers and their valid EPs. Pair
 *		sink/source streams from the same server together to make sure they are part of the
 *		same CIS.
 */
static void unicast_group_create(void)
{
	int ret;
	struct bt_cap_unicast_group_stream_pair_param
		pair_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_unicast_group_stream_param
		group_sink_stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_unicast_group_stream_param
		group_source_stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_unicast_group_param group_param;

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	/* Find out how many servers we have connected
	 */
	uint8_t num_servers = srv_store_num_get(true);

	if (num_servers == 0) {
		LOG_ERR("No servers found, cannot create unicast group");
		srv_store_unlock();
		return;
	}

	/* Find out how many valid sink EPs we have
	 */
	uint8_t num_valid_sink_eps = 0;
	uint8_t num_valid_source_eps = 0;

	for (int i = 0; i < num_servers; i++) {
		struct server_store *tmp_server = NULL;

		ret = srv_store_server_get(&tmp_server, i);
		if (ret < 0) {
			LOG_ERR("Failed to get server store from index %d: %d", i, ret);
			srv_store_unlock();
			return;
		}

		num_valid_sink_eps += tmp_server->snk.num_eps;
		num_valid_source_eps += tmp_server->src.num_eps;
	}

	LOG_INF("We have %d servers, with a total of %d valid sink EPs and %d valid source EPs",
		num_servers, num_valid_sink_eps, num_valid_source_eps);

	if (num_valid_sink_eps == 0 && num_valid_source_eps == 0) {
		LOG_ERR("No valid sink or source EPs found, cannot create unicast group");
		srv_store_unlock();
		return;
	}

	uint8_t group_sink_iterator = 0;
	uint8_t group_source_iterator = 0;

	for (int i = 0; i < num_servers; i++) {
		struct server_store *tmp_server = NULL;

		ret = srv_store_server_get(&tmp_server, i);
		if (ret < 0) {
			LOG_ERR("Failed to get server store from index %d: %d", i, ret);
			srv_store_unlock();
			return;
		}
		if (tmp_server->snk.num_eps == 0 && tmp_server->src.num_eps == 0) {
			LOG_DBG("Server %d has no valid sink or source EPs, skipping", i);
			continue;
		}

		/* Add only the streams that has a valid preset set */
		for (int j = 0;
		     j < MIN(tmp_server->snk.num_eps, POPCOUNT(tmp_server->snk.locations)); j++) {
			if (tmp_server->snk.lc3_preset[j].qos.pd == 0) {
				LOG_DBG("Sink EP %d has no valid preset, skipping", j);
				continue;
			}

			group_sink_stream_params[group_sink_iterator].qos_cfg =
				&tmp_server->snk.lc3_preset[j].qos;
			group_sink_stream_params[group_sink_iterator].stream =
				&tmp_server->snk.cap_streams[j];
			group_sink_iterator++;
		}

		/* Add only the streams that has a valid preset set */
		for (int j = 0;
		     j < MIN(tmp_server->src.num_eps, POPCOUNT(tmp_server->src.locations)); j++) {
			if (tmp_server->src.lc3_preset[j].qos.pd == 0) {
				LOG_DBG("Source EP %d has no valid preset, skipping", j);
				continue;
			}

			group_source_stream_params[group_source_iterator].qos_cfg =
				&tmp_server->src.lc3_preset[j].qos;
			group_source_stream_params[group_source_iterator].stream =
				&tmp_server->src.cap_streams[j];
			group_source_iterator++;
		}
	}

	int stream_iterator = 0;

	/* Pair TX and RX from same server.
	 * We pair in the order of sink to source because the sink stream will always be created
	 * before the source stream
	 */
	for (int i = 0; i < group_sink_iterator; i++) {
		pair_params[i].tx_param = &group_sink_stream_params[i];
		/* Search streams for a matching source */
		bool source_found = false;
		struct server_store *sink_server = NULL;

		ret = srv_store_from_stream_get(&group_sink_stream_params[i].stream->bap_stream,
						&sink_server);
		if (ret < 0) {
			LOG_ERR("Failed to get server from sink stream %p: %d",
				(void *)&group_sink_stream_params[i].stream->bap_stream, ret);
			srv_store_unlock();
			return;
		}

		for (int j = stream_iterator; j < group_source_iterator; j++) {
			/* Check if the source stream belongs to the same device */
			struct server_store *source_server = NULL;

			ret = srv_store_from_stream_get(
				&group_source_stream_params[j].stream->bap_stream, &source_server);
			if (ret < 0) {
				LOG_ERR("Failed to get server from source stream %p: %d",
					(void *)&group_source_stream_params[j].stream->bap_stream,
					ret);
				srv_store_unlock();
				return;
			}

			if (sink_server == source_server) {
				pair_params[i].rx_param = &group_source_stream_params[j];
				source_found = true;
				stream_iterator++;
				break;
			}
		}

		if (!source_found) {
			LOG_DBG("Setting RX param for sink EP %d to NULL", i);
			pair_params[i].rx_param = NULL;
			stream_iterator++;
		}
	}

	/* Check if there are unpaired source streams, if so; add them */
	for (int i = 0; i < group_source_iterator; i++) {
		/* Check if the source has already been added */
		bool source_already_added = false;

		for (int j = 0; j < stream_iterator; j++) {
			if (pair_params[j].rx_param == &group_source_stream_params[i]) {
				source_already_added = true;
				break;
			}
		}

		if (source_already_added) {
			LOG_DBG("Source EP %d already added, skipping", i);
			continue;
		}

		LOG_DBG("Adding unpaired source EP %d", i);
		pair_params[stream_iterator].tx_param = NULL;
		pair_params[stream_iterator].rx_param = &group_source_stream_params[i];
		stream_iterator++;
	}

	group_param.params = pair_params;
	group_param.params_count = stream_iterator;

	if (IS_ENABLED(CONFIG_BT_AUDIO_PACKING_INTERLEAVED)) {
		group_param.packing = BT_ISO_PACKING_INTERLEAVED;
	} else {
		group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	}

	ret = bt_cap_unicast_group_create(&group_param, &unicast_group);
	if (ret) {
		LOG_ERR("Failed to create unicast group: %d", ret);
	} else {
		LOG_INF("Created unicast group");
		unicast_group_created = true;
	}

	srv_store_unlock();
}

/**
 * @brief	Function to check if a stream is already in the unicast group.
 *
 * @param stream	Stream to check.
 * @param user_data	User data, in this case a pointer to the server_store to check against.
 *
 * @return		True if the stream is not in the group, false if it is.
 */
static bool stream_in_group_check(struct bt_cap_stream *stream, void *user_data)
{
	struct server_store *tmp_server = (struct server_store *)user_data;

	for (int j = 0; j < ARRAY_SIZE(tmp_server->snk.cap_streams); j++) {
		if (stream == &tmp_server->snk.cap_streams[j]) {
			LOG_DBG("Sink stream %p already in unicast group, skipping",
				(void *)stream);
			return false;
		}
	}

	for (int j = 0; j < ARRAY_SIZE(tmp_server->src.cap_streams); j++) {
		if (stream == &tmp_server->src.cap_streams[j]) {
			LOG_DBG("Source stream %p already in unicast group, skipping",
				(void *)stream);
			return false;
		}
	}

	return true;
}

/**
 * @brief	Worker to start unicast streams. If a unicast group doesn't exist, it will be
 *		created. If it does exist, it will check if there is room for more streams, and if
 *		so, check if there are any connected servers that are not part of the group that
 *		can be added.
 *		If the group is full, or there are no more servers to add, it will start the streams
 *		in the unicast group.
 */
static void cap_start_worker(struct k_work *work)
{
	int ret;

	/* Create a unicast group if it doesn't already exist */
	if (unicast_group_created == false) {
		unicast_group_create();
		goto start_streams;
	}

	struct bt_cap_unicast_group_info info;
	uint8_t group_length = 0;

	ret = bt_cap_unicast_group_get_info(unicast_group, &info);
	if (ret) {
		LOG_ERR("Failed to get unicast group info: %d", ret);
		return;
	}

	group_length = sys_slist_len(&info.unicast_group->streams);
	if (group_length >= CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT) {
		/* The group is as full as it can get, start the relevant streams */
		goto start_streams;
	}

	/* Group is created, but there is still room for more devices, check if there are any
	 * waiting to join.
	 */

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	uint8_t num_servers = srv_store_num_get(true);

	/* Check if each of the connected servers in srv_store are in the unicast_group */
	for (int i = 0; i < num_servers; i++) {
		struct server_store *tmp_server = NULL;

		ret = srv_store_server_get(&tmp_server, i);
		if (ret < 0) {
			LOG_ERR("Failed to get server store from index %d: %d", i, ret);
			continue;
		}

		/* Check that the server is connected */
		struct bt_conn_info info;

		ret = bt_conn_get_info(tmp_server->conn, &info);
		if (ret) {
			LOG_ERR("Failed to get connection info for conn: %p",
				(void *)tmp_server->conn);
			continue;
		}

		if (info.state != BT_CONN_STATE_CONNECTED) {
			LOG_DBG("Connection %p is not connected, skipping",
				(void *)tmp_server->conn);
			continue;
		}

		/* Check if the server has at least one valid preset set */
		if (tmp_server->snk.lc3_preset[0].qos.pd == 0 &&
		    tmp_server->src.lc3_preset[0].qos.pd == 0) {
			LOG_DBG("Server %d has no valid preset, skipping", i);
			continue;
		}

		/* Check each of the streams in the unicast_group against all of the streams in the
		 * server
		 */
		ret = bt_cap_unicast_group_foreach_stream(unicast_group, stream_in_group_check,
							  tmp_server);
		if (ret == -ECANCELED) {
			LOG_INF("Server %d not found in unicast group, will stop the current "
				"streams and create a new group",
				i);
			srv_store_unlock();

			unicast_client_stop(0);
			unicast_group_created = false;

			/* A new group will be created after the released_cb has been called */
			return;
		}
	}

	srv_store_unlock();

start_streams:

	ret = unicast_client_start(0);
	if (ret < 0) {
		LOG_ERR("Failed to start unicast client: %d", ret);
		return;
	}
}

K_WORK_DEFINE(cap_start_work, cap_start_worker);

/**
 * @brief	Callback for when a location is discovered. This function will store the location
 *		in the server_store struct for the corresponding connection and direction.
 *
 * @note	If both front left and front right channel locations are set, the device is
 *		considered a stereo device, and the location will be set to front left and
 *		front right. If only one of the channels are set, the location will be set to
 *		front left or front right accordingly. If the location is not supported, a
 *		warning will be logged and the LE_AUDIO_EVT_NO_VALID_CFG event will be
 *		published.
 *
 * @param conn		Connection pointer to the unicast_server where the location was discovered.
 * @param dir		Direction of the location (sink or source).
 * @param loc		Discovered location.
 */
static void unicast_client_location_cb(struct bt_conn *conn, enum bt_audio_dir dir,
				       enum bt_audio_location loc)
{
	int ret;
	struct server_store *server = NULL;

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	ret = srv_store_from_conn_get(conn, &server);
	if (ret) {
		LOG_ERR("%s: Unknown connection, should not reach here", __func__);
		srv_store_unlock();
		return;
	}

	if (loc & BT_AUDIO_LOCATION_FRONT_LEFT && loc & BT_AUDIO_LOCATION_FRONT_RIGHT &&
	    dir == BT_AUDIO_DIR_SINK) {
		LOG_INF("Both front left and right channel locations are set, stereo device found");

		ret = srv_store_location_set(
			conn, dir, BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT);
		if (ret) {
			LOG_ERR("Failed to set location for conn %p, dir %d, loc %d: %d",
				(void *)conn, dir, loc, ret);
			srv_store_unlock();
			return;
		}

		server->name = "STEREO";
		srv_store_unlock();
		return;
	}

	if ((loc & BT_AUDIO_LOCATION_FRONT_LEFT) || (loc & BT_AUDIO_LOCATION_BACK_LEFT) ||
	    (loc & BT_AUDIO_LOCATION_FRONT_LEFT_OF_CENTER) || (loc & BT_AUDIO_LOCATION_SIDE_LEFT) ||
	    (loc & BT_AUDIO_LOCATION_TOP_FRONT_LEFT) || (loc & BT_AUDIO_LOCATION_TOP_BACK_LEFT) ||
	    (loc & BT_AUDIO_LOCATION_TOP_SIDE_LEFT) ||
	    (loc & BT_AUDIO_LOCATION_BOTTOM_FRONT_LEFT) ||
	    (loc & BT_AUDIO_LOCATION_FRONT_LEFT_WIDE) || (loc & BT_AUDIO_LOCATION_LEFT_SURROUND) ||
	    (loc == BT_AUDIO_LOCATION_MONO_AUDIO)) {
		ret = srv_store_location_set(conn, dir, BT_AUDIO_LOCATION_FRONT_LEFT);
		if (ret) {
			LOG_ERR("Failed to set location for conn %p, dir %d, loc %d: %d",
				(void *)conn, dir, loc, ret);
			srv_store_unlock();
			return;
		}

		if (dir == BT_AUDIO_DIR_SINK) {
			server->name = "LEFT";
		}
	} else if ((loc & BT_AUDIO_LOCATION_FRONT_RIGHT) || (loc & BT_AUDIO_LOCATION_BACK_RIGHT) ||
		   (loc & BT_AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER) ||
		   (loc & BT_AUDIO_LOCATION_SIDE_RIGHT) ||
		   (loc & BT_AUDIO_LOCATION_TOP_FRONT_RIGHT) ||
		   (loc & BT_AUDIO_LOCATION_TOP_BACK_RIGHT) ||
		   (loc & BT_AUDIO_LOCATION_TOP_SIDE_RIGHT) ||
		   (loc & BT_AUDIO_LOCATION_BOTTOM_FRONT_RIGHT) ||
		   (loc & BT_AUDIO_LOCATION_FRONT_RIGHT_WIDE) ||
		   (loc & BT_AUDIO_LOCATION_RIGHT_SURROUND)) {
		ret = srv_store_location_set(conn, dir, BT_AUDIO_LOCATION_FRONT_RIGHT);
		if (ret) {
			LOG_ERR("Failed to set location for conn %p, dir %d, loc %d: %d",
				(void *)conn, dir, loc, ret);
			srv_store_unlock();
			return;
		}

		if (dir == BT_AUDIO_DIR_SINK) {
			server->name = "RIGHT";
		}
	} else {
		LOG_WRN("Channel location not supported: %d", loc);
		le_audio_event_publish(LE_AUDIO_EVT_NO_VALID_CFG, conn, NULL, dir);
	}

	srv_store_unlock();
}

/**
 * @brief	Callback for when available contexts are discovered. This function will store the
 *		available contexts in the server_store struct for the corresponding connection.
 *
 * @param conn		Connection pointer to the unicast_server where the available contexts
 *			was discovered.
 * @param snk_ctx	Available sink contexts.
 * @param src_ctx	Available source contexts.
 */
static void available_contexts_cb(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				  enum bt_audio_context src_ctx)
{
	int ret;

	LOG_DBG("conn: %p, snk ctx %d src ctx %d", (void *)conn, snk_ctx, src_ctx);

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	ret = srv_store_avail_context_set(conn, snk_ctx, src_ctx);
	if (ret) {
		LOG_ERR("Failed to set available contexts for conn %p, snk ctx %d src ctx %d: %d",
			(void *)conn, snk_ctx, src_ctx, ret);
	}

	srv_store_unlock();
}

/**
 * @brief	Callback for when a codec capability is discovered. This function will store the
 *		codec capability in the server_store struct for the corresponding connection and
 *		direction.
 *
 * @param conn		Connection pointer to the unicast_server where the codec capability was
 *			discovered.
 * @param dir		Direction of the codec capability (sink or source).
 * @param codec		Pointer to the discovered codec capability.
 */
static void pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			  const struct bt_audio_codec_cap *codec)
{
	int ret;

	if (codec->id != BT_HCI_CODING_FORMAT_LC3) {
		LOG_DBG("Only the LC3 codec is supported");
		return;
	}

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	ret = srv_store_codec_cap_set(conn, dir, codec);
	if (ret) {
		LOG_ERR("Failed to set codec capability: %d", ret);
	}

	srv_store_unlock();
}

/**
 * @brief	Callback for when an endpoint is discovered. This function will store the endpoint
 *		in the server_store struct for the corresponding connection and direction.
 *
 * @param conn		Connection pointer to the unicast_server where the endpoint was discovered.
 * @param dir		Direction of the endpoint (sink or source).
 * @param ep		Pointer to the discovered endpoint. NULL if the discovery is complete.
 */
static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	int ret;

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret) {
		LOG_ERR("%s: Unknown connection, should not reach here", __func__);
		srv_store_unlock();
		return;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		if (ep != NULL) {
			if (server->snk.num_eps >= ARRAY_SIZE(server->snk.eps)) {
				LOG_WRN("No more space (%d) for sink endpoints, increase "
					"CONFIG_SNK_EP_COUNT_MAX (%d)",
					server->snk.num_eps, ARRAY_SIZE(server->snk.eps));
				srv_store_unlock();
				return;
			}

			server->snk.eps[server->snk.num_eps] = ep;
			server->snk.num_eps++;
		}

		if (server->snk.eps[0] == NULL) {
			LOG_WRN("No sink endpoints found");
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		if (ep != NULL) {
			if (server->src.num_eps >= ARRAY_SIZE(server->src.eps)) {
				LOG_WRN("No more space for source endpoints, increase "
					"CONFIG_SRC_EP_COUNT_MAX");
				srv_store_unlock();
				return;
			}

			server->src.eps[server->src.num_eps] = ep;
			server->src.num_eps++;
		}

		if (server->src.eps[0] == NULL) {
			LOG_WRN("No source endpoints found");
		}
	} else {
		LOG_WRN("Endpoint direction not recognized: %d", dir);
	}

	srv_store_unlock();
}

/**
 * @brief	Callback when sink discovery has finished. This function will also set the codec
 *		configuration to be used in the preset(s) based on the discovered codec capabilities
 *		and channel allocation.
 *
 * @note	If a stereo sink is found, the preset for both left and right channel will be set to
 *		the same preset, only differing in channel allocation.
 *
 * @param conn		Connection pointer to the unicast_server where the discovery was done.
 * @param err		Error code, 0 if successful.
 * @param server	Server pointer to the server_store struct where the discovered data will be
 *			stored.
 */
static void discover_cb_sink(struct bt_conn *conn, int err, struct server_store *server)
{
	int ret;

	if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		LOG_WRN("No sinks found");
		return;
	}

	if (err) {
		LOG_ERR("Discovery failed: %d", err);
		return;
	}

	uint32_t valid_sink_caps = 0;

	ret = srv_store_valid_codec_cap_check(conn, BT_AUDIO_DIR_SINK, &valid_sink_caps, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to check for valid codec capabilities: %d", ret);
		return;
	}

	if (!valid_sink_caps) {
		/* NOTE: The string below is used by the Nordic CI system */
		LOG_WRN("No valid codec capability found for %s sink", server->name);
		return;
	}

	/* Get the valid configuration to set for this stream and put that in the corresponding
	 * preset
	 */
	if (POPCOUNT(server->snk.locations) == 1 && server->snk.num_eps >= 1) {
		ret = bt_audio_codec_cfg_set_val(
			&server->snk.lc3_preset[0].codec_cfg, BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
			(uint8_t *)&server->snk.locations, sizeof(server->snk.locations));
		if (ret < 0) {
			LOG_ERR("Failed to set codec channel allocation: %d", ret);
			return;
		}

	} else if (POPCOUNT(server->snk.locations) == 2 && server->snk.num_eps >= 2 &&
		   (server->snk.locations &
		    (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT)) ==
			   (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT)) {

		uint32_t left = BT_AUDIO_LOCATION_FRONT_LEFT;
		uint32_t right = BT_AUDIO_LOCATION_FRONT_RIGHT;

		LOG_INF("STEREO sink found, setting up stereo codec capabilities");
		ret = bt_audio_codec_cfg_set_val(&server->snk.lc3_preset[0].codec_cfg,
						 BT_AUDIO_CODEC_CFG_CHAN_ALLOC, (uint8_t *)&left,
						 sizeof(server->snk.locations));
		if (ret < 0) {
			LOG_ERR("Failed to set codec channel allocation: %d", ret);
			return;
		}

		/* Use the same preset for both channels */
		memcpy(&server->snk.lc3_preset[1], &server->snk.lc3_preset[0],
		       sizeof(struct bt_bap_lc3_preset));

		ret = bt_audio_codec_cfg_set_val(&server->snk.lc3_preset[1].codec_cfg,
						 BT_AUDIO_CODEC_CFG_CHAN_ALLOC, (uint8_t *)&right,
						 sizeof(server->snk.locations));
		if (ret < 0) {
			LOG_ERR("Failed to set codec channel allocation: %d", ret);
			return;
		}
	} else {
		LOG_WRN("Unsupported unicast server/headset configuration");
		LOG_WRN("Number of sink channels: %d, number of sink endpoints: %d",
			POPCOUNT(server->snk.locations), server->snk.num_eps);
		le_audio_event_publish(LE_AUDIO_EVT_NO_VALID_CFG, conn, NULL, BT_AUDIO_DIR_SINK);
		return;
	}
}

/**
 * @brief	Callback when source discovery has finished. This function will also set the codec
 *		configuration to be used in the preset(s) based on the discovered codec capabilities
 *		and channel allocation.
 *
 * @param conn		Connection pointer to the unicast_server where the discovery was done.
 * @param err		Error code, 0 if successful.
 * @param server	Server pointer to the server_store struct where the discovered data will be
 *			stored.
 */
static void discover_cb_source(struct bt_conn *conn, int err, struct server_store *server)
{
	int ret;

	if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		LOG_WRN("No sources found");
		return;
	}

	if (err) {
		LOG_ERR("Discovery failed: %d", err);
		return;
	}

	uint32_t valid_source_caps = 0;

	ret = srv_store_valid_codec_cap_check(conn, BT_AUDIO_DIR_SOURCE, &valid_source_caps, NULL,
					      0);
	if (!valid_source_caps) {
		LOG_WRN("No valid codec capability found for %s source", server->name);
		return;
	}

	ret = bt_audio_codec_cfg_set_val(
		&server->src.lc3_preset[0].codec_cfg, BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
		(uint8_t *)&server->src.locations, sizeof(server->src.locations));
	if (ret < 0) {
		LOG_ERR("Failed to set codec channel allocation: %d", ret);
		return;
	}
}

static void discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	int ret;

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret) {
		LOG_ERR("%s: Unknown connection, should not reach here", __func__);
		srv_store_unlock();
		return;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		server->snk.waiting_for_disc = false;
		discover_cb_sink(conn, err, server);
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		server->src.waiting_for_disc = false;
		discover_cb_source(conn, err, server);
	} else {
		LOG_ERR("%s: Unknown direction: %d", __func__, dir);
		srv_store_unlock();
		return;
	}

	if (server->src.waiting_for_disc) {
		ret = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SOURCE);
		if (ret) {
			LOG_WRN("Failed to start source discovery: %d", ret);
		}

		srv_store_unlock();
		return;
	}

	if (!playing_state) {
		/* Since we are not in a playing state we return before starting the new streams */
		srv_store_unlock();
		return;
	}

	srv_store_unlock();
	k_work_submit(&cap_start_work);
}

#if (CONFIG_BT_AUDIO_TX)
static void stream_sent_cb(struct bt_bap_stream *stream)
{
	int ret;
	struct stream_index idx;
	uint8_t state;

	ret = le_audio_ep_state_get(stream->ep, &state);
	if (ret) {
		LOG_ERR("Failed to get endpoint state: %d", ret);
		return;
	}

	if (state == BT_BAP_EP_STATE_STREAMING) {
		ret = stream_idx_get(stream, &idx);
		if (ret) {
			LOG_ERR("%s: Failed to get stream index: %d", __func__, ret);
			return;
		}
		ERR_CHK(bt_le_audio_tx_stream_sent(idx));
	} else {
		LOG_WRN("Not in streaming state: %d", state);
	}
}
#endif /* CONFIG_BT_AUDIO_TX */

static bool new_pres_dly_us_set(struct bt_cap_stream *stream, void *user_data)
{
	uint32_t *new_pres_dly_us = (uint32_t *)user_data;

	stream->bap_stream.qos->pd = *new_pres_dly_us;

	return false;
}

static void stream_configured_cb(struct bt_bap_stream *stream,
				 const struct bt_bap_qos_cfg_pref *pref)
{
	int ret;
	enum bt_audio_dir dir;
	uint32_t new_pres_dly_us = 0;

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_stream_get(stream, &server);
	if (ret) {
		LOG_ERR("Unknown stream, should not reach here");
		srv_store_unlock();
		return;
	}

	dir = le_audio_stream_dir_get(stream);
	if ((dir != BT_AUDIO_DIR_SINK && dir != BT_AUDIO_DIR_SOURCE) || (dir <= 0)) {
		if (dir <= 0) {
			LOG_ERR("Failed to get dir of stream %p", (void *)stream);
		} else {
			LOG_ERR("Endpoint direction not recognized: %d", dir);
		}

		srv_store_unlock();
		return;
	}

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("%s %s stream configured", server->name,
		(dir == BT_AUDIO_DIR_SINK) ? "sink" : "source");
	le_audio_print_codec(stream->codec_cfg, dir);

	LOG_DBG("Configured Stream info: %s, %p, dir %d", server->name, (void *)stream, dir);

	bool group_reconfigure_needed = false;
	uint32_t existing_pres_dly_us = 0;

	ret = srv_store_pres_dly_find(stream, &new_pres_dly_us, &existing_pres_dly_us, pref,
				      &group_reconfigure_needed);
	if (ret) {
		LOG_ERR("Cannot get a valid presentation delay");
		srv_store_unlock();
		return;
	}

	if (server->src.waiting_for_disc) {
		srv_store_unlock();
		return;
	}

	srv_store_unlock();

	if (((new_pres_dly_us != stream->qos->pd) &&
	     le_audio_ep_state_check(stream->ep, BT_BAP_EP_STATE_CODEC_CONFIGURED)) ||
	    group_reconfigure_needed) {
		LOG_INF("Stream QoS PD: %d, prev group PD: %d, new PD %d", stream->qos->pd,
			existing_pres_dly_us, new_pres_dly_us);
		bt_cap_unicast_group_foreach_stream(unicast_group, new_pres_dly_us_set,
						    &new_pres_dly_us);
	}

	le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED, stream->conn, stream, dir);
}

static void stream_qos_set_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("QoS set cb");
}

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Stream enabled: %p", (void *)stream);
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	int ret;
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", (void *)stream);
		return;
	}

	struct stream_index idx = {0};

	ret = stream_idx_get(stream, &idx);
	if (ret) {
		LOG_ERR("%s: Failed to get stream index: %d", __func__, ret);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_TX)) {
		ERR_CHK(bt_le_audio_tx_stream_started(idx));
	}

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Stream %p started, idx: %d %d %d", (void *)stream, idx.lvl1, idx.lvl2, idx.lvl3);

	le_audio_event_publish(LE_AUDIO_EVT_STREAMING, stream->conn, stream, dir);
}

static void stream_metadata_updated_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Audio Stream %p metadata updated", (void *)stream);
}

static void stream_disabled_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Audio Stream %p disabled", (void *)stream);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int ret;

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Stream %p stopped. Reason %d", (void *)stream, reason);

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	/* Check if the other streams are streaming, send event if not */
	if (srv_store_all_ep_state_count(BT_BAP_EP_STATE_STREAMING, BT_AUDIO_DIR_SINK) ||
	    srv_store_all_ep_state_count(BT_BAP_EP_STATE_STREAMING, BT_AUDIO_DIR_SOURCE)) {
		LOG_DBG("Other streams are still streaming, not publishing NOT_STREAMING event");
		srv_store_unlock();
		return;
	}

	srv_store_unlock();

	enum bt_audio_dir dir = 0;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", (void *)stream);
	}

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn, stream, dir);
}

/**
 * @brief	Function to check if all streams in the unicast group have been released.
 *
 * @param stream	Stream to check.
 * @param user_data	User data, not used.
 *
 * @return		True if a stream is found that is not released, false if all streams are
 *			released.
 */
static bool all_streams_released_check(struct bt_cap_stream *stream, void *user_data)
{
	ARG_UNUSED(user_data);

	if (stream->bap_stream.ep != NULL) {
		LOG_DBG("stream %p is not released", stream);
		/* Found a stream that is not released, will stop iterating */
		return true;
	}

	return false;
}

static void stream_released_cb(struct bt_bap_stream *stream)
{
	int ret;

	LOG_DBG("Audio Stream %p released", (void *)stream);

	/* Check if unicast_group_recreate has been requested */
	if (unicast_group_created == false) {
		/* Check if all streams have been released, only delete group if they all are */
		ret = bt_cap_unicast_group_foreach_stream(unicast_group, all_streams_released_check,
							  NULL);
		if (ret == -ECANCELED) {
			LOG_DBG("Not all streams have been released, not deleting group");
			return;
		}

		ret = bt_cap_unicast_group_delete(unicast_group);
		if (ret) {
			LOG_ERR("Failed to delete unicast group: %d", ret);
		}

		/* Create a new unicast group */
		k_work_submit(&cap_start_work);
	}
}

#if (CONFIG_BT_AUDIO_RX)
static void stream_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *audio_frame)
{
	int ret;
	struct audio_metadata meta;

	if (receive_cb == NULL) {
		LOG_ERR("The RX callback has not been set");
		return;
	}

	ret = le_audio_metadata_populate(&meta, stream, info, audio_frame);
	if (ret) {
		LOG_ERR("Failed to populate meta data: %d", ret);
		return;
	}

	struct stream_index idx;

	ret = stream_idx_get(stream, &idx);
	if (ret) {
		LOG_ERR("%s: Failed to get stream index: %d", __func__, ret);
		return;
	}

	receive_cb(audio_frame, &meta, idx.lvl3);
}
#endif /* (CONFIG_BT_AUDIO_RX) */

static struct bt_bap_stream_ops stream_ops = {
	.configured = stream_configured_cb,
	.qos_set = stream_qos_set_cb,
	.enabled = stream_enabled_cb,
	.started = stream_started_cb,
	.metadata_updated = stream_metadata_updated_cb,
	.disabled = stream_disabled_cb,
	.stopped = stream_stopped_cb,
	.released = stream_released_cb,
#if (CONFIG_BT_AUDIO_RX)
	.recv = stream_recv_cb,
#endif /* (CONFIG_BT_AUDIO_RX) */
#if (CONFIG_BT_AUDIO_TX)
	.sent = stream_sent_cb,
#endif /* (CONFIG_BT_AUDIO_TX) */
};

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = available_contexts_cb,
	.pac_record = pac_record_cb,
	.endpoint = endpoint_cb,
	.discover = discover_cb,
};

static void unicast_discovery_complete_cb(struct bt_conn *conn, int err,
					  const struct bt_csip_set_coordinator_set_member *member,
					  const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	int ret;
	struct le_audio_msg msg;
	struct server_store *server = NULL;

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	ret = srv_store_from_conn_get(conn, &server);
	if (ret) {
		LOG_ERR("%s: Unknown connection, should not reach here", __func__);
		srv_store_unlock();
		return;
	}

	if (err || csis_inst == NULL) {
		LOG_WRN("Got err: %d from conn: %p", err, (void *)conn);
		msg.set_size = 0;
		msg.sirk = NULL;
	} else {
		LOG_DBG("\tErr: %d, set_size: %d", err, csis_inst->info.set_size);
		LOG_HEXDUMP_DBG(csis_inst->info.sirk, BT_CSIP_SIRK_SIZE, "\tSIRK:");

		server->member = member;
		msg.set_size = csis_inst->info.set_size;
		msg.sirk = csis_inst->info.sirk;
	}

	LOG_DBG("Unicast discovery complete cb");

	msg.event = LE_AUDIO_EVT_COORD_SET_DISCOVERED;
	msg.conn = conn;

	ret = zbus_chan_pub(&le_audio_chan, &msg, LE_AUDIO_ZBUS_EVENT_WAIT_TIME);
	ERR_CHK(ret);

	srv_store_unlock();
}

static void unicast_start_complete_cb(int err, struct bt_conn *conn)
{
	k_sem_give(&sem_cap_procedure_proceed);

	if (err) {
		LOG_WRN("Failed start_complete for conn: %p, err: %d", (void *)conn, err);
	}

	LOG_DBG("Unicast start complete cb");
	playing_state = true;

	cap_proc_waiting_check();
}

static void unicast_update_complete_cb(int err, struct bt_conn *conn)
{
	k_sem_give(&sem_cap_procedure_proceed);

	if (err) {
		LOG_WRN("Failed update_complete for conn: %p, err: %d", (void *)conn, err);
	}

	LOG_DBG("Unicast update complete cb");
}

static void unicast_stop_complete_cb(int err, struct bt_conn *conn)
{
	k_sem_give(&sem_cap_procedure_proceed);

	if (err) {
		LOG_WRN("Failed stop_complete for conn: %p, err: %d", (void *)conn, err);
	}

	LOG_DBG("Unicast stop complete cb");
	playing_state = false;

	cap_proc_waiting_check();
}

static struct bt_cap_initiator_cb cap_cbs = {
	.unicast_discovery_complete = unicast_discovery_complete_cb,
	.unicast_start_complete = unicast_start_complete_cb,
	.unicast_update_complete = unicast_update_complete_cb,
	.unicast_stop_complete = unicast_stop_complete_cb,
};

int le_audio_concurrent_sync_num_get(void)
{
	return 1; /* Only one stream supported at the moment */
}

int unicast_client_config_get(struct bt_bap_stream *stream, uint32_t *bitrate,
			      uint32_t *sampling_rate_hz)
{
	int ret;

	if (stream == NULL) {
		LOG_ERR("No valid stream pointer received");
		return -EINVAL;
	}

	if (bitrate == NULL && sampling_rate_hz == NULL) {
		LOG_ERR("No valid pointers received");
		return -ENXIO;
	}

	if (stream->codec_cfg == NULL) {
		LOG_ERR("No codec found for the stream");
		return -ENXIO;
	}

	if (sampling_rate_hz != NULL) {
		ret = le_audio_freq_hz_get(stream->codec_cfg, sampling_rate_hz);
		if (ret) {
			LOG_ERR("Invalid sampling frequency: %d", ret);
			return -ENXIO;
		}
	}

	if (bitrate != NULL) {
		ret = le_audio_bitrate_get(stream->codec_cfg, bitrate);
		if (ret) {
			LOG_ERR("Unable to calculate bitrate: %d", ret);
			return -ENXIO;
		}
	}

	return 0;
}

void unicast_client_conn_disconnected(struct bt_conn *conn)
{
	int ret;

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret) {
		LOG_DBG("%s: Unknown server", __func__);
		srv_store_unlock();
		return;
	}

	/* Clear endpoint information */
	memset(&server->snk.eps, 0, sizeof(server->snk.eps));
	memset(&server->src.eps, 0, sizeof(server->src.eps));

	server->snk.num_eps = 0;
	server->src.num_eps = 0;

	srv_store_unlock();
}

int unicast_client_discover(struct bt_conn *conn, enum unicast_discover_dir dir)
{
	int ret;

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return ret;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret) {
		LOG_ERR("%s: Unknown connection, should not reach here", __func__);
		srv_store_unlock();
		return ret;
	}

	/* Register ops */
	for (int i = 0; i < ARRAY_SIZE(server->snk.cap_streams); i++) {
		bt_cap_stream_ops_register(&server->snk.cap_streams[i], &stream_ops);
	}

	for (int i = 0; i < ARRAY_SIZE(server->src.cap_streams); i++) {
		bt_cap_stream_ops_register(&server->src.cap_streams[i], &stream_ops);
	}

	ret = bt_cap_initiator_unicast_discover(conn);
	if (ret) {
		LOG_WRN("Failed to start cap discover: %d", ret);
		srv_store_unlock();
		return ret;
	}

	if (dir & BT_AUDIO_DIR_SOURCE) {
		server->src.waiting_for_disc = true;
	}

	if (dir & BT_AUDIO_DIR_SINK) {
		server->snk.waiting_for_disc = true;
	}

	if (dir == UNICAST_SERVER_BIDIR) {
		/* If we need to discover both source and sink, do sink first */
		ret = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SINK);
		srv_store_unlock();
		return ret;
	}

	ret = bt_bap_unicast_client_discover(conn, dir);
	if (ret) {
		LOG_WRN("Failed to discover %d", ret);
		srv_store_unlock();
		return ret;
	}

	srv_store_unlock();

	return 0;
}

int unicast_client_start(uint8_t cig_index)
{
	int ret;

	ret = k_sem_take(&sem_cap_procedure_proceed, K_NO_WAIT);
	if (ret == -EBUSY) {
		/* Ongoing procedure, try again later */
		enum cap_procedure_type proc_type;

		proc_type = CAP_PROCEDURE_START;

		ret = k_msgq_put(&cap_proc_q, &proc_type, K_NO_WAIT);
		if (ret) {
			LOG_WRN("Failed to put start procedure in queue: %d", ret);
		}

		return ret;
	} else if (ret) {
		LOG_ERR("Failed to take sem_cap_procedure_proceed: %d", ret);
		return ret;
	}

	/* Start all unicast_servers with valid endpoints */
	struct bt_cap_unicast_audio_start_stream_param
		cap_stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
				  CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];

	struct bt_cap_unicast_audio_start_param param;

	param.stream_params = cap_stream_params;
	param.count = 0;
	param.type = BT_CAP_SET_TYPE_AD_HOC;

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return ret;
	}

	uint8_t num_servers = srv_store_num_get(true);

	for (int i = 0; i < num_servers; i++) {
		struct server_store *server = NULL;

		ret = srv_store_server_get(&server, i);
		if (ret) {
			LOG_ERR("Failed to get server store for index %d: %d", i, ret);
			continue;
		}

		/* Check if we have valid sink endpoints to start */
		for (int j = 0; j < MIN(server->snk.num_eps, POPCOUNT(server->snk.locations));
		     j++) {
			uint8_t state = BT_BAP_EP_STATE_IDLE;

			ret = le_audio_ep_state_get(server->snk.eps[j], &state);
			if (state == BT_BAP_EP_STATE_STREAMING || ret) {
				LOG_DBG("Sink endpoint is already streaming, skipping start");
				continue;
			}

			cap_stream_params[param.count].member.member = server->conn;
			cap_stream_params[param.count].stream = &server->snk.cap_streams[j];
			cap_stream_params[param.count].ep = server->snk.eps[j];
			cap_stream_params[param.count].codec_cfg =
				&server->snk.lc3_preset[j].codec_cfg;
			param.count++;
		}

		/* Check if we have valid source endpoints to start */
		for (int j = 0; j < MIN(server->src.num_eps, POPCOUNT(server->src.locations));
		     j++) {
			uint8_t state = BT_BAP_EP_STATE_IDLE;

			ret = le_audio_ep_state_get(server->src.eps[j], &state);
			if (state == BT_BAP_EP_STATE_STREAMING || ret) {
				LOG_DBG("Source endpoint is already streaming, skipping start");
				continue;
			}

			cap_stream_params[param.count].member.member = server->conn;
			cap_stream_params[param.count].stream = &server->src.cap_streams[j];
			cap_stream_params[param.count].ep = server->src.eps[j];
			cap_stream_params[param.count].codec_cfg =
				&server->src.lc3_preset[j].codec_cfg;
			param.count++;
		}
	}

	if (param.count == 0) {
		LOG_DBG("No streams to start");
		k_sem_give(&sem_cap_procedure_proceed);
		srv_store_unlock();
		return -EIO;
	}

	ret = bt_cap_initiator_unicast_audio_start(&param);
	if (ret) {
		LOG_ERR("Failed to start unicast sink audio: %d", ret);
		k_sem_give(&sem_cap_procedure_proceed);
		srv_store_unlock();
		return ret;
	}

	srv_store_unlock();

	return 0;
}

int unicast_client_stop(uint8_t cig_index)
{
	int ret;
	struct bt_cap_stream *streams[(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT +
				       CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT) *
				      CONFIG_BT_MAX_CONN];
	static struct bt_cap_unicast_audio_stop_param param;

	ret = k_sem_take(&sem_cap_procedure_proceed, K_NO_WAIT);
	if (ret == -EBUSY) {
		enum cap_procedure_type proc_type;

		proc_type = CAP_PROCEDURE_STOP;

		ret = k_msgq_put(&cap_proc_q, &proc_type, K_NO_WAIT);
		if (ret) {
			LOG_WRN("Failed to put stop procedure in queue: %d", ret);
		}

		return ret;
	} else if (ret) {
		LOG_ERR("Failed to take sem_cap_procedure_proceed: %d", ret);
		return ret;
	}

	if (cig_index >= CONFIG_BT_ISO_MAX_CIG) {
		LOG_ERR("Trying to stop CIG %d out of %d", cig_index, CONFIG_BT_ISO_MAX_CIG);
		return -EINVAL;
	}

	param.streams = streams;
	param.count = 0;
	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.release = true;

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, NULL, NULL, 0);

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		k_sem_give(&sem_cap_procedure_proceed);
		return ret;
	}

	uint8_t num_servers = srv_store_num_get(true);

	for (int i = 0; i < num_servers; i++) {
		struct server_store *server = NULL;

		ret = srv_store_server_get(&server, i);
		if (ret) {
			LOG_ERR("Failed to get server store for index %d: %d", i, ret);
			continue;
		}

		if (server->snk.num_eps > 0) {
			for (int j = 0; j < server->snk.num_eps; j++) {
				uint8_t state = BT_BAP_EP_STATE_IDLE;

				ret = le_audio_ep_state_get(server->snk.eps[j], &state);
				if (state != BT_BAP_EP_STATE_STREAMING || ret) {
					LOG_DBG("Sink endpoint is not streaming, skipping stop");
					continue;
				}

				streams[param.count++] = &server->snk.cap_streams[j];
			}
		}

		if (server->src.num_eps > 0) {
			for (int j = 0; j < server->src.num_eps; j++) {
				uint8_t state = BT_BAP_EP_STATE_IDLE;

				ret = le_audio_ep_state_get(server->src.eps[j], &state);
				if (state != BT_BAP_EP_STATE_STREAMING || ret) {
					LOG_DBG("Source endpoint is not streaming, skipping stop");
					continue;
				}

				streams[param.count++] = &server->src.cap_streams[j];
			}
		}
	}

	if (param.count == 0) {
		LOG_DBG("No streams to stop");
		srv_store_unlock();
		k_sem_give(&sem_cap_procedure_proceed);
		return -EIO;
	}

	ret = bt_cap_initiator_unicast_audio_stop(&param);
	if (ret) {
		LOG_ERR("Failed to stop unicast audio: %d", ret);
		srv_store_unlock();
		k_sem_give(&sem_cap_procedure_proceed);
		return ret;
	}

	srv_store_unlock();
	return 0;
}

int unicast_client_send(struct net_buf const *const audio_frame, uint8_t cig_index)
{
#if (CONFIG_BT_AUDIO_TX)
	int ret;
	uint8_t num_active_streams = 0;

	if (cig_index >= CONFIG_BT_ISO_MAX_CIG) {
		LOG_ERR("Trying to send to CIG %d out of %d", cig_index, CONFIG_BT_ISO_MAX_CIG);
		return -EINVAL;
	}

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return ret;
	}

	uint8_t num_streaming =
		srv_store_all_ep_state_count(BT_BAP_EP_STATE_STREAMING, BT_AUDIO_DIR_SINK);
	struct le_audio_tx_info tx[num_streaming];

	for (int i = 0; i < srv_store_num_get(true); i++) {
		struct server_store *server = NULL;

		ret = srv_store_server_get(&server, i);
		if (ret) {
			LOG_ERR("Failed to get server store for index %d: %d", i, ret);
			continue;
		}

		for (int j = 0; j < server->snk.num_eps; j++) {
			/* Skip unicast_servers not in a streaming state */
			if (!le_audio_ep_state_check(server->snk.cap_streams[j].bap_stream.ep,
						     BT_BAP_EP_STATE_STREAMING)) {
				continue;
			}

			/* Set cap stream pointer */
			tx[num_active_streams].cap_stream = &server->snk.cap_streams[j];

			/* Set index */
			ret = stream_idx_get(&server->snk.cap_streams[j].bap_stream,
					     &tx[num_active_streams].idx);
			if (ret) {
				LOG_ERR("%s: Failed to get stream index: %d", __func__, ret);
				srv_store_unlock();
				return ret;
			}

			const uint8_t *loc;

			bt_audio_codec_cfg_get_val(server->snk.cap_streams[j].bap_stream.codec_cfg,
						   BT_AUDIO_CODEC_CFG_CHAN_ALLOC, &loc);

			/* Set channel location */
			/* Both mono and left unicast_servers will receive left channel */

			tx[num_active_streams].audio_channel =
				*loc == BT_AUDIO_LOCATION_FRONT_RIGHT ? AUDIO_CH_R : AUDIO_CH_L;

			num_active_streams++;
		}
	}

	if (num_active_streams == 0) {
		/* This could happen if we had a sink + source and just the sink disconnected */
		LOG_DBG("No active streams");
		srv_store_unlock();
		return -ECANCELED;
	}

	ret = bt_le_audio_tx_send(audio_frame, tx, num_active_streams);
	if (ret) {
		srv_store_unlock();
		return ret;
	}

	srv_store_unlock();
#endif /* (CONFIG_BT_AUDIO_TX) */
	return 0;
}

int unicast_client_disable(uint8_t cig_index)
{
	ARG_UNUSED(cig_index);

	return -ENOTSUP;
}

int unicast_client_enable(uint8_t cig_index, le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	ret = srv_store_lock(CAP_PROCED_SEM_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return ret;
	}

	ret = srv_store_init();
	if (ret) {
		srv_store_unlock();
		return ret;
	}

	if (recv_cb == NULL) {
		LOG_ERR("Receive callback is NULL");
		srv_store_unlock();
		return -EINVAL;
	}

	receive_cb = recv_cb;

	ret = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (ret) {
		LOG_ERR("Failed to register client callbacks: %d", ret);
		srv_store_unlock();
		return ret;
	}

	ret = bt_cap_initiator_register_cb(&cap_cbs);
	if (ret) {
		LOG_ERR("Failed to register cap callbacks: %d", ret);
		srv_store_unlock();
		return ret;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_TX)) {
		bt_le_audio_tx_init();
	}

	initialized = true;

	srv_store_unlock();

	return 0;
}
