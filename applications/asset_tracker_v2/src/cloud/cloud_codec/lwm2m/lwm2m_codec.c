/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cloud_codec.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <lwm2m_resource_ids.h>
#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_location.h>
#include <date_time.h>

#include "lwm2m_codec_defines.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cloud_codec, CONFIG_CLOUD_CODEC_LOG_LEVEL);

/* Some resources does not have designated buffers. Therefore we define those in here. */
static uint8_t bearers[2] = { LTE_FDD_BEARER, NB_IOT_BEARER };
static int battery_voltage;
static time_t pressure_ts;
static time_t temperature_ts;
static time_t humidity_ts;
static time_t button_ts;
static int64_t current_time;

/* Minimum and maximum values for the BME680 present on the Thingy:91. */
static double temp_min_range_val = BME680_TEMP_MIN_RANGE_VALUE;
static double temp_max_range_val = BME680_TEMP_MAX_RANGE_VALUE;
static double humid_min_range_val = BME680_HUMID_MIN_RANGE_VALUE;
static double humid_max_range_val = BME680_HUMID_MAX_RANGE_VALUE;
static double pressure_min_range_val = BME680_PRESSURE_MIN_RANGE_VALUE;
static double pressure_max_range_val = BME680_PRESSURE_MAX_RANGE_VALUE;

/* Module event handler.  */
static cloud_codec_evt_handler_t module_evt_handler;

/* Convenience API used to generate path lists with reference to objects.
 * Strings in the path array are expected to be NULL terminated.
 */
static int object_path_list_add(struct cloud_codec_data *output, const char * const path[],
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

/* Function that is called whenever the configuration object is written to. */
static int config_update_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			    uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	/* Because we are dependent on providing all configurations in the
	 * CLOUD_CODEC_EVT_CONFIG_UPDATE event, all configuration is retrieved whenever the
	 * configuration object changes.
	 */
	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(res_id);
	ARG_UNUSED(res_inst_id);
	ARG_UNUSED(data);
	ARG_UNUSED(data_len);
	ARG_UNUSED(last_block);
	ARG_UNUSED(total_size);

	int err;
	struct cloud_data_cfg cfg = { 0 };
	struct cloud_codec_evt evt = {
		.type = CLOUD_CODEC_EVT_CONFIG_UPDATE
	};

	/* There has been a configuration update. Send callback to application with the latest
	 * state of the configuration.
	 */
	err = lwm2m_engine_get_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, LOCATION_TIMEOUT_RID),
				   &cfg.location_timeout);
	if (err) {
		LOG_ERR("Failed getting GNSS resource value.");
	}

	err = lwm2m_engine_get_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID),
				   &cfg.active_wait_timeout);
	if (err) {
		LOG_ERR("Failed getting Active wait timeout resource value.");
	}

	err = lwm2m_engine_get_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID),
				   &cfg.movement_resolution);
	if (err) {
		LOG_ERR("Failed getting Movement resolution resource value.");
	}

	err = lwm2m_engine_get_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID),
				   &cfg.movement_timeout);
	if (err) {
		LOG_ERR("Failed getting Movement timeout resource value.");
	}

	err = lwm2m_engine_get_float(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
				     ACCELEROMETER_ACT_THRESHOLD_RID),
				     &cfg.accelerometer_activity_threshold);
	if (err) {
		LOG_ERR("Failed getting Accelerometer activity threshold resource value.");
	}

	err = lwm2m_engine_get_float(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
				     ACCELEROMETER_INACT_THRESHOLD_RID),
				     &cfg.accelerometer_inactivity_threshold);
	if (err) {
		LOG_ERR("Failed getting Accelerometer inactivity threshold resource value.");
	}

	err = lwm2m_engine_get_float(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
				     ACCELEROMETER_INACT_TIMEOUT_RID),
				     &cfg.accelerometer_inactivity_timeout);
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

	cfg.active_mode = (passive_mode_temp == true) ? false : true;

	err = lwm2m_engine_get_bool(LWM2M_PATH(CONFIGURATION_OBJECT_ID,
					       0,
					       GNSS_ENABLE_RID),
					       &gnss_enable_temp);
	if (err) {
		LOG_ERR("Failed getting GNSS enable resource value.");
	}

	cfg.no_data.gnss = (gnss_enable_temp == true) ? false : true;

	err = lwm2m_engine_get_bool(LWM2M_PATH(CONFIGURATION_OBJECT_ID,
					       0,
					       NEIGHBOR_CELL_ENABLE_RID),
					       &ncell_enable_temp);
	if (err) {
		LOG_ERR("Failed getting Neighbor cell enable resource value.");
	}

	cfg.no_data.neighbor_cell = (ncell_enable_temp == true) ? false : true;

	evt.config_update = cfg;
	module_evt_handler(&evt);

	return 0;
}

