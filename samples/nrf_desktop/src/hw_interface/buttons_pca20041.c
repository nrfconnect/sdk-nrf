/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <device.h>
#include <gpio.h>

#include "event_manager.h"
#include "button_event.h"
#include "power_event.h"

#define MODULE buttons
#include "module_state_event.h"

#include <logging/log.h>
#define LOG_LEVEL CONFIG_DESKTOP_LOG_BUTTONS_MODULE_LEVEL
LOG_MODULE_REGISTER(MODULE);


#define SCAN_INTERVAL 50 /* ms */


static const u8_t col_pin[] = { 2, 21, 20, 19};
static const u8_t row_pin[] = {29, 31, 22, 24};

static struct device *gpio_dev;
static struct gpio_callback gpio_cbs[ARRAY_SIZE(row_pin)];
static struct k_delayed_work matrix_scan;
static atomic_t scanning;
static atomic_t active;


static int set_cols(u32_t mask)
{
	for (size_t i = 0; i < ARRAY_SIZE(col_pin); i++) {
		u32_t val = (mask & (1 << i)) ? (1) : (0);

		if (gpio_pin_write(gpio_dev, col_pin[i], val)) {
			LOG_ERR("cannot set pin");

			return -EFAULT;
		}
	}

	return 0;
}

static int get_rows(u32_t *mask)
{
	for (size_t i = 0; i < ARRAY_SIZE(col_pin); i++) {
		u32_t val;

		if (gpio_pin_read(gpio_dev, row_pin[i], &val)) {
			LOG_ERR("cannot get pin");
			return -EFAULT;
		}

		(*mask) |= (val << i);
	}

	return 0;
}

static int set_trig_mode(int trig_mode)
{
	__ASSERT_NO_MSG((trig_mode == GPIO_INT_EDGE) ||
			(trig_mode == GPIO_INT_LEVEL));

	int flags = GPIO_PUD_PULL_DOWN | GPIO_DIR_IN | GPIO_INT |
		    GPIO_INT_ACTIVE_HIGH;
	flags |= trig_mode;

	int err = 0;

	for (size_t i = 0; (i < ARRAY_SIZE(row_pin)) && !err; i++) {
		err = gpio_pin_configure(gpio_dev, row_pin[i], flags);
	}

	return err;
}

static int callback_ctrl(bool enable)
{
	int err = 0;

	/* This must be done with irqs disabled to avoid pin callback
	 * being fired before others are still not activated.
	 */
	unsigned int flags = irq_lock();
	for (size_t i = 0; (i < ARRAY_SIZE(row_pin)) && !err; i++) {
		if (enable) {
			err = gpio_pin_enable_callback(gpio_dev, row_pin[i]);
		} else {
			err = gpio_pin_disable_callback(gpio_dev, row_pin[i]);
		}
	}
	irq_unlock(flags);

	return err;
}

