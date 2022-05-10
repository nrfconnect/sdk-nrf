/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define LOG_MODULE_NAME net_lwm2m_obj_loc_assistance
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include <net/lwm2m_client_utils.h>
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
#include <net/nrf_cloud_agps.h>
#endif

#define LOCATION_ASSIST_VERSION_MAJOR 1
#define LOCATION_ASSIST_VERSION_MINOR 0

/* Assistance types */
#define ASSISTANCE_REQUEST_TYPE_IDLE			0
#define ASSISTANCE_REQUEST_TYPE_SINGLECELL_INFORM	1
#define ASSISTANCE_REQUEST_TYPE_MULTICELL_INFORM	2
#define ASSISTANCE_REQUEST_TYPE_SINGLECELL_REQUEST	3
#define ASSISTANCE_REQUEST_TYPE_MULTICELL_REQUEST	4
#define ASSISTANCE_REQUEST_TYPE_AGPS			5
#define ASSISTANCE_REQUEST_TYPE_PGPS			6

/* Location Assistance resource IDs */
#define LOCATION_ASSIST_ASSIST_TYPE			0
#define LOCATION_ASSIST_AGPS_MASK			1
#define LOCATION_ASSIST_PGPS_PRED_COUNT			2
#define LOCATION_ASSIST_PGPS_PRED_INTERVAL		3
#define LOCATION_ASSIST_PGPS_START_GPS_DAY		4
#define LOCATION_ASSIST_PGPS_START_GPS_TIME_OF_DAY	5
#define LOCATION_ASSIST_ASSIST_DATA			6
#define LOCATION_ASSIST_RESULT_CODE			7
#define LOCATION_ASSIST_LATITUDE			8
#define LOCATION_ASSIST_LONGITUDE			9
#define LOCATION_ASSIST_ALTITUDE			10
#define LOCATION_ASSIST_ACCURACY			11

#define LOCATION_ASSIST_MAX_ID				12

static int32_t assist_type;
static int32_t agps_mask;
static int32_t pgps_pred_count;
static int32_t pgps_pred_interval;
static int32_t pgps_start_gps_day;
static int32_t pgps_start_gps_time_of_day;
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
static char assist_buf[CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS_BUF_SIZE];
static char assist_data[CONFIG_LWM2M_COAP_BLOCK_SIZE];
#endif

static int32_t result;
static double latitude;
static double longitude;
static double altitude;
static double accuracy;

/*
 * Calculate resource instances as follows:
 * start with LOCATION_ASSIST_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT	(LOCATION_ASSIST_MAX_ID)

static struct lwm2m_engine_obj location_assistance;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(LOCATION_ASSIST_ASSIST_TYPE, R, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_AGPS_MASK, R_OPT, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_PGPS_PRED_COUNT, R_OPT, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_PGPS_PRED_INTERVAL, R_OPT, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_PGPS_START_GPS_DAY, R_OPT, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_PGPS_START_GPS_TIME_OF_DAY, R_OPT, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_ASSIST_DATA, W, OPAQUE),
	OBJ_FIELD_DATA(LOCATION_ASSIST_RESULT_CODE, W, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_LATITUDE, W, FLOAT),
	OBJ_FIELD_DATA(LOCATION_ASSIST_LONGITUDE, W, FLOAT),
	OBJ_FIELD_DATA(LOCATION_ASSIST_ALTITUDE, W, FLOAT),
	OBJ_FIELD_DATA(LOCATION_ASSIST_ACCURACY, W, FLOAT),
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[LOCATION_ASSIST_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_INSTANCE_COUNT];

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
static void *get_assist_buf(uint16_t obj_inst_id, uint16_t res_id,
			      uint16_t res_inst_id, size_t *data_len)
{
	LOG_INF("Get assist buf");
	*data_len = sizeof(assist_data);
	return assist_data;
}


static int location_assist_write_cb(uint16_t obj_inst_id, uint16_t res_id,
			    uint16_t res_inst_id, uint8_t *data, uint16_t data_len,
			    bool last_block, size_t total_size)
{
	LOG_INF("Writing assistance data");
	int err = 0;
	static uint32_t bytes_downloaded;

	memcpy(&assist_buf[bytes_downloaded], &data[0], data_len);
	bytes_downloaded += data_len;

	if (last_block) {
		err = nrf_cloud_agps_process(assist_buf, bytes_downloaded);
		if (err) {
			LOG_WRN("Unable to process A-GPS data, error: %d", err);
		} else {
			LOG_INF("A-GPS data processed");
		}
		bytes_downloaded = 0;
		return err;
	}

	return 0;
}

void location_assist_agps_request_set(uint32_t request_mask)
{
	LOG_INF("Requesting A-GPS data, mask 0x%08x", request_mask);
	/* Store mask to object resource */
	agps_mask = request_mask;
	assist_type = ASSISTANCE_REQUEST_TYPE_AGPS;
}
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_CELL)
void location_assist_cell_inform_set(void)
{
#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
	assist_type = ASSISTANCE_REQUEST_TYPE_MULTICELL_INFORM;
#else
	assist_type = ASSISTANCE_REQUEST_TYPE_SINGLECELL_INFORM;
#endif
}

