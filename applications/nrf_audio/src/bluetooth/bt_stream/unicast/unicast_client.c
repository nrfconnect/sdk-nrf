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
#include "bt_mgmt.h"
#include "server_store.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(unicast_client, 4);

static struct k_thread cap_state_machine_thread_data;
static k_tid_t cap_state_machine_thread_id;
#define CAP_STATE_MACHINE_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(cap_state_machine_thread_stack, CAP_STATE_MACHINE_STACK_SIZE);

ZBUS_CHAN_DEFINE(le_audio_chan, struct le_audio_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

enum cap_action_type {
	CAP_ACTION_STOP, /* Highest pri */
	CAP_ACTION_STOP_THEN_START,
	CAP_ACTION_START, /* lower pri */
	CAP_ACTION_MAX,
};

K_MUTEX_DEFINE(next_action_lock);
K_SEM_DEFINE(cap_state_machine_sem, 1, 1);
static enum cap_action_type next_action;

static ATOMIC_DEFINE(cap_state_machine_delay_start, 1);
static struct k_poll_signal poll_sig = K_POLL_SIGNAL_INITIALIZER(poll_sig);
static struct k_poll_event poll_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &poll_sig);

static const k_timeout_t LOCK_WAIT_TIME_MS = K_MSEC(10);
/**
 * @brief	Signal that we want to start a CAP action.
 *
 * @param	action	The CAP action type to signal.
 */
static void cap_thread_next_evt_set(enum cap_action_type action)
{
	k_mutex_lock(&next_action_lock, K_FOREVER);
	next_action = action;
	k_mutex_unlock(&next_action_lock);
	k_poll_signal_raise(poll_evt.signal, 1);
}

/**
 * @brief	Signal that a CAP action has been completed.
 *
 * @param	err	The error code of the completed CAP action. 0 if successful.
 * 		If there was an failure, we give the state machine a pause before continuing.
 */
static void cap_thread_cap_action_complete(int err)
{
	if (err) {
		atomic_set_bit(cap_state_machine_delay_start, 1);
	}

	k_sem_give(&cap_state_machine_sem);
}

enum cap_procedure_type {
	CAP_PROCEDURE_START = 1,
	CAP_PROCEDURE_UPDATE,
	CAP_PROCEDURE_STOP,
};

/* For unicast (as opposed to broadcast) level 2/subgroup is not defined in the specification */
#define LVL2		 0
/* Will return 1 if x == 0, due to how locations are defined in LE Audio */
#define POPCOUNT_ZERO(x) ((x) == 0 ? 1 : POPCOUNT(x))

struct discover_dir {
	struct bt_conn *conn;
	bool sink;
	bool source;
};

BUILD_ASSERT(CONFIG_BT_ISO_MAX_CIG == 1, "Only one CIG is supported");

static le_audio_receive_cb receive_cb;

BT_LE_AUDIO_TX_DEFINE(bt_le_audio_tx);

static struct bt_cap_unicast_group *unicast_group;
static bool unicast_group_created;
static struct bt_cap_unicast_group_stream_pair_param
	pair_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
static struct bt_cap_unicast_group_param group_param = {0};
static struct group_streams_populate_data data;

static bool in_playing_state = true;

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

struct num_eps_total {
	uint16_t sink;
	uint16_t source;
};

static bool num_eps_count(struct server_store *server, void *user_data)
{
	struct num_eps_total *num_eps = (struct num_eps_total *)user_data;

	num_eps->sink += server->snk.num_eps;
	num_eps->source += server->src.num_eps;

	return true;
}

static int group_info_get(const struct bt_cap_unicast_group *cap_unicast_group,
			  struct bt_bap_unicast_group_info *bap_info)
{

	int ret;

	struct bt_cap_unicast_group_info cap_info;

	ret = bt_cap_unicast_group_get_info(cap_unicast_group, &cap_info);
	if (ret) {
		LOG_ERR("Failed to get unicast group info: %d", ret);
		return ret;
	}

	ret = bt_bap_unicast_group_get_info(cap_info.unicast_group, bap_info);
	if (ret) {
		LOG_ERR("Failed to get BAP unicast group info: %d", ret);
		return ret;
	}

	return 0;
}

static int group_info_print(const struct bt_cap_unicast_group *cap_unicast_group)
{
	struct bt_bap_unicast_group_info bap_info;
	int ret;

	ret = group_info_get(cap_unicast_group, &bap_info);
	if (ret != 0) {
		LOG_ERR("Failed to get unicast group info: %d", ret);
		return ret;
	}

	LOG_INF("Unicast group info");
	if (bap_info.sink_pd == BT_BAP_PD_UNSET) {
		LOG_INF("\tSink PD:\t Unset");
	} else {
		LOG_INF("\tSink PD:\t %d", bap_info.sink_pd);
	}

	LOG_INF("\tC->P interval:\t %d", bap_info.c_to_p_interval);
	LOG_INF("\tC->P latency:\t %d", bap_info.c_to_p_latency);
	if (bap_info.source_pd == BT_BAP_PD_UNSET) {
		LOG_INF("\tSource PD:\t Unset");
	} else {
		LOG_INF("\tSource PD:\t %d", bap_info.source_pd);
	}
	LOG_INF("\tP->C interval:\t %d", bap_info.p_to_c_interval);
	LOG_INF("\tP->C latency:\t %d", bap_info.p_to_c_latency);

	return 0;
}

struct group_streams_populate_data {
	uint8_t sink_iterator;
	uint8_t source_iterator;
	struct bt_cap_unicast_group_stream_param
		sink_stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
	struct bt_cap_unicast_group_stream_param
		source_stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
};

static bool unicast_group_populate(struct server_store *server, void *user_data)
{

	LOG_WRN("Populating unicast group for server %s", server->name);

	struct group_streams_populate_data *data = (struct group_streams_populate_data *)user_data;

	if (server->snk.num_eps == 0 && server->src.num_eps == 0) {
		LOG_WRN("Server %s has no valid sink or source EPs, skipping", server->name);
		return true;
	}

	/* Add only the streams that has a valid preset set */
	for (int j = 0; j < MIN(server->snk.num_eps, POPCOUNT_ZERO(server->snk.locations)); j++) {
		if (server->snk.lc3_preset[j].qos.pd == 0) {
			LOG_WRN("Sink EP %d has no valid preset, skipping", j);
			return true;
		}

		data->sink_stream_params[data->sink_iterator].qos_cfg =
			&server->snk.lc3_preset[j].qos;
		data->sink_stream_params[data->sink_iterator].stream = &server->snk.cap_streams[j];
		data->sink_iterator++;
	}

	/* Add only the streams that has a valid preset set */
	for (int j = 0; j < MIN(server->src.num_eps, POPCOUNT_ZERO(server->src.locations)); j++) {
		if (server->src.lc3_preset[j].qos.pd == 0) {
			LOG_WRN("Source EP %d has no valid preset, skipping", j);
			return true;
		}

		data->source_stream_params[data->source_iterator].qos_cfg =
			&server->src.lc3_preset[j].qos;
		data->source_stream_params[data->source_iterator].stream =
			&server->src.cap_streams[j];
		data->source_iterator++;
	}

	return true;
}

