/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <adp536x.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(board_secure, CONFIG_BOARD_LOG_LEVEL);

#define ADP536X_I2C_DEVICE	DEVICE_DT_GET(DT_NODELABEL(i2c2))
#define LC_MAX_READ_LENGTH	128

/* The BH1749 on must be powered by the ADP5360 before it can be initialized. */
#if IS_ENABLED(CONFIG_BH1749)
BUILD_ASSERT(CONFIG_BOARD_INIT_PRIORITY < CONFIG_SENSOR_INIT_PRIORITY,
	     "The board initialization must happen before the sensor initialization "
	     "for the sensors to be powered before initializing");
#endif

static int power_mgmt_init(void)
{
	int err;

	err = adp536x_init(ADP536X_I2C_DEVICE);
	if (err) {
		LOG_ERR("ADP536X failed to initialize, error: %d\n", err);
		return err;
	}

	err = adp536x_buck_1v8_set();
	if (err) {
		LOG_ERR("Could not set buck to 1.8 V, error: %d\n", err);
		return err;
	}

	err = adp536x_buckbst_3v3_set();
	if (err) {
		LOG_ERR("Could not set buck/boost to 3.3 V, error: %d\n", err);
		return err;
	}

	err = adp536x_buckbst_enable(true);
	if (err) {
		LOG_ERR("Could not enable buck/boost output, error: %d\n", err);
		return err;
	}

	/* Enables discharge resistor for buck regulator that brings the voltage
	 * on its output faster down when it's inactive. Needed because some
	 * components require to boot up from ~0V.
	 */
	err = adp536x_buck_discharge_set(true);
	if (err) {
		return err;
	}

	/* Sets the VBUS current limit to 500 mA. */
	err = adp536x_vbus_current_set(ADP536X_VBUS_ILIM_500mA);
	if (err) {
		LOG_ERR("Could not set VBUS current limit, error: %d\n", err);
		return err;
	}

	/* Sets the charging current to 320 mA. */
	err = adp536x_charger_current_set(ADP536X_CHG_CURRENT_320mA);
	if (err) {
		LOG_ERR("Could not set charging current, error: %d\n", err);
		return err;
	}

	/* Sets the charge current protection threshold to 400 mA. */
	err = adp536x_oc_chg_current_set(ADP536X_OC_CHG_THRESHOLD_400mA);
	if (err) {
		LOG_ERR("Could not set charge current protection, error: %d\n",
			err);
		return err;
	}

	err = adp536x_charging_enable(true);
	if (err) {
		LOG_ERR("Could not enable charging: %d\n", err);
		return err;
	}

	err = adp536x_fg_set_mode(ADP566X_FG_ENABLED, ADP566X_FG_MODE_SLEEP);
	if (err) {
		LOG_ERR("Could not enable fuel gauge: %d", err);
		return err;
	}

	return 0;
}

static int thingy91_board_init(void)
{
	int err;

	err = power_mgmt_init();
	if (err) {
		LOG_ERR("power_mgmt_init failed with error: %d", err);
		return err;
	}

	return 0;
}

SYS_INIT(thingy91_board_init, POST_KERNEL, CONFIG_BOARD_INIT_PRIORITY);
