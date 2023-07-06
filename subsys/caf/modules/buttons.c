/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#include <caf/key_id.h>
#include <caf/gpio_pins.h>
#include CONFIG_CAF_BUTTONS_DEF_PATH

#include <app_event_manager.h>
#include <caf/events/button_event.h>
#include <caf/events/power_event.h>

#define MODULE buttons
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_BUTTONS_LOG_LEVEL);

#define SCAN_INTERVAL CONFIG_CAF_BUTTONS_SCAN_INTERVAL
#define DEBOUNCE_INTERVAL CONFIG_CAF_BUTTONS_DEBOUNCE_INTERVAL

/* For directly connected GPIO, scan rows once. */
#define COLUMNS MAX(ARRAY_SIZE(col), 1)

BUILD_ASSERT(ARRAY_SIZE(col) <= 32,
	     "Implementation uses uint32_t for bitmasks and supports up to 32 columns");
BUILD_ASSERT(ARRAY_SIZE(row) <= 32,
	     "Implementation uses uint32_t for bitmasks and supports up to 32 rows");

enum state {
	STATE_IDLE,
	STATE_ACTIVE,
	STATE_SCANNING,
	STATE_SUSPENDING
};

static const struct device * const gpio_devs[] = {
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio0)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio1)),
};
static struct gpio_callback gpio_cb[ARRAY_SIZE(gpio_devs)];
static struct k_work_delayable matrix_scan;
static struct k_work_delayable button_pressed;
static enum state state;


static void scan_fn(struct k_work *work);

static uint32_t get_wakeup_mask(const struct gpio_pin *pins, size_t cnt)
{
	uint32_t mask = BIT64_MASK(32);

	for (size_t i = 0; i < cnt; i++) {
		WRITE_BIT(mask, i, !pins[i].wakeup_blocked);
	}

	return mask;
}

static int set_cols(uint32_t mask)
{
	for (size_t i = 0; i < ARRAY_SIZE(col); i++) {
		uint32_t val = (mask & BIT(i)) ? (1) : (0);
		int err;

		if (val || !mask) {
			if (IS_ENABLED(CONFIG_CAF_BUTTONS_POLARITY_INVERSED)) {
				val = !val;
			}

			err = gpio_pin_configure(gpio_devs[col[i].port],
						 col[i].pin, GPIO_OUTPUT);
			if (!err) {
				err = gpio_pin_set_raw(gpio_devs[col[i].port],
						       col[i].pin, val);
			}
		} else {
			gpio_flags_t flags = GPIO_INPUT;
			gpio_flags_t pull = (IS_ENABLED(CONFIG_CAF_BUTTONS_POLARITY_INVERSED) ?
					    (GPIO_PULL_UP) : (GPIO_PULL_DOWN));

			/* The pull is necessary to ensure pin state and prevent unexpected
			 * behaviour that could be triggered by accumulating charge.
			 */
			flags |= pull;

			err = gpio_pin_configure(gpio_devs[col[i].port], col[i].pin, flags);
		}

		if (err) {
			LOG_ERR("Cannot set pin");
			return -EFAULT;
		}
	}

	return 0;
}

static int get_rows(uint32_t *mask)
{
	for (size_t i = 0; i < ARRAY_SIZE(row); i++) {
		int val = gpio_pin_get_raw(gpio_devs[row[i].port], row[i].pin);

		if (val < 0) {
			LOG_ERR("Cannot get pin");
			return -EFAULT;
		}

		if (IS_ENABLED(CONFIG_CAF_BUTTONS_POLARITY_INVERSED)) {
			val = !val;
		}

		*mask |= (val << i);
	}

	return 0;
}

static int set_trig_mode(void)
{
	gpio_flags_t flags = (IS_ENABLED(CONFIG_CAF_BUTTONS_POLARITY_INVERSED) ?
			      (GPIO_PULL_UP) : (GPIO_PULL_DOWN));
	flags |= GPIO_INPUT;

	int err = 0;

	for (size_t i = 0; (i < ARRAY_SIZE(row)) && !err; i++) {
		__ASSERT_NO_MSG(row[i].port < ARRAY_SIZE(gpio_devs));
		__ASSERT_NO_MSG(gpio_devs[row[i].port] != NULL);

		err = gpio_pin_configure(gpio_devs[row[i].port], row[i].pin,
					 flags);
	}

	return err;
}

