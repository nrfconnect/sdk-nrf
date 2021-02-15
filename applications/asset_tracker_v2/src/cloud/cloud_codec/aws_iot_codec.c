/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cloud_codec.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "json_aux.h"
#include <date_time.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(cloud_codec, CONFIG_CLOUD_CODEC_LOG_LEVEL);

#define MODEM_CURRENT_BAND	"band"
#define MODEM_NETWORK_MODE	"nw"
#define MODEM_ICCID		"iccid"
#define MODEM_FIRMWARE_VERSION	"modV"
#define MODEM_BOARD		"brdV"
#define MODEM_APP_VERSION	"appV"
#define MODEM_RSRP		"rsrp"
#define MODEM_AREA_CODE		"area"
#define MODEM_MCCMNC		"mccmnc"
#define MODEM_CELL_ID		"cell"
#define MODEM_IP_ADDRESS	"ip"

#define CONFIG_DEVICE_MODE	"act"
#define CONFIG_ACTIVE_TIMEOUT	"actwt"
#define CONFIG_MOVE_TIMEOUT	"mvt"
#define CONFIG_MOVE_RES		"mvres"
#define CONFIG_GPS_TIMEOUT	"gpst"
#define CONFIG_ACC_THRESHOLD	"acct"

#define OBJECT_CONFIG		"cfg"
#define OBJECT_REPORTED		"reported"
#define OBJECT_STATE		"state"
#define OBJECT_VALUE		"v"
#define OBJECT_TIMESTAMP	"ts"

#define DATA_MODEM_DYNAMIC	"roam"
#define DATA_MODEM_STATIC	"dev"
#define DATA_BATTERY		"bat"
#define DATA_TEMPERATURE	"temp"
#define DATA_HUMID		"hum"
#define DATA_ENVIRONMENTALS	"env"
#define DATA_BUTTON		"btn"

#define DATA_MOVEMENT		"acc"
#define DATA_MOVEMENT_X		"x"
#define DATA_MOVEMENT_Y		"y"
#define DATA_MOVEMENT_Z		"z"

#define DATA_GPS		"gps"
#define DATA_GPS_LONGITUDE	"lng"
#define DATA_GPS_LATITUDE	"lat"
#define DATA_GPS_ALTITUDE	"alt"
#define DATA_GPS_SPEED		"spd"
#define DATA_GPS_HEADING	"hdg"

/* Static functions */
static int static_modem_data_add(cJSON *parent,
				 struct cloud_data_modem_static *data)
{
	int err = 0;
	char nw_mode[50] = {0};

	static const char lte_string[] = "LTE-M";
	static const char nbiot_string[] = "NB-IoT";
	static const char gps_string[] = " GPS";

	if (!data->queued) {
		LOG_DBG("Head of modem buffer not indexing a queued entry");
		goto exit;
	}

