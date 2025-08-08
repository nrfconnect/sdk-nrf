/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/slist.h>
#include <zephyr/bluetooth/conn.h>

#include "server_store.h"
#include "min_heap.h"

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

int srv_store_num_running_streams_get(void)
{
	return -EPERM;
}

int srv_store_from_stream_get(struct bt_bap_stream const *const stream, struct server_store *server)
{
	return -EPERM;
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
