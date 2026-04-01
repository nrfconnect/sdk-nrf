/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <dk_buttons_and_leds.h>

#include "app_ui.h"
#include "app_ui_priv.h"

#include "app_ui_speaker_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fhn, LOG_LEVEL_DBG);

#define BTN_ACTION_IND_LED_FLASH_SHORT_ON_MS		40
#define BTN_ACTION_IND_LED_FLASH_SHORT_OFF_MS		200
#define BTN_ACTION_IND_LED_FLASH_MID_ON_MS		500
#define BTN_ACTION_IND_LED_FLASH_MID_OFF_MS		200
#define BTN_ACTION_IND_LED_FLASH_LONG_ON_MS		1000
#define BTN_ACTION_IND_LED_FLASH_LONG_OFF_MS		200

/* In platforms with only LED indication, the added clearance off time visually separates
 * the button action indication from the previous LED state.
 * In platforms with speaker indication, the clearance off time is not needed as the button
 * action indication is also represented by the sound.
 * With clearance off time set to 0, the LED indication will start at the same time
 * as the sound indication.
 */
#ifdef CONFIG_APP_UI_USE_HW_SPEAKER
#define BTN_ACTION_IND_LED_FLASH_CLEARANCE_OFF_MS	0
#else
#define BTN_ACTION_IND_LED_FLASH_CLEARANCE_OFF_MS	250
#endif

#define BTN_ACTION_IND_SPEAKER_BEEP_SHORT_ON_MS		20
#define BTN_ACTION_IND_SPEAKER_BEEP_SHORT_OFF_MS	100
#define BTN_ACTION_IND_SPEAKER_BEEP_MID_ON_MS		250
#define BTN_ACTION_IND_SPEAKER_BEEP_MID_OFF_MS		100
#define BTN_ACTION_IND_SPEAKER_BEEP_LONG_ON_MS		1000
#define BTN_ACTION_IND_SPEAKER_BEEP_LONG_OFF_MS		100

/* Reference LED hardware assignments. */
#define LED_COLOR_RED				DK_LED1
#define LED_COLOR_GREEN				DK_LED2
#define LED_COLOR_BLUE				DK_LED3

#define LED_COLOR_NONE				0xFF
#define LED_RGB_DEF(...)			{__VA_ARGS__, LED_COLOR_NONE}
#define LED_RGB_DEF_LEN				4


#define LED_RINGING_STATUS			LED_COLOR_ID_GREEN

/* Reference state status LEDs. */
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
#define LED_BLINK_STATE_RING_ON_OFF_MS		125
#define LED_BLINK_STATE_RING_ON_OFF_CNT		4

/* Speaker thread configuration. */
#define SPK_THREAD_PRIORITY			K_PRIO_PREEMPT(0)
#define SPK_THREAD_STACK_SIZE			512

/* Reference button hardware assignments. */
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

#define BTN_RELEASE_DEF(_request, _start_ms, _ind_cnt, _ind_duration)	\
	{								\
		.request = _request,					\
		.start_ms = _start_ms,					\
		.action_ind = {						\
			.cnt = _ind_cnt,				\
			.duration = _ind_duration},			\
	}

/* Indication request encoding */
#define IND_REQUEST_CNT_POS			0
#define IND_REQUEST_CNT_MASK			0xFF
#define IND_REQUEST_ON_MS_POS			8
#define IND_REQUEST_ON_MS_MASK			0xFFF
#define IND_REQUEST_OFF_MS_POS			20
#define IND_REQUEST_OFF_MS_MASK			0xFFF

#define IND_REQUEST_ENCODE(_cnt, _on_ms, _off_ms)					\
	((((_cnt) & IND_REQUEST_CNT_MASK) << IND_REQUEST_CNT_POS) |			\
	 (((_on_ms) & IND_REQUEST_ON_MS_MASK) << IND_REQUEST_ON_MS_POS) |		\
	 (((_off_ms) & IND_REQUEST_OFF_MS_MASK) << IND_REQUEST_OFF_MS_POS))

#define IND_REQUEST_DECODE_CNT(req)	\
	(((req) >> IND_REQUEST_CNT_POS) & IND_REQUEST_CNT_MASK)
#define IND_REQUEST_DECODE_ON_MS(req)	\
	(((req) >> IND_REQUEST_ON_MS_POS) & IND_REQUEST_ON_MS_MASK)
#define IND_REQUEST_DECODE_OFF_MS(req)	\
	(((req) >> IND_REQUEST_OFF_MS_POS) & IND_REQUEST_OFF_MS_MASK)


struct ind_param {
	/* Number of indications. */
	uint8_t cnt;

	/* On time in milliseconds for a single indication. */
	uint32_t on_ms;

