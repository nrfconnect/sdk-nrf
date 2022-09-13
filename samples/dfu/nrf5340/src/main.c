/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <dfu_target_uart_host.h>
#include <zephyr/logging/log.h>
#include <dfu/dfu_target_mcuboot.h>
LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

/* CONFIG_DOWNLOAD_CLIENT_BUF_SIZE + some room */
static uint8_t buf[CONFIG_CLIENT_RX_BUF_SIZE];

static void on_update_done(bool success)
{
	LOG_WRN("Update success: %d", success);
}

void main(void)
{
	const struct device *uart = DEVICE_DT_GET(DT_CHOSEN(sample_dfu_uart));

	LOG_WRN("Starting version " CONFIG_MCUBOOT_IMAGE_VERSION);

	if (!device_is_ready(uart)) {
		LOG_WRN("UART is not available!");
		return;
	}

	LOG_WRN("UART is available! Waiting for update");

	dfu_target_mcuboot_set_buf(buf, ARRAY_SIZE(buf));

	dfu_target_uart_host_start(uart, on_update_done);
}
