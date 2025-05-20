/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <ctype.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_fota_external, CONFIG_APP_LOG_LEVEL);
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <mcumgr_smp_client.h>

#if defined(CONFIG_APP_SMP_SERVER_RECOVERY_MODE)

#define RESET_NODE DT_NODELABEL(nrf52840_reset)

#if DT_NODE_HAS_STATUS(RESET_NODE, okay)

static int nrf52840_reset_api(void)
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
		LOG_ERR("Pin configure err:%d", err);
		return err;
	}

	/* Reset the nRF52840 and let it wait until the pin is inactive again
	 * before running to main to ensure that it won't send any data until
	 * the H4 device is setup and ready to receive.
	 */
	err = gpio_pin_set_dt(&reset_pin_spec, 1);
	if (err) {
		LOG_ERR("GPIO Pin set to 1 err:%d", err);
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
		LOG_ERR("GPIO Pin set to 0 err:%d", err);
		return err;
	}

	LOG_DBG("Reset Pin %d", reset_pin_spec.pin);

	return 0;
}
#endif /* DT_NODE_HAS_STATUS(RESET_NODE, okay) */
#endif

int fota_external_init(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_APP_SMP_SERVER_RECOVERY_MODE)) {
#if DT_NODE_HAS_STATUS(RESET_NODE, okay)
		LOG_INF("Init external FOTA for use with MCUBoot recovery mode");
		ret = mcumgr_smp_client_init(nrf52840_reset_api);
#else
		LOG_ERR("Reset not okay");
		return -EIO;
#endif
	} else {
		LOG_INF("Init external FOTA");
		ret = mcumgr_smp_client_init(NULL);
	}

	return ret;
}
