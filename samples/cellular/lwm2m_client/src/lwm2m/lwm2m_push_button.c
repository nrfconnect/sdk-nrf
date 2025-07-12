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
#include "lwm2m_engine.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_lwm2m, CONFIG_APP_LOG_LEVEL);

#define BUTTON1_OBJ_INST_ID 0
#define BUTTON1_APP_NAME "Push button 1"

/* Button 2 not supported on Thingy:91 */
#define BUTTON2_OBJ_INST_ID 1
#define BUTTON2_APP_NAME "Push button 2"

static const char * const button_names[] = {
	"Push button 1",
#if CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT > 1
	"Push button 2",
#endif
#if CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT > 2
	"Push button 3",
#endif
#if CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT > 3
	"Push button 4",
#endif
};

static time_t lwm2m_timestamp[ARRAY_SIZE(button_names)];

static int lwm2m_init_push_button(void)
{
	int ret;

	ret = ui_input_init();
	if (ret < 0) {
		return ret;
	}

	/* create button objects */
	for (int i = 0; i < ARRAY_SIZE(button_names); i++) {
		struct lwm2m_obj_path path = LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, i);

		ret = lwm2m_create_object_inst(&path);
		if (ret < 0) {
			return ret;
		}

		path = LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, i, APPLICATION_TYPE_RID);

		const uint16_t len = sizeof("Push button X");

		ret = lwm2m_set_res_buf(&path, (void *)button_names[i], len, len,
					LWM2M_RES_DATA_FLAG_RO);
		if (ret < 0) {
			return ret;
		}

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)) {
			path = LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, i, TIMESTAMP_RID);
			ret = lwm2m_set_res_buf(&path, &lwm2m_timestamp[i], sizeof(time_t),
						sizeof(time_t), 0);
			if (ret < 0) {
				return ret;
			}
		}
		path = LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, i, DIGITAL_INPUT_STATE_RID);
		ret = lwm2m_set_bool(&path, false);
		if (ret < 0) {
			return ret;
		}

		path = LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, i, DIGITAL_INPUT_COUNTER_RID);
		ret = lwm2m_set_s64(&path, 0);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
LWM2M_APP_INIT(lwm2m_init_push_button);

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_ui_input_event(aeh)) {
		struct ui_input_event *event = cast_ui_input_event(aeh);

		if (event->type != PUSH_BUTTON) {
			return false;
		}

		uint16_t id = event->device_number - 1;

		lwm2m_set_bool(&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, id,
					  DIGITAL_INPUT_STATE_RID),
			       event->state);

		if (event->state) {
			if (IS_ENABLED(CONFIG_LWM2M_IPSO_PUSH_BUTTON_VERSION_1_1)) {
				set_ipso_obj_timestamp(IPSO_OBJECT_PUSH_BUTTON_ID,
						       id);
			}
		}

		LOG_DBG("Button %d changed state to %d.", event->device_number, event->state);
		return true;
	}

	return false;
}

APP_EVENT_LISTENER(button, app_event_handler);
APP_EVENT_SUBSCRIBE(button, ui_input_event);