int cloud_codec_init(struct cloud_data_cfg *cfg, cloud_codec_evt_handler_t event_handler)
{
	int err;

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

	err = lwm2m_engine_create_res_inst(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
						      POWER_SOURCE_VOLTAGE_RID, 0));
	if (err) {
		return err;
	}

	/* Device current time. */
	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
						  CURRENT_TIME_RID),
				       &current_time, sizeof(current_time), sizeof(current_time),
				       LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	/* Battery voltage buffer. */
	err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
						  POWER_SOURCE_VOLTAGE_RID),
				       &battery_voltage, sizeof(battery_voltage),
				       sizeof(battery_voltage), LWM2M_RES_DATA_FLAG_RW);
	if (err) {
		return err;
	}

	/* Sensor timestamps. */
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

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_UNITS_RID),
				       BME680_TEMP_UNIT, sizeof(BME680_TEMP_UNIT),
				       sizeof(BME680_TEMP_UNIT), LWM2M_RES_DATA_FLAG_RO);
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

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
						  SENSOR_UNITS_RID),
				       BME680_HUMID_UNIT, sizeof(BME680_HUMID_UNIT),
				       sizeof(BME680_HUMID_UNIT), LWM2M_RES_DATA_FLAG_RO);
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

	err = lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_UNITS_RID),
				       BME680_PRESSURE_UNIT, sizeof(BME680_PRESSURE_UNIT),
				       sizeof(BME680_PRESSURE_UNIT), LWM2M_RES_DATA_FLAG_RO);
	if (err) {
		return err;
	}

	/* Current configuration. */
	err = lwm2m_engine_set_bool(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, PASSIVE_MODE_RID),
				    !cfg->active_mode);
	if (err) {
		return err;
	}

	err = lwm2m_engine_set_s32(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, LOCATION_TIMEOUT_RID),
				   cfg->location_timeout);
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

	/* Register callback for configuration changes. */
	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   PASSIVE_MODE_RID),
							config_update_cb);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   LOCATION_TIMEOUT_RID),
							config_update_cb);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   ACTIVE_WAIT_TIMEOUT_RID),
							config_update_cb);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   MOVEMENT_RESOLUTION_RID),
							config_update_cb);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   MOVEMENT_TIMEOUT_RID),
							config_update_cb);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
			   ACCELEROMETER_ACT_THRESHOLD_RID),
		config_update_cb);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
			   ACCELEROMETER_INACT_THRESHOLD_RID),
		config_update_cb);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
			   ACCELEROMETER_INACT_TIMEOUT_RID),
		config_update_cb);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   GNSS_ENABLE_RID),
							config_update_cb);
	if (err) {
		return err;
	}

	err = lwm2m_engine_register_post_write_callback(LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0,
								   NEIGHBOR_CELL_ENABLE_RID),
							config_update_cb);
	if (err) {
		return err;
	}


	/* Create Button 1 object instance. */
	err = lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
						      BUTTON1_OBJ_INST_ID));
	if (err) {
		return err;
	}

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

	/* Create Button 2 object instance. */
	if (CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT == 2) {
		err = lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
						   BUTTON2_OBJ_INST_ID));
		if (err) {
			return err;
		}

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

	module_evt_handler = event_handler;

	return 0;
}

