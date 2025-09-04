/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/device_runtime.h>

#include "transport/dtm_transport.h"

#if DT_HAS_ALIAS(dtm_alive_pin)
// Use system_halt provided by the OS.
extern void arch_system_halt(unsigned int reason);

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	const struct gpio_dt_spec alive_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(dtm_alive_pin), gpios);
	(void)gpio_pin_set_dt(&alive_gpio, 0);

	LOG_PANIC();

	arch_system_halt(reason);

	CODE_UNREACHABLE;
}
#endif

int main(void)
{
	int err;
	union dtm_tr_packet cmd;

	printk("Starting Direct Test Mode sample\n");

#if defined(CONFIG_SOC_SERIES_NRF54HX)
	const struct device *dtm_uart = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(ncs_dtm_uart));

	if (dtm_uart != NULL) {
		int ret = pm_device_runtime_get(dtm_uart);

		if (ret < 0) {
			printk("Failed to get DTM UART runtime PM: %d\n", ret);
		}
	}
#endif /* defined(CONFIG_SOC_SERIES_NRF54HX) */

	err = dtm_tr_init();
	if (err) {
		printk("Error initializing DTM transport: %d\n", err);
		return err;
	}

#if DT_HAS_ALIAS(dtm_alive_pin)
	const struct gpio_dt_spec alive_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(dtm_alive_pin), gpios);

	err = gpio_pin_configure_dt(&alive_gpio, GPIO_OUTPUT_ACTIVE);
	if (err) {
		printk("Error setting DTM alive pin: %d\n", err);
		return err;
	}
#endif

	for (;;) {
		cmd = dtm_tr_get();
		err = dtm_tr_process(cmd);
		if (err) {
			printk("Error processing command: %d\n", err);
			return err;
		}
	}
}
