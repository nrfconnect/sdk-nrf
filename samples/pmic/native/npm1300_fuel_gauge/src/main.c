/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>

#include "fuel_gauge.h"

#define SLEEP_TIME_MS 1000

static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_charger));

int main(void)
{
	if (!device_is_ready(charger)) {
		printk("Charger device not ready.\n");
		return 0;
	}

	if (fuel_gauge_init(charger) < 0) {
		printk("Could not initialise fuel gauge.\n");
		return 0;
	}

	printk("PMIC device ok\n");

	while (1) {
		fuel_gauge_update(charger);
		k_msleep(SLEEP_TIME_MS);
	}
}
