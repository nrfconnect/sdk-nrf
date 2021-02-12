/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Zigbee Network Co-processor sample
 */

#include <drivers/uart.h>
#include <usb/usb_device.h>
#include <logging/log.h>
#include <zb_nrf_platform.h>
#include <zb_osif_ext.h>

LOG_MODULE_REGISTER(app);

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
