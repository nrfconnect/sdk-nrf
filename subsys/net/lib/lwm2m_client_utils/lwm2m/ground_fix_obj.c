/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define LOG_MODULE_NAME net_lwm2m_obj_ground_fix
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include <net/lwm2m_client_utils_location.h>
#include "ground_fix_obj.h"
#include "lwm2m_engine.h"

#define GROUND_FIX_VERSION_MAJOR 1
#define GROUND_FIX_VERSION_MINOR 0

/* Location Assistance resource IDs */
#define GROUND_FIX_SEND_LOCATION_BACK		0
#define GROUND_FIX_RESULT_CODE			1
#define GROUND_FIX_LATITUDE			2
#define GROUND_FIX_LONGITUDE			3
#define GROUND_FIX_ACCURACY			4

#define GROUND_FIX_MAX_ID			5

#define LOCATION_LATITUDE_ID			0
#define LOCATION_LONGITUDE_ID			1
#define LOCATION_RADIUS_ID			3
#define LOCATION_TIMESTAMP_ID			5

static bool send_location_back;

static int32_t result;
static double latitude;
static double longitude;
static double accuracy;

static ground_fix_get_result_code_cb_t result_code_cb;

/*
 * Calculate resource instances as follows:
 * start with GROUND_FIX_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT	(GROUND_FIX_MAX_ID)

static struct lwm2m_engine_obj ground_fix;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(GROUND_FIX_SEND_LOCATION_BACK, R, BOOL),
	OBJ_FIELD_DATA(GROUND_FIX_RESULT_CODE, W, S32),
	OBJ_FIELD_DATA(GROUND_FIX_LATITUDE, W_OPT, FLOAT),
	OBJ_FIELD_DATA(GROUND_FIX_LONGITUDE, W_OPT, FLOAT),
	OBJ_FIELD_DATA(GROUND_FIX_ACCURACY, W_OPT, FLOAT),
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[GROUND_FIX_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_INSTANCE_COUNT];

void ground_fix_set_result_code_cb(ground_fix_get_result_code_cb_t cb)
{
	result_code_cb = cb;
}

static const char *gfix_result_str(int32_t result)
{
	switch (result) {
	case LOCATION_ASSIST_RESULT_CODE_OK:
		return "OK";
	case LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR:
		return "Permanent error";
	case LOCATION_ASSIST_RESULT_CODE_TEMP_ERR:
		return "Temporary error";
	case LOCATION_ASSIST_RESULT_CODE_NO_RESP_ERR:
		return "No response";
	default:
		return "Unknown";
	}
}

static int ground_fix_result_code_cb(uint16_t obj_inst_id, uint16_t res_id,
				     uint16_t res_inst_id, uint8_t *data,
				     uint16_t data_len, bool last_block,
				     size_t total_size, size_t offset)
{
	if (data_len != sizeof(int32_t)) {
		return -EINVAL;
	}

	result =  *(int32_t *)data;

	if (result != LOCATION_ASSIST_RESULT_CODE_OK) {
		LOG_ERR("Ground Fix result %s (%d)", gfix_result_str(result), result);
	} else {
		LOG_INF("Ground Fix result %s (%d)", gfix_result_str(result), result);
	}

	if (result_code_cb) {
		result_code_cb(result);
	}

	return 0;
}

static int forward_to_location_obj(uint16_t obj_inst_id,
					  uint16_t res_id, uint16_t res_inst_id,
					  uint8_t *data, uint16_t data_len,
					  bool last_block, size_t total_size,
					  size_t offset)
{
	struct lwm2m_obj_path path;

	switch (res_id) {
	case GROUND_FIX_LATITUDE:
		if (IS_ENABLED(CONFIG_POSIX_CLOCK)) {
			/* Update timestamp as well */
			lwm2m_set_time(
				&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LOCATION_TIMESTAMP_ID),
				time(NULL));
		}
		path = LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LOCATION_LATITUDE_ID);
		break;
	case GROUND_FIX_LONGITUDE:
		path = LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LOCATION_LONGITUDE_ID);
		break;
	case GROUND_FIX_ACCURACY:
		path = LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LOCATION_RADIUS_ID);
		break;
	default:
		return -EINVAL;
	}

	if (data_len != sizeof(double)) {
		return -EINVAL;
	}

	double val = *(double *)data;

	return lwm2m_set_f64(&path, val);
}

static struct lwm2m_engine_obj_inst *ground_fix_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	init_res_instance(res_inst, ARRAY_SIZE(res_inst));

	lwm2m_engine_set_data_cb_t forward_cb =
		IS_ENABLED(CONFIG_LWM2M_LOCATION_OBJ_SUPPORT) ? forward_to_location_obj : NULL;

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(GROUND_FIX_SEND_LOCATION_BACK, res, i, res_inst, j,
			  &send_location_back, sizeof(send_location_back));
	INIT_OBJ_RES(GROUND_FIX_RESULT_CODE, res, i, res_inst, j, 1, false, true, &result,
		     sizeof(result), NULL, NULL, NULL, ground_fix_result_code_cb, NULL);
	INIT_OBJ_RES(GROUND_FIX_LATITUDE, res, i, res_inst, j, 1, false, true, &latitude,
		     sizeof(latitude), NULL, NULL, NULL, forward_cb, NULL);
	INIT_OBJ_RES(GROUND_FIX_LONGITUDE, res, i, res_inst, j, 1, false, true, &longitude,
		     sizeof(longitude), NULL, NULL, NULL, forward_cb, NULL);
	INIT_OBJ_RES(GROUND_FIX_ACCURACY, res, i, res_inst, j, 1, false, true, &accuracy,
		     sizeof(accuracy), NULL, NULL, NULL, forward_cb, NULL);

	inst.resources = res;
	inst.resource_count = i;
	LOG_DBG("Created a ground fix object: %d", obj_inst_id);
	return &inst;
}

static int lwm2m_ground_fix_init(void)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	ground_fix.obj_id = GROUND_FIX_OBJECT_ID;
	ground_fix.version_major = GROUND_FIX_VERSION_MAJOR;
	ground_fix.version_minor = GROUND_FIX_VERSION_MINOR;
	ground_fix.is_core = false;
	ground_fix.fields = fields;
	ground_fix.field_count = ARRAY_SIZE(fields);
	ground_fix.max_instance_count = 1U;
	ground_fix.create_cb = ground_fix_create;
	lwm2m_register_obj(&ground_fix);

	ret = lwm2m_create_obj_inst(GROUND_FIX_OBJECT_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_ERR("Create ground fix object error: %d", ret);
	}

	return 0;
}

LWM2M_OBJ_INIT(lwm2m_ground_fix_init);

void ground_fix_set_report_back(bool report_back)
{
	send_location_back = report_back;
}

int32_t ground_fix_get_result_code(void)
{
	return result;
}
