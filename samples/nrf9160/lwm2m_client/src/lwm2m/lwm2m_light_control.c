/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <ui.h>
#include <net/lwm2m.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_light, CONFIG_APP_LOG_LEVEL);

#define LIGHT_NAME	"LED1"

static uint32_t led_state;

/* TODO: Move to a pre write hook that can handle ret codes once available */
static int lc_on_off_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			uint8_t *data, uint16_t data_len,
			bool last_block, size_t total_size)
{
	uint32_t led_val;

	led_val = *(uint8_t *) data;
	if (led_val != led_state) {
		ui_led_set_state(UI_LED_1, led_val);
		led_state = led_val;
		/* TODO: Move to be set by an internal post write function */
		lwm2m_engine_set_s32("3311/0/5852", 0);
	}

	return 0;
}

int lwm2m_init_light_control(void)
{
	/* start with LED off */
	ui_led_set_state(UI_LED_1, 0);

	/* create light control device */
	lwm2m_engine_create_obj_inst("3311/0");
	lwm2m_engine_register_post_write_callback("3311/0/5850", lc_on_off_cb);
	lwm2m_engine_set_res_data("3311/0/5750",
				  LIGHT_NAME, sizeof(LIGHT_NAME),
				  LWM2M_RES_DATA_FLAG_RO);

	return 0;
}
