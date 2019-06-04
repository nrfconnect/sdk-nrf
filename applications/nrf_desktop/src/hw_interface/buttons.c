/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <kernel.h>
#include <spinlock.h>
#include <soc.h>
#include <device.h>
#include <gpio.h>
#include <misc/util.h>

#include "key_id.h"
#include "buttons.h"
#include "buttons_def.h"

#include "event_manager.h"
#include "button_event.h"
#include "power_event.h"

#define MODULE buttons
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BUTTONS_LOG_LEVEL);

#define SCAN_INTERVAL CONFIG_DESKTOP_BUTTONS_SCAN_INTERVAL

/* For directly connected GPIO, scan rows once. */
#define COLUMNS MAX(ARRAY_SIZE(col), 1)

enum state {
	STATE_IDLE,
	STATE_ACTIVE,
	STATE_SCANNING,
	STATE_SUSPENDING
};

static struct device *gpio_devs[ARRAY_SIZE(port_map)];
static struct gpio_callback gpio_cb[ARRAY_SIZE(port_map)];
static struct k_delayed_work matrix_scan;
static enum state state;
static struct k_spinlock lock;


static void scan_fn(struct k_work *work);


static int set_cols(u32_t mask)
{
	for (size_t i = 0; i < ARRAY_SIZE(col); i++) {
		u32_t val = (mask & BIT(i)) ? (1) : (0);
		int err;

		if (val) {
			if (IS_ENABLED(CONFIG_DESKTOP_BUTTONS_POLARITY_INVERSED)) {
				val = !val;
			}

			err = gpio_pin_configure(gpio_devs[col[i].port],
						 col[i].pin, GPIO_DIR_OUT);
			if (!err) {
				err = gpio_pin_write(gpio_devs[col[i].port],
						     col[i].pin, val);
			}
		} else {
			err = gpio_pin_configure(gpio_devs[col[i].port],
						 col[i].pin, GPIO_DIR_IN);
		}

		if (err) {
			LOG_ERR("Cannot set pin");
			return -EFAULT;
		}
	}

	return 0;
}

static int get_rows(u32_t *mask)
{
	for (size_t i = 0; i < ARRAY_SIZE(row); i++) {
		u32_t val;

		if (gpio_pin_read(gpio_devs[row[i].port], row[i].pin, &val)) {
			LOG_ERR("Cannot get pin");
			return -EFAULT;
		}

		if (IS_ENABLED(CONFIG_DESKTOP_BUTTONS_POLARITY_INVERSED)) {
			val = !val;
		}

		*mask |= (val << i);
	}

	return 0;
}

static int set_trig_mode(int trig_mode)
{
	__ASSERT_NO_MSG((trig_mode == GPIO_INT_EDGE) ||
			(trig_mode == GPIO_INT_LEVEL));

	int flags = (IS_ENABLED(CONFIG_DESKTOP_BUTTONS_POLARITY_INVERSED) ?
		(GPIO_PUD_PULL_UP | GPIO_INT_ACTIVE_LOW) :
		(GPIO_PUD_PULL_DOWN | GPIO_INT_ACTIVE_HIGH));
	flags |= (GPIO_DIR_IN | GPIO_INT | trig_mode);

	int err = 0;

	for (size_t i = 0; (i < ARRAY_SIZE(row)) && !err; i++) {
		__ASSERT_NO_MSG(row[i].port < ARRAY_SIZE(port_map));

		err = gpio_pin_configure(gpio_devs[row[i].port], row[i].pin,
					 flags);
	}

	return err;
}

static int callback_ctrl(bool enable)
{
	int err = 0;

	/* This must be done with irqs disabled to avoid pin callback
	 * being fired before others are still not activated.
	 */

	u32_t pin_mask[ARRAY_SIZE(port_map)] = {0};

	for (size_t i = 0; (i < ARRAY_SIZE(row)) && !err; i++) {
		if (enable) {
			pin_mask[row[i].port] |= BIT(row[i].pin);
			err = gpio_pin_enable_callback(gpio_devs[row[i].port],
						       row[i].pin);
		} else {
			err = gpio_pin_disable_callback(gpio_devs[row[i].port],
							row[i].pin);
		}
	}

	/* Below code is workaround for Zephyr not allowing to disable
	 * interrupt already pending for fire.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(port_map); i++) {
		gpio_cb[i].pin_mask = pin_mask[i];
	}

	return err;
}

static int suspend_nolock(void)
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
		state = STATE_IDLE;

		/* Leaving deep sleep requires level interrupt */
		err = callback_ctrl(true);
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

static int suspend(void)
{
	int err;

	k_spinlock_key_t key = k_spin_lock(&lock);
	err = suspend_nolock();
	k_spin_unlock(&lock, key);

	return err;
}

static void resume(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	if (state != STATE_IDLE) {
		/* Already activated. */
		k_spin_unlock(&lock, key);
		return;
	}

	int err = callback_ctrl(false);
	if (err) {
		LOG_ERR("Cannot disable callbacks");
	} else {
		state = STATE_SCANNING;
	}

	/* GPIO callback is disabled - it is safe to unlock */
	k_spin_unlock(&lock, key);

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	} else {
		scan_fn(NULL);

		module_set_state(MODULE_STATE_READY);
	}
}