/**
 * @brief	Create a unicast group with all connected servers and their valid EPs.
 *		Pair sink/source streams from the same server together to make sure they are
 *		part of the same CIS.
 */
static void unicast_group_create(void)
{
	int ret;

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	/* Find out how many valid sink EPs we have */
	struct num_eps_total num_eps = {0, 0};

	ret = srv_store_foreach_server(num_eps_count, &num_eps);
	if (ret < 0) {
		LOG_ERR("Failed to count valid EPs: %d", ret);
		srv_store_unlock();
		return;
	}

	if (num_eps.sink == 0 && num_eps.source == 0) {
		LOG_ERR("No valid sink or source EPs found, cannot create unicast "
			"group");
		srv_store_unlock();
		return;
	}

	/* Populate the stream params arrays */
	(void)memset(&data, 0, sizeof(data));
	(void)memset(&group_param, 0, sizeof(group_param));
	(void)memset(&pair_params, 0, sizeof(pair_params));

	ret = srv_store_foreach_server(unicast_group_populate, &data);
	if (ret < 0) {
		LOG_ERR("Failed to populate unicast group stream params: %d", ret);
		srv_store_unlock();
		return;
	}

	int stream_iterator = 0;

	LOG_WRN("sink iterator is %d", data.sink_iterator);

	/* Pair TX and RX from same server.
	 * We pair in the order of sink to source because the sink stream will
	 * always be created before the source stream
	 */
	for (int i = 0; i < data.sink_iterator; i++) {
		pair_params[i].tx_param = &data.sink_stream_params[i];

		/* Search streams for a matching source */
		bool source_found = false;
		struct server_store *sink_server = NULL;

		ret = srv_store_from_stream_get(&data.sink_stream_params[i].stream->bap_stream,
						&sink_server);
		if (ret < 0) {
			LOG_ERR("Failed to get server from sink stream %p: %d",
				&data.sink_stream_params[i].stream->bap_stream, ret);
			srv_store_unlock();
			return;
		}

		for (int j = stream_iterator; j < data.source_iterator; j++) {
			/* Check if the source stream belongs to the same device */
			struct server_store *source_server = NULL;

			ret = srv_store_from_stream_get(
				&data.source_stream_params[j].stream->bap_stream, &source_server);
			if (ret < 0) {
				LOG_ERR("Failed to get server from source stream %p: %d",
					&data.source_stream_params[j].stream->bap_stream, ret);
				srv_store_unlock();
				return;
			}

			if (sink_server == source_server) {
				pair_params[i].rx_param = &data.source_stream_params[j];
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
	for (int i = 0; i < data.source_iterator; i++) {
		/* Check if the source has already been added */
		bool source_already_added = false;

		for (int j = 0; j < stream_iterator; j++) {
			if (pair_params[j].rx_param == &data.source_stream_params[i]) {
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
		pair_params[stream_iterator].rx_param = &data.source_stream_params[i];
		stream_iterator++;
	}

	group_param.params = pair_params;
	group_param.params_count = stream_iterator;

	if (group_param.params_count == 0) {
		LOG_ERR("No valid streams to create a unicast group");
		srv_store_unlock();
		return;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_PACKING_INTERLEAVED)) {
		group_param.packing = BT_ISO_PACKING_INTERLEAVED;
	} else {
		group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	}

	ret = bt_cap_unicast_group_create(&group_param, &unicast_group);
	if (ret != 0) {
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
 * @param[in] stream	Stream to check.
 * @param[in] user_data	User data, in this case a pointer to the server_store to
 *			check against.
 *
 * @retval		False	The stream is in the group.
 * @retval		True	The stream is not already in the group. (stop iterating)
 */
static bool stream_in_group_check(struct bt_cap_stream *stream, void *user_data)
{
	struct bt_cap_stream *server_stream = (struct bt_cap_stream *)user_data;

	if (stream == server_stream) {
		/* Found the stream in the group, stop iterating */
		return false;
	}

	return true;
}

/**
 * @brief	Function to check if a server has any streams in the unicast group.
 *
 * @param[in] server	Server to check.
 * @param[in] user_data	Unused.
 *
 * @retval		True	All streams from the server are in the group.
 * @retval		False	At least one stream is missing from the group.
 */
static bool server_stream_in_unicast_group_check(struct server_store *server, void *user_data)
{
	int ret;

	ARG_UNUSED(user_data);

	/* Check that the server is connected */
	struct bt_conn_info info;

	ret = bt_conn_get_info(server->conn, &info);
	if (ret != 0) {
		LOG_ERR("Failed to get connection info for conn: %p", server->conn);
		return true;
	}

	if (info.state != BT_CONN_STATE_CONNECTED) {
		LOG_DBG("Connection %p is not connected, skipping", server->conn);
		return true;
	}

	/* Check if the server has at least one valid preset set */
	if (server->snk.lc3_preset[0].qos.pd == 0 && server->src.lc3_preset[0].qos.pd == 0) {
		LOG_DBG("Server %s has no valid preset, skipping", server->name);
		return true;
	}

	/* Check each of the streams in the unicast_group against all of the
	 * streams in the server
	 */
	for (int i = 0; i < MIN(server->snk.num_eps, POPCOUNT_ZERO(server->snk.locations)); i++) {
		ret = bt_cap_unicast_group_foreach_stream(unicast_group, stream_in_group_check,
							  &server->snk.cap_streams[i]);
		if (ret == 0) {
			LOG_INF("Server %s sink stream %d (%p) not found in unicast group",
				server->name, i, &server->snk.cap_streams[i]);
			/* A stream is missing from the group, stop iterating */
			return false;
		}
	}

	return true;
}

static bool server_is_not_waiting_for_disc_check(struct server_store *server, void *user_data)
{
	ARG_UNUSED(user_data);

	if (server->snk.waiting_for_disc || server->src.waiting_for_disc) {
		LOG_DBG("Server %s is still waiting for discovery to complete", server->name);
		/* Stop iterating */
		return false;
	}

	/* Continue iterating */
	return true;
}

/**
 * @brief	Worker to start unicast streams. If a unicast group doesn't exist, it will
 *		be created. If it does exist, it will check if there is room for more streams, and
 *		if so, check if there are any connected servers that are not part of the group that
 *		can be added. If the group is full, or there are no more servers to add, it will
 *		start the streams in the unicast group.
 */
static void cap_start_worker(void)
{
	int ret;

	/* Create a unicast group if it doesn't already exist */
	if (unicast_group_created == false) {
		LOG_DBG("Unicast group not created, creating unicast group");
		unicast_group_create();
		goto start_streams;
	}

	struct bt_cap_unicast_group_info info;
	uint8_t group_length = 0;

	ret = bt_cap_unicast_group_get_info(unicast_group, &info);
	if (ret != 0) {
		LOG_ERR("Failed to get unicast group info: %d", ret);
		return;
	}

	group_length = sys_slist_len(&info.unicast_group->streams);
	if (group_length >= CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT) {
		/* The group is as full as it can get, start the relevant streams */
		LOG_DBG("Unicast group is full, cannot add more streams");
		goto start_streams;
	}

	/* Group is created, but there is still room for more devices, check if
	 * there are any waiting to join.
	 */

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	// If any servers are still doing discovery, return as this will be triggered again
	ret = srv_store_foreach_server(server_is_not_waiting_for_disc_check, NULL);

	if (ret == -ECANCELED) {
		/* If any of the servers are still waiting for discovery to complete, we should not
		 * start the streams yet
		 */
		LOG_WRN("Cancelling start due to discovery in progress");
		srv_store_unlock();
		return;
	}

	/* Check if each of the connected servers in srv_store are in the unicast_group */
	ret = srv_store_foreach_server(server_stream_in_unicast_group_check, NULL);
	srv_store_unlock();

	if (ret == -ECANCELED) {
		/* A new group will be created after the released_cb has been called */
		ret = unicast_client_stop(0);
		if (ret == -EAGAIN) {
			unicast_group_create();
			goto start_streams;
		}

		unicast_group_created = false;

		return;
	}

start_streams:

	ret = unicast_client_start(0);
	if (ret < 0) {
		LOG_ERR("Failed to start unicast client: %d", ret);
		return;
	}
}

/* bt_bap_unicast_client_cb begin ----------------------------------------------------------------*/

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
 * @param[in]	conn	Connection pointer to the unicast_server where the location was discovered.
 * @param[in]	dir	Direction of the location (sink or source).
 * @param[in]	loc	Discovered location.
 */
static void bap_location_cb(struct bt_conn *conn, enum bt_audio_dir dir, enum bt_audio_location loc)
{
	int ret;
	struct server_store *server = NULL;

	LOG_DBG("CB BAP location discovered for conn %p dir %d loc %d", conn, dir, loc);

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	ret = srv_store_from_conn_get(conn, &server);
	if (ret != 0) {
		LOG_ERR("%s: Unknown connection, should not reach here", __func__);
		srv_store_unlock();
		return;
	}

	if (loc & BT_AUDIO_LOCATION_FRONT_LEFT && loc & BT_AUDIO_LOCATION_FRONT_RIGHT &&
	    dir == BT_AUDIO_DIR_SINK) {
		LOG_INF("Both front left and right channel locations are set, stereo device found");

		ret = srv_store_location_set(
			conn, dir, BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT);
		if (ret != 0) {
			LOG_ERR("Failed to set location for conn %p, dir %d, loc %d: %d", conn, dir,
				loc, ret);
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
		if (ret != 0) {
			LOG_ERR("Failed to set location for conn %p, dir %d, loc %d: %d", conn, dir,
				loc, ret);
			srv_store_unlock();
			return;
		}

		server->name = "LEFT";

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
		if (ret != 0) {
			LOG_ERR("Failed to set location for conn %p, dir %d, loc %d: %d", conn, dir,
				loc, ret);
			srv_store_unlock();
			return;
		}

		server->name = "RIGHT";

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
 * @param[in]	conn	Connection pointer to the unicast_server where the available contexts
 *			was discovered.
 * @param[in]	snk_ctx	Available sink contexts.
 * @param[in]	src_ctx	Available source contexts.
 */
static void bap_available_contexts_cb(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				      enum bt_audio_context src_ctx)
{
	int ret;

	LOG_DBG("CB BAP available contexts for conn %p snk ctx %d src ctx %d", conn, snk_ctx,
		src_ctx);

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	ret = srv_store_avail_context_set(conn, snk_ctx, src_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to set available contexts for conn %p, snk ctx %d src ctx %d: %d",
			conn, snk_ctx, src_ctx, ret);
	}

	srv_store_unlock();
}

/**
 * @brief	Callback for when a codec capability is discovered. This function will store the
 *		codec capability in the server_store struct for the corresponding connection and
 *		direction.
 *
 * @param[in]	conn	Connection pointer to the unicast_server where the codec capability was
 *			discovered.
 * @param[in]	dir	Direction of the codec capability (sink or source).
 * @param[in]	codec	Pointer to the discovered codec capability.
 */
static void bap_pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			      const struct bt_audio_codec_cap *codec)
{
	int ret;

	LOG_DBG("CB BAP PAC record for conn %p, dir %d", conn, dir);

	if (codec->id != BT_HCI_CODING_FORMAT_LC3) {
		LOG_DBG("Only the LC3 codec is supported");
		return;
	}

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	ret = srv_store_codec_cap_set(conn, dir, codec);
	if (ret != 0) {
		LOG_ERR("Failed to set codec capability: %d", ret);
	}

	srv_store_unlock();
}

/**
 * @brief	Callback for when an endpoint is discovered. This function will store the endpoint
 *		in the server_store struct for the corresponding connection and direction.
 *
 * @param[in]	conn	Connection pointer to the unicast_server where the endpoint was discovered.
 * @param[in]	dir	Direction of the endpoint (sink or source).
 * @param[in]	ep	Pointer to the discovered endpoint. NULL if the discovery is complete.
 */
static void bap_endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	int ret;

	LOG_DBG("CB BAP endpoint discovery for conn %p, dir %d", conn, dir);

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret != 0) {
		LOG_ERR("%s: Unknown connection, should not reach here", __func__);
		srv_store_unlock();
		return;
	}

	if (ep == NULL) {
		if (dir == BT_AUDIO_DIR_SINK && server->snk.eps[0] == NULL) {
			LOG_WRN("No sink endpoints found");
		}

		if (dir == BT_AUDIO_DIR_SOURCE && server->src.eps[0] == NULL) {
			LOG_WRN("No source endpoints found");
		}

		srv_store_unlock();
		return;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		if (server->snk.num_eps >= ARRAY_SIZE(server->snk.eps)) {
			LOG_WRN("No more space (%d) for sink endpoints, increase "
				"CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT (%d)",
				server->snk.num_eps, ARRAY_SIZE(server->snk.eps));
			srv_store_unlock();
			return;
		}

		server->snk.eps[server->snk.num_eps] = ep;
		server->snk.num_eps++;

	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		if (server->src.num_eps >= ARRAY_SIZE(server->src.eps)) {
			LOG_WRN("No more space for source endpoints, increase "
				"CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT");
			srv_store_unlock();
			return;
		}

		server->src.eps[server->src.num_eps] = ep;
		server->src.num_eps++;

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
 * @param[in]	conn	Connection pointer to the unicast_server where the discovery was done.
 * @param[in]	err	Error code, 0 if successful.
 * @param[in]	server	Server pointer to the server_store struct where the discovered data will be
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
	if (ret != 0) {
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
	if (POPCOUNT_ZERO(server->snk.locations) == 1 && server->snk.num_eps >= 1) {
		ret = bt_audio_codec_cfg_set_val(
			&server->snk.lc3_preset[0].codec_cfg, BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
			(uint8_t *)&server->snk.locations, sizeof(server->snk.locations));
		if (ret < 0) {
			LOG_ERR("Failed to set codec channel allocation: %d", ret);
			return;
		}

	} else if (POPCOUNT_ZERO(server->snk.locations) == 2 && server->snk.num_eps >= 2 &&
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
		LOG_WRN("Number of sink locations: %d, number of sink endpoints: %d",
			POPCOUNT_ZERO(server->snk.locations), server->snk.num_eps);

		le_audio_event_publish(LE_AUDIO_EVT_NO_VALID_CFG, conn, NULL, BT_AUDIO_DIR_SINK);

		return;
	}
}

/**
 * @brief	Callback when source discovery has finished. This function will also set the codec
 *		configuration to be used in the preset(s) based on the discovered codec capabilities
 *		and channel allocation.
 *
 * @param[in]	conn	Connection pointer to the unicast_server where the discovery was done.
 * @param[in]	err	Error code, 0 if successful.
 * @param[in]	server	Server pointer to the server_store struct where the discovered data will be
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

static void bap_discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	int ret;

	if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		/* Will happen if e.g. the server does not have a source */
		LOG_DBG("CB BAP discover attribute not found for conn: %p", conn);
	} else if (err != 0) {
		LOG_ERR("CB BAP discover for conn %p, dir %d, err %d", conn, dir, err);
	} else {
		LOG_DBG("CB BAP discover for conn %p, dir %d", conn, dir);
	}

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret != 0) {
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
		if (ret != 0) {
			LOG_WRN("Failed to start source discovery: %d", ret);
		}

		srv_store_unlock();
		return;
	}

	ret = srv_store_foreach_server(server_is_not_waiting_for_disc_check, NULL);

	if (ret == -ECANCELED) {
		/* If any of the servers are still waiting for discovery to complete, we should not
		 * start the streams yet
		 */
		srv_store_unlock();
		return;
	}

	le_audio_event_publish(LE_AUDIO_EVT_DISCOVERY_COMPLETE, conn, NULL, dir);

	srv_store_unlock();

	if (!in_playing_state) {
		/* Since we are not in a playing state we return before starting the new streams */
		return;
	}

	LOG_INF("Submitting work to start CAP");

	/* Instruct the CAP state machine to perform a CAP start.
	If the state machine is idle, we can start. If not, we need to cancel first */

	if (k_sem_count_get(&cap_state_machine_sem) > 0) {

		cap_thread_next_evt_set(CAP_ACTION_START);
	} else {
		ret = bt_cap_initiator_unicast_audio_cancel();
		if (ret != 0) {
			LOG_ERR("Failed to cancel ongoing CAP start: %d", ret);
		}

		cap_thread_next_evt_set(CAP_ACTION_START);
	}
}

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.location = bap_location_cb,
	.available_contexts = bap_available_contexts_cb,
	.pac_record = bap_pac_record_cb,
	.endpoint = bap_endpoint_cb,
	.discover = bap_discover_cb,
};

/* bt_bap_unicast_client_cb end ------------------------------------------------------------------*/

/* bt_bap_stream_ops begin -----------------------------------------------------------------------*/

struct qos_settings_by_dir {
	uint32_t pres_dly_us;
	uint32_t transport_latency_ms;
	uint8_t framing;
};

struct qos_settings_write {
	struct qos_settings_by_dir snk;
	struct qos_settings_by_dir src;
};

/* Set common parameters for all existing streams */
static bool common_params_existing_streams_set(struct bt_cap_stream *stream, void *user_data)
{
	int ret;
	struct qos_settings_write *qos_write = (struct qos_settings_write *)user_data;

	struct server_store *server = NULL;
	enum bt_audio_dir dir;

	ret = le_audio_stream_dir_get(&stream->bap_stream);
	if (ret < 0) {
		/* This stream is not yet configured */
		LOG_DBG("Failed to get dir of stream %p", (void *)&stream->bap_stream);
		return true;
	}

	dir = (enum bt_audio_dir)ret;

	ret = srv_store_from_stream_get(&stream->bap_stream, &server);
	if (ret) {
		LOG_ERR("Srv store from stream get failed: %d", ret);
		return false;
	}

	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		LOG_DBG("Setting common QoS params for existing sink stream %p, PD %d, framing "
			"%d, latency %d",
			(void *)&stream->bap_stream, qos_write->snk.pres_dly_us,
			qos_write->snk.framing, qos_write->snk.transport_latency_ms);

		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT; i++) {
			server->snk.lc3_preset[i].qos.pd = qos_write->snk.pres_dly_us;
			// server->snk.lc3_preset[i].qos.framing = qos_write->snk.framing;
			// server->snk.lc3_preset[i].qos.latency =
			// qos_write->snk.transport_latency_ms;
		}
		break;
	case BT_AUDIO_DIR_SOURCE:
		LOG_DBG("Setting common QoS params for existing source stream %p, PD %d, framing "
			"%d, latency %d",
			&stream->bap_stream, qos_write->src.pres_dly_us, qos_write->src.framing,
			qos_write->src.transport_latency_ms);

		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT; i++) {
			server->src.lc3_preset[i].qos.pd = qos_write->src.pres_dly_us;
			// server->src.lc3_preset[i].qos.framing = qos_write->src.framing;
			// server->src.lc3_preset[i].qos.latency =
			// qos_write->src.transport_latency_ms;
		}
		break;
	default:
		LOG_ERR("Unknown direction: %d", dir);
		break;
	}

	return true;
}

