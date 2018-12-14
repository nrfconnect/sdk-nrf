/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ui_out.h>
#include <gpio.h>
#include <soc.h>
#include <misc/__assert.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(ui_out_lib, CONFIG_UI_OUT_LIB_LOG_LEVEL);

static atomic_t normal_state = UI_OUT_STATE_IDLE;
static u32_t current_normal_state = UI_OUT_STATE_IDLE;
static atomic_t alert_state = UI_OUT_STATE_ALERT_OFF;
static u32_t current_alert_state = UI_OUT_STATE_ALERT_OFF;

static struct k_delayed_work normal_state_work;
static struct k_delayed_work alert_state_work;
static struct k_work event_work;

static bool alert_on;

extern const struct ui_out_user_state __start_ui_out_user[];
extern const struct ui_out_user_state __stop_ui_out_user[];


static bool led_state_check(u32_t led_mask)
{
	int err;
	u32_t led_state = 0;

	err = dk_check_leds_state(led_mask, &led_state);
	if (err) {
		LOG_WRN("Cannot check leds state");
		return false;
	}

	return led_mask == led_state;
}

static int leds_off(void)
{
	int err;

	if (alert_on) {
		err = dk_set_leds_state(UI_OUT_LED_ALERT, DK_ALL_LEDS_MSK);
	} else {
		err = dk_set_leds_state(DK_NO_LEDS_MSK, DK_ALL_LEDS_MSK);
	}

	return err;
}

static int led_blinky(struct k_delayed_work *work,
		      u32_t leds_mask, u32_t leds_on_interval,
		      u32_t led_off_interval)
{
	int err;
	u32_t delay;

	if (led_state_check(leds_mask)) {
		err = dk_set_leds_state(DK_NO_LEDS_MSK, leds_mask);
		delay = led_off_interval;
	} else {
		err = dk_set_leds_state(leds_mask, DK_NO_LEDS_MSK);
		delay = leds_on_interval;
	}

	k_delayed_work_submit(work, delay);

	return err;
}

static int user_state_led_process(const struct ui_out_user_state *state)
{
	int err;

	if (state->led) {
		if (state->led->blinky) {
			err = led_blinky(&normal_state_work,
					 state->led->led_mask,
					 state->led->on_interval,
					 state->led->off_interval);
		} else {
			err = dk_set_leds(state->led->led_mask);
		}
	}

	return err;
}

static void user_state_log_process(const struct ui_out_user_state *state)
{
	if (state->message) {
		LOG_INF("%s", state->message);
	}

}

static int user_state_indicate(u32_t id, bool log_no_led)
{
	for (const struct ui_out_user_state *state = __start_ui_out_user;
	     (state != NULL) && (state != __stop_ui_out_user);
	     state++) {
		if (id == state->id) {
			if (log_no_led) {
				user_state_log_process(state);
				return 0;
			} else {
				return user_state_led_process(state);
			}
		}
	}

	return -EINVAL;
}

static int user_state_set(u32_t state_id, bool log_no_led)
{
	if (IS_ENABLED(CONFIG_UI_OUT_USER_STATE)) {
		int err = user_state_indicate(state_id, log_no_led);
		if (err) {
			LOG_ERR("User event indication error");
			return err;
		}
	}

	LOG_WRN("User event not enabled");
	return -EPERM;
}

static int normal_led_indication(u32_t state)
{
	int err = 0;

	switch (state) {
	case UI_OUT_STATE_IDLE:
		err = leds_off();
		break;

	case UI_OUT_STATE_BLE_ADVERTISING:
		led_blinky(&normal_state_work, UI_OUT_LED_ADV,
			   CONFIG_UI_OUT_ADV_LED_ON_INTERVAL,
			   CONFIG_UI_OUT_ADV_LED_OFF_INTERVAL);
		break;

	case UI_OUT_STATE_BLE_SCANNING:
		led_blinky(&normal_state_work, UI_OUT_LED_SCANNING,
			   CONFIG_UI_OUT_SCAN_LED_ON_INTERVAL,
			   CONFIG_UI_OUT_SCAN_LED_OFF_INTERVAL);
		break;

	case UI_OUT_STATE_BLE_ADVERTISING_AND_SCANNING:
		led_blinky(&normal_state_work,
			   UI_OUT_LED_ADV | UI_OUT_LED_SCANNING,
			   CONFIG_UI_OUT_ADV_SCAN_LEDS_ON_INTERVAL,
			   CONFIG_UI_OUT_ADV_SCAN_LEDS_OFF_INTERVAL);
		break;

	case UI_OUT_STATE_BLE_BONDING:
		led_blinky(&normal_state_work, UI_OUT_LED_BONDING,
			   CONFIG_UI_OUT_BONDING_LED_INTERVAL,
			   CONFIG_UI_OUT_BONDING_LED_INTERVAL);
		break;

	case UI_OUT_STATE_BLE_CONNECTED:
		err = dk_set_leds_state(UI_OUT_LED_CONNECTED, DK_ALL_LEDS_MSK);
		break;

	case UI_OUT_STATE_FATAL_ERROR:
		err = dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);
		break;

	default:
		break;
	}

	return err;
}

