/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define LOG_MODULE_NAME net_lwm2m_obj_configuration
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define OBJECT_ID 50009
#define OBJECT_VERSION_MAJOR 1
#define OBJECT_VERSION_MINOR 0

/* Configuration object resource IDs */
#define RESOURCE_PASSIVE_MODE			0
#define RESOURCE_LOCATION_TIMEOUT		1
#define RESOURCE_ACTIVE_WAIT_TIMEOUT		2
#define RESOURCE_MOVEMENT_RESOLUTION		3
#define RESOURCE_MOVEMENT_TIMEOUT		4
#define RESOURCE_ACCELEROMETER_ACT_THRESHOLD	5
#define RESOURCE_GNSS_ENABLE			6
#define RESOURCE_NEIGHBOR_CELL_ENABLE		7
#define RESOURCE_ACCELEROMETER_INACT_THRESHOLD	8
#define RESOURCE_ACCELEROMETER_INACT_TIMEOUT	9

#define RESOURCES_MAX_ID			10
#define RESOURCE_INSTANCE_COUNT	(RESOURCES_MAX_ID)

/* Storage variables to hold configuration values. */
static bool passive_mode;
static int location_timeout;
static int active_wait_timeout;
static int movement_resolution;
static int movement_timeout;
static double accelerometer_activity_threshold;
static double accelerometer_inactivity_threshold;
static double accelerometer_inactivity_timeout;
static bool gnss_enable;
static bool ncell_enable;

static struct lwm2m_engine_obj object;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(RESOURCE_PASSIVE_MODE, RW, BOOL),
	OBJ_FIELD_DATA(RESOURCE_LOCATION_TIMEOUT, RW, S32),
	OBJ_FIELD_DATA(RESOURCE_ACTIVE_WAIT_TIMEOUT, RW, S32),
	OBJ_FIELD_DATA(RESOURCE_MOVEMENT_RESOLUTION, RW, S32),
	OBJ_FIELD_DATA(RESOURCE_MOVEMENT_TIMEOUT, RW, S32),
	OBJ_FIELD_DATA(RESOURCE_ACCELEROMETER_ACT_THRESHOLD, RW, FLOAT),
	OBJ_FIELD_DATA(RESOURCE_ACCELEROMETER_INACT_THRESHOLD, RW, FLOAT),
	OBJ_FIELD_DATA(RESOURCE_ACCELEROMETER_INACT_TIMEOUT, RW, FLOAT),
	OBJ_FIELD_DATA(RESOURCE_GNSS_ENABLE, RW, BOOL),
	OBJ_FIELD_DATA(RESOURCE_NEIGHBOR_CELL_ENABLE, RW, BOOL)
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[RESOURCES_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *object_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	init_res_instance(res_inst, ARRAY_SIZE(res_inst));

	/* Initialize object instance resource data */
	INIT_OBJ_RES_DATA(RESOURCE_PASSIVE_MODE, res, i, res_inst, j,
			  &passive_mode, sizeof(passive_mode));
	INIT_OBJ_RES_DATA(RESOURCE_LOCATION_TIMEOUT, res, i, res_inst, j,
			  &location_timeout, sizeof(location_timeout));
	INIT_OBJ_RES_DATA(RESOURCE_ACTIVE_WAIT_TIMEOUT, res, i, res_inst, j,
			  &active_wait_timeout, sizeof(active_wait_timeout));
	INIT_OBJ_RES_DATA(RESOURCE_MOVEMENT_RESOLUTION, res, i, res_inst, j,
			  &movement_resolution, sizeof(movement_resolution));
	INIT_OBJ_RES_DATA(RESOURCE_MOVEMENT_TIMEOUT, res, i, res_inst, j,
			  &movement_timeout, sizeof(movement_timeout));
	INIT_OBJ_RES_DATA(RESOURCE_ACCELEROMETER_ACT_THRESHOLD, res, i, res_inst, j,
			  &accelerometer_activity_threshold,
			  sizeof(accelerometer_activity_threshold));
	INIT_OBJ_RES_DATA(RESOURCE_ACCELEROMETER_INACT_THRESHOLD, res, i, res_inst, j,
			  &accelerometer_inactivity_threshold,
			  sizeof(accelerometer_inactivity_threshold));
	INIT_OBJ_RES_DATA(RESOURCE_ACCELEROMETER_INACT_TIMEOUT, res, i, res_inst, j,
			  &accelerometer_inactivity_timeout,
			  sizeof(accelerometer_inactivity_timeout));
	INIT_OBJ_RES_DATA(RESOURCE_GNSS_ENABLE, res, i, res_inst, j,
			  &gnss_enable, sizeof(gnss_enable));
	INIT_OBJ_RES_DATA(RESOURCE_NEIGHBOR_CELL_ENABLE, res, i, res_inst, j,
			  &ncell_enable, sizeof(ncell_enable));

	inst.resources = res;
	inst.resource_count = i;

	LOG_DBG("Created a configuration object: %d", obj_inst_id);
	return &inst;
}

static int object_init(void)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	object.obj_id = OBJECT_ID;
	object.version_major = OBJECT_VERSION_MAJOR;
	object.version_minor = OBJECT_VERSION_MINOR;
	object.is_core = false;
	object.fields = fields;
	object.field_count = ARRAY_SIZE(fields);
	object.max_instance_count = 1U;
	object.create_cb = object_create;
	lwm2m_register_obj(&object);

	ret = lwm2m_create_obj_inst(OBJECT_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_ERR("Create configuration object error: %d", ret);
		return ret;
	}

	return 0;
}

SYS_INIT(object_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
