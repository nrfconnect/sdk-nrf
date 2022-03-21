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
#include <net/nrf_cloud_cell_pos.h>
#include <cloud_codec.h>

#include "cJSON.h"
#include "json_helpers.h"
#include "json_common.h"
#include "json_protocol_names.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cloud_codec, CONFIG_CLOUD_CODEC_LOG_LEVEL);

/* Data types that are supported in batch messages. */
enum batch_data_type {
	GNSS,
	ENVIRONMENTALS,
	BUTTON,
	MODEM_STATIC,
	MODEM_DYNAMIC,
	VOLTAGE,
};

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

static int add_meta_data(cJSON *parent, const char *app_id, int64_t *timestamp, bool convert_time)
{
	int err;

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

	if (timestamp != NULL) {
		if (convert_time) {
			err = date_time_uptime_to_unix_time_ms(timestamp);
			if (err) {
				LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
				return err;
			}
		}

		err = json_add_number(parent, DATA_TIMESTAMP, *timestamp);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			return err;
		}
	}

	return 0;
}

static int add_data(cJSON *parent, cJSON *child, const char *app_id, const char *str_val,
		    int64_t *timestamp, bool queued, const char *object_name, bool convert_time)
{
	int err;

	if (!queued) {
		return -ENODATA;
	}

	cJSON *data_obj = cJSON_CreateObject();

	if (data_obj == NULL) {
		return -ENOMEM;
	}

	err = add_meta_data(data_obj, app_id, timestamp, convert_time);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	if (str_val != NULL) {
		err = json_add_str(data_obj, DATA_TYPE, str_val);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
	}

	if (child != NULL) {
		cJSON *content_obj = cJSON_CreateObject();

		if (content_obj == NULL) {
			err = -ENOMEM;
			goto exit;
		}

		json_add_obj(content_obj, object_name, child);
		json_add_obj(data_obj, DATA_TYPE, content_obj);
	}

	json_add_obj_array(parent, data_obj);

	return 0;

exit:
	cJSON_Delete(data_obj);
	return err;
}

