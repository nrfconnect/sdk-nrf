/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#include "idle_power.h"

#ifdef CONFIG_NRF7120_IDLE_DIAGNOSTICS

#include <hal/nrf_power.h>
#include <hal/nrf_memconf.h>
#include <hal/nrf_grtc.h>
#include <hal/nrf_lfxo.h>

/* GPIO configuration for idle phase marker on P0.00 */
static const struct gpio_dt_spec idle_marker = GPIO_DT_SPEC_GET_OR(
	DT_ALIAS(idle_phase), gpios, {0});

int idle_power_init(void)
{
	int ret = 0;

	if (gpio_is_ready_dt(&idle_marker)) {
		ret = gpio_pin_configure_dt(&idle_marker, GPIO_OUTPUT_HIGH);
		if (ret < 0) {
			printk("Error configuring idle marker GPIO: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

void idle_power_print_snapshot(const char *phase)
{
	printk("\n%s: Power State Snapshot\n", phase);
	printk("  CONSTLATSTAT=0x%08x SYSCOUNTER_ACTIVE=0x%08x\n",
	       NRF_POWER->CONSTLATSTAT,
	       NRF_GRTC->SYSCOUNTER[0].ACTIVE);

	printk("  MEMCONF0: CONTROL=0x%08x RET=0x%08x RET2=0x%08x\n",
	       NRF_MEMCONF->POWER[0].CONTROL,
	       NRF_MEMCONF->POWER[0].RET,
	       NRF_MEMCONF->POWER[0].RET2);

	printk("  MEMCONF1: CONTROL=0x%08x RET=0x%08x RET2=0x%08x\n",
	       NRF_MEMCONF->POWER[1].CONTROL,
	       NRF_MEMCONF->POWER[1].RET,
	       NRF_MEMCONF->POWER[1].RET2);

	printk("  GRTC_MODE=0x%08x LFXO_STATUS=0x%08x LFXO_MODE=0x%08x\n",
	       NRF_GRTC->MODE,
	       NRF_LFXO->STATUS,
	       NRF_LFXO->MODE);
}

#else

int idle_power_init(void)
{
	return 0;
}

void idle_power_print_snapshot(const char *phase)
{
	ARG_UNUSED(phase);
}

#endif
