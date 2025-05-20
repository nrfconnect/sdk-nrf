/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_battery_internal Generic Battery Model internals
 * @{
 * @brief Internal defines and functions for the Generic Battery models.
 */
#ifndef BT_MESH_GEN_BATTERY_INTERNAL_H__
#define BT_MESH_GEN_BATTERY_INTERNAL_H__

#include <bluetooth/mesh/gen_battery.h>

int bt_mesh_gen_bat_decode_status(struct net_buf_simple *buf,
				  struct bt_mesh_battery_status *status);
void bt_mesh_gen_bat_encode_status(struct net_buf_simple *buf,
				   const struct bt_mesh_battery_status *status);

#endif /* BT_MESH_GEN_BATTERY_INTERNAL_H__ */

/** @} */
