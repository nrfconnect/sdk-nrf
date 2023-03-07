/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

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

#include "cloud_codec.h"
#include "lwm2m_codec_defines.h"
#include "lwm2m_codec_helpers.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cloud_codec, CONFIG_CLOUD_CODEC_LOG_LEVEL);

/* Module event handler.  */
static cloud_codec_evt_handler_t module_evt_handler;

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

	err = lwm2m_codec_helpers_get_configuration_object(&cfg);
	if (err) {
		LOG_ERR("lwm2m_codec_helpers_get_configuration_object, error: %d", err);
		return err;
	}

	evt.config_update = cfg;
	module_evt_handler(&evt);
	return 0;
}

int cloud_codec_init(struct cloud_data_cfg *cfg, cloud_codec_evt_handler_t event_handler)
{
	int err;

	err = lwm2m_codec_helpers_create_objects_and_resources();
	if (err) {
		LOG_ERR("lwm2m_codec_helpers_create_objects_and_resources, error: %d", err);
		return err;
	}

	err = lwm2m_codec_helpers_setup_resources();
	if (err) {
		LOG_ERR("lwm2m_codec_helpers_setup_resources, error: %d", err);
		return err;
	}

	err = lwm2m_codec_helpers_setup_configuration_object(cfg, &config_update_cb);
	if (err) {
		LOG_ERR("lwm2m_codec_helpers_setup_configuration_object, error: %d", err);
		return err;
	}

	module_evt_handler = event_handler;
	return 0;
}

int cloud_codec_encode_cloud_location(struct cloud_codec_data *output,
				 struct cloud_data_cloud_location *cloud_location)
{
	ARG_UNUSED(output);

	int err;

	err = lwm2m_codec_helpers_set_neighbor_cell_data(&cloud_location->neighbor_cells);
	if (err) {
		return err;
	}

	cloud_location->queued = false;
	return 0;
}

int cloud_codec_encode_agps_request(struct cloud_codec_data *output,
				    struct cloud_data_agps_request *agps_request)
{
	ARG_UNUSED(output);

	int err;

	err = lwm2m_codec_helpers_set_agps_data(agps_request);
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

	err = lwm2m_codec_helpers_set_pgps_data(pgps_request);
	if (err) {
		return err;
	}

	pgps_request->queued = false;
	return 0;
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
	ARG_UNUSED(impact_buf);

	int err;
	bool objects_written = false;

	err = lwm2m_codec_helpers_set_gnss_data(gnss_buf);
	if (err == 0) {

		static const struct lwm2m_obj_path path_list[] = {
			LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID)
		};

		err = lwm2m_codec_helpers_object_path_list_add(output,
							       path_list,
							       ARRAY_SIZE(path_list));
		if (err) {
			LOG_ERR("Failed populating object path list, error: %d", err);
			return err;
		}

		gnss_buf->queued = false;
		objects_written = true;

	} else if (err != -ENODATA) {
		LOG_ERR("lwm2m_codec_helpers_set_gnss_data, error: %d", err);
		return err;
	}

	err = lwm2m_codec_helpers_set_modem_dynamic_data(modem_dyn_buf);
	if (err == 0) {

		static const struct lwm2m_obj_path path_list[] = {
			LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, CURRENT_TIME_RID),
			LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID)
		};

		err = lwm2m_codec_helpers_object_path_list_add(output,
							       path_list,
							       ARRAY_SIZE(path_list));
		if (err) {
			LOG_ERR("Failed populating object path list, error: %d", err);
			return err;
		}

		modem_dyn_buf->queued = false;
		objects_written = true;

	} else if (err != -ENODATA) {
		LOG_ERR("lwm2m_codec_helpers_set_modem_dynamic_data, error: %d", err);
		return err;
	}

	err = lwm2m_codec_helpers_set_modem_static_data(modem_stat_buf);
	if (err == 0) {

		static const struct lwm2m_obj_path path_list[] = {
			LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MANUFACTURER_RID),
			LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MODEL_NUMBER_RID),
			LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID),
			LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, FIRMWARE_VERSION_RID),
			LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_SERIAL_NUMBER_ID)
		};

		err = lwm2m_codec_helpers_object_path_list_add(output,
							       path_list,
							       ARRAY_SIZE(path_list));
		if (err) {
			LOG_ERR("Failed populating object path list, error: %d", err);
			return err;
		}

		modem_stat_buf->queued = false;
		objects_written = true;

	} else if (err != -ENODATA) {
		LOG_ERR("lwm2m_codec_helpers_set_modem_static_data, error: %d", err);
		return err;
	}

	err = lwm2m_codec_helpers_set_battery_data(bat_buf);
	if (err == 0) {

		static const struct lwm2m_obj_path path_list[] = {
			LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID)
		};

		err = lwm2m_codec_helpers_object_path_list_add(output,
							       path_list,
							       ARRAY_SIZE(path_list));
		if (err) {
			LOG_ERR("Failed populating object path list, error: %d", err);
			return err;
		}

		bat_buf->queued = false;
		objects_written = true;

	} else if (err != -ENODATA) {
		LOG_ERR("lwm2m_codec_helpers_set_battery_data, error: %d", err);
		return err;
	}

