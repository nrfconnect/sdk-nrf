/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi Throughput sample
 */

#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#if defined(CONFIG_NRFX_CLOCK_HFCLK) && defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT)
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#define HFCLK_REQUESTED_FREQUENCY MHZ(128)
#endif
#include <stdio.h>

int main(void)
{
#if defined(CONFIG_NRFX_CLOCK_HFCLK) && defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT)
	clock_control_set_rate(DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk), NULL,
			       (clock_control_subsys_rate_t)HFCLK_REQUESTED_FREQUENCY);
#endif
	printk("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock/MHZ(1));

	return 0;
}
