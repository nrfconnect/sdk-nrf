/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>
#include "lwm2m_app_utils.h"

#define MODULE app_lwm2m_portfolio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);


static char host_device_id[40] = "Host Device ID #1";
static char manufacturer_id[40] = "Host Develce Manufacturer #1";
static char device_model[40] = "Host Device Model #1";
static char software_version_id[40] = "Host Device Software Version #1";

int lwm2m_init_portfolio_object(void)
{
	/* create switch1 object */
	lwm2m_engine_create_obj_inst(LWM2M_PATH(16, 0));
	lwm2m_engine_create_res_inst("16/0/0/0");
	lwm2m_engine_set_res_buf("16/0/0/0", host_device_id, 40, 40, 0);
	lwm2m_engine_create_res_inst("16/0/0/1");
	lwm2m_engine_set_res_buf("16/0/0/1", manufacturer_id, 40, 40, 0);
	lwm2m_engine_create_res_inst("16/0/0/2");
	lwm2m_engine_set_res_buf("16/0/0/2", device_model, 40, 40, 0);
	lwm2m_engine_create_res_inst("16/0/0/3");
	lwm2m_engine_set_res_buf("16/0/0/3", software_version_id, 40, 40, 0);

	return 0;
}