#if defined(CONFIG_CLOUD_CODEC_LWM2M_THINGY91_SENSORS)
	err = lwm2m_codec_helpers_set_sensor_data(sensor_buf);
	if (err == 0) {

		static const struct lwm2m_obj_path path_list[] = {
			LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
			LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
			LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
			LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
			LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
			LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID)
		};

		err = lwm2m_codec_helpers_object_path_list_add(output,
							       path_list,
							       ARRAY_SIZE(path_list));
		if (err) {
			LOG_ERR("Failed populating object path list, error: %d", err);
			return err;
		}

		sensor_buf->queued = false;
		objects_written = true;

	} else if (err != -ENODATA) {
		LOG_ERR("lwm2m_codec_helpers_set_sensor_data, error: %d", err);
		return err;
	}
#endif /* CONFIG_CLOUD_CODEC_LWM2M_THINGY91_SENSORS */

	if (!objects_written) {
		return -ENODATA;
	}

	return 0;
}

int cloud_codec_encode_ui_data(struct cloud_codec_data *output,
			       struct cloud_data_ui *ui_buf)
{
	int err;

	err = lwm2m_codec_helpers_set_user_interface_data(ui_buf);
	if (err) {
		return err;
	}

	static const struct lwm2m_obj_path path_list[] = {
		LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID),
	};

	err = lwm2m_codec_helpers_object_path_list_add(output, path_list, ARRAY_SIZE(path_list));
	if (err) {
		LOG_ERR("Failed populating object path list, error: %d", err);
		return err;
	}

	ui_buf->queued = false;
	return 0;
}

/* Unsupported APIs. */
int cloud_codec_decode_config(const char *input, size_t input_len,
			      struct cloud_data_cfg *cfg)
{
	ARG_UNUSED(input);
	ARG_UNUSED(input_len);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}

int cloud_codec_encode_config(struct cloud_codec_data *output,
			      struct cloud_data_cfg *data)
{
	ARG_UNUSED(output);
	ARG_UNUSED(data);

	return -ENOTSUP;
}

int cloud_codec_encode_impact_data(struct cloud_codec_data *output,
				   struct cloud_data_impact *impact_buf)
{
	ARG_UNUSED(output);
	ARG_UNUSED(impact_buf);

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
	ARG_UNUSED(output);
	ARG_UNUSED(gnss_buf);
	ARG_UNUSED(sensor_buf);
	ARG_UNUSED(modem_stat_buf);
	ARG_UNUSED(modem_dyn_buf);
	ARG_UNUSED(ui_buf);
	ARG_UNUSED(impact_buf);
	ARG_UNUSED(bat_buf);
	ARG_UNUSED(gnss_buf_count);
	ARG_UNUSED(sensor_buf_count);
	ARG_UNUSED(modem_stat_buf_count);
	ARG_UNUSED(modem_dyn_buf_count);
	ARG_UNUSED(ui_buf_count);
	ARG_UNUSED(impact_buf_count);
	ARG_UNUSED(bat_buf_count);

	return -ENOTSUP;
}
