/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/lwm2m.h>
#include <lwm2m_resource_ids.h>

#include "lwm2m_app_utils.h"

#define NOTIFICATION_REQUEST_DELAY_MS 1500

bool is_regular_read_cb(int64_t read_timestamp)
{
	int64_t dt = k_uptime_get() - read_timestamp;

	return dt > NOTIFICATION_REQUEST_DELAY_MS;
}

void lwm2m_set_timestamp(int ipso_obj_id, unsigned int obj_inst_id)
{
	int32_t timestamp;
	char path[MAX_LWM2M_PATH_LEN];

	lwm2m_engine_get_s32(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, CURRENT_TIME_RID), &timestamp);
	snprintk(path, MAX_LWM2M_PATH_LEN, "%d/%u/%d", ipso_obj_id, obj_inst_id, TIMESTAMP_RID);
	lwm2m_engine_set_s32(path, timestamp);
}

float32_value_t double_to_float32(double val)
{
	float32_value_t out;

	out.val1 = (int32_t)val;
	out.val2 = (int32_t)((out.val1 - val) * LWM2M_FLOAT32_DEC_MAX);
	return out;
}

float32_value_t sensor_value_to_float32(struct sensor_value val)
{
	return (float32_value_t){ .val1 = val.val1, .val2 = val.val2 };
}

double float32_to_double(float32_value_t val)
{
	return (double)val.val1 + (double)val.val2 / 1000000;
}