	err = date_time_uptime_to_unix_time_ms(&data->ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *static_m = cJSON_CreateObject();
	cJSON *static_m_v = cJSON_CreateObject();

	if (static_m == NULL || static_m_v == NULL) {
		cJSON_Delete(static_m);
		cJSON_Delete(static_m_v);
		return -ENOMEM;
	}

	if (data->nw_lte_m) {
		strcpy(nw_mode, lte_string);
	} else if (data->nw_nb_iot) {
		strcpy(nw_mode, nbiot_string);
	}

	if (data->nw_gps) {
		strcat(nw_mode, gps_string);
	}

	err += json_add_number(static_m_v, MODEM_CURRENT_BAND, data->bnd);
	err += json_add_str(static_m_v, MODEM_NETWORK_MODE, nw_mode);
	err += json_add_str(static_m_v, MODEM_ICCID, data->iccid);
	err += json_add_str(static_m_v, MODEM_FIRMWARE_VERSION, data->fw);
	err += json_add_str(static_m_v, MODEM_BOARD, data->brdv);
	err += json_add_str(static_m_v, MODEM_APP_VERSION, data->appv);

	err += json_add_obj(static_m, OBJECT_VALUE, static_m_v);
	err += json_add_number(static_m, OBJECT_TIMESTAMP, data->ts);
	err += json_add_obj(parent, DATA_MODEM_STATIC, static_m);

	if (err) {
		return err;
	}

	data->queued = false;

exit:
	return 0;
}

static int dynamic_modem_data_add(cJSON *parent,
				  struct cloud_data_modem_dynamic *data,
				  bool batch_entry)
{
	int err = 0;
	long mccmnc;

	if (!data->queued) {
		LOG_DBG("Head of modem buffer not indexing a queued entry");
		goto exit;
	}

	err = date_time_uptime_to_unix_time_ms(&data->ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *dynamic_m = cJSON_CreateObject();
	cJSON *dynamic_m_v = cJSON_CreateObject();

	if (dynamic_m == NULL || dynamic_m_v == NULL) {
		cJSON_Delete(dynamic_m);
		cJSON_Delete(dynamic_m_v);
		return -ENOMEM;
	}

	mccmnc = strtol(data->mccmnc, NULL, 10);

	err += json_add_number(dynamic_m_v, MODEM_RSRP, data->rsrp);
	err += json_add_number(dynamic_m_v, MODEM_AREA_CODE, data->area);
	err += json_add_number(dynamic_m_v, MODEM_MCCMNC, mccmnc);
	err += json_add_number(dynamic_m_v, MODEM_CELL_ID, data->cell);
	err += json_add_str(dynamic_m_v, MODEM_IP_ADDRESS, data->ip);

	err += json_add_obj(dynamic_m, OBJECT_VALUE, dynamic_m_v);
	err += json_add_number(dynamic_m, OBJECT_TIMESTAMP, data->ts);

	if (batch_entry) {
		err += json_add_obj_array(parent, dynamic_m);
	} else {
		err += json_add_obj(parent, DATA_MODEM_DYNAMIC, dynamic_m);
	}

	if (err) {
		return err;
	}

	data->queued = false;

exit:
	return 0;
}

static int sensor_data_add(cJSON *parent, struct cloud_data_sensors *data,
			   bool batch_entry)
{
	int err = 0;

	if (!data->queued) {
		LOG_DBG("Head of sensor buffer not indexing a queued entry");
		goto exit;
	}

	err = date_time_uptime_to_unix_time_ms(&data->env_ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *sensor_obj = cJSON_CreateObject();
	cJSON *sensor_val_obj = cJSON_CreateObject();

	if (sensor_obj == NULL || sensor_val_obj == NULL) {
		cJSON_Delete(sensor_obj);
		cJSON_Delete(sensor_val_obj);
		return -ENOMEM;
	}

	err = json_add_number(sensor_val_obj, DATA_TEMPERATURE, data->temp);
	err += json_add_number(sensor_val_obj, DATA_HUMID, data->hum);
	err += json_add_obj(sensor_obj, OBJECT_VALUE, sensor_val_obj);
	err += json_add_number(sensor_obj, OBJECT_TIMESTAMP, data->env_ts);

	if (batch_entry) {
		err += json_add_obj_array(parent, sensor_obj);
	} else {
		err += json_add_obj(parent, DATA_ENVIRONMENTALS, sensor_obj);
	}

	if (err) {
		return err;
	}

	data->queued = false;

exit:
	return 0;
}

static int gps_data_add(cJSON *parent, struct cloud_data_gps *data,
			bool batch_entry)
{
	int err = 0;

	if (!data->queued) {
		LOG_DBG("Head of gps buffer not indexing a queued entry");
		goto exit;
	}

	err = date_time_uptime_to_unix_time_ms(&data->gps_ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *gps_obj = cJSON_CreateObject();
	cJSON *gps_val_obj = cJSON_CreateObject();

	if (gps_obj == NULL || gps_val_obj == NULL) {
		cJSON_Delete(gps_obj);
		cJSON_Delete(gps_val_obj);
		return -ENOMEM;
	}

	err += json_add_number(gps_val_obj, DATA_GPS_LONGITUDE, data->longi);
	err += json_add_number(gps_val_obj, DATA_GPS_LATITUDE, data->lat);
	err += json_add_number(gps_val_obj, DATA_MOVEMENT, data->acc);
	err += json_add_number(gps_val_obj, DATA_GPS_ALTITUDE, data->alt);
	err += json_add_number(gps_val_obj, DATA_GPS_SPEED, data->spd);
	err += json_add_number(gps_val_obj, DATA_GPS_HEADING, data->hdg);

	err += json_add_obj(gps_obj, OBJECT_VALUE, gps_val_obj);
	err += json_add_number(gps_obj, OBJECT_TIMESTAMP, data->gps_ts);

	if (batch_entry) {
		err += json_add_obj_array(parent, gps_obj);
	} else {
		err += json_add_obj(parent, DATA_GPS, gps_obj);
	}

	if (err) {
		return err;
	}

	data->queued = false;

exit:
	return 0;
}

static int accel_data_add(cJSON *parent, struct cloud_data_accelerometer *data,
			  bool batch_entry)
{
	int err = 0;

	if (!data->queued) {
		LOG_DBG("Head of accel buffer not indexing a queued entry");
		goto exit;
	}

	err = date_time_uptime_to_unix_time_ms(&data->ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *acc_obj = cJSON_CreateObject();
	cJSON *acc_v_obj = cJSON_CreateObject();

	if (acc_obj == NULL || acc_v_obj == NULL) {
		cJSON_Delete(acc_obj);
		cJSON_Delete(acc_v_obj);
		return -ENOMEM;
	}

	err += json_add_number(acc_v_obj, DATA_MOVEMENT_X, data->values[0]);
	err += json_add_number(acc_v_obj, DATA_MOVEMENT_Y, data->values[1]);
	err += json_add_number(acc_v_obj, DATA_MOVEMENT_Z, data->values[2]);

	err += json_add_obj(acc_obj, OBJECT_VALUE, acc_v_obj);
	err += json_add_number(acc_obj, OBJECT_TIMESTAMP, data->ts);

	if (batch_entry) {
		err += json_add_obj_array(parent, acc_obj);
	} else {
		err += json_add_obj(parent, DATA_MOVEMENT, acc_obj);
	}

	if (err) {
		return err;
	}

	data->queued = false;

exit:
	return 0;
}

static int ui_data_add(cJSON *parent, struct cloud_data_ui *data,
		       bool batch_entry)
{
	int err = 0;

	if (!data->queued) {
		LOG_DBG("Head of UI buffer not indexing a queued entry");
		goto exit;
	}

	err = date_time_uptime_to_unix_time_ms(&data->btn_ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *btn_obj = cJSON_CreateObject();

	if (btn_obj == NULL) {
		cJSON_Delete(btn_obj);
		return -ENOMEM;
	}

	err += json_add_number(btn_obj, OBJECT_VALUE, data->btn);
	err += json_add_number(btn_obj, OBJECT_TIMESTAMP, data->btn_ts);

	if (batch_entry) {
		err += json_add_obj_array(parent, btn_obj);
	} else {
		err += json_add_obj(parent, DATA_BUTTON, btn_obj);
	}

	if (err) {
		return err;
	}

	data->queued = false;

exit:
	return 0;
}

static int bat_data_add(cJSON *parent, struct cloud_data_battery *data,
			bool batch_entry)
{
	int err = 0;

	if (!data->queued) {
		LOG_DBG("Head of battery buffer not indexing a queued entry");
		goto exit;
	}

	err = date_time_uptime_to_unix_time_ms(&data->bat_ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *bat_obj = cJSON_CreateObject();

	if (bat_obj == NULL) {
		cJSON_Delete(bat_obj);
		return -ENOMEM;
	}

	err += json_add_number(bat_obj, OBJECT_VALUE, data->bat);
	err += json_add_number(bat_obj, OBJECT_TIMESTAMP, data->bat_ts);

	if (batch_entry) {
		err += json_add_obj_array(parent, bat_obj);
	} else {
		err += json_add_obj(parent, DATA_BATTERY, bat_obj);
	}

	if (err) {
		return err;
	}

	data->queued = false;

exit:
	return 0;
}

/* Public interface */
int cloud_codec_decode_config(char *input, struct cloud_data_cfg *data)
{
	int err = 0;
	cJSON *root_obj = NULL;
	cJSON *group_obj = NULL;
	cJSON *subgroup_obj = NULL;
	cJSON *gps_timeout = NULL;
	cJSON *active = NULL;
	cJSON *active_wait = NULL;
	cJSON *move_res = NULL;
	cJSON *move_timeout = NULL;
	cJSON *acc_thres = NULL;

	if (input == NULL) {
		return -EINVAL;
	}

	root_obj = cJSON_Parse(input);
	if (root_obj == NULL) {
		return -ENOENT;
	}

	if (IS_ENABLED(CONFIG_CLOUD_CODEC_LOG_LEVEL_DBG)) {
		json_print_obj("Decoded message:\n", root_obj);
	}

	group_obj = json_object_decode(root_obj, OBJECT_CONFIG);
	if (group_obj != NULL) {
		subgroup_obj = group_obj;
		goto get_data;
	}

	group_obj = json_object_decode(root_obj, OBJECT_STATE);
	if (group_obj == NULL) {
		err = -ENODATA;
		goto exit;
	}

	subgroup_obj = json_object_decode(group_obj, OBJECT_CONFIG);
	if (subgroup_obj == NULL) {
		err = -ENODATA;
		goto exit;
	}

get_data:

	gps_timeout = cJSON_GetObjectItem(subgroup_obj, CONFIG_GPS_TIMEOUT);
	active = cJSON_GetObjectItem(subgroup_obj, CONFIG_DEVICE_MODE);
	active_wait = cJSON_GetObjectItem(subgroup_obj, CONFIG_ACTIVE_TIMEOUT);
	move_res = cJSON_GetObjectItem(subgroup_obj, CONFIG_MOVE_RES);
	move_timeout = cJSON_GetObjectItem(subgroup_obj, CONFIG_MOVE_TIMEOUT);
	acc_thres = cJSON_GetObjectItem(subgroup_obj, CONFIG_ACC_THRESHOLD);

	if (gps_timeout != NULL) {
		data->gps_timeout = gps_timeout->valueint;
	}

	if (active != NULL) {
		data->active_mode = active->valueint;
	}

	if (active_wait != NULL) {
		data->active_wait_timeout = active_wait->valueint;
	}

	if (move_res != NULL) {
		data->movement_resolution = move_res->valueint;
	}

	if (move_timeout != NULL) {
		data->movement_timeout = move_timeout->valueint;
	}

	if (acc_thres != NULL) {
		data->accelerometer_threshold = acc_thres->valuedouble;
	}

exit:
	cJSON_Delete(root_obj);
	return err;
}

int cloud_codec_encode_config(struct cloud_codec_data *output,
			      struct cloud_data_cfg *data)
{
	int err = 0;
	char *buffer;

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *rep_obj = cJSON_CreateObject();
	cJSON *cfg_obj = cJSON_CreateObject();

	if (root_obj == NULL || state_obj == NULL || rep_obj == NULL ||
	    cfg_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(rep_obj);
		cJSON_Delete(cfg_obj);
		return -ENOMEM;
	}

	err += json_add_bool(cfg_obj, CONFIG_DEVICE_MODE,
			     data->active_mode);
	err += json_add_number(cfg_obj, CONFIG_GPS_TIMEOUT,
			       data->gps_timeout);
	err += json_add_number(cfg_obj, CONFIG_ACTIVE_TIMEOUT,
			       data->active_wait_timeout);
	err += json_add_number(cfg_obj, CONFIG_MOVE_RES,
			       data->movement_resolution);
	err += json_add_number(cfg_obj, CONFIG_MOVE_TIMEOUT,
			       data->movement_timeout);
	err += json_add_number(cfg_obj, CONFIG_ACC_THRESHOLD,
			       data->accelerometer_threshold);

	err += json_add_obj(rep_obj, OBJECT_CONFIG, cfg_obj);
	err += json_add_obj(state_obj, OBJECT_REPORTED, rep_obj);
	err += json_add_obj(root_obj, OBJECT_STATE, state_obj);

	if (err) {
		goto exit;
	}

	buffer = cJSON_PrintUnformatted(root_obj);
	if (buffer == NULL) {
		LOG_ERR("Failed to allocate memory for JSON string");

		err = -ENOMEM;
		goto exit;
	}

	if (IS_ENABLED(CONFIG_CLOUD_CODEC_LOG_LEVEL_DBG)) {
		json_print_obj("Encoded message:\n", root_obj);
	}

	output->buf = buffer;
	output->len = strlen(buffer);

exit:
	cJSON_Delete(root_obj);
	return err;
}

int cloud_codec_encode_data(struct cloud_codec_data *output,
			    struct cloud_data_gps *gps_buf,
			    struct cloud_data_sensors *sensor_buf,
			    struct cloud_data_modem_static *modem_stat_buf,
			    struct cloud_data_modem_dynamic *modem_dyn_buf,
			    struct cloud_data_ui *ui_buf,
			    struct cloud_data_accelerometer *mov_buf,
			    struct cloud_data_battery *bat_buf)
{
	int err = 0;
	char *buffer;
	bool data_encoded = false;

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *rep_obj = cJSON_CreateObject();

	if (root_obj == NULL || state_obj == NULL || rep_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(rep_obj);
		return -ENOMEM;
	}

	if (bat_buf->queued) {
		err += bat_data_add(rep_obj, bat_buf, false);
		data_encoded = true;
	}

	if (modem_stat_buf->queued) {
		err += static_modem_data_add(rep_obj, modem_stat_buf);
		data_encoded = true;
	}

	if (modem_dyn_buf->queued) {
		err += dynamic_modem_data_add(rep_obj, modem_dyn_buf, false);
		data_encoded = true;
	}

	if (sensor_buf->queued) {
		err += sensor_data_add(rep_obj, sensor_buf, false);
		data_encoded = true;
	}

	if (gps_buf->queued) {
		err += gps_data_add(rep_obj, gps_buf, false);
		data_encoded = true;
	}

	if (mov_buf->queued) {
		err += accel_data_add(rep_obj, mov_buf, false);
		data_encoded = true;
	}

	err += json_add_obj(state_obj, OBJECT_REPORTED, rep_obj);
	err += json_add_obj(root_obj, OBJECT_STATE, state_obj);

	/* Exit upon encoding errors or no data encoded. */
	if (err) {
		goto exit;
	}

	if (!data_encoded) {
		err = -ENODATA;
		LOG_DBG("No data to encode...");
		goto exit;
	}

	buffer = cJSON_PrintUnformatted(root_obj);
	if (buffer == NULL) {
		LOG_ERR("Failed to allocate memory for JSON string");

		err = -ENOMEM;
		goto exit;
	}

	if (IS_ENABLED(CONFIG_CLOUD_CODEC_LOG_LEVEL_DBG)) {
		json_print_obj("Encoded message:\n", root_obj);
	}

	output->buf = buffer;
	output->len = strlen(buffer);

exit:
	cJSON_Delete(root_obj);

	return err;
}

int cloud_codec_encode_ui_data(struct cloud_codec_data *output,
			       struct cloud_data_ui *ui_buf)
{
	int err = 0;
	char *buffer;

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	if (ui_buf->queued) {
		err += ui_data_add(root_obj, ui_buf, false);
	} else {
		goto exit;
	}

	if (err) {
		goto exit;
	}

	buffer = cJSON_PrintUnformatted(root_obj);
	if (buffer == NULL) {
		LOG_ERR("Failed to allocate memory for JSON string");

		err = -ENOMEM;
		goto exit;
	}

	if (IS_ENABLED(CONFIG_CLOUD_CODEC_LOG_LEVEL_DBG)) {
		json_print_obj("Encoded message:\n", root_obj);
	}

	output->buf = buffer;
	output->len = strlen(buffer);

exit:
	cJSON_Delete(root_obj);

	return err;
}

int cloud_codec_encode_batch_data(
				struct cloud_codec_data *output,
				struct cloud_data_gps *gps_buf,
				struct cloud_data_sensors *sensor_buf,
				struct cloud_data_modem_dynamic *modem_dyn_buf,
				struct cloud_data_ui *ui_buf,
				struct cloud_data_accelerometer *accel_buf,
				struct cloud_data_battery *bat_buf,
				size_t gps_buf_count,
				size_t sensor_buf_count,
				size_t modem_dyn_buf_count,
				size_t ui_buf_count,
				size_t accel_buf_count,
				size_t bat_buf_count)
{
	int err = 0;
	char *buffer;
	bool data_encoded = false;

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *gps_obj = cJSON_CreateArray();
	cJSON *sensor_obj = cJSON_CreateArray();
	cJSON *modem_obj = cJSON_CreateArray();
	cJSON *ui_obj = cJSON_CreateArray();
	cJSON *accel_obj = cJSON_CreateArray();
	cJSON *bat_obj = cJSON_CreateArray();

	if (root_obj == NULL || gps_obj == NULL || sensor_obj == NULL ||
	    modem_obj == NULL || ui_obj == NULL || accel_obj == NULL ||
	    bat_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(gps_obj);
		cJSON_Delete(sensor_obj);
		cJSON_Delete(modem_obj);
		cJSON_Delete(ui_obj);
		cJSON_Delete(accel_obj);
		cJSON_Delete(bat_obj);
		return -ENOMEM;
	}

	/* GPS data */
	for (int i = 0; i < gps_buf_count; i++) {
		if (gps_buf[i].queued) {
			err += gps_data_add(gps_obj, &gps_buf[i], true);
		}
	}

	if (cJSON_GetArraySize(gps_obj) > 0) {
		err += json_add_obj(root_obj, DATA_GPS, gps_obj);
		data_encoded = true;
	} else {
		cJSON_Delete(gps_obj);
	}

	/* Environmental sensor data */
	for (int i = 0; i < sensor_buf_count; i++) {
		if (sensor_buf[i].queued) {
			err += sensor_data_add(sensor_obj,
					       &sensor_buf[i],
					       true);
		}
	}

	if (cJSON_GetArraySize(sensor_obj) > 0) {
		err += json_add_obj(root_obj, DATA_ENVIRONMENTALS, sensor_obj);
		data_encoded = true;
	} else {
		cJSON_Delete(sensor_obj);
	}

	/* UI data */
	for (int i = 0; i < ui_buf_count; i++) {
		if (ui_buf[i].queued) {
			err += ui_data_add(ui_obj, &ui_buf[i], true);
		}
	}

	if (cJSON_GetArraySize(ui_obj) > 0) {
		err += json_add_obj(root_obj, DATA_BUTTON, ui_obj);
		data_encoded = true;
	} else {
		cJSON_Delete(ui_obj);
	}

	/* Movement data */
	for (int i = 0; i < accel_buf_count; i++) {
		if (accel_buf[i].queued) {
			err += accel_data_add(accel_obj,
					      &accel_buf[i],
					      true);
		}
	}

	if (cJSON_GetArraySize(accel_obj) > 0) {
		err += json_add_obj(root_obj, DATA_MOVEMENT, accel_obj);
		data_encoded = true;
	} else {
		cJSON_Delete(accel_obj);
	}

	/* Battery data */
	for (int i = 0; i < bat_buf_count; i++) {
		if (bat_buf[i].queued) {
			err += bat_data_add(bat_obj, &bat_buf[i], true);
		}
	}

	if (cJSON_GetArraySize(bat_obj) > 0) {
		err += json_add_obj(root_obj, DATA_BATTERY, bat_obj);
		data_encoded = true;
	} else {
		cJSON_Delete(bat_obj);
	}

	/* Dynamic modem data */
	for (int i = 0; i < modem_dyn_buf_count; i++) {
		if (modem_dyn_buf[i].queued) {
			err += dynamic_modem_data_add(modem_obj,
						     &modem_dyn_buf[i],
						     true);
		}
	}

	if (cJSON_GetArraySize(modem_obj) > 0) {
		err += json_add_obj(root_obj, DATA_MODEM_DYNAMIC, modem_obj);
		data_encoded = true;
	} else {
		cJSON_Delete(modem_obj);
	}

	if (err) {
		goto exit;
	} else if (!data_encoded) {
		err = -ENODATA;
		goto exit;
	}

	buffer = cJSON_PrintUnformatted(root_obj);
	if (buffer == NULL) {
		LOG_ERR("Failed to allocate memory for JSON string");

		err = -ENOMEM;
		goto exit;
	}

	if (IS_ENABLED(CONFIG_CLOUD_CODEC_LOG_LEVEL_DBG)) {
		json_print_obj("Encoded batch message:\n", root_obj);
	}

	output->buf = buffer;
	output->len = strlen(buffer);

exit:

	cJSON_Delete(root_obj);

	return err;
}
