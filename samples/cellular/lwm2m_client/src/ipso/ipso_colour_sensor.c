/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/*
 * Source material for IPSO Colour object (3335) v1.1:
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/version_history/3335-1_0.xml
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <lwm2m_object.h>
#include <lwm2m_engine.h>
#include <lwm2m_resource_ids.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_sensors, CONFIG_APP_LOG_LEVEL);

#define IPSO_OBJECT_COLOUR_ID 3335

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_INSTANCE_COUNT

#define COLOUR_VERSION_MAJOR 1

#ifdef CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1
#define COLOUR_VERSION_MINOR 1
#define NUMBER_OF_OBJECT_FIELDS 7
#else
#define COLOUR_VERSION_MINOR 0
#define NUMBER_OF_OBJECT_FIELDS 3
#endif /* ifdef CONFIG_LWM2M_IPSO_APP_COLOUR_VERSION_1_1 */

#define RESOURCE_INSTANCE_COUNT NUMBER_OF_OBJECT_FIELDS

#define UNIT_STR_MAX_SIZE 8
#define COLOUR_STR_MAX_SIZE 16
#define APP_TYPE_STR_MAX_SIZE 25

#define SENSOR_NAME "Colour"

static char colour[MAX_INSTANCE_COUNT][COLOUR_STR_MAX_SIZE];
static char units[MAX_INSTANCE_COUNT][UNIT_STR_MAX_SIZE];
static char app_type[MAX_INSTANCE_COUNT][APP_TYPE_STR_MAX_SIZE];

static struct lwm2m_engine_obj sensor;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(COLOUR_RID, R, STRING),
	OBJ_FIELD_DATA(SENSOR_UNITS_RID, R_OPT, STRING),
	OBJ_FIELD_DATA(APPLICATION_TYPE_RID, RW_OPT, STRING),
#ifdef CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1
	OBJ_FIELD_DATA(TIMESTAMP_RID, R_OPT, TIME),
	OBJ_FIELD_DATA(FRACTIONAL_TIMESTAMP_RID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(MEASUREMENT_QUALITY_INDICATOR_RID, R_OPT, U8),
	OBJ_FIELD_DATA(MEASUREMENT_QUALITY_LEVEL_RID, R_OPT, U8)
#endif
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][NUMBER_OF_OBJECT_FIELDS];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *colour_sensor_create(uint16_t obj_inst_id)
{
	int index, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create instance - already existing: %u", obj_inst_id);
			return NULL;
		}
	}

	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (!inst[index].obj) {
			break;
		}
	}

	if (index >= MAX_INSTANCE_COUNT) {
		LOG_ERR("Can not create instance - no more room: %u", obj_inst_id);
		return NULL;
	}

	/* Set default values */
	colour[index][0] = '\0';
	units[index][0] = '\0';
	app_type[index][0] = '\0';

	(void)memset(res[index], 0, sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA_LEN(COLOUR_RID, res[index], i, res_inst[index], j, colour[index],
			      COLOUR_STR_MAX_SIZE, 0);
	INIT_OBJ_RES_DATA_LEN(SENSOR_UNITS_RID, res[index], i, res_inst[index], j, units[index],
			      UNIT_STR_MAX_SIZE, 0);
	INIT_OBJ_RES_DATA_LEN(APPLICATION_TYPE_RID, res[index], i, res_inst[index], j,
			      app_type[index], APP_TYPE_STR_MAX_SIZE, 0);
#ifdef CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1
	INIT_OBJ_RES_OPTDATA(TIMESTAMP_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(FRACTIONAL_TIMESTAMP_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(MEASUREMENT_QUALITY_INDICATOR_RID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(MEASUREMENT_QUALITY_LEVEL_RID, res[index], i, res_inst[index], j);
#endif

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	LOG_DBG("Created IPSO %s Sensor instance: %d", SENSOR_NAME, obj_inst_id);
	return &inst[index];
}

static int ipso_colour_sensor_init(void)
{
	sensor.obj_id = IPSO_OBJECT_COLOUR_ID;
	sensor.version_major = COLOUR_VERSION_MAJOR;
	sensor.version_minor = COLOUR_VERSION_MINOR;
	sensor.is_core = false;
	sensor.fields = fields;
	sensor.field_count = ARRAY_SIZE(fields);
	sensor.max_instance_count = MAX_INSTANCE_COUNT;
	sensor.create_cb = colour_sensor_create;
	lwm2m_register_obj(&sensor);

	return 0;
}

SYS_INIT(ipso_colour_sensor_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
