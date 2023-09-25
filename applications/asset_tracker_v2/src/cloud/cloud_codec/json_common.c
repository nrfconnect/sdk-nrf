/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <cJSON.h>
#include <date_time.h>

#include "cloud_codec.h"
#include "json_common.h"
#include "json_helpers.h"
#include "json_protocol_names.h"
#include <net/nrf_cloud_location.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(json_common, CONFIG_CLOUD_CODEC_LOG_LEVEL);

/* MAC address string size including null character */
#define AP_STRING_SIZE	13

static int op_code_handle(cJSON *parent, enum json_common_op_code op,
			  const char *object_label, cJSON *child, cJSON **parent_ref)
{
	switch (op) {
	case JSON_COMMON_ADD_DATA_TO_ARRAY:
		if (!cJSON_IsArray(parent)) {
			LOG_WRN("Passed in parent object is not an array");
			return -EINVAL;
		}
		json_add_obj_array(parent, child);
		break;
	case JSON_COMMON_ADD_DATA_TO_OBJECT:
		if (!cJSON_IsObject(parent)) {
			LOG_WRN("Passed in parent object is not an object");
			return -EINVAL;
		}

		if (object_label == NULL) {
			LOG_WRN("Missing object label");
			return -EINVAL;
		}
		json_add_obj(parent, object_label, child);
		break;
	case JSON_COMMON_GET_POINTER_TO_OBJECT:
		if (*parent_ref != NULL) {
			LOG_WRN("Passed in parent reference is not NULL");
			return -EINVAL;
		}
		*parent_ref = child;
		break;
	default:
		LOG_WRN("OP code invalid");
		return -EINVAL;
	}

	return 0;
}

