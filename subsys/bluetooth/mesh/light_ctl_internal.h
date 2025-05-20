/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Internal Light CTL API
 */

#ifndef BT_MESH_INTERNAL_LIGHT_CTL_H__
#define BT_MESH_INTERNAL_LIGHT_CTL_H__

#include <bluetooth/mesh/light_temp_srv.h>
#include "model_utils.h"

#define LIGHT_TEMP_LVL_OFFSET (-INT16_MIN)

static inline uint16_t get_range_min(uint16_t min)
{
	return min == BT_MESH_LIGHT_TEMP_UNKNOWN ? BT_MESH_LIGHT_TEMP_MIN : min;
}

static inline uint16_t get_range_max(uint16_t max)
{
	return max == BT_MESH_LIGHT_TEMP_UNKNOWN ? BT_MESH_LIGHT_TEMP_MAX : max;
}

static inline uint16_t lvl_to_temp(struct bt_mesh_light_temp_srv *srv, int16_t lvl)
{
	uint16_t min = get_range_min(srv->range.min);
	uint16_t max = get_range_max(srv->range.max);

	return min + ROUNDED_DIV((int32_t)(lvl + LIGHT_TEMP_LVL_OFFSET) * (max - min),
				 (int32_t)UINT16_MAX);
}

static inline int16_t temp_to_lvl(struct bt_mesh_light_temp_srv *srv, uint16_t raw_temp)
{
	uint16_t min = get_range_min(srv->range.min);
	uint16_t max = get_range_max(srv->range.max);
	uint16_t temp = CLAMP(raw_temp, min, max);

	return max == min ? 0
			  : ROUNDED_DIV((temp - min) * UINT16_MAX, (max - min)) -
				    LIGHT_TEMP_LVL_OFFSET;
}

void bt_mesh_light_temp_srv_set(struct bt_mesh_light_temp_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_light_temp_set *set,
				struct bt_mesh_light_temp_status *rsp);

enum bt_mesh_model_status
bt_mesh_light_temp_srv_range_set(struct bt_mesh_light_temp_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 struct bt_mesh_light_temp_range *range);

void bt_mesh_light_temp_srv_default_set(struct bt_mesh_light_temp_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_temp *dflt);

#endif /* BT_MESH_INTERNAL_LIGHT_CTL_H__ */
