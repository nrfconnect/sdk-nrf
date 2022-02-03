/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log.h>

#include "weather_station.h"
#include "sensor.h"

/* Zigbee Cluster Library 4.4.2.2.1.1: MeasuredValue = 100x temperature in degrees Celsius */
#define ZCL_TEMPERATURE_MEMASUREMENT_MEASURED_VALUE_MULTIPLIER 100
/* Zigbee Cluster Library 4.5.2.2.1.1: MeasuredValue = 10x pressure in kPa */
#define ZCL_PRESSURE_MEMASUREMENT_MEASURED_VALUE_MULTIPLIER 10
/* Zigbee Cluster Library 4.7.2.1.1: MeasuredValue = 100x water content in % */
#define ZCL_HUMIDITY_MEMASUREMENT_MEASURED_VALUE_MULTIPLIER 100

LOG_MODULE_DECLARE(app, LOG_LEVEL_INF);

/* Store all attribute values in one place */
struct weather_attributes_s {
	int16_t temperature;
	int16_t pressure;
	int16_t humidity;
};
struct weather_attributes_s weather_attributes;

/* Convert sensor measurements to attribute values, as specified by ZCL */
static void calculate_attributes(void)
{
	weather_attributes.temperature =
		(int16_t)(sensor_get_temperature() *
			  ZCL_TEMPERATURE_MEMASUREMENT_MEASURED_VALUE_MULTIPLIER);
	weather_attributes.pressure =
		(int16_t)(sensor_get_pressure() *
			  ZCL_PRESSURE_MEMASUREMENT_MEASURED_VALUE_MULTIPLIER);
	weather_attributes.humidity =
		(int16_t)(sensor_get_humidity() *
			  ZCL_HUMIDITY_MEMASUREMENT_MEASURED_VALUE_MULTIPLIER);
}

void weather_station_init(void)
{
	weather_attributes.temperature = 0;
	weather_attributes.pressure = 0;
	weather_attributes.humidity = 0;

	sensor_init();
}

void weather_station_update_attributes(void)
{
	sensor_update();
	calculate_attributes();

	LOG_INF("Attributes T:%10d      P:%10d       H:%10d",
		weather_attributes.temperature,
		weather_attributes.pressure,
		weather_attributes.humidity);


	zb_zcl_set_attr_val(WEATHER_STATION_ENDPOINT_NB,
			    ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
			    ZB_ZCL_CLUSTER_SERVER_ROLE,
			    ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
			    (zb_uint8_t *)&weather_attributes.temperature,
			    ZB_FALSE);

	zb_zcl_set_attr_val(WEATHER_STATION_ENDPOINT_NB,
			    ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT,
			    ZB_ZCL_CLUSTER_SERVER_ROLE,
			    ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_VALUE_ID,
			    (zb_uint8_t *)&weather_attributes.pressure,
			    ZB_FALSE);

	zb_zcl_set_attr_val(WEATHER_STATION_ENDPOINT_NB,
			    ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
			    ZB_ZCL_CLUSTER_SERVER_ROLE,
			    ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID,
			    (zb_uint8_t *)&weather_attributes.humidity,
			    ZB_FALSE);
}