static void scan_fn(struct k_work *work)
{
	if (IS_ENABLED(CONFIG_ASSERT)) {
		/* Validate state */
		k_spinlock_key_t key = k_spin_lock(&lock);
		__ASSERT_NO_MSG((state == STATE_SCANNING) ||
				(state == STATE_SUSPENDING));
		k_spin_unlock(&lock, key);
	}

	/* Get current state */
	u32_t raw_state[COLUMNS];
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

	/* Prevent ghosting */
	u32_t cur_state[COLUMNS];
	for (size_t i = 0; i < COLUMNS; i++) {
		u32_t blocking_mask = 0;
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
	static u32_t old_state[COLUMNS];
	bool any_pressed = false;
	size_t evt_limit = 0;

	for (size_t i = 0; i < COLUMNS; i++) {
		for (size_t j = 0; j < ARRAY_SIZE(row); j++) {
			bool is_pressed = cur_state[i] & BIT(j);
			bool was_pressed = old_state[i] & BIT(j);

			if ((is_pressed != was_pressed) &&
			    (evt_limit < CONFIG_DESKTOP_BUTTONS_EVENT_LIMIT)) {
				struct button_event *event = new_button_event();

				event->key_id = KEY_ID(i, j);
				event->pressed = is_pressed;
				EVENT_SUBMIT(event);

				evt_limit++;

				WRITE_BIT(old_state[i], j, is_pressed);
			}
		}

		any_pressed = any_pressed || (old_state[i] != 0) || (cur_state[i] != 0);
	}

	if (any_pressed) {
		/* Avoid draining current between scans */
		if (set_cols(0x00000000)) {
			LOG_ERR("Cannot set neutral state");
			goto error;
		}

		/* Schedule next scan */
		k_delayed_work_submit(&matrix_scan, SCAN_INTERVAL);
	} else {
		/* If no button is pressed module can switch to callbacks */

		/* Prepare to wait for a callback */
		if (set_cols(0xFFFFFFFF)) {
			LOG_ERR("Cannot set neutral state");
			goto error;
		}

		/* Make sure that mode is set before callbacks are enabled */
		int err = 0;

		k_spinlock_key_t key = k_spin_lock(&lock);
		switch (state) {
		case STATE_SCANNING:
			state = STATE_ACTIVE;
			err = callback_ctrl(true);
			break;

		case STATE_SUSPENDING:
			state = STATE_ACTIVE;
			err = suspend_nolock();
			if (!err) {
				module_set_state(MODULE_STATE_STANDBY);
			}
			__ASSERT_NO_MSG((err != -EBUSY) && (err != -EALREADY));
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}
		k_spin_unlock(&lock, key);

		if (err) {
			LOG_ERR("Cannot enable callbacks");
			goto error;
		}
	}

	return;

error:
	module_set_state(MODULE_STATE_ERROR);
}

void button_pressed(struct device *gpio_dev, struct gpio_callback *cb,
		    u32_t pins)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	int err = callback_ctrl(false);
	if (err) {
		LOG_ERR("Cannot disable callbacks");
	}

	switch (state) {
	case STATE_IDLE:;
		struct wake_up_event *event = new_wake_up_event();
		EVENT_SUBMIT(event);
		break;

	case STATE_ACTIVE:
		state = STATE_SCANNING;
		k_delayed_work_submit(&matrix_scan,
				      CONFIG_DESKTOP_BUTTONS_DEBOUNCE_INTERVAL);
		break;

	case STATE_SCANNING:
	case STATE_SUSPENDING:
	default:
		/* Invalid state */
		__ASSERT_NO_MSG(false);
		break;
	}

	k_spin_unlock(&lock, key);
}

static void init_fn(void)
{
	/* Setup GPIO configuration */
	for (size_t i = 0; i < ARRAY_SIZE(port_map); i++) {
		gpio_devs[i] = device_get_binding(port_map[i]);
		if (!gpio_devs[i]) {
			LOG_ERR("Cannot get GPIO device binding");
			goto error;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(col); i++) {
		__ASSERT_NO_MSG(col[i].port < ARRAY_SIZE(port_map));

		int err = gpio_pin_configure(gpio_devs[col[i].port],
					     col[i].pin, GPIO_DIR_IN);

		if (err) {
			LOG_ERR("Cannot configure cols");
			goto error;
		}
	}

	/* Level interrupt is needed to leave deep sleep mode.
	 * Edge interrupt gives only 7 channels so is not suitable
	 * for larger matrix or number of GPIO buttons.
	 */
	int err = set_trig_mode(GPIO_INT_LEVEL);
	if (err) {
		LOG_ERR("Cannot set interrupt mode");
		goto error;
	}

	u32_t pin_mask[ARRAY_SIZE(port_map)] = {0};
	for (size_t i = 0; i < ARRAY_SIZE(row); i++) {
		/* Module starts in scanning mode and will switch to
		 * callback mode if no button is pressed.
		 */
		err = gpio_pin_disable_callback(gpio_devs[row[i].port],
						row[i].pin);
		if (err) {
			LOG_ERR("Cannot configure rows");
			goto error;
		}

		pin_mask[row[i].port] |= BIT(row[i].pin);
	}

	for (size_t i = 0; i < ARRAY_SIZE(port_map); i++) {
		gpio_init_callback(&gpio_cb[i], button_pressed, pin_mask[i]);
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

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			k_delayed_work_init(&matrix_scan, scan_fn);

			init_fn();

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		resume();

		return false;
	}

	if (is_power_down_event(eh)) {
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
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