	/* Off time in milliseconds for a single indication. */
	uint32_t off_ms;
};

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

/** Button action indicator pattern. */
struct btn_action_ind {
	/* Number of indications. */
	uint8_t cnt;

	/* Type of indication which maps to the indicator on and off periods. */
	enum {
		BTN_ACTION_IND_DURATION_SHORT,
		BTN_ACTION_IND_DURATION_MID,
		BTN_ACTION_IND_DURATION_LONG,

		BTN_ACTION_IND_DURATION_COUNT,
	} duration;
};

/** Button release action. */
struct btn_release {
	/** UI request which will be sent. */
	enum app_ui_request request;

	/** Threshold from which this release action will be proceeded. */
	uint32_t start_ms;

	/** Indicator when the threshold will be reached. */
	struct btn_action_ind action_ind;
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
			BTN_ACTION_IND_DURATION_COUNT),
	BTN_RELEASE_DEF(APP_UI_REQUEST_FP_ADV_PAIRING_MODE_CHANGE,
			BTN_FP_ADV_MODE_CHANGE_START_MS,
			1,
			BTN_ACTION_IND_DURATION_SHORT),
	BTN_RELEASE_DEF(APP_UI_REQUEST_ID_MODE_ENTER,
			BTN_ID_MODE_ENTER_START_MS,
			2,
			BTN_ACTION_IND_DURATION_SHORT),
	BTN_RELEASE_DEF(APP_UI_REQUEST_RECOVERY_MODE_ENTER,
			BTN_RECOVERY_MODE_ENTER_START_MS,
			3,
			BTN_ACTION_IND_DURATION_SHORT),
	BTN_RELEASE_DEF(APP_UI_REQUEST_DFU_MODE_ENTER,
			BTN_DFU_MODE_ENTER_START_MS,
			4,
			BTN_ACTION_IND_DURATION_SHORT),
	BTN_RELEASE_DEF(APP_UI_REQUEST_COUNT,
			BTN_NOOP_LAST_START_MS,
			1,
			BTN_ACTION_IND_DURATION_MID),
};
static void btn_press_beep_work_handle(struct k_work *w);
static K_WORK_DELAYABLE_DEFINE(btn_press_beep_work, btn_press_beep_work_handle);
static bool btn_pressed;

static ATOMIC_DEFINE(ui_state_status, APP_UI_STATE_COUNT);
static const struct led_state_id_map led_state_id_maps[] = {
	{.state = APP_UI_STATE_PROVISIONED,		.id = LED_PROVISIONED},
	{.state = APP_UI_STATE_ID_MODE,			.id = LED_ID_MODE},
	{.state = APP_UI_STATE_RECOVERY_MODE,		.id = LED_RECOVERY_MODE},
	{.state = APP_UI_STATE_FP_ADV,			.id = LED_FP_ADV},
	{.state = APP_UI_STATE_DFU_MODE,		.id = LED_DFU_MODE},
	{.state = APP_UI_STATE_MOTION_DETECTOR_ACTIVE,	.id = LED_MOTION_DETECTOR_ACTIVE},
};

static atomic_t led_flash_request = ATOMIC_INIT(0);
static K_SEM_DEFINE(led_flash_request_sem, 0, 1);

static atomic_t spk_beep_request = ATOMIC_INIT(0);
static K_SEM_DEFINE(spk_beep_request_sem, 0, 1);

static atomic_val_t ind_request_encode(const struct ind_param *param)
{
	__ASSERT_NO_MSG(param);
	__ASSERT_NO_MSG(param->cnt <= IND_REQUEST_CNT_MASK);
	__ASSERT_NO_MSG(param->on_ms <= IND_REQUEST_ON_MS_MASK);
	__ASSERT_NO_MSG(param->off_ms <= IND_REQUEST_OFF_MS_MASK);

	return IND_REQUEST_ENCODE(param->cnt, param->on_ms, param->off_ms);
}

static struct ind_param ind_request_decode(atomic_val_t req)
{
	struct ind_param param = {
		.cnt = IND_REQUEST_DECODE_CNT(req),
		.on_ms = IND_REQUEST_DECODE_ON_MS(req),
		.off_ms = IND_REQUEST_DECODE_OFF_MS(req),
	};

	return param;
}

static void led_flash_indicate_request(struct ind_param *param)
{
	__ASSERT_NO_MSG(param);

	atomic_set(&led_flash_request, ind_request_encode(param));
	k_sem_give(&led_flash_request_sem);
}

static void speaker_beep_indicate_request(struct ind_param *param)
{
	__ASSERT_NO_MSG(param);

	if (!IS_ENABLED(CONFIG_APP_UI_USE_HW_SPEAKER)) {
		return;
	}

	atomic_set(&spk_beep_request, ind_request_encode(param));
	k_sem_give(&spk_beep_request_sem);
}

