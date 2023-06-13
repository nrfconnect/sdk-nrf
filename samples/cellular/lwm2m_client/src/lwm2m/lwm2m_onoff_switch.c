/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>

#include "ui_input.h"
#include "ui_input_event.h"
#include "lwm2m_app_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_lwm2m, CONFIG_APP_LOG_LEVEL);

#define SWICTH1_OBJ_INST_ID 0
#define SWITCH1_APP_NAME "On/Off Switch 1"

#define SWITCH2_OBJ_INST_ID 1
#define SWITCH2_APP_NAME "On/Off Switch 2"

static time_t lwm2m_timestamp[2];

int lwm2m_init_onoff_switch(void)
{
	ui_input_init();

	/* create switch1 object */
	lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_ONOFF_SWITCH_ID, SWICTH1_OBJ_INST_ID));
	lwm2m_set_res_buf(
		&LWM2M_OBJ(IPSO_OBJECT_ONOFF_SWITCH_ID, SWICTH1_OBJ_INST_ID, APPLICATION_TYPE_RID),
		SWITCH1_APP_NAME, sizeof(SWITCH1_APP_NAME), sizeof(SWITCH1_APP_NAME),
		LWM2M_RES_DATA_FLAG_RO);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_ONOFF_SWITCH_VERSION_1_1)) {
		lwm2m_set_res_buf(
			&LWM2M_OBJ(IPSO_OBJECT_ONOFF_SWITCH_ID, SWICTH1_OBJ_INST_ID, TIMESTAMP_RID),
			&lwm2m_timestamp[SWICTH1_OBJ_INST_ID],
			sizeof(lwm2m_timestamp[SWICTH1_OBJ_INST_ID]),
			sizeof(lwm2m_timestamp[SWICTH1_OBJ_INST_ID]),
			LWM2M_RES_DATA_FLAG_RW);
	}

	/* create switch2 object */
	lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_ONOFF_SWITCH_ID, SWITCH2_OBJ_INST_ID));
	lwm2m_set_res_buf(
		&LWM2M_OBJ(IPSO_OBJECT_ONOFF_SWITCH_ID, SWITCH2_OBJ_INST_ID, APPLICATION_TYPE_RID),
		SWITCH2_APP_NAME, sizeof(SWITCH2_APP_NAME), sizeof(SWITCH2_APP_NAME),
		LWM2M_RES_DATA_FLAG_RO);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_ONOFF_SWITCH_VERSION_1_1)) {
		lwm2m_set_res_buf(
			&LWM2M_OBJ(IPSO_OBJECT_ONOFF_SWITCH_ID, SWITCH2_OBJ_INST_ID, TIMESTAMP_RID),
			&lwm2m_timestamp[SWITCH2_OBJ_INST_ID],
			sizeof(lwm2m_timestamp[SWITCH2_OBJ_INST_ID]),
			sizeof(lwm2m_timestamp[SWITCH2_OBJ_INST_ID]),
			LWM2M_RES_DATA_FLAG_RW);
	}

	return 0;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_ui_input_event(aeh)) {
		struct ui_input_event *event = cast_ui_input_event(aeh);

		if (event->type != ON_OFF_SWITCH) {
			return false;
		}

		switch (event->device_number) {
		case 1:
			lwm2m_set_bool(&LWM2M_OBJ(IPSO_OBJECT_ONOFF_SWITCH_ID,
						  SWICTH1_OBJ_INST_ID,
						  DIGITAL_INPUT_STATE_RID),
				       event->state);
			if (IS_ENABLED(CONFIG_LWM2M_IPSO_ONOFF_SWITCH_VERSION_1_1)) {
				set_ipso_obj_timestamp(IPSO_OBJECT_ONOFF_SWITCH_ID,
						    SWICTH1_OBJ_INST_ID);
			}
			break;

		case 2:
			lwm2m_set_bool(&LWM2M_OBJ(IPSO_OBJECT_ONOFF_SWITCH_ID,
						  SWITCH2_OBJ_INST_ID,
						  DIGITAL_INPUT_STATE_RID),
				       event->state);
			if (IS_ENABLED(CONFIG_LWM2M_IPSO_ONOFF_SWITCH_VERSION_1_1)) {
				set_ipso_obj_timestamp(IPSO_OBJECT_ONOFF_SWITCH_ID,
						    SWITCH2_OBJ_INST_ID);
			}
			break;

		default:
			return false;
		}

		LOG_DBG("Switch %d changed state to %d.", event->device_number, event->state);
		return true;
	}

	return false;
}

APP_EVENT_LISTENER(onoff_switch, app_event_handler);
APP_EVENT_SUBSCRIBE(onoff_switch, ui_input_event);