int cloud_codec_encode_neighbor_cells(struct cloud_codec_data *output,
				      struct cloud_data_neighbor_cells *neighbor_cells)
{
	int err;
	struct lte_lc_cells_info *cells = &neighbor_cells->cell_data;

	cells->neighbor_cells = neighbor_cells->neighbor_cells;
	cells->ncells_count = neighbor_cells->cell_data.ncells_count;

	if (!neighbor_cells->queued) {
		return -ENODATA;
	}

	err = lwm2m_update_signal_meas_objects((const struct lte_lc_cells_info *)cells);
	if (err == -ENODATA) {
		LOG_DBG("No neighboring cells available");
	} else if (err) {
		LOG_ERR("lwm2m_update_signal_meas_objects, error: %d", err);
		return err;
	}

	err = lwm2m_engine_set_s8(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
					     RADIO_SIGNAL_STRENGTH),
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

	neighbor_cells->queued = false;
	return 0;
}

int cloud_codec_encode_agps_request(struct cloud_codec_data *output,
				    struct cloud_data_agps_request *agps_request)
{
	ARG_UNUSED(output);

	int err;

	err = location_assistance_agps_set_mask(&agps_request->request);
	if (err) {
		LOG_ERR("location_assistance_agps_set_mask, error: %d", err);
		return err;
	}

	/* Disable filtered A-GPS. */
	location_assist_agps_set_elevation_mask(-1);

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

	agps_request->queued = false;
	return 0;
}

int cloud_codec_encode_pgps_request(struct cloud_codec_data *output,
				    struct cloud_data_pgps_request *pgps_request)
{
	ARG_UNUSED(output);

	int err;

	err = location_assist_pgps_set_prediction_count(pgps_request->count);
	if (err) {
		LOG_ERR("location_assist_pgps_set_prediction_count, error: %d", err);
		return err;
	}

	err = location_assist_pgps_set_prediction_interval(pgps_request->interval);
	if (err) {
		LOG_ERR("location_assist_pgps_set_prediction_interval, error: %d", err);
		return err;
	}

	err = location_assist_pgps_set_start_time(pgps_request->time);
	if (err) {
		LOG_ERR("location_assist_pgps_set_start_time, error: %d", err);
		return err;
	}

	location_assist_pgps_set_start_gps_day(pgps_request->day);

	pgps_request->queued = false;
	return 0;
}

int cloud_codec_decode_config(const char *input, size_t input_len,
			      struct cloud_data_cfg *cfg)
{
	return -ENOTSUP;
}

int cloud_codec_encode_config(struct cloud_codec_data *output,
			      struct cloud_data_cfg *data)
{
	return -ENOTSUP;
}

int cloud_codec_encode_data(struct cloud_codec_data *output,
			    struct cloud_data_gnss *gnss_buf,
			    struct cloud_data_sensors *sensor_buf,
			    struct cloud_data_modem_static *modem_stat_buf,
			    struct cloud_data_modem_dynamic *modem_dyn_buf,
			    struct cloud_data_ui *ui_buf,
			    struct cloud_data_impact *impact_buf,
			    struct cloud_data_battery *bat_buf)
{
	ARG_UNUSED(ui_buf);

	int err;
	bool objects_written = false;
	double alt, acc, spd;

