/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/gpio.h>

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);
const struct device *lfclk_dev = DEVICE_DT_GET(DT_NODELABEL(lfclk));

const struct nrf_clock_spec lfclk_lfrc_mode = {
	.frequency = 32768,
	.accuracy = 0,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

const struct nrf_clock_spec lfclk_synth_mode = {
	.frequency = 32768,
	.accuracy = 30,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

int main(void)
{
	int ret, res;
	struct nrf_clock_spec *clk_spec;
	struct onoff_client cli;
	uint32_t counter = 0;

	__ASSERT(gpio_is_ready_dt(&led), "GPIO Device not ready");
	__ASSERT(gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) == 0,
		 "Could not configure led GPIO");

	while (1) {
		clk_spec = (struct nrf_clock_spec *)(counter++ % 2 ? &lfclk_lfrc_mode
								   : &lfclk_synth_mode);
		sys_notify_init_spinwait(&cli.notify);
		ret = nrf_clock_control_request(lfclk_dev, clk_spec, &cli);
		__ASSERT(ret >= 0 && ret <= 2, "Clock control request failed");
		do {
			ret = sys_notify_fetch_result(&cli.notify, &res);
		} while (ret == -EAGAIN);
		__ASSERT(ret == 0, "return code: %d", ret);
		__ASSERT(res == 0, "response: %d", res);
		k_msleep(1000);
		ret = nrf_clock_control_release(lfclk_dev, clk_spec);
		__ASSERT(ret == ONOFF_STATE_ON, "return code: %d", ret);
		gpio_pin_toggle_dt(&led);
	}

	return 0;
}
