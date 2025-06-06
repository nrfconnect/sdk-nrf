/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <dk_buttons_and_leds.h>

#include "app_ui.h"
#include "app_ui_priv.h"

#include "app_ui_speaker.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_DBG);

/* Speaker beep timings. */
#define SPEAKER_BEEP_SHORT_ON_MS		20
#define SPEAKER_BEEP_SHORT_OFF_MS		100
#define SPEAKER_BEEP_MID_ON_MS			250
#define SPEAKER_BEEP_MID_OFF_MS			100
#define SPEAKER_BEEP_LONG_ON_MS			1000
#define SPEAKER_BEEP_LONG_OFF_MS		100

/* Thingy LED hardware assignments. */
#define LED_COLOR_RED				DK_LED1
#define LED_COLOR_GREEN				DK_LED2
#define LED_COLOR_BLUE				DK_LED3

#define LED_COLOR_NONE				0xFF
#define LED_RGB_DEF(...)			{__VA_ARGS__, LED_COLOR_NONE}
#define LED_RGB_DEF_LEN				4

/* Thingy state status LEDs. */
#define LED_RUN_STATUS				LED_COLOR_ID_GREEN
#define LED_PROVISIONED				LED_COLOR_ID_BLUE
#define LED_ID_MODE				LED_COLOR_ID_YELLOW
#define LED_RECOVERY_MODE			LED_COLOR_ID_RED
#define LED_FP_ADV				LED_COLOR_ID_WHITE
#define LED_DFU_MODE				LED_COLOR_ID_PURPLE
#define LED_MOTION_DETECTOR_ACTIVE		LED_COLOR_ID_CYAN

/* UI state status LEDs handling. */
#define LED_BLINK_STATE_STATUS_ON_MS		500
#define LED_THREAD_INTERVAL_MS			1000
#define LED_THREAD_PRIORITY			K_PRIO_PREEMPT(0)
#define LED_THREAD_STACK_SIZE			512

/* Thingy button hardware assignments. */
#define BTN_MULTI_ACTION			DK_BTN1_MSK

/* Release button action windows.
 * Action window = [ <BTN_ACTION>_START_MS ; <BTN_ACTION + 1>_START_MS )
 */
#define BTN_NOOP_FIRST_START_MS			0
#define BTN_FP_ADV_MODE_CHANGE_START_MS		1000
#define BTN_ID_MODE_ENTER_START_MS		3000
#define BTN_RECOVERY_MODE_ENTER_START_MS	5000
#define BTN_DFU_MODE_ENTER_START_MS		7000
#define BTN_NOOP_LAST_START_MS			10000

#define BTN_RELEASE_DEF(_request, _start_ms, _beep_cnt, _beep_duration)			\
	{										\
		.request = _request,							\
		.start_ms = _start_ms,							\
		.beep = {.cnt = _beep_cnt, .speaker_beep_duration = _beep_duration},	\
	}

/** LED colors that can be mixed using red, green and blue LEDs with full brightness. */
enum led_color_id {
	LED_COLOR_ID_RED,
	LED_COLOR_ID_GREEN,
	LED_COLOR_ID_BLUE,
	LED_COLOR_ID_YELLOW,
	LED_COLOR_ID_PURPLE,
	LED_COLOR_ID_CYAN,
	LED_COLOR_ID_WHITE,

	LED_COLOR_ID_COUNT,
};

/** Beep pattern. */
struct speaker_beep {
	/* Number of beeps. */
	uint8_t cnt;

	/* Type of beep which maps to the speaker on and off periods. */
	enum {
		SPEAKER_BEEP_DURATION_SHORT,
		SPEAKER_BEEP_DURATION_MID,
		SPEAKER_BEEP_DURATION_LONG,

		SPEAKER_BEEP_DURATION_COUNT,
	} speaker_beep_duration;
};

/** Button release action. */
struct btn_release {
	/** UI request which will be sent. */
	enum app_ui_request request;

	/** Threshold from which this release action will be proceeded. */
	uint32_t start_ms;

	/** Beep indicator when the threshold will be reached. */
	struct speaker_beep beep;
};

/** State to LED ID map. */
struct led_state_id_map {
	/** UI state on which the related LED should blink. */
	enum app_ui_state state;

	/** UI state related LED. */
	enum led_color_id id;
};