void location_assist_cell_request_set(void)
{
#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
	assist_type = ASSISTANCE_REQUEST_TYPE_MULTICELL_REQUEST;
#else
	assist_type = ASSISTANCE_REQUEST_TYPE_SINGLECELL_REQUEST;
#endif
}
#endif

static struct lwm2m_engine_obj_inst *location_assist_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	init_res_instance(res_inst, ARRAY_SIZE(res_inst));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_ASSIST_TYPE, res, i, res_inst, j,
			  &assist_type, sizeof(assist_type));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_AGPS_MASK, res, i, res_inst, j,
			  &agps_mask, sizeof(agps_mask));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_PGPS_PRED_COUNT, res, i, res_inst, j,
			  &pgps_pred_count, sizeof(pgps_pred_count));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_PGPS_PRED_INTERVAL, res, i, res_inst, j,
			  &pgps_pred_interval, sizeof(pgps_pred_interval));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_PGPS_START_GPS_DAY, res, i, res_inst, j,
			  &pgps_start_gps_day, sizeof(pgps_start_gps_day));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_PGPS_START_GPS_TIME_OF_DAY, res, i, res_inst, j,
			  &pgps_start_gps_time_of_day, sizeof(pgps_start_gps_time_of_day));
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
	INIT_OBJ_RES_OPT(LOCATION_ASSIST_ASSIST_DATA, res, i, res_inst, j, 1, false, true, NULL,
			 get_assist_buf, NULL, location_assist_write_cb, NULL);
#endif
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_RESULT_CODE, res, i, res_inst, j,
			  &result, sizeof(result));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_LATITUDE, res, i, res_inst, j,
			  &latitude, sizeof(latitude));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_LONGITUDE, res, i, res_inst, j,
			  &longitude, sizeof(longitude));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_ALTITUDE, res, i, res_inst, j,
			  &altitude, sizeof(altitude));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_ACCURACY, res, i, res_inst, j,
			  &accuracy, sizeof(accuracy));

	inst.resources = res;
	inst.resource_count = i;
	LOG_INF("Created a location assistance object: %d", obj_inst_id);
	return &inst;
}

static int lwm2m_location_assist_init(const struct device *dev)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	location_assistance.obj_id = LOCATION_ASSIST_OBJECT_ID;
	location_assistance.version_major = LOCATION_ASSIST_VERSION_MAJOR;
	location_assistance.version_minor = LOCATION_ASSIST_VERSION_MINOR;
	location_assistance.is_core = false;
	location_assistance.fields = fields;
	location_assistance.field_count = ARRAY_SIZE(fields);
	location_assistance.max_instance_count = 1U;
	location_assistance.create_cb = location_assist_create;
	lwm2m_register_obj(&location_assistance);

	ret = lwm2m_create_obj_inst(LOCATION_ASSIST_OBJECT_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_ERR("Create location assist object error: %d", ret);
	}

	return 0;
}

SYS_INIT(lwm2m_location_assist_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
