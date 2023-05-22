/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <nrfx.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(dk_buttons_and_leds, CONFIG_DK_LIBRARY_LOG_LEVEL);

#define BUTTONS_NODE DT_PATH(buttons)
#define LEDS_NODE DT_PATH(leds)

#define GPIO0_DEV DEVICE_DT_GET(DT_NODELABEL(gpio0))
#define GPIO1_DEV DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio1))

/* GPIO0, GPIO1 and GPIO expander devices require different interrupt flags. */
#define FLAGS_GPIO_0_1_ACTIVE GPIO_INT_LEVEL_ACTIVE
#define FLAGS_GPIO_EXP_ACTIVE (GPIO_INT_EDGE | GPIO_INT_HIGH_1 | GPIO_INT_LOW_0 | GPIO_INT_ENABLE)

#define GPIO_SPEC_AND_COMMA(button_or_led) GPIO_DT_SPEC_GET(button_or_led, gpios),

static const struct gpio_dt_spec buttons[] = {
#if DT_NODE_EXISTS(BUTTONS_NODE)
	DT_FOREACH_CHILD(BUTTONS_NODE, GPIO_SPEC_AND_COMMA)
#endif
};

static const struct gpio_dt_spec leds[] = {
#if DT_NODE_EXISTS(LEDS_NODE)
	DT_FOREACH_CHILD(LEDS_NODE, GPIO_SPEC_AND_COMMA)
#endif
};

enum state {
	STATE_WAITING,
	STATE_SCANNING,
};

static enum state state;
static struct k_work_delayable buttons_scan;
static button_handler_t button_handler_cb;
static atomic_t my_buttons;
static struct gpio_callback gpio_cb;
static struct k_spinlock lock;
static sys_slist_t button_handlers;
static struct k_mutex button_handler_mut;
static bool irq_enabled;

static int callback_ctrl(bool enable)
{
	int err = 0;
	gpio_flags_t flags;

	/* This must be done with irqs disabled to avoid pin callback
	 * being fired before others are still not activated.
	 */
	for (size_t i = 0; (i < ARRAY_SIZE(buttons)) && !err; i++) {
		if (enable) {
			flags = ((buttons[i].port == GPIO0_DEV || buttons[i].port == GPIO1_DEV) ?
					 FLAGS_GPIO_0_1_ACTIVE :
					 FLAGS_GPIO_EXP_ACTIVE);
		} else {
			flags = GPIO_INT_DISABLE;
		}

		err = gpio_pin_interrupt_configure_dt(&buttons[i], flags);
		if (err) {
			LOG_ERR("GPIO IRQ config failed, err: %d", err);
			return err;
		}
	}

	return err;
}

static uint32_t get_buttons(void)
{
	uint32_t ret = 0;
	for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
		int val;

		val = gpio_pin_get_dt(&buttons[i]);
		if (val < 0) {
			LOG_ERR("Cannot read gpio pin");
			return 0;
		}
		if (val) {
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
	int err;
	static uint32_t last_button_scan;
	static bool initial_run = true;
	uint32_t button_scan;

	if (irq_enabled) {
		/* Disable GPIO interrupts for edge triggered devices.
		 * Devices that are configured with active high interrupts are already disabled.
		 */
		err = callback_ctrl(false);
		if (err) {
			LOG_ERR("Cannot disable callbacks");
			return;
		}

		irq_enabled = false;
	}

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
		k_work_reschedule(&buttons_scan,
		  K_MSEC(CONFIG_DK_LIBRARY_BUTTON_SCAN_INTERVAL));
	} else {
		/* If no button is pressed module can switch to callbacks */
		int err = 0;

		k_spinlock_key_t key = k_spin_lock(&lock);

		switch (state) {
		case STATE_SCANNING:
			state = STATE_WAITING;
			err = callback_ctrl(true);
			irq_enabled = true;
			break;

		default:
			/* Do nothing */
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

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		err = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT);
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
	int err;
	k_spinlock_key_t key = k_spin_lock(&lock);

	switch (state) {
	case STATE_WAITING:
		if (gpio_dev == GPIO0_DEV || gpio_dev == GPIO1_DEV) {
			/* GPIO0 & GPIO1 has active high triggered interrupts and must be
			 * disabled here to avoid successive events from blocking the
			 * buttons_scan_fn function.
			 */
			err = callback_ctrl(false);
			if (err) {
				LOG_ERR("Failed disabling interrupts");
			}
			irq_enabled = false;
		}

		state = STATE_SCANNING;
		k_work_reschedule(&buttons_scan, K_MSEC(1));
		break;

	case STATE_SCANNING:
	default:
		/* Do nothing */
		break;
	}

	k_spin_unlock(&lock, key);
}

int dk_buttons_init(button_handler_t button_handler)
{
	int err;

	button_handler_cb = button_handler;

	if (IS_ENABLED(CONFIG_DK_LIBRARY_DYNAMIC_BUTTON_HANDLERS)) {
		k_mutex_init(&button_handler_mut);
	}

	for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
		/* Enable pull resistor towards the inactive voltage. */
		gpio_flags_t flags =
			buttons[i].dt_flags & GPIO_ACTIVE_LOW ?
			GPIO_PULL_UP : GPIO_PULL_DOWN;
		err = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT | flags);

		if (err) {
			LOG_ERR("Cannot configure button gpio");
			return err;
		}
	}

	uint32_t pin_mask = 0;

	for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
		/* Module starts in scanning mode and will switch to
		 * callback mode if no button is pressed.
		 */
		err = gpio_pin_interrupt_configure_dt(&buttons[i],
						      GPIO_INT_DISABLE);
		if (err) {
			LOG_ERR("Cannot disable callbacks()");
			return err;
		}

		pin_mask |= BIT(buttons[i].pin);
	}

	gpio_init_callback(&gpio_cb, button_pressed, pin_mask);

	for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
		err = gpio_add_callback(buttons[i].port, &gpio_cb);
		if (err) {
			LOG_ERR("Cannot add callback");
			return err;
		}
	}

	k_work_init_delayable(&buttons_scan, buttons_scan_fn);

	state = STATE_SCANNING;

	k_work_schedule(&buttons_scan, K_NO_WAIT);

	dk_read_buttons(NULL, NULL);

	atomic_set(&my_buttons, (atomic_val_t)get_buttons());

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

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		int val, err;

		if (BIT(i) & leds_on_mask) {
			val = 1;
		} else if (BIT(i) & leds_off_mask) {
			val = 0;
		} else {
			continue;
		}

		err = gpio_pin_set_dt(&leds[i], val);
		if (err) {
			LOG_ERR("Cannot write LED gpio");
			return err;
		}
	}

	return 0;
}

int dk_set_led(uint8_t led_idx, uint32_t val)
{
	int err;

	if (led_idx >= ARRAY_SIZE(leds)) {
		LOG_ERR("LED index out of the range");
		return -EINVAL;
	}
	err = gpio_pin_set_dt(&leds[led_idx], val);
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
