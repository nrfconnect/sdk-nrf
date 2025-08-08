/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SERVER_STORE_H_
#define _SERVER_STORE_H_

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>

struct unicast_server_snk_vars {
	/* This value will propagate to the streams */
	struct bt_bap_lc3_preset lc3_preset[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	bool waiting_for_disc;
	struct bt_audio_codec_cap codec_caps[CONFIG_CODEC_CAP_COUNT_MAX];
	/* Do we need this as we have the locations in the codec? */
	uint32_t location;

	/* We should have all info here. (Locations, stream status etc.) */
	struct bt_cap_stream *cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	/* One array for discovering the eps. Do not use this for operations */
	struct bt_bap_ep *eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	uint8_t num_eps;
	enum bt_audio_context ctx;
};

struct unicast_server_src_vars {
	/* This value will propagate to the streams */
	struct bt_bap_lc3_preset lc3_preset[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
	bool waiting_for_disc;
	struct bt_audio_codec_cap codec_caps[CONFIG_CODEC_CAP_COUNT_MAX];
	uint32_t location;
	struct bt_cap_stream *cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
	struct bt_bap_ep *eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
	uint8_t num_eps;
	enum bt_audio_context ctx;
};

/* This struct holds the actual values of codec configs and QoS across all CIGs */
struct server_store {
	char *name;
	struct bt_conn *conn;
	const struct bt_csip_set_coordinator_set_member *member;

	/* Flag set if we need to reconfig codec and/or QoS. */
	bool flag_reconfig; /* Check, this may be omitted. */

	struct unicast_server_snk_vars snk;
	struct unicast_server_src_vars src;
};

int srv_store_src_num_get(uint8_t cig_idx);

int srv_store_snk_num_get(uint8_t cig_idx);

int srv_store_pres_dly_find(uint8_t cig_idx, uint32_t *new_pres_dly_us);

int srv_store_location_set(struct bt_conn *conn, enum bt_audio_dir dir, enum bt_audio_location loc);

int srv_store_valid_codec_cap_check(struct bt_conn const *const conn, enum bt_audio_dir dir,
				    bool **array_of_valid_caps);

int srv_store_stream_idx_get(struct bt_bap_stream const *const stream); /* May be not needed?*/

int srv_store_from_stream_get(struct bt_cap_stream const *const stream,
			      struct server_store **server);

int srv_store_from_conn_get(struct bt_conn const *const conn, struct server_store **server);

int srv_store_num_get(void);

int srv_store_add(struct bt_conn *conn);

int srv_store_remove(struct bt_conn *conn);

int srv_store_clear_all(void);

int srv_store_init(void);

#endif /* _SERVER_STORE_H_ */
