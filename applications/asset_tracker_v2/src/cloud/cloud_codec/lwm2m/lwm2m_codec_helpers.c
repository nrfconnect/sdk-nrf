/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_location.h>
#include <date_time.h>
#include <lwm2m_resource_ids.h>
#include <string.h>
#include <modem/lte_lc.h>

#include "lwm2m_codec_defines.h"
#include "lwm2m_codec_helpers.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lwm2m_codec_helpers, CONFIG_CLOUD_CODEC_LOG_LEVEL);

/* Some resources does not have designated buffers. Therefore we define those in here. */
static uint8_t bearers[2] = { LTE_FDD_BEARER, NB_IOT_BEARER };
static int battery_voltage;
static time_t button_ts;

#if defined(CONFIG_CLOUD_CODEC_LWM2M_THINGY91_SENSORS)
/* Timestamps, minimum, and maximum values for the BME680 present on the Thingy:91. */
static double temp_min_range_val = BME680_TEMP_MIN_RANGE_VALUE;
static double temp_max_range_val = BME680_TEMP_MAX_RANGE_VALUE;
static double humid_min_range_val = BME680_HUMID_MIN_RANGE_VALUE;
static double humid_max_range_val = BME680_HUMID_MAX_RANGE_VALUE;
static double pressure_min_range_val = BME680_PRESSURE_MIN_RANGE_VALUE;
static double pressure_max_range_val = BME680_PRESSURE_MAX_RANGE_VALUE;
static time_t pressure_ts;
static time_t temperature_ts;
static time_t humidity_ts;

static int lwm2m_codec_helpers_set_sensor_ranges(void)
{
	int err;

	/* Temperature object. */
	err = lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MIN_RANGE_VALUE_RID),
			    temp_min_range_val);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MAX_RANGE_VALUE_RID),
			    temp_max_range_val);
	if (err) {
		return err;
	}

	/* Humidity object. */
	err = lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
				       MIN_RANGE_VALUE_RID),
			    humid_min_range_val);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
				       MAX_RANGE_VALUE_RID),
			    humid_max_range_val);
	if (err) {
		return err;
	}

	/* Pressure object. */
	err = lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, MIN_RANGE_VALUE_RID),
			    pressure_min_range_val);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, MAX_RANGE_VALUE_RID),
			    pressure_max_range_val);
	if (err) {
		return err;
	}

	return 0;
}
#endif /* CONFIG_CLOUD_CODEC_LWM2M_THINGY91_SENSORS */

int lwm2m_codec_helpers_create_objects_and_resources(void)
{
	int err;

#if defined(CONFIG_CLOUD_CODEC_LWM2M_THINGY91_SENSORS)
	err = lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0));
	if (err) {
		return err;
	}

	err = lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0));
	if (err) {
		return err;
	}

	err = lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0));
	if (err) {
		return err;
	}
#endif /* CONFIG_CLOUD_CODEC_LWM2M_THINGY91_SENSORS */

	err = lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID));
	if (err) {
		return err;
	}

	if (CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT == 2) {
		err = lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID,
							  BUTTON2_OBJ_INST_ID));
		if (err) {
			return err;
		}
	}

	err = lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					       AVAIL_NETWORK_BEARER_ID, 0));
	if (err) {
		return err;
	}

	err = lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					       AVAIL_NETWORK_BEARER_ID, 1));
	if (err) {
		return err;
	}

	err = lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					       IP_ADDRESSES, 0));
	if (err) {
		return err;
	}

	err = lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					       APN, 0));
	if (err) {
		return err;
	}

	err = lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0,
					       POWER_SOURCE_VOLTAGE_RID, 0));
	if (err) {
		return err;
	}

	return 0;
}

