/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <soc_lrcconf.h>
#include <zephyr/drivers/gpio.h>

#define FLL16M_MODE_OPEN_LOOP 0
#define FLL16M_MODE_BYPASS    2
#define FLL16M_MODE_LOOP_MASK BIT(0)

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

int main(void)
{
	__ASSERT(gpio_is_ready_dt(&led), "GPIO Device not ready");
	__ASSERT(gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) == 0,
		 "Could not configure led GPIO");

	soc_lrcconf_poweron_request((sys_snode_t *)NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN);
	nrf_lrcconf_clock_source_set(
		NRF_LRCCONF010, 0,
		(nrf_lrcconf_clk_src_t)(FLL16M_MODE_BYPASS & FLL16M_MODE_LOOP_MASK), true);
	nrf_lrcconf_task_trigger(NRF_LRCCONF010, NRF_LRCCONF_TASK_CLKSTART_0);

	k_msleep(1000);

	gpio_pin_set_dt(&led, 0);
	nrf_lrcconf_clock_source_set(
		NRF_LRCCONF010, 0,
		(nrf_lrcconf_clk_src_t)(FLL16M_MODE_OPEN_LOOP & FLL16M_MODE_LOOP_MASK), false);
	nrf_lrcconf_task_trigger(NRF_LRCCONF010, NRF_LRCCONF_TASK_CLKSTART_0);

	return 0;
}
