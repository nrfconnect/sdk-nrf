/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

LOG_MODULE_REGISTER(modem_trace_flash_sample, CONFIG_MODEM_TRACE_FLASH_SAMPLE_LOG_LEVEL);

#define READ_BUF_SIZE CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE
#define UART_DEVICE_NODE DT_CHOSEN(nordic_modem_trace_uart)
static const struct device *const uart_dev = DEVICE_DT_GET_OR_NULL(UART_DEVICE_NODE);

static void print_uart(char *buf, int len)
{
	if (uart_dev == NULL || !device_is_ready(uart_dev)) {
		LOG_ERR("uart1 device not found/ready!");
		return;
	}
	for (int i = 0; i < len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

static void print_traces(void)
{
	uint8_t read_buf[READ_BUF_SIZE];
	int ret;
	size_t read_offset = 0;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("uart1 device not found/ready!");
	}

	ret = nrf_modem_lib_trace_data_size();
	printk("Reading out %d bytes of trace data\n", ret);

	/* Read out the trace data from flash */
	while (ret > 0) {
		ret = nrf_modem_lib_trace_read(read_buf, READ_BUF_SIZE);
		if (ret < 0) {
			if (ret == -ENODATA) {
				break;
			}
			LOG_ERR("Error reading modem traces: %d", ret);
			break;
		}
		if (ret == 0) {
			LOG_DBG("No more traces to read from flash");
			break;
		}

		read_offset += ret;
		print_uart(read_buf, ret);
	}

	LOG_INF("Total trace bytes read from flash: %d", read_offset);
}
