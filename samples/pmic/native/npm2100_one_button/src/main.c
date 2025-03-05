/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/npm2100.h>
#include <zephyr/drivers/regulator.h>

#define FLASH_FAST_MS	100
#define FLASH_SLOW_MS	500
#define PRESS_SHORT_MS	1000
#define PRESS_MEDIUM_MS 5000

static int flash_time_ms = FLASH_SLOW_MS;
static bool shipmode;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct device *pmic = DEVICE_DT_GET(DT_NODELABEL(npm2100_pmic));
static const struct device *regulators = DEVICE_DT_GET(DT_NODELABEL(npm2100_regulators));

static void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t events)
{
	static int64_t press_t;

	if (events & BIT(NPM2100_EVENT_SYS_SHIPHOLD_FALL)) {
		press_t = k_uptime_get();
	}

	if (events & BIT(NPM2100_EVENT_SYS_SHIPHOLD_RISE)) {
		int64_t delta_t = k_uptime_get() - press_t;

		if (delta_t < PRESS_SHORT_MS) {
			printk("Short press\n");
			flash_time_ms = FLASH_FAST_MS;
		} else if (delta_t < PRESS_MEDIUM_MS) {
			printk("Medium press\n");
			flash_time_ms = FLASH_SLOW_MS;
		} else {
			shipmode = true;
		}
	}
}

static bool configure_events(void)
{
	int ret;
	static struct gpio_callback event_cb;

	if (!device_is_ready(pmic)) {
		printk("Error: PMIC device not ready.\n");
		return false;
	}

	if (!device_is_ready(regulators)) {
		printk("Error: Regulator device not ready.\n");
		return false;
	}

	gpio_init_callback(&event_cb, event_callback,
			   BIT(NPM2100_EVENT_SYS_SHIPHOLD_FALL) |
				   BIT(NPM2100_EVENT_SYS_SHIPHOLD_RISE));

	ret = mfd_npm2100_add_callback(pmic, &event_cb);

	if (ret < 0) {
		printk("Error: failed to add a PMIC event callback.\n");
		return false;
	}

	return true;
}

static bool configure_ui(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		printk("Error: led device %s is not ready.\n", led.port->name);
		return false;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);

	if (ret < 0) {
		printk("Error: failed to configure led device %s.\n", led.port->name);
		return false;
	}

	printk("Set up led at %s pin %d\n", led.port->name, led.pin);

	return true;
}

int main(void)
{
	int ret;

	if (!configure_ui()) {
		return 0;
	}

	if (!configure_events()) {
		return 0;
	}

	printk("PMIC device ok\n");

	while (1) {
		if (shipmode) {
			shipmode = false;
			printk("Ship mode...\n");
			/* make sure to display the message before shutting down */
			LOG_PROCESS();

			ret = regulator_parent_ship_mode(regulators);
			if (ret < 0) {
				printk("Error: failed to enter ship mode.\n");
			}
		}
		(void)gpio_pin_toggle_dt(&led);
		k_msleep(flash_time_ms);
	}
}
