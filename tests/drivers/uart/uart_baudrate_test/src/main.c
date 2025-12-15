/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>
#include <math.h>

static const struct gpio_dt_spec gpio_spec =
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), gpios, 0);
static const struct device *const uart_dev = DEVICE_DT_GET(DT_NODELABEL(dut));

static const uint8_t tx_buf[1] = {0x00};

#define PIN_STATE_SIZE 16384
static int pin_state[PIN_STATE_SIZE] = {};

#define REPEAT_NUMBER 10

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_fifo_callback(const struct device *dev, void *user_data)
{
	int ret;

	ARG_UNUSED(user_data);

	ret = uart_irq_update(dev);
	zassert_true(ret >= 0, "uart_irq_update: %d\n", ret);

	ret = uart_irq_tx_ready(dev);
	zassert_true(ret >= 0, "uart_irq_tx_ready: %d\n", ret);

	ret = uart_fifo_fill(dev, (uint8_t *)&tx_buf, sizeof(tx_buf));
	zassert_true(ret == sizeof(tx_buf), "uart_fifo_fill: %d\n", ret);
	uart_irq_tx_disable(dev);
}
#endif

static void check_timing(const struct gpio_dt_spec *gpio_dt, uint32_t baudrate)
{
	uint64_t cycles_s_sys;
	struct uart_config test_uart_config;
	int ret;
	uint32_t cycle_start_time;
	uint32_t cycle_stop_time;
	double gpio_read_time_us_mean;
	int32_t start_index;
	int32_t stop_index;
	int32_t start_index_count_zero;
	double bit_diviation_mean;
	double symbol_diviation_mean;
	int key;

	ret = uart_config_get(uart_dev, &test_uart_config);
	zassert_equal(ret, 0, "uart_config_get: %d\n", ret);

	test_uart_config.parity = UART_CFG_PARITY_EVEN;
	test_uart_config.stop_bits = UART_CFG_STOP_BITS_2;
	test_uart_config.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	test_uart_config.baudrate = baudrate;
	ret = uart_configure(uart_dev, &test_uart_config);
	if (ret == -ENOTSUP) {
		TC_PRINT("[%d] Not supported\n", baudrate);
		ztest_test_skip();
	}
	zassert_equal(ret, 0, "uart_configure: %d\n", ret);

	cycles_s_sys = (uint64_t)sys_clock_hw_cycles_per_sec();
	TC_PRINT("Cycles: %llu cycles\n", cycles_s_sys);

	/*
	 * Measure time needed to read gpios
	 */
	gpio_read_time_us_mean = 0;
	key = irq_lock();
	for (uint32_t t = 0; t < REPEAT_NUMBER; ++t) {
		cycle_start_time = k_cycle_get_32();
		for (uint32_t i = 0; i < PIN_STATE_SIZE; ++i) {
			pin_state[i] = gpio_pin_get_dt(&gpio_spec);
		}
		cycle_stop_time = k_cycle_get_32();
		double gpio_read_time_us =
			((cycle_stop_time - cycle_start_time) / (double)PIN_STATE_SIZE) *
			(1e6 / cycles_s_sys);
		gpio_read_time_us_mean += gpio_read_time_us;
	}
	irq_unlock(key);
	gpio_read_time_us_mean /= (double)REPEAT_NUMBER;
	TC_PRINT("GPIO get takes: %.2f us\n", gpio_read_time_us_mean);

	double expected_bit_period_us = 1e6 / (double)baudrate;
	double number_of_bits = 8;

	number_of_bits += 1;
	if (test_uart_config.stop_bits == UART_CFG_STOP_BITS_1) {
		number_of_bits += 1;
	} else if (test_uart_config.stop_bits == UART_CFG_STOP_BITS_2) {
		number_of_bits += 2;
	} else {
		zassert_true(false, "Unsupported stop_bits: %d", test_uart_config.stop_bits);
	}
	if (test_uart_config.parity != UART_CFG_PARITY_NONE) {
		number_of_bits += 1;
	}
	double expected_symbol_period_us = number_of_bits * expected_bit_period_us;

	TC_PRINT("[%d] Expected symbol time: %.2f us, expected bit time: %.2f us\n", baudrate,
		 expected_symbol_period_us, expected_bit_period_us);

	if (expected_bit_period_us < gpio_read_time_us_mean) {
		TC_PRINT("[%d] Not supported - gpio measurement is too slow.\n", baudrate);
		ztest_test_skip();
	}

	start_index_count_zero = 0;
	bit_diviation_mean = 0;
	symbol_diviation_mean = 0;
	for (uint32_t t = 0; t < REPEAT_NUMBER; ++t) {
		/*
		 * Send character
		 */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
		uart_irq_tx_enable(uart_dev);
#else
		uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 0);
#endif

		/*
		 * Check gpio
		 */
		key = irq_lock();
		for (uint32_t i = 0; i < PIN_STATE_SIZE; ++i) {
			pin_state[i] = gpio_pin_get_dt(&gpio_spec);
		}
		irq_unlock(key);

		/*
		 * Find start of start bit and end of stop bit
		 */
		start_index = -1;
		stop_index = -1;
		for (uint32_t i = 0; i < PIN_STATE_SIZE; ++i) {
			if (-1 == start_index) {
				if (0 == pin_state[i]) {
					start_index = i;
				}
			} else {
				if (-1 == stop_index) {
					if (1 == pin_state[i]) {
						stop_index = i;
					}
				} else {
					zassert_true(1 == pin_state[i], "Unexpected low at %d\n",
						     i);
				}
			}
		}
		TC_PRINT("Start index: %d, stop index: %d\n", start_index, stop_index);
		zassert_true(start_index != -1, "Missing start_index\n");
		zassert_true(stop_index != -1, "Missing stop_index\n");

		double measured_period_us = (stop_index - start_index) * gpio_read_time_us_mean;
		double measured_bit_us = measured_period_us / (double)number_of_bits;

		TC_PRINT("[%d][%s][%d] Measured symbol period: %.2f us, measured bit time: %.2f "
			 "us\n",
			 t, uart_dev->name, baudrate, measured_period_us, measured_bit_us);

		double symbol_diviation = 100 *
					  fabs(measured_period_us - expected_symbol_period_us) /
					  expected_symbol_period_us;
		double bit_diviation = 100 * fabs(measured_bit_us - expected_bit_period_us) /
				       expected_bit_period_us;

		TC_PRINT("[%d][%s][%d] Symbol diviation: %.2f%%, bit diviation: %.2f%%\n", t,
			 uart_dev->name, baudrate, symbol_diviation, bit_diviation);

		if (start_index == 0) {
			start_index_count_zero++;
		}
		symbol_diviation_mean += symbol_diviation;
		bit_diviation_mean += bit_diviation;
	}
	symbol_diviation_mean /= (double)REPEAT_NUMBER;
	bit_diviation_mean /= (double)REPEAT_NUMBER;

	TC_PRINT("[%s][%d] Mean symbol diviation: %.2f%%, mean bit diviation: %.2f%%\n",
		 uart_dev->name, baudrate, symbol_diviation_mean, bit_diviation_mean);

	if (start_index_count_zero == 0) {
		zassert_true(symbol_diviation_mean <= (double)CONFIG_ALLOWED_DEVIATION,
			     "Symbol diviation %0.f%%  higher than %d%%\n", symbol_diviation_mean,
			     CONFIG_ALLOWED_DEVIATION);
		zassert_true(bit_diviation_mean <= (double)CONFIG_ALLOWED_DEVIATION,
			     "Bit diviation %0.f%% higher than %d%%\n", bit_diviation_mean,
			     CONFIG_ALLOWED_DEVIATION);
	} else {
		TC_PRINT("Not checking diviation due to lost start of start bit\n");
	}
}

