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

/* In platforms with only LED indication, the added clearance off time visually separates
 * the button action indication from the previous LED state.
 * In platforms with speaker indication, the clearance off time is not needed as the button
 * action indication is also represented by the sound.
 * With clearance off time set to 0, the LED indication will start at the same time
 * as the sound indication.
 */
#ifdef CONFIG_APP_UI_USE_HW_SPEAKER
#define IND_CLEARANCE_OFF_MS		0
#else
#define IND_CLEARANCE_OFF_MS		250
#endif

#define BTN_ACTION_IND_SHORT_ON_MS		40
#define BTN_ACTION_IND_SHORT_OFF_MS		200
#define BTN_ACTION_IND_MID_ON_MS		500
#define BTN_ACTION_IND_MID_OFF_MS		200
#define BTN_ACTION_IND_LONG_ON_MS		1000
#define BTN_ACTION_IND_LONG_OFF_MS		200

/* Reference LED hardware assignments. */
#define LED_COLOR_RED				DK_LED1
#define LED_COLOR_GREEN				DK_LED2
#define LED_COLOR_BLUE				DK_LED3

#define LED_COLOR_NONE				0xFF
#define LED_RGB_DEF(...)			{__VA_ARGS__, LED_COLOR_NONE}
#define LED_RGB_DEF_LEN				4

#define LED_RINGING_STATUS			LED_COLOR_ID_GREEN
#define LED_INDICATOR_STATUS			LED_COLOR_ID_GREEN

/* Reference state status LEDs. */
#define LED_PROVISIONED				LED_COLOR_ID_BLUE
#define LED_ID_MODE				LED_COLOR_ID_YELLOW
#define LED_RECOVERY_MODE			LED_COLOR_ID_RED
#define LED_FP_ADV				LED_COLOR_ID_WHITE
#define LED_DFU_MODE				LED_COLOR_ID_PURPLE
#define LED_MOTION_DETECTOR_ACTIVE		LED_COLOR_ID_CYAN

/* UI state status LEDs handling. */
#define LED_BLINK_STATE_STATUS_ON_MS		500
#define INDICATION_THREAD_INTERVAL_MS		1000
#define INDICATION_THREAD_PRIORITY		K_PRIO_PREEMPT(0)
#define INDICATION_THREAD_STACK_SIZE		512
#define LED_BLINK_STATE_RING_ON_OFF_MS		125
#define LED_BLINK_STATE_RING_ON_OFF_CNT		1

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

#define BTN_PRESS_FP_ADV_MODE_CHANGE_TIME	\
	(BTN_FP_ADV_MODE_CHANGE_START_MS - BTN_NOOP_FIRST_START_MS)
#define BTN_PRESS_ID_MODE_ENTER_TIME		\
	(BTN_ID_MODE_ENTER_START_MS - BTN_FP_ADV_MODE_CHANGE_START_MS)
#define BTN_PRESS_RECOVERY_MODE_ENTER_TIME	\
	(BTN_RECOVERY_MODE_ENTER_START_MS - BTN_ID_MODE_ENTER_START_MS)
#define BTN_PRESS_DFU_MODE_ENTER_TIME		\
	(BTN_DFU_MODE_ENTER_START_MS - BTN_RECOVERY_MODE_ENTER_START_MS)
#define BTN_PRESS_NOOP_LAST_TIME		\
	(BTN_NOOP_LAST_START_MS - BTN_DFU_MODE_ENTER_START_MS)

#define BTN_PRESS_MAX_TIME					\
	MAX_FROM_LIST(BTN_PRESS_FP_ADV_MODE_CHANGE_TIME,	\
		      BTN_PRESS_ID_MODE_ENTER_TIME,		\
		      BTN_PRESS_RECOVERY_MODE_ENTER_TIME,	\
		      BTN_PRESS_DFU_MODE_ENTER_TIME,		\
		      BTN_PRESS_NOOP_LAST_TIME)