	/* GPS PVT */
	if (gnss_buf->queued) {
		/*
		 * The LwM2M engine expect double pointers, so floats have to be casted to double
		 * first. Then we can pass pointers to the double values.
		 */
		alt = (double) gnss_buf->pvt.alt;
		acc = (double) gnss_buf->pvt.acc;
		spd = (double) gnss_buf->pvt.spd;

		err = date_time_uptime_to_unix_time_ms(&gnss_buf->gnss_ts);
		if (err) {
			LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
			return err;
		}

		err = lwm2m_engine_set_float(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, LATITUDE_RID),
					     &gnss_buf->pvt.lat);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_float(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, LONGITUDE_RID),
					     &gnss_buf->pvt.longi);
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
					   (int32_t)(gnss_buf->gnss_ts / MSEC_PER_SEC));
		if (err) {
			return err;
		}

		static const char * const path_list[] = {
			LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID)
		};

		err = object_path_list_add(output, path_list, ARRAY_SIZE(path_list));
		if (err) {
			LOG_ERR("Failed populating object path list, error: %d", err);
			return err;
		}

		gnss_buf->queued = false;
		objects_written = true;
	}

	/* Modem dynamic */
	if (modem_dyn_buf->queued) {
		if (modem_dyn_buf->nw_mode == LTE_LC_LTE_MODE_LTEM) {
			err = lwm2m_engine_set_u8(LWM2M_PATH(
							LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
							NETWORK_BEARER_ID),
						  LTE_FDD_BEARER);
			if (err) {
				return err;
			}
		} else if (modem_dyn_buf->nw_mode == LTE_LC_LTE_MODE_NBIOT) {
			err = lwm2m_engine_set_u8(LWM2M_PATH(
							LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
							NETWORK_BEARER_ID),
						  NB_IOT_BEARER);
			if (err) {
				return err;
			}
		} else {
			LOG_WRN("No network bearer set");
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
					       modem_dyn_buf->ip, sizeof(modem_dyn_buf->ip),
					       sizeof(modem_dyn_buf->ip), LWM2M_RES_DATA_FLAG_RO);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_res_buf(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID,
							   0, APN, 0),
					       modem_dyn_buf->apn, sizeof(modem_dyn_buf->apn),
					       sizeof(modem_dyn_buf->apn), LWM2M_RES_DATA_FLAG_RO);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_s8(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						      RADIO_SIGNAL_STRENGTH),
					  modem_dyn_buf->rsrp);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_u32(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						      CELLID),
					   modem_dyn_buf->cell);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						      SMNC),
					   modem_dyn_buf->mnc);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						      SMCC),
					   modem_dyn_buf->mcc);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_u16(LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
						      LAC),
					   modem_dyn_buf->area);
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

		static const char * const path_list[] = {
			LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, CURRENT_TIME_RID),
			LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID)
		};

		err = object_path_list_add(output, path_list, ARRAY_SIZE(path_list));
		if (err) {
			LOG_ERR("Failed populating object path list, error: %d", err);
			return err;
		}

		modem_dyn_buf->queued = false;
		objects_written = true;
	}

	/* Modem static */
	if (modem_stat_buf->queued) {

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
			modem_stat_buf->appv, sizeof(modem_stat_buf->appv),
			sizeof(modem_stat_buf->appv), LWM2M_RES_DATA_FLAG_RO);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_res_buf(
			LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID),
			modem_stat_buf->fw, sizeof(modem_stat_buf->fw),
			sizeof(modem_stat_buf->fw), LWM2M_RES_DATA_FLAG_RO);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_res_buf(
			LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_SERIAL_NUMBER_ID),
			modem_stat_buf->imei, sizeof(modem_stat_buf->imei),
			sizeof(modem_stat_buf->imei), LWM2M_RES_DATA_FLAG_RO);
		if (err) {
			return err;
		}

		static const char * const path_list[] = {
			LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, MANUFACTURER_RID),
			LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, MODEL_NUMBER_RID),
			LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID),
			LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, FIRMWARE_VERSION_RID),
			LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_SERIAL_NUMBER_ID)
		};

		err = object_path_list_add(output, path_list, ARRAY_SIZE(path_list));
		if (err) {
			LOG_ERR("Failed populating object path list, error: %d", err);
			return err;
		}

		modem_stat_buf->queued = false;
		objects_written = true;
	}

	if (bat_buf->queued) {

		err = lwm2m_engine_set_s32(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
						      POWER_SOURCE_VOLTAGE_RID),
					   bat_buf->bat);
		if (err) {
			return err;
		}

		static const char * const path_list[] = {
			LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID)
		};

		err = object_path_list_add(output, path_list, ARRAY_SIZE(path_list));
		if (err) {
			LOG_ERR("Failed populating object path list, error: %d", err);
			return err;
		}

		bat_buf->queued = false;
		objects_written = true;
	}

	/* Environmental sensor data */
	if (sensor_buf->queued) {

		err = date_time_uptime_to_unix_time_ms(&sensor_buf->env_ts);
		if (err) {
			LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
			return err;
		}

		err = lwm2m_engine_set_s32(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0,
						      TIMESTAMP_RID),
					   (int32_t)(sensor_buf->env_ts / MSEC_PER_SEC));
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_s32(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
						      TIMESTAMP_RID),
					   (int32_t)(sensor_buf->env_ts / MSEC_PER_SEC));
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_s32(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0,
						      TIMESTAMP_RID),
					   (int32_t)(sensor_buf->env_ts / MSEC_PER_SEC));
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0,
							SENSOR_VALUE_RID),
					     &sensor_buf->temperature);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0,
							SENSOR_VALUE_RID),
					     &sensor_buf->humidity);
		if (err) {
			return err;
		}

		err = lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0,
							SENSOR_VALUE_RID),
					     &sensor_buf->pressure);
		if (err) {
			return err;
		}

		static const char * const path_list[] = {
			LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
			LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
			LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
			LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
			LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
			LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID)
		};

		err = object_path_list_add(output, path_list, ARRAY_SIZE(path_list));
		if (err) {
			LOG_ERR("Failed populating object path list, error: %d", err);
			return err;
		}

		sensor_buf->queued = false;
		objects_written = true;
	}

	if (!objects_written) {
		return -ENODATA;
	}

	return 0;
}

