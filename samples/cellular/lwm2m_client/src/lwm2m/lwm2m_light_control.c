/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>
#include <stdlib.h>
#include "lwm2m_engine.h"
#include "lwm2m_app_utils.h"
#include "ui_led.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_lwm2m, CONFIG_APP_LOG_LEVEL);

#if (defined(CONFIG_BOARD_THINGY91_NRF9160_NS) || \
	defined(CONFIG_BOARD_THINGY91X_NRF9151_NS)) && defined(CONFIG_UI_LED_USE_PWM)
#define APP_TYPE	"RGB PWM LED controller"
#elif defined(CONFIG_BOARD_THINGY91_NRF9160_NS) || defined(CONFIG_BOARD_THINGY91X_NRF9151_NS)
#define APP_TYPE	"RGB GPIO LED controller"
#elif defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS) && defined(CONFIG_UI_LED_USE_PWM)
#define APP_TYPE	"PWM LED controller"
#elif defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS)
#define APP_TYPE	"GPIO LED controller"
#elif defined(CONFIG_BOARD_NRF9161DK_NRF9161_NS) && defined(CONFIG_UI_LED_USE_PWM)
#define APP_TYPE	"PWM LED controller"
#elif defined(CONFIG_BOARD_NRF9161DK_NRF9161_NS)
#define APP_TYPE	"GPIO LED controller"
#elif defined(CONFIG_BOARD_NRF9151DK_NRF9151_NS) && defined(CONFIG_UI_LED_USE_PWM)
#define APP_TYPE	"PWM LED controller"
#elif defined(CONFIG_BOARD_NRF9151DK_NRF9151_NS)
#define APP_TYPE	"GPIO LED controller"
#endif

#define BRIGHTNESS_MAX   100U

static bool state[NUM_LEDS];
static uint8_t brightness_val[NUM_LEDS];
static uint8_t colour_val[NUM_LEDS];

static void reset_on_time(uint16_t obj_inst_id)
{
	lwm2m_set_s32(&LWM2M_OBJ(IPSO_OBJECT_LIGHT_CONTROL_ID, obj_inst_id, ON_TIME_RID), 0);
}

static int rgb_lc_on_off_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			uint8_t *data, uint16_t data_len, bool last_block,
			size_t total_size, size_t offset)
{
	int ret = 0;
	bool new_state = *(bool *)data;

	if (IS_ENABLED(CONFIG_UI_LED_USE_PWM)) {
		/* Reset on-time if transition from off to on */
		if (state[0] == false && new_state == true) {
			reset_on_time(obj_inst_id);
		}

		for (int i = 0; i < NUM_LEDS; ++i) {
			state[i] = new_state;
			ret = ui_led_pwm_on_off(i, new_state);
			if (ret) {
				LOG_ERR("Set PWM LED %d on/off failed (%d)", i, ret);
				continue;
			}
		}
	} else if (IS_ENABLED(CONFIG_UI_LED_USE_GPIO)) {
		/* Reset on-time if transition from off to on */
		if (state[0] == false && new_state == true) {
			reset_on_time(obj_inst_id);
		}

		for (int i = 0; i < NUM_LEDS; ++i) {
			state[i] = new_state;
			ret = ui_led_gpio_on_off(i, state[i] && (bool)colour_val[i]);
			if (ret) {
				LOG_ERR("Set GPIO LED %d on/off failed (%d)", i, ret);
				continue;
			}
		}
	}

	return ret;
}

static uint8_t calculate_intensity(uint8_t colour_value, uint8_t brightness_value)
{
	uint32_t numerator = (uint32_t)colour_value * brightness_value;
	uint32_t denominator = BRIGHTNESS_MAX;

	return (uint8_t)(numerator / denominator);
}

static int rgb_lc_colour_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			uint8_t *data, uint16_t data_len, bool last_block,
			size_t total_size, size_t offset)
{
	int ret = 0;
	uint32_t colour_values = strtoul(data, NULL, 0);

	if (IS_ENABLED(CONFIG_UI_LED_USE_PWM)) {
		uint8_t intensity;

		for (int i = 0; i < NUM_LEDS; ++i) {
			colour_val[i] = (uint8_t)(colour_values >> 8 * (2 - i));
			intensity = calculate_intensity(colour_val[i], brightness_val[i]);
			ret = ui_led_pwm_set_intensity(i, intensity);
			if (ret) {
				LOG_ERR("Set PWM LED %d intensity failed (%d)", i, ret);
				continue;
			}
		}
	} else if (IS_ENABLED(CONFIG_UI_LED_USE_GPIO)) {
		for (int i = 0; i < NUM_LEDS; ++i) {
			colour_val[i] = (uint8_t)(colour_values >> 8 * (2 - i));
			ret = ui_led_gpio_on_off(i, state[i] && (bool)colour_val[i]);
			if (ret) {
				LOG_ERR("Set GPIO LED %d failed (%d)", i, ret);
				continue;
			}
		}
	}

	return ret;
}

static int rgb_lc_dimmer_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			uint8_t *data, uint16_t data_len, bool last_block,
			size_t total_size, size_t offset)
{
	int ret = 0;
	uint8_t new_brightness = *data;
	uint8_t intensity;

	if (IS_ENABLED(CONFIG_UI_LED_USE_PWM)) {
		for (int i = 0; i < NUM_LEDS; ++i) {
			brightness_val[i] = new_brightness;
			intensity = calculate_intensity(colour_val[i], brightness_val[i]);
			ret = ui_led_pwm_set_intensity(i, intensity);
			if (ret) {
				LOG_ERR("Set PWM LED %d intensity failed (%d)", i, ret);
				continue;
			}
		}
	}

	return ret;
}

