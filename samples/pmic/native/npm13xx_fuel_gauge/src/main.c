/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/npm13xx.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#include "fuel_gauge.h"

#define SLEEP_TIME_MS 1000

#if DT_NODE_EXISTS(DT_NODELABEL(npm1300_ek_pmic))
#define NPM13XX_DEVICE(dev) DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_ ## dev))
#elif DT_NODE_EXISTS(DT_NODELABEL(npm1304_ek_pmic))
#define NPM13XX_DEVICE(dev) DEVICE_DT_GET(DT_NODELABEL(npm1304_ek_ ## dev))
#else
#error "neither npm1300 nor npm1304 found in devicetree"
#endif

static const struct device *pmic = NPM13XX_DEVICE(pmic);
static const struct device *charger = NPM13XX_DEVICE(charger);
static volatile bool vbus_connected;

static void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (pins & BIT(NPM13XX_EVENT_VBUS_DETECTED)) {
		printk("Vbus connected\n");
		vbus_connected = true;
	}

	if (pins & BIT(NPM13XX_EVENT_VBUS_REMOVED)) {
		printk("Vbus removed\n");
		vbus_connected = false;
	}
}

int main(void)
{
	int err;

	if (!device_is_ready(pmic)) {
		printk("Pmic device not ready.\n");
		return 0;
	}

	if (!device_is_ready(charger)) {
		printk("Charger device not ready.\n");
		return 0;
	}

	if (fuel_gauge_init(charger) < 0) {
		printk("Could not initialise fuel gauge.\n");
		return 0;
	}

	static struct gpio_callback event_cb;

	gpio_init_callback(&event_cb, event_callback,
				   BIT(NPM13XX_EVENT_VBUS_DETECTED) |
				   BIT(NPM13XX_EVENT_VBUS_REMOVED));

	err = mfd_npm13xx_add_callback(pmic, &event_cb);
	if (err) {
		printk("Failed to add pmic callback.\n");
		return 0;
	}

	/* Initialise vbus detection status. */
	struct sensor_value val;
	int ret = sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);

	if (ret < 0) {
		return false;
	}

	vbus_connected = (val.val1 != 0) || (val.val2 != 0);

	printk("PMIC device ok\n");

	while (1) {
		fuel_gauge_update(charger, vbus_connected);
		k_msleep(SLEEP_TIME_MS);
	}
}