static void bap_stream_codec_configured_cb(struct bt_bap_stream *stream,
					   const struct bt_bap_qos_cfg_pref *server_pref)
{
	LOG_DBG("CB BAP stream %p codec configured", stream);

	// TODO: Optimize, get this once for all streams in each dir
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return;
	}

	le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED, stream->conn, stream, dir);
}

static void bap_stream_qos_configured_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("CB BAP stream %p QoS configured", stream);
}

static void bap_stream_enabled_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("CB stream %p enabled", stream);
}

static void bap_stream_metadata_updated_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("CB BAP stream %p metadata updated", stream);
}

static void bap_stream_disabled_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("CB BAP stream %p disabled", stream);
}

static void bap_stream_released_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("CB BAP stream %p released", stream);
}

static void bap_stream_started_cb(struct bt_bap_stream *stream)
{
	int ret;
	enum bt_audio_dir dir;

	LOG_DBG("CB BAP stream %p started", stream);

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return;
	}

	struct stream_index idx = {0};

	ret = stream_idx_get(stream, &idx);
	if (ret != 0) {
		LOG_ERR("Failed to get stream index: %d", ret);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_TX)) {
		ERR_CHK(bt_le_audio_tx_stream_started(bt_le_audio_tx, idx));
	}

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Stream %p started, idx: %d %d %d", stream, idx.lvl1, idx.lvl2, idx.lvl3);

	le_audio_event_publish(LE_AUDIO_EVT_STREAMING, stream->conn, stream, dir);
}