static int callback_ctrl(uint32_t enable_mask)
{
	int err = 0;

	/* Normally this should be done with irqs disabled to avoid pin callback
	 * being fired before others are still not activated.
	 * Since we defer the handling code to work we can however assume
	 * cancel executed after callbacks maintenance will keep things safe.
	 *
	 * Note that this code MUST be executed from a system workqueue context.
	 *
	 * We are also safe to access state without a lock as long as it is
	 * done only from system workqueue context.
	 */

	for (size_t i = 0; (i < ARRAY_SIZE(row)) && !err; i++) {
		if (enable_mask & BIT(i)) {
			/* Level interrupt is needed to leave deep sleep mode.
			 * Edge interrupt gives only 7 channels. It is not
			 * suitable for larger matrix/number of GPIO buttons.
			 */
			gpio_flags_t flag_irq = (IS_ENABLED(CONFIG_CAF_BUTTONS_POLARITY_INVERSED) ?
						(GPIO_INT_LEVEL_LOW) : (GPIO_INT_LEVEL_HIGH));

			err = gpio_pin_interrupt_configure(gpio_devs[row[i].port],
							   row[i].pin,
							   flag_irq);
		} else {
			err = gpio_pin_interrupt_configure(gpio_devs[row[i].port],
							   row[i].pin,
							   GPIO_INT_DISABLE);
		}
	}
	if (!enable_mask) {
		/* Callbacks are disabled but they could fire in the meantime.
		 * Make sure pending work is canceled.
		 */
		k_work_cancel_delayable(&button_pressed);
	}

	return err;
}

static int setup_pin_wakeup(void)
{
	uint32_t wakeup_cols = get_wakeup_mask(col, ARRAY_SIZE(col));
	uint32_t wakeup_rows = get_wakeup_mask(row, ARRAY_SIZE(row));

	/* Disable callbacks (and cancel the work) to ensure it will not be scheduled by an
	 * invalid button in the idle state.
	 */
	int err = callback_ctrl(0);

	if (!err) {
		/* Setup callbacks and columns for the idle state. */
		err = set_cols(wakeup_cols);
		if (!err) {
			err = callback_ctrl(wakeup_rows);
		}
	}

	return err;
}

