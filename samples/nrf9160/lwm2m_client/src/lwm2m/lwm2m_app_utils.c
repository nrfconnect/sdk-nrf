/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>

#include "lwm2m_app_utils.h"

LOG_MODULE_REGISTER(app_lwm2m_utils, CONFIG_APP_LOG_LEVEL);

void set_ipso_obj_timestamp(int ipso_obj_id, unsigned int obj_inst_id)
{
	int32_t timestamp;
	char path[MAX_LWM2M_PATH_LEN];

	int ret = lwm2m_engine_get_s32(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
					CURRENT_TIME_RID), &timestamp);
	if (ret) {
		LOG_ERR("Unable to retrieve timestamp");
	}

	snprintk(path, MAX_LWM2M_PATH_LEN, "%d/%u/%d", ipso_obj_id, obj_inst_id, TIMESTAMP_RID);

	ret = lwm2m_engine_set_s32(path, timestamp);
	if (ret) {
		LOG_ERR("Unable to set timestamp");
	}
}