static void bap_stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int ret;

	LOG_DBG("CB BAP stream %p stopped. Reason 0x%02X", stream, reason);

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Stream %p stopped. Reason %d", stream, reason);

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
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
		LOG_ERR("Failed to get dir of stream %p", stream);
	}

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn, stream, dir);
}

#if (CONFIG_BT_AUDIO_RX)
static void bap_stream_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			       struct net_buf *audio_frame)
{
	int ret;
	struct audio_metadata meta;

	if (receive_cb == NULL) {
		LOG_ERR("The RX callback has not been set");
		return;
	}

	ret = le_audio_metadata_populate(&meta, stream, info, audio_frame);
	if (ret != 0) {
		LOG_ERR("Failed to populate meta data: %d", ret);
		return;
	}

	struct stream_index idx;

	ret = stream_idx_get(stream, &idx);
	if (ret != 0) {
		LOG_ERR("Failed to get stream index: %d", ret);
		return;
	}

	receive_cb(audio_frame, &meta, idx.lvl3);
}
#endif /* (CONFIG_BT_AUDIO_RX) */

#if (CONFIG_BT_AUDIO_TX)
static void bap_stream_sent_cb(struct bt_bap_stream *stream)
{
	int ret;
	struct stream_index idx;
	uint8_t state;

	ret = le_audio_ep_state_get(stream->ep, &state);
	if (ret != 0) {
		LOG_ERR("Failed to get endpoint state: %d", ret);
		return;
	}

	if (state == BT_BAP_EP_STATE_STREAMING) {
		ret = stream_idx_get(stream, &idx);
		if (ret != 0) {
			LOG_ERR("Failed to get stream index: %d", ret);
			return;
		}
		ERR_CHK(bt_le_audio_tx_stream_sent(bt_le_audio_tx, idx));
	} else {
		LOG_WRN("Not in streaming state: %d", state);
	}
}
#endif /* CONFIG_BT_AUDIO_TX */

