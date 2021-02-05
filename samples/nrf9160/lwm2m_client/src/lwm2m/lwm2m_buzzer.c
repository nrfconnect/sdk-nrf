/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/lwm2m.h>

#include "ui.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_buzzer, CONFIG_APP_LOG_LEVEL);

#define BUZZER_NAME	"BUZZER"

static int buzzer_state_cb(uint16_t obj_inst_id,
			   uint16_t res_id, uint16_t res_inst_id,
			   uint8_t *data, uint16_t data_len,
			   bool last_block, size_t total_size)
{
	if (*data == 0) {
		LOG_DBG("Buzzer OFF");
		/* TODO: Fix secure memory access crash */
		/* ui_buzzer_set_frequency(0, 0); */
	} else {
		LOG_DBG("Buzzer ON");
		/* TODO: Fix secure memory access crash */
		/* ui_buzzer_set_frequency(5000, 35); */
	}

	return 0;
}

int lwm2m_init_buzzer(void)
{
	/* create buzzer object */
	lwm2m_engine_create_obj_inst("3338/0");
	lwm2m_engine_register_post_write_callback("3338/0/5500",
						  buzzer_state_cb);
	lwm2m_engine_set_res_data("3338/0/5750",
				  BUZZER_NAME, sizeof(BUZZER_NAME),
				  LWM2M_RES_DATA_FLAG_RO);

	return 0;
}