static int add_batch_data(cJSON *array, enum batch_data_type type, void *buf, size_t buf_count)
{
	for (int i = 0; i < buf_count; i++) {
		switch (type) {
		case GNSS: {
			int err;
			struct cloud_data_gnss *data = (struct cloud_data_gnss *)buf;

			err =  add_data(array, NULL, APP_ID_GPS, data[i].nmea,
					&data[i].gnss_ts, data[i].queued, NULL, true);
			if (err && err != -ENODATA) {
				return err;
			}

			data[i].queued = false;
			break;
		}
		case ENVIRONMENTALS: {
			int err, len;
			char humidity[10];
			char temperature[10];
			char pressure[10];
			char bsec_air_quality[4];
			struct cloud_data_sensors *data = (struct cloud_data_sensors *)buf;

			if (data[i].queued == false) {
				break;
			}

			err = date_time_uptime_to_unix_time_ms(&data[i].env_ts);
			if (err) {
				LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
				return -EOVERFLOW;
			}

			len = snprintk(humidity, sizeof(humidity), "%.2f",
				       data[i].humidity);
			if ((len < 0) || (len >= sizeof(humidity))) {
				LOG_ERR("Cannot convert humidity to string, buffer too small");
			}

			len = snprintk(temperature, sizeof(temperature), "%.2f",
				       data[i].temperature);
			if ((len < 0) || (len >= sizeof(temperature))) {
				LOG_ERR("Cannot convert temperature to string, buffer too small");
			}

			len = snprintk(pressure, sizeof(pressure), "%.2f",
				       data[i].pressure);
			if ((len < 0) || (len >= sizeof(pressure))) {
				LOG_ERR("Cannot convert pressure to string, buffer too small");
			}

			if (data[i].bsec_air_quality >= 0) {
				len = snprintk(bsec_air_quality, sizeof(bsec_air_quality), "%d",
					data[i].bsec_air_quality);
				if ((len < 0) || (len >= sizeof(bsec_air_quality))) {
					LOG_ERR("Cannot convert BSEC air quality to string, "
						"buffer too small");
				}

				err = add_data(array, NULL, APP_ID_AIR_QUAL, bsec_air_quality,
					       &data[i].env_ts, data[i].queued, NULL, false);
				if (err && err != -ENODATA) {
					return err;
				}
			}

			err = add_data(array, NULL, APP_ID_HUMIDITY, humidity,
				       &data[i].env_ts, data[i].queued, NULL, false);
			if (err && err != -ENODATA) {
				return err;
			}

			err = add_data(array, NULL, APP_ID_TEMPERATURE, temperature,
				       &data[i].env_ts, data[i].queued, NULL, false);
			if (err && err != -ENODATA) {
				return err;
			}

			err =  add_data(array, NULL, APP_ID_AIR_PRESS, pressure,
					&data[i].env_ts, data[i].queued, NULL, false);
			if (err && err != -ENODATA) {
				return err;
			}

			data[i].queued = false;
			break;
		}
		case BUTTON: {
			int err, len;
			char button[2];
			struct cloud_data_ui *data = (struct cloud_data_ui *)buf;

			len = snprintk(button, sizeof(button), "%d", data[i].btn);
			if ((len < 0) || (len >= sizeof(button))) {
				LOG_ERR("Cannot convert button number to string, buffer too small");
				return -ENOMEM;
			}

			err =  add_data(array, NULL, APP_ID_BUTTON, button,
					&data[i].btn_ts, data[i].queued, NULL, true);
			if (err && err != -ENODATA) {
				return err;
			}

			data[i].queued = false;
			break;
		}
		case MODEM_STATIC: {
			int err;
			cJSON *modem_static_ref = NULL;
			cJSON *data_ref = NULL;
			struct cloud_data_modem_static *modem_static =
						(struct cloud_data_modem_static *)buf;

			err = json_common_modem_static_data_add(NULL, modem_static,
								JSON_COMMON_GET_POINTER_TO_OBJECT,
								NULL, &modem_static_ref);
			if (err && err != -ENODATA) {
				return err;
			} else if (err == -ENODATA) {
				break;
			}

			data_ref = cJSON_DetachItemFromObject(modem_static_ref, DATA_VALUE);
			cJSON_Delete(modem_static_ref);

			if (data_ref == NULL) {
				LOG_ERR("Cannot find object: %s", DATA_VALUE);
				return -ENOENT;
			}

			err = add_data(array, data_ref, APP_ID_DEVICE, NULL, &modem_static->ts,
				       true, DATA_MODEM_STATIC, false);
			if (err && err != -ENODATA) {
				cJSON_Delete(data_ref);
				return err;
			}

			break;
		}
		case MODEM_DYNAMIC: {
			int err, len;
			char rsrp[5];
			cJSON *modem_dynamic_ref = NULL;
			cJSON *data_ref = NULL;
			struct cloud_data_modem_dynamic *modem_dynamic =
				(struct cloud_data_modem_dynamic *)buf;

			err = json_common_modem_dynamic_data_add(NULL, &modem_dynamic[i],
								 JSON_COMMON_GET_POINTER_TO_OBJECT,
								 NULL, &modem_dynamic_ref);
			if (err && err != -ENODATA) {
				return err;
			} else if (err == -ENODATA) {
				break;
			}

			data_ref = cJSON_DetachItemFromObject(modem_dynamic_ref, DATA_VALUE);
			cJSON_Delete(modem_dynamic_ref);

			if (data_ref == NULL) {
				LOG_ERR("Cannot find object: %s", DATA_VALUE);
				return -ENOENT;
			}

			err = add_data(array, data_ref, APP_ID_DEVICE, NULL, &modem_dynamic[i].ts,
				       true, DATA_MODEM_DYNAMIC, false);
			if (err && err != -ENODATA) {
				cJSON_Delete(data_ref);
				return err;
			}

			/* Retrieve and construct RSRP APP_ID message from dynamic modem data */
			if (modem_dynamic[i].rsrp_fresh) {
				len = snprintk(rsrp, sizeof(rsrp), "%d", modem_dynamic[i].rsrp);
				if ((len < 0) || (len >= sizeof(rsrp))) {
					LOG_ERR("Cannot convert RSRP value, buffer too small");
					return -ENOMEM;
				}

				err = add_data(array, NULL, APP_ID_RSRP, rsrp, &modem_dynamic[i].ts,
					       true, NULL, false);
				if (err && err != -ENODATA) {
					return err;
				}
			}

			break;
		}
		case VOLTAGE: {
			int err, len;
			char voltage[5];
			struct cloud_data_battery *data = (struct cloud_data_battery *)buf;

			len = snprintk(voltage, sizeof(voltage), "%d", data[i].bat);
			if ((len < 0) || (len >= sizeof(voltage))) {
				LOG_ERR("Cannot convert voltage to string, buffer too small");
				return -ENOMEM;
			}

			err = add_data(array, NULL, APP_ID_VOLTAGE, voltage, &data[i].bat_ts,
				       data[i].queued, NULL, true);
			if (err && err != -ENODATA) {
				return err;
			}

			data[i].queued = false;
			break;
		}
		default:
			LOG_ERR("Unknown batch data type");
			return -EINVAL;
		}
	}

	return 0;
}

int cloud_codec_encode_neighbor_cells(struct cloud_codec_data *output,
				      struct cloud_data_neighbor_cells *neighbor_cells)
{
 #if defined(NRF_CLOUD_CELL_POS)
	int err;
	char *buffer;
	cJSON *root_obj = NULL;

	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(neighbor_cells != NULL);

	struct lte_lc_cells_info info = neighbor_cells->cell_data;

	info.neighbor_cells = neighbor_cells->neighbor_cells;

	if (!neighbor_cells->queued) {
		return -ENODATA;
	}

	/* Set the request location flag when encoding the cellular position request.
	 * In general, the application does not care about
	 * getting the location back from cellular position requests. However, this is
	 * needed to ensure that the cellular location of the application
	 * is visualized in the nRF Cloud web UI.
	 */
	err = nrf_cloud_cell_pos_request_json_get(&info, true, &root_obj);
	if (err) {
		LOG_ERR("nrf_cloud_cell_pos_request_json_get, error: %d", err);
		return -ENOMEM;
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
	neighbor_cells->queued = false;
	cJSON_Delete(root_obj);
	return err;
#endif /* CONFIG_NRF_CLOUD_CELL_POS */

	return -ENOTSUP;
}

int cloud_codec_encode_agps_request(struct cloud_codec_data *output,
				    struct cloud_data_agps_request *agps_request)
{
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(agps_request != NULL);

	agps_request->queued = false;
	return -ENOTSUP;
}

int cloud_codec_encode_pgps_request(struct cloud_codec_data *output,
				    struct cloud_data_pgps_request *pgps_request)
{
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(pgps_request != NULL);

	pgps_request->queued = false;

	return -ENOTSUP;
}

int cloud_codec_decode_config(char *input, size_t input_len,
			      struct cloud_data_cfg *cfg)
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

	json_common_config_get(subgroup_obj, cfg);

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
			    struct cloud_data_gnss *gnss_buf,
			    struct cloud_data_sensors *sensor_buf,
			    struct cloud_data_modem_static *modem_stat_buf,
			    struct cloud_data_modem_dynamic *modem_dyn_buf,
			    struct cloud_data_ui *ui_buf,
			    struct cloud_data_accelerometer *accel_buf,
			    struct cloud_data_battery *bat_buf)
{
	/* Encoding of the latest buffer entries is not supported.
	 * Only batch encoding is supported.
	 */
	return -ENOTSUP;
}

int cloud_codec_encode_ui_data(struct cloud_codec_data *output,
			       struct cloud_data_ui *ui_buf)
{
	int err, len;
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

	len = snprintk(button, sizeof(button), "%d", ui_buf->btn);
	if ((len < 0) || (len >= sizeof(button))) {
		LOG_ERR("Cannot convert button number to string, buffer too small");
		err = -ENOMEM;
		goto exit;
	}

	err = json_add_str(root_obj, DATA_TYPE, button);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = add_meta_data(root_obj, APP_ID_BUTTON, &ui_buf->btn_ts, true);
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

int cloud_codec_encode_batch_data(struct cloud_codec_data *output,
				  struct cloud_data_gnss *gnss_buf,
				  struct cloud_data_sensors *sensor_buf,
				  struct cloud_data_modem_static *modem_stat_buf,
				  struct cloud_data_modem_dynamic *modem_dyn_buf,
				  struct cloud_data_ui *ui_buf,
				  struct cloud_data_accelerometer *accel_buf,
				  struct cloud_data_battery *bat_buf,
				  size_t gnss_buf_count,
				  size_t sensor_buf_count,
				  size_t modem_stat_buf_count,
				  size_t modem_dyn_buf_count,
				  size_t ui_buf_count,
				  size_t accel_buf_count,
				  size_t bat_buf_count)
{
	int err;
	char *buffer;

	cJSON *root_array = cJSON_CreateArray();

	if (root_array == NULL) {
		return -ENOMEM;
	}

	err = add_batch_data(root_array, GNSS, gnss_buf, gnss_buf_count);
	if (err) {
		LOG_ERR("Failed adding GNSS data to array, error: %d", err);
		goto exit;
	}

	err = add_batch_data(root_array, ENVIRONMENTALS, sensor_buf, sensor_buf_count);
	if (err) {
		LOG_ERR("Failed adding environmental data to array, error: %d", err);
		goto exit;
	}

	err = add_batch_data(root_array, BUTTON, ui_buf, ui_buf_count);
	if (err) {
		LOG_ERR("Failed adding button data to array, error: %d", err);
		goto exit;
	}

	err = add_batch_data(root_array, VOLTAGE, bat_buf, bat_buf_count);
	if (err) {
		LOG_ERR("Failed adding battery data to array, error: %d", err);
		goto exit;
	}

	err = add_batch_data(root_array, MODEM_STATIC, modem_stat_buf, modem_stat_buf_count);
	if (err) {
		LOG_ERR("Failed adding static modem data to array, error: %d", err);
		goto exit;
	}

	err = add_batch_data(root_array, MODEM_DYNAMIC, modem_dyn_buf, modem_dyn_buf_count);
	if (err) {
		LOG_ERR("Failed adding dynamic modem data to array, error: %d", err);
		goto exit;
	}

	if (cJSON_GetArraySize(root_array) == 0) {
		err = -ENODATA;
		LOG_DBG("No data to encode, JSON string empty...");
		goto exit;
	} else {
		/* At this point err can be either 0 or -ENODATA. Explicitly set err to 0 if
		 * objects has been added to the root object to avoid propagating -ENODATA.
		 */
		err = 0;
	}

	buffer = cJSON_PrintUnformatted(root_array);
	if (buffer == NULL) {
		LOG_ERR("Failed to allocate memory for JSON string");

		err = -ENOMEM;
		goto exit;
	}

	if (IS_ENABLED(CONFIG_CLOUD_CODEC_LOG_LEVEL_DBG)) {
		json_print_obj("Encoded batch message:\n", root_array);
	}

	output->buf = buffer;
	output->len = strlen(buffer);

exit:
	/* Clear buffers that are not handled by this function. */
	memset(accel_buf, 0, accel_buf_count * sizeof(struct cloud_data_accelerometer));
	cJSON_Delete(root_array);
	return err;
}