static void bap_stream_connected_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("CB BAP stream %p connected", stream);
}

static void bap_stream_disconnected_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	LOG_DBG("CB BAP stream %p disconnected. Reason 0x%02X", stream, reason);
}

static struct bt_bap_stream_ops stream_ops = {
	.configured = bap_stream_codec_configured_cb,
	.qos_set = bap_stream_qos_configured_cb,
	.enabled = bap_stream_enabled_cb,
	.metadata_updated = bap_stream_metadata_updated_cb,
	.disabled = bap_stream_disabled_cb,
	.released = bap_stream_released_cb,
	.started = bap_stream_started_cb,
	.stopped = bap_stream_stopped_cb,
#if (CONFIG_BT_AUDIO_RX)
	.recv = bap_stream_recv_cb,
#endif /* (CONFIG_BT_AUDIO_RX) */
#if (CONFIG_BT_AUDIO_TX)
	.sent = bap_stream_sent_cb,
#endif /* (CONFIG_BT_AUDIO_TX) */
	.connected = bap_stream_connected_cb,
	.disconnected = bap_stream_disconnected_cb,
};

/* bt_bap_stream_ops end -------------------------------------------------------------------------*/

/* bt_cap_initiator_cb begin ---------------------------------------------------------------------*/

static void cap_discovery_complete_cb(struct bt_conn *conn, int err,
				      const struct bt_csip_set_coordinator_set_member *member,
				      const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	int ret;
	struct le_audio_msg msg;
	struct server_store *server = NULL;

	if (err != 0) {
		LOG_ERR("CB CAP discovery complete for conn: %p, err: %d", conn, err);
	} else {
		LOG_DBG("CB CAP discovery complete for conn: %p", conn);
	}

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	ret = srv_store_from_conn_get(conn, &server);
	if (ret != 0) {
		LOG_ERR("%s: Unknown connection, should not reach here", __func__);
		srv_store_unlock();
		return;
	}

	if (err) {
		LOG_WRN("Got err: %d from conn: %p", err, conn);
		msg.set_size = 0;
		msg.sirk = NULL;
	} else if (csis_inst == NULL) {
		LOG_WRN("csis_inst is NULL from conn: %p", conn);
		msg.set_size = 0;
		msg.sirk = NULL;
	} else {
		LOG_DBG("\tErr: %d, set_size: %d", err, csis_inst->info.set_size);
		LOG_HEXDUMP_DBG(csis_inst->info.sirk, BT_CSIP_SIRK_SIZE, "\tSIRK:");

		server->member = member;
		msg.set_size = csis_inst->info.set_size;
		msg.sirk = csis_inst->info.sirk;
	}

	msg.event = LE_AUDIO_EVT_COORD_SET_DISCOVERED;
	msg.conn = conn;

	ret = zbus_chan_pub(&le_audio_chan, &msg, LE_AUDIO_ZBUS_EVENT_WAIT_TIME);
	ERR_CHK(ret);

	srv_store_unlock();
}

static void cap_start_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		LOG_ERR("CB CAP start complete for conn: %p, err: %d", conn, err);
		return;
	} else {
		LOG_DBG("CB CAP start complete for conn: %p", conn);
	}

	// TODO: Need to look at WD issue, and check that we do not end up in a deadlock situation.

	in_playing_state = true;
	cap_thread_cap_action_complete(err);
}

static void cap_start_codec_configured_cb(void)
{
	LOG_INF("CB CAP codec configured");

	int ret;
	uint32_t new_pres_dly_snk_us = BT_BAP_PD_UNSET;
	uint32_t new_pres_dly_src_us = BT_BAP_PD_UNSET;
	bool group_reconfigure_needed_due_to_src_pd = false;
	bool group_reconfigure_needed_due_to_snk_pd = false;
	bool group_reconfigure_needed = false;

	/* The group is always created at the start, so we must have group here */
	__ASSERT(unicast_group != NULL, "Unicast group is NULL");

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	/* All streams have been codec configured. Hence, we need to check
	both directions here.*/

	struct bt_bap_unicast_group_info bap_info;

	ret = group_info_get(unicast_group, &bap_info);
	if (ret != 0) {
		LOG_ERR("Failed to get group info: %d", ret);
		srv_store_unlock();
		return;
	}

	ret = srv_store_pres_dly_by_dir_find(BT_AUDIO_DIR_SINK, &new_pres_dly_snk_us,
					     &group_reconfigure_needed_due_to_snk_pd,
					     unicast_group);
	if (ret) {
		LOG_ERR("Failed to find presentation delay for sink direction: %d", ret);
		srv_store_unlock();
		return;
	}

	ret = srv_store_pres_dly_by_dir_find(BT_AUDIO_DIR_SOURCE, &new_pres_dly_src_us,
					     &group_reconfigure_needed_due_to_src_pd,
					     unicast_group);
	if (ret) {
		LOG_ERR("Failed to find presentation delay for source direction: %d", ret);
		srv_store_unlock();
		return;
	}

	group_reconfigure_needed =
		group_reconfigure_needed_due_to_snk_pd || group_reconfigure_needed_due_to_src_pd;

	if (group_reconfigure_needed) {
		struct qos_settings_write qos_write;
		qos_write.snk.pres_dly_us = new_pres_dly_snk_us;
		qos_write.src.pres_dly_us = new_pres_dly_src_us;

		/* Need to update all streams with the new presentation delay in this
		   direction */
		ret = bt_cap_unicast_group_foreach_stream(
			unicast_group, common_params_existing_streams_set, (void *)&qos_write);
		if (ret) {
			LOG_ERR("Failed to update presentation delay for unicast group: %d", ret);
			srv_store_unlock();
			return;
		}

		/* If the group has been connected we need to cancel and restart*/
		if (bap_info.has_been_connected) {
			/* Create the unicast group anew. This will happen when all streams have
			 * been released */
			unicast_group_created = false;
			srv_store_unlock();

			LOG_INF("calling cancel in the CAP");
			ret = bt_cap_initiator_unicast_audio_cancel();
			if (ret != 0 && ret != -EALREADY) {
				LOG_DBG("Audio cancel EALREADY");
			} else if (ret != 0) {
				LOG_ERR("Failed to cancel unicast audio: %d", ret);
			}

			cap_thread_next_evt_set(CAP_ACTION_STOP_THEN_START);

			// Signal reconfig

			// How do we abort the current CAP procedure? We need to stop the
			// current streams, and then recreate the group with the new
			// presentation delay. This will be done in the stream_released_cb
			// when all streams have been released.

			// TODO: THis publish shall not be done, as the group is not ready
			// yet. LOG_DBG("LE_AUDIO_EVT_CONFIG_RECEIVED publish reconfig");
			// le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED,
			// stream->conn, stream, dir);
			srv_store_unlock();
			return;
		} else {
			LOG_INF("Reconfiguring unicast group as it has not been connected yet");
			ret = bt_cap_unicast_group_reconfig(unicast_group, &group_param);
			if (ret) {
				LOG_ERR("Failed to reconfigure unicast group: %d", ret);
				srv_store_unlock();
				return;
			}
			srv_store_unlock();
			return;
		}

	} else {
		/* No action required, keep group settings as-is */
	}

	srv_store_unlock();

	ret = group_info_print(unicast_group);
	if (ret != 0) {
		LOG_ERR("Failed to print unicast group info: %d", ret);
	}
}