int json_common_modem_static_data_add(cJSON *parent,
				      struct cloud_data_modem_static *data,
				      enum json_common_op_code op,
				      const char *object_label,
				      cJSON **parent_ref)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	err = date_time_uptime_to_unix_time_ms(&data->ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *modem_obj = cJSON_CreateObject();
	cJSON *modem_val_obj = cJSON_CreateObject();

	if (modem_obj == NULL || modem_val_obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	err = json_add_str(modem_val_obj, MODEM_IMEI, data->imei);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_str(modem_val_obj, MODEM_ICCID, data->iccid);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_str(modem_val_obj, MODEM_FIRMWARE_VERSION, data->fw);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_str(modem_val_obj, MODEM_BOARD, data->brdv);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_str(modem_val_obj, MODEM_APP_VERSION, data->appv);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	json_add_obj(modem_obj, DATA_VALUE, modem_val_obj);

	err = json_add_number(modem_obj, DATA_TIMESTAMP, data->ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(modem_obj);
		return err;
	}

	err = op_code_handle(parent, op, object_label, modem_obj, parent_ref);
	if (err) {
		cJSON_Delete(modem_obj);
		return err;
	}

	data->queued = false;

	return 0;

exit:
	cJSON_Delete(modem_obj);
	cJSON_Delete(modem_val_obj);
	return err;
}

int json_common_modem_dynamic_data_add(cJSON *parent,
				       struct cloud_data_modem_dynamic *data,
				       enum json_common_op_code op,
				       const char *object_label,
				       cJSON **parent_ref)
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

	cJSON *modem_obj = cJSON_CreateObject();
	cJSON *modem_val_obj = cJSON_CreateObject();

	if (modem_obj == NULL || modem_val_obj == NULL) {
		err = -ENOMEM;
		goto exit;
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

	json_add_obj(modem_obj, DATA_VALUE, modem_val_obj);

	err = json_add_number(modem_obj, DATA_TIMESTAMP, data->ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(modem_obj);
		return err;
	}

	err = op_code_handle(parent, op, object_label, modem_obj, parent_ref);
	if (err) {
		cJSON_Delete(modem_obj);
		return err;
	}

	data->queued = false;

	return 0;

exit:
	cJSON_Delete(modem_obj);
	cJSON_Delete(modem_val_obj);
	return err;
}

int json_common_sensor_data_add(cJSON *parent,
				struct cloud_data_sensors *data,
				enum json_common_op_code op,
				const char *object_label,
				cJSON **parent_ref)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	err = date_time_uptime_to_unix_time_ms(&data->env_ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *sensor_obj = cJSON_CreateObject();
	cJSON *sensor_val_obj = cJSON_CreateObject();

	if (sensor_obj == NULL || sensor_val_obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	err = json_add_number(sensor_val_obj, DATA_TEMPERATURE, data->temperature);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(sensor_val_obj, DATA_HUMIDITY, data->humidity);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(sensor_val_obj, DATA_PRESSURE, data->pressure);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	/* If air quality is negative, the value is not provided. */
	if (data->bsec_air_quality >= 0) {
		err = json_add_number(sensor_val_obj, DATA_BSEC_IAQ, data->bsec_air_quality);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
	}

	json_add_obj(sensor_obj, DATA_VALUE, sensor_val_obj);

	err = json_add_number(sensor_obj, DATA_TIMESTAMP, data->env_ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(sensor_obj);
		return err;
	}

	err = op_code_handle(parent, op, object_label, sensor_obj, parent_ref);
	if (err) {
		cJSON_Delete(sensor_obj);
		return err;
	}

	data->queued = false;

	return 0;

exit:
	cJSON_Delete(sensor_obj);
	cJSON_Delete(sensor_val_obj);
	return err;
}

int json_common_gnss_data_add(cJSON *parent,
			      struct cloud_data_gnss *data,
			      enum json_common_op_code op,
			      const char *object_label,
			      cJSON **parent_ref)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	err = date_time_uptime_to_unix_time_ms(&data->gnss_ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *gnss_obj = cJSON_CreateObject();
	cJSON *gnss_val_obj = cJSON_CreateObject();

	if (gnss_obj == NULL || gnss_val_obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	err = json_add_number(gnss_val_obj, DATA_GNSS_LONGITUDE, data->pvt.longi);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(gnss_val_obj, DATA_GNSS_LATITUDE, data->pvt.lat);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(gnss_val_obj, DATA_GNSS_ACCURACY, data->pvt.acc);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(gnss_val_obj, DATA_GNSS_ALTITUDE, data->pvt.alt);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(gnss_val_obj, DATA_GNSS_SPEED, data->pvt.spd);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(gnss_val_obj, DATA_GNSS_HEADING, data->pvt.hdg);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	json_add_obj(gnss_obj, DATA_VALUE, gnss_val_obj);

	err = json_add_number(gnss_obj, DATA_TIMESTAMP, data->gnss_ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(gnss_obj);
		return err;
	}

	err = op_code_handle(parent, op, object_label, gnss_obj, parent_ref);
	if (err) {
		cJSON_Delete(gnss_obj);
		return err;
	}

	data->queued = false;

	return 0;

exit:
	cJSON_Delete(gnss_obj);
	cJSON_Delete(gnss_val_obj);
	return err;
}

int json_common_ui_data_add(cJSON *parent,
			    struct cloud_data_ui *data,
			    enum json_common_op_code op,
			    const char *object_label,
			    cJSON **parent_ref)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	err = date_time_uptime_to_unix_time_ms(&data->btn_ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *button_obj = cJSON_CreateObject();

	if (button_obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	err = json_add_number(button_obj, DATA_VALUE, data->btn);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(button_obj, DATA_TIMESTAMP, data->btn_ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = op_code_handle(parent, op, object_label, button_obj, parent_ref);
	if (err) {
		goto exit;
	}

	data->queued = false;

	return 0;

exit:
	cJSON_Delete(button_obj);
	return err;
}

int json_common_impact_data_add(cJSON *parent,
				struct cloud_data_impact *data,
				enum json_common_op_code op,
				const char *object_label,
				cJSON **parent_ref)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	err = date_time_uptime_to_unix_time_ms(&data->ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *impact_obj = cJSON_CreateObject();

	if (impact_obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	err = json_add_number(impact_obj, DATA_VALUE, data->magnitude);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(impact_obj, DATA_TIMESTAMP, data->ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = op_code_handle(parent, op, object_label, impact_obj, parent_ref);
	if (err) {
		goto exit;
	}

	data->queued = false;

	return 0;

exit:
	cJSON_Delete(impact_obj);
	return err;
}

int json_common_neighbor_cells_data_add(cJSON *parent,
					struct cloud_data_neighbor_cells *data,
					enum json_common_op_code op)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	cJSON *root = cJSON_CreateObject();

	if (root == NULL) {
		err = -ENOMEM;
		return err;
	}


	err = date_time_uptime_to_unix_time_ms(&data->ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		cJSON_Delete(root);
		return err;
	}

	err = json_add_number(root, DATA_NEIGHBOR_CELLS_MCC, data->cell_data.current_cell.mcc);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(root);
		return err;
	}

	err = json_add_number(root, DATA_NEIGHBOR_CELLS_MNC, data->cell_data.current_cell.mnc);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(root);
		return err;
	}

	err = json_add_number(root, DATA_NEIGHBOR_CELLS_CID, data->cell_data.current_cell.id);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(root);
		return err;
	}

	err = json_add_number(root, DATA_NEIGHBOR_CELLS_TAC, data->cell_data.current_cell.tac);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(root);
		return err;
	}

	err = json_add_number(root, DATA_NEIGHBOR_CELLS_EARFCN,
			      data->cell_data.current_cell.earfcn);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(root);
		return err;
	}

	err = json_add_number(root, DATA_NEIGHBOR_CELLS_TIMING,
			      data->cell_data.current_cell.timing_advance);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(root);
		return err;
	}

	err = json_add_number(root, DATA_NEIGHBOR_CELLS_RSRP,
			      data->cell_data.current_cell.rsrp);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(root);
		return err;
	}

	err = json_add_number(root, DATA_NEIGHBOR_CELLS_RSRQ,
			      data->cell_data.current_cell.rsrq);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(root);
		return err;
	}

	err = json_add_number(root, DATA_TIMESTAMP, data->ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(root);
		return err;
	}

	if (data->cell_data.ncells_count > 0) {
		cJSON *neighbor_cells = cJSON_CreateArray();

		if (neighbor_cells == NULL) {
			err = -ENOMEM;
			cJSON_Delete(root);
			return err;
		}

		for (int i = 0; i < data->cell_data.ncells_count; i++) {

			cJSON *cell = cJSON_CreateObject();

			if (cell == NULL) {
				err = -ENOMEM;
				cJSON_Delete(root);
				cJSON_Delete(neighbor_cells);
				return err;
			}

			err = json_add_number(cell, DATA_NEIGHBOR_CELLS_EARFCN,
					      data->neighbor_cells[i].earfcn);
			if (err) {
				LOG_ERR("Encoding error: %d returned at %s:%d", err,
					__FILE__, __LINE__);
				cJSON_Delete(root);
				cJSON_Delete(neighbor_cells);
				cJSON_Delete(cell);
				return err;
			}

			err = json_add_number(cell, DATA_NEIGHBOR_CELLS_PCI,
					      data->neighbor_cells[i].phys_cell_id);
			if (err) {
				LOG_ERR("Encoding error: %d returned at %s:%d", err,
					__FILE__, __LINE__);
				cJSON_Delete(root);
				cJSON_Delete(neighbor_cells);
				cJSON_Delete(cell);
				return err;
			}

			err = json_add_number(cell, DATA_NEIGHBOR_CELLS_RSRP,
					      data->neighbor_cells[i].rsrp);
			if (err) {
				LOG_ERR("Encoding error: %d returned at %s:%d", err,
					__FILE__, __LINE__);
				cJSON_Delete(root);
				cJSON_Delete(neighbor_cells);
				cJSON_Delete(cell);
				return err;
			}

			err = json_add_number(cell, DATA_NEIGHBOR_CELLS_RSRQ,
					      data->neighbor_cells[i].rsrq);
			if (err) {
				LOG_ERR("Encoding error: %d returned at %s:%d", err,
					__FILE__, __LINE__);
				cJSON_Delete(root);
				cJSON_Delete(neighbor_cells);
				cJSON_Delete(cell);
				return err;
			}

			err = op_code_handle(neighbor_cells, JSON_COMMON_ADD_DATA_TO_ARRAY, NULL,
					     cell, NULL);
			if (err) {
				cJSON_Delete(root);
				cJSON_Delete(neighbor_cells);
				cJSON_Delete(cell);
				return err;
			}
		}

		err = op_code_handle(root, JSON_COMMON_ADD_DATA_TO_OBJECT,
				     DATA_NEIGHBOR_CELLS_NEIGHBOR_MEAS, neighbor_cells, NULL);
		if (err) {
			cJSON_Delete(root);
			cJSON_Delete(neighbor_cells);
			return err;
		}
	}
	json_add_obj(parent, DATA_NEIGHBOR_CELLS_ROOT, root);

	data->queued = false;
	return err;
}

