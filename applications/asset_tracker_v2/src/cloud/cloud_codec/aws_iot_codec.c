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

int cloud_codec_encode_neighbor_cells(struct cloud_codec_data *output,
				      struct cloud_data_neighbor_cells *neighbor_cells)
{
	int err;
	char *buffer;

	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(neighbor_cells != NULL);

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	err = json_common_neighbor_cells_data_add(root_obj, neighbor_cells,
						  JSON_COMMON_ADD_DATA_TO_OBJECT);
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
	bool object_added = false;

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *rep_obj = cJSON_CreateObject();

	if (root_obj == NULL || state_obj == NULL || rep_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(rep_obj);
		return -ENOMEM;
	}

	err = json_common_ui_data_add(rep_obj, ui_buf,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      DATA_BUTTON,
				      NULL);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}

	err = json_common_modem_static_data_add(rep_obj, modem_stat_buf,
						JSON_COMMON_ADD_DATA_TO_OBJECT,
						DATA_MODEM_STATIC,
						NULL);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}

	err = json_common_modem_dynamic_data_add(rep_obj, modem_dyn_buf,
						 JSON_COMMON_ADD_DATA_TO_OBJECT,
						 DATA_MODEM_DYNAMIC,
						 NULL);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}

	err = json_common_gps_data_add(rep_obj, gps_buf,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       DATA_GPS,
				       NULL);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}

	err = json_common_sensor_data_add(rep_obj, sensor_buf,
					  JSON_COMMON_ADD_DATA_TO_OBJECT,
					  DATA_ENVIRONMENTALS,
					  NULL);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}

	err = json_common_accel_data_add(rep_obj, accel_buf,
					 JSON_COMMON_ADD_DATA_TO_OBJECT,
					 DATA_MOVEMENT,
					 NULL);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}

	err = json_common_battery_data_add(rep_obj, bat_buf,
					   JSON_COMMON_ADD_DATA_TO_OBJECT,
					   DATA_BATTERY,
					   NULL);
	if (err == 0) {
		object_added = true;
	} else if (err != -ENODATA) {
		goto add_object;
	}

add_object:

	json_add_obj(state_obj, OBJECT_REPORTED, rep_obj);
	json_add_obj(root_obj, OBJECT_STATE, state_obj);

	/* Check error code after all objects has been added to the root object.
	 * This way, we only need to clear the root_object upon an error.
	 */
	if ((err != 0) && (err != -ENODATA)) {
		goto exit;
	}

	if (!object_added) {
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

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	err = json_common_ui_data_add(root_obj, ui_buf,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      DATA_BUTTON,
				      NULL);
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
	int err;
	char *buffer;
	bool object_added = false;

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		cJSON_Delete(root_obj);
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
		json_print_obj("Encoded batch message:\n", root_obj);
	}

	output->buf = buffer;
	output->len = strlen(buffer);

exit:
	cJSON_Delete(root_obj);
	return err;
}
