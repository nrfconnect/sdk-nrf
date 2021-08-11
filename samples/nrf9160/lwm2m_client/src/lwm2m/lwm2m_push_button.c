/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/lwm2m.h>
#include <lwm2m_resource_ids.h>

#include "ui_input.h"
#include "ui_input_event.h"
#include "lwm2m_app_utils.h"

#define MODULE app_lwm2m_push_button

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);

#define BUTTON1_OBJ_INST_ID 0
#define BUTTON1_APP_NAME "Push button 1"

/* Button 2 not supported on Thingy:91 */
#define BUTTON2_OBJ_INST_ID 1
#define BUTTON2_APP_NAME "Push button 2"

static uint64_t btn_input_count[2];
static int32_t lwm2m_timstamp[2];

int lwm2m_init_push_button(void)
{
	ui_input_init();

	/* create button1 object */
	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID));
	/*
	 * Overwriting post write callback of Digital Input State, as the original
	 * callback in the ipso object directly modifies the Digital Input Counter
	 * resource data buffer without notifying the engine, which effectively
	 * disables Value Tracking functionality for the counter resource.
	 * Won't be needed with new Zephyr update.
	 */
	lwm2m_engine_register_post_write_callback(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
							     BUTTON1_OBJ_INST_ID,
							     DIGITAL_INPUT_STATE_RID),
						  NULL);
	lwm2m_engine_set_res_data(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, APPLICATION_TYPE_RID),
		BUTTON1_APP_NAME, sizeof(BUTTON1_APP_NAME), LWM2M_RES_DATA_FLAG_RO);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)) {
		lwm2m_engine_set_res_data(
			LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, TIMESTAMP_RID),
			&lwm2m_timstamp[BUTTON1_OBJ_INST_ID],
			sizeof(lwm2m_timstamp[BUTTON1_OBJ_INST_ID]), LWM2M_RES_DATA_FLAG_RW);
	}

	if (CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT == 2) {
		/* create button2 object */
		lwm2m_engine_create_obj_inst(
			LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON2_OBJ_INST_ID));
		lwm2m_engine_register_post_write_callback(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
								     BUTTON2_OBJ_INST_ID,
								     DIGITAL_INPUT_STATE_RID),
							  NULL);
		lwm2m_engine_set_res_data(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
						     BUTTON2_OBJ_INST_ID, APPLICATION_TYPE_RID),
					  BUTTON2_APP_NAME, sizeof(BUTTON2_APP_NAME),
					  LWM2M_RES_DATA_FLAG_RO);

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)) {
			lwm2m_engine_set_res_data(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
							     BUTTON2_OBJ_INST_ID, TIMESTAMP_RID),
						  &lwm2m_timstamp[BUTTON2_OBJ_INST_ID],
						  sizeof(lwm2m_timstamp[BUTTON2_OBJ_INST_ID]),
						  LWM2M_RES_DATA_FLAG_RW);
		}
	}

	return 0;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_ui_input_event(eh)) {
		struct ui_input_event *event = cast_ui_input_event(eh);

		if (event->type != PUSH_BUTTON) {
			return false;
		}

		switch (event->device_number) {
		case 1:
			lwm2m_engine_set_bool(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
							 BUTTON1_OBJ_INST_ID,
							 DIGITAL_INPUT_STATE_RID),
					      event->state);

			if (event->state) {
				if (IS_ENABLED(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)) {
					lwm2m_set_timestamp(IPSO_OBJECT_PUSH_BUTTON_ID,
							    BUTTON1_OBJ_INST_ID);
				}

				/*
				 * Won't be needed with new Zephyr update, as the counter
				 * object is automatically updated in the ipso_push_button file.
				 */
				btn_input_count[BUTTON1_OBJ_INST_ID]++;
				lwm2m_engine_set_u64(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
								BUTTON1_OBJ_INST_ID,
								DIGITAL_INPUT_COUNTER_RID),
						     btn_input_count[BUTTON1_OBJ_INST_ID]);
			}
			break;

		case 2:
			lwm2m_engine_set_bool(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
							 BUTTON2_OBJ_INST_ID,
							 DIGITAL_INPUT_STATE_RID),
					      event->device_number);

			if (event->state) {
				if (IS_ENABLED(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)) {
					lwm2m_set_timestamp(IPSO_OBJECT_PUSH_BUTTON_ID,
							    BUTTON2_OBJ_INST_ID);
				}

				/*
				 * Won't be needed with new Zephyr update, as the counter
				 * object is automatically updated in the ipso_push_button file.
				 */
				btn_input_count[BUTTON2_OBJ_INST_ID]++;
				lwm2m_engine_set_u64(LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID,
								BUTTON2_OBJ_INST_ID,
								DIGITAL_INPUT_COUNTER_RID),
						     btn_input_count[BUTTON2_OBJ_INST_ID]);
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

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, ui_input_event);
