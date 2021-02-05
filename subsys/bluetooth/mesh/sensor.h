/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file
 * @brief Internal sensor API
 */

#ifndef BT_MESH_INTERNAL_SENSOR_H__
#define BT_MESH_INTERNAL_SENSOR_H__

#include <bluetooth/mesh/sensor.h>
#include <bluetooth/mesh/sensor_cli.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup BT_MESH_SENSOR_FORMATS Sensor channel formats
 * @brief All available sensor channel formats in the mesh device properties
 * Specification.
 * @{
 */

/* Percentage formats */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_percentage_8;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_percentage_16;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_percentage_delta_trigger;

/* Environmental formats */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_temp_8;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_temp;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_co2_concentration;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_noise;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_voc_concentration;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_wind_speed;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_temp_8_wide;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_gust_factor;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_magnetic_flux_density;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_pollen_concentration;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_pressure;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_rainfall;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_uv_index;

/* Time formats */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_decihour_8;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_hour_24;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_second_16;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_time_millisecond_24;
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
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_perceived_lightness;

/* Miscellaneous formats */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_direction_16;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_count_16;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_gen_lvl;
extern const struct bt_mesh_sensor_format
	bt_mesh_sensor_format_cos_of_the_angle;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_boolean;
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_coefficient;

/** @} */

/**
 * @defgroup BT_MESH_SENSOR_UNITS Sensor value units
 * @brief All available sensor value units in the mesh device properties
 * specification.
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
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_degrees;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_mps;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_microtesla;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_concentration;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_pascal;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_metre;
extern const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_unitless;

/** @} */

int sensor_status_encode(struct net_buf_simple *buf,
			 const struct bt_mesh_sensor *sensor,
			 const struct sensor_value *values);

int sensor_status_id_encode(struct net_buf_simple *buf, uint8_t len, uint16_t id);
void sensor_status_id_decode(struct net_buf_simple *buf, uint8_t *len, uint16_t *id);

void sensor_descriptor_decode(struct net_buf_simple *buf,
			      struct bt_mesh_sensor_info *sensor);
void sensor_descriptor_encode(struct net_buf_simple *buf,
			      struct bt_mesh_sensor *sensor);

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
			  uint8_t fast_period_div, uint8_t min_int,
			  const struct bt_mesh_sensor_threshold *threshold);
int sensor_cadence_decode(struct net_buf_simple *buf,
			  const struct bt_mesh_sensor_type *sensor_type,
			  uint8_t *fast_period_div, uint8_t *min_int,
			  struct bt_mesh_sensor_threshold *threshold);
uint8_t sensor_value_len(const struct bt_mesh_sensor_type *type);

uint8_t sensor_powtime_encode(uint64_t raw);
uint64_t sensor_powtime_decode(uint8_t encoded);
uint64_t sensor_powtime_decode_us(uint8_t val);

uint8_t sensor_pub_div_get(const struct bt_mesh_sensor *s, uint32_t base_period);

void sensor_cadence_update(struct bt_mesh_sensor *sensor,
			   const struct sensor_value *value);

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_INTERNAL_SENSOR_H__ */