static int alert_led_indication(u32_t state)
{
	int err = 0;

	switch (state) {
	case UI_OUT_STATE_ALERT_OFF:
		err = dk_set_leds_state(DK_NO_LEDS_MSK, UI_OUT_LED_ALERT);
		alert_on = false;
		break;

	case UI_OUT_STATE_ALERT_0:
	case UI_OUT_STATE_ALERT_1:
	case UI_OUT_STATE_ALERT_2:
	case UI_OUT_STATE_ALERT_3:
		__ASSERT_NO_MSG(state > UI_OUT_STATE_ALERT_OFF);
		u32_t delay = (UI_OUT_STATE_ALERT_OFF - state) * CONFIG_UI_OUT_ALERT_INTERVAL;

		err = led_blinky(&alert_state_work, UI_OUT_LED_ALERT,
				 delay, delay);
		alert_on = true;
		break;

	default:
		break;
	}

	return err;
}

static void log_indication(u32_t state)
{
	if (state >= UI_OUT_STATE_USER_STATE_START) {
		int err = user_state_set(state, true);

		if (err) {
			LOG_WRN("Log indication error");
		}

		return;
	}

	switch (state) {

	case UI_OUT_STATE_BLE_ADVERTISING:
		LOG_INF("BLE Advertising started.");
		break;

	case UI_OUT_STATE_BLE_BONDING:
		LOG_INF("BLE Bonding.");
		break;

	case UI_OUT_STATE_BLE_CONNECTED:
		LOG_INF("BLE Connected.");
		break;

	case UI_OUT_STATE_BLE_SCANNING:
		LOG_INF("BLE Scanning started.");
		break;

	case UI_OUT_STATE_BLE_ADVERTISING_AND_SCANNING:
		LOG_INF("BLE Advertising and scanning started.");
		break;

	case UI_OUT_STATE_ALERT_0:
		LOG_INF("Alert 0.");
		break;

	case UI_OUT_STATE_ALERT_1:
		LOG_INF("Alert 1.");
		break;

	case UI_OUT_STATE_ALERT_2:
		LOG_INF("Alert 2.");
		break;

	case UI_OUT_STATE_ALERT_3:
		LOG_INF("Alert 3.");
		break;

	case UI_OUT_STATE_ALERT_OFF:
		LOG_INF("Alert OFF.");
		break;

	case UI_OUT_STATE_FATAL_ERROR:
		LOG_ERR("Fatal error.");
		break;

	default:
		break;

	}
}

static void normal_work_handler(struct k_work *work)
{
	int err = 0;

	if (current_normal_state >= UI_OUT_STATE_USER_STATE_START) {
		err = user_state_set(current_normal_state, false);
	} else {
		err = normal_led_indication(current_normal_state);
	}


	if (err) {
		LOG_ERR("LED indication error.");
	}
}

static void alert_work_handler(struct k_work *work)
{
	int err = alert_led_indication(current_alert_state);

	if (err) {
		LOG_ERR("Alert indication failed");
	}
}


static void set_state_work(struct k_work *work)
{
	u32_t l_alert_state = atomic_get(&alert_state);
	u32_t l_normal_state = atomic_get(&normal_state);

	if (l_normal_state != current_normal_state) {
		current_normal_state = l_normal_state;

		if (IS_ENABLED(CONFIG_UI_OUT_LEDS)) {
			k_delayed_work_submit(&normal_state_work, K_NO_WAIT);
		}

		if (IS_ENABLED(CONFIG_UI_OUT_LOG)) {
			log_indication(current_normal_state);
		}
	}

	if (l_alert_state != current_alert_state) {
		current_alert_state = l_alert_state;

		if (IS_ENABLED(CONFIG_UI_OUT_LEDS)) {
			k_delayed_work_submit(&alert_state_work, K_NO_WAIT);
		}

		if (IS_ENABLED(CONFIG_UI_OUT_LOG)) {
			log_indication(current_alert_state);
		}
	}
}

int ui_out_init(void)
{
	int err = 0;

	static_assert(IS_ENABLED(CONFIG_UI_OUT_LEDS) || IS_ENABLED(CONFIG_UI_OUT_LOG),
		      "Output not given.");

	k_work_init(&event_work, set_state_work);

	if (IS_ENABLED(CONFIG_UI_OUT_LEDS)) {
		k_delayed_work_init(&normal_state_work, normal_work_handler);
		k_delayed_work_init(&alert_state_work, alert_work_handler);

		err = dk_leds_init();
		if (!err) {
			return leds_off();
		}
	}

	return err;
}

void ui_out_state_indicate(u32_t state_id)
{
	static_assert(UI_OUT_STATE_ALERT_0 < UI_OUT_STATE_ALERT_OFF, "");

	if ((state_id > UI_OUT_STATE_ALERT_0) &&
	    (state_id < UI_OUT_STATE_ALERT_OFF)) {
		alert_state = state_id;
	} else {
		normal_state = state_id;
	}

	k_work_submit(&event_work);
}

int ui_out_user_get_state_id(const char *name, u32_t *id)
{
	__ASSERT_NO_MSG(name != NULL);

	if (!IS_ENABLED(CONFIG_UI_OUT_USER_STATE)) {
		return -EACCES;
	}

	for (const struct ui_out_user_state *state = __start_ui_out_user;
	     (state != NULL) && (state != __stop_ui_out_user);
	     state++) {
		if (!strcmp(state->name, name)) {
			*id = state->id;

			return 0;
		}
	}

	return -EINVAL;
}
