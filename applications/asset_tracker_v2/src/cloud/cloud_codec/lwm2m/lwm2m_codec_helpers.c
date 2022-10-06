/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_location.h>
#include <date_time.h>
#include <lwm2m_resource_ids.h>
#include <string.h>
#include <modem/lte_lc.h>

#include "lwm2m_codec_defines.h"
#include "lwm2m_codec_helpers.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cloud_codec_lwm2m_helpers, CONFIG_CLOUD_CODEC_LOG_LEVEL);

/* Some resources does not have designated buffers. Therefore we define those in here. */
static uint8_t bearers[2] = { LTE_FDD_BEARER, NB_IOT_BEARER };
static int battery_voltage;
static int32_t button_ts;
static int64_t current_time;

#if defined(CONFIG_CLOUD_CODEC_LWM2M_SENSORS)
/* Timestamps, minimum, and maximum values for the BME680 present on the Thingy:91. */
static double temp_min_range_val = BME680_TEMP_MIN_RANGE_VALUE;
static double temp_max_range_val = BME680_TEMP_MAX_RANGE_VALUE;
static double humid_min_range_val = BME680_HUMID_MIN_RANGE_VALUE;
static double humid_max_range_val = BME680_HUMID_MAX_RANGE_VALUE;
static double pressure_min_range_val = BME680_PRESSURE_MIN_RANGE_VALUE;
static double pressure_max_range_val = BME680_PRESSURE_MAX_RANGE_VALUE;
static int32_t pressure_ts;
static int32_t temperature_ts;
static int32_t humidity_ts;

static int lwm2m_codec_helpers_set_sensor_ranges(void)
{
	int err;

	/* Temperature object. */
	err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MIN_RANGE_VALUE_RID),
				     &temp_min_range_val);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MAX_RANGE_VALUE_RID),
				     &temp_max_range_val);
	if (err) {
		return err;
	}

	/* Humidity object. */
	err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
						MIN_RANGE_VALUE_RID),
				     &humid_min_range_val);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
						MAX_RANGE_VALUE_RID),
				     &humid_max_range_val);
	if (err) {
		return err;
	}

	/* Pressure object. */
	err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, MIN_RANGE_VALUE_RID),
				     &pressure_min_range_val);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, MAX_RANGE_VALUE_RID),
				     &pressure_max_range_val);
	if (err) {
		return err;
	}

	return 0;
}
#endif /* CONFIG_CLOUD_CODEC_LWM2M_SENSORS */

int lwm2m_codec_helpers_create_objects_and_resources(void)
{
	int err;

#if defined(CONFIG_CLOUD_CODEC_LWM2M_SENSORS)
	err = lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0));
	if (err) {
		return err;
	}

	err = lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0));
	if (err) {
		return err;
	}

	err = lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0));
	if (err) {
		return err;
	}
#endif /* CONFIG_CLOUD_CODEC_LWM2M_SENSORS */

	err = lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
						      BUTTON1_OBJ_INST_ID));
	if (err) {
		return err;
	}

	if (CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT == 2) {
		err = lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
							BUTTON2_OBJ_INST_ID));
		if (err) {
			return err;
		}
	}

	err = lwm2m_engine_create_res_inst(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						      AVAIL_NETWORK_BEARER_ID, 0));
	if (err) {
		return err;
	}

	err = lwm2m_engine_create_res_inst(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						      AVAIL_NETWORK_BEARER_ID, 1));
	if (err) {
		return err;
	}

	err = lwm2m_engine_create_res_inst(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						      IP_ADDRESSES, 0));
	if (err) {
		return err;
	}

	err = lwm2m_engine_create_res_inst(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						      APN, 0));
	if (err) {
		return err;
	}

	err = lwm2m_engine_create_res_inst(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
						      POWER_SOURCE_VOLTAGE_RID, 0));
	if (err) {
		return err;
	}

	return 0;
}