static void cap_start_qos_configured_cb(void)
{
	int ret;
	LOG_INF("CB CAP QoS configured");

	ret = group_info_print(unicast_group);
	if (ret != 0) {
		LOG_ERR("Failed to print unicast group info: %d", ret);
	}
}

static void cap_update_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		LOG_ERR("CB CAP update complete for conn: %p, err: %d", conn, err);
		return;
	} else {
		LOG_DBG("CB CAP update complete for conn: %p", conn);
	}
}

static void cap_stop_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		LOG_ERR("CB CAP stop complete for conn: %p, err: %d", conn, err);
		return;
	} else {
		LOG_DBG("CB CAP stop complete for conn: %p", conn);
	}

	in_playing_state = false;
}

static void cap_stop_released_cb(void)
{
	int ret;

	LOG_WRN("CB CAP stop released");
	/* Check if unicast_group_recreate has been requested.
	 * If so, delete the group and submit work to recreate it.
	 */
	if (unicast_group_created) {
		return;
	}

	ret = bt_cap_unicast_group_delete(unicast_group);
	if (ret != 0) {
		LOG_ERR("Failed to delete unicast group: %d", ret);
	}

	cap_thread_cap_action_complete(0);
}

static struct bt_cap_initiator_cb cap_cbs = {
	.unicast_discovery_complete = cap_discovery_complete_cb,
	.unicast_start_complete = cap_start_complete_cb,
	.unicast_start_codec_configured = cap_start_codec_configured_cb,
	.unicast_start_qos_configured = cap_start_qos_configured_cb,
	.unicast_update_complete = cap_update_complete_cb,
	.unicast_stop_complete = cap_stop_complete_cb,
	.unicast_stop_released = cap_stop_released_cb,
};

/* bt_cap_initiator_cb end -----------------------------------------------------------------------*/

static bool first_source_location_get(struct bt_cap_stream *stream, void *user_data)
{
	int ret;
	enum bt_audio_dir dir;
	struct stream_index idx;

	if (stream == NULL || user_data == NULL) {
		LOG_ERR("Invalid parameters");
		return true;
	}

	enum bt_audio_location *locations = (enum bt_audio_location *)user_data;

	dir = le_audio_stream_dir_get(&stream->bap_stream);

	ret = stream_idx_get(&stream->bap_stream, &idx);
	if (ret != 0) {
		LOG_ERR("Failed to get stream index: %d", ret);
		return ret;
	}

	if ((dir != BT_AUDIO_DIR_SOURCE) || (idx.lvl1 != 0) || (idx.lvl2 != 0) || (idx.lvl3 != 0)) {
		/* Not the first source stream, continue searching */
		return true;
	}

	ret = bt_audio_codec_cfg_get_chan_allocation(stream->bap_stream.codec_cfg, locations,
						     false);
	if (ret != 0) {
		LOG_WRN("Failed to get channel allocation");
		return ret;
	}

	/* Found the first source stream, stop iterating */
	return false;
}

int le_audio_concurrent_sync_num_get(uint8_t *num_streams, enum bt_audio_location *locations)
{
	int ret;

	if (num_streams == NULL || locations == NULL) {
		return -EINVAL;
	}

	/* Only one stream supported at the moment */
	*num_streams = 1;
	/* Get location of source stream with idx 0.0.0 */
	ret = bt_cap_unicast_group_foreach_stream(unicast_group, first_source_location_get,
						  locations);
	if (ret != -ECANCELED) {
		LOG_ERR("Failed to get source location: %d", ret);
		return ret;
	}

	return 0;
}

int unicast_client_is_streaming(bool *is_streaming)
{
	int ret;
	int snk_count;
	int src_count;

	if (is_streaming == NULL) {
		return -EINVAL;
	}

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return -EINVAL;
	}

	snk_count = srv_store_all_ep_state_count(BT_BAP_EP_STATE_STREAMING, BT_AUDIO_DIR_SINK);
	src_count = srv_store_all_ep_state_count(BT_BAP_EP_STATE_STREAMING, BT_AUDIO_DIR_SOURCE);

	srv_store_unlock();

	if (snk_count < 0 || src_count < 0) {
		LOG_ERR("Failed to get streaming count: snk %d, src %d", snk_count, src_count);
		return -EACCES;
	}

	*is_streaming = (snk_count > 0 || src_count > 0);

	return 0;
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
		if (ret != 0) {
			LOG_ERR("Invalid sampling frequency: %d", ret);
			return -ENXIO;
		}
	}

	if (bitrate != NULL) {
		ret = le_audio_bitrate_get(stream->codec_cfg, bitrate);
		if (ret != 0) {
			LOG_ERR("Unable to calculate bitrate: %d", ret);
			return -ENXIO;
		}
	}

	return 0;
}

/* Get the supported sink locations from all connected unicast servers, called once per server */
static bool sink_locations_get(struct server_store *server, void *user_data)
{
	uint32_t *locations = (uint32_t *)user_data;

	*locations |= server->snk.locations;

	return true;
}

/* Get the supported source locations from all connected unicast servers, called once per server */
static bool source_locations_get(struct server_store *server, void *user_data)
{
	uint32_t *locations = (uint32_t *)user_data;

	*locations |= server->src.locations;

	return true;
}

