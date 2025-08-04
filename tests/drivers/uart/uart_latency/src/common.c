/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));

uint8_t tx_test_buffer[MAX_BUFFER_SIZE];
uint8_t rx_test_buffer[MAX_BUFFER_SIZE];

void set_test_pattern(uart_test_data *test_data)
{
	for (uint32_t counter = 0; counter < test_data->buffer_size; counter++) {
		test_data->tx_buffer[counter] = (uint8_t)(counter & 0xFF);
		test_data->rx_buffer[counter] = 0xFF;
	}
}

void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms)
{
	struct counter_alarm_cfg counter_cfg;

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)count_time_ms * 1000);
	counter_cfg.user_data = &counter_cfg;
}

uint32_t calculate_theoretical_transsmison_time_us(struct uart_config *tst_uart_config,
						   size_t buffer_size)
{
	uint32_t number_of_bits;
	double ratio;

	number_of_bits = 1 + (tst_uart_config->data_bits + 5) + tst_uart_config->stop_bits;
	ratio = 1000000.0 / tst_uart_config->baudrate;
	return (uint32_t)(ratio * (double)number_of_bits * (double)buffer_size);
}

void check_transmitted_data(uart_test_data *test_data)
{
	for (int index = 0; index < test_data->buffer_size; index++) {
		zassert_equal(test_data->rx_buffer[index], test_data->tx_buffer[index],
			      "Received data byte %d does not match pattern 0x%x != 0x%x", index,
			      test_data->rx_buffer[index], test_data->tx_buffer[index]);
	}
}
