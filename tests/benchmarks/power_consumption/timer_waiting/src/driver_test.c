/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>

#define ALARM_CHANNEL_ID (0)
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
const struct device *const counter_dev = DEVICE_DT_GET(DT_ALIAS(counter));
static bool state = true;

struct counter_alarm_cfg counter_cfg;

void counter_handler(const struct device *counter_dev, uint8_t id, uint32_t ticks, void *user_data)
{
	counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, user_data);
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	for (int i = 1000000UL; i > 0; i--) {
		if (state == true) {
			state = false;
			gpio_pin_set_dt(&led, 1);
		} else {
			state = true;
			gpio_pin_set_dt(&led, 0);
		}
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
}

int main(void)
{
	counter_start(counter_dev);

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(counter_dev, 1000000UL);
	counter_cfg.callback = counter_handler;
	counter_cfg.user_data = &counter_cfg;

	counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, &counter_cfg);

	while (1) {
		k_sleep(K_FOREVER);
	}
	return 0;
}
