/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <gpio.h>
#include <net/socket.h>
#include <nrf9160.h>
#include <stdio.h>
#include <string.h>
#include <uart.h>
#include <zephyr.h>

struct device *gpio_dev;
static const u8_t led_pins[] = { LED0_GPIO_PIN, LED1_GPIO_PIN, LED2_GPIO_PIN,
				 LED3_GPIO_PIN };

#pragma GCC push_options
#pragma GCC optimize ("O3")
static inline void nrf_delay_us(u32_t microsec)
{
	NRF_TIMER1_NS->TASKS_CLEAR = 1;
	if (microsec < 2 )
		return;
	NRF_TIMER1_NS->CC[0] = (microsec << 4) - 26;
	NRF_TIMER1_NS->PRESCALER = 0;
	NRF_TIMER1_NS->TASKS_START = 1;
	while (NRF_TIMER1_NS->EVENTS_COMPARE[0] == 0)
		;
	NRF_TIMER1_NS->EVENTS_COMPARE[0] = 0;
	NRF_TIMER1_NS->TASKS_STOP = 1;	
}
#pragma GCC pop_options

int dk_leds_init(void)
{
	int err = 0;

	gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!gpio_dev) {
		LOG_ERR("Cannot bind gpio device");
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(led_pins); i++) {
		err = gpio_pin_configure(gpio_dev, led_pins[i], GPIO_DIR_OUT);

		if (err) {
			LOG_ERR("Cannot configure LED gpio");
			return err;
		}
	}
	return err;
}

int main(void)
{
	printk("usec demo\n\r");
	printk("DO NOT INSERT 1 us! Will make timer roll over!\n\r");
	int err = dk_leds_init();

	NRF_CLOCK_NS->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK_NS->EVENTS_HFCLKSTARTED == 0);

	if (err) {
		printk("Error gpio config: %d", err);
	}
	while (1) {
		/* Using native gpio library adds delay */

		/* err = gpio_pin_write(gpio_dev, led_pins[0], 1); */
		NRF_P0_NS->OUTSET = 1 << 12;
		nrf_delay_us(2);
		/* err = gpio_pin_write(gpio_dev, led_pins[0], 0); */
		NRF_P0_NS->OUTCLR = 1 << 12;
		nrf_delay_us(3);
	}
}
