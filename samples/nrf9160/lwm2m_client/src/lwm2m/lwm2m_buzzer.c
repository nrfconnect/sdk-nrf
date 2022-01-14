/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/lwm2m.h>
#include <lwm2m_resource_ids.h>

#include "ui_buzzer.h"
#include "lwm2m_app_utils.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_buzzer, CONFIG_APP_LOG_LEVEL);

#define FREQUENCY_START_VAL 440U
#define INTENSITY_START_VAL 100.0

#define BUZZER_APP_TYPE "Buzzer"

static int32_t lwm2m_timestamp;

static int buzzer_state_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			   uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	int ret;
	bool state = *(bool *)data;

	ret = ui_buzzer_on_off(state);
	if (ret) {
		LOG_ERR("Set buzzer on/off failed (%d)", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_BUZZER_VERSION_1_1)) {
		lwm2m_set_timestamp(IPSO_OBJECT_BUZZER_ID, obj_inst_id);
	}

	LOG_DBG("Buzzer on/off: %d", state);

	return 0;
}

static int buzzer_intensity_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			       uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	int ret;
	uint8_t intensity = *data;

	ret = ui_buzzer_set_intensity(intensity);
	if (ret) {
		LOG_ERR("Set dutycycle failed (%d)", ret);
		return ret;
	}

	LOG_DBG("Intensity: %u", intensity);

	return 0;
}

int lwm2m_init_buzzer(void)
{
	int ret;
	double start_intensity = INTENSITY_START_VAL;

	ret = ui_buzzer_init();
	if (ret) {
		LOG_ERR("Init ui buzzer failed (%d)", ret);
		return ret;
	}

	ret = ui_buzzer_set_intensity(INTENSITY_START_VAL);
	if (ret) {
		LOG_ERR("Set buzzer intensity failed (%d)", ret);
	}
	ret = ui_buzzer_set_frequency(FREQUENCY_START_VAL);
	if (ret) {
		LOG_ERR("Set buzzer frequency failed (%d)", ret);
	}

	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_BUZZER_ID, 0));
	lwm2m_engine_register_post_write_callback(
		LWM2M_PATH(IPSO_OBJECT_BUZZER_ID, 0, DIGITAL_INPUT_STATE_RID), buzzer_state_cb);
	lwm2m_engine_register_post_write_callback(LWM2M_PATH(IPSO_OBJECT_BUZZER_ID, 0, LEVEL_RID),
						  buzzer_intensity_cb);
	lwm2m_engine_set_res_data(LWM2M_PATH(IPSO_OBJECT_BUZZER_ID, 0, APPLICATION_TYPE_RID),
				  BUZZER_APP_TYPE, sizeof(BUZZER_APP_TYPE), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_BUZZER_ID, 0, LEVEL_RID), &start_intensity);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_BUZZER_VERSION_1_1)) {
		lwm2m_engine_set_res_data(LWM2M_PATH(IPSO_OBJECT_BUZZER_ID, 0, TIMESTAMP_RID),
					  &lwm2m_timestamp, sizeof(lwm2m_timestamp),
					  LWM2M_RES_DATA_FLAG_RW);
	}

	return 0;
}
