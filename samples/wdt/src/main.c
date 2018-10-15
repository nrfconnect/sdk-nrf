/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <watchdog.h>
#include <gpio.h>
#include <misc/printk.h>

#define WDT_DEV_NAME   CONFIG_WDT_0_NAME
#define LEDS_NUMBER 4

struct {
	struct device *port;
	u8_t pin;
} leds[LEDS_NUMBER] = {
	{
		.pin = LED0_GPIO_PIN,
	},
	{
		.pin = LED1_GPIO_PIN,
	},
	{
		.pin = LED2_GPIO_PIN,
	},
	{
		.pin = LED3_GPIO_PIN,
	},
};

static void wdt_callback(struct device *wdt_dev, int channel_id)
{
	ARG_UNUSED(wdt_dev);
	ARG_UNUSED(channel_id);
	/* Watchdog timer expired. Handle it here.
	 * Remember that SoC reset will be done soon.
	 */
	for (int i = 0; i < LEDS_NUMBER; i++) {
		gpio_pin_write(leds[i].port, leds[i].pin, 1);
	}
}

struct device *wdt;
int wdt_channel_id;
void button_pressed(struct device *gpiob, struct gpio_callback *cb,
		    u32_t pins)
{
	wdt_feed(wdt, wdt_channel_id);
	printk("Feeding watchdog...\n");
}

static struct gpio_callback gpio_cb;

void main(void)
{
	int err;

	leds[0].port = device_get_binding(LED0_GPIO_CONTROLLER);
	leds[1].port = device_get_binding(LED1_GPIO_CONTROLLER);
	leds[2].port = device_get_binding(LED2_GPIO_CONTROLLER);
	leds[3].port = device_get_binding(LED3_GPIO_CONTROLLER);

	for (int i = 0; i < LEDS_NUMBER; i++) {
		gpio_pin_configure(leds[i].port, leds[i].pin, GPIO_DIR_OUT);
		gpio_pin_write(leds[i].port, leds[i].pin, 1);
	}

	struct device *sw_port;
	sw_port = device_get_binding(SW0_GPIO_CONTROLLER);
	gpio_pin_configure(sw_port, SW0_GPIO_PIN, GPIO_DIR_IN
						  | GPIO_INT
						  | GPIO_INT_ACTIVE_LOW
						  | GPIO_INT_EDGE
						  | GPIO_PUD_PULL_UP);
	gpio_init_callback(&gpio_cb, button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(sw_port, &gpio_cb);
	gpio_pin_enable_callback(sw_port, SW0_GPIO_PIN);

	struct wdt_timeout_cfg wdt_config;
	wdt = device_get_binding(WDT_DEV_NAME);
	if (!wdt) {
		printk("Cannot get WDT device\n");
		return;
	}

	/* Reset SoC when watchdog timer expires. */
	wdt_config.flags = WDT_FLAG_RESET_SOC;

	/* Expire watchdog after 2000 milliseconds. */
	wdt_config.window.min = 0;
	wdt_config.window.max = 2000;

	/* Set up watchdog callback. Jump into it when watchdog expired. */
	wdt_config.callback = wdt_callback;

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id < 0) {
		printk("Watchdog install error\n");
		return;
	}

	err = wdt_setup(wdt, 0);
	if (err < 0) {
		printk("Watchdog setup error\n");
		return;
	}

	/* Indicate program start on leds */
	for (int i = 0; i < LEDS_NUMBER; i++)
	{
		k_sleep(200);
		gpio_pin_write(leds[i].port, leds[i].pin, 0);
	}

	/* Waiting for the SoC reset. */
	while (1) {
		k_yield();
	};
}
