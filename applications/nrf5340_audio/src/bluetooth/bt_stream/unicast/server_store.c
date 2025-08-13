/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/slist.h>
#include <zephyr/bluetooth/conn.h>

#include "server_store.h"
#include "min_heap.h"

/* TODO: Dicuss this include */
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(unicast_client, CONFIG_UNICAST_CLIENT_LOG_LEVEL);

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

struct pd {
	uint32_t min;
	uint32_t max;
	uint32_t pref_min;
	uint32_t pref_max;
};

static bool pres_dly_in_range(uint32_t existing_pres_dly_us,
			      struct bt_bap_qos_cfg_pref const *const qos_cfg_pref_in)
{
	if (qos_cfg_pref_in->pd_max == 0 || qos_cfg_pref_in->pd_min == 0) {
		LOG_ERR("No max or min presentation delay set");
		return false;
	}

	if (existing_pres_dly_us > qos_cfg_pref_in->pd_max) {
		LOG_DBG("Existing presentation delay %u is greater than max %u",
			existing_pres_dly_us, qos_cfg_pref_in->pd_max);
		return false;
	}

	if (existing_pres_dly_us < qos_cfg_pref_in->pd_min) {
		LOG_DBG("Existing presentation delay %u is less than min %u", existing_pres_dly_us,
			qos_cfg_pref_in->pd_min);
		return false;
	}
	/* We will not check the preferred presentation delay if we already have a running stream in
	 * the same group*/

	return true;
}

/* Looking for the smallest commonly acceptable set of presentation delays */
static int pres_delay_find(struct bt_bap_qos_cfg_pref *common,
			   struct bt_bap_qos_cfg_pref const *const in)
{
	/* Checking min values */
	if (!IN_RANGE(in->pd_min, common->pd_min, common->pd_max)) {
		LOG_ERR("Incoming pd_max %d is not in range [%d, %d]", in->pd_max, common->pd_min,
			common->pd_max);
		return -EINVAL;
	}

	common->pd_min = MAX(in->pd_min, common->pd_min);

	if (common->pref_pd_min == 0) {
		common->pref_pd_min = in->pd_min;
	} else if (in->pref_pd_min == 0) {
		/* Incoming has no preferred min */
	} else {
		if (!IN_RANGE(in->pref_pd_min, common->pd_min, common->pd_max)) {
			LOG_WRN("Incoming pref_pd_min %d is not in range [%d, %d]. Will revert to "
				"common "
				"pd_min %d",
				in->pref_pd_min, common->pd_min, common->pd_max, common->pd_min);
			common->pref_pd_min = UINT32_MAX;
		}
	}

	/* Checking max values */
	if (!IN_RANGE(in->pd_max, common->pd_min, common->pd_max)) {
		LOG_WRN("Incoming pd_max %d is not in range [%d, %d]. Ignored.", in->pd_max,
			common->pd_min, common->pd_max);
		common->pd_max = 0;
	} else {
		common->pd_max = MIN(in->pd_max, common->pd_max);
	}

	if (common->pref_pd_max == 0) {
		common->pref_pd_max = in->pd_max;
	} else if (in->pref_pd_max == 0) {
		/* Incoming has no preferred max */
	} else {
		if (!IN_RANGE(in->pref_pd_max, common->pd_min, common->pd_max)) {
			LOG_WRN("Incoming pref_pd_max %d is not in range [%d, %d]. Will revert to "
				"common "
				"pd_max %d. Ignored.",
				in->pref_pd_max, common->pd_min, common->pd_max, common->pd_max);
			common->pref_pd_max = 0;
		} else {
			common->pref_pd_max = MIN(in->pref_pd_max, common->pref_pd_max);
		}
	}

	return 0;
}

/* One needs to look at the group pointer, if this group pointer already exists, we may need
to update the entire group. If it is a new group, no reconfig is needed.
*  New A, no ext group
*  New A, existing A. If new pres dlay needed, reconfig all
*  New B, existing A, no reconfig needed.
*/

/* The presentation delay within a CIG for a given dir needs to be the same.
 * bt_bap_ep -> cig_id;
 */
