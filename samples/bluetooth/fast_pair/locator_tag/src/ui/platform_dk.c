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

/* Minimum button hold time in milliseconds to trigger the FMDN recovery mode. */
#define RECOVERY_MODE_BTN_MIN_HOLD_TIME_MS	3000

/* Minimum button hold time in milliseconds to trigger the DFU mode. */
#define DFU_MODE_BTN_MIN_HOLD_TIME_MS		7000

/* Run status LED blinking interval. */
#define RUN_LED_BLINK_INTERVAL_MS		1000

/* Run status LED blinking interval in the DFU mode. */
#define RUN_LED_DFU_BLINK_INTERVAL_MS		250

/* Mode LED blinking interval in recovery mode. */
#define MODE_LED_RECOVERY_BLINK_INTERVAL_MS	1000

/* Mode LED blinking interval in identification mode. */
#define MODE_LED_ID_BLINK_INTERVAL_MS		500

/* Provisioning LED blinking interval when provisioned and FP advertising is active. */
#define PROVISIONING_LED_FP_ADV_PROV_BLINK_INTERVAL_MS		250

/* Provisioning LED blinking interval when not provisioned and FP advertising is active. */
#define PROVISIONING_LED_FP_ADV_NOT_PROV_BLINK_INTERVAL_MS	1000

/* Assignments of the DK LEDs and buttons to different functionalities and modules. */

/* Run and DFU mode status LED. */
#define APP_RUN_STATUS_LED			DK_LED1

/* Ringing status LED. */
#define APP_RING_LED				DK_LED2

/* Provisioning and FP advertising status LED. */
#define APP_PROVISIONING_LED			DK_LED3

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

#define LED_WORKQ_PRIORITY	K_PRIO_PREEMPT(0)
#define LED_WORKQ_STACK_SIZE	512

static K_THREAD_STACK_DEFINE(led_workq_stack, LED_WORKQ_STACK_SIZE);
static struct k_work_q led_workq;

static void run_led_work_handle(struct k_work *item);
static K_WORK_DELAYABLE_DEFINE(run_led_work, run_led_work_handle);

static void mode_led_work_handle(struct k_work *item);
static K_WORK_DELAYABLE_DEFINE(mode_led_work, mode_led_work_handle);

static void provisioning_led_work_handle(struct k_work *item);
static K_WORK_DELAYABLE_DEFINE(provisioning_led_work, provisioning_led_work_handle);

static struct {
	struct k_work_delayable *work;
	const uint32_t displayed_state_bm;
} led_works_map[] = {
	{.work = &run_led_work,		 .displayed_state_bm = BIT(APP_UI_STATE_APP_RUNNING)},
	{.work = &mode_led_work,	 .displayed_state_bm = (BIT(APP_UI_STATE_ID_MODE) |
								BIT(APP_UI_STATE_RECOVERY_MODE))},
	{.work = &provisioning_led_work, .displayed_state_bm = (BIT(APP_UI_STATE_PROVISIONED) |
								BIT(APP_UI_STATE_FP_ADV))},
};

static ATOMIC_DEFINE(ui_state_status, APP_UI_STATE_COUNT);

BUILD_ASSERT(APP_UI_STATE_COUNT <= (sizeof(uint32_t) * __CHAR_BIT__));

BUILD_ASSERT(DFU_MODE_BTN_MIN_HOLD_TIME_MS > RECOVERY_MODE_BTN_MIN_HOLD_TIME_MS);

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
				if (hold_time > DFU_MODE_BTN_MIN_HOLD_TIME_MS) {
					app_ui_request_broadcast(APP_UI_REQUEST_DFU_MODE_ENTER);
				} else if (hold_time > RECOVERY_MODE_BTN_MIN_HOLD_TIME_MS) {
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

static void run_led_work_handle(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	static bool run_led_on;

	if (atomic_test_bit(ui_state_status, APP_UI_STATE_APP_RUNNING)) {
		uint32_t blink_interval_ms = atomic_test_bit(ui_state_status,
							     APP_UI_STATE_DFU_MODE) ?
					     RUN_LED_DFU_BLINK_INTERVAL_MS :
					     RUN_LED_BLINK_INTERVAL_MS;

		run_led_on = !run_led_on;
		(void) k_work_reschedule_for_queue(&led_workq, dwork, K_MSEC(blink_interval_ms));
	} else {
		run_led_on = false;
	}

	dk_set_led(APP_RUN_STATUS_LED, run_led_on);
}

static void mode_led_work_handle(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	static bool mode_led_on;

	bool id_mode = atomic_test_bit(ui_state_status, APP_UI_STATE_ID_MODE);
	bool recovery_mode = atomic_test_bit(ui_state_status, APP_UI_STATE_RECOVERY_MODE);

	if (recovery_mode != id_mode) {
		uint32_t blink_interval_ms = recovery_mode ?
					 MODE_LED_RECOVERY_BLINK_INTERVAL_MS :
					 MODE_LED_ID_BLINK_INTERVAL_MS;

		mode_led_on = !mode_led_on;
		(void) k_work_reschedule_for_queue(&led_workq, dwork, K_MSEC(blink_interval_ms));
	} else {
		mode_led_on = recovery_mode && id_mode;
	}

	dk_set_led(APP_MODE_LED, mode_led_on);
}

static void provisioning_led_work_handle(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	static bool provisioning_led_on;

	bool provisioned = atomic_test_bit(ui_state_status, APP_UI_STATE_PROVISIONED);
	bool fp_adv = atomic_test_bit(ui_state_status, APP_UI_STATE_FP_ADV);

	if (fp_adv) {
		uint32_t blink_interval_ms = provisioned ?
					 PROVISIONING_LED_FP_ADV_PROV_BLINK_INTERVAL_MS :
					 PROVISIONING_LED_FP_ADV_NOT_PROV_BLINK_INTERVAL_MS;

		provisioning_led_on = !provisioning_led_on;
		(void) k_work_reschedule_for_queue(&led_workq, dwork, K_MSEC(blink_interval_ms));
	} else {
		provisioning_led_on = provisioned;
	}

	dk_set_led(APP_PROVISIONING_LED, provisioning_led_on);
}

int app_ui_state_change_indicate(enum app_ui_state state, bool active)
{
	__ASSERT_NO_MSG(state < APP_UI_STATE_COUNT);

	atomic_set_bit_to(ui_state_status, state, active);

	for (size_t i = 0; i < ARRAY_SIZE(led_works_map); i++) {
		if (led_works_map[i].displayed_state_bm & BIT(state)) {
			(void) k_work_reschedule_for_queue(&led_workq,
							   led_works_map[i].work,
							   K_NO_WAIT);
		}
	}

	/* Only the ring LED needs to be driven here directly, the remaining LEDs
	 * are handled by the respective work items.
	 */
	if (state == APP_UI_STATE_RINGING) {
		dk_set_led(APP_RING_LED, active);
	}

	return 0;
}

int app_ui_init(void)
{
	int err;

	k_work_queue_init(&led_workq);
	k_work_queue_start(&led_workq, led_workq_stack,
			   K_THREAD_STACK_SIZEOF(led_workq_stack), LED_WORKQ_PRIORITY,
			   NULL);

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
