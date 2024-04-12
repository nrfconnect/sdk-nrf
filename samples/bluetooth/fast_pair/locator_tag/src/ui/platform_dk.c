/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <dk_buttons_and_leds.h>

#include "app_ui.h"
#include "app_ui_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_DBG);

#define NO_LED					0xFF

/* Minimum button hold time in milliseconds to trigger the FMDN recovery mode. */
#define RECOVERY_MODE_BTN_MIN_HOLD_TIME_MS	3000

/* Run status LED blinking interval. */
#define RUN_LED_BLINK_INTERVAL_MS		1000

/* Mode LED blinking interval in recovery mode. */
#define MODE_LED_RECOVERY_BLINK_INTERVAL_MS	1000

/* Mode LED blinking interval in identification mode. */
#define MODE_LED_ID_BLINK_INTERVAL_MS		500

/* Mode LED thread backoff timeout. */
#define MODE_LED_THREAD_BACKOFF_TIMEOUT_MS	500

/* Assignments of the DK LEDs and buttons to different functionalities and modules. */

/* Run status LED. */
#define APP_RUN_STATUS_LED			DK_LED1

/* Ringing status LED. */
#define APP_RING_LED				DK_LED2

/* Provisioning status LED. */
#define APP_PROVISIONED_LED			DK_LED3

/* Recovery mode and identification mode status LED. */
#define APP_MODE_LED				DK_LED4

/* Button used to change the Fast Pair advertising mode. */
#define APP_FP_ADV_MODE_BTN			DK_BTN1_MSK

/* Button used to stop the ringing action. */
#define APP_RING_BTN				DK_BTN2_MSK

/* Button used to change the battery level. */
#define APP_BATTERY_BTN				DK_BTN3_MSK

/* Button used to enter the following modes:
 * - recovery mode
 * - identification mode
 */
#define APP_MODE_CTLR_BTN			DK_BTN4_MSK

/* Button used perform a factory reset at the bootup (by polling the state). */
#define APP_FACTORY_SETTINGS_RESET_BTN		DK_BTN4_MSK

#define RUN_LED_THREAD_PRIORITY			K_PRIO_PREEMPT(0)
#define RUN_LED_THREAD_STACK_SIZE		512

#define MODE_LED_THREAD_PRIORITY		K_PRIO_PREEMPT(0)
#define MODE_LED_THREAD_STACK_SIZE		512

enum app_state_mode {
	APP_STATE_MODE_NONE,
	APP_STATE_MODE_ID,
	APP_STATE_MODE_RECOVERY,
	APP_STATE_MODE_ALL,
};

static atomic_t app_state_mode = ATOMIC_INIT(APP_STATE_MODE_NONE);
static atomic_t run_led_thread_state_app_running;

static void btn_handle(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & APP_MODE_CTLR_BTN) {
		static int64_t prev_uptime;

		if (button_state & APP_MODE_CTLR_BTN) {
			/* On button press */
			prev_uptime = k_uptime_get();
		} else {
			/* On button release */
			int64_t hold_time;

			/* Mode control button is shared with the factory reset action and also
			 * handled in the bootup_btn_handle function.
			 * Holding the button while DK restarts will not emit a button press event.
			 * Button release not preceded by the button press should be ignored.
			 */
			if (prev_uptime != 0) {
				hold_time = (k_uptime_get() - prev_uptime);
				if (hold_time > RECOVERY_MODE_BTN_MIN_HOLD_TIME_MS) {
					app_ui_request_broadcast(
						APP_UI_REQUEST_RECOVERY_MODE_ENTER);
				} else {
					app_ui_request_broadcast(APP_UI_REQUEST_ID_MODE_ENTER);
				}
			}
		}
	}

	if (has_changed & button_state & APP_FP_ADV_MODE_BTN) {
		app_ui_request_broadcast(APP_UI_REQUEST_FP_ADV_MODE_CHANGE);
	}

	if (has_changed & button_state & APP_RING_BTN) {
		app_ui_request_broadcast(APP_UI_REQUEST_RINGING_STOP);
	}

	if (has_changed & button_state & APP_BATTERY_BTN) {
		app_ui_request_broadcast(APP_UI_REQUEST_SIMULATED_BATTERY_CHANGE);
	}
}

static void bootup_btn_handle(void)
{
	uint32_t button_state;
	uint32_t has_changed;

	dk_read_buttons(&button_state, &has_changed);

	if (button_state & APP_FACTORY_SETTINGS_RESET_BTN) {
		app_ui_request_broadcast(APP_UI_REQUEST_FACTORY_RESET);
	}
}