static int lwm2m_codec_helpers_set_callback_for_config_object(lwm2m_engine_set_data_cb_t callback)
{
	int err;

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
							    PASSIVE_MODE_RID),
						 callback);
	if (err) {
		return err;
	}

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
							    LOCATION_TIMEOUT_RID),
						 callback);
	if (err) {
		return err;
	}

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
							    ACTIVE_WAIT_TIMEOUT_RID),
						 callback);
	if (err) {
		return err;
	}

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
							    MOVEMENT_RESOLUTION_RID),
						 callback);
	if (err) {
		return err;
	}

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
							    MOVEMENT_TIMEOUT_RID),
						 callback);
	if (err) {
		return err;
	}

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
							    ACCELEROMETER_ACT_THRESHOLD_RID),
						 callback);
	if (err) {
		return err;
	}

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
							    ACCELEROMETER_INACT_THRESHOLD_RID),
						 callback);
	if (err) {
		return err;
	}

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
							    ACCELEROMETER_INACT_TIMEOUT_RID),
						 callback);
	if (err) {
		return err;
	}

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
							    GNSS_ENABLE_RID),
						 callback);
	if (err) {
		return err;
	}

	err = lwm2m_register_post_write_callback(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
							    NEIGHBOR_CELL_ENABLE_RID),
						 callback);
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_setup_resources(void)
{
	int err;

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0,
					   POWER_SOURCE_VOLTAGE_RID),
				&battery_voltage, sizeof(battery_voltage),
				sizeof(battery_voltage), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

#if defined(CONFIG_CLOUD_CODEC_LWM2M_THINGY91_SENSORS)
	err = lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
				&pressure_ts, sizeof(pressure_ts),
				sizeof(pressure_ts), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
				&temperature_ts, sizeof(temperature_ts),
				sizeof(temperature_ts), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
				&humidity_ts, sizeof(humidity_ts),
				sizeof(humidity_ts), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_UNITS_RID),
				BME680_TEMP_UNIT, (uint16_t)strlen(BME680_TEMP_UNIT),
				(uint16_t)strlen(BME680_TEMP_UNIT), LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}
	err = lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
					   SENSOR_UNITS_RID),
				BME680_HUMID_UNIT, (uint16_t)strlen(BME680_HUMID_UNIT),
				(uint16_t)strlen(BME680_HUMID_UNIT),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_UNITS_RID),
				BME680_PRESSURE_UNIT, (uint16_t)strlen(BME680_PRESSURE_UNIT),
				(uint16_t)strlen(BME680_PRESSURE_UNIT),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_codec_helpers_set_sensor_ranges();
	if (err) {
		return err;
	}
#endif /* CONFIG_CLOUD_CODEC_LWM2M_THINGY91_SENSORS */

	err = lwm2m_set_res_buf(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, APPLICATION_TYPE_RID),
		BUTTON1_APP_NAME, (uint16_t)strlen(BUTTON1_APP_NAME),
		(uint16_t)strlen(BUTTON1_APP_NAME), LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, TIMESTAMP_RID),
		&button_ts, sizeof(button_ts), sizeof(button_ts), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	if (CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT == 2) {
		err = lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID,
							  BUTTON2_OBJ_INST_ID,
							  APPLICATION_TYPE_RID),
					       BUTTON2_APP_NAME, (uint16_t)strlen(BUTTON2_APP_NAME),
					       (uint16_t)strlen(BUTTON2_APP_NAME),
					       LWM2M_RES_DATA_FLAG_RO);
		if (err) {
			return err;
		}

		err = lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID,
							  BUTTON2_OBJ_INST_ID, TIMESTAMP_RID),
					       &button_ts,
					       sizeof(button_ts),
					       sizeof(button_ts),
					       LWM2M_RES_DATA_FLAG_RW);
		if (err) {
			return err;
		}
	}

	return 0;
}

