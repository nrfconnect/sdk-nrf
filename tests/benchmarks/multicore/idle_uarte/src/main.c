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
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/gpio.h>

/* Note: logging is normally disabled for this test
 * Enable only for debugging purposes
 */
LOG_MODULE_REGISTER(idle_uarte);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

#if DT_NODE_EXISTS(DT_NODELABEL(dut))
#define UART_NODE DT_NODELABEL(dut)
#else
#error Improper device tree configuration, UARTE test node not available
#endif

#define UART_ACTION_BASE_TIMEOUT_US 1000
#define TEST_BUFFER_LEN		    10

static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);
static const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

const uint8_t test_pattern[TEST_BUFFER_LEN] = {0x11, 0x12, 0x13, 0x14, 0x15,
					       0x16, 0x17, 0x18, 0x19, 0x20};
static uint8_t test_buffer[TEST_BUFFER_LEN];
static volatile uint8_t uart_error_counter;

#if defined(CONFIG_CLOCK_CONTROL)
const struct nrf_clock_spec clk_spec_global_hsfll = {
	.frequency = MHZ(CONFIG_GLOBAL_DOMAIN_CLOCK_FREQUENCY_MHZ)
};

/*
 * Set Global Domain frequency (HSFLL120)
 * based on: CONFIG_GLOBAL_DOMAIN_CLOCK_FREQUENCY_MHZ
 */
void set_global_domain_frequency(void)
{
	int err;
	int res;
	struct onoff_client cli;
	const struct device *hsfll_dev = DEVICE_DT_GET(DT_NODELABEL(hsfll120));

	printk("Requested frequency [Hz]: %d\n", clk_spec_global_hsfll.frequency);
	sys_notify_init_spinwait(&cli.notify);
	err = nrf_clock_control_request(hsfll_dev, &clk_spec_global_hsfll, &cli);
	printk("Return code: %d\n", err);
	__ASSERT_NO_MSG(err < 3);
	__ASSERT_NO_MSG(err >= 0);
	do {
		err = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (err == -EAGAIN);
	printk("Clock control request return value: %d\n", err);
	printk("Clock control request response code: %d\n", res);
	__ASSERT_NO_MSG(err == 0);
	__ASSERT_NO_MSG(res == 0);
}
#endif /* CONFIG_CLOCK_CONTROL */

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
		for (int index = 0; index < TEST_BUFFER_LEN; index++) {
			printk("test_pattern[%d]=%d\n", index, test_pattern[index]);
			printk("test_buffer[%d]=%d\n", index, test_buffer[index]);
			if (test_buffer[index] != test_pattern[index]) {
				printk("Recieived data byte %d does not match pattern 0x%x != "
				       "0x%x\n",
				       index, test_buffer[index], test_pattern[index]);
				__ASSERT_NO_MSG(test_buffer[index] == test_pattern[index]);
			}
		}
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

	printk("Enable UART RX\n");
	err = uart_rx_enable(uart_dev, test_buffer, TEST_BUFFER_LEN,
			     5 * UART_ACTION_BASE_TIMEOUT_US);
	if (err != 0) {
		printk("Unexpected error when enabling UART RX: %d\n", err);
	}
}

/* Helper function for disabling UART RX */
void disable_uart_rx(void)
{
	int err;

	k_msleep(250);
	printk("Disable UART RX\n");
	err = uart_rx_disable(uart_dev);
	if (err != 0) {
		printk("Unexpected error when disabling RX: %d\n", err);
	}
	k_msleep(250);
}

int main(void)
{
	int err;
	int test_repetitions = 3;
	struct uart_config test_uart_config = {.baudrate = 115200,
					       .parity = UART_CFG_PARITY_ODD,
					       .stop_bits = UART_CFG_STOP_BITS_1,
					       .data_bits = UART_CFG_DATA_BITS_8,
					       .flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS};

	err = gpio_is_ready_dt(&led);
	__ASSERT(err, "Error: GPIO Device not ready");

#if defined(CONFIG_CLOCK_CONTROL)
	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	__ASSERT(err == 0, "Could not configure led GPIO");
	k_msleep(1000);
	gpio_pin_set_dt(&led, 1);
	set_global_domain_frequency();
#else
	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(err == 0, "Could not configure led GPIO");
#endif

	printk("Hello World! %s\n", CONFIG_BOARD_TARGET);
	printk("UART instance: %s\n", uart_dev->name);

	k_msleep(250);

	err = uart_configure(uart_dev, &test_uart_config);
	if (err != 0) {
		printk("Unexpected error when configuring UART: %d\n", err);
		return -1;
	}

	/* UART is disabled so expect error. */
	err = uart_rx_disable(uart_dev);
	if (err != -EFAULT) {
		printk("Unexpected error when disabling RX: %d\n", err);
		return -1;
	}

	err = uart_callback_set(uart_dev, async_uart_callback, NULL);
	if (err != 0) {
		printk("Unexpected error when setting callback %d\n", err);
		return -1;
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_enable(uart_dev);
		pm_device_runtime_enable(console_dev);
	}

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis enabled\n");
	while (test_repetitions--)
#else
	while (test_repetitions)
#endif
	{
		printk("Hello\n");
		enable_uart_rx();
		printk("UART test transmission\n");
		err = uart_tx(uart_dev, test_pattern, TEST_BUFFER_LEN, UART_ACTION_BASE_TIMEOUT_US);
		if (err != 0) {
			printk("Unexpected error when sending UART TX data: %d\n", err);
			return -1;
		}
		disable_uart_rx();
		printk("Good night\n");
		gpio_pin_set_dt(&led, 0);
		k_msleep(2000);
		gpio_pin_set_dt(&led, 1);
	}

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis start\n");
#endif
	return 0;
}
