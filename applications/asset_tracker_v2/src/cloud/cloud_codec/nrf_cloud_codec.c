/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <date_time.h>
#include <cloud_codec.h>

#include "cJSON.h"
#include "json_helpers.h"
#include "json_common.h"
#include "json_protocol_names.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cloud_codec, CONFIG_CLOUD_CODEC_LOG_LEVEL);

/* Function that checks the version number of the incoming message and determines if it has already
 * been handled. Receiving duplicate messages can often occur upon retransmissions from the AWS IoT
 * broker due to unACKed MQTT QoS 1 messages and unACKed TCP packets. Messages from AWS IoT
 * shadow topics contains an incrementing version number.
 */
static bool has_shadow_update_been_handled(cJSON *root_obj)
{
	cJSON *version_obj;
	static int version_prev;
	bool retval = false;

	version_obj = cJSON_GetObjectItem(root_obj, DATA_VERSION);
	if (version_obj == NULL) {
		/* No version number present in message. */
		return false;
	}

	/* If the incoming version number is lower than or equal to the previous,
	 * the incoming message is considered a retransmitted message and will be ignored.
	 */
	if (version_prev >= version_obj->valueint) {
		retval = true;
	}

	version_prev = version_obj->valueint;

	return retval;
}

static int add_meta_data(cJSON *parent, const char *app_id, int64_t *timestamp)
{
	int err;

	err = date_time_uptime_to_unix_time_ms(timestamp);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	err = json_add_str(parent, DATA_ID, app_id);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_str(parent, DATA_GROUP, MESSAGE_TYPE_DATA);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_TIMESTAMP, *timestamp);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	return 0;
}

static int add_data(cJSON *parent, const char *app_id, const char *str_val,
		    int64_t *timestamp, bool queued, const char *object_name)
{
	int err;

	if (!queued) {
		return -ENODATA;
	}

	cJSON *data_obj = cJSON_CreateObject();

	err = json_add_str(data_obj, DATA_TYPE, str_val);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = add_meta_data(data_obj, app_id, timestamp);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	json_add_obj(parent, object_name, data_obj);
	return 0;

exit:
	cJSON_Delete(data_obj);
	return err;
}

int cloud_codec_encode_neighbor_cells(struct cloud_codec_data *output,
				      struct cloud_data_neighbor_cells *neighbor_cells)
{
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(neighbor_cells != NULL);

	neighbor_cells->queued = false;
	return -ENOTSUP;
}

int cloud_codec_decode_config(char *input, size_t input_len,
			      struct cloud_data_cfg *data)
{
	int err = 0;
	cJSON *root_obj = NULL;
	cJSON *group_obj = NULL;
	cJSON *subgroup_obj = NULL;

	if (input == NULL) {
		return -EINVAL;
	}

	root_obj = cJSON_ParseWithLength(input, input_len);
	if (root_obj == NULL) {
		return -ENOENT;
	}

	/* Verify that the incoming JSON string is an object. */
	if (!cJSON_IsObject(root_obj)) {
		return -ENOENT;
	}

	if (has_shadow_update_been_handled(root_obj)) {
		err = -ECANCELED;
		goto exit;
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

	json_common_config_get(subgroup_obj, data);

exit:
	cJSON_Delete(root_obj);
	return err;
}

int cloud_codec_encode_config(struct cloud_codec_data *output,
			      struct cloud_data_cfg *data)
{
	int err;
	char *buffer;

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *rep_obj = cJSON_CreateObject();

	if (root_obj == NULL || state_obj == NULL || rep_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(rep_obj);
		return -ENOMEM;
	}

	err = json_common_config_add(rep_obj, data, DATA_CONFIG);

	json_add_obj(state_obj, OBJECT_REPORTED, rep_obj);
	json_add_obj(root_obj, OBJECT_STATE, state_obj);

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
			    struct cloud_data_accelerometer *accel_buf,
			    struct cloud_data_battery *bat_buf)
{
	int err;
	char *buffer;
	bool msg_obj_added = false;
	bool state_obj_added = false;

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *rep_obj = cJSON_CreateObject();
	cJSON *dev_obj = cJSON_CreateObject();

