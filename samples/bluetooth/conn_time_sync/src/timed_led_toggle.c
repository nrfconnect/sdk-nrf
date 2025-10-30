/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** This file implements timed toggling a LED
 *
 * To achieve accurate toggling, it interacts with the controller
 * clock using PPI.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>
#include <helpers/nrfx_gppi.h>
#include "conn_time_sync.h"

#define GPIOTE_NODE NRF_DT_GPIOTE_NODE(DT_ALIAS(led1), gpios)
#define LED_PIN NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(led1), gpios)

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});

static uint8_t previous_led_value;

int timed_led_toggle_init(void)
{
	int err;
	nrfx_gppi_handle_t ppi_led_toggle;
	uint8_t gpiote_chan_led_toggle;
	nrfx_gpiote_t *gpiote = &GPIOTE_NRFX_INST_BY_NODE(GPIOTE_NODE);

	const nrfx_gpiote_output_config_t gpiote_output_cfg = NRFX_GPIOTE_DEFAULT_OUTPUT_CONFIG;

	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		printk("Error %d: failed to configure LED device %s pin %d\n", err, led.port->name,
		       led.pin);
		return err;
	}

	if (nrfx_gpiote_channel_alloc(gpiote, &gpiote_chan_led_toggle) != 0) {
		printk("Failed allocating GPIOTE chan for setting led\n");
		return -ENOMEM;
	}

	const nrfx_gpiote_task_config_t task_cfg_led_toggle = {
		.task_ch = gpiote_chan_led_toggle,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = (led.dt_flags & GPIO_ACTIVE_LOW) ?
			NRF_GPIOTE_INITIAL_VALUE_HIGH : NRF_GPIOTE_INITIAL_VALUE_LOW,
	};

	if (nrfx_gpiote_output_configure(gpiote, LED_PIN, &gpiote_output_cfg,
					 &task_cfg_led_toggle) != 0) {
		printk("Failed configuring GPIOTE chan for toggling led\n");
		return -ENOMEM;
	}

	err = nrfx_gppi_conn_alloc(controller_time_trigger_event_addr_get(),
				   nrfx_gpiote_out_task_address_get(&gpiote, LED_PIN),
				   &ppi_led_toggle);
	if (err < 0) {
		printk("Failed allocating PPI chan for toggling led\n");
		return -ENOMEM;
	}

	nrfx_gppi_conn_enable(ppi_led_toggle);
	nrfx_gpiote_out_task_enable(gpiote, LED_PIN);

	return 0;
}

void timed_led_toggle_trigger_at(uint8_t value, uint32_t timestamp_us)
{
	/* First obtain the full 64-bit time. */
	const uint64_t current_time_us = controller_time_us_get();
	const uint64_t current_time_most_significant_word = current_time_us & 0xFFFFFFFF00000000UL;

	uint64_t full_timestamp_us = current_time_most_significant_word | timestamp_us;

	if (timestamp_us < (current_time_us & UINT32_MAX)) {
		/* Trigger time is after UINT32 wrap */
		full_timestamp_us += 0x100000000UL;
	}

	controller_time_trigger_set(full_timestamp_us);

	previous_led_value = value;
}
