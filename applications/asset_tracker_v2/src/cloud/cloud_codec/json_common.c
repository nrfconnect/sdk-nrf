/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <cJSON.h>
#include <date_time.h>

#include "cloud_codec.h"
#include "json_common.h"
#include "json_helpers.h"
#include "json_protocol_names.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(json_common, CONFIG_CLOUD_CODEC_LOG_LEVEL);

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
	char nw_mode[50] = {0};

	static const char lte_string[] = "LTE-M";
	static const char nbiot_string[] = "NB-IoT";
	static const char gps_string[] = " GPS";

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

	if (data->nw_lte_m) {
		strcpy(nw_mode, lte_string);
	} else if (data->nw_nb_iot) {
		strcpy(nw_mode, nbiot_string);
	}

	if (data->nw_gps) {
		strcat(nw_mode, gps_string);
	}

	err = json_add_number(modem_val_obj, MODEM_CURRENT_BAND, data->bnd);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_str(modem_val_obj, MODEM_NETWORK_MODE, nw_mode);
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
	bool values_added = false;

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

	if (data->rsrp_fresh) {
		err = json_add_number(modem_val_obj, MODEM_RSRP, data->rsrp);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		values_added = true;
	}

	if (data->area_code_fresh) {
		err = json_add_number(modem_val_obj, MODEM_AREA_CODE, data->area);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		values_added = true;
	}

	if (data->mccmnc_fresh) {
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
		values_added = true;
	}

	if (data->cell_id_fresh) {
		err = json_add_number(modem_val_obj, MODEM_CELL_ID, data->cell);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		values_added = true;
	}

	if (data->ip_address_fresh) {
		err = json_add_str(modem_val_obj, MODEM_IP_ADDRESS, data->ip);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		values_added = true;
	}

	if (!values_added) {
		err = -ENODATA;
		data->queued = false;
		LOG_WRN("No valid dynamic modem data values present, entry unqueued");
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

	err = json_add_number(sensor_val_obj, DATA_TEMPERATURE, data->temp);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(sensor_val_obj, DATA_HUMID, data->hum);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
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

int json_common_gps_data_add(cJSON *parent,
			     struct cloud_data_gps *data,
			     enum json_common_op_code op,
			     const char *object_label,
			     cJSON **parent_ref)
{
	int err;

	if (!data->queued) {
		return -ENODATA;
	}

	err = date_time_uptime_to_unix_time_ms(&data->gps_ts);
	if (err) {
		LOG_ERR("date_time_uptime_to_unix_time_ms, error: %d", err);
		return err;
	}

	cJSON *gps_obj = cJSON_CreateObject();
	cJSON *gps_val_obj = cJSON_CreateObject();

	if (gps_obj == NULL || gps_val_obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	switch (data->format) {
	case CLOUD_CODEC_GPS_FORMAT_PVT:
		err = json_add_number(gps_val_obj, DATA_GPS_LONGITUDE, data->pvt.longi);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}

		err = json_add_number(gps_val_obj, DATA_GPS_LATITUDE, data->pvt.lat);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}

		err = json_add_number(gps_val_obj, DATA_MOVEMENT, data->pvt.acc);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}

		err = json_add_number(gps_val_obj, DATA_GPS_ALTITUDE, data->pvt.alt);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}

		err = json_add_number(gps_val_obj, DATA_GPS_SPEED, data->pvt.spd);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}

		err = json_add_number(gps_val_obj, DATA_GPS_HEADING, data->pvt.hdg);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}

		json_add_obj(gps_obj, DATA_VALUE, gps_val_obj);
		break;
	case CLOUD_CODEC_GPS_FORMAT_NMEA:
		cJSON_Delete(gps_val_obj);
		err = json_add_str(gps_obj, DATA_VALUE, data->nmea);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		break;
	case CLOUD_CODEC_GPS_FORMAT_INVALID:
		/* Fall through */
	default:
		err = -EINVAL;
		LOG_WRN("GPS data format not set");
		goto exit;
	}

	err = json_add_number(gps_obj, DATA_TIMESTAMP, data->gps_ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(gps_obj);
		return err;
	}

	err = op_code_handle(parent, op, object_label, gps_obj, parent_ref);
	if (err) {
		cJSON_Delete(gps_obj);
		return err;
	}

	data->queued = false;

	return 0;

exit:
	cJSON_Delete(gps_obj);
	cJSON_Delete(gps_val_obj);
	return err;
}

