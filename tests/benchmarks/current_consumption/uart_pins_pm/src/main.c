/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

#if DT_NODE_EXISTS(DT_NODELABEL(dut))
#define UART_NODE DT_NODELABEL(dut)
#else
#error Improper device tree configuration, UARTE test node not available
#endif

#define SLEEP_TIME_MS		    1000
#define UART_ACTION_BASE_TIMEOUT_US 1000
#define TEST_BUFFER_LEN		    10

static uint8_t test_buffer[TEST_BUFFER_LEN];
const uint8_t test_pattern[TEST_BUFFER_LEN] = {0x11, 0x12, 0x13, 0x14, 0x15,
					       0x16, 0x17, 0x18, 0x19, 0x20};

static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

const struct gpio_dt_spec test_pins[] = {DT_FOREACH_PROP_ELEM_SEP(
	DT_NODELABEL(test_in_gpios), gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))};

static void async_uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		printk("UART_TX_DONE\n");
		break;
	case UART_TX_ABORTED:
		printk("UART_TX_ABORTED\n");
		break;
	case UART_RX_RDY:
		printk("UART_RX_RDY\n");
		break;
	case UART_RX_BUF_RELEASED:
		printk("UART_RX_BUF_RELEASED\n");
		break;
	case UART_RX_BUF_REQUEST:
		printk("UART_RX_BUF_REQUEST\n");
		break;
	case UART_RX_DISABLED:
		printk("UART_RX_DISABLED\n");
		break;
	default:
		break;
	}
}

void enable_uart_rx(void)
{
	int err;

	err = uart_rx_enable(uart_dev, test_buffer, TEST_BUFFER_LEN,
			     5 * UART_ACTION_BASE_TIMEOUT_US);
	if (err != 0) {
		printk("Unexpected error when enabling UART RX: %d\n", err);
	}
}

void disable_uart_rx(void)
{
	k_busy_wait(5000);
	uart_rx_disable(uart_dev);
	k_busy_wait(5000);
}

void configure_test_pins(void)
{
	int err;

	for (int i = 0; i < ARRAY_SIZE(test_pins); i++) {
		__ASSERT(gpio_is_ready_dt(&test_pins[i]) == 1, "Test pin %u is not ready\n", i);
		err = gpio_pin_configure_dt(&test_pins[i], GPIO_INPUT);
		__ASSERT(err == 0, "Test pin %u configuration failed\n: %d", i, err);
	}
}

int main(void)
{
	int err;
	uint8_t pin_state;

	printk("UART instance: %s\n", uart_dev->name);

	err = gpio_is_ready_dt(&led);
	__ASSERT(err == 1, "Error: LED gpio is not ready");

	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(err == 0, "Could not configure led GPIO");

	err = uart_callback_set(uart_dev, async_uart_callback, NULL);
	__ASSERT(err == 0, "UART callback set failed: %d\n", err);

	pm_device_runtime_enable(console_dev);

	configure_test_pins();
	gpio_pin_set_dt(&led, 0);

	while (1) {
		enable_uart_rx();
		uart_tx(uart_dev, test_pattern, TEST_BUFFER_LEN, UART_ACTION_BASE_TIMEOUT_US);
		disable_uart_rx();

		pm_device_runtime_put(uart_dev);

		/* Suspending UART device must release its pins */
		for (int i = 0; i < ARRAY_SIZE(test_pins); i++) {
			k_busy_wait(1000);
			pin_state = gpio_pin_get_dt(&test_pins[i]);
			printk("Pin %u, state: %u\n", i, pin_state);
			__ASSERT(pin_state == 0, "Test pin %u is in HIGH state\n", i);
		}

		gpio_pin_set_dt(&led, 0);
		k_msleep(SLEEP_TIME_MS);
		gpio_pin_set_dt(&led, 1);
		pm_device_runtime_get(uart_dev);
	}

	return 0;
}
