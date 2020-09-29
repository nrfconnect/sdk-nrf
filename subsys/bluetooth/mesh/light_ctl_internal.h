/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**
 * @file
 * @brief Internal Light CTL API
 */

#ifndef BT_MESH_INTERNAL_LIGHT_CTL_H__
#define BT_MESH_INTERNAL_LIGHT_CTL_H__

#include <bluetooth/mesh/light_temp_srv.h>
#include "model_utils.h"

static inline uint16_t set_temp(struct bt_mesh_light_temp_srv *srv,
				uint16_t temp_val)
{
	if (temp_val < srv->temp_range.min) {
		return srv->temp_range.min;
	} else if (temp_val > srv->temp_range.max) {
		return srv->temp_range.max;
	} else {
		return temp_val;
	}
}

static inline uint16_t lvl_to_temp(struct bt_mesh_light_temp_srv *srv,
				   int16_t lvl)
{
	return srv->temp_range.min +
	       (lvl + ((UINT16_MAX / 2) + 1)) *
		       (srv->temp_range.max - srv->temp_range.min) / UINT16_MAX;
}

static inline int16_t temp_to_lvl(struct bt_mesh_light_temp_srv *srv,
				  uint16_t raw_temp)
{
	uint16_t temp = MAX(raw_temp, srv->temp_range.min);

	temp = MIN(raw_temp, srv->temp_range.max);

	return (temp - srv->temp_range.min) * UINT16_MAX /
		       (srv->temp_range.max - srv->temp_range.min) -
	       ((UINT16_MAX / 2) + 1);
}

#endif /* BT_MESH_INTERNAL_LIGHT_CTL_H__ */
