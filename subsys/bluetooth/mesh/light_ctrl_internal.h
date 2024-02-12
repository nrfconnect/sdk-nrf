/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Light LC internal types and utilities
 */

#ifndef LIGHT_LC_INTERNAL_H__
#define LIGHT_LC_INTERNAL_H__

#include <bluetooth/mesh/light_ctrl.h>
#include <bluetooth/mesh/sensor.h>
#include <bluetooth/mesh/properties.h>
#include "sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
static inline int prop_encode(struct net_buf_simple *buf,
			      enum bt_mesh_light_ctrl_prop id,
			      const struct sensor_value *val)
{
	const struct bt_mesh_sensor_format *format;

	format = bt_mesh_lc_prop_format_get(id);
	if (!format) {
		return -ENOENT;
	}

	return sensor_ch_encode(buf, format, val);
}

static inline int prop_decode(struct net_buf_simple *buf,
			      enum bt_mesh_light_ctrl_prop id,
			      struct sensor_value *val)
{
	const struct bt_mesh_sensor_format *format;

	format = bt_mesh_lc_prop_format_get(id);
	if (!format) {
		return -ENOENT;
	}

	return sensor_ch_decode(buf, format, val);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* LIGHT_LC_INTERNAL_H__ */
