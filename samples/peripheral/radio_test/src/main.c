/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <sys/printk.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

static void clock_init(void)
{
	int err;
	struct device *clock;
	enum clock_control_status clock_status;

	clock = device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL);
	if (!clock) {
		printk("Unable to find clock device binding\n");
		return;
	}

	err = clock_control_on(clock, CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (err) {
		printk("Unable to turn on the clock: %d", err);
	}

	do {
		clock_status = clock_control_get_status(clock,
			CLOCK_CONTROL_NRF_SUBSYS_HF);
	} while (clock_status != CLOCK_CONTROL_STATUS_ON);

	printk("Clock has started\n");
}

void main(void)
{
	printk("Starting Radio Test example\n");

	clock_init();
}