#if defined(CONFIG_LOCATION_METHOD_WIFI)
int json_common_wifi_ap_data_add(cJSON *parent,
				 struct cloud_data_wifi_access_points *data,
				 enum json_common_op_code op)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	cJSON *root = cJSON_CreateObject();

	if (root == NULL) {
		err = -ENOMEM;
		return err;
	}


	err = date_time_uptime_to_unix_time_ms(&data->ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		cJSON_Delete(root);
		return err;
	}

	err = json_add_number(root, DATA_TIMESTAMP, data->ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(root);
		return err;
	}

	if (data->cnt < NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN) {
		cJSON_Delete(root);
		return -ENODATA;
	}

	cJSON *ap_array = cJSON_CreateArray();

	if (ap_array == NULL) {
		cJSON_Delete(root);
		return err;
	}

	for (size_t i = 0; i < data->cnt; ++i) {
		char str_buf[AP_STRING_SIZE];
		struct wifi_scan_result const *const ap = (data->ap_info + i);

		/* MAC address is the only required parameter for the API call */
		int ret = snprintk(str_buf, sizeof(str_buf),
				   "%02x%02x%02x%02x%02x%02x",
				   ap->mac[0], ap->mac[1], ap->mac[2],
				   ap->mac[3], ap->mac[4], ap->mac[5]);

		if (ret < 0) {
			err = -EFAULT;
			goto cleanup;
		}

		cJSON *ap_obj = cJSON_CreateString(str_buf);

		if (!cJSON_AddItemToArray(ap_array, ap_obj)) {
			cJSON_Delete(ap_obj);
			err = -ENOMEM;
			goto cleanup;
		}
	}

	err = op_code_handle(root, JSON_COMMON_ADD_DATA_TO_OBJECT,
			     DATA_WIFI_AP_MEAS, ap_array, NULL);
	if (err) {
		goto cleanup;
	}

	json_add_obj(parent, DATA_WIFI_ROOT, root);
	data->queued = false;
	return err;