static void run_led_thread_process(void)
{
	static bool run_led_on = true;

	while (1) {
		if (atomic_get(&run_led_thread_state_app_running)) {
			dk_set_led(APP_RUN_STATUS_LED, run_led_on);
			run_led_on = !run_led_on;
		}

		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL_MS));
	}
}

K_THREAD_DEFINE(run_led_thread_id, RUN_LED_THREAD_STACK_SIZE, run_led_thread_process,
		NULL, NULL, NULL, RUN_LED_THREAD_PRIORITY, 0, 0);

static void mode_led_thread_process(void)
{
	static bool mode_led_on;

	while (1) {
		uint32_t sleep_time_ms = MODE_LED_THREAD_BACKOFF_TIMEOUT_MS;
		enum app_state_mode mode = atomic_get(&app_state_mode);

		switch (mode) {
		case APP_STATE_MODE_NONE:
			if (mode_led_on) {
				mode_led_on = false;
				dk_set_led(APP_MODE_LED, mode_led_on);
			}
			break;

		case APP_STATE_MODE_RECOVERY:
			mode_led_on = !mode_led_on;
			dk_set_led(APP_MODE_LED, mode_led_on);
			sleep_time_ms = MODE_LED_RECOVERY_BLINK_INTERVAL_MS;
			break;

		case APP_STATE_MODE_ID:
			mode_led_on = !mode_led_on;
			dk_set_led(APP_MODE_LED, mode_led_on);
			sleep_time_ms = MODE_LED_ID_BLINK_INTERVAL_MS;
			break;

		case APP_STATE_MODE_ALL:
			if (!mode_led_on) {
				mode_led_on = true;
				dk_set_led(APP_MODE_LED, mode_led_on);
			}
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		k_sleep(K_MSEC(sleep_time_ms));
	}
}

K_THREAD_DEFINE(mode_led_thread_id, MODE_LED_THREAD_STACK_SIZE, mode_led_thread_process,
		NULL, NULL, NULL, MODE_LED_THREAD_PRIORITY, 0, 0);

static void state_app_running_indicate(bool active)
{
	atomic_set(&run_led_thread_state_app_running, active);

	if (!active) {
		dk_set_led(APP_RUN_STATUS_LED, false);
	}
}

static void state_mode_indicate(enum app_ui_state state, bool active)
{
	static bool recovery_mode;
	static bool id_mode;
	enum app_state_mode mode;

	switch (state) {
	case APP_UI_STATE_RECOVERY_MODE:
		recovery_mode = active;
		break;
	case APP_UI_STATE_ID_MODE:
		id_mode = active;
		break;
	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	if (!recovery_mode && !id_mode) {
		mode = APP_STATE_MODE_NONE;
	} else if (recovery_mode && !id_mode) {
		mode = APP_STATE_MODE_RECOVERY;
	} else if (!recovery_mode && id_mode) {
		mode = APP_STATE_MODE_ID;
	} else {
		mode = APP_STATE_MODE_ALL;
	}
	atomic_set(&app_state_mode, mode);
}

int app_ui_state_change_indicate(enum app_ui_state state, bool active)
{
	static const uint8_t state_led_map[APP_UI_STATE_COUNT] = {
		[APP_UI_STATE_APP_RUNNING] = APP_RUN_STATUS_LED,
		[APP_UI_STATE_RINGING] = APP_RING_LED,
		[APP_UI_STATE_RECOVERY_MODE] = APP_MODE_LED,
		[APP_UI_STATE_ID_MODE] = APP_MODE_LED,
		[APP_UI_STATE_PROVISIONED] = APP_PROVISIONED_LED,
		[APP_UI_STATE_FP_ADV] = NO_LED,
	};

	__ASSERT_NO_MSG(state < APP_UI_STATE_COUNT);

	if (state == APP_UI_STATE_APP_RUNNING) {
		state_app_running_indicate(active);
		return 0;
	}

	if ((state == APP_UI_STATE_RECOVERY_MODE) || (state == APP_UI_STATE_ID_MODE)) {
		state_mode_indicate(state, active);
		return 0;
	}

	if (state_led_map[state] == NO_LED) {
		return 0;
	}

	dk_set_led(state_led_map[state], active);

	return 0;
}

int app_ui_init(void)
{
	int err;

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)", err);
		return err;
	}

	err = dk_buttons_init(btn_handle);
	if (err) {
		LOG_ERR("Buttons init failed (err %d)", err);
		return err;
	}

	bootup_btn_handle();

	return 0;
}
