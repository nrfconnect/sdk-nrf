/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Internal time API
 */

#ifndef BT_MESH_INTERNAL_TIME_H__
#define BT_MESH_INTERNAL_TIME_H__

#include <zephyr/types.h>
#include "model_utils.h"

#define ZONE_CHANGE_ZERO_POINT 0x40
#define UTC_CHANGE_ZERO_POINT 0x00FF

void bt_mesh_time_decode_time_params(struct net_buf_simple *buf,
				     struct bt_mesh_time_status *status);

void bt_mesh_time_encode_time_params(struct net_buf_simple *buf,
				     const struct bt_mesh_time_status *status);

static inline uint64_t bt_mesh_time_buf_pull_tai_sec(struct net_buf_simple *buf)
{
	uint64_t sec;

	sec = net_buf_simple_pull_le32(buf);
	sec |= (uint64_t)net_buf_simple_pull_u8(buf) << 32U;
	return sec;
}

static inline void bt_mesh_time_buf_put_tai_sec(struct net_buf_simple *buf,
						uint64_t sec)
{
	uint8_t *seconds = net_buf_simple_add(buf, 5);

	for (uint8_t i = 0; i < 5; i++) {
		seconds[i] = sec >> (i * 8);
	}
}

#endif /* BT_MESH_INTERNAL_TIME_H__ */
