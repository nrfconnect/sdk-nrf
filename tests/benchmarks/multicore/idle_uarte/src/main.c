/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

/* Note: logging is normally disabled for this test
 * Enable only for debugging purposes
 */
LOG_MODULE_REGISTER(idle_uarte);

#if DT_NODE_EXISTS(DT_NODELABEL(dut))
#define UART_NODE DT_NODELABEL(dut)
#else
#error Improper device tree configuration, UARTE test node not available
#endif

#define TEST_BUFFER_LEN 512

static K_SEM_DEFINE(uart_rx_ready_sem, 0, 1);
static K_SEM_DEFINE(uart_tx_done_sem, 0, 1);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);
static const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

uint8_t test_pattern[TEST_BUFFER_LEN];
static uint8_t test_buffer[TEST_BUFFER_LEN];
static volatile uint8_t uart_error_counter;

void timer_handler(struct k_timer *dummy)
{
	k_sem_give(&uart_tx_done_sem);
}

K_TIMER_DEFINE(timer, timer_handler, NULL);

/*
 * Callback function for UART async transmission
 */
static void async_uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	printk("Callback !\n");
	switch (evt->type) {
	case UART_TX_DONE:
		printk("UART_TX_DONE\n");
		break;
	case UART_TX_ABORTED:
		printk("UART_TX_ABORTED\n");
		printk("Callback should not enter here\n");
		__ASSERT_NO_MSG(1 == 0);
		break;
	case UART_RX_RDY:
		printk("UART_RX_RDY\n");
		k_sem_give(&uart_rx_ready_sem);
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

/* Helper function for enabling UART RX */
void enable_uart_rx(void)
{
	int err;
	do {
		err = uart_rx_enable(uart_dev, test_buffer, TEST_BUFFER_LEN, 1000);
	} while (err == -EBUSY);
	__ASSERT(err == 0, "Unexpected error when enabling UART RX: %d", err);
}

void set_test_pattern(void)
{
	for (uint32_t counter = 0; counter < TEST_BUFFER_LEN; counter++) {
		test_pattern[counter] = (uint8_t)(counter & 0xFF);
	}
}

int main(void)
{
	int err;
	uint8_t switch_flag;
	uint32_t counter;
	struct uart_config test_uart_config;

	err = gpio_is_ready_dt(&led);
	__ASSERT(err, "Error: GPIO Device not ready");

	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(err == 0, "Could not configure led GPIO");

	printk("Hello World! %s\n", CONFIG_BOARD_TARGET);
	printk("UART instance: %s\n", uart_dev->name);
	set_test_pattern();
	err = uart_config_get(uart_dev, &test_uart_config);
	__ASSERT(err == 0, "Failed to get uart config");
	gpio_pin_set_dt(&led, 0);
	k_msleep(10);
	gpio_pin_set_dt(&led, 1);

	err = uart_configure(uart_dev, &test_uart_config);
	__ASSERT(err == 0, "Unexpected error when configuring UART: %d", err);

	/* UART is disabled so expect error. */
	err = uart_rx_disable(uart_dev);
	__ASSERT(err == -EFAULT, "Expected an error when disabling RX");

	err = uart_callback_set(uart_dev, async_uart_callback, NULL);
	__ASSERT(err == 0, "Unexpected error when setting callback %d", err);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_enable(uart_dev);
		pm_device_runtime_enable(console_dev);
	}

	counter = 0;
	while (1) {
		switch_flag = 1;
		k_timer_start(&timer, K_SECONDS(1), K_NO_WAIT);
		while (k_sem_take(&uart_tx_done_sem, K_NO_WAIT) != 0) {
			enable_uart_rx();
			do {
				err = uart_tx(uart_dev, test_pattern, TEST_BUFFER_LEN, 1000000);
			} while (err == -EBUSY);
			__ASSERT(err == 0, "Unexpected error when sending UART TX data: %d", err);
			while (k_sem_take(&uart_rx_ready_sem, K_NO_WAIT) != 0) {
			};
			for (int index = 0; index < TEST_BUFFER_LEN; index++) {
				__ASSERT(test_buffer[index] == test_pattern[index],
					 "Recieived data byte %d does not match pattern 0x%x != "
					 "0x%x",
					 index, test_buffer[index], test_pattern[index]);
			}
		}
		gpio_pin_set_dt(&led, 0);
		k_msleep(1000);
		gpio_pin_set_dt(&led, 1);
#if defined(CONFIG_COVERAGE)
		if (switch_flag) {
			printk("Coverage analysis start\n");
			break;
		}
#endif
	}
	return 0;
}
