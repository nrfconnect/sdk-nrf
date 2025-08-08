/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/slist.h>
#include <zephyr/bluetooth/conn.h>

#include "server_store.h"
#include "min_heap.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(unicast_client, CONFIG_UNICAST_CLIENT_LOG_LEVEL);

/* This array keeps track of all the remote unicast servers this unicast client is operating on.
 **/
/* static struct server_store servers[CONFIG_BT_MAX_CONN]; */

static int min_heap_cmp(const void *a, const void *b)
{
	struct server_store *store_a = (struct server_store *)a;
	struct server_store *store_b = (struct server_store *)b;

	return &store_a->conn - &store_b->conn;
}

static bool min_heap_conn_eq(const void *node, const void *other)
{
	const struct server_store *store = (const struct server_store *)node;
	const struct bt_conn *conn = (const struct bt_conn *)other;

	return store->conn == conn;
}

MIN_HEAP_DEFINE_STATIC(server_heap, CONFIG_BT_MAX_CONN, sizeof(struct server_store), 4,
		       min_heap_cmp);

int srv_store_src_num_get(uint8_t cig_idx)
{
	return -EPERM;
}

int srv_store_snk_num_get(uint8_t cig_idx)
{
	return -EPERM;
}

int srv_store_pres_dly_find(uint8_t cig_idx, uint32_t *new_pres_dly_us)
{
	return -EPERM;
}

int srv_store_location_set(struct bt_conn *conn, enum bt_audio_dir dir, enum bt_audio_location loc)
{
	return -EPERM;
}

int srv_store_avail_context_set(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				enum bt_audio_context src_ctx)
{
	return -EPERM;
}

int srv_store_codec_cap_set(struct bt_conn *conn, enum bt_audio_dir dir,
			    const struct bt_audio_codec_cap *codec)
{
	return -EPERM;
}

int srv_store_ep_set(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	return -EPERM;
}

int srv_store_valid_codec_cap_check(struct bt_conn const *const conn, enum bt_audio_dir dir,
				    bool **array_of_valid_caps)
{
	return -EPERM;
}

int srv_store_stream_idx_get(struct bt_bap_stream const *const stream) /* May not be required */
{
	return -EPERM;
}

int srv_store_from_stream_get(struct bt_cap_stream const *const stream,
			      struct server_store **server)
{
	struct server_store *tmp_server = NULL;
	uint32_t matches = 0;

	*server = NULL;

	if (stream == NULL || server == NULL) {
		LOG_ERR("Invalid parameters: stream or server is NULL");
		return -EINVAL;
	}

	for (int srv_idx = 0; srv_idx < server_heap.size; srv_idx++) {
		tmp_server = (struct server_store *)min_heap_get_element(&server_heap, srv_idx);
		if (server == NULL) {
			LOG_ERR("Server is NULL at index %d", srv_idx);
			return -EINVAL;
		}
		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT; i++) {
			if (tmp_server->snk.cap_streams[i] == stream) {
				*server = tmp_server;
				LOG_DBG("Found server for sink stream at index %d", srv_idx);
				matches++;
			}
		}
		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT; i++) {
			if (tmp_server->src.cap_streams[i] == stream) {
				*server = tmp_server;
				LOG_DBG("Found server for source stream at index %d", srv_idx);
				matches++;
			}
		}
	}

	if (matches == 0) {
		LOG_ERR("No server found for the given stream");
		return -ENOENT;
	} else if (matches > 1) {
		LOG_ERR("Multiple servers found for the same stream, this should not happen");
		return -ESPIPE;
	}

	return 0;
}

int srv_store_ep_state_count(struct bt_conn const *const conn, enum bt_bap_ep_state state,
			     enum bt_audio_dir dir)
{
	int ret;
	int count = 0;
	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret < 0) {
		return ret;
	}

	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT; i++) {
			if (server->snk.cap_streams[i] != NULL &&
			    server->snk.cap_streams[i]->bap_stream.ep != NULL) {
				struct bt_bap_ep_info ep_info;

				ret = bt_bap_ep_get_info(server->snk.cap_streams[i]->bap_stream.ep,
							 &ep_info);
				if (ret) {
					return ret;
				}

				if (ep_info.state == state) {
					count++;
				}
			} else {
				break;
			}
		}
		break;

	case BT_AUDIO_DIR_SOURCE:
		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT; i++) {
			if (server->src.cap_streams[i] != NULL &&
			    server->src.cap_streams[i]->bap_stream.ep != NULL) {
				struct bt_bap_ep_info ep_info;

				ret = bt_bap_ep_get_info(server->src.cap_streams[i]->bap_stream.ep,
							 &ep_info);
				if (ret) {
					return ret;
				}

				if (ep_info.state == state) {
					count++;
				}
			} else {
				break;
			}
		}
		break;

	default:
		LOG_ERR("Unknown direction: %d", dir);
		return -EINVAL;
	}

	return count;
}

int srv_store_all_ep_state_count(enum bt_bap_ep_state state, enum bt_audio_dir dir)
{
	int count = 0;
	int count_total = 0;
	struct server_store *server = NULL;

	for (int srv_idx = 0; srv_idx < server_heap.size; srv_idx++) {
		server = (struct server_store *)min_heap_get_element(&server_heap, srv_idx);
		if (server == NULL) {
			LOG_ERR("Server is NULL at index %d", srv_idx);
			return -EINVAL;
		}
		count = srv_store_ep_state_count(server->conn, state, dir);
		if (count < 0) {
			LOG_ERR("Failed to get ep state count for server %d: %d", srv_idx, count);
			return count;
		}
		count_total += count;
	}

	return count_total;
}

int srv_store_from_conn_get(struct bt_conn const *const conn, struct server_store **server)
{
	*server = (struct server_store *)min_heap_find(&server_heap, min_heap_conn_eq, conn, NULL);
	if (*server == NULL) {
		return -ENOENT;
	}

	return 0;
}

int srv_store_num_get(void)
{
	return server_heap.size;
}

int srv_store_add(struct bt_conn *conn)
{
	struct server_store server;

	server.conn = conn;
	return min_heap_push(&server_heap, (void *)&server);
}

int srv_store_remove(struct bt_conn *conn)
{
	struct server_store *dummy_server;
	size_t id;

	dummy_server =
		(struct server_store *)min_heap_find(&server_heap, min_heap_conn_eq, conn, &id);
	if (dummy_server == NULL) {
		return -ENOENT;
	}

	if (!min_heap_remove(&server_heap, id, (void *)dummy_server)) {
		return -ENOENT;
	}

	return 0;
}

int srv_store_clear_all(void)
{
	struct server_store dummy_server;

	while (!min_heap_is_empty(&server_heap)) {
		if (!min_heap_pop(&server_heap, (void *)&dummy_server)) {
			return -EIO;
		}
	}

	return 0;
}

int srv_store_init(void)
{
	return srv_store_clear_all();
}
