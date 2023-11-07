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

#define GROUND_FIX_VERSION_MAJOR 1
#define GROUND_FIX_VERSION_MINOR 0

/* Location Assistance resource IDs */
#define GROUND_FIX_SEND_LOCATION_BACK		0
#define GROUND_FIX_RESULT_CODE			1
#define GROUND_FIX_LATITUDE			2
#define GROUND_FIX_LONGITUDE			3
#define GROUND_FIX_ACCURACY			4

#define GROUND_FIX_MAX_ID			5

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

static int ground_fix_result_code_cb(uint16_t obj_inst_id, uint16_t res_id,
				     uint16_t res_inst_id, uint8_t *data,
				     uint16_t data_len, bool last_block,
				     size_t total_size)
{
	if (data_len != sizeof(int32_t)) {
		return -EINVAL;
	}

	result =  *(int32_t *)data;

	if (result_code_cb) {
		result_code_cb(result);
	}

	if (result != LOCATION_ASSIST_RESULT_CODE_OK) {
		LOG_ERR("Result code %d", result);
	}
	return 0;
}

static struct lwm2m_engine_obj_inst *ground_fix_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	init_res_instance(res_inst, ARRAY_SIZE(res_inst));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(GROUND_FIX_SEND_LOCATION_BACK, res, i, res_inst, j,
			  &send_location_back, sizeof(send_location_back));
	INIT_OBJ_RES(GROUND_FIX_RESULT_CODE, res, i, res_inst, j, 1, false, true, &result,
		     sizeof(result), NULL, NULL, NULL, ground_fix_result_code_cb, NULL);
	INIT_OBJ_RES_DATA(GROUND_FIX_LATITUDE, res, i, res_inst, j,
			  &latitude, sizeof(latitude));
	INIT_OBJ_RES_DATA(GROUND_FIX_LONGITUDE, res, i, res_inst, j,
			  &longitude, sizeof(longitude));
	INIT_OBJ_RES_DATA(GROUND_FIX_ACCURACY, res, i, res_inst, j,
			  &accuracy, sizeof(accuracy));

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

SYS_INIT(lwm2m_ground_fix_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

void ground_fix_set_report_back(bool report_back)
{
	send_location_back = report_back;
}

int32_t ground_fix_get_result_code(void)
{
	return result;
}