cleanup:
	cJSON_Delete(ap_array);
	cJSON_Delete(root);
	LOG_ERR("Failed to format WiFi location request, out of memory");
	return err;
}
#endif /* CONFIG_LOCATION_METHOD_WIFI */

int json_common_agps_request_data_add(cJSON *parent,
				      struct cloud_data_agps_request *data,
				      enum json_common_op_code op)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	err = json_add_number(parent, DATA_AGPS_REQUEST_MCC, data->mcc);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_AGPS_REQUEST_MNC, data->mnc);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_AGPS_REQUEST_TAC, data->area);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_AGPS_REQUEST_CID, data->cell);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	cJSON *agps_types = cJSON_CreateArray();

	if (agps_types == NULL) {
		return -ENOMEM;
	}

	if (data->request.data_flags & NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST) {
		err = json_add_number_to_array(agps_types,
					       DATA_AGPS_REQUEST_TYPE_UTC_PARAMETERS);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
	}

	/* GPS data need is always expected to be present and first in list. */
	__ASSERT(data->request.system_count > 0,
		 "GNSS system data need not found");
	__ASSERT(data->request.system[0].system_id == NRF_MODEM_GNSS_SYSTEM_GPS,
		 "GPS data need not found");

	if (data->request.system[0].sv_mask_ephe) {
		err = json_add_number_to_array(agps_types,
					       DATA_AGPS_REQUEST_TYPE_EPHEMERIDES);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
	}

	if (data->request.system[0].sv_mask_alm) {
		err = json_add_number_to_array(agps_types,
					       DATA_AGPS_REQUEST_TYPE_ALMANAC);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
	}

	if (data->request.data_flags & NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST) {
		err = json_add_number_to_array(agps_types,
					       DATA_AGPS_REQUEST_TYPE_KLOBUCHAR_CORRECTION);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
	}

	if (data->request.data_flags & NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST) {
		err = json_add_number_to_array(agps_types,
					       DATA_AGPS_REQUEST_TYPE_NEQUICK_CORRECTION);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
	}

	if (data->request.data_flags & NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		err = json_add_number_to_array(agps_types,
					       DATA_AGPS_REQUEST_TYPE_GPS_TOWS);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}

		err = json_add_number_to_array(agps_types,
					       DATA_AGPS_REQUEST_TYPE_GPS_SYSTEM_CLOCK_AND_TOWS);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
	}

	if (data->request.data_flags & NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST) {
		err = json_add_number_to_array(agps_types,
					       DATA_AGPS_REQUEST_TYPE_LOCATION);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
	}

	if (data->request.data_flags & NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST) {
		err = json_add_number_to_array(agps_types,
					       DATA_AGPS_REQUEST_TYPE_INTEGRITY);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
	}

	if (cJSON_GetArraySize(agps_types) == 0) {
		LOG_ERR("No AGPS request types to encode");
		err = -ENODATA;
		goto exit;
	}

	err = op_code_handle(parent, JSON_COMMON_ADD_DATA_TO_OBJECT,
			     DATA_AGPS_REQUEST_TYPES, agps_types, NULL);
	if (err) {
		goto exit;
	}

	data->queued = false;
	return 0;