static void speaker_beep_play(struct ind_param *param)
{
	__ASSERT_NO_MSG(param);

	for (uint8_t i = 0; i < param->cnt; i++) {
		app_ui_speaker_on();
		k_msleep(param->on_ms);
		app_ui_speaker_off();
		k_msleep(param->off_ms);
	}
}

static uint32_t btn_work_timeout_get(int8_t idx)
{
	__ASSERT_NO_MSG(idx >= 0 && idx < ARRAY_SIZE(btn_release_actions));

	if (idx == 0) {
		return btn_release_actions[idx].start_ms;
	}

	__ASSERT_NO_MSG(btn_release_actions[idx].start_ms > btn_release_actions[idx - 1].start_ms);

	return btn_release_actions[idx].start_ms - btn_release_actions[idx - 1].start_ms;
}

static void btn_action_ind_handle(const struct btn_action_ind *req)
{
	struct ind_param param_led;
	struct ind_param param_spk;

	switch (req->duration) {
	case BTN_ACTION_IND_DURATION_SHORT:
		param_led.on_ms = BTN_ACTION_IND_LED_FLASH_SHORT_ON_MS;
		param_led.off_ms = BTN_ACTION_IND_LED_FLASH_SHORT_OFF_MS;
		param_spk.on_ms = BTN_ACTION_IND_SPEAKER_BEEP_SHORT_ON_MS;
		param_spk.off_ms = BTN_ACTION_IND_SPEAKER_BEEP_SHORT_OFF_MS;
		break;

	case BTN_ACTION_IND_DURATION_MID:
		param_led.on_ms = BTN_ACTION_IND_LED_FLASH_MID_ON_MS;
		param_led.off_ms = BTN_ACTION_IND_LED_FLASH_MID_OFF_MS;
		param_spk.on_ms = BTN_ACTION_IND_SPEAKER_BEEP_MID_ON_MS;
		param_spk.off_ms = BTN_ACTION_IND_SPEAKER_BEEP_MID_OFF_MS;
		break;

	case BTN_ACTION_IND_DURATION_LONG:
		param_led.on_ms = BTN_ACTION_IND_LED_FLASH_LONG_ON_MS;
		param_led.off_ms = BTN_ACTION_IND_LED_FLASH_LONG_OFF_MS;
		param_spk.on_ms = BTN_ACTION_IND_SPEAKER_BEEP_LONG_ON_MS;
		param_spk.off_ms = BTN_ACTION_IND_SPEAKER_BEEP_LONG_OFF_MS;
		break;

	case BTN_ACTION_IND_DURATION_COUNT:
		__ASSERT_NO_MSG(!req->cnt);
		return;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	param_led.cnt = req->cnt;
	param_spk.cnt = req->cnt;

	led_flash_indicate_request(&param_led);
	speaker_beep_indicate_request(&param_spk);
}

static void btn_press_beep_work_handle(struct k_work *w)
{
	btn_idx++;

	__ASSERT_NO_MSG(btn_idx >= 0 && btn_idx < ARRAY_SIZE(btn_release_actions));

	btn_action_ind_handle(&btn_release_actions[btn_idx].action_ind);

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
			btn_pressed = true;

			/* Schedule speaker notifications & btn_idx counting on each threshold. */
			k_work_reschedule(&btn_press_beep_work,
					  K_MSEC(btn_work_timeout_get(btn_idx + 1)));

			/* If ringing is active then always stop it on any button press. */
			if (atomic_test_bit(ui_state_status, APP_UI_STATE_RINGING)) {
				app_ui_request_broadcast(APP_UI_REQUEST_RINGING_STOP);
			}
		} else {
			/* On button release */
			btn_pressed = false;

			/* Reference platform has only one button which is shared with the factory
			 * reset action and also handled in the bootup_btn_handle function.
			 * Holding the button while the platform restarts will not emit a button
			 * press event.
			 * Button release not preceded by the button press should be ignored.
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
		btn_action_ind_handle(&(struct btn_action_ind)
			{.cnt = 1, .duration = BTN_ACTION_IND_DURATION_LONG});
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

static void led_reset(void)
{
	dk_set_led(LED_COLOR_RED, false);
	dk_set_led(LED_COLOR_GREEN, false);
	dk_set_led(LED_COLOR_BLUE, false);
}

static bool led_sleep_interruptible(uint32_t ms)
{
	return k_sem_take(&led_flash_request_sem, K_MSEC(ms)) == 0;
}

static bool led_blink_interruptible(uint8_t color, struct ind_param *param)
{
	__ASSERT_NO_MSG(param);

	for (uint8_t i = 0; i < param->cnt; i++) {
		led_drive(color, true);
		if (led_sleep_interruptible(param->on_ms)) {
			led_drive(color, false);
			return true;
		}

		led_drive(color, false);
		if (led_sleep_interruptible(param->off_ms)) {
			return true;
		}
	}
	return false;
}

static bool led_blink_once_interruptible(uint8_t color, uint32_t on_ms)
{
	struct ind_param param = {
		.cnt = 1,
		.on_ms = on_ms,
		.off_ms = 0
	};

	return led_blink_interruptible(color, &param);
}

static void led_blink(uint8_t color, struct ind_param *param)
{
	__ASSERT_NO_MSG(param);

	for (uint8_t i = 0; i < param->cnt; i++) {
		led_drive(color, true);
		k_msleep(param->on_ms);
		led_drive(color, false);
		k_msleep(param->off_ms);
	}
}

static void led_flash_indicate(struct ind_param *param)
{
	__ASSERT_NO_MSG(param);

	/* Add clearance delay to visually separate
	 * flash from the previous LED state.
	 */
	led_reset();
	k_msleep(BTN_ACTION_IND_LED_FLASH_CLEARANCE_OFF_MS);
	led_blink(LED_RINGING_STATUS, param);
}

/* Indicate button and ringing state
 * Return true if the indication was interrupted.
 */
static bool led_flash_and_ring_status_indicate(void)
{
	atomic_val_t req = atomic_clear(&led_flash_request);
	struct ind_param param_btn;
	struct ind_param param_ring = {
		.cnt = LED_BLINK_STATE_RING_ON_OFF_CNT,
		.on_ms = LED_BLINK_STATE_RING_ON_OFF_MS,
		.off_ms = LED_BLINK_STATE_RING_ON_OFF_MS
	};

	if (req) {
		param_btn = ind_request_decode(req);

		/* In the rare event of semaphore not interrupting any signaling. */
		k_sem_take(&led_flash_request_sem, K_NO_WAIT);
		if (param_btn.cnt) {
			led_flash_indicate(&param_btn);
		}
	}

	if (btn_pressed) {
		led_sleep_interruptible(LED_THREAD_INTERVAL_MS);
		return true;
	}

	if (atomic_test_bit(ui_state_status, APP_UI_STATE_RINGING)) {
		led_blink_interruptible(LED_RINGING_STATUS, &param_ring);
		return true;
	}

	return false;
}

static void led_thread_process(void)
{
	while (1) {
		if (led_flash_and_ring_status_indicate()) {
			continue;
		}

		bool interrupted = false;

		for (uint8_t i = 0; i < ARRAY_SIZE(led_state_id_maps); i++) {
			if (atomic_test_bit(ui_state_status, led_state_id_maps[i].state)) {
				interrupted = led_blink_once_interruptible(led_state_id_maps[i].id,
								LED_BLINK_STATE_STATUS_ON_MS);
				if (interrupted) {
					break;
				}
			}
		}

		if (interrupted) {
			continue;
		}

		/* Idle wait: wakes on beep request or after the normal interval. */
		led_sleep_interruptible(LED_THREAD_INTERVAL_MS);
	}
}

K_THREAD_DEFINE(led_thread_id, LED_THREAD_STACK_SIZE, led_thread_process,
		NULL, NULL, NULL, LED_THREAD_PRIORITY, 0, 0);

#ifdef CONFIG_APP_UI_USE_HW_SPEAKER
static void speaker_thread_process(void)
{
	struct ind_param param;
	atomic_val_t req;

	while (1) {
		k_sem_take(&spk_beep_request_sem, K_FOREVER);

		req = atomic_clear(&spk_beep_request);

		if (req) {
			param = ind_request_decode(req);
			speaker_beep_play(&param);
		}
	}
}

K_THREAD_DEFINE(spk_thread_id, SPK_THREAD_STACK_SIZE, speaker_thread_process,
		NULL, NULL, NULL, SPK_THREAD_PRIORITY, 0, 0);
#endif /* CONFIG_APP_UI_USE_HW_SPEAKER */

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
		 * A slight delay has been added as there is an issue with the Find Hub app that
		 * does not register sound completion after a very short (close to zero) timeout.
		 */
		if (active && (btn_idx != -1)) {
			k_work_reschedule(&cancel_ringing_work, K_MSEC(50));
		} else {
			if (IS_ENABLED(CONFIG_APP_UI_USE_HW_SPEAKER)) {
				active ? app_ui_speaker_on() : app_ui_speaker_off();
			}
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

	if (IS_ENABLED(CONFIG_APP_UI_USE_HW_SPEAKER)) {
		err = app_ui_speaker_init();
		if (err) {
			LOG_ERR("Speaker init failed (err %d)", err);
			return err;
		}
	}

	bootup_btn_handle();

	return 0;
}