#define BTN_RELEASE_DEF(_request, _start_ms, _ind_cnt, _ind_duration)	\
	{								\
		.request = _request,					\
		.start_ms = _start_ms,					\
		.action_ind = {						\
			.cnt = _ind_cnt,				\
			.duration = _ind_duration},			\
	}

/* Indication request encoding */
#define BTN_ACTION_IND_REQUEST_CNT_POS			0
#define BTN_ACTION_IND_REQUEST_CNT_MASK			0xFF
#define BTN_ACTION_IND_REQUEST_ON_MS_POS		8
#define BTN_ACTION_IND_REQUEST_ON_MS_MASK		0xFFF
#define BTN_ACTION_IND_REQUEST_OFF_MS_POS		20
#define BTN_ACTION_IND_REQUEST_OFF_MS_MASK		0xFFF

#define BTN_ACTION_IND_REQUEST_ENCODE(_cnt, _on_ms, _off_ms)					\
	((((_cnt) & BTN_ACTION_IND_REQUEST_CNT_MASK) << BTN_ACTION_IND_REQUEST_CNT_POS) |	\
	 (((_on_ms) & BTN_ACTION_IND_REQUEST_ON_MS_MASK) << BTN_ACTION_IND_REQUEST_ON_MS_POS) |	\
	 (((_off_ms) & BTN_ACTION_IND_REQUEST_OFF_MS_MASK) << BTN_ACTION_IND_REQUEST_OFF_MS_POS))

#define BTN_ACTION_IND_REQUEST_DECODE_CNT(req)		\
	(((req) >> BTN_ACTION_IND_REQUEST_CNT_POS) & BTN_ACTION_IND_REQUEST_CNT_MASK)
#define BTN_ACTION_IND_REQUEST_DECODE_ON_MS(req)	\
	(((req) >> BTN_ACTION_IND_REQUEST_ON_MS_POS) & BTN_ACTION_IND_REQUEST_ON_MS_MASK)
#define BTN_ACTION_IND_REQUEST_DECODE_OFF_MS(req)	\
	(((req) >> BTN_ACTION_IND_REQUEST_OFF_MS_POS) & BTN_ACTION_IND_REQUEST_OFF_MS_MASK)

#define IND_EVENT_BTN_ACTION		BIT(0)
#define IND_EVENT_RINGING_STATUS	BIT(1)
#define IND_EVENT_MASK			(IND_EVENT_BTN_ACTION | IND_EVENT_RINGING_STATUS)

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

/* Button action indicator pattern. */
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

/* Button release action. */
struct btn_release {
	/* UI request which will be sent. */
	enum app_ui_request request;

	/* Threshold from which this release action will be proceeded. */
	uint32_t start_ms;

	/* Indicator when the threshold will be reached. */
	struct btn_action_ind action_ind;
};

/* State to LED ID map. */
struct led_state_id_map {
	/* UI state on which the related LED should blink. */
	enum app_ui_state state;

