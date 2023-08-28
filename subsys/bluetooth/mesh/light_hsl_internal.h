/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file
 * @brief Internal Light HSL API
 */

#ifndef BT_MESH_INTERNAL_LIGHT_HSL_H__
#define BT_MESH_INTERNAL_LIGHT_HSL_H__

#include <bluetooth/mesh/light_sat_srv.h>
#include <bluetooth/mesh/light_hue_srv.h>
#include "lightness_internal.h"

static inline void light_hsl_buf_push(struct net_buf_simple *buf,
				      const struct bt_mesh_light_hsl *hsl)
{
	net_buf_simple_add_le16(buf, to_actual(hsl->lightness));
	net_buf_simple_add_le16(buf, hsl->hue);
	net_buf_simple_add_le16(buf, hsl->saturation);
}

static inline void
light_hue_sat_range_buf_push(struct net_buf_simple *buf,
			     const struct bt_mesh_light_hue_sat_range *range)
{
	net_buf_simple_add_le16(buf, range->min.hue);
	net_buf_simple_add_le16(buf, range->max.hue);
	net_buf_simple_add_le16(buf, range->min.saturation);
	net_buf_simple_add_le16(buf, range->max.saturation);
}

static inline void light_hsl_buf_pull(struct net_buf_simple *buf,
				      struct bt_mesh_light_hsl *hsl)
{
	hsl->lightness = from_actual(net_buf_simple_pull_le16(buf));
	hsl->hue = net_buf_simple_pull_le16(buf);
	hsl->saturation = net_buf_simple_pull_le16(buf);
}

static inline void
light_hue_sat_range_buf_pull(struct net_buf_simple *buf,
			     struct bt_mesh_light_hue_sat_range *range)
{
	range->min.hue = net_buf_simple_pull_le16(buf);
	range->max.hue = net_buf_simple_pull_le16(buf);
	range->min.saturation = net_buf_simple_pull_le16(buf);
	range->max.saturation = net_buf_simple_pull_le16(buf);
}

void bt_mesh_light_hue_srv_set(struct bt_mesh_light_hue_srv *srv,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_light_hue *set,
				   struct bt_mesh_light_hue_status *status);

void bt_mesh_light_hue_srv_default_set(struct bt_mesh_light_hue_srv *srv,
					   struct bt_mesh_msg_ctx *ctx,
					   uint16_t dflt);

void bt_mesh_light_hue_srv_range_set(
	struct bt_mesh_light_hue_srv *srv, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_hsl_range *range);

void bt_mesh_light_sat_srv_set(struct bt_mesh_light_sat_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_light_sat *set,
			       struct bt_mesh_light_sat_status *status);

void bt_mesh_light_sat_srv_default_set(struct bt_mesh_light_sat_srv *srv,
					   struct bt_mesh_msg_ctx *ctx,
					   uint16_t dflt);

void bt_mesh_light_sat_srv_range_set(
	struct bt_mesh_light_sat_srv *srv, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_hsl_range *range);

#endif /* BT_MESH_INTERNAL_LIGHT_HSL_H__ */