int json_common_accel_data_add(cJSON *parent,
			       struct cloud_data_accelerometer *data,
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

	cJSON *accel_obj = cJSON_CreateObject();
	cJSON *accel_val_obj = cJSON_CreateObject();

	if (accel_obj == NULL || accel_val_obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	err = json_add_number(accel_val_obj, DATA_MOVEMENT_X, data->values[0]);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(accel_val_obj, DATA_MOVEMENT_Y, data->values[1]);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	err = json_add_number(accel_val_obj, DATA_MOVEMENT_Z, data->values[2]);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		goto exit;
	}

	json_add_obj(accel_obj, DATA_VALUE, accel_val_obj);

	err = json_add_number(accel_obj, DATA_TIMESTAMP, data->ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		cJSON_Delete(accel_obj);
		return err;
	}

	err = op_code_handle(parent, op, object_label, accel_obj, parent_ref);
	if (err) {
		cJSON_Delete(accel_obj);
		return err;
	}

	data->queued = false;

	return 0;

exit:
	cJSON_Delete(accel_obj);
	cJSON_Delete(accel_val_obj);
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

int json_common_neighbor_cells_data_add(cJSON *parent,
					struct cloud_data_neighbor_cells *data,
					enum json_common_op_code op)
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

	err = json_add_number(parent, DATA_NEIGHBOR_CELLS_MCC, data->cell_data.current_cell.mcc);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_NEIGHBOR_CELLS_MNC, data->cell_data.current_cell.mnc);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_NEIGHBOR_CELLS_CID, data->cell_data.current_cell.id);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_NEIGHBOR_CELLS_TAC, data->cell_data.current_cell.tac);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_NEIGHBOR_CELLS_EARFCN,
			      data->cell_data.current_cell.earfcn);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_NEIGHBOR_CELLS_TIMING,
			      data->cell_data.current_cell.timing_advance);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_NEIGHBOR_CELLS_RSRP,
			      data->cell_data.current_cell.rsrp);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_NEIGHBOR_CELLS_RSRQ,
			      data->cell_data.current_cell.rsrq);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	err = json_add_number(parent, DATA_TIMESTAMP, data->ts);
	if (err) {
		LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
		return err;
	}

	if (data->cell_data.ncells_count > 0) {
		cJSON *neighbor_cells = cJSON_CreateArray();

		if (neighbor_cells == NULL) {
			return err;
		}

		for (int i = 0; i < data->cell_data.ncells_count; i++) {

			cJSON *cell = cJSON_CreateObject();

			if (cell == NULL) {
				err = -ENOMEM;
				cJSON_Delete(neighbor_cells);
				return err;
			}

			err = json_add_number(cell, DATA_NEIGHBOR_CELLS_EARFCN,
					      data->neighbor_cells[i].earfcn);
			if (err) {
				LOG_ERR("Encoding error: %d returned at %s:%d", err,
					__FILE__, __LINE__);
				cJSON_Delete(neighbor_cells);
				cJSON_Delete(cell);
				return err;
			}

			err = json_add_number(cell, DATA_NEIGHBOR_CELLS_PCI,
					      data->neighbor_cells[i].phys_cell_id);
			if (err) {
				LOG_ERR("Encoding error: %d returned at %s:%d", err,
					__FILE__, __LINE__);
				cJSON_Delete(neighbor_cells);
				cJSON_Delete(cell);
				return err;
			}

			err = json_add_number(cell, DATA_NEIGHBOR_CELLS_RSRP,
					      data->neighbor_cells[i].rsrp);
			if (err) {
				LOG_ERR("Encoding error: %d returned at %s:%d", err,
					__FILE__, __LINE__);
				cJSON_Delete(neighbor_cells);
				cJSON_Delete(cell);
				return err;
			}

			err = json_add_number(cell, DATA_NEIGHBOR_CELLS_RSRQ,
					      data->neighbor_cells[i].rsrq);
			if (err) {
				LOG_ERR("Encoding error: %d returned at %s:%d", err,
					__FILE__, __LINE__);
				cJSON_Delete(neighbor_cells);
				cJSON_Delete(cell);
				return err;
			}

			err = op_code_handle(neighbor_cells, JSON_COMMON_ADD_DATA_TO_ARRAY, NULL,
					     cell, NULL);
			if (err) {
				cJSON_Delete(neighbor_cells);
				cJSON_Delete(cell);
				return err;
			}
		}

		err = op_code_handle(parent, JSON_COMMON_ADD_DATA_TO_OBJECT,
				     DATA_NEIGHBOR_CELLS_NEIGHBOR_MEAS, neighbor_cells, NULL);
		if (err) {
			cJSON_Delete(neighbor_cells);
			return err;
		}
	}

	data->queued = false;
	return err;
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
	bool values_added = false;

	if (object_label == NULL) {
		LOG_WRN("Missing object label");
		return -EINVAL;
	}

	cJSON *config_obj = cJSON_CreateObject();

	if (config_obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	if (data->active_mode_fresh) {
		err = json_add_bool(config_obj, CONFIG_DEVICE_MODE,
				    data->active_mode);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		values_added = true;
	}

	if (data->gps_timeout_fresh) {
		err = json_add_number(config_obj, CONFIG_GPS_TIMEOUT,
				      data->gps_timeout);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		values_added = true;
	}

	if (data->active_wait_timeout_fresh) {
		err = json_add_number(config_obj, CONFIG_ACTIVE_TIMEOUT,
				      data->active_wait_timeout);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		values_added = true;
	}

	if (data->movement_resolution_fresh) {
		err = json_add_number(config_obj, CONFIG_MOVE_RES,
				      data->movement_resolution);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		values_added = true;
	}

	if (data->movement_timeout_fresh) {
		err = json_add_number(config_obj, CONFIG_MOVE_TIMEOUT,
				      data->movement_timeout);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		values_added = true;
	}

	if (data->accelerometer_threshold_fresh) {
		err = json_add_number(config_obj, CONFIG_ACC_THRESHOLD,
				      data->accelerometer_threshold);
		if (err) {
			LOG_ERR("Encoding error: %d returned at %s:%d", err, __FILE__, __LINE__);
			goto exit;
		}
		values_added = true;
	}

	if (data->nod_list_fresh) {
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

		/* If there are no flag set in the no_data structure, an empty array is
		 * encoded.
		 */
		values_added = true;
		json_add_obj(config_obj, CONFIG_NO_DATA_LIST, nod_list);
	}

	if (!values_added) {
		err = -ENODATA;
		LOG_WRN("No valid configuration data values present");
		goto exit;
	}

	json_add_obj(parent, object_label, config_obj);

	return 0;

exit:
	cJSON_Delete(config_obj);
	return err;
}

