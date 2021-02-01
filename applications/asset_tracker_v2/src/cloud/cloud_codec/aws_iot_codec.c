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
#include "cJSON_os.h"
#include "json_aux.h"
#include <date_time.h>
#include "buffer.h"

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

/* List of JSON object labels associated with a buffer. */
static char *object_label[BUFFER_COUNT] = {
	[BUFFER_UI]		= DATA_BUTTON,
	[BUFFER_MODEM_STATIC]	= DATA_MODEM_STATIC,
	[BUFFER_MODEM_DYNAMIC]	= DATA_MODEM_DYNAMIC,
	[BUFFER_GPS]		= DATA_GPS,
	[BUFFER_SENSOR]		= DATA_ENVIRONMENTALS,
	[BUFFER_ACCELEROMETER]	= DATA_MOVEMENT,
	[BUFFER_BATTERY]	= DATA_BATTERY,
};

/* Static functions */
static int modem_static_data_add(cJSON *parent,
				 struct cloud_data_modem_static *data,
				 bool batch_entry)
{
	int err = 0;
	char nw_mode[50];

	static const char lte_string[] = "LTE-M";
	static const char nbiot_string[] = "NB-IoT";
	static const char gps_string[] = " GPS";

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

	if (batch_entry) {
		err += json_add_obj_array(parent, static_m);
	} else {
		err += json_add_obj(parent, DATA_MODEM_STATIC, static_m);
	}

	if (err) {
		return err;
	}

	return 0;
}

static int modem_dynamic_data_add(cJSON *parent,
				  struct cloud_data_modem_dynamic *data,
				  bool batch_entry)
{
	int err = 0;
	long mccmnc;

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

	return 0;
}

static int sensor_data_add(cJSON *parent, struct cloud_data_sensors *data,
			   bool batch_entry)
{
	int err = 0;

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

	return 0;
}

static int gps_data_add(cJSON *parent, struct cloud_data_gps *data,
			bool batch_entry)
{
	int err = 0;

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

	return 0;
}

static int accel_data_add(cJSON *parent, struct cloud_data_accelerometer *data,
			  bool batch_entry)
{
	int err = 0;

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

	return 0;
}

static int ui_data_add(cJSON *parent, struct cloud_data_ui *data,
		       bool batch_entry)
{
	int err = 0;

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

	return 0;
}

static int battery_data_add(cJSON *parent, struct cloud_data_battery *data,
			    bool batch_entry)
{
	int err = 0;

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

	return 0;
}