static int suspend(void)
{
	int err = -EBUSY;

	switch (state) {
	case STATE_SCANNING:
		state = STATE_SUSPENDING;
		break;

	case STATE_SUSPENDING:
		/* Waiting for scanning to stop */
		break;

	case STATE_ACTIVE:
		err = setup_pin_wakeup();
		if (!err) {
			state = STATE_IDLE;
		}

		break;

	case STATE_IDLE:
		err = -EALREADY;
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	return err;
}

static void resume(void)
{
	if (state == STATE_SUSPENDING) {
		state = STATE_SCANNING;
		return;
	}

	if (state != STATE_IDLE) {
		/* Already activated. */
		return;
	}

	int err = callback_ctrl(0);

	if (err) {
		LOG_ERR("Cannot disable callbacks");
		module_set_state(MODULE_STATE_ERROR);
	} else {
		state = STATE_SCANNING;
		scan_fn(NULL);

		module_set_state(MODULE_STATE_READY);
	}
}

static void scan_fn(struct k_work *work)
{
	/* Validate state */
	__ASSERT_NO_MSG((state == STATE_SCANNING) ||
			(state == STATE_SUSPENDING));

	/* Get current state */
	uint32_t raw_state[COLUMNS];
	memset(raw_state, 0, sizeof(raw_state));

	for (size_t i = 0; i < COLUMNS; i++) {
		int err = set_cols(BIT(i));

		if (!err) {
			err = get_rows(&raw_state[i]);
		}

		if (err) {
			LOG_ERR("Cannot scan matrix");
			goto error;
		}
	}

	/* Avoid draining current between scans */
	if (set_cols(0)) {
		LOG_ERR("Cannot set neutral state");
		goto error;
	}

	static uint32_t settled_state[COLUMNS];

	/* Prevent bouncing */
	static uint32_t prev_state[COLUMNS];
	for (size_t i = 0; i < COLUMNS; i++) {
		uint32_t bounce_mask = prev_state[i] ^ raw_state[i];
		prev_state[i] = raw_state[i];
		raw_state[i] &= ~bounce_mask;
		raw_state[i] |= settled_state[i] & bounce_mask;
	}

	/* Prevent ghosting */
	uint32_t cur_state[COLUMNS];
	for (size_t i = 0; i < COLUMNS; i++) {
		uint32_t blocking_mask = 0;
		for (size_t j = 0; j < COLUMNS; j++) {
			if (i == j) {
				continue;
			}
			blocking_mask |= raw_state[j];
		}
		blocking_mask ^= raw_state[i];
		cur_state[i] = raw_state[i];
		if (!is_power_of_two(raw_state[i])) {
			/* Power of two means only one bit is set */
			cur_state[i] &= blocking_mask;
		}
	}

	/* Emit event for any key state change */
	bool any_pressed = false;
	size_t evt_limit = 0;

	for (size_t i = 0; i < COLUMNS; i++) {
		for (size_t j = 0; j < ARRAY_SIZE(row); j++) {
			bool is_raw_pressed = raw_state[i] & BIT(j);
			bool is_pressed = cur_state[i] & BIT(j);
			bool was_pressed = settled_state[i] & BIT(j);

			if ((is_pressed != was_pressed) &&
			    (is_pressed == is_raw_pressed) &&
			    (evt_limit < CONFIG_CAF_BUTTONS_EVENT_LIMIT)) {
				struct button_event *event = new_button_event();

				event->key_id = KEY_ID(i, j);
				event->pressed = is_pressed;
				APP_EVENT_SUBMIT(event);

				evt_limit++;

				WRITE_BIT(settled_state[i], j, is_pressed);
			}
		}

		any_pressed = any_pressed ||
			      (prev_state[i] != 0) ||
			      (settled_state[i] != 0) ||
			      (cur_state[i] != 0);
	}

	if (any_pressed) {
		/* Schedule next scan */
		k_work_reschedule(&matrix_scan, K_MSEC(SCAN_INTERVAL));
	} else {
		/* If no button is pressed module can switch to callbacks */

		int err = 0;

		/* Enable callbacks and switch state, then set pins */
		switch (state) {
		case STATE_SCANNING:
			state = STATE_ACTIVE;
			err = callback_ctrl(BIT64_MASK(32));

			if (!err) {
				err = set_cols(BIT64_MASK(32));
			}
			break;

		case STATE_SUSPENDING:
			state = STATE_ACTIVE;
			err = suspend();
			if (!err) {
				module_set_state(MODULE_STATE_STANDBY);
			}
			__ASSERT_NO_MSG((err != -EBUSY) && (err != -EALREADY));
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		if (err) {
			LOG_ERR("Cannot enable callbacks or set neutral state");
			goto error;
		}

	}

	return;

error:
	module_set_state(MODULE_STATE_ERROR);
}

static void button_pressed_isr(const struct device *gpio_dev,
			       struct gpio_callback *cb,
			       uint32_t pins)
{
	int err = 0;

	/* Scanning will be scheduled, switch off pins */
	if (set_cols(0)) {
		LOG_ERR("Cannot control pins");
		err = -EFAULT;
	}

	/* This is a workaround. Zephyr will set any pin triggering interrupt
	 * at the moment. Not only our pins.
	 */
	pins = pins & cb->pin_mask;

	/* Disable all interrupts synchronously requires holding a spinlock.
	 * The problem is that GPIO callback disable code takes time. If lock
	 * is kept during this operation BLE stack can fail in some cases.
	 * Instead we disable callbacks associated with the pins. This is to
	 * make sure CPU is available for threads. The remaining callbacks are
	 * disabled in the workqueue thread context. Work code also cancels
	 * itself to prevent double execution when interrupt for another
	 * pin was triggered in-between.
	 */
	for (uint32_t pin = 0; (pins != 0) && !err; pins >>= 1, pin++) {
		if ((pins & 1) != 0) {
			err = gpio_pin_interrupt_configure(gpio_dev, pin,
							   GPIO_INT_DISABLE);
		}
	}

	if (err) {
		LOG_ERR("Cannot disable callbacks");
		module_set_state(MODULE_STATE_ERROR);
	} else {
		k_work_reschedule(&button_pressed, K_NO_WAIT);
	}
}

static void button_pressed_fn(struct k_work *work)
{
	int err = callback_ctrl(0);

	if (err) {
		LOG_ERR("Cannot disable callbacks");
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	switch (state) {
	case STATE_IDLE:
		if (IS_ENABLED(CONFIG_CAF_BUTTONS_PM_EVENTS)) {
			APP_EVENT_SUBMIT(new_wake_up_event());
		}
		break;

	case STATE_ACTIVE:
		state = STATE_SCANNING;
		k_work_reschedule(&matrix_scan, K_MSEC(DEBOUNCE_INTERVAL));
		break;

	case STATE_SCANNING:
	case STATE_SUSPENDING:
	default:
		/* Invalid state */
		__ASSERT_NO_MSG(false);
		break;
	}
}

static void init_fn(void)
{
	/* Setup GPIO configuration */
	for (size_t i = 0; i < ARRAY_SIZE(gpio_devs); i++) {
		if (!gpio_devs[i]) {
			/* Skip non-existing ports */
			continue;
		}
		if (!device_is_ready(gpio_devs[i])) {
			LOG_ERR("GPIO device not ready");
			goto error;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(col); i++) {
		__ASSERT_NO_MSG(col[i].port < ARRAY_SIZE(gpio_devs));
		__ASSERT_NO_MSG(gpio_devs[col[i].port] != NULL);

		int err = gpio_pin_configure(gpio_devs[col[i].port],
					     col[i].pin, GPIO_INPUT);

		if (err) {
			LOG_ERR("Cannot configure cols");
			goto error;
		}
	}

	int err = set_trig_mode();
	if (err) {
		LOG_ERR("Cannot set interrupt mode");
		goto error;
	}

	uint32_t pin_mask[ARRAY_SIZE(gpio_devs)] = {0};
	for (size_t i = 0; i < ARRAY_SIZE(row); i++) {
		/* Module starts in scanning mode and will switch to
		 * callback mode if no button is pressed.
		 */
		err = gpio_pin_interrupt_configure(gpio_devs[row[i].port],
						   row[i].pin,
						   GPIO_INT_DISABLE);
		if (err) {
			LOG_ERR("Cannot configure rows");
			goto error;
		}

		pin_mask[row[i].port] |= BIT(row[i].pin);
	}

	for (size_t i = 0; i < ARRAY_SIZE(gpio_devs); i++) {
		if (!gpio_devs[i]) {
			/* Skip non-existing ports */
			continue;
		}
		gpio_init_callback(&gpio_cb[i], button_pressed_isr, pin_mask[i]);
		err = gpio_add_callback(gpio_devs[i], &gpio_cb[i]);
		if (err) {
			LOG_ERR("Cannot add callback");
			goto error;
		}
	}

	module_set_state(MODULE_STATE_READY);

	/* Perform initial scan */
	state = STATE_SCANNING;

	scan_fn(NULL);

	return;

error:
	module_set_state(MODULE_STATE_ERROR);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			k_work_init_delayable(&matrix_scan, scan_fn);
			k_work_init_delayable(&button_pressed, button_pressed_fn);

			init_fn();

			return false;
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_BUTTONS_PM_EVENTS) &&
	    is_wake_up_event(aeh)) {
		resume();

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_BUTTONS_PM_EVENTS) &&
	    is_power_down_event(aeh)) {
		int err = suspend();

		if (!err) {
			module_set_state(MODULE_STATE_STANDBY);
			return false;
		} else if (err == -EALREADY) {
			/* Already suspended */
			return false;
		} else if (err == -EBUSY) {
			/* Cannot suspend while scanning */
			return true;
		}

		LOG_ERR("Error while suspending");
		module_set_state(MODULE_STATE_ERROR);
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_CAF_BUTTONS_PM_EVENTS
APP_EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif
