/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>
#include "light_sensor.h"
#include "lwm2m_app_utils.h"
#include "lwm2m_engine.h"

#define LIGHT_OBJ_INSTANCE_ID 0
#define COLOUR_OBJ_INSTANCE_ID 1

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sensor_sim), okay)
#define LIGHT_APP_TYPE "Simulated Light Sensor"
#define COLOUR_APP_TYPE "Simulated Colour Sensor"
#else
#define LIGHT_APP_TYPE "BH1749 Light Sensor"
#define COLOUR_APP_TYPE "BH1749 Colour Sensor"
#endif

#define LIGHT_SENSOR_APP_NAME "Light sensor"
#define COLOUR_SENSOR_APP_NAME "Colour sensor"
#define LIGHT_UNIT "RGB-IR"

static time_t timestamp[2];

static int update_timestamp_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			       uint8_t *data, uint16_t data_len, bool last_block,
			       size_t total_size, size_t offset)
{
	set_ipso_obj_timestamp(IPSO_OBJECT_COLOUR_ID, obj_inst_id);
	return 0;
}

static int lwm2m_init_light_sensor(void)
{
	light_sensor_init();

	/* Ambient light sensor */
	lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID));
	lwm2m_set_string(&LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
					    APPLICATION_TYPE_RID),
				 LIGHT_APP_TYPE);
	lwm2m_set_string(&LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
					     SENSOR_UNITS_RID),
				  LIGHT_UNIT);

	/* Surface colour sensor */
	lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID));
	lwm2m_set_string(&LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID,
					     APPLICATION_TYPE_RID),
				  COLOUR_APP_TYPE);
	lwm2m_set_string(&LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID,
					     SENSOR_UNITS_RID),
				  LIGHT_UNIT);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1)) {
		/* Ambient light sensor */
		lwm2m_set_res_buf(
			&LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID, TIMESTAMP_RID),
			&timestamp[LIGHT_OBJ_INSTANCE_ID],
			sizeof(timestamp[LIGHT_OBJ_INSTANCE_ID]),
			sizeof(timestamp[LIGHT_OBJ_INSTANCE_ID]), LWM2M_RES_DATA_FLAG_RW);
		lwm2m_register_post_write_callback(
			&LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID, COLOUR_RID),
			update_timestamp_cb);

		/* Surface colour sensor */
		lwm2m_set_res_buf(
			&LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID, TIMESTAMP_RID),
			&timestamp[COLOUR_OBJ_INSTANCE_ID],
			sizeof(timestamp[COLOUR_OBJ_INSTANCE_ID]),
			sizeof(timestamp[COLOUR_OBJ_INSTANCE_ID]),
			LWM2M_RES_DATA_FLAG_RW);
		lwm2m_register_post_write_callback(
			&LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID, COLOUR_RID),
			update_timestamp_cb);
	}

	return 0;
}

LWM2M_APP_INIT(lwm2m_init_light_sensor);
