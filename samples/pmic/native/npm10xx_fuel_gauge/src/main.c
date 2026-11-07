/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/charger/npm10xx.h>
#include <zephyr/drivers/mfd/npm10xx.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm10xx.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "fuel_gauge.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define SLEEP_TIME_MS 500

static const struct device *pmic = DEVICE_DT_GET(DT_NODELABEL(npm1012));
static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1012_charger));
static const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(npm1012_adc));
static const struct led_dt_spec charge_indicator = LED_DT_SPEC_GET(DT_NODELABEL(charge_indicator));
static atomic_t vbus_state_changed;

static void event_handler(const struct device *mfd, struct mfd_npm10xx_event_callback *cb,
			  uint64_t events)
{
	if (events & (NPM10XX_EVENT_VBUSPRESENTPE | NPM10XX_EVENT_VBUSPRESENTNE)) {
		atomic_set(&vbus_state_changed, true);
	}
}

int main(void)
{
	int err;
	bool vbus_connected;
	union charger_propval chgr_prop;

	if (!device_is_ready(pmic)) {
		LOG_ERR("Pmic device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(charger)) {
		LOG_ERR("Charger device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(adc)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		err = settings_subsys_init();
		if (err < 0) {
			LOG_ERR("Settings subsystem could not be initialized (err %d)", err);
		}
	}

	err = fuel_gauge_init(adc, charger);
	if (err < 0) {
		LOG_ERR("Could not initialise fuel gauge (err %d)", err);
		return err;
	}

	/* initialise VBUS status */
	err = charger_get_prop(charger, CHARGER_PROP_ONLINE, &chgr_prop);
	if (err) {
		LOG_ERR("Failed to get VBUS status (err %d)", err);
		return err;
	}
	vbus_connected = chgr_prop.online == CHARGER_ONLINE_FIXED;

	/* configure callback for VBUS events */
	static struct mfd_npm10xx_event_callback event_cb = {
		.event_mask = NPM10XX_EVENT_VBUSPRESENTPE | NPM10XX_EVENT_VBUSPRESENTNE,
		.handler = event_handler,
	};

	err = mfd_npm10xx_manage_callback(pmic, &event_cb, true);
	if (err) {
		LOG_ERR("Failed to add pmic callback (err %d)", err);
		return err;
	}

	LOG_INF("PMIC device ok");

	err = led_set_brightness_dt(&charge_indicator, 50);
	if (err) {
		LOG_WRN("Failed to set charge indicator brightness (err %d)", err);
	}

	vbus_connected = false;
	atomic_set(&vbus_state_changed, true);

	while (1) {
		if (atomic_set(&vbus_state_changed, false)) {
			err = charger_get_prop(charger, CHARGER_PROP_ONLINE, &chgr_prop);
			if (err) {
				LOG_ERR("Failed to get VBUS status (err %d)", err);
				break;
			}

			vbus_connected = chgr_prop.online == CHARGER_ONLINE_FIXED;
			LOG_INF("VBUS %s", vbus_connected ? "connected" : "removed");
		}

		fuel_gauge_update(adc, charger, vbus_connected);

		/* Using constant sleep time to keep the sample simple.
		 * In a real application, the interval can be long when the battery is discharging
		 * at a slow rate. Reduce the interval during periods of higher discharge current
		 * (relative to the battery size). Also use shorter intervals during charging to
		 * facilitate state-of-health estimation.
		 */
		k_msleep(SLEEP_TIME_MS);
	}
}
