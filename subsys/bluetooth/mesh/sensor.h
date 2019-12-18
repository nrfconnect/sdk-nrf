/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**
 * @file
 * @brief Internal sensor API
 */

#ifndef BT_MESH_INTERNAL_SENSOR_H__
#define BT_MESH_INTERNAL_SENSOR_H__

#include <bluetooth/mesh/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup BT_MESH_SENSOR_FORMATS Sensor channel formats
 * @brief All available sensor channel formats in the Mesh Device Properties
 *        Specification.
 * @{
 */

/* Percentage formats */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_percentage_8;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_percentage_16;

/* Environmental formats */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_temp_8;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_temp;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_humidity;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_co2_concentration;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_voc_concentration;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_noise;

/* Time formats */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_decihour_8;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_hour_24;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_second_16;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_exp_8;

/* Electrical formats */
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_electric_current;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_voltage;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_energy;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_energy32;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_power;

/* Lighting formats */
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_chromatic_distance;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_chromaticity_coordinate;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_correlated_color_temp;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_illuminance;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_luminous_efficacy;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_luminous_energy;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_luminous_exposure;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_luminous_flux;

/* Miscellaneous formats */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_count_16;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_gen_lvl;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_cos_of_the_angle;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_boolean;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_coefficient;

/** @} */

/**
 * @defgroup BT_MESH_SENSOR_UNITS Sensor value units
 * @brief All available sensor value units in the Mesh Device Properties
 *        Specification.
 * @{
 */
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_hours;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_seconds;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_celsius;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_kelvin;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_percent;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_ppm;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_ppb;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_volt;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_ampere;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_watt;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_kwh;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_db;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_lux;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_lux_hour;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_lumen;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_lumen_per_watt;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_lumen_hour;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_gram_per_sec;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_litre_per_sec;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_unitless;

/** @} */

int sensor_status_encode(struct net_buf_simple *buf,
			 const struct bt_mesh_sensor *sensor,
			 const struct sensor_value *values);

int sensor_status_id_encode(struct net_buf_simple *buf, u8_t len, u16_t id);
void sensor_status_id_decode(struct net_buf_simple *buf, u8_t *len, u16_t *id);

int sensor_value_encode(struct net_buf_simple *buf,
			const struct bt_mesh_sensor_type *type,
			const struct sensor_value *values);
int sensor_value_decode(struct net_buf_simple *buf,
			const struct bt_mesh_sensor_type *type,
			struct sensor_value *values);

int sensor_ch_encode(struct net_buf_simple *buf,
		     const struct bt_mesh_sensor_format *format,
		     const struct sensor_value *value);
int sensor_ch_decode(struct net_buf_simple *buf,
		     const struct bt_mesh_sensor_format *format,
		     struct sensor_value *value);

int sensor_column_encode(struct net_buf_simple *buf,
			 struct bt_mesh_sensor *sensor,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_sensor_column *col);
int sensor_column_decode(
	struct net_buf_simple *buf, const struct bt_mesh_sensor_type *type,
	struct bt_mesh_sensor_column *col,
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX]);

int sensor_cadence_encode(struct net_buf_simple *buf,
			  const struct bt_mesh_sensor_type *sensor_type,
			  u8_t fast_period_div, u8_t min_int,
			  const struct bt_mesh_sensor_threshold *threshold);
int sensor_cadence_decode(struct net_buf_simple *buf,
			  const struct bt_mesh_sensor_type *sensor_type,
			  u8_t *fast_period_div, u8_t *min_int,
			  struct bt_mesh_sensor_threshold *threshold);
u8_t sensor_value_len(const struct bt_mesh_sensor_type *type);

u8_t sensor_powtime_encode(u64_t raw);
u64_t sensor_powtime_decode(u8_t encoded);
u64_t sensor_powtime_decode_ns(u8_t val);

u8_t sensor_pub_div_get(const struct bt_mesh_sensor *s, u32_t base_period);

void sensor_cadence_update(struct bt_mesh_sensor *sensor,
			   const struct sensor_value *value);

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_INTERNAL_SENSOR_H__ */
