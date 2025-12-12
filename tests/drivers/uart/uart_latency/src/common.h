/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/uart.h>

#define UART_TIMEOUT_US		       5000000
#define MAX_BUFFER_SIZE		       4096
#define TEST_TIMER_COUNT_TIME_LIMIT_MS 500
#define MAX_TOLERANCE		       2.0
#define MEASUREMENT_REPEATS	       10

#define UART_BAUD_9k6	9600
#define UART_BAUD_115k2 115200
#define UART_BAUD_921k6 921600
#define UART_BAUD_10M 10000000

#if DT_NODE_EXISTS(DT_NODELABEL(dut))
#define UART_NODE DT_NODELABEL(dut)
#else
#error Improper device tree configuration, UART test node not available
#endif

typedef struct {
	uint8_t *tx_buffer;
	uint8_t *rx_buffer;
	size_t buffer_size;

} uart_test_data;

void set_test_pattern(uart_test_data *test_data);

void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms);

uint32_t calculate_theoretical_transsmison_time_us(struct uart_config *tst_uart_config,
						   size_t buffer_size);

void check_transmitted_data(uart_test_data *test_data);
