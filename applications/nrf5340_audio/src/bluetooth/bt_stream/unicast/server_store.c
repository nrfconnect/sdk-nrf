/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/slist.h>

#include "server_store.h"

/* This array keeps track of all the remote unicast servers this unicast client is operating on.
 **/
/* static struct server_store servers[CONFIG_BT_MAX_CONN]; */

/* #define MIN_HEAP_DEFINE_STATIC \
	("servers", CONFIG_BT_MAX_CONN, sizeof(struct server_store), 4, cmp_func); */

int srv_store_src_num_get(uint8_t cig_idx);

int srv_store_snk_num_get(uint8_t cig_idx);

int srv_store_pres_dly_find(uint8_t cig_idx, uint32_t *new_pres_dly_us);

int srv_store_location_set(struct bt_conn *conn, enum bt_audio_dir dir, enum bt_audio_location loc);

int srv_store_avail_context_set(
	struct bt_conn *conn, enum bt_audio_context snk_ctx,
	enum bt_audio_context src_ctx); /* Emil: Do we have to store all caps? */

int srv_store_codec_cap_set(struct bt_conn *conn, enum bt_audio_dir dir,
			    const struct bt_audio_codec_cap *codec);

int srv_store_ep_set(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep);

int srv_store_valid_codec_cap_check(struct bt_conn const *const conn, enum bt_audio_dir dir,
				    bool **array_of_valid_caps);

int srv_store_stream_idx_get(struct bt_bap_stream const *const stream); /* May not be required */

int srv_store_num_running_streams_get(void);

int srv_store_add(struct bt_conn *conn);

int srv_store_remove(struct bt_conn *conn);

int srv_store_from_conn_get(struct bt_conn const *const conn, struct server_store *server)
{
	return 0;
}

int srv_store_from_stream_get(struct bt_bap_stream const *const stream, struct server_store *server)
{
	return 0;
}

int srv_store_clear_all(void)
{

	return 0;
}

int srv_store_init(void)
{
	/* sys_slist_init(&server_list); */
	return srv_store_clear_all();
}