int lwm2m_codec_helpers_setup_configuration_object(struct cloud_data_cfg *cfg,
						   lwm2m_engine_set_data_cb_t callback)
{
	int err;

	err = lwm2m_set_bool(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, PASSIVE_MODE_RID),
			     !cfg->active_mode);
	if (err) {
		return err;
	}

	err = lwm2m_set_s32(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, LOCATION_TIMEOUT_RID),
			    cfg->location_timeout);
	if (err) {
		return err;
	}

	err = lwm2m_set_s32(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID),
			    cfg->active_wait_timeout);
	if (err) {
		return err;
	}

	err = lwm2m_set_s32(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID),
			    cfg->movement_resolution);
	if (err) {
		return err;
	}

	err = lwm2m_set_s32(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID),
			    cfg->movement_timeout);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
			    ACCELEROMETER_ACT_THRESHOLD_RID),
			    cfg->accelerometer_activity_threshold);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
			    ACCELEROMETER_INACT_THRESHOLD_RID),
			    cfg->accelerometer_inactivity_threshold);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
			    ACCELEROMETER_INACT_TIMEOUT_RID),
			    cfg->accelerometer_inactivity_timeout);
	if (err) {
		return err;
	}

	/* If the GNSS and Neighbor cell entry in the No data structure is set, its disabled in the
	 * corresponding object.
	 */
	err = lwm2m_set_bool(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
					GNSS_ENABLE_RID),
			     !cfg->no_data.gnss);
	if (err) {
		return err;
	}

	err = lwm2m_set_bool(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
					NEIGHBOR_CELL_ENABLE_RID),
			     !cfg->no_data.neighbor_cell);
	if (err) {
		return err;
	}

	err = lwm2m_codec_helpers_set_callback_for_config_object(callback);
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_get_configuration_object(struct cloud_data_cfg *cfg)
{
	int err;

	/* There has been a configuration update. Send callback to application with the latest
	 * state of the configuration.
	 */
	err = lwm2m_get_s32(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, LOCATION_TIMEOUT_RID),
			    &cfg->location_timeout);
	if (err) {
		return err;
	}

	err = lwm2m_get_s32(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID),
			    &cfg->active_wait_timeout);
	if (err) {
		return err;
	}

	err = lwm2m_get_s32(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID),
			    &cfg->movement_resolution);
	if (err) {
		return err;
	}

	err = lwm2m_get_s32(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID),
			    &cfg->movement_timeout);
	if (err) {
		return err;
	}

	err = lwm2m_get_f64(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
			    ACCELEROMETER_ACT_THRESHOLD_RID),
			    &cfg->accelerometer_activity_threshold);
	if (err) {
		return err;
	}

	err = lwm2m_get_f64(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
			    ACCELEROMETER_INACT_THRESHOLD_RID),
			    &cfg->accelerometer_inactivity_threshold);
	if (err) {
		return err;
	}

	err = lwm2m_get_f64(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0,
			    ACCELEROMETER_INACT_TIMEOUT_RID),
			    &cfg->accelerometer_inactivity_timeout);
	if (err) {
		return err;
	}

	/* If the GNSS and neighbor cell entry in the No data structure is set, its disabled in the
	 * corresponding object.
	 */
	bool gnss_enable_temp;
	bool ncell_enable_temp;
	bool passive_mode_temp;

	err = lwm2m_get_bool(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID,
					0,
					PASSIVE_MODE_RID),
					&passive_mode_temp);
	if (err) {
		return err;
	}

	cfg->active_mode = (passive_mode_temp == true) ? false : true;

	err = lwm2m_get_bool(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID,
					0,
					GNSS_ENABLE_RID),
					&gnss_enable_temp);
	if (err) {
		return err;
	}

	err = lwm2m_get_bool(&LWM2M_OBJ(CONFIGURATION_OBJECT_ID,
					0,
					NEIGHBOR_CELL_ENABLE_RID),
					&ncell_enable_temp);
	if (err) {
		return err;
	}

	cfg->no_data.gnss = (gnss_enable_temp == true) ? false : true;
	cfg->no_data.neighbor_cell = (ncell_enable_temp == true) ? false : true;

	return 0;
}

int lwm2m_codec_helpers_set_agnss_data(struct cloud_data_agnss_request *agnss_request)
{
	int err;

	if (!agnss_request->queued) {
		return -ENODATA;
	}

	err = location_assistance_agnss_set_mask(&agnss_request->request);
	if (err) {
		return err;
	}

	/* Disable filtered A-GNSS. */
	location_assist_agnss_set_elevation_mask(-1);

	err = lwm2m_set_u32(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
			    agnss_request->cell);
	if (err) {
		return err;
	}

	err = lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
			    (uint16_t)agnss_request->mnc);
	if (err) {
		return err;
	}

	err = lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
			    (uint16_t)agnss_request->mcc);
	if (err) {
		return err;
	}

	err = lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC),
			    (uint16_t)agnss_request->area);
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_set_pgps_data(struct cloud_data_pgps_request *pgps_request)
{
	int err;

	if (!pgps_request->queued) {
		return -ENODATA;
	}

	err = location_assist_pgps_set_prediction_count(pgps_request->count);
	if (err) {
		return err;
	}

	err = location_assist_pgps_set_prediction_interval(pgps_request->interval);
	if (err) {
		return err;
	}

	err = location_assist_pgps_set_start_time(pgps_request->time);
	if (err) {
		return err;
	}

	location_assist_pgps_set_start_gps_day(pgps_request->day);
	return 0;
}