ZTEST(uart_baudrate_test, test_08_2400)
{
	check_timing(&gpio_spec, 2400);
}

ZTEST(uart_baudrate_test, test_09_4800)
{
	check_timing(&gpio_spec, 4800);
}

ZTEST(uart_baudrate_test, test_10_9600)
{
	check_timing(&gpio_spec, 9600);
}

ZTEST(uart_baudrate_test, test_11_14400)
{
	check_timing(&gpio_spec, 14400);
}
ZTEST(uart_baudrate_test, test_12_19200)
{
	check_timing(&gpio_spec, 19200);
}

ZTEST(uart_baudrate_test, test_13_38400)
{
	check_timing(&gpio_spec, 38400);
}

ZTEST(uart_baudrate_test, test_14_57600)
{
	check_timing(&gpio_spec, 57600);
}

ZTEST(uart_baudrate_test, test_15_115200)
{
	check_timing(&gpio_spec, 115200);
}

ZTEST(uart_baudrate_test, test_16_230400)
{
	check_timing(&gpio_spec, 230400);
}

ZTEST(uart_baudrate_test, test_17_460800)
{
	check_timing(&gpio_spec, 460800);
}

ZTEST(uart_baudrate_test, test_18_576000)
{
	check_timing(&gpio_spec, 576000);
}

ZTEST(uart_baudrate_test, test_19_921600)
{
	check_timing(&gpio_spec, 921600);
}

ZTEST(uart_baudrate_test, test_20_1000000)
{
	check_timing(&gpio_spec, 1000000);
}

ZTEST(uart_baudrate_test, test_21_2000000)
{
	check_timing(&gpio_spec, 2000000);
}

ZTEST(uart_baudrate_test, test_22_3000000)
{
	check_timing(&gpio_spec, 3000000);
}

ZTEST(uart_baudrate_test, test_23_4000000)
{
	check_timing(&gpio_spec, 4000000);
}

ZTEST(uart_baudrate_test, test_24_8000000)
{
	check_timing(&gpio_spec, 8000000);
}

static void *uart_baudrate_test_setup(void)
{
	int ret;

	/*
	 * Wait for any initialization to finish
	 */
	k_msleep(1000);

	zassert_true(gpio_is_ready_dt(&gpio_spec), "GPIO is not ready");
	ret = gpio_pin_configure_dt(&gpio_spec, GPIO_INPUT);
	zassert_true(ret == 0, "gpio_pin_configure_dt: %d", ret);
	zassert_true(device_is_ready(uart_dev), "UART device is not ready");
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	ret = uart_irq_callback_set(uart_dev, uart_fifo_callback);
	zassert_true(ret == 0, "uart_irq_callback_set: %d", ret);
#endif

	return NULL;
}

ZTEST_SUITE(uart_baudrate_test, NULL, uart_baudrate_test_setup, NULL, NULL, NULL);