void json_common_config_get(cJSON *parent, struct cloud_data_cfg *data)
{
	cJSON *gps_timeout = cJSON_GetObjectItem(parent, CONFIG_GPS_TIMEOUT);
	cJSON *active = cJSON_GetObjectItem(parent, CONFIG_DEVICE_MODE);
	cJSON *active_wait = cJSON_GetObjectItem(parent, CONFIG_ACTIVE_TIMEOUT);
	cJSON *move_res = cJSON_GetObjectItem(parent, CONFIG_MOVE_RES);
	cJSON *move_timeout = cJSON_GetObjectItem(parent, CONFIG_MOVE_TIMEOUT);
	cJSON *acc_thres = cJSON_GetObjectItem(parent, CONFIG_ACC_THRESHOLD);
	cJSON *nod_list = cJSON_GetObjectItem(parent, CONFIG_NO_DATA_LIST);

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

	if (nod_list != NULL && cJSON_IsArray(nod_list)) {
		cJSON *item;
		bool gnss_found = false;
		bool ncell_found = false;

		for (int i = 0; i < cJSON_GetArraySize(nod_list); i++) {
			item = cJSON_GetArrayItem(nod_list, i);

			if (strcmp(item->valuestring, CONFIG_NO_DATA_LIST_GNSS) == 0) {
				gnss_found = true;
			}

			if (strcmp(item->valuestring, CONFIG_NO_DATA_LIST_NEIGHBOR_CELL) == 0) {
				ncell_found = true;
			}
		}

		/* If a supported entry is present in the no data list we set the corresponding flag
		 * to true. Signifying that no data is to be sampled for that data type.
		 */
		data->no_data.gnss = gnss_found;
		data->no_data.neighbor_cell = ncell_found;
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
		case JSON_COMMON_GPS: {
			struct cloud_data_gps *data =
					(struct cloud_data_gps *)buf;
			err = json_common_gps_data_add(array_obj,
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
		case JSON_COMMON_ACCELEROMETER: {
			struct cloud_data_accelerometer *data =
					(struct cloud_data_accelerometer *)buf;
			err = json_common_accel_data_add(array_obj,
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