int lwm2m_codec_helpers_set_gnss_data(struct cloud_data_gnss *gnss)
{
	int err;

	if (!gnss->queued) {
		return -ENODATA;
	}

	double alt = (double) gnss->pvt.alt;
	double acc = (double) gnss->pvt.acc;
	double spd = (double) gnss->pvt.spd;

	err = date_time_uptime_to_unix_time_ms(&gnss->gnss_ts);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LATITUDE_RID),
			    gnss->pvt.lat);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LONGITUDE_RID),
			    gnss->pvt.longi);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, ALTITUDE_RID), alt);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, RADIUS_RID), acc);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, SPEED_RID), spd);
	if (err) {
		return err;
	}

	err = lwm2m_set_time(&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LOCATION_TIMESTAMP_RID),
			     (gnss->gnss_ts / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_set_modem_dynamic_data(struct cloud_data_modem_dynamic *modem_dynamic)
{
	int err;
	int64_t current_time;

	if (!modem_dynamic->queued) {
		return -ENODATA;
	}

	if (modem_dynamic->nw_mode == LTE_LC_LTE_MODE_LTEM) {
		err = lwm2m_set_u8(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					      NETWORK_BEARER_ID), LTE_FDD_BEARER);
		if (err) {
			return err;
		}
	} else if (modem_dynamic->nw_mode == LTE_LC_LTE_MODE_NBIOT) {
		err = lwm2m_set_u8(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					      NETWORK_BEARER_ID), NB_IOT_BEARER);
		if (err) {
			return err;
		}
	} else {
		return -EINVAL;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID,
					   0, AVAIL_NETWORK_BEARER_ID, 0),
				&bearers[0], sizeof(bearers[0]), sizeof(bearers[0]),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID,
					   0, AVAIL_NETWORK_BEARER_ID, 1),
				&bearers[1], sizeof(bearers[1]), sizeof(bearers[1]),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID,
					   0, IP_ADDRESSES, 0),
				modem_dynamic->ip, (uint16_t)strlen(modem_dynamic->ip),
				(uint16_t)strlen(modem_dynamic->ip),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID,
					   0, APN, 0),
				modem_dynamic->apn, (uint16_t)strlen(modem_dynamic->apn),
				(uint16_t)strlen(modem_dynamic->apn),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_s8(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, RSS),
			   (int8_t)modem_dynamic->rsrp);
	if (err) {
		return err;
	}

	err = lwm2m_set_u32(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
			    modem_dynamic->cell);
	if (err) {
		return err;
	}

	err = lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
			    modem_dynamic->mnc);
	if (err) {
		return err;
	}

	err = lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
			    modem_dynamic->mcc);
	if (err) {
		return err;
	}

	err = lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC),
			    modem_dynamic->area);
	if (err) {
		return err;
	}

	err = date_time_now(&current_time);
	if (err) {
		return err;
	}

	err = lwm2m_set_time(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, CURRENT_TIME_RID),
			     (current_time / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_set_modem_static_data(struct cloud_data_modem_static *modem_static)
{
	int err;

	if (!modem_static->queued) {
		return -ENODATA;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MODEL_NUMBER_RID),
				CONFIG_BOARD,
				(uint16_t)strlen(CONFIG_BOARD),
				(uint16_t)strlen(CONFIG_BOARD),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, HARDWARE_VERSION_RID),
				CONFIG_SOC,
				(uint16_t)strlen(CONFIG_SOC),
				(uint16_t)strlen(CONFIG_SOC),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MANUFACTURER_RID),
				CONFIG_CLOUD_CODEC_MANUFACTURER,
				(uint16_t)strlen(CONFIG_CLOUD_CODEC_MANUFACTURER),
				(uint16_t)strlen(CONFIG_CLOUD_CODEC_MANUFACTURER),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, FIRMWARE_VERSION_RID),
				modem_static->appv,
				(uint16_t)strlen(modem_static->appv),
				(uint16_t)strlen(modem_static->appv),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID),
				modem_static->fw,
				(uint16_t)strlen(modem_static->fw),
				(uint16_t)strlen(modem_static->fw),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0,
					   DEVICE_SERIAL_NUMBER_ID),
				modem_static->imei,
				(uint16_t)strlen(modem_static->imei),
				(uint16_t)strlen(modem_static->imei),
				LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_set_battery_data(struct cloud_data_battery *battery)
{
	int err;

	if (!battery->queued) {
		return -ENODATA;
	}

	err = lwm2m_set_s32(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID),
			    battery->bat);
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_set_sensor_data(struct cloud_data_sensors *sensor)
{
	int err;

	if (!sensor->queued) {
		return -ENODATA;
	}

	err = date_time_uptime_to_unix_time_ms(&sensor->env_ts);
	if (err) {
		return err;
	}

	err = lwm2m_set_time(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
			     (int32_t)(sensor->env_ts / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	err = lwm2m_set_time(&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
			     (int32_t)(sensor->env_ts / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	err = lwm2m_set_time(&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
			     (int32_t)(sensor->env_ts / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
			    sensor->temperature);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
				       SENSOR_VALUE_RID),
			    sensor->humidity);
	if (err) {
		return err;
	}

	err = lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID),
			    sensor->pressure);
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_set_user_interface_data(struct cloud_data_ui *user_interface)
{
	int err;

	if (!user_interface->queued) {
		return -ENODATA;
	}

	if ((user_interface->btn == 2) && (CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT > 2)) {
		return -EINVAL;
	}

	err = date_time_uptime_to_unix_time_ms(&user_interface->btn_ts);
	if (err) {
		return err;
	}

	/* Toggle digital input state of the toggled button to increment
	 * the digital input counter. This is done automatically in the push button object
	 * abstraction in the LwM2M Zephyr API.
	 */
	err = lwm2m_set_bool(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, (user_interface->btn - 1),
			     DIGITAL_INPUT_STATE_RID), true);
	if (err) {
		return err;
	}

	err = lwm2m_set_bool(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, (user_interface->btn - 1),
			     DIGITAL_INPUT_STATE_RID), false);
	if (err) {
		return err;
	}

	err = lwm2m_set_time(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, (user_interface->btn - 1),
			     TIMESTAMP_RID), (user_interface->btn_ts / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_set_neighbor_cell_data(struct cloud_data_neighbor_cells *neighbor_cells)
{
	int err;
	struct lte_lc_cells_info *cells = &neighbor_cells->cell_data;

	if (!neighbor_cells->queued) {
		return -ENODATA;
	}

	cells->neighbor_cells = neighbor_cells->neighbor_cells;
	cells->ncells_count = neighbor_cells->cell_data.ncells_count;

	err = lwm2m_update_signal_meas_objects((const struct lte_lc_cells_info *)cells);
	if (err) {
		return err;
	}

	err = lwm2m_set_s8(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, RSS),
			   (int8_t)neighbor_cells->cell_data.current_cell.rsrp);
	if (err) {
		return err;
	}

	err = lwm2m_set_u32(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
			    neighbor_cells->cell_data.current_cell.id);
	if (err) {
		return err;
	}

	err = lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
			    (uint16_t)neighbor_cells->cell_data.current_cell.mnc);
	if (err) {
		return err;
	}

	err = lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
			    (uint16_t)neighbor_cells->cell_data.current_cell.mcc);
	if (err) {
		return err;
	}

	err = lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC),
			    (uint16_t)neighbor_cells->cell_data.current_cell.tac);
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_object_path_list_add(struct cloud_codec_data *output,
					     const struct lwm2m_obj_path path[],
					     size_t path_size)
{
	bool path_added = false;
	uint8_t j = 0;

	if (output == NULL || path == NULL || path_size == 0) {
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(output->paths); i++) {
		if (output->paths[i].level == 0) {

			output->paths[i].obj_id = path[j].obj_id;
			output->paths[i].obj_inst_id = path[j].obj_inst_id;
			output->paths[i].res_id = path[j].res_id;
			output->paths[i].res_inst_id = path[j].res_inst_id;
			output->paths[i].level = path[j].level;

			output->valid_object_paths++;
			path_added = true;
			j++;

			if (j == path_size) {
				break;
			}
		}
	}

	/* This API can be called multiple times to populate the same path buffer. Due to this we
	 * also check if all entries in the incoming path[] list was added.
	 */
	if (!path_added || (j != path_size)) {
		return -ENOMEM;
	}

	return 0;
}