static int lwm2m_codec_helpers_set_callback_for_config_object(lwm2m_engine_set_data_cb_t callback)
{
	int err;

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   PASSIVE_MODE_RID),
							callback);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   GNSS_TIMEOUT_RID),
							callback);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   ACTIVE_WAIT_TIMEOUT_RID),
							callback);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   MOVEMENT_RESOLUTION_RID),
							callback);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   MOVEMENT_TIMEOUT_RID),
							callback);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
			   ACCELEROMETER_ACT_THRESHOLD_RID),
		callback);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
			   ACCELEROMETER_INACT_THRESHOLD_RID),
		callback);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
			   ACCELEROMETER_INACT_TIMEOUT_RID),
		callback);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   GNSS_ENABLE_RID),
							callback);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
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

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
						  POWER_SOURCE_VOLTAGE_RID),
				       &battery_voltage, sizeof(battery_voltage),
				       sizeof(battery_voltage), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

#if defined(CONFIG_CLOUD_CODEC_LWM2M_SENSORS)
	err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
				       &pressure_ts, sizeof(pressure_ts),
				       sizeof(pressure_ts), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
				       &temperature_ts, sizeof(temperature_ts),
				       sizeof(temperature_ts), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
						  TIMESTAMP_RID),
				       &humidity_ts, sizeof(humidity_ts),
				       sizeof(humidity_ts), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_UNITS_RID),
				       BME680_TEMP_UNIT, sizeof(BME680_TEMP_UNIT),
				       sizeof(BME680_TEMP_UNIT), LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}
	err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
						  SENSOR_UNITS_RID),
				       BME680_HUMID_UNIT, sizeof(BME680_HUMID_UNIT),
				       sizeof(BME680_HUMID_UNIT), LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_UNITS_RID),
				       BME680_PRESSURE_UNIT, sizeof(BME680_PRESSURE_UNIT),
				       sizeof(BME680_PRESSURE_UNIT), LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_codec_helpers_set_sensor_ranges();
	if (err) {
		return err;
	}

