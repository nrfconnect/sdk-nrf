/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <date_time.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_location.h>
#include <cloud_codec.h>

#include "cJSON.h"
#include "json_helpers.h"
#include "json_protocol_names.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cloud_codec, CONFIG_CLOUD_CODEC_LOG_LEVEL);

/* Data types that are supported in batch messages. */
enum batch_data_type {
	GNSS,
	ENVIRONMENTALS,
	BUTTON,
	/* Only dynamic modem data is handled here. Static modem data is handled
	 * in the nrf_cloud library; see CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS.
	 */
	MODEM_DYNAMIC,
	VOLTAGE,
	IMPACT,
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

static int add_pvt_data(cJSON *parent, struct cloud_data_gnss *gnss)
{
	int err;
	struct nrf_cloud_gnss_data gnss_pvt = {
		.type = NRF_CLOUD_GNSS_TYPE_PVT,
		.ts_ms = NRF_CLOUD_NO_TIMESTAMP,
		.pvt = {
			.lon =		gnss->pvt.longi,
			.lat =		gnss->pvt.lat,
			.accuracy =	gnss->pvt.acc,
			.alt =		gnss->pvt.alt,
			.has_alt =	1,
			.speed =	gnss->pvt.spd,
			.has_speed =	1,
			.heading =	gnss->pvt.hdg,
			.has_heading =	1
		}
	};
	cJSON *data_obj = cJSON_CreateObject();

	if (data_obj == NULL) {
		return -ENOMEM;
	}

	err = date_time_uptime_to_unix_time_ms(&gnss->gnss_ts);
	if (err) {
		LOG_WRN("date_time_uptime_to_unix_time_ms, error: %d", err);
	} else {
		gnss_pvt.ts_ms = gnss->gnss_ts;
	}

	/* Encode the location data into a device message */
	err = nrf_cloud_gnss_msg_json_encode(&gnss_pvt, data_obj);
	if (err) {
		LOG_ERR("nrf_cloud_gnss_msg_json_encode, error: %d", err);
		cJSON_Delete(data_obj);
		return err;
	}

	json_add_obj_array(parent, data_obj);
	return 0;
}

static int modem_dynamic_data_add(struct cloud_data_modem_dynamic *data, cJSON **val_obj_ref)
{
	int err;
	uint32_t mccmnc;
	char *end_ptr;

	if (!data->queued) {
		return -ENODATA;
	}

	err = date_time_uptime_to_unix_time_ms(&data->ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *modem_val_obj = cJSON_CreateObject();

	if (modem_val_obj == NULL) {
		return -ENOMEM;
	}

	err = json_add_number(modem_val_obj, MODEM_CURRENT_BAND, data->band);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_str(modem_val_obj, MODEM_NETWORK_MODE,
			   (data->nw_mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" :
			   (data->nw_mode == LTE_LC_LTE_MODE_NBIOT) ? "NB-IoT" : "Unknown");
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(modem_val_obj, MODEM_RSRP, data->rsrp);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(modem_val_obj, MODEM_AREA_CODE, data->area);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	/* Convert mccmnc to unsigned long integer. */
	errno = 0;
	mccmnc = strtoul(data->mccmnc, &end_ptr, 10);

	if ((errno == ERANGE) || (*end_ptr != '\0')) {
		LOG_ERR("MCCMNC string could not be converted.");
		err = -ENOTEMPTY;
		goto exit;
	}

	err = json_add_number(modem_val_obj, MODEM_MCCMNC, mccmnc);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(modem_val_obj, MODEM_CELL_ID, data->cell);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_str(modem_val_obj, MODEM_IP_ADDRESS, data->ip);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	*val_obj_ref = modem_val_obj;

	data->queued = false;

	return 0;

exit:
	cJSON_Delete(modem_val_obj);
	return err;
}

static int config_add(cJSON *parent, struct cloud_data_cfg *data, const char *object_label)
{
	int err;

	if (object_label == NULL) {
		LOG_WRN("Missing object label");
		return -EINVAL;
	}

	cJSON *config_obj = cJSON_CreateObject();

	if (config_obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	err = json_add_bool(config_obj, CONFIG_DEVICE_MODE, data->active_mode);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(config_obj, CONFIG_LOCATION_TIMEOUT, data->location_timeout);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(config_obj, CONFIG_ACTIVE_TIMEOUT, data->active_wait_timeout);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(config_obj, CONFIG_MOVE_RES, data->movement_resolution);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(config_obj, CONFIG_MOVE_TIMEOUT, data->movement_timeout);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(config_obj,
			      CONFIG_ACC_ACT_THRESHOLD, data->accelerometer_activity_threshold);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(config_obj,
			      CONFIG_ACC_INACT_THRESHOLD, data->accelerometer_inactivity_threshold);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(config_obj,
			      CONFIG_ACC_INACT_TIMEOUT, data->accelerometer_inactivity_timeout);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	cJSON *nod_list = cJSON_CreateArray();

	if (nod_list == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	/* If a flag in the no_data structure is set to true the corresponding JSON entry is
	 * added to the no data array configuration.
	 */
	if (data->no_data.gnss) {
		cJSON *gnss_str = cJSON_CreateString(CONFIG_NO_DATA_LIST_GNSS);

		if (gnss_str == NULL) {
			cJSON_Delete(nod_list);
			err = -ENOMEM;
			goto exit;
		}

		json_add_obj_array(nod_list, gnss_str);
	}

	if (data->no_data.neighbor_cell) {
		cJSON *ncell_str = cJSON_CreateString(CONFIG_NO_DATA_LIST_NEIGHBOR_CELL);

		if (ncell_str == NULL) {
			cJSON_Delete(nod_list);
			err = -ENOMEM;
			goto exit;
		}

		json_add_obj_array(nod_list, ncell_str);
	}

	if (data->no_data.wifi) {
		cJSON *wifi_str = cJSON_CreateString(CONFIG_NO_DATA_LIST_WIFI);

		if (wifi_str == NULL) {
			cJSON_Delete(nod_list);
			err = -ENOMEM;
			goto exit;
		}

		json_add_obj_array(nod_list, wifi_str);
	}

	/* If there are no flag set in the no_data structure, an empty array is encoded. */
	json_add_obj(config_obj, CONFIG_NO_DATA_LIST, nod_list);
	json_add_obj(parent, object_label, config_obj);

	return 0;

exit:
	cJSON_Delete(config_obj);
	return err;
}

static void config_get(cJSON *parent, struct cloud_data_cfg *data)
{
	cJSON *location_timeout = cJSON_GetObjectItem(parent, CONFIG_LOCATION_TIMEOUT);
	cJSON *active = cJSON_GetObjectItem(parent, CONFIG_DEVICE_MODE);
	cJSON *active_wait = cJSON_GetObjectItem(parent, CONFIG_ACTIVE_TIMEOUT);
	cJSON *move_res = cJSON_GetObjectItem(parent, CONFIG_MOVE_RES);
	cJSON *move_timeout = cJSON_GetObjectItem(parent, CONFIG_MOVE_TIMEOUT);
	cJSON *acc_act_thres = cJSON_GetObjectItem(parent, CONFIG_ACC_ACT_THRESHOLD);
	cJSON *acc_inact_thres = cJSON_GetObjectItem(parent, CONFIG_ACC_INACT_THRESHOLD);
	cJSON *acc_inact_timeout = cJSON_GetObjectItem(parent, CONFIG_ACC_INACT_TIMEOUT);
	cJSON *nod_list = cJSON_GetObjectItem(parent, CONFIG_NO_DATA_LIST);

	if (location_timeout != NULL) {
		data->location_timeout = location_timeout->valueint;
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

	if (acc_act_thres != NULL) {
		data->accelerometer_activity_threshold = acc_act_thres->valuedouble;
	}

	if (acc_inact_thres != NULL) {
		data->accelerometer_inactivity_threshold = acc_inact_thres->valuedouble;
	}

	if (acc_inact_timeout != NULL) {
		data->accelerometer_inactivity_timeout = acc_inact_timeout->valuedouble;
	}

	if (nod_list != NULL && cJSON_IsArray(nod_list)) {
		cJSON *item;
		bool gnss_found = false;
		bool ncell_found = false;
		bool wifi_found = false;

		for (int i = 0; i < cJSON_GetArraySize(nod_list); i++) {
			item = cJSON_GetArrayItem(nod_list, i);

			if (strcmp(item->valuestring, CONFIG_NO_DATA_LIST_GNSS) == 0) {
				gnss_found = true;
			}

			if (strcmp(item->valuestring, CONFIG_NO_DATA_LIST_NEIGHBOR_CELL) == 0) {
				ncell_found = true;
			}

			if (strcmp(item->valuestring, CONFIG_NO_DATA_LIST_WIFI) == 0) {
				wifi_found = true;
			}
		}

		/* If a supported entry is present in the no data list we set the corresponding flag
		 * to true. Signifying that no data is to be sampled for that data type.
		 */
		data->no_data.gnss = gnss_found;
		data->no_data.neighbor_cell = ncell_found;
		data->no_data.wifi = wifi_found;
	}
}

static int add_batch_data(cJSON *array, enum batch_data_type type, void *buf, size_t buf_count)
{
	for (int i = 0; i < buf_count; i++) {
		switch (type) {
		case GNSS: {
			int err;
			struct cloud_data_gnss *data = (struct cloud_data_gnss *)buf;

			if (data[i].queued == false) {
				break;
			}

			err =  add_pvt_data(array, &data[i]);
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
		case IMPACT: {
			int err, len;
			char magnitude[10];
			struct cloud_data_impact *data = (struct cloud_data_impact *)buf;

			if (data[i].queued == false) {
				break;
			}

			err = date_time_uptime_to_unix_time_ms(&data[i].ts);
			if (err) {
				LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
				return -EOVERFLOW;
			}

			len = snprintk(magnitude, sizeof(magnitude), "%.2f",
				       data[i].magnitude);
			if ((len < 0) || (len >= sizeof(magnitude))) {
				LOG_ERR("Cannot convert magnitude to string, buffer too small");
				return -ERANGE;
			}

			err = add_data(array, NULL, APP_ID_IMPACT, magnitude,
				       &data[i].ts, data[i].queued, NULL, false);
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
		case MODEM_DYNAMIC: {
			int err, len;
			char rsrp[5];
			cJSON *data_ref = NULL;
			struct cloud_data_modem_dynamic *modem_dynamic =
				(struct cloud_data_modem_dynamic *)buf;

			err = modem_dynamic_data_add(&modem_dynamic[i], &data_ref);
			if (err && err != -ENODATA) {
				return err;
			} else if (err == -ENODATA) {
				break;
			}

			err = add_data(array, data_ref, APP_ID_DEVICE, NULL, &modem_dynamic[i].ts,
				       true, DATA_MODEM_DYNAMIC, false);
			if (err && err != -ENODATA) {
				cJSON_Delete(data_ref);
				return err;
			}

			/* Retrieve and construct RSRP APP_ID message from dynamic modem data */
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

int cloud_codec_init(struct cloud_data_cfg *cfg, cloud_codec_evt_handler_t event_handler)
{
	ARG_UNUSED(cfg);
	ARG_UNUSED(event_handler);

	cJSON_Init();
	return 0;
}

int cloud_codec_encode_cloud_location(
	struct cloud_codec_data *output,
	struct cloud_data_cloud_location *cloud_location)
{
#if defined(CONFIG_NRF_CLOUD_LOCATION)
	int err;
	char *buffer;
	cJSON *root_obj = NULL;

	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(cloud_location != NULL);

	struct lte_lc_cells_info cell_info;

	if (cloud_location->neighbor_cells_valid) {
		cell_info = cloud_location->neighbor_cells.cell_data;
		cell_info.neighbor_cells = cloud_location->neighbor_cells.neighbor_cells;
	}

#if defined(CONFIG_LOCATION_METHOD_WIFI)
	struct wifi_scan_info wifi_info;

	if (cloud_location->wifi_access_points_valid) {
		wifi_info.ap_info = cloud_location->wifi_access_points.ap_info;
		wifi_info.cnt = cloud_location->wifi_access_points.cnt;
	}
#endif

	if (!cloud_location->queued) {
		return -ENODATA;
	}

	err = nrf_cloud_location_request_json_get(
		cloud_location->neighbor_cells_valid ? &cell_info : NULL,
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		cloud_location->wifi_access_points_valid ? &wifi_info : NULL,
#else
		NULL,
#endif
		true,
		&root_obj);
	if (err) {
		LOG_ERR("nrf_cloud_location_request_json_get, error: %d", err);
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
	cloud_location->queued = false;
	cJSON_Delete(root_obj);
	return err;
#endif /* CONFIG_NRF_CLOUD_LOCATION */

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

int cloud_codec_decode_config(const char *input, size_t input_len,
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

	/* Check for an nRF Cloud message */
	if ((json_object_decode(root_obj, DATA_GROUP) != NULL) ||
	    (json_object_decode(root_obj, DATA_ID) != NULL)) {
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

	config_get(subgroup_obj, cfg);

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

	err = config_add(rep_obj, data, DATA_CONFIG);

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
			    struct cloud_data_impact *impact_buf,
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
	char button[2] = "";

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

int cloud_codec_encode_impact_data(struct cloud_codec_data *output,
				   struct cloud_data_impact *impact_buf)
{
	int err, len;
	char *buffer;
	cJSON *root_obj = NULL;
	char magnitude[10];

	if (!impact_buf->queued) {
		err = -ENODATA;
		goto exit;
	}

	root_obj = cJSON_CreateObject();
	if (root_obj == NULL) {
		return -ENOMEM;
	}

	len = snprintk(magnitude, sizeof(magnitude), "%.2f", impact_buf->magnitude);
	if ((len < 0) || (len >= sizeof(magnitude))) {
		LOG_ERR("Cannot convert magnitude to string, buffer too small");
		err = -ERANGE;
		goto exit;
	}

	err = json_add_str(root_obj, DATA_TYPE, magnitude);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = add_meta_data(root_obj, APP_ID_IMPACT, &impact_buf->ts, true);
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

	impact_buf->queued = false;

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
	ARG_UNUSED(modem_stat_buf);

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

	err = add_batch_data(root_array, IMPACT, impact_buf, impact_buf_count);
	if (err) {
		LOG_ERR("Failed adding button data to array, error: %d", err);
		goto exit;
	}

	err = add_batch_data(root_array, VOLTAGE, bat_buf, bat_buf_count);
	if (err) {
		LOG_ERR("Failed adding battery data to array, error: %d", err);
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
	cJSON_Delete(root_array);
	return err;
}
