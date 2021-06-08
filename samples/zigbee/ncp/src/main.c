/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Zigbee Network Co-processor sample
 */

#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <usb/usb_device.h>
#include <logging/log.h>
#include <zb_nrf_platform.h>
#include <zb_osif_ext.h>
#include <zephyr.h>
#include <device.h>

#if CONFIG_BOOTLOADER_MCUBOOT
#include <dfu/mcuboot.h>
#endif

LOG_MODULE_REGISTER(app);

#if DT_NODE_EXISTS(DT_ALIAS(rst0))
#define RST_PIN_PORT DT_GPIO_LABEL(DT_ALIAS(rst0), gpios)
#define RST_PIN_NUMBER DT_GPIO_PIN(DT_ALIAS(rst0), gpios)
#define RST_PIN_LVL	0
#endif

zb_ret_t zb_osif_bootloader_run_after_reboot(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(rst0))
	int err = 0;
	const struct device *rst_dev = device_get_binding(RST_PIN_PORT);

	if (!rst_dev) {
		return RET_ERROR;
	}

	err = gpio_pin_configure(rst_dev, RST_PIN_NUMBER, GPIO_OUTPUT);
	if (err) {
		return RET_ERROR;
	}

	/* Perform pin reset */
	err = gpio_pin_set_raw(rst_dev, RST_PIN_NUMBER, RST_PIN_LVL);
	if (err) {
		return RET_ERROR;
	}
#endif
	return RET_OK;
}

void zb_osif_bootloader_report_successful_loading(void)
{
#if CONFIG_BOOTLOADER_MCUBOOT
	if (!boot_is_img_confirmed()) {
		int ret = boot_write_img_confirmed();

		if (ret) {
			LOG_ERR("Couldn't confirm image: %d", ret);
		} else {
			LOG_INF("Marked image as OK");
		}
	}
#endif
}

int main(void)
{
	LOG_INF("Starting Zigbee Network Co-processor sample");

#ifdef CONFIG_USB
	/* Enable USB device. */
	int ret = usb_enable(NULL);

	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_UART_LINE_CTRL
	const struct device *uart_dev;
	uint32_t dtr = 0U;

	uart_dev = device_get_binding(CONFIG_ZIGBEE_UART_DEVICE_NAME);
	if (uart_dev == NULL) {
		LOG_ERR("Error during serial initialization");
		return -EIO;
	}

	while (true) {
		uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}
		/* Give CPU resources to low priority threads. */
		k_sleep(K_MSEC(100));
	}

	/* Data Carrier Detect Modem - mark connection as established. */
	(void)uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DCD, 1);
	/* Data Set Ready - the NCP SoC is ready to communicate. */
	(void)uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DSR, 1);
#endif /* CONFIG_UART_LINE_CTRL */

	/* Wait 1 sec for the host to do all settings */
	k_sleep(K_SECONDS(1));
#endif /* CONFIG_USB */

	zb_osif_ncp_set_nvram_filter();

	/* Start Zigbee default thread */
	zigbee_enable();

	LOG_INF("Zigbee Network Co-processor sample started");

	return 0;
}