static void matrix_scan_fn(struct k_work *work)
{
	/* Get current state */
	u32_t cur_state[ARRAY_SIZE(col_pin)] = {0};

	for (size_t i = 0; i < ARRAY_SIZE(col_pin); i++) {

		int err = set_cols(1 << i);

		if (!err) {
			err = get_rows(&cur_state[i]);
		}

		if (err) {
			LOG_ERR("cannot scan matrix");
			goto error;
		}
	}

	/* Emit event for any key state change */

	static u32_t old_state[ARRAY_SIZE(col_pin)];
	bool any_pressed = false;

	for (size_t i = 0; i < ARRAY_SIZE(col_pin); i++) {
		for (size_t j = 0; j < ARRAY_SIZE(row_pin); j++) {
			bool is_pressed = (cur_state[i] & (1 << j));
			bool was_pressed = (old_state[i] & (1 << j));

			if (is_pressed != was_pressed) {
				struct button_event *event = new_button_event();

				event->key_id = (i << 8) | (j & 0xFF);
				event->pressed = is_pressed;
				EVENT_SUBMIT(event);
			}

			any_pressed = any_pressed || is_pressed;
		}
	}

	memcpy(old_state, cur_state, sizeof(old_state));

	if (atomic_get(&scanning) && any_pressed) {
		/* Avoid draining current between scans */
		if (set_cols(0x00)) {
			LOG_ERR("cannot set neutral state");
			goto error;
		}

		/* Schedule next scan */
		k_delayed_work_submit(&matrix_scan, SCAN_INTERVAL);
	} else {
		/* If no button is pressed module can switch to callbacks */

		/* Prepare to wait for a callback */
		if (set_cols(0xFF)) {
			LOG_ERR("cannot set neutral state");
			goto error;
		}

		/* Make sure that mode is set before callbacks are enabled */
		atomic_set(&scanning, false);

		int err = callback_ctrl(true);
		if (err) {
			LOG_ERR("cannot enable callbacks");
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
	/* Disable GPIO interrupt */
	for (size_t i = 0; i < ARRAY_SIZE(row_pin); i++) {
		int err = gpio_pin_disable_callback(gpio_dev, row_pin[i]);
		if (err) {
			LOG_ERR("cannot disable callbacks");
		}
	}

	if (!atomic_get(&active)) {
		struct wake_up_event *event = new_wake_up_event();
		EVENT_SUBMIT(event);

		return;
	}

	/* Activate periodic scan */
	__ASSERT_NO_MSG(!atomic_get(&scanning));
	atomic_set(&scanning, true);
	k_delayed_work_submit(&matrix_scan, 0);
}

static void init_fn(void)
{
	/* Setup GPIO configuration */
	gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!gpio_dev) {
		LOG_ERR("cannot get GPIO device binding");
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(col_pin); i++) {
		int err = gpio_pin_configure(gpio_dev, col_pin[i],
				GPIO_DIR_OUT);

		if (err) {
			LOG_ERR("cannot configure cols");
			goto error;
		}
	}

	int err = set_trig_mode(GPIO_INT_EDGE);
	if (err) {
		LOG_ERR("cannot set interrupt mode");
		goto error;
	}

	for (size_t i = 0; i < ARRAY_SIZE(row_pin); i++) {
		gpio_init_callback(&gpio_cbs[i], button_pressed,
				   BIT(row_pin[i]));
		err = gpio_add_callback(gpio_dev, &gpio_cbs[i]);

		if (!err) {
			/* Module starts in scanning mode and will switch to
			 * callback mode if no button is pressed.
			 */
			err = gpio_pin_disable_callback(gpio_dev, row_pin[i]);
		}

		if (err) {
			LOG_ERR("cannot configure rows");
			goto error;
		}
	}

	module_set_state(MODULE_STATE_READY);

	/* Perform initial scan */
	atomic_set(&scanning, true);
	matrix_scan_fn(NULL);

	return;

error:
	module_set_state(MODULE_STATE_ERROR);
}

static void term_fn(void)
{
	module_set_state(MODULE_STATE_STANDBY);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			k_delayed_work_init(&matrix_scan, matrix_scan_fn);

			init_fn();
			atomic_set(&active, true);

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!atomic_get(&active)) {
			int err = callback_ctrl(false);
			if (err) {
				LOG_ERR("cannot disable callbacks");
			} else {
				err = set_trig_mode(GPIO_INT_EDGE);
				if (err) {
					LOG_ERR("cannot set trig mode");
					module_set_state(MODULE_STATE_ERROR);
				} else {
					module_set_state(MODULE_STATE_READY);
					atomic_set(&scanning, true);
					atomic_set(&active, true);
					matrix_scan_fn(NULL);
				}
			}
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		atomic_set(&active, false);

		bool scn = atomic_get(&scanning);
		if (!scn) {
			term_fn();
		}

		/* Leaving deep sleep requires level interrupt */
		int err = set_trig_mode(GPIO_INT_LEVEL);
		if (err) {
			LOG_ERR("Cannot configure an interrupt");
			module_set_state(MODULE_STATE_ERROR);
		}

		return scn;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