#endif /* CONFIG_CLOUD_CODEC_LWM2M_SENSORS */

	err = lwm2m_engine_set_res_buf(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, APPLICATION_TYPE_RID),
		BUTTON1_APP_NAME, sizeof(BUTTON1_APP_NAME), sizeof(BUTTON1_APP_NAME),
		LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, TIMESTAMP_RID),
		&button_ts, sizeof(button_ts), sizeof(button_ts), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	if (CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT == 2) {
		err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
								BUTTON2_OBJ_INST_ID,
								APPLICATION_TYPE_RID),
						BUTTON2_APP_NAME, sizeof(BUTTON2_APP_NAME),
						sizeof(BUTTON2_APP_NAME), LWM2M_RES_DATA_FLAG_RO);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
								BUTTON2_OBJ_INST_ID, TIMESTAMP_RID),
						&button_ts,
						sizeof(button_ts),
						sizeof(button_ts),
						LWM2M_RES_DATA_FLAG_RW);
		if (err) {
			return err;
		}
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
						  CURRENT_TIME_RID),
				       &current_time, sizeof(current_time), sizeof(current_time),
				       LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_setup_configuration_object(struct cloud_data_cfg *cfg,
						   lwm2m_engine_set_data_cb_t callback)
{
	int err;

	err = lwm2m_engine_set_bool(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, PASSIVE_MODE_RID),
				    !cfg->active_mode);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, GNSS_TIMEOUT_RID),
				   cfg->gnss_timeout);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID),
				   cfg->active_wait_timeout);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID),
				   cfg->movement_resolution);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID),
				   cfg->movement_timeout);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
				     ACCELEROMETER_ACT_THRESHOLD_RID),
				     &cfg->accelerometer_activity_threshold);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
				     ACCELEROMETER_INACT_THRESHOLD_RID),
				     &cfg->accelerometer_inactivity_threshold);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
				     ACCELEROMETER_INACT_TIMEOUT_RID),
				     &cfg->accelerometer_inactivity_timeout);
	if (err) {
		return err;
	}

	/* If the GNSS and Neighbor cell entry in the No data structure is set, its disabled in the
	 * corresponding object.
	 */
	err = lwm2m_engine_set_bool(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
					       GNSS_ENABLE_RID),
				    !cfg->no_data.gnss);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_bool(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
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
	err = lwm2m_engine_get_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, GNSS_TIMEOUT_RID),
				   &cfg->gnss_timeout);
	if (err) {
		LOG_ERR("Failed getting GNSS resource value.");
	}

	err = lwm2m_engine_get_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID),
				   &cfg->active_wait_timeout);
	if (err) {
		LOG_ERR("Failed getting Active wait timeout resource value.");
	}

	err = lwm2m_engine_get_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID),
				   &cfg->movement_resolution);
	if (err) {
		LOG_ERR("Failed getting Movement resolution resource value.");
	}

	err = lwm2m_engine_get_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID),
				   &cfg->movement_timeout);
	if (err) {
		LOG_ERR("Failed getting Movement timeout resource value.");
	}

	err = lwm2m_engine_get_float(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
				     ACCELEROMETER_ACT_THRESHOLD_RID),
				     &cfg->accelerometer_activity_threshold);
	if (err) {
		LOG_ERR("Failed getting Accelerometer activity threshold resource value.");
	}

	err = lwm2m_engine_get_float(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
				     ACCELEROMETER_INACT_THRESHOLD_RID),
				     &cfg->accelerometer_inactivity_threshold);
	if (err) {
		LOG_ERR("Failed getting Accelerometer inactivity threshold resource value.");
	}

	err = lwm2m_engine_get_float(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
				     ACCELEROMETER_INACT_TIMEOUT_RID),
				     &cfg->accelerometer_inactivity_timeout);
	if (err) {
		LOG_ERR("Failed getting Accelerometer inactivity timeout resource value.");
	}

	/* If the GNSS and neighbor cell entry in the No data structure is set, its disabled in the
	 * corresponding object.
	 */
	bool gnss_enable_temp;
	bool ncell_enable_temp;
	bool passive_mode_temp;

	err = lwm2m_engine_get_bool(LWM2M_PATH(CONFIGURATION_OBJECT_ID,
					       0,
					       PASSIVE_MODE_RID),
					       &passive_mode_temp);
	if (err) {
		LOG_ERR("Failed getting Passive mode resource value.");
	}

	cfg->active_mode = (passive_mode_temp == true) ? false : true;

	err = lwm2m_engine_get_bool(LWM2M_PATH(CONFIGURATION_OBJECT_ID,
					       0,
					       GNSS_ENABLE_RID),
					       &gnss_enable_temp);
	if (err) {
		LOG_ERR("Failed getting GNSS enable resource value.");
	}


	err = lwm2m_engine_get_bool(LWM2M_PATH(CONFIGURATION_OBJECT_ID,
					       0,
					       NEIGHBOR_CELL_ENABLE_RID),
					       &ncell_enable_temp);
	if (err) {
		LOG_ERR("Failed getting Neighbor cell enable resource value.");
	}

	cfg->no_data.gnss = (gnss_enable_temp == true) ? false : true;
	cfg->no_data.neighbor_cell = (ncell_enable_temp == true) ? false : true;

	return 0;
}

