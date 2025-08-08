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

int srv_store_clear_all(void);

int srv_store_clear_conn(struct bt_conn *conn);

int srv_store_src_num_get(uint8_t cig_idx);

int srv_store_snk_num_get(uint8_t cig_idx);

int srv_store_pres_dly_find(uint8_t cig_idx, uint32_t *new_pres_dly_us);

int srv_store_location_set(struct bt_conn *conn, enum bt_audio_dir dir, enum bt_audio_location loc);

int srv_store_avail_context_set(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				enum bt_audio_context src_ctx); /* Do we have to store all caps? */

int srv_store_codec_cap_set(struct bt_conn *conn, enum bt_audio_dir dir,
			    const struct bt_audio_codec_cap *codec);

int srv_store_ep_set(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep);

int srv_store_valid_codec_cap_check(struct bt_conn const *const conn, enum bt_audio_dir dir,
				    bool **array_of_valid_caps);

int srv_store_stream_idx_get(struct bt_bap_stream const *const stream); /* May be not needed?*/

int srv_store_num_running_streams_get(void);

int srv_store_init(void);

int srv_store_init(void);

#endif /* _SERVER_STORE_H_ */