	cJSON *modem_static_ref = NULL;
	cJSON *modem_dynamic_ref = NULL;
	cJSON *data_ref = NULL;

	if (root_obj == NULL || state_obj == NULL || rep_obj == NULL || dev_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(rep_obj);
		cJSON_Delete(dev_obj);
		return -ENOMEM;
	}

	/******************************************************************************************/

	/* TEMPHACK - Add GPS, humidity, and temperateure to root object and unpack in nrf cloud
	 * integration layer before it is adressed to the right endpoint.
	 */

	/* GPS NMEA */

	err = add_data(root_obj, APP_ID_GPS, gps_buf->nmea, &gps_buf->gps_ts, gps_buf->queued,
		       OBJECT_MSG_GPS);
	if (err == 0) {
		msg_obj_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}
	gps_buf->queued = false;

	/* Humidity and Temperateure */

	/* Convert to humidity and temperateure to string format. */
	char humidity[10];
	char temperature[10];

	snprintf(humidity, sizeof(humidity), "%f", sensor_buf->hum);
	snprintf(temperature, sizeof(temperature), "%f", sensor_buf->temp);

	/* Make a copy of the sensor timestamp. Timestamps are converted before encoding add
	 * humidity and temperature share the same timestamp.
	 */
	int64_t ts_temp = sensor_buf->env_ts;

	err = add_data(root_obj, APP_ID_HUMIDITY, humidity, &sensor_buf->env_ts, sensor_buf->queued,
		       OBJECT_MSG_HUMID);
	if (err == 0) {
		msg_obj_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}

	err = add_data(root_obj, APP_ID_TEMPERATURE, temperature, &ts_temp, sensor_buf->queued,
		       OBJECT_MSG_TEMP);
	if (err == 0) {
		msg_obj_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}
	sensor_buf->queued = false;

	/* Separate RSRP from static modem data if the entry is queued and encode as a separate
	 * message.
	 */
	if (modem_dyn_buf->queued && modem_dyn_buf->rsrp_fresh) {
		char rsrp[7];

		/* Make a copy of the timestamp to avoid error when converting the relative
		 * timestamp twice with date_time_uptime_to_unix_time_ms().
		 */
		int64_t rsrp_ts = modem_dyn_buf->ts;

		snprintf(rsrp, sizeof(rsrp), "%d", modem_dyn_buf->rsrp);

		err = add_data(root_obj, APP_ID_RSRP, rsrp, &rsrp_ts, true, OBJECT_MSG_RSRP);
		if (err == 0) {
			msg_obj_added = true;
		} else if (err != -ENODATA) {
			goto add_object;
		}
	}

	/******************************************************************************************/

	/* deviceInfo */

	err = json_common_modem_static_data_add(NULL, modem_stat_buf,
						JSON_COMMON_GET_POINTER_TO_OBJECT,
						NULL,
						&modem_static_ref);
	if (err == 0) {
		state_obj_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}

	data_ref = cJSON_DetachItemFromObject(modem_static_ref, DATA_VALUE);
	cJSON_Delete(modem_static_ref);

	if (data_ref != NULL) {
		/* Temporary add battery data to deviceInfo. */
		if (bat_buf->queued) {
			err = json_add_number(data_ref, DATA_BATTERY, bat_buf->bat);
			if (err) {
				cJSON_Delete(data_ref);
				goto add_object;
			}
			bat_buf->queued = false;
		}

		json_add_obj(dev_obj, DATA_MODEM_STATIC, data_ref);
	} else {
		/* If deviceInfo is empty but battery data is queued, we create a deviceInfo object
		 * and populate it with battery data.
		 */
		if (bat_buf->queued) {

			data_ref = cJSON_CreateObject();
			if (data_ref == NULL) {
				err = -ENOMEM;
				goto add_object;
			}

			err = json_add_number(data_ref, DATA_BATTERY, bat_buf->bat);
			if (err) {
				cJSON_Delete(data_ref);
				goto add_object;
			}

			state_obj_added = true;
			json_add_obj(dev_obj, DATA_MODEM_STATIC, data_ref);
			bat_buf->queued = false;
		}
	}