int lwm2m_codec_helpers_set_agps_data(struct cloud_data_agps_request *agps_request)
{
	int err;
	uint32_t mask = 0;

	if (!agps_request->queued) {
		return -ENODATA;
	}

	if (agps_request->request.data_flags & NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_UTC;
	}

	if (agps_request->request.sv_mask_ephe) {
		mask |= LOCATION_ASSIST_NEEDS_EPHEMERIES;
	}

	if (agps_request->request.sv_mask_alm) {
		mask |= LOCATION_ASSIST_NEEDS_ALMANAC;
	}

	if (agps_request->request.data_flags & NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_KLOBUCHAR;
	}

	if (agps_request->request.data_flags & NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_NEQUICK;
	}

	if (agps_request->request.data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_TOW;
		mask |= LOCATION_ASSIST_NEEDS_CLOCK;
	}

	if (agps_request->request.data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_LOCATION;
	}

	if (agps_request->request.data_flags & NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_INTEGRITY;
	}

	/* Sets A-GPS mask and type of request. */
	location_assist_agps_request_set(mask);

	err = lwm2m_engine_set_u32(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					      CELLID),
				   agps_request->cell);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					      SMNC),
				   agps_request->mnc);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					      SMCC),
				   agps_request->mcc);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					      LAC),
				   agps_request->area);
	if (err) {
		return err;
	}

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
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, LATITUDE_RID),
					&gnss->pvt.lat);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, LONGITUDE_RID),
					&gnss->pvt.longi);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, ALTITUDE_RID),
					&alt);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, RADIUS_RID),
					&acc);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, SPEED_RID),
					&spd);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_s32(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0,
						LOCATION_TIMESTAMP_RID),
					(int32_t)(gnss->gnss_ts / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_set_modem_dynamic_data(struct cloud_data_modem_dynamic *modem_dynamic)
{
	int err;

	if (!modem_dynamic->queued) {
		return -ENODATA;
	}

	if (modem_dynamic->nw_mode == LTE_LC_LTE_MODE_LTEM) {
		err = lwm2m_engine_set_u8(LWM2M_PATH(
						LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						NETWORK_BEARER_ID),
						LTE_FDD_BEARER);
		if (err) {
			return err;
		}
	} else if (modem_dynamic->nw_mode == LTE_LC_LTE_MODE_NBIOT) {
		err = lwm2m_engine_set_u8(LWM2M_PATH(
						LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						NETWORK_BEARER_ID),
						NB_IOT_BEARER);
		if (err) {
			return err;
		}
	} else {
		LOG_ERR("Network bearer not set.");
		return -EINVAL;
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID,
							0, AVAIL_NETWORK_BEARER_ID, 0),
					&bearers[0], sizeof(bearers[0]), sizeof(bearers[0]),
					LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID,
							0, AVAIL_NETWORK_BEARER_ID, 1),
					&bearers[1], sizeof(bearers[1]), sizeof(bearers[1]),
					LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID,
							0, IP_ADDRESSES, 0),
					modem_dynamic->ip, sizeof(modem_dynamic->ip),
					sizeof(modem_dynamic->ip), LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID,
							0, APN, 0),
					modem_dynamic->apn, sizeof(modem_dynamic->apn),
					sizeof(modem_dynamic->apn), LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_s8(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						RSS),
					modem_dynamic->rsrp);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u32(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						CELLID),
					modem_dynamic->cell);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						SMNC),
					modem_dynamic->mnc);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						SMCC),
					modem_dynamic->mcc);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						LAC),
					modem_dynamic->area);
	if (err) {
		return err;
	}

	err = date_time_now(&current_time);
	if (err) {
		LOG_WRN("Failed getting current time, error: %d", err);
		return err;
	}

	err = lwm2m_engine_set_s32(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, CURRENT_TIME_RID),
					(int32_t)(current_time / MSEC_PER_SEC));
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

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
							MODEL_NUMBER_RID),
					CONFIG_BOARD,
					sizeof(CONFIG_BOARD),
					sizeof(CONFIG_BOARD),
					LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
							HARDWARE_VERSION_RID),
					CONFIG_SOC,
					sizeof(CONFIG_SOC),
					sizeof(CONFIG_SOC),
					LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
							MANUFACTURER_RID),
					CONFIG_CLOUD_CODEC_MANUFACTURER,
					sizeof(CONFIG_CLOUD_CODEC_MANUFACTURER),
					sizeof(CONFIG_CLOUD_CODEC_MANUFACTURER),
					LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, FIRMWARE_VERSION_RID),
		modem_static->appv, sizeof(modem_static->appv),
		sizeof(modem_static->appv), LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID),
		modem_static->fw, sizeof(modem_static->fw),
		sizeof(modem_static->fw), LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_res_buf(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_SERIAL_NUMBER_ID),
		modem_static->imei, sizeof(modem_static->imei),
		sizeof(modem_static->imei), LWM2M_RES_DATA_FLAG_RO);
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

	err = lwm2m_engine_set_s32(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
					POWER_SOURCE_VOLTAGE_RID),
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
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	err = lwm2m_engine_set_s32(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0,
						TIMESTAMP_RID),
					(int32_t)(sensor->env_ts / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_s32(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
						TIMESTAMP_RID),
					(int32_t)(sensor->env_ts / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_s32(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0,
						TIMESTAMP_RID),
					(int32_t)(sensor->env_ts / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0,
						SENSOR_VALUE_RID),
					&sensor->temperature);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
						SENSOR_VALUE_RID),
					&sensor->humidity);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0,
						SENSOR_VALUE_RID),
					&sensor->pressure);
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_set_user_interface_data(struct cloud_data_ui *user_interface)
{
	int err, len;

	if (!user_interface->queued) {
		return -ENODATA;
	}

	if ((user_interface->btn == 2) && (CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT > 2)) {
		return -EINVAL;
	}

	/* Create object instance according to the button number. */
	char digital_input_path[12] = "\0", timestamp_path[12] = "\0";

	len = snprintk(digital_input_path,
		       sizeof(digital_input_path),
		       "%d/%d/%d",
		       IPSO_OBJECT_PUSH_BUTTON_ID,
		       (user_interface->btn - 1),
		       DIGITAL_INPUT_STATE_RID);
	if ((len < 0) || (len >= sizeof(digital_input_path))) {
		return -ERANGE;
	}

	len = snprintk(timestamp_path,
		       sizeof(timestamp_path),
		       "%d/%d/%d",
		       IPSO_OBJECT_PUSH_BUTTON_ID,
		       (user_interface->btn - 1),
		       TIMESTAMP_RID);
	if ((len < 0) || (len >= sizeof(timestamp_path))) {
		return -ERANGE;
	}

	err = date_time_uptime_to_unix_time_ms(&user_interface->btn_ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	/* Toggle digital input state of the toggled button to increment
	 * the digital input counter. This is done automatically in the push button object
	 * abstraction in the LwM2M Zephyr API.
	 */
	err = lwm2m_engine_set_bool(digital_input_path, true);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_bool(digital_input_path, false);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_s32(timestamp_path,
				   (int32_t)(user_interface->btn_ts / MSEC_PER_SEC));
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
	if (err == -ENODATA) {
		LOG_DBG("No neighboring cells available, using single cell location request");

		/* No neighboring cells found, set single cell request. */
		err = lwm2m_engine_set_s32(LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0,
						      ASSIST_TYPE_RID),
					   ASSISTANCE_REQUEST_TYPE_SINGLECELL_REQUEST);
		if (err) {
			return err;
		}

	} else if (err == 0) {

		/* Neighboring cells found, set multicell request. */
		err = lwm2m_engine_set_s32(LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0,
							ASSIST_TYPE_RID),
					   ASSISTANCE_REQUEST_TYPE_MULTICELL_REQUEST);
		if (err) {
			return err;
		}

	} else {
		LOG_ERR("lwm2m_update_signal_meas_objects, error: %d", err);
		return err;
	}

	err = lwm2m_engine_set_s8(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					     RSS),
				  neighbor_cells->cell_data.current_cell.rsrp);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u32(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					      CELLID),
				   neighbor_cells->cell_data.current_cell.id);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					      SMNC),
				   neighbor_cells->cell_data.current_cell.mnc);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					      SMCC),
				   neighbor_cells->cell_data.current_cell.mcc);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					      LAC),
				   neighbor_cells->cell_data.current_cell.tac);
	if (err) {
		return err;
	}

	return 0;
}

int lwm2m_codec_helpers_object_path_list_add(struct cloud_codec_data *output,
					     const char * const path[],
					     size_t path_size)
{
	bool path_added = false;
	uint8_t j = 0;

	if (output == NULL || path == NULL || path_size == 0) {
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(output->paths); i++) {
		if (strcmp(output->paths[i], "\0") == 0) {

			if ((sizeof(output->paths[i]) - 1) < strlen(path[j])) {
				LOG_ERR("Path entry is too long");
				return -EMSGSIZE;
			}

			strncpy(output->paths[i], path[j], sizeof(output->paths[i]) - 1);

			output->paths[i][sizeof(output->paths[i]) - 1] = '\0';
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
		LOG_ERR("Current object path list is full, or not all entries in the incoming "
			"path[] list was added");
		return -ENOMEM;
	}

	return 0;
}