int cloud_codec_encode_ui_data(struct cloud_codec_data *output,
			       struct cloud_data_ui *ui_buf)
{
	int err, len;

	if (!ui_buf->queued) {
		return -ENODATA;
	}

	/* Create object instance according to the button number. */
	char digital_input_path[12] = "\0", timestamp_path[12] = "\0";

	len = snprintk(digital_input_path,
		       sizeof(digital_input_path),
		       "%d/%d/%d",
		       IPSO_OBJECT_PUSH_BUTTON_ID,
		       (ui_buf->btn - 1),
		       DIGITAL_INPUT_STATE_RID);
	if ((len < 0) || (len >= sizeof(digital_input_path))) {
		return -ERANGE;
	}

	len = snprintk(timestamp_path,
		       sizeof(timestamp_path),
		       "%d/%d/%d",
		       IPSO_OBJECT_PUSH_BUTTON_ID,
		       (ui_buf->btn - 1),
		       TIMESTAMP_RID);
	if ((len < 0) || (len >= sizeof(timestamp_path))) {
		return -ERANGE;
	}

	err = date_time_uptime_to_unix_time_ms(&ui_buf->btn_ts);
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

	err = lwm2m_engine_set_s32(timestamp_path, (int32_t)(ui_buf->btn_ts / MSEC_PER_SEC));
	if (err) {
		return err;
	}

	static const char * const path_list[] = {
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID),
	};

	err = object_path_list_add(output, path_list, ARRAY_SIZE(path_list));
	if (err) {
		LOG_ERR("Failed populating object path list, error: %d", err);
		return err;
	}

	ui_buf->queued = false;
	return 0;
}

int cloud_codec_encode_impact_data(struct cloud_codec_data *output,
				   struct cloud_data_impact *impact_buf)
{
	return -ENOTSUP;
}

int cloud_codec_encode_batch_data(struct cloud_codec_data *output,
				  struct cloud_data_gnss *gnss_buf,
				  struct cloud_data_sensors *sensor_buf,
				  struct cloud_data_modem_static *modem_stat_buf,
				  struct cloud_data_modem_dynamic *modem_dyn_buf,
				  struct cloud_data_ui *ui_buf,
				  struct cloud_data_impact *impact_buf,
				  struct cloud_data_battery *bat_buf,
				  size_t gnss_buf_count,
				  size_t sensor_buf_count,
				  size_t modem_stat_buf_count,
				  size_t modem_dyn_buf_count,
				  size_t ui_buf_count,
				  size_t impact_buf_count,
				  size_t bat_buf_count)
{
	return -ENOTSUP;
}