int unicast_client_locations_get(uint32_t *locations, enum bt_audio_dir dir)
{
	int ret;

	if (locations == NULL) {
		return -EINVAL;
	}

	*locations = 0;

	if (dir != BT_AUDIO_DIR_SINK && dir != BT_AUDIO_DIR_SOURCE) {
		return -EINVAL;
	}

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return ret;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		ret = srv_store_foreach_server(sink_locations_get, locations);
		if (ret != 0) {
			LOG_ERR("Failed to get locations: %d", ret);
			srv_store_unlock();
			return ret;
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		ret = srv_store_foreach_server(source_locations_get, locations);
		if (ret != 0) {
			LOG_ERR("Failed to get locations: %d", ret);
			srv_store_unlock();
			return ret;
		}
	}

	srv_store_unlock();

	return 0;
}

void unicast_client_conn_disconnected(struct bt_conn *conn)
{
	int ret;

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return;
	}

	ret = srv_store_clear_by_conn(conn);
	if (ret != 0) {
		LOG_ERR("Failed to clear server store for conn %p: %d", conn, ret);
	}

	srv_store_unlock();
}

int unicast_client_discover(struct bt_conn *conn, enum unicast_discover_dir dir)
{
	int ret;

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return ret;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret != 0) {
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
	if (ret != 0) {
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
	if (ret != 0) {
		LOG_WRN("Failed to discover %d", ret);
		srv_store_unlock();
		return ret;
	}

	srv_store_unlock();

	return 0;
}

static bool is_connected(struct bt_conn const *const conn)
{
	int ret;

	/* Check if the connection is valid */
	struct bt_conn_info info;

	if (conn == NULL) {
		LOG_DBG("Connection is NULL, not connected");
		return false;
	}

	ret = bt_conn_get_info(conn, &info);
	if (ret != 0) {
		LOG_ERR("Failed to get connection info for conn %p: %d", conn, ret);
		return false;
	}

	if (info.state == BT_CONN_STATE_CONNECTED) {
		LOG_DBG("Connection %p is connected", conn);
		return true;
	}

	return false;
}

static bool add_to_start_params(struct server_store *server, void *user_data)
{
	int ret;
	struct bt_cap_unicast_audio_start_param *param = user_data;

	if (!is_connected(server->conn)) {
		LOG_DBG("Server %s is not connected, skipping", server->name);
		return true;
	}

	for (int j = 0; j < MIN(server->snk.num_eps, POPCOUNT_ZERO(server->snk.locations)); j++) {
		uint8_t state;

		ret = le_audio_ep_state_get(server->snk.eps[j], &state);
		if (state == BT_BAP_EP_STATE_STREAMING || ret) {
			LOG_DBG("Sink endpoint is already streaming, skipping start");
			continue;
		}

		param->stream_params[param->count].member.member = server->conn;
		param->stream_params[param->count].stream = &server->snk.cap_streams[j];
		param->stream_params[param->count].ep = server->snk.eps[j];
		param->stream_params[param->count].codec_cfg = &server->snk.lc3_preset[j].codec_cfg;
		param->count++;
	}

	/* Check if we have valid source endpoints to start */
	for (int j = 0; j < MIN(server->src.num_eps, POPCOUNT_ZERO(server->src.locations)); j++) {
		uint8_t state;

		ret = le_audio_ep_state_get(server->src.eps[j], &state);
		if (state == BT_BAP_EP_STATE_STREAMING || ret) {
			LOG_DBG("Source endpoint is already streaming, skipping start");
			continue;
		}

		param->stream_params[param->count].member.member = server->conn;
		param->stream_params[param->count].stream = &server->src.cap_streams[j];
		param->stream_params[param->count].ep = server->src.eps[j];
		param->stream_params[param->count].codec_cfg = &server->src.lc3_preset[j].codec_cfg;
		param->count++;
	}

	return true;
}

// This shall not be accessed from outside the unicast client module
int unicast_client_start(uint8_t cig_index)
{
	int ret;

	if (unicast_group == NULL) {
		LOG_INF("No unicast group to start");
		return -EIO;
	}

	/* Start all unicast_servers with valid endpoints */
	struct bt_cap_unicast_audio_start_stream_param
		cap_stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
				  CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];

	struct bt_cap_unicast_audio_start_param param;

	param.stream_params = cap_stream_params;
	param.count = 0;
	param.type = BT_CAP_SET_TYPE_AD_HOC;

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);

		return ret;
	}

	ret = srv_store_foreach_server(add_to_start_params, &param);
	if (ret != 0) {
		LOG_ERR("Failed to add streams to start params: %d", ret);

		srv_store_unlock();

		return ret;
	}

	if (param.count == 0) {
		LOG_DBG("No streams to start");

		srv_store_unlock();

		return -EIO;
	}

	ret = bt_cap_initiator_unicast_audio_start(&param);
	if (ret != 0) {
		LOG_ERR("Failed to start unicast sink audio: %d", ret);

		srv_store_unlock();

		return ret;
	}

	srv_store_unlock();

	return 0;
}

static bool add_to_stop_params(struct bt_cap_stream *stream, void *user_data)
{
	struct bt_cap_unicast_audio_stop_param *param = user_data;

	if (stream->bap_stream.ep == NULL) {
		/* Stream already released */
		return true;
	}

	param->streams[param->count++] = stream;

	return true;
}

static bool server_connected_check(struct bt_cap_stream *stream, void *user_data)
{
	int ret;
	struct server_store *server = NULL;
	bool *connected_server_found = (bool *)user_data;

	ret = srv_store_from_stream_get(&stream->bap_stream, &server);
	if (ret != 0) {
		LOG_ERR("Failed to get server from stream: %d", ret);
		return true;
	}

	if (server && is_connected(server->conn)) {
		*connected_server_found = true;
		/* Found a connected server, will stop iterating */
		return false;
	}

	return true;
}

int unicast_client_stop(uint8_t cig_index)
{
	int ret;
	struct bt_cap_stream *streams[(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT +
				       CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT) *
				      CONFIG_BT_MAX_CONN];
	static struct bt_cap_unicast_audio_stop_param param;

	if (cig_index >= CONFIG_BT_ISO_MAX_CIG) {
		LOG_ERR("Trying to stop CIG %d out of %d", cig_index, CONFIG_BT_ISO_MAX_CIG);
		return -EINVAL;
	}

	if (unicast_group == NULL) {
		LOG_WRN("No unicast group to stop");

		return -EIO;
	}

	param.streams = streams;
	param.count = 0;
	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.release = true;

	ret = bt_cap_unicast_group_foreach_stream(unicast_group, add_to_stop_params, &param);
	if (ret != 0) {
		LOG_ERR("Failed to add streams to stop params: %d", ret);

		return ret;
	}

	if (param.count == 0) {
		LOG_DBG("No streams to stop");

		/* No streams found. Check if devices are connected, if no, delete the group */
		bool connected_server_found = false;

		ret = srv_store_lock(LOCK_WAIT_TIME_MS);
		if (ret < 0) {
			LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
			return ret;
		}

		ret = bt_cap_unicast_group_foreach_stream(unicast_group, server_connected_check,
							  &connected_server_found);

		if (ret == -ECANCELED) {
			/* If cancelled a connected server has been found */
			srv_store_unlock();
			return ret;
		} else if (ret != 0) {
			LOG_ERR("Failed to check if servers are connected: %d", ret);
			srv_store_unlock();
			return ret;
		}

		srv_store_unlock();

		if (!connected_server_found) {
			LOG_DBG("No connected servers found, deleting unicast group");
			ret = bt_cap_unicast_group_delete(unicast_group);
			if (ret != 0) {
				LOG_ERR("Failed to delete unicast group: %d", ret);
			}

			unicast_group = NULL;
			unicast_group_created = false;

			return -EAGAIN;
		}

		return -EIO;
	}

	ret = bt_cap_initiator_unicast_audio_stop(&param);
	if (ret != 0) {
		LOG_ERR("Failed to stop unicast audio: %d", ret);

		return ret;
	}

	return 0;
}