	/* networkInfo */

	err = json_common_modem_dynamic_data_add(NULL, modem_dyn_buf,
						 JSON_COMMON_GET_POINTER_TO_OBJECT,
						 NULL,
						 &modem_dynamic_ref);
	if (err == 0) {
		state_obj_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}

	data_ref = cJSON_DetachItemFromObject(modem_dynamic_ref, DATA_VALUE);
	cJSON_Delete(modem_dynamic_ref);
	if (data_ref != NULL) {
		json_add_obj(dev_obj, DATA_MODEM_DYNAMIC, data_ref);
	}

add_object:

	/* Delete state object if empty. */
	if (state_obj_added) {
		json_add_obj(rep_obj, OBJECT_DEVICE, dev_obj);
		json_add_obj(state_obj, OBJECT_REPORTED, rep_obj);
		json_add_obj(root_obj, OBJECT_STATE, state_obj);
	} else {
		cJSON_Delete(dev_obj);
		cJSON_Delete(rep_obj);
		cJSON_Delete(state_obj);
	}

	/* Check error code after all objects has been added to the root object.
	 * This way, we only need to clear the root_object upon an error.
	 */
	if ((err != 0) && (err != -ENODATA)) {
		goto exit;
	}

	if (!state_obj_added && !msg_obj_added) {
		err = -ENODATA;
		LOG_DBG("No data to encode, JSON string empty...");
		goto exit;
	} else {
		/* At this point err can be either 0 or -ENODATA. Explicitly set err to 0 if
		 * objects has been added to the rootj object.
		 */
		err = 0;
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
	int err;
	char *buffer;
	cJSON *root_obj = NULL;

	if (!ui_buf->queued) {
		err = -ENODATA;
		goto exit;
	}

	root_obj = cJSON_CreateObject();
	if (root_obj == NULL) {
		return -ENOMEM;
	}

	/* Convert to string format. */
	char button[2];

	snprintk(button, sizeof(button), "%d", ui_buf->btn);

	err = json_add_str(root_obj, DATA_TYPE, button);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = add_meta_data(root_obj, APP_ID_BUTTON, &ui_buf->btn_ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	buffer = cJSON_PrintUnformatted(root_obj);
	if (buffer == NULL) {
		LOG_ERR("Failed to allocate memory for JSON string");

		err = -ENOMEM;
		goto exit;
	}

	ui_buf->queued = false;

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
	int err;
	char *buffer;
	bool object_added = false;

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		return -ENOMEM;
	}

	err = json_common_batch_data_add(root_obj, JSON_COMMON_MODEM_DYNAMIC,
					 modem_dyn_buf, modem_dyn_buf_count,
					 DATA_MODEM_DYNAMIC);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto exit;
	}

	err = json_common_batch_data_add(root_obj, JSON_COMMON_GPS,
					 gps_buf, gps_buf_count,
					 DATA_GPS);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto exit;
	}

	err = json_common_batch_data_add(root_obj, JSON_COMMON_SENSOR,
					 sensor_buf, sensor_buf_count,
					 DATA_ENVIRONMENTALS);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto exit;
	}

	err = json_common_batch_data_add(root_obj, JSON_COMMON_UI,
					 ui_buf, ui_buf_count,
					 DATA_BUTTON);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto exit;
	}

	err = json_common_batch_data_add(root_obj, JSON_COMMON_BATTERY,
					 bat_buf, bat_buf_count,
					 DATA_BATTERY);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto exit;
	}

	err = json_common_batch_data_add(root_obj, JSON_COMMON_ACCELEROMETER,
					 accel_buf, accel_buf_count,
					 DATA_MOVEMENT);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto exit;
	}

	if (!object_added) {
		err = -ENODATA;
		LOG_DBG("No data to encode, JSON string empty...");
		goto exit;
	} else {
		/* At this point err can be either 0 or -ENODATA. Explicitly set err to 0 if
		 * objects has been added to the root object to avoid propagating -ENODATA.
		 */
		err = 0;
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
