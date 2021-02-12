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

static inline uint16_t lvl_to_temp(struct bt_mesh_light_temp_srv *srv,
				   int16_t lvl)
{
	return srv->range.min + (lvl + ((UINT16_MAX / 2) + 1)) *
					(srv->range.max - srv->range.min) /
					UINT16_MAX;
}

static inline int16_t temp_to_lvl(struct bt_mesh_light_temp_srv *srv,
				  uint16_t raw_temp)
{
	uint16_t temp = CLAMP(raw_temp, srv->range.min, srv->range.max);

	return (temp - srv->range.min) * UINT16_MAX /
		       (srv->range.max - srv->range.min) -
	       ((UINT16_MAX / 2) + 1);
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
