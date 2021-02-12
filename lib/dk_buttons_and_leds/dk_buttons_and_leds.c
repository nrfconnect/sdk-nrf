/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <logging/log.h>
#include <nrfx.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(dk_buttons_and_leds, CONFIG_DK_LIBRARY_LOG_LEVEL);


struct gpio_pin {
	const char * const port;
	const uint8_t number;
};

static const struct gpio_pin button_pins[] = {
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	{DT_GPIO_LABEL(DT_ALIAS(sw0), gpios),
	 DT_GPIO_PIN(DT_ALIAS(sw0), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw1))
	{DT_GPIO_LABEL(DT_ALIAS(sw1), gpios),
	 DT_GPIO_PIN(DT_ALIAS(sw1), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw2))
	{DT_GPIO_LABEL(DT_ALIAS(sw2), gpios),
	 DT_GPIO_PIN(DT_ALIAS(sw2), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw3))
	{DT_GPIO_LABEL(DT_ALIAS(sw3), gpios),
	 DT_GPIO_PIN(DT_ALIAS(sw3), gpios)},
#endif
};

static const struct gpio_pin led_pins[] = {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
	{DT_GPIO_LABEL(DT_ALIAS(led0), gpios),
	 DT_GPIO_PIN(DT_ALIAS(led0), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led1))
	{DT_GPIO_LABEL(DT_ALIAS(led1), gpios),
	 DT_GPIO_PIN(DT_ALIAS(led1), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led2))
	{DT_GPIO_LABEL(DT_ALIAS(led2), gpios),
	 DT_GPIO_PIN(DT_ALIAS(led2), gpios)},
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led3))
	{DT_GPIO_LABEL(DT_ALIAS(led3), gpios),
	 DT_GPIO_PIN(DT_ALIAS(led3), gpios)},
#endif
};

enum state {
	STATE_WAITING,
	STATE_SCANNING,
};

static enum state state;
static struct k_delayed_work buttons_scan;
static button_handler_t button_handler_cb;
static atomic_t my_buttons;
static const struct device *button_devs[ARRAY_SIZE(button_pins)];
static const struct device *led_devs[ARRAY_SIZE(led_pins)];
static struct gpio_callback gpio_cb;
static struct k_spinlock lock;
static sys_slist_t button_handlers;
static struct k_mutex button_handler_mut;

static int callback_ctrl(bool enable)
{
	int err = 0;
	gpio_flags_t flags;

	/* This must be done with irqs disabled to avoid pin callback
	 * being fired before others are still not activated.
	 */
	for (size_t i = 0; (i < ARRAY_SIZE(button_pins)) && !err; i++) {
		if (enable) {
			flags = (IS_ENABLED(CONFIG_DK_LIBRARY_INVERT_BUTTONS) ?
				(GPIO_INT_LEVEL_LOW) : (GPIO_INT_LEVEL_HIGH));

			err = gpio_pin_interrupt_configure(button_devs[i],
				button_pins[i].number, flags);
		} else {
			err = gpio_pin_interrupt_configure(button_devs[i],
				button_pins[i].number, GPIO_INT_DISABLE);
		}
	}

	return err;
}

static uint32_t get_buttons(void)
{
	uint32_t ret = 0;
	for (size_t i = 0; i < ARRAY_SIZE(button_pins); i++) {
		int val;

		val = gpio_pin_get_raw(button_devs[i], button_pins[i].number);
		if (val < 0) {
			LOG_ERR("Cannot read gpio pin");
			return 0;
		}
		if ((val && !IS_ENABLED(CONFIG_DK_LIBRARY_INVERT_BUTTONS)) ||
		    (!val && IS_ENABLED(CONFIG_DK_LIBRARY_INVERT_BUTTONS))) {
			ret |= 1U << i;
		}
	}

	return ret;
}