static int8_t btn_idx = -1;
static const struct btn_release btn_release_actions[] = {
	BTN_RELEASE_DEF(APP_UI_REQUEST_COUNT,
			BTN_NOOP_FIRST_START_MS,
			0,
			SPEAKER_BEEP_DURATION_COUNT),
	BTN_RELEASE_DEF(APP_UI_REQUEST_FP_ADV_PAIRING_MODE_CHANGE,
			BTN_FP_ADV_MODE_CHANGE_START_MS,
			1,
			SPEAKER_BEEP_DURATION_SHORT),
	BTN_RELEASE_DEF(APP_UI_REQUEST_ID_MODE_ENTER,
			BTN_ID_MODE_ENTER_START_MS,
			2,
			SPEAKER_BEEP_DURATION_SHORT),
	BTN_RELEASE_DEF(APP_UI_REQUEST_RECOVERY_MODE_ENTER,
			BTN_RECOVERY_MODE_ENTER_START_MS,
			3,
			SPEAKER_BEEP_DURATION_SHORT),
	BTN_RELEASE_DEF(APP_UI_REQUEST_DFU_MODE_ENTER,
			BTN_DFU_MODE_ENTER_START_MS,
			4,
			SPEAKER_BEEP_DURATION_SHORT),
	BTN_RELEASE_DEF(APP_UI_REQUEST_COUNT,
			BTN_NOOP_LAST_START_MS,
			1,
			SPEAKER_BEEP_DURATION_MID),
};
static void btn_press_beep_work_handle(struct k_work *w);
static K_WORK_DELAYABLE_DEFINE(btn_press_beep_work, btn_press_beep_work_handle);

static ATOMIC_DEFINE(ui_state_status, APP_UI_STATE_COUNT);
static const struct led_state_id_map led_state_id_maps[] = {
	{.state = APP_UI_STATE_APP_RUNNING,		.id = LED_RUN_STATUS},
	{.state = APP_UI_STATE_PROVISIONED,		.id = LED_PROVISIONED},
	{.state = APP_UI_STATE_ID_MODE,			.id = LED_ID_MODE},
	{.state = APP_UI_STATE_RECOVERY_MODE,		.id = LED_RECOVERY_MODE},
	{.state = APP_UI_STATE_FP_ADV,			.id = LED_FP_ADV},
	{.state = APP_UI_STATE_DFU_MODE,		.id = LED_DFU_MODE},
	{.state = APP_UI_STATE_MOTION_DETECTOR_ACTIVE,	.id = LED_MOTION_DETECTOR_ACTIVE},
};

static void speaker_beep_play(uint8_t cnt, uint32_t on_ms, uint32_t off_ms)
{
	for (uint8_t i = 0; i < cnt; i++) {
		app_ui_speaker_on();
		k_msleep(on_ms);
		app_ui_speaker_off();
		k_msleep(off_ms);
	}
}

static void speaker_beep_handle(const struct speaker_beep *req)
{
	switch (req->speaker_beep_duration) {
	case SPEAKER_BEEP_DURATION_SHORT:
		speaker_beep_play(req->cnt, SPEAKER_BEEP_SHORT_ON_MS, SPEAKER_BEEP_SHORT_OFF_MS);
		break;

	case SPEAKER_BEEP_DURATION_MID:
		speaker_beep_play(req->cnt, SPEAKER_BEEP_MID_ON_MS, SPEAKER_BEEP_MID_OFF_MS);
		break;

	case SPEAKER_BEEP_DURATION_LONG:
		speaker_beep_play(req->cnt, SPEAKER_BEEP_LONG_ON_MS, SPEAKER_BEEP_LONG_OFF_MS);
		break;

	case SPEAKER_BEEP_DURATION_COUNT:
		/* ignore */
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}
};

static uint32_t btn_work_timeout_get(int8_t idx)
{
	__ASSERT_NO_MSG(idx >= 0 && idx < ARRAY_SIZE(btn_release_actions));

	if (idx == 0) {
		return btn_release_actions[idx].start_ms;
	}

	__ASSERT_NO_MSG(btn_release_actions[idx].start_ms > btn_release_actions[idx - 1].start_ms);

	return btn_release_actions[idx].start_ms - btn_release_actions[idx - 1].start_ms;
}

static void btn_press_beep_work_handle(struct k_work *w)
{
	btn_idx++;

	__ASSERT_NO_MSG(btn_idx >= 0 && btn_idx < ARRAY_SIZE(btn_release_actions));

	speaker_beep_handle(&btn_release_actions[btn_idx].beep);

	if (btn_idx >= ARRAY_SIZE(btn_release_actions) - 1) {
		/* Last button release action. */
		return;
	}

	k_work_reschedule(&btn_press_beep_work, K_MSEC(btn_work_timeout_get(btn_idx + 1)));
}

static void btn_handle(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & BTN_MULTI_ACTION) {
		if (button_state & BTN_MULTI_ACTION) {
			/* On button press */

			/* Schedule speaker notifications & btn_idx counting on each threshold. */
			k_work_reschedule(&btn_press_beep_work,
					  K_MSEC(btn_work_timeout_get(btn_idx + 1)));

			/* If ringing is active then always stop it on any button press. */
			if (atomic_test_bit(ui_state_status, APP_UI_STATE_RINGING)) {
				app_ui_request_broadcast(APP_UI_REQUEST_RINGING_STOP);
			}
		} else {
			/* On button release */

			/* Thingy has only one button which is shared with the factory reset action
			 * and also handled in the bootup_btn_handle function. Holding the button
			 * while Thingy restarts will not emit a button press event. Button release
			 * not preceded by the button press should be ignored.
			 */
			if ((btn_idx >= 0) && (btn_idx < ARRAY_SIZE(btn_release_actions))) {
				/* No more speaker notifications & btn_idx counting needed. */
				k_work_cancel_delayable(&btn_press_beep_work);

				if (btn_release_actions[btn_idx].request != APP_UI_REQUEST_COUNT) {
					app_ui_request_broadcast(
						btn_release_actions[btn_idx].request);
				}
			}

			/* Action has been completed, consume. */
			btn_idx = -1;
		}
	}
}

