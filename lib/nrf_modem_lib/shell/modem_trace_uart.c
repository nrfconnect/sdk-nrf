/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <modem/nrf_modem_lib_trace.h>

#include "modem_trace_uart.h"

#define UART_DEVICE_NODE DT_CHOSEN(nordic_modem_trace_uart)
#define READ_BUF_SIZE CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static void print_uart(const struct shell *sh, char *buf, int len)
{
	if (!device_is_ready(uart_dev)) {
		shell_error(sh, "uart device not found/ready!");
		return;
	}
	for (int i = 0; i < len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

void modem_trace_uart_send(const struct shell *sh, size_t size)
{
	uint8_t read_buf[READ_BUF_SIZE];
	int ret = 1;
	size_t read_offset = 0;

	printk("Reading out %d bytes of trace data\n", size);
	/* Read out the trace data from flash */
	while (ret > 0) {
		ret = nrf_modem_lib_trace_read(read_buf, READ_BUF_SIZE);
		if (ret == 0 || ret == -ENODATA) {
			break;
		} else if (ret < 0) {
			shell_error(sh, "Error reading modem traces: %d", ret);
			break;
		}
		read_offset += ret;
		print_uart(sh, read_buf, ret);
	}
	shell_print(sh, "Total trace bytes read from flash: %d", read_offset);
}