static void button_handlers_call(uint32_t button_state, uint32_t has_changed)
{
	struct button_handler *handler;

	if (button_handler_cb != NULL) {
		button_handler_cb(button_state, has_changed);
	}

	if (IS_ENABLED(CONFIG_DK_LIBRARY_DYNAMIC_BUTTON_HANDLERS)) {
		k_mutex_lock(&button_handler_mut, K_FOREVER);
		SYS_SLIST_FOR_EACH_CONTAINER(&button_handlers, handler, node) {
			handler->cb(button_state, has_changed);
		}
		k_mutex_unlock(&button_handler_mut);
	}
}

static void buttons_scan_fn(struct k_work *work)
{
	static uint32_t last_button_scan;
	static bool initial_run = true;
	uint32_t button_scan;

	button_scan = get_buttons();
	atomic_set(&my_buttons, (atomic_val_t)button_scan);

	if (!initial_run) {
		if (button_scan != last_button_scan) {
			uint32_t has_changed = (button_scan ^ last_button_scan);

			button_handlers_call(button_scan, has_changed);
		}
	} else {
		initial_run = false;
	}

	last_button_scan = button_scan;

	if (button_scan != 0) {
		int err = k_delayed_work_submit(&buttons_scan,
		  K_MSEC(CONFIG_DK_LIBRARY_BUTTON_SCAN_INTERVAL));

		if (err) {
			LOG_ERR("Cannot add work to workqueue");
		}
	} else {
		/* If no button is pressed module can switch to callbacks */
		int err = 0;

		k_spinlock_key_t key = k_spin_lock(&lock);

		switch (state) {
		case STATE_SCANNING:
			state = STATE_WAITING;
			err = callback_ctrl(true);
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		k_spin_unlock(&lock, key);

		if (err) {
			LOG_ERR("Cannot enable callbacks");
		}
	}
}

int dk_leds_init(void)
{
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(led_pins); i++) {
		led_devs[i] = device_get_binding(led_pins[i].port);
		if (!led_devs[i]) {
			LOG_ERR("Cannot bind gpio device");
			return -ENODEV;
		}

		err = gpio_pin_configure(led_devs[i], led_pins[i].number,
					 GPIO_OUTPUT);
		if (err) {
			LOG_ERR("Cannot configure LED gpio");
			return err;
		}
	}

	return dk_set_leds_state(DK_NO_LEDS_MSK, DK_ALL_LEDS_MSK);
}

static void button_pressed(const struct device *gpio_dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	/* Disable GPIO interrupt */
	int err = callback_ctrl(false);

	if (err) {
		LOG_ERR("Cannot disable callbacks");
	}

	switch (state) {
	case STATE_WAITING:
		state = STATE_SCANNING;
		k_delayed_work_submit(&buttons_scan, K_MSEC(1));
		break;

	case STATE_SCANNING:
	default:
		/* Invalid state */
		__ASSERT_NO_MSG(false);
		break;
	}

	k_spin_unlock(&lock, key);
}

int dk_buttons_init(button_handler_t button_handler)
{
	int err;

	button_handler_cb = button_handler;

	gpio_flags_t flags = (IS_ENABLED(CONFIG_DK_LIBRARY_INVERT_BUTTONS) ?
			     GPIO_PULL_UP : GPIO_PULL_DOWN);

	if (IS_ENABLED(CONFIG_DK_LIBRARY_DYNAMIC_BUTTON_HANDLERS)) {
		k_mutex_init(&button_handler_mut);
	}

	for (size_t i = 0; i < ARRAY_SIZE(button_pins); i++) {
		button_devs[i] = device_get_binding(button_pins[i].port);
		if (!button_devs[i]) {
			LOG_ERR("Cannot bind gpio device");
			return -ENODEV;
		}

		err = gpio_pin_configure(button_devs[i], button_pins[i].number,
					GPIO_INPUT | flags);

		if (err) {
			LOG_ERR("Cannot configure button gpio");
			return err;
		}
	}

	uint32_t pin_mask = 0;

	for (size_t i = 0; i < ARRAY_SIZE(button_pins); i++) {
		/* Module starts in scanning mode and will switch to
		 * callback mode if no button is pressed.
		 */
		err = gpio_pin_interrupt_configure(button_devs[i],
			button_pins[i].number, GPIO_INT_DISABLE);
		if (err) {
			LOG_ERR("Cannot disable callbacks()");
			return err;
		}

		pin_mask |= BIT(button_pins[i].number);
	}

	gpio_init_callback(&gpio_cb, button_pressed, pin_mask);

	for (size_t i = 0; i < ARRAY_SIZE(button_pins); i++) {
		err = gpio_add_callback(button_devs[i], &gpio_cb);
		if (err) {
			LOG_ERR("Cannot add callback");
			return err;
		}
	}

	k_delayed_work_init(&buttons_scan, buttons_scan_fn);

	state = STATE_SCANNING;

	err = k_delayed_work_submit(&buttons_scan, K_NO_WAIT);
	if (err) {
		LOG_ERR("Cannot add work to workqueue");
		return err;
	}

	dk_read_buttons(NULL, NULL);

	return 0;
}