exit:
	cJSON_Delete(agps_types);
	return err;
}

int json_common_pgps_request_data_add(cJSON *parent, struct cloud_data_pgps_request *data)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	err = json_add_number(parent, DATA_PGPS_REQUEST_COUNT, data->count);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_PGPS_REQUEST_INTERVAL, data->interval);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_PGPS_REQUEST_DAY, data->day);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_PGPS_REQUEST_TIME, data->time);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	data->queued = false;
	return 0;
}

int json_common_battery_data_add(cJSON *parent,
				 struct cloud_data_battery *data,
				 enum json_common_op_code op,
				 const char *object_label,
				 cJSON **parent_ref)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	err = date_time_uptime_to_unix_time_ms(&data->bat_ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *battery_obj = cJSON_CreateObject();

	if (battery_obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	err = json_add_number(battery_obj, DATA_VALUE, data->bat);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(battery_obj, DATA_TIMESTAMP, data->bat_ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = op_code_handle(parent, op, object_label, battery_obj, parent_ref);
	if (err) {
		goto exit;
	}

	data->queued = false;

	return 0;

exit:
	cJSON_Delete(battery_obj);
	return err;
}

int json_common_config_add(cJSON *parent, struct cloud_data_cfg *data, const char *object_label)
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

	err = json_add_number(config_obj, CONFIG_ACC_ACT_THRESHOLD,
			      data->accelerometer_activity_threshold);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(config_obj, CONFIG_ACC_INACT_THRESHOLD,
			      data->accelerometer_inactivity_threshold);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(config_obj, CONFIG_ACC_INACT_TIMEOUT,
			      data->accelerometer_inactivity_timeout);
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

void json_common_config_get(cJSON *parent, struct cloud_data_cfg *data)
{
	cJSON *location_timeout = cJSON_GetObjectItem(parent, CONFIG_LOCATION_TIMEOUT);
	cJSON *active = cJSON_GetObjectItem(parent, CONFIG_DEVICE_MODE);
	cJSON *active_wait = cJSON_GetObjectItem(parent, CONFIG_ACTIVE_TIMEOUT);
	cJSON *move_res = cJSON_GetObjectItem(parent, CONFIG_MOVE_RES);
	cJSON *move_timeout = cJSON_GetObjectItem(parent, CONFIG_MOVE_TIMEOUT);
	cJSON *acc_act_thres = cJSON_GetObjectItem(parent, CONFIG_ACC_ACT_THRESHOLD);
	cJSON *acc_inact_thres = cJSON_GetObjectItem(parent, CONFIG_ACC_INACT_THRESHOLD);
	cJSON *acc_inact_time = cJSON_GetObjectItem(parent, CONFIG_ACC_INACT_TIMEOUT);
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

	if (acc_inact_time != NULL) {
		data->accelerometer_inactivity_timeout = acc_inact_time->valuedouble;
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

int json_common_batch_data_add(cJSON *parent, enum json_common_buffer_type type, void *buf,
			       size_t buf_count, const char *object_label)
{
	int err = 0;
	cJSON *array_obj = cJSON_CreateArray();

	if (parent == NULL || array_obj == NULL) {
		cJSON_Delete(array_obj);
		return -ENOMEM;
	}

	if (object_label == NULL) {
		LOG_WRN("Missing object label");
		cJSON_Delete(array_obj);
		return -EINVAL;
	}

	for (int i = 0; i < buf_count; i++) {
		switch (type) {
		case JSON_COMMON_UI: {
			struct cloud_data_ui *data =
					(struct cloud_data_ui *)buf;
			err = json_common_ui_data_add(array_obj,
						      &data[i],
						      JSON_COMMON_ADD_DATA_TO_ARRAY,
						      NULL,
						      NULL);
		}
			break;
		case JSON_COMMON_IMPACT: {
			struct cloud_data_impact *data =
					(struct cloud_data_impact *)buf;
			err = json_common_impact_data_add(array_obj,
						      &data[i],
						      JSON_COMMON_ADD_DATA_TO_ARRAY,
						      NULL,
						      NULL);
		}
			break;
		case JSON_COMMON_MODEM_STATIC: {
			struct cloud_data_modem_static *data =
					(struct cloud_data_modem_static *)buf;
			err = json_common_modem_static_data_add(array_obj,
								&data[i],
								JSON_COMMON_ADD_DATA_TO_ARRAY,
								NULL,
								NULL);
		}
			break;
		case JSON_COMMON_MODEM_DYNAMIC: {
			struct cloud_data_modem_dynamic *data =
					(struct cloud_data_modem_dynamic *)buf;
			err = json_common_modem_dynamic_data_add(array_obj,
								 &data[i],
								 JSON_COMMON_ADD_DATA_TO_ARRAY,
								 NULL,
								 NULL);
		}
			break;
		case JSON_COMMON_GNSS: {
			struct cloud_data_gnss *data =
					(struct cloud_data_gnss *)buf;
			err = json_common_gnss_data_add(array_obj,
						       &data[i],
						       JSON_COMMON_ADD_DATA_TO_ARRAY,
						       NULL,
						       NULL);
		}
			break;
		case JSON_COMMON_SENSOR: {
			struct cloud_data_sensors *data =
					(struct cloud_data_sensors *)buf;
			err = json_common_sensor_data_add(array_obj,
							  &data[i],
							  JSON_COMMON_ADD_DATA_TO_ARRAY,
							  NULL,
							  NULL);
		}
			break;
		case JSON_COMMON_BATTERY: {
			struct cloud_data_battery *data =
					(struct cloud_data_battery *)buf;
			err = json_common_battery_data_add(array_obj,
							   &data[i],
							   JSON_COMMON_ADD_DATA_TO_ARRAY,
							   NULL,
							   NULL);
		}
			break;
		default:
			LOG_WRN("Unknown buffer type: %d", type);
			break;
		}

		if ((err != 0) && (err != -ENODATA)) {
			LOG_ERR("Failed adding data to array object");
			cJSON_Delete(array_obj);
			return err;
		}
	}

	if (cJSON_GetArraySize(array_obj) == 0) {
		cJSON_Delete(array_obj);
		/* At this point err can be either 0 or -ENODATA depending on the return
		 * value of the function that adds the last item present in the respective buffer.
		 * Because of this we explicitly set err to -ENODATA if the array is empty.
		 */
		return -ENODATA;
	}

	json_add_obj(parent, object_label, array_obj);
	return 0;
}
