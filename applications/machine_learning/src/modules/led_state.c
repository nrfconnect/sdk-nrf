/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include "led_state_def.h"

#include <caf/events/led_event.h>
#include "ml_app_mode_event.h"
#include "ml_result_event.h"
#include "ei_data_forwarder_event.h"
#include "sensor_sim_event.h"

#define MODULE led_state
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_LED_STATE_LOG_LEVEL);

#define DISPLAY_ML_RESULTS		IS_ENABLED(CONFIG_ML_APP_ML_RESULT_EVENTS)
#define DISPLAY_SIM_SIGNAL		IS_ENABLED(CONFIG_ML_APP_SENSOR_SIM_EVENTS)
#define DISPLAY_DATA_FORWARDER		IS_ENABLED(CONFIG_ML_APP_EI_DATA_FORWARDER_EVENTS)

#define ANOMALY_THRESH			(CONFIG_ML_APP_LED_STATE_ANOMALY_THRESH / 1000.0f)
#define VALUE_THRESH			(CONFIG_ML_APP_LED_STATE_VALUE_THRESH / 1000.0f)
#define PREDICTION_STREAK_THRESH	CONFIG_ML_APP_LED_STATE_PREDICTION_STREAK_THRESH

BUILD_ASSERT(PREDICTION_STREAK_THRESH > 0);

#define DEFAULT_EFFECT			(&ml_result_led_effects[0])

static enum ml_app_mode ml_app_mode = ML_APP_MODE_COUNT;
static enum ei_data_forwarder_state forwarder_state = DISPLAY_DATA_FORWARDER ?
	EI_DATA_FORWARDER_STATE_DISCONNECTED : EI_DATA_FORWARDER_STATE_TRANSMITTING;

static const struct led_effect *blocking_led_effect;

static const char *cur_label;
static size_t prediction_streak;


static bool is_led_effect_valid(const struct led_effect *le)
{
	const uint8_t zeros[sizeof(struct led_effect)] = {0};

	return memcmp(le, zeros, sizeof(struct led_effect));
}

static bool is_led_effect_blocking(const struct led_effect *le)
{
	return ((!le->loop_forever) && (le->step_count > 1));
}

static void clear_prediction(void)
{
	cur_label = NULL;
	prediction_streak = 0;
}

static void send_led_event(size_t led_id, const struct led_effect *led_effect)
{
	__ASSERT_NO_MSG(led_effect);
	__ASSERT_NO_MSG(led_id < LED_ID_COUNT);

	struct led_event *event = new_led_event();

	event->led_id = led_id;
	event->led_effect = led_effect;
	APP_EVENT_SUBMIT(event);
}

static const struct ml_result_led_effect *get_led_effect(const char *label)
{
	const struct ml_result_led_effect *result = DEFAULT_EFFECT;

	if (!label) {
		return result;
	}

	for (size_t i = 1; i < ARRAY_SIZE(ml_result_led_effects); i++) {
		const struct ml_result_led_effect *t = &ml_result_led_effects[i];

		if ((t->label == label) || !strcmp(t->label, label)) {
			result = t;
			break;
		}
	}

	return result;
}

static void display_sensor_sim(const char *label)
{
	static const struct ml_result_led_effect *sensor_sim_effect;

	if (label) {
		sensor_sim_effect = get_led_effect(label);

		if (sensor_sim_effect == DEFAULT_EFFECT) {
			LOG_WRN("No LED effect for sensor_sim label %s", label);
		}
	}

	if (sensor_sim_effect) {
		__ASSERT_NO_MSG(!is_led_effect_blocking(&sensor_sim_effect->effect));
		send_led_event(led_map[LED_ID_SENSOR_SIM], &sensor_sim_effect->effect);
	}
}

static void display_ml_result(const char *label, bool force_update)
{
	__ASSERT_NO_MSG(ml_app_mode == ML_APP_MODE_MODEL_RUNNING);

	static const struct ml_result_led_effect *ml_result_effect;
	const struct ml_result_led_effect *new_effect = get_led_effect(label);

	/* Update not needed. */
	if ((ml_result_effect == new_effect) && !force_update) {
		return;
	}

	__ASSERT_NO_MSG(!force_update || !label);

	if (!force_update) {
		if (!label) {
			LOG_INF("Anomaly detected");
		} else if (new_effect == DEFAULT_EFFECT) {
			LOG_INF("No LED effect for label: %s", label);
		} else {
			LOG_INF("Displaying LED effect for label: %s", label);
		}
	}

	/* Synchronize LED effect displayed for simulated signal. */
	if (DISPLAY_SIM_SIGNAL && (new_effect != DEFAULT_EFFECT)) {
		display_sensor_sim(NULL);
	}

	ml_result_effect = new_effect;
	send_led_event(led_map[LED_ID_ML_STATE], &ml_result_effect->effect);

	if (is_led_effect_blocking(&ml_result_effect->effect)) {
		blocking_led_effect = &ml_result_effect->effect;
	} else {
		blocking_led_effect = NULL;
	}
}

