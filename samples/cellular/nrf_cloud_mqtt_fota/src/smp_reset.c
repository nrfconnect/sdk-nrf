/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

LOG_MODULE_DECLARE(nrf_cloud_mqtt_fota,	CONFIG_NRF_CLOUD_MQTT_FOTA_SAMPLE_LOG_LEVEL);

#define RESET_NODE DT_NODELABEL(nrf52840_reset)
#define HAS_RECOVERY_MODE (DT_NODE_HAS_STATUS(RESET_NODE, okay))

int nrf52840_reset_api(void)
{
	int err;
	const struct gpio_dt_spec reset_pin_spec = GPIO_DT_SPEC_GET(RESET_NODE, gpios);

	if (!device_is_ready(reset_pin_spec.port)) {
		LOG_ERR("Reset device not ready");
		return -EIO;
	}

	/* Configure pin as output and initialize it to inactive state. */
	err = gpio_pin_configure_dt(&reset_pin_spec, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Pin configure err: %d", err);
		return err;
	}

	/* Reset the nRF52840 and let it wait until the pin is inactive again
	 * before running to main to ensure that it won't send any data until
	 * the H4 device is setup and ready to receive.
	 */
	err = gpio_pin_set_dt(&reset_pin_spec, 1);
	if (err) {
		LOG_ERR("GPIO Pin set to 1 err: %d", err);
		return err;
	}

	/* Wait for the nRF52840 peripheral to stop sending data.
	 *
	 * It is critical (!) to wait here, so that all bytes
	 * on the lines are received and drained correctly.
	 */
	k_sleep(K_MSEC(10));

	/* We are ready, let the nRF52840 run to main */
	err = gpio_pin_set_dt(&reset_pin_spec, 0);
	if (err) {
		LOG_ERR("GPIO Pin set to 0 err: %d", err);
		return err;
	}

	LOG_DBG("Reset Pin %d", reset_pin_spec.pin);

	return 0;
}
