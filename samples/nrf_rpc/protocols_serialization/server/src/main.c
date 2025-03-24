/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <nrf_rpc.h>

LOG_MODULE_REGISTER(nrf_ps_server, CONFIG_NRF_PS_SERVER_LOG_LEVEL);

static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details", report->code);
}

#ifdef CONFIG_NRF_PS_SERVER_RPC_ALIVE_LED
void nrf_rpc_uart_initialized_hook(const struct device *uart_dev)
{
	const struct gpio_dt_spec alive_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(rpc_alive_led), gpios);
	int ret;

	__ASSERT_NO_MSG(uart_dev == DEVICE_DT_GET(DT_CHOSEN(nordic_rpc_uart)));
	ret = gpio_pin_configure_dt(&alive_gpio, GPIO_OUTPUT_ACTIVE);

	if (ret) {
		LOG_ERR("Failed to configure RPC alive GPIO: %d", ret);
	}
}
#endif

int main(void)
{
	int ret;

	LOG_INF("Initializing RPC server");

	ret = nrf_rpc_init(err_handler);

	if (ret != 0) {
		LOG_ERR("RPC init failed");
	}

	LOG_INF("RPC server ready");

	return 0;
}