static void bootup_btn_handle(void)
{
	uint32_t button_state;
	uint32_t has_changed;

	dk_read_buttons(&button_state, &has_changed);

	if (button_state & BTN_MULTI_ACTION) {
		speaker_beep_handle(&(struct speaker_beep)
			{.cnt = 1, .speaker_beep_duration = SPEAKER_BEEP_DURATION_LONG});
		app_ui_request_broadcast(APP_UI_REQUEST_FACTORY_RESET);
	}
}

static void led_drive(enum led_color_id color, bool active)
{
	static const uint8_t led_id_to_hw[LED_COLOR_ID_COUNT][LED_RGB_DEF_LEN] = {
		[LED_COLOR_ID_RED] = LED_RGB_DEF(LED_COLOR_RED),
		[LED_COLOR_ID_GREEN] = LED_RGB_DEF(LED_COLOR_GREEN),
		[LED_COLOR_ID_BLUE] = LED_RGB_DEF(LED_COLOR_BLUE),
		[LED_COLOR_ID_YELLOW] = LED_RGB_DEF(LED_COLOR_RED, LED_COLOR_GREEN),
		[LED_COLOR_ID_PURPLE] = LED_RGB_DEF(LED_COLOR_RED, LED_COLOR_BLUE),
		[LED_COLOR_ID_CYAN] = LED_RGB_DEF(LED_COLOR_GREEN, LED_COLOR_BLUE),
		[LED_COLOR_ID_WHITE] = LED_RGB_DEF(LED_COLOR_RED, LED_COLOR_GREEN, LED_COLOR_BLUE),
	};

	__ASSERT_NO_MSG(color < LED_COLOR_ID_COUNT);

	const uint8_t *leds = led_id_to_hw[color];

	for (uint8_t i = 0; leds[i] != LED_COLOR_NONE; i++) {
		dk_set_led(leds[i], active);
	}
}

static void led_blink(uint8_t color, uint8_t cnt, uint32_t on_ms, uint32_t off_ms)
{
	for (uint8_t i = 0; i < cnt; i++) {
		led_drive(color, true);
		k_msleep(on_ms);
		led_drive(color, false);
		k_msleep(off_ms);
	}
}

static void led_blink_once(uint8_t color, uint32_t on_ms)
{
	led_blink(color, 1, on_ms, 0);
}

static void led_thread_process(void)
{
	while (1) {
		for (uint8_t i = 0; i < ARRAY_SIZE(led_state_id_maps); i++) {
			if (atomic_test_bit(ui_state_status, led_state_id_maps[i].state)) {
				led_blink_once(led_state_id_maps[i].id,
					       LED_BLINK_STATE_STATUS_ON_MS);
			}
		}

		k_msleep(LED_THREAD_INTERVAL_MS);
	}
}

K_THREAD_DEFINE(led_thread_id, LED_THREAD_STACK_SIZE, led_thread_process,
		NULL, NULL, NULL, LED_THREAD_PRIORITY, 0, 0);

static void cancel_ringing_work_handle(struct k_work *w)
{
	if (atomic_test_bit(ui_state_status, APP_UI_STATE_RINGING)) {
		app_ui_request_broadcast(APP_UI_REQUEST_RINGING_STOP);
	}
}

int app_ui_state_change_indicate(enum app_ui_state state, bool active)
{
	static K_WORK_DELAYABLE_DEFINE(cancel_ringing_work, cancel_ringing_work_handle);

	if (active) {
		atomic_set_bit(ui_state_status, state);
	} else {
		atomic_clear_bit(ui_state_status, state);
	}

	if (state == APP_UI_STATE_RINGING) {
		/* If there is any button action in progress, then send ringing stop request.
		 * A slight delay has been added as there is an issue with the FMD app that
		 * does not register sound completion after a very short (close to zero) timeout.
		 */
		if (active && (btn_idx != -1)) {
			k_work_reschedule(&cancel_ringing_work, K_MSEC(50));
		} else {
			active ? app_ui_speaker_on() : app_ui_speaker_off();
		}
	}

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

	err = app_ui_speaker_init();
	if (err) {
		LOG_ERR("Speaker init failed (err %d)", err);
		return err;
	}

	bootup_btn_handle();

	return 0;
}