	/* UI state related LED. */
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

/* Last button release action is a no-op used to indicate the end of the button press sequence. */
#define BTN_RELEASE_ACTION_COUNT	(ARRAY_SIZE(btn_release_actions) - 1)

static void btn_press_beep_work_handle(struct k_work *w);
static K_WORK_DELAYABLE_DEFINE(btn_press_beep_work, btn_press_beep_work_handle);

static ATOMIC_DEFINE(ui_state_status, APP_UI_STATE_COUNT);
static const struct led_state_id_map led_state_id_maps[] = {
	{.state = APP_UI_STATE_PROVISIONED,		.id = LED_PROVISIONED},
	{.state = APP_UI_STATE_ID_MODE,			.id = LED_ID_MODE},
	{.state = APP_UI_STATE_RECOVERY_MODE,		.id = LED_RECOVERY_MODE},
	{.state = APP_UI_STATE_FP_ADV,			.id = LED_FP_ADV},
	{.state = APP_UI_STATE_DFU_MODE,		.id = LED_DFU_MODE},
	{.state = APP_UI_STATE_MOTION_DETECTOR_ACTIVE,	.id = LED_MOTION_DETECTOR_ACTIVE},
};

static K_EVENT_DEFINE(ind_events);
static atomic_t btn_action_ind_request = ATOMIC_INIT(0);

static atomic_val_t btn_action_ind_request_encode(const struct ind_param *param)
{
	__ASSERT_NO_MSG(param);
	__ASSERT_NO_MSG(param->cnt <= BTN_ACTION_IND_REQUEST_CNT_MASK);
	__ASSERT_NO_MSG(param->on_ms <= BTN_ACTION_IND_REQUEST_ON_MS_MASK);
	__ASSERT_NO_MSG(param->off_ms <= BTN_ACTION_IND_REQUEST_OFF_MS_MASK);

	return BTN_ACTION_IND_REQUEST_ENCODE(param->cnt, param->on_ms, param->off_ms);
}

static struct ind_param btn_action_ind_request_decode(atomic_val_t req)
{
	struct ind_param param = {
		.cnt = BTN_ACTION_IND_REQUEST_DECODE_CNT(req),
		.on_ms = BTN_ACTION_IND_REQUEST_DECODE_ON_MS(req),
		.off_ms = BTN_ACTION_IND_REQUEST_DECODE_OFF_MS(req),
	};