static int data_batch_add(cJSON *parent,
			 enum buffer_type type,
			 char *entry_name)
{
	int err;
	void *item;
	size_t item_len = buffer_get_item_size(type);
	cJSON *array_obj = cJSON_CreateArray();

	if (array_obj == NULL) {
		cJSON_Delete(array_obj);
		return -ENOMEM;
	}

	while (buffer_is_empty(type) == 0) {
		err = buffer_get_data(type, &item, item_len);
		if (err) {
			LOG_ERR("buffer_get_data, error: %d", err);
			break;
		}

		switch (type) {
		case BUFFER_UI:
			err = ui_data_add(array_obj,
					 (struct cloud_data_ui *)item,
					 true);
			if (err) {
				LOG_ERR("ui_data_add, error: %d", err);
			}
			break;
		case BUFFER_MODEM_STATIC:
			err = modem_static_data_add(
					array_obj,
					(struct cloud_data_modem_static *)item,
					true);
			if (err) {
				LOG_ERR("modem_static_data_add, error: %d",
					err);
			}
			break;
		case BUFFER_MODEM_DYNAMIC:
			err = modem_dynamic_data_add(
				array_obj,
				(struct cloud_data_modem_dynamic *)item,
				true);
			if (err) {
				LOG_ERR("modem_dynamic_data_add, error: %d",
					err);
			}
			break;
		case BUFFER_ACCELEROMETER:
			err = accel_data_add(
				array_obj,
				(struct cloud_data_accelerometer *)item,
				true);
			if (err) {
				LOG_ERR("accel_data_add, error: %d", err);
			}
			break;
		case BUFFER_GPS:
			err = gps_data_add(array_obj,
					  (struct cloud_data_gps *)item,
					  true);
			if (err) {
				LOG_ERR("gps_data_add, error: %d", err);
			}
			break;
		case BUFFER_SENSOR:
			err = sensor_data_add(
					array_obj,
					(struct cloud_data_sensors *)item,
					true);
			if (err) {
				LOG_ERR("sensor_data_add, error: %d", err);
			}
			break;
		case BUFFER_BATTERY:
			err = battery_data_add(
					array_obj,
					(struct cloud_data_battery *)item,
					true);
			if (err) {
				LOG_ERR("battery_data_add, error: %d", err);
			}
			break;
		default:
			LOG_WRN("Unknown buffer");
			break;
		}

		err = buffer_get_data_finished(type, item_len);
		if (err) {
			LOG_ERR("buffer_get_data_finished, error: %d", err);
			break;
		}
	}

	if (cJSON_GetArraySize(array_obj) > 0) {
		json_add_obj(parent, entry_name, array_obj);
	} else {
		/* No data added to the array. Delete array object. */
		err = -ENODATA;
		cJSON_Delete(array_obj);
	}

	return err;
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

int cloud_codec_encode_data(struct cloud_codec_data *output)
{
	int err = 0;
	char *buffer;
	bool objects_added = false;

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *rep_obj = cJSON_CreateObject();

	if (root_obj == NULL || state_obj == NULL || rep_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(rep_obj);
		return -ENOMEM;
	}

	struct cloud_data_ui *ui;

	if (buffer_last_known_get(BUFFER_UI, &ui)) {
		err = ui_data_add(rep_obj, ui, false);
		if (err) {
			LOG_ERR("ui_data_add, error: %d", err);
			goto add_obj;
		}
		objects_added = true;
	}

	struct cloud_data_modem_static *modem_static;

	if (buffer_last_known_get(BUFFER_MODEM_STATIC, &modem_static)) {
		err = modem_static_data_add(rep_obj, modem_static, false);
		if (err) {
			LOG_ERR("modem_static_data_add, error: %d", err);
			goto add_obj;
		}
		objects_added = true;
	}

	struct cloud_data_modem_dynamic *modem_dynamic;

	if (buffer_last_known_get(BUFFER_MODEM_DYNAMIC, &modem_dynamic)) {
		err = modem_dynamic_data_add(rep_obj, modem_dynamic, false);
		if (err) {
			LOG_ERR("modem_dynamic_data_add, error: %d", err);
			goto add_obj;
		}
		objects_added = true;
	}

	struct cloud_data_sensors *sensor;

	if (buffer_last_known_get(BUFFER_SENSOR, &sensor)) {
		err = sensor_data_add(rep_obj, sensor, false);
		if (err) {
			LOG_ERR("sensor_data_add, error: %d", err);
			goto add_obj;
		}
		objects_added = true;
	}

	struct cloud_data_gps *gps;

	if (buffer_last_known_get(BUFFER_GPS, &gps)) {
		err = gps_data_add(rep_obj, gps, false);
		if (err) {
			LOG_ERR("gps_data_add, error: %d", err);
			goto add_obj;
		}
		objects_added = true;
	}

	struct cloud_data_battery *battery;

	if (buffer_last_known_get(BUFFER_BATTERY, &battery)) {
		err = battery_data_add(rep_obj, battery, false);
		if (err) {
			LOG_ERR("battery_data_add, error: %d", err);
			goto add_obj;
		}
		objects_added = true;
	}

	struct cloud_data_accelerometer *accel;

	if (buffer_last_known_get(BUFFER_ACCELEROMETER, &accel)) {
		err = accel_data_add(rep_obj, accel, false);
		if (err) {
			LOG_ERR("accel_data_add, error: %d", err);
			goto add_obj;
		}
		objects_added = true;
	}

add_obj:

	json_add_obj(state_obj, OBJECT_REPORTED, rep_obj);
	json_add_obj(root_obj, OBJECT_STATE, state_obj);

	if (err) {
		goto exit;
	}

	if (!objects_added) {
		LOG_DBG("No data added to encoded message");

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
		json_print_obj("Encoded message:\n", root_obj);
	}

	output->buf = buffer;
	output->len = strlen(buffer);

exit:
	cJSON_Delete(root_obj);
	return err;
}

int cloud_codec_encode_ui_data(struct cloud_codec_data *output)
{
	int err = 0;
	char *buffer;

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	struct cloud_data_ui *ui;

	if (buffer_last_known_get(BUFFER_UI, &ui)) {
		err = ui_data_add(root_obj, ui, false);
		if (err) {
			LOG_ERR("ui_data_add, error: %d", err);
			goto exit;
		}
	} else {
		LOG_DBG("No data added to encoded UI message");

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
		json_print_obj("Encoded message:\n", root_obj);
	}

	output->buf = buffer;
	output->len = strlen(buffer);

exit:
	cJSON_Delete(root_obj);
	return err;
}

int cloud_codec_encode_batch_data(struct cloud_codec_data *output)
{
	int err;
	char *buffer;
	bool object_added = false;

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		return -ENOMEM;
	}

	for (int i = 0; i < ARRAY_SIZE(object_label); i++) {
		err = data_batch_add(root_obj, i, object_label[i]);
		if (err == -ENODATA) {
			/* No data in buffer. */
			continue;
		} else if (err) {
			goto exit;
		}
		object_added = true;
	}

	if (!object_added) {
		LOG_DBG("No data added to encoded batch message");

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