static void update_ml_result(const char *label, float value, float anomaly)
{
	const char *new_label = cur_label;
	bool increment = true;

	if (anomaly > ANOMALY_THRESH) {
		new_label = ANOMALY_LABEL;
	} else if (value >= VALUE_THRESH) {
		new_label = label;
	} else {
		increment = false;
	}

	if (new_label != cur_label) {
		if ((!new_label || !cur_label) || strcmp(new_label, cur_label)) {
			cur_label = new_label;
			prediction_streak = 0;
		}
	}

	if (increment) {
		prediction_streak++;
	}

	if (prediction_streak >= PREDICTION_STREAK_THRESH) {
		display_ml_result(cur_label, false);
		clear_prediction();
	}
}

static void validate_configuration(void)
{
	BUILD_ASSERT(ARRAY_SIZE(ml_result_led_effects) >= 1);
	__ASSERT_NO_MSG(!is_led_effect_blocking(&DEFAULT_EFFECT->effect));
	__ASSERT_NO_MSG(!DEFAULT_EFFECT->label);

	size_t anomaly_label_cnt = 0;

	for (size_t i = 1; i < ARRAY_SIZE(ml_result_led_effects); i++) {
		const struct ml_result_led_effect *t = &ml_result_led_effects[i];

		__ASSERT_NO_MSG(is_led_effect_valid(&t->effect));
		__ASSERT_NO_MSG(t->label);

		if (!strcmp(t->label, ANOMALY_LABEL)) {
			anomaly_label_cnt++;
		}
	}

	__ASSERT_NO_MSG(anomaly_label_cnt <= 1);
}

static bool handle_ml_result_event(const struct ml_result_event *event)
{
	if ((ml_app_mode == ML_APP_MODE_MODEL_RUNNING) && !blocking_led_effect) {
		update_ml_result(event->label, event->value, event->anomaly);
	}

	return false;
}

static bool handle_sensor_sim_event(const struct sensor_sim_event *event)
{
	display_sensor_sim(event->label);

	return false;
}

static bool handle_ei_data_forwarder_event(const struct ei_data_forwarder_event *event)
{
	__ASSERT_NO_MSG(event->state != EI_DATA_FORWARDER_STATE_DISABLED);
	forwarder_state = event->state;

	__ASSERT_NO_MSG(is_led_effect_valid(&ei_data_forwarder_led_effects[forwarder_state]));

	if (ml_app_mode == ML_APP_MODE_DATA_FORWARDING) {
		send_led_event(led_map[LED_ID_ML_STATE],
			       &ei_data_forwarder_led_effects[forwarder_state]);
	}

	return false;
}

static bool handle_led_ready_event(const struct led_ready_event *event)
{
	if ((event->led_id == led_map[LED_ID_ML_STATE]) &&
	    (ml_app_mode == ML_APP_MODE_MODEL_RUNNING) &&
	    (blocking_led_effect == event->led_effect)) {
		display_ml_result(NULL, true);
	}

	return false;
}

static bool handle_ml_app_mode_event(const struct ml_app_mode_event *event)
{
	ml_app_mode = event->mode;

	if (event->mode == ML_APP_MODE_MODEL_RUNNING) {
		clear_prediction();
		display_ml_result(NULL, true);
	} else if (event->mode == ML_APP_MODE_DATA_FORWARDING) {
		send_led_event(led_map[LED_ID_ML_STATE],
			       &ei_data_forwarder_led_effects[forwarder_state]);
	} else {
		/* Not supported. */
		__ASSERT_NO_MSG(false);
	}

	return false;
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		if (IS_ENABLED(CONFIG_ASSERT)) {
			validate_configuration();
		} else {
			ARG_UNUSED(is_led_effect_valid);
		}

		static bool initialized;

		__ASSERT_NO_MSG(!initialized);
		module_set_state(MODULE_STATE_READY);
		initialized = true;
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (DISPLAY_ML_RESULTS &&
	    is_ml_result_event(aeh)) {
		return handle_ml_result_event(cast_ml_result_event(aeh));
	}

	if (DISPLAY_SIM_SIGNAL &&
	    is_sensor_sim_event(aeh)) {
		return handle_sensor_sim_event(cast_sensor_sim_event(aeh));
	}

	if (DISPLAY_DATA_FORWARDER &&
	    is_ei_data_forwarder_event(aeh)) {
		return handle_ei_data_forwarder_event(cast_ei_data_forwarder_event(aeh));
	}

	if (is_led_ready_event(aeh)) {
		return handle_led_ready_event(cast_led_ready_event(aeh));
	}

	if (is_ml_app_mode_event(aeh)) {
		return handle_ml_app_mode_event(cast_ml_app_mode_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, ml_app_mode_event);
APP_EVENT_SUBSCRIBE(MODULE, led_ready_event);
#if DISPLAY_DATA_FORWARDER
APP_EVENT_SUBSCRIBE(MODULE, ei_data_forwarder_event);
#endif /* DISPLAY_DATA_FORWARDER */
#if DISPLAY_ML_RESULTS
APP_EVENT_SUBSCRIBE(MODULE, ml_result_event);
#endif /* DISPLAY_ML_RESULTS */
#if DISPLAY_SIM_SIGNAL
APP_EVENT_SUBSCRIBE(MODULE, sensor_sim_event);
#endif /* DISPLAY_SIM_SIGNAL */
