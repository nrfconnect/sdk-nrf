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

#include "le_audio.h"

struct unicast_server_snk_vars {
	bool waiting_for_disc;
	/* PACS response. Location should be a superset of all codec locations. Bitfield */
	uint32_t locations;
	/* This value will propagate to the streams. */
	struct bt_bap_lc3_preset lc3_preset[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	struct bt_audio_codec_cap codec_caps[CONFIG_CODEC_CAP_COUNT_MAX];
	size_t num_codec_caps;
	/* One array for discovering the eps. Do not use this for operations */
	struct bt_bap_ep *eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	size_t num_eps;
	enum bt_audio_context supported_ctx;
	/* Check this before calling unicast audio start */
	enum bt_audio_context available_ctx;
	/* We should have all info here. (Locations, stream status etc.) */
	struct bt_cap_stream cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
};

struct unicast_server_src_vars {
	bool waiting_for_disc;
	uint32_t locations;
	struct bt_bap_lc3_preset lc3_preset[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
	struct bt_audio_codec_cap codec_caps[CONFIG_CODEC_CAP_COUNT_MAX];
	size_t num_codec_caps;
	struct bt_bap_ep *eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
	size_t num_eps;
	enum bt_audio_context supported_ctx;
	enum bt_audio_context available_ctx;
	struct bt_cap_stream cap_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
};

/* This struct holds the actual values of codec configs and QoS across all CIGs */
struct server_store {
	char *name;
	struct bt_conn *conn;
	const struct bt_csip_set_coordinator_set_member *member;

	struct unicast_server_snk_vars snk;
	struct unicast_server_src_vars src;
};

int srv_store_src_num_get(uint8_t cig_id);

int srv_store_snk_num_get(uint8_t cig_id);

// What is unicast group versus CIG?
int srv_store_cig_get(uint8_t cig_id, struct bt_bap_stream const *const stream);

int srv_store_cig_pres_dly_find(uint8_t cig_id, uint32_t *common_pres_dly_us,
				enum bt_audio_dir dir);

/**
 * @brief Search for a common presentation delay across all server Audio Stream Endpoints (ASEs) for
 * the given direction. This function will try to satisfy the preffered presentation delay for all
 * ASEs. If that is not possible, it will try to satisfy the max and min values.
 *
 * @param common_pres_dly_us Pointer to store the new common presentation delay in microseconds.
 * @param dir The direction of the Audio Stream Endpoints (ASEs) to search for.
 *
 * @note Thisfunction will search across CIGs. This may not make sense, as the the same presentation
 * delay is only mandated within a CIG.
 *
 * @return 0 on success, negative error code on failure.
 * @return -ESPIPE if there is no common presentation delay found.
 * @return -EMLINK if the search was conducted across multiple CIGs
 */
int srv_store_pres_dly_find(struct bt_bap_stream *stream, uint32_t *new_pres_dly_us,
			    struct bt_bap_qos_cfg_pref const *qos_cfg_pref_in,
			    bool *group_reconfig_needed);

int srv_store_location_set(struct bt_conn *conn, enum bt_audio_dir dir, enum bt_audio_location loc);

int srv_store_valid_codec_cap_check(struct bt_conn const *const conn, enum bt_audio_dir dir,
				    uint32_t *valid_codec_caps);

int srv_store_stream_dir_get(struct bt_bap_stream const *const stream);

int srv_store_from_stream_get(struct bt_bap_stream const *const stream,
			      struct server_store **server);

int srv_store_stream_idx_get(struct bt_bap_stream const *const stream, struct stream_index *idx);

int srv_store_all_ep_state_count(enum bt_bap_ep_state state, enum bt_audio_dir dir);

int srv_store_avail_context_set(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				enum bt_audio_context src_ctx);

int srv_store_codec_cap_set(struct bt_conn *conn, enum bt_audio_dir dir,
			    const struct bt_audio_codec_cap *codec);

int srv_store_from_conn_get(struct bt_conn const *const conn, struct server_store **server);

int srv_store_num_get(void);

int srv_store_add(struct bt_conn *conn);

int srv_store_remove(struct bt_conn *conn);

int srv_store_remove_all(void);

int srv_store_init(void);

#endif /* _SERVER_STORE_H_ */
