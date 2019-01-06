/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <logging/log.h>
#include <nrfx.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(dk_buttons_and_leds, CONFIG_DK_LIBRARY_LOG_LEVEL);

static struct k_delayed_work buttons_scan;
static const u8_t button_pins[] = { SW0_GPIO_PIN, SW1_GPIO_PIN,
				    SW2_GPIO_PIN, SW3_GPIO_PIN };
static const u8_t led_pins[] = { LED0_GPIO_PIN, LED1_GPIO_PIN,
				 LED2_GPIO_PIN, LED3_GPIO_PIN };
static button_handler_t button_handler_cb;
static atomic_t my_buttons;
struct device *gpio_dev;

static u32_t get_buttons(void)
{
	u32_t ret = 0;
	for (size_t i = 0; i < ARRAY_SIZE(button_pins); i++) {
		u32_t val;

		if (gpio_pin_read(gpio_dev, button_pins[i], &val)) {
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

static void buttons_scan_fn(struct k_work *work)
{
	static u32_t last_button_scan;
	static bool initial_run = true;
	u32_t button_scan;

	button_scan = get_buttons();
	atomic_set(&my_buttons, (atomic_val_t)button_scan);

	if (!initial_run) {
		if (button_handler_cb != NULL) {
			if (button_scan != last_button_scan) {
				u32_t has_changed;

				has_changed = (button_scan ^ last_button_scan);
				button_handler_cb(button_scan, has_changed);
			}
		}
	} else {
		initial_run = false;
	}

	last_button_scan = button_scan;
	int err = k_delayed_work_submit(&buttons_scan,
		  CONFIG_DK_LIBRARY_BUTTON_SCAN_INTERVAL);

	if (err) {
		LOG_ERR("Cannot add work to workqueue");
	}
}

int dk_leds_init(void)
{
	int err;

	gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!gpio_dev) {
		LOG_ERR("Cannot bind gpio device");
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(led_pins); i++) {
		err = gpio_pin_configure(gpio_dev, led_pins[i],
					 GPIO_DIR_OUT);

		if (err) {
			LOG_ERR("Cannot configure LED gpio");
			return err;
		}
	}

	return dk_set_leds_state(DK_NO_LEDS_MSK, DK_ALL_LEDS_MSK);
}

int dk_buttons_init(button_handler_t button_handler)
{
	int err;

	button_handler_cb = button_handler;

	gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!gpio_dev) {
		LOG_ERR("Cannot bind gpio device");
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(button_pins); i++) {
		err = gpio_pin_configure(gpio_dev, button_pins[i],
					 GPIO_DIR_IN | GPIO_PUD_PULL_UP);

		if (err) {
			LOG_ERR("Cannot configure button gpio");
			return err;
		}
	}

	k_delayed_work_init(&buttons_scan, buttons_scan_fn);
	err = k_delayed_work_submit(&buttons_scan, 0);
	if (err) {
		LOG_ERR("Cannot add work to workqueue");
		return err;
	}

	dk_read_buttons(NULL, NULL);

	return 0;
}

void dk_read_buttons(u32_t *button_state, u32_t *has_changed)
{
	static u32_t last_state;
	u32_t current_state = atomic_get(&my_buttons);

	if (button_state != NULL) {
		*button_state = current_state;
	}

	if (has_changed != NULL) {
		*has_changed = (current_state ^ last_state);
	}

	last_state = current_state;
}

int dk_set_leds(u32_t leds)
{
	return dk_set_leds_state(leds, DK_ALL_LEDS_MSK);
}

int dk_set_leds_state(u32_t leds_on_mask, u32_t leds_off_mask)
{
	if ((leds_on_mask & ~DK_ALL_LEDS_MSK) != 0 ||
	   (leds_off_mask & ~DK_ALL_LEDS_MSK) != 0) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(led_pins); i++) {
		if ((BIT(i) & leds_on_mask) || (BIT(i) & leds_off_mask)) {
			u32_t val = (BIT(i) & leds_on_mask) ? (1) : (0);

			if (IS_ENABLED(CONFIG_DK_LIBRARY_INVERT_LEDS)) {
				val = 1 - val;
			}

			int err = gpio_pin_write(gpio_dev, led_pins[i], val);

			if (err) {
				LOG_ERR("Cannot write LED gpio");
				return err;
			}
		}
	}

	return 0;
}
