/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>

#include "esl.h"

LOG_MODULE_DECLARE(peripheral_esl);

void hw_chrc_init(struct bt_esl_init_param *init_param)
{
	/** Initial Display elements here **/
	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_DISPLAY_MAX; idx++) {
		init_param->displays[idx].width = CONFIG_ESL_DISPLAY_WIDTH;
		init_param->displays[idx].height = CONFIG_ESL_DISPLAY_HEIGHT;
		init_param->displays[idx].type = CONFIG_ESL_DISPLAY_TYPE;
	}

	/** Initial Sensor elements here
	 * There is always temperature sensor in nRF52
	 * Present device operating temperature.
	 **/
#define BT_MESH_PROP_ID_PRESENT_DEV_OP_TEMP 0x0054
#define DUMMY_SENSOR_CODE		    0x9487
	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_SENSOR_MAX; idx++) {
		if (idx == 0) {
			init_param->sensors[idx].size = 0;
			init_param->sensors[idx].property_id = BT_MESH_PROP_ID_PRESENT_DEV_OP_TEMP;
			init_param->sensor_data[idx].size = 2;
		} else {
			/** Demostrate how to assign long sensor type*/
			init_param->sensors[idx].size = 1;
			init_param->sensors[idx].vendor_specific.company_id = CONFIG_BT_DIS_PNP_VID;
			init_param->sensors[idx].vendor_specific.sensor_code = DUMMY_SENSOR_CODE;
			init_param->sensor_data[idx].size = (idx + 2) % ESL_RESPONSE_SENSOR_LEN;
		}
		/* Assume data of all the sensos is not ready */
		init_param->sensor_data[idx].data_available = false;
	}
	/** Initial LED elements here **/
#if (CONFIG_BOARD_NRF52840_PAPYR)
	init_param->led_type[0] = (ESL_LED_MONO << ESL_LED_TYPE_BIT) | BIT(ESL_LED_GREEN_HI_BIT) |
				  BIT(ESL_LED_GREEN_LO_BIT);
	init_param->led_type[1] = (ESL_LED_MONO << ESL_LED_TYPE_BIT) | BIT(ESL_LED_BLUE_HI_BIT) |
				  BIT(ESL_LED_BLUE_LO_BIT);
	init_param->led_type[2] = (ESL_LED_MONO << ESL_LED_TYPE_BIT) | BIT(ESL_LED_RED_HI_BIT) |
				  BIT(ESL_LED_RED_LO_BIT);
#elif (CONFIG_BOARD_THINGY52_NRF52832)
	init_param->led_type[0] = (ESL_LED_MONO << ESL_LED_TYPE_BIT) | BIT(ESL_LED_RED_HI_BIT) |
				  BIT(ESL_LED_RED_HI_BIT);
	init_param->led_type[1] = (ESL_LED_MONO << ESL_LED_TYPE_BIT) | BIT(ESL_LED_GREEN_HI_BIT) |
				  BIT(ESL_LED_GREEN_HI_BIT);
	init_param->led_type[2] = (ESL_LED_MONO << ESL_LED_TYPE_BIT) | BIT(ESL_LED_BLUE_HI_BIT) |
				  BIT(ESL_LED_BLUE_HI_BIT);
#else /* DK has only green monochrome LED*/
	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_LED_MAX; idx++) {
		init_param->led_type[idx] = (ESL_LED_MONO << ESL_LED_TYPE_BIT) |
					    BIT(ESL_LED_GREEN_HI_BIT) | BIT(ESL_LED_GREEN_LO_BIT);
	}
#if defined(CONFIG_BT_ESL_PTS)
	/* set led 0 as rgb for PTS test */
	init_param->led_type[0] = (ESL_LED_SRGB << ESL_LED_TYPE_BIT) | BIT(ESL_LED_GREEN_HI_BIT) |
				  BIT(ESL_LED_GREEN_LO_BIT);
#endif /* CONFIG_BT_ESL_PTS */
#endif /* (CONFIG_BOARD_NRF52840_PAPYR) (CONFIG_BOARD_THINGY52_NRF52832) */
}

void buffer_img(const void *data, size_t len, off_t offset)
{
	struct bt_esls *esl_obj = esl_get_esl_obj();

	LOG_DBG("buffering len %d offset %ld", len, offset);
#if defined(CONFIG_BT_ESL_IMAGE_AVAILABLE)
	memcpy((esl_obj->img_obj_buf + offset), data, len);
#endif
}
