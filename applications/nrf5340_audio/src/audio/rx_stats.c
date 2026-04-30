/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "rx_stats.h"

#include <stdint.h>
#include <zephyr/bluetooth/audio/bap.h>
#include "le_audio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rx_stats, 4);

struct rx_stats {
	struct bt_bap_stream *stream;
	uint32_t recv_cnt;
	uint32_t good_frame_cnt;
	uint32_t bad_frame_cnt;
	bool prev_bad_data;
};

static struct rx_stats rx_stats[CONFIG_BT_AUDIO_CONCURRENT_RX_STREAMS_MAX];

int rx_stats_stream_recv(struct bt_bap_stream const *const stream, struct audio_metadata meta)
{
	uint8_t idx = UINT8_MAX;

	for (int i = 0; i < ARRAY_SIZE(rx_stats); i++) {
		if (rx_stats[i].stream == stream) {
			idx = i;
			break;
		}
	}

	if (idx == UINT8_MAX) {
		LOG_INF_RATELIMIT("Could not find stream in stats");
		return -ESRCH;
	}

	rx_stats[idx].recv_cnt++;
	if (meta.bad_data) {
		rx_stats[idx].bad_frame_cnt++;
	} else {
		rx_stats[idx].good_frame_cnt++;
	}
	if ((rx_stats[idx].recv_cnt % 100) == 0 && rx_stats[idx].recv_cnt) {
		/* NOTE: The string below is used by the Nordic CI system */
		LOG_DBG("ISO RX SDUs: Loc: %d Total: %d Bad: %d", idx, rx_stats[idx].recv_cnt,
			rx_stats[idx].bad_frame_cnt);
	}
	if (meta.bad_data && !rx_stats[idx].prev_bad_data) {
		LOG_INF_RATELIMIT_RATE(1000,
				       "Bad or 0 SDU: Loc: %u Total: %u Bad: %u. (Prints "
				       "good->bad transition)",
				       idx, rx_stats[idx].recv_cnt, rx_stats[idx].bad_frame_cnt);
	}
	rx_stats[idx].prev_bad_data = meta.bad_data;

	return 0;
}

int rx_stats_stream_clear(struct bt_bap_stream const *const stream)
{
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir == BT_AUDIO_DIR_SOURCE) {
		/* We only want to track RX stats for sink streams */
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(rx_stats); i++) {
		if (rx_stats[i].stream == stream) {
			memset(&rx_stats[i], 0, sizeof(struct rx_stats));
			LOG_DBG("Cleared stream from stats");
			return 0;
		}
	}

	LOG_ERR("Stream not found in stats");
	return -EINVAL;
}

int rx_stats_stream_start(struct bt_bap_stream const *const stream)
{
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir == BT_AUDIO_DIR_SOURCE) {
		/* We only want to track RX stats for sink streams */
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(rx_stats); i++) {
		if (rx_stats[i].stream == stream) {
			LOG_DBG("Stream already in stats");
			return 0;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(rx_stats); i++) {
		if (rx_stats[i].stream == NULL) {
			rx_stats[i].stream = (struct bt_bap_stream *)stream;
			LOG_DBG("Added stream to stats");
			return 0;
		}
	}

	LOG_ERR("No free slot for stream in stats");
	return -EINVAL;
}