	return param;
}

static void btn_action_ind_request_set(struct ind_param *param)
{
	__ASSERT_NO_MSG(param);

	atomic_set(&btn_action_ind_request, btn_action_ind_request_encode(param));
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
	struct ind_param param;

	switch (req->duration) {
	case BTN_ACTION_IND_DURATION_SHORT:
		param.on_ms = BTN_ACTION_IND_SHORT_ON_MS;
		param.off_ms = BTN_ACTION_IND_SHORT_OFF_MS;
		break;

	case BTN_ACTION_IND_DURATION_MID:
		param.on_ms = BTN_ACTION_IND_MID_ON_MS;
		param.off_ms = BTN_ACTION_IND_MID_OFF_MS;
		break;

	case BTN_ACTION_IND_DURATION_LONG:
		param.on_ms = BTN_ACTION_IND_LONG_ON_MS;
		param.off_ms = BTN_ACTION_IND_LONG_OFF_MS;
		break;

	case BTN_ACTION_IND_DURATION_COUNT:
		__ASSERT_NO_MSG(!req->cnt);
		/* Post the button event to the indication thread
		 * in order to stop other indications.
		 */
		k_event_post(&ind_events, IND_EVENT_BTN_ACTION);
		return;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	param.cnt = req->cnt;

	btn_action_ind_request_set(&param);
	k_event_post(&ind_events, IND_EVENT_BTN_ACTION);
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
			/* Schedule speaker notifications & btn_idx counting on each threshold. */
			k_work_reschedule(&btn_press_beep_work,
					  K_MSEC(btn_work_timeout_get(btn_idx + 1)));

			/* If ringing is active then always stop it on any button press. */
			if (atomic_test_bit(ui_state_status, APP_UI_STATE_RINGING)) {
				app_ui_request_broadcast(APP_UI_REQUEST_RINGING_STOP);
			}
		} else {
			bool post_event;

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

			post_event = (btn_idx < BTN_RELEASE_ACTION_COUNT);

			/* Action has been completed, consume. */
			btn_idx = -1;

			if (post_event) {
				k_event_post(&ind_events, IND_EVENT_BTN_ACTION);
			}
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

static void speaker_state_set(bool active)
{
	if (!IS_ENABLED(CONFIG_APP_UI_USE_HW_SPEAKER)) {
		return;
	}

	if (active) {
		app_ui_speaker_on();
	} else {
		app_ui_speaker_off();
	}
}

static uint32_t indication_sleep_interruptible(uint32_t ms, uint32_t abort_mask)
{
	__ASSERT_NO_MSG(abort_mask);

	if (ms == 0) {
		return 0;
	}

	return k_event_wait_safe(&ind_events, abort_mask, false, K_MSEC(ms));
}

static uint32_t led_blink_interruptible(uint8_t color, struct ind_param *param, uint32_t abort_mask)
{
	__ASSERT_NO_MSG(param);
	__ASSERT_NO_MSG(abort_mask);

	uint32_t events;

	for (uint8_t i = 0; i < param->cnt; i++) {
		led_drive(color, true);
		events = indication_sleep_interruptible(param->on_ms, abort_mask);
		if (events) {
			led_drive(color, false);
			return events;
		}

		led_drive(color, false);
		events = indication_sleep_interruptible(param->off_ms, abort_mask);
		if (events) {
			return events;
		}
	}
	return 0;
}

static uint32_t led_blink_once_interruptible(uint8_t color, uint32_t on_ms, uint32_t abort_mask)
{
	struct ind_param param = {
		.cnt = 1,
		.on_ms = on_ms,
		.off_ms = 0
	};

	return led_blink_interruptible(color, &param, abort_mask);
}

static uint32_t led_speaker_indicate_interruptible(uint8_t color,
						   struct ind_param *param,
						   uint32_t abort_mask)
{
	__ASSERT_NO_MSG(param);
	__ASSERT_NO_MSG(abort_mask);

	uint32_t events;

	for (uint8_t i = 0; i < param->cnt; i++) {
		speaker_state_set(true);
		led_drive(color, true);
		events = indication_sleep_interruptible(param->on_ms, abort_mask);
		if (events) {
			speaker_state_set(false);
			led_drive(color, false);
			return events;
		}

		speaker_state_set(false);
		led_drive(color, false);
		events = indication_sleep_interruptible(param->off_ms, abort_mask);
		if (events) {
			return events;
		}
	}

	return 0;
}

static void led_speaker_indicate(uint8_t color, struct ind_param *param)
{
	__ASSERT_NO_MSG(param);

	for (uint8_t i = 0; i < param->cnt; i++) {
		speaker_state_set(true);
		led_drive(color, true);
		k_msleep(param->on_ms);
		speaker_state_set(false);
		led_drive(color, false);
		k_msleep(param->off_ms);
	}
}

static void btn_ind_handle(struct ind_param *param)
{
	__ASSERT_NO_MSG(param);

	led_speaker_indicate(LED_INDICATOR_STATUS, param);
}

static uint32_t ringing_indication_handle(bool add_clearance_delay)
{
	struct ind_param param = {
		.cnt = LED_BLINK_STATE_RING_ON_OFF_CNT,
		.on_ms = LED_BLINK_STATE_RING_ON_OFF_MS,
		.off_ms = LED_BLINK_STATE_RING_ON_OFF_MS
	};
	uint32_t events;

	/* Add clearance delay to visually separate
	 * ringing indication from the previous LED state.
	 */
	if (add_clearance_delay) {
		events = indication_sleep_interruptible(IND_CLEARANCE_OFF_MS, IND_EVENT_MASK);
		if (events) {
			return events;
		}
	}

	return led_speaker_indicate_interruptible(LED_RINGING_STATUS, &param, IND_EVENT_MASK);
}

static uint32_t status_indication_handle(void)
{
	uint32_t events;

	for (uint8_t i = 0; i < ARRAY_SIZE(led_state_id_maps); i++) {
		if (atomic_test_bit(ui_state_status, led_state_id_maps[i].state)) {
			events = led_blink_once_interruptible(led_state_id_maps[i].id,
							      LED_BLINK_STATE_STATUS_ON_MS,
							      IND_EVENT_MASK);
			if (events) {
				return events;
			}
		}
	}

	return 0;
}

/* The button indication can not be interrupted by other indications.
 * The indication checks if there is a button action request and starts the indication if there is.
 * The release of the button allows to exit the indication.
 * If the button is held for too long (up to noop), the indication exits automatically.
 */
static uint32_t btn_indication_process(uint32_t events, bool *restart)
{
	if (events & IND_EVENT_BTN_ACTION) {
		atomic_val_t req = atomic_clear(&btn_action_ind_request);

		if (req) {
			struct ind_param param = btn_action_ind_request_decode(req);

			btn_ind_handle(&param);
		}
	}

	/* If button indication is in progress, then skip
	 * the next indication processing.
	 */
	if ((btn_idx != -1) && (btn_idx < BTN_RELEASE_ACTION_COUNT)) {
		*restart = true;
		return indication_sleep_interruptible(BTN_PRESS_MAX_TIME, IND_EVENT_BTN_ACTION);
	}

	return events;
}

/* The ringing indication can be interrupted by the button indication.
 * The indication checks if the ringing state is active and starts the indication if it is.
 * When the first ring starts, the clearance delay is added to visually separate
 * the ringing indication from the previous LED state.
 */
static uint32_t ringing_indication_process(uint32_t events, bool *restart)
{
	if (!atomic_test_bit(ui_state_status, APP_UI_STATE_RINGING)) {
		return events;
	}

	bool add_clearance_delay = ((events & IND_EVENT_RINGING_STATUS) != 0);

	events = ringing_indication_handle(add_clearance_delay);

	if ((events & IND_EVENT_BTN_ACTION) || !(events & IND_EVENT_RINGING_STATUS)) {
		*restart = true;
	}

	return events;
}

/* The status indication can be interrupted by the button or ringing indication.
 * The indication does not need special conditions to start.
 */
static uint32_t status_indication_process(uint32_t events, bool *restart)
{
	events = status_indication_handle();
	if (events) {
		*restart = true;
	}

	return events;
}

/* Process the indications in the following order:
 * 1. Button indication
 * 2. Ringing indication
 * 3. Status indication
 */
static void indication_thread_process(void)
{
	uint32_t (*const handlers[])(uint32_t, bool *) = {
		btn_indication_process,
		ringing_indication_process,
		status_indication_process,
	};

	uint32_t events = 0;

	while (1) {
		bool restart = false;

		for (uint8_t i = 0; i < ARRAY_SIZE(handlers); i++) {
			events = handlers[i](events, &restart);
			if (restart) {
				break;
			}
		}

		if (restart) {
			continue;
		}

		events = indication_sleep_interruptible(INDICATION_THREAD_INTERVAL_MS,
							IND_EVENT_MASK);
	}
}

K_THREAD_DEFINE(indication_thread_id, INDICATION_THREAD_STACK_SIZE, indication_thread_process,
		NULL, NULL, NULL, INDICATION_THREAD_PRIORITY, 0, 0);

static void cancel_ringing_work_handle(struct k_work *w)
{
	if (atomic_test_bit(ui_state_status, APP_UI_STATE_RINGING)) {
		app_ui_request_broadcast(APP_UI_REQUEST_RINGING_STOP);
	}
}

int app_ui_state_change_indicate(enum app_ui_state state, bool active)
{
	static K_WORK_DELAYABLE_DEFINE(cancel_ringing_work, cancel_ringing_work_handle);
	bool was_active;

	if (active) {
		was_active = atomic_test_and_set_bit(ui_state_status, state);
	} else {
		was_active = atomic_test_and_clear_bit(ui_state_status, state);
	}

	if (state == APP_UI_STATE_RINGING) {
		/* If there is any button action in progress, then send ringing stop request.
		 * A slight delay has been added as there is an issue with the Find Hub app that
		 * does not register sound completion after a very short (close to zero) timeout.
		 */
		if (active && (btn_idx != -1)) {
			k_work_reschedule(&cancel_ringing_work, K_MSEC(50));
		} else {
			if (was_active != active) {
				uint32_t prev_events = k_event_post(&ind_events,
								    IND_EVENT_RINGING_STATUS);
				if (prev_events & IND_EVENT_RINGING_STATUS) {
					LOG_WRN("Ringing status event was already posted.");
					k_event_clear(&ind_events, IND_EVENT_RINGING_STATUS);
				}
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
