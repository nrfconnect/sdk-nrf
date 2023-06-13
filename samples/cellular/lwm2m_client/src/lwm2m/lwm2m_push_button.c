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

#define BUTTON1_OBJ_INST_ID 0
#define BUTTON1_APP_NAME "Push button 1"

/* Button 2 not supported on Thingy:91 */
#define BUTTON2_OBJ_INST_ID 1
#define BUTTON2_APP_NAME "Push button 2"

static time_t lwm2m_timestamp[2];

int lwm2m_init_push_button(void)
{
	ui_input_init();

	/* create button1 object */
	lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID));

	lwm2m_set_res_buf(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, APPLICATION_TYPE_RID),
		BUTTON1_APP_NAME, sizeof(BUTTON1_APP_NAME), sizeof(BUTTON1_APP_NAME),
		LWM2M_RES_DATA_FLAG_RO);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)) {
		lwm2m_set_res_buf(
			&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, TIMESTAMP_RID),
			&lwm2m_timestamp[BUTTON1_OBJ_INST_ID],
			sizeof(lwm2m_timestamp[BUTTON1_OBJ_INST_ID]),
			sizeof(lwm2m_timestamp[BUTTON1_OBJ_INST_ID]),
			LWM2M_RES_DATA_FLAG_RW);
	}

	if (CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT == 2) {
		/* create button2 object */
		lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID,
					 BUTTON2_OBJ_INST_ID));

		lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID,
					     BUTTON2_OBJ_INST_ID, APPLICATION_TYPE_RID),
				  BUTTON2_APP_NAME, sizeof(BUTTON2_APP_NAME),
				  sizeof(BUTTON2_APP_NAME), LWM2M_RES_DATA_FLAG_RO);

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)) {
			lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID,
						     BUTTON2_OBJ_INST_ID, TIMESTAMP_RID),
					  &lwm2m_timestamp[BUTTON2_OBJ_INST_ID],
					  sizeof(lwm2m_timestamp[BUTTON2_OBJ_INST_ID]),
					  sizeof(lwm2m_timestamp[BUTTON2_OBJ_INST_ID]),
					  LWM2M_RES_DATA_FLAG_RW);
		}
	}

	return 0;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_ui_input_event(aeh)) {
		struct ui_input_event *event = cast_ui_input_event(aeh);

		if (event->type != PUSH_BUTTON) {
			return false;
		}

		switch (event->device_number) {
		case 1:
			lwm2m_set_bool(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID,
						  BUTTON1_OBJ_INST_ID,
						  DIGITAL_INPUT_STATE_RID),
				       event->state);

			if (event->state) {
				if (IS_ENABLED(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)) {
					set_ipso_obj_timestamp(IPSO_OBJECT_PUSH_BUTTON_ID,
							    BUTTON1_OBJ_INST_ID);
				}
			}
			break;

		case 2:
			lwm2m_set_bool(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID,
						  BUTTON2_OBJ_INST_ID,
						  DIGITAL_INPUT_STATE_RID),
				       event->state);

			if (event->state) {
				if (IS_ENABLED(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)) {
					set_ipso_obj_timestamp(IPSO_OBJECT_PUSH_BUTTON_ID,
							    BUTTON2_OBJ_INST_ID);
				}
			}
			break;

		default:
			return false;
		}

		LOG_DBG("Button %d changed state to %d.", event->device_number, event->state);
		return true;
	}

	return false;
}

APP_EVENT_LISTENER(button, app_event_handler);
APP_EVENT_SUBSCRIBE(button, ui_input_event);