#ifdef CONFIG_DK_LIBRARY_DYNAMIC_BUTTON_HANDLERS
void dk_button_handler_add(struct button_handler *handler)
{
	k_mutex_lock(&button_handler_mut, K_FOREVER);
	sys_slist_append(&button_handlers, &handler->node);
	k_mutex_unlock(&button_handler_mut);
}

int dk_button_handler_remove(struct button_handler *handler)
{
	bool found;

	k_mutex_lock(&button_handler_mut, K_FOREVER);
	found = sys_slist_find_and_remove(&button_handlers, &handler->node);
	k_mutex_unlock(&button_handler_mut);

	return found ? 0 : -ENOENT;
}
#endif

void dk_read_buttons(uint32_t *button_state, uint32_t *has_changed)
{
	static uint32_t last_state;
	uint32_t current_state = atomic_get(&my_buttons);

	if (button_state != NULL) {
		*button_state = current_state;
	}

	if (has_changed != NULL) {
		*has_changed = (current_state ^ last_state);
	}

	last_state = current_state;
}

uint32_t dk_get_buttons(void)
{
	return atomic_get(&my_buttons);
}

int dk_set_leds(uint32_t leds)
{
	return dk_set_leds_state(leds, DK_ALL_LEDS_MSK);
}

int dk_set_leds_state(uint32_t leds_on_mask, uint32_t leds_off_mask)
{
	if ((leds_on_mask & ~DK_ALL_LEDS_MSK) != 0 ||
	   (leds_off_mask & ~DK_ALL_LEDS_MSK) != 0) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(led_pins); i++) {
		if ((BIT(i) & leds_on_mask) || (BIT(i) & leds_off_mask)) {
			uint32_t val = (BIT(i) & leds_on_mask) ? (1) : (0);

			if (IS_ENABLED(CONFIG_DK_LIBRARY_INVERT_LEDS)) {
				val = 1 - val;
			}

			int err = gpio_pin_set_raw(led_devs[i],
						   led_pins[i].number, val);
			if (err) {
				LOG_ERR("Cannot write LED gpio");
				return err;
			}
		}
	}

	return 0;
}

int dk_set_led(uint8_t led_idx, uint32_t val)
{
	int err;

	if (led_idx > ARRAY_SIZE(led_pins)) {
		LOG_ERR("LED index out of the range");
		return -EINVAL;
	}
	err = gpio_pin_set_raw(led_devs[led_idx], led_pins[led_idx].number,
		IS_ENABLED(CONFIG_DK_LIBRARY_INVERT_LEDS) ? !val : val);
	if (err) {
		LOG_ERR("Cannot write LED gpio");
	}
	return err;
}

int dk_set_led_on(uint8_t led_idx)
{
	return dk_set_led(led_idx, 1);
}

int dk_set_led_off(uint8_t led_idx)
{
	return dk_set_led(led_idx, 0);
}
