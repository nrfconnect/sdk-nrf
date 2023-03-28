/*
 * Copyright 2020-2023 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include <anjay_zephyr/config.h>
#include <anjay_zephyr/lwm2m.h>
#include <anjay_zephyr/objects.h>

#include "sensors_config.h"
#include "peripherals.h"
#include "status_led.h"

LOG_MODULE_REGISTER(main_app);
static const anjay_dm_object_def_t **location_obj;
#if BUZZER_AVAILABLE
static const anjay_dm_object_def_t **buzzer_obj;
#endif // BUZZER_AVAILABLE
#if LED_COLOR_LIGHT_AVAILABLE
static const anjay_dm_object_def_t **led_color_light_obj;
#endif // LED_COLOR_LIGHT_AVAILABLE
#if SWITCH_AVAILABLE_ANY
static const anjay_dm_object_def_t **switch_obj;
#endif // SWITCH_AVAILABLE_ANY
static avs_sched_handle_t update_objects_handle;

#if PUSH_BUTTON_AVAILABLE_ANY
static struct anjay_zephyr_ipso_button_instance buttons[] = {
#if PUSH_BUTTON_AVAILABLE(0)
	PUSH_BUTTON_GLUE_ITEM(0),
#endif // PUSH_BUTTON_AVAILABLE(0)
#if PUSH_BUTTON_AVAILABLE(1)
	PUSH_BUTTON_GLUE_ITEM(1),
#endif // PUSH_BUTTON_AVAILABLE(1)
#if PUSH_BUTTON_AVAILABLE(2)
	PUSH_BUTTON_GLUE_ITEM(2),
#endif // PUSH_BUTTON_AVAILABLE(2)
#if PUSH_BUTTON_AVAILABLE(3)
	PUSH_BUTTON_GLUE_ITEM(3),
#endif // PUSH_BUTTON_AVAILABLE(3)
};
#endif // PUSH_BUTTON_AVAILABLE_ANY

#if SWITCH_AVAILABLE_ANY
static struct anjay_zephyr_switch_instance switches[] = {
#if SWITCH_AVAILABLE(0)
	SWITCH_BUTTON_GLUE_ITEM(0),
#endif // SWITCH_AVAILABLE(0)
#if SWITCH_AVAILABLE(1)
	SWITCH_BUTTON_GLUE_ITEM(1),
#endif // SWITCH_AVAILABLE(1)
#if SWITCH_AVAILABLE(2)
	SWITCH_BUTTON_GLUE_ITEM(2),
#endif // SWITCH_AVAILABLE(2)
};
#endif // SWITCH_AVAILABLE_ANY
struct anjay_zephyr_network_preferred_bearer_list_t anjay_zephyr_config_get_preferred_bearers(void);

static int register_objects(anjay_t *anjay)
{
	location_obj = anjay_zephyr_location_object_create();
	if (location_obj) {
		anjay_register_object(anjay, location_obj);
	}

	sensors_install(anjay);
#if PUSH_BUTTON_AVAILABLE_ANY
	anjay_zephyr_ipso_push_button_object_install(anjay, buttons, AVS_ARRAY_SIZE(buttons));
#endif // PUSH_BUTTON_AVAILABLE_ANY

#if BUZZER_AVAILABLE
	buzzer_obj = anjay_zephyr_buzzer_object_create(BUZZER_NODE);
	if (buzzer_obj) {
		anjay_register_object(anjay, buzzer_obj);
	}
#endif // BUZZER_AVAILABLE

#if LED_COLOR_LIGHT_AVAILABLE
	led_color_light_obj = anjay_zephyr_led_color_light_object_create(DEVICE_DT_GET(RGB_NODE));
	if (led_color_light_obj) {
		anjay_register_object(anjay, led_color_light_obj);
	}
#endif // LED_COLOR_LIGHT_AVAILABLE

#if SWITCH_AVAILABLE_ANY
	switch_obj = anjay_zephyr_switch_object_create(switches, AVS_ARRAY_SIZE(switches));
	if (switch_obj) {
		anjay_register_object(anjay, switch_obj);
	}
#endif // SWITCH_AVAILABLE_ANY
	return 0;
}

static void update_objects_frequent(anjay_t *anjay)
{
#if SWITCH_AVAILABLE_ANY
	anjay_zephyr_switch_object_update(anjay, switch_obj);
#endif // SWITCH_AVAILABLE_ANY
#if BUZZER_AVAILABLE
	anjay_zephyr_buzzer_object_update(anjay, buzzer_obj);
#endif // BUZZER_AVAILABLE
}

static void update_objects_periodic(anjay_t *anjay)
{
	anjay_zephyr_ipso_sensors_update(anjay);
	anjay_zephyr_location_object_update(anjay, location_obj);
}

static void update_objects(avs_sched_t *sched, const void *anjay_ptr)
{
	anjay_t *anjay = *(anjay_t *const *)anjay_ptr;

	static size_t cycle;

	update_objects_frequent(anjay);
	if (cycle % 5 == 0) {
		update_objects_periodic(anjay);
	}

	status_led_toggle();

	cycle++;
	AVS_SCHED_DELAYED(sched, &update_objects_handle,
			  avs_time_duration_from_scalar(1, AVS_TIME_S), update_objects, &anjay,
			  sizeof(anjay));
}

static int init_update_objects(anjay_t *anjay)
{
	avs_sched_t *sched = anjay_get_scheduler(anjay);

	update_objects(sched, &anjay);

	status_led_init();

	return 0;
}

static int clean_before_anjay_destroy(anjay_t *anjay)
{
	avs_sched_del(&update_objects_handle);

	return 0;
}

static int release_objects(void)
{
	anjay_zephyr_location_object_release(&location_obj);
#if SWITCH_AVAILABLE_ANY
	anjay_zephyr_switch_object_release(&switch_obj);
#endif // SWITCH_AVAILABLE_ANY
#if LED_COLOR_LIGHT_AVAILABLE
	anjay_zephyr_led_color_light_object_release(&led_color_light_obj);
#endif // LED_COLOR_LIGHT_AVAILABLE
#if BUZZER_AVAILABLE
	anjay_zephyr_buzzer_object_release(&buzzer_obj);
#endif // BUZZER_AVAILABLE
	return 0;
}

int lwm2m_callback(anjay_t *anjay, enum anjay_zephyr_lwm2m_callback_reasons reason)
{
	switch (reason) {
	case ANJAY_ZEPHYR_LWM2M_CALLBACK_REASON_INIT:
		return register_objects(anjay);
	case ANJAY_ZEPHYR_LWM2M_CALLBACK_REASON_ANJAY_READY:
		return init_update_objects(anjay);
	case ANJAY_ZEPHYR_LWM2M_CALLBACK_REASON_ANJAY_SHUTTING_DOWN:
		return clean_before_anjay_destroy(anjay);
	case ANJAY_ZEPHYR_LWM2M_CALLBACK_REASON_CLEANUP:
		return release_objects();
	default:
		return -1;
	}
}

void main(void)
{
	LOG_INF("Initializing Anjay-zephyr-client demo " CONFIG_ANJAY_ZEPHYR_VERSION);

	anjay_zephyr_lwm2m_set_user_callback(lwm2m_callback);

	anjay_zephyr_lwm2m_init_from_settings();
	anjay_zephyr_lwm2m_start();

	while (1) {
		k_sleep(K_SECONDS(1));
	}
}
