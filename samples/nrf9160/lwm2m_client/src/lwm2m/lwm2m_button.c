/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/lwm2m.h>

#include "ui.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_button, CONFIG_APP_LOG_LEVEL);

#define BUTTON1_NAME	"BUTTON1"
#define BUTTON2_NAME	"BUTTON2"
#define SWITCH1_NAME	"SWITCH1"
#define SWITCH2_NAME	"SWITCH2"

static int32_t timestamp[4];

int handle_button_events(struct ui_evt *evt)
{
	int32_t ts;
	bool state;

	if (!evt) {
		return -EINVAL;
	}

	/* skip input handling for flip sensor */
#if CONFIG_FLIP_INPUT > 0
	if (evt->button == CONFIG_FLIP_INPUT) {
		return -ENOENT;
	}
#endif

	/* get current time from device */
	lwm2m_engine_get_s32("3/0/13", &ts);
	state = (evt->type == UI_EVT_BUTTON_ACTIVE);
	switch (evt->button) {
	case UI_BUTTON_1:
		lwm2m_engine_set_bool("3347/0/5500", state);
		lwm2m_engine_set_s32("3347/0/5518", ts);
		return 0;
	case UI_BUTTON_2:
		lwm2m_engine_set_bool("3347/1/5500", state);
		lwm2m_engine_set_s32("3347/1/5518", ts);
		return 0;
	case UI_SWITCH_1:
		lwm2m_engine_set_bool("3342/0/5500", state);
		lwm2m_engine_set_s32("3342/0/5518", ts);
		return 0;
	case UI_SWITCH_2:
		lwm2m_engine_set_bool("3342/1/5500", state);
		lwm2m_engine_set_s32("3342/1/5518", ts);
		return 0;
	}

	return -ENOENT;
}

int lwm2m_init_button(void)
{
#if CONFIG_FLIP_INPUT != UI_BUTTON_1
	/* create button1 object */
	lwm2m_engine_create_obj_inst("3347/0");
	lwm2m_engine_set_bool("3347/0/5500", ui_button_is_active(UI_BUTTON_1));
	lwm2m_engine_set_res_data("3347/0/5750",
				  BUTTON1_NAME, sizeof(BUTTON1_NAME),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3347/0/5518",
				  &timestamp[0], sizeof(timestamp[0]), 0);
#endif

#if CONFIG_FLIP_INPUT != UI_BUTTON_2
	/* create button2 object */
	lwm2m_engine_create_obj_inst("3347/1");
	lwm2m_engine_set_bool("3347/1/5500", ui_button_is_active(UI_BUTTON_2));
	lwm2m_engine_set_res_data("3347/1/5750",
				  BUTTON2_NAME, sizeof(BUTTON2_NAME),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3347/1/5518",
				  &timestamp[1], sizeof(timestamp[1]), 0);
#endif

#if CONFIG_FLIP_INPUT != UI_SWITCH_1
	/* create switch1 object */
	lwm2m_engine_create_obj_inst("3342/0");
	lwm2m_engine_set_bool("3342/0/5500", ui_button_is_active(UI_SWITCH_1));
	lwm2m_engine_set_res_data("3342/0/5750",
				  SWITCH1_NAME, sizeof(SWITCH1_NAME),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3342/0/5518",
				  &timestamp[2], sizeof(timestamp[2]), 0);
#endif

#if CONFIG_FLIP_INPUT != UI_SWITCH_2
	/* create switch2 object */
	lwm2m_engine_create_obj_inst("3342/1");
	lwm2m_engine_set_bool("3342/1/5500", ui_button_is_active(UI_SWITCH_2));
	lwm2m_engine_set_res_data("3342/1/5750",
				  SWITCH2_NAME, sizeof(SWITCH2_NAME),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3342/1/5518",
				  &timestamp[3], sizeof(timestamp[3]), 0);
#endif

	return 0;
}
