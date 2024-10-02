/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/lwm2m.h>
#include "lwm2m_engine.h"

/*
 * Our location libraries make use of optional resources on LwM2M Location object
 * so provide a buffer for them.
 */

#define LOCATION_ALTITUDE_ID 2
#define LOCATION_RADIUS_ID   3
#define LOCATION_VELOCITY_ID 4

static double altitude;
static double radius;
static double velocity;

static int lwm2m_init_optional_location_res(void)
{
	struct lwm2m_obj_path path;
	int ret;

	path = LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LOCATION_ALTITUDE_ID);
	ret = lwm2m_set_res_buf(&path, &altitude, sizeof(altitude), sizeof(altitude), 0);
	if (ret < 0) {
		return ret;
	}

	path = LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LOCATION_RADIUS_ID);
	ret = lwm2m_set_res_buf(&path, &radius, sizeof(radius), sizeof(radius), 0);
	if (ret < 0) {
		return ret;
	}

	path = LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LOCATION_VELOCITY_ID);
	ret = lwm2m_set_res_buf(&path, &velocity, sizeof(velocity), sizeof(velocity), 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

LWM2M_APP_INIT(lwm2m_init_optional_location_res);
