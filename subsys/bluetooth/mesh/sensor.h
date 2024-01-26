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
#include <bluetooth/mesh/sensor_srv.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* Temporary typedef while we need to support two different sensor value types
 * in the deprecation period. Can be removed once support for
 * struct sensor_value is removed, and internal APIs changed to use
 * struct bt_mesh_sensor_value.
 */
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
typedef struct sensor_value sensor_value_type;
#else
typedef struct bt_mesh_sensor_value sensor_value_type;
#endif

#ifndef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
/** @brief Check if a value change breaks the delta threshold.
 *
 *  Sensors should publish their value if the measured sample is outside the
 *  delta threshold compared to the previously published value. This function
 *  checks the threshold and the previously published value for this sensor,
 *  and returns whether the sensor should publish its value.
 *
 *  @note Only single-channel sensors support cadence. Multi-channel sensors are
 *        always considered out of their threshold range, and will always return
 *        true from this function. Single-channel sensors that haven't been
 *        assigned a threshold will return true if the value is different.
 *
 *  @param[in] sensor    The sensor instance.
 *  @param[in] value     Sensor value.
 *
 *  @return true if the difference between the measurements exceeds the delta
 *          threshold, false otherwise.
 */
bool bt_mesh_sensor_delta_threshold(const struct bt_mesh_sensor *sensor,
				    const struct bt_mesh_sensor_value *value);
#endif /* CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE */

int sensor_status_encode(struct net_buf_simple *buf,
			 const struct bt_mesh_sensor *sensor,
			 const sensor_value_type *values);

int sensor_status_id_encode(struct net_buf_simple *buf, uint8_t len, uint16_t id);
void sensor_status_id_decode(struct net_buf_simple *buf, uint8_t *len, uint16_t *id);

void sensor_descriptor_decode(struct net_buf_simple *buf,
			      struct bt_mesh_sensor_info *sensor);
void sensor_descriptor_encode(struct net_buf_simple *buf,
			      struct bt_mesh_sensor *sensor);

int sensor_value_encode(struct net_buf_simple *buf,
			const struct bt_mesh_sensor_type *type,
			const sensor_value_type *values);
int sensor_value_decode(struct net_buf_simple *buf,
			const struct bt_mesh_sensor_type *type,
			sensor_value_type *values);

int sensor_ch_encode(struct net_buf_simple *buf,
		     const struct bt_mesh_sensor_format *format,
		     const sensor_value_type *value);
int sensor_ch_decode(struct net_buf_simple *buf,
		     const struct bt_mesh_sensor_format *format,
		     sensor_value_type *value);

int sensor_column_value_encode(struct net_buf_simple *buf,
			       struct bt_mesh_sensor_srv *srv,
			       struct bt_mesh_sensor *sensor,
			       struct bt_mesh_msg_ctx *ctx,
			       uint32_t column_index);
int sensor_column_encode(struct net_buf_simple *buf,
			 struct bt_mesh_sensor_srv *srv,
			 struct bt_mesh_sensor *sensor,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_sensor_column *col);
int sensor_column_decode(
	struct net_buf_simple *buf, const struct bt_mesh_sensor_type *type,
	struct bt_mesh_sensor_column *col,
	sensor_value_type value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX]);

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
			   const sensor_value_type *value);

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_INTERNAL_SENSOR_H__ */