int srv_store_pres_dly_find(struct bt_bap_stream *stream, uint32_t *computed_pres_dly_us,
			    struct bt_bap_qos_cfg_pref const *qos_cfg_pref_in,
			    bool *group_reconfig_needed)
{
	if (stream == NULL || computed_pres_dly_us == NULL || qos_cfg_pref_in == NULL ||
	    group_reconfig_needed == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	if (stream->group == NULL) {
		LOG_ERR("The incoming stream %p has no group", (void *)stream);
		return -EINVAL;
	}

	if (qos_cfg_pref_in->pd_min == 0 || qos_cfg_pref_in->pd_max == 0) {
		LOG_ERR("Incoming pd_min or pd_max is zero");
		return -EINVAL;
	}

	int ret;

	*group_reconfig_needed = false;
	*computed_pres_dly_us = UINT32_MAX;
	struct bt_bap_qos_cfg_pref common_pd = *qos_cfg_pref_in;

	struct bt_bap_ep_info ep_info;
	bt_bap_ep_get_info(stream->ep, &ep_info);
	enum bt_audio_dir dir = ep_info.dir;

	struct server_store *server = NULL;
	for (int srv_idx = 0; srv_idx < server_heap.size; srv_idx++) {
		LOG_ERR("HEAP SIZE %d", server_heap.size);

		/* Across all servers, we need to first check if another stream is in the
		 * same subgroup as the new incoming stream.
		 */
		server = (struct server_store *)min_heap_get_element(&server_heap, srv_idx);
		if (server == NULL) {
			LOG_ERR("Server is NULL at index %d", srv_idx);
			return -EINVAL;
		}

		if (server->conn == stream->conn) {
			/* Do not compare against the same unicast server */
			continue;
		}

		switch (dir) {
		case BT_AUDIO_DIR_SINK:
			for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT; i++) {
				if (server->snk.cap_streams[i] == NULL) {
					break;
				}

				if (server->snk.cap_streams[i]->bap_stream.group == NULL) {
					/* The stream we are comparing against has no group
					 */
					continue;
				}

				/* Only need to check pres delay if the stream is part of the same
				 * group */
				if (stream->group != server->snk.cap_streams[i]->bap_stream.group) {
					continue;
				}

				if (stream == &server->snk.cap_streams[i]->bap_stream) {
					/* This is our own stream, so we can skip it */
					continue;
				}

				if (server->snk.cap_streams[i]->bap_stream.ep == NULL) {
					continue;
				}

				uint32_t existing_pres_dly_us =
					server->snk.cap_streams[i]->bap_stream.qos->pd;

				if (pres_dly_in_range(existing_pres_dly_us, qos_cfg_pref_in)) {

					*computed_pres_dly_us = existing_pres_dly_us;
					return 0;
				}

				*group_reconfig_needed = true;

				ret = pres_delay_find(
					&common_pd,
					&server->snk.cap_streams[i]->bap_stream.ep->qos_pref);
				if (ret) {
					return ret;
				}
			}
			break;

		case BT_AUDIO_DIR_SOURCE:

			break;
		default:
			LOG_ERR("Unknown direction: %d", dir);
			return -EINVAL;
		};

		return -EPERM;
	}

	if (group_reconfig_needed) {
		/* Found other streams, need to use common denominator */
		if (common_pd.pref_pd_min == UINT32_MAX || common_pd.pref_pd_min == 0) {
			/* No preferred min, use common min */
			*computed_pres_dly_us = common_pd.pd_min;
		} else {
			/* Use preferred min */
			*computed_pres_dly_us = common_pd.pref_pd_min;
		}
		return 0;
	}

	/* No other streams in the same group found */

	if (qos_cfg_pref_in->pref_pd_min == 0) {
		*computed_pres_dly_us = qos_cfg_pref_in->pd_min;
	} else {
		*computed_pres_dly_us = qos_cfg_pref_in->pref_pd_min;
	}
	return 0;
}

int srv_store_cig_pres_dly_find(uint8_t cig_id, uint32_t *common_pres_dly_us, enum bt_audio_dir dir)
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
				    uint32_t *valid_codec_caps)
{
	return -EPERM;
}

int srv_store_stream_idx_get(struct bt_bap_stream const *const stream, struct stream_index *idx)
{
	return -EPERM;
}

int srv_store_stream_dir_get(struct bt_bap_stream const *const stream)
{
	int ret;

	if (stream == NULL) {
		LOG_ERR("Stream is NULL");
		return -EINVAL;
	}

	if (stream->ep == NULL) {
		LOG_ERR("Stream endpoint is NULL");
		return -EINVAL;
	}

	struct bt_bap_ep_info ep_info;

	ret = bt_bap_ep_get_info(stream->ep, &ep_info);
	if (ret) {
		LOG_ERR("Failed to get endpoint info: %d", ret);
		return ret;
	}

	return ep_info.dir;
}

int srv_store_from_stream_get(struct bt_bap_stream const *const stream,
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
			if (&tmp_server->snk.cap_streams[i]->bap_stream == stream) {
				*server = tmp_server;
				LOG_DBG("Found server for sink stream at index %d", srv_idx);
				matches++;
			}
		}
		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT; i++) {
			if (&tmp_server->src.cap_streams[i]->bap_stream == stream) {
				*server = tmp_server;
				LOG_DBG("Found server for source stream at index "
					"%d",
					srv_idx);
				matches++;
			}
		}
	}

	if (matches == 0) {
		LOG_ERR("No server found for the given stream");
		return -ENOENT;
	} else if (matches > 1) {
		LOG_ERR("Multiple servers found for the same stream, this should "
			"not happen");
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
			if (server->snk.cap_streams[i] == NULL ||
			    server->snk.cap_streams[i]->bap_stream.ep == NULL) {
				break;
			}

			struct bt_bap_ep_info ep_info;

			ret = bt_bap_ep_get_info(server->snk.cap_streams[i]->bap_stream.ep,
						 &ep_info);
			if (ret) {
				return ret;
			}

			if (ep_info.state == state) {
				count++;
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

int srv_store_remove_all(void)
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
	return srv_store_remove_all();
}
