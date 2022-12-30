/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>

#include "lwm2m_app_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m, CONFIG_APP_LOG_LEVEL);

void set_ipso_obj_timestamp(int ipso_obj_id, unsigned int obj_inst_id)
{
	int ret;
	char path[MAX_LWM2M_PATH_LEN];

	snprintk(path, MAX_LWM2M_PATH_LEN, "%d/%u/%d", ipso_obj_id, obj_inst_id, TIMESTAMP_RID);

	ret = lwm2m_engine_set_time(path, time(NULL));
	if (ret) {
		LOG_ERR("Unable to set timestamp");
	}
}