static int lc_on_off_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			uint16_t data_len, bool last_block, size_t total_size, size_t offset)
{
	int ret = 0;
	bool new_state = *(bool *)data;

	if (IS_ENABLED(CONFIG_UI_LED_USE_PWM)) {
		/* Reset on-time if transition from off to on */
		if (state[0] == false && new_state == true) {
			reset_on_time(obj_inst_id);
		}

		state[obj_inst_id] = new_state;
		ret = ui_led_pwm_on_off(obj_inst_id, new_state);
		if (ret) {
			LOG_ERR("Set PWM LED %u on/off failed (%d)", obj_inst_id, ret);
			return ret;
		}
	} else if (IS_ENABLED(CONFIG_UI_LED_USE_GPIO)) {
		/* Reset on-time if transition from off to on */
		if (state[0] == false && new_state == true) {
			reset_on_time(obj_inst_id);
		}

		state[obj_inst_id] = new_state;
		ret = ui_led_gpio_on_off(obj_inst_id, new_state);
		if (ret) {
			LOG_ERR("Set GPIO LED %u on/off failed (%d)", obj_inst_id, ret);
			return ret;
		}
	}

	return ret;
}

static int lc_dimmer_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			uint16_t data_len, bool last_block, size_t total_size, size_t offset)
{
	int ret;
	uint8_t new_brightness = *data;
	uint8_t intensity;

	if (IS_ENABLED(CONFIG_UI_LED_USE_PWM)) {
		brightness_val[obj_inst_id] = new_brightness;
		intensity = calculate_intensity(UINT8_MAX, brightness_val[obj_inst_id]);
		ret = ui_led_pwm_set_intensity(obj_inst_id, intensity);
		if (ret) {
			LOG_ERR("Set PWM LED %u intensity failed (%d)", obj_inst_id, ret);
			return ret;
		}
	}

	return 0;
}

static int lwm2m_init_light_control(void)
{
	int ret = 0;
	uint8_t intensity;
	char colour_str[RGBIR_STR_LENGTH];

	if (IS_ENABLED(CONFIG_UI_LED_USE_PWM)) {
		ui_led_pwm_init();
		for (int i = 0; i < NUM_LEDS; ++i) {
			colour_val[i] = UINT8_MAX;
			brightness_val[i] = BRIGHTNESS_MAX;
			intensity = calculate_intensity(colour_val[i], brightness_val[i]);
			ui_led_pwm_set_intensity(i, intensity);
		}
		snprintk(colour_str, RGBIR_STR_LENGTH, "0xFFFFFF");
	} else if (IS_ENABLED(CONFIG_UI_LED_USE_GPIO)) {
		ui_led_gpio_init();
		snprintk(colour_str, RGBIR_STR_LENGTH, "0x010101");
	}

	if (IS_ENABLED(CONFIG_BOARD_THINGY91_NRF9160_NS)) {
		/* Create RGB light control object */
		lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_LIGHT_CONTROL_ID, 0));
		lwm2m_register_post_write_callback(
			&LWM2M_OBJ(IPSO_OBJECT_LIGHT_CONTROL_ID, 0, ON_OFF_RID), rgb_lc_on_off_cb);
		lwm2m_register_post_write_callback(
			&LWM2M_OBJ(IPSO_OBJECT_LIGHT_CONTROL_ID, 0, COLOUR_RID), rgb_lc_colour_cb);
		lwm2m_register_post_write_callback(
			&LWM2M_OBJ(IPSO_OBJECT_LIGHT_CONTROL_ID, 0, DIMMER_RID), rgb_lc_dimmer_cb);
		lwm2m_set_res_buf(
			&LWM2M_OBJ(IPSO_OBJECT_LIGHT_CONTROL_ID, 0, APPLICATION_TYPE_RID),
			APP_TYPE, sizeof(APP_TYPE), sizeof(APP_TYPE), LWM2M_RES_DATA_FLAG_RO);
		lwm2m_set_string(&LWM2M_OBJ(IPSO_OBJECT_LIGHT_CONTROL_ID, 0, COLOUR_RID),
			colour_str);
		lwm2m_set_u8(&LWM2M_OBJ(IPSO_OBJECT_LIGHT_CONTROL_ID, 0, DIMMER_RID),
			     BRIGHTNESS_MAX);
	} else if (IS_ENABLED(CONFIG_BOARD_NRF9160DK_NRF9160_NS) ||
		   IS_ENABLED(CONFIG_BOARD_NRF9161DK_NRF9161_NS)) {
		struct lwm2m_obj_path lwm2m_path = LWM2M_OBJ(IPSO_OBJECT_LIGHT_CONTROL_ID);

		for (int i = 0; i < NUM_LEDS; ++i) {
			lwm2m_path.obj_inst_id = i;
			lwm2m_path.level = 2;
			/* Create light control object */
			lwm2m_create_object_inst(&lwm2m_path);

			lwm2m_path.res_id = ON_OFF_RID;
			lwm2m_path.level = 3;
			lwm2m_register_post_write_callback(&lwm2m_path, lc_on_off_cb);

			lwm2m_path.res_id = DIMMER_RID;
			lwm2m_register_post_write_callback(&lwm2m_path, lc_dimmer_cb);
			lwm2m_set_u8(&lwm2m_path, BRIGHTNESS_MAX);

			lwm2m_path.res_id = APPLICATION_TYPE_RID;
			lwm2m_set_res_buf(&lwm2m_path, APP_TYPE, sizeof(APP_TYPE),
					  sizeof(APP_TYPE), LWM2M_RES_DATA_FLAG_RO);
		}
	}

	return ret;
}

LWM2M_APP_INIT(lwm2m_init_light_control);