struct unicast_send_info {
	struct le_audio_tx_info *tx;
	uint8_t num_active_streams;
};

static bool unicast_send_info_populate(struct server_store *server, void *user_data)
{
	int ret;
	struct unicast_send_info *info = (struct unicast_send_info *)user_data;

	for (int i = 0; i < server->snk.num_eps; i++) {
		/* Skip unicast_servers not in a streaming state */
		if (!le_audio_ep_state_check(server->snk.cap_streams[i].bap_stream.ep,
					     BT_BAP_EP_STATE_STREAMING)) {
			continue;
		}

		/* Set cap stream pointer */
		info->tx[info->num_active_streams].cap_stream = &server->snk.cap_streams[i];

		/* Set index */
		ret = stream_idx_get(&server->snk.cap_streams[i].bap_stream,
				     &info->tx[info->num_active_streams].idx);
		if (ret != 0) {
			LOG_ERR("Failed to get stream index: %d", ret);
			return false;
		}

		const uint8_t *loc;

		ret = bt_audio_codec_cfg_get_val(server->snk.cap_streams[i].bap_stream.codec_cfg,
						 BT_AUDIO_CODEC_CFG_CHAN_ALLOC, &loc);
		if (ret < 0) {
			LOG_ERR("Failed to get channel allocation: %d", ret);
			return false;
		}

		/* Set channel location */
		/* Both mono and left unicast_servers will receive left channel */
		info->tx[info->num_active_streams].audio_location =
			*loc == BT_AUDIO_LOCATION_FRONT_RIGHT ? AUDIO_CH_R : AUDIO_CH_L;

		info->num_active_streams++;
	}

	return true;
}

int unicast_client_send(struct net_buf const *const audio_frame, uint8_t cig_index)
{
#if (CONFIG_BT_AUDIO_TX)
	int ret;

	if (cig_index >= CONFIG_BT_ISO_MAX_CIG) {
		LOG_ERR("Trying to send to CIG %d out of %d", cig_index, CONFIG_BT_ISO_MAX_CIG);
		return -EINVAL;
	}

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return ret;
	}

	uint8_t num_streaming =
		srv_store_all_ep_state_count(BT_BAP_EP_STATE_STREAMING, BT_AUDIO_DIR_SINK);
	struct le_audio_tx_info tx[num_streaming];

	struct unicast_send_info info = {
		.tx = tx,
		.num_active_streams = 0,
	};

	/* Populate tx struct */
	ret = srv_store_foreach_server(unicast_send_info_populate, &info);
	if (ret != 0) {
		LOG_ERR("Failed to populate send info: %d", ret);
		srv_store_unlock();
		return ret;
	}

	if (info.num_active_streams == 0) {
		/* This could happen if we had a sink + source and just the sink disconnected */
		LOG_DBG("No active streams");
		srv_store_unlock();
		return -ECANCELED;
	}

	ret = bt_le_audio_tx_send(bt_le_audio_tx, audio_frame, info.tx, info.num_active_streams);
	if (ret != 0) {
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

enum cap_action_type cap_thread_next_event_get(void)
{
	LOG_INF("Waiting for next event");
	(void)k_poll(&poll_evt, 1, K_FOREVER);
	k_mutex_lock(&next_action_lock, K_FOREVER);
	enum cap_action_type action = next_action;
	k_mutex_unlock(&next_action_lock);
	return action;
}

/* CAP thread for the state machine. Must be a lower priority than
 * the system workqueue and bt_rx.
 */
static void cap_state_machine_thread(void *dummy1, void *dummy2, void *dummy3)
{

	while (1) {
		/* Must wait for previous CAP procedure to complete */
		enum cap_action_type action;
		LOG_INF("Waiting for CAP signal");

		/* State machine is IDLE */

		LOG_INF("Fetch next action");
		action = cap_thread_next_event_get();
		LOG_INF("Waiting for CAP state machine semaphore");
		k_sem_take(&cap_state_machine_sem, K_FOREVER);

		/* State machine is processing */
		k_poll_signal_reset(&poll_sig);
		if (atomic_test_and_clear_bit(cap_state_machine_delay_start, 0)) {
			LOG_INF("CAP delay start");
			k_sleep(K_MSEC(400));
		}

		switch (action) {
		case CAP_ACTION_STOP:

			break;
		case CAP_ACTION_STOP_THEN_START:
			// DO start

			break;
		case CAP_ACTION_START:
			cap_start_worker();
			break;
		default:
			LOG_ERR("Unknown CAP action: %d", action);
			break;
		}
	}
}

int unicast_client_enable(uint8_t cig_index, le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	ret = srv_store_lock(LOCK_WAIT_TIME_MS);
	if (ret < 0) {
		LOG_ERR("%s: Failed to lock server store: %d", __func__, ret);
		return ret;
	}

	ret = srv_store_init();
	if (ret != 0) {
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
	if (ret != 0) {
		LOG_ERR("Failed to register client callbacks: %d", ret);
		srv_store_unlock();
		return ret;
	}

	ret = bt_cap_initiator_register_cb(&cap_cbs);
	if (ret != 0) {
		LOG_ERR("Failed to register cap callbacks: %d", ret);
		srv_store_unlock();
		return ret;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_TX)) {
		/* A unicast client is the Bluetooth central device */
		ret = bt_le_audio_tx_init(bt_le_audio_tx, true);
		if (ret != 0) {
			LOG_ERR("Failed to initialize LE Audio TX: %d", ret);
			srv_store_unlock();
			return ret;
		}
	}

	cap_state_machine_thread_id = k_thread_create(
		&cap_state_machine_thread_data, cap_state_machine_thread_stack,
		CAP_STATE_MACHINE_STACK_SIZE, (k_thread_entry_t)cap_state_machine_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
	ret = k_thread_name_set(cap_state_machine_thread_id, "Cap_state_machine");
	if (ret) {
		LOG_ERR("Failed to create cap_state_machine thread");
		return ret;
	}

	initialized = true;

	srv_store_unlock();

	return 0;
}
