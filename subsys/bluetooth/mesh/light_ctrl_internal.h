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

static inline const struct bt_mesh_sensor_format *
prop_format_get(enum bt_mesh_light_ctrl_prop id)
{
	switch (id) {
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_ON:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_PROLONG:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_STANDBY:
		return &bt_mesh_sensor_format_illuminance;
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_ON:
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_PROLONG:
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_STANDBY:
		return &bt_mesh_sensor_format_perceived_lightness;
	case BT_MESH_LIGHT_CTRL_PROP_REG_ACCURACY:
		return &bt_mesh_sensor_format_percentage_8;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_PROLONG:
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_ON:
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_STANDBY_AUTO:
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_STANDBY_MANUAL:
	case BT_MESH_LIGHT_CTRL_PROP_TIME_OCCUPANCY_DELAY:
	case BT_MESH_LIGHT_CTRL_PROP_TIME_PROLONG:
	case BT_MESH_LIGHT_CTRL_PROP_TIME_ON:
		return &bt_mesh_sensor_format_time_millisecond_24;
	default:
		return NULL;
	}
}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
static inline int prop_encode(struct net_buf_simple *buf,
			      enum bt_mesh_light_ctrl_prop id,
			      const struct sensor_value *val)
{
	const struct bt_mesh_sensor_format *format;

	format = prop_format_get(id);
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

	format = prop_format_get(id);
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
