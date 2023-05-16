/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <npmx_driver.h>
#include <npmx_led.h>

#define LOG_MODULE_NAME pmic_pof
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/**
 * @brief Function called when battery voltage drops below the configured threshold.
 *
 * @param[in] p_pm Pointer to the instance of an nPM device.
 */
void my_pof_callback(npmx_instance_t *p_pm)
{
	LOG_INF("POF callback");
}

void main(void)
{
	const struct device *pmic_dev = DEVICE_DT_GET(DT_NODELABEL(npm_0));

	if (!device_is_ready(pmic_dev)) {
		LOG_INF("PMIC device is not ready");
		return;
	}

	LOG_INF("PMIC device ok");

	/* Set configuration for the power fail comparator. */
	npmx_pof_config_t config = {
		.status = NPMX_POF_STATUS_ENABLE,
		.threshold = NPMX_POF_THRESHOLD_3V0,
		.polarity = NPMX_POF_POLARITY_HIGH,
	};

	/* Configure and register a callback. */
	if (npmx_driver_register_pof_cb(pmic_dev, &config, my_pof_callback) == 0) {
		LOG_INF("Successfully registered callback");
	}
}
