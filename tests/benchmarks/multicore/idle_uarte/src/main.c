/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <nrfs_backend_ipc_service.h>
#include <nrfs_gdpwr.h>

/* Note: logging is normally disabled for this test
 * Enable only for debugging purposes
 */
LOG_MODULE_REGISTER(idle_uarte);

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

/* Required to power off the GD2 and GD3 domains
 * Will be removed when GD handling
 * is implemented in sdk-zephyr
 */
static void gdpwr_handler(nrfs_gdpwr_evt_t const *p_evt, void *context)
{
	switch (p_evt->type) {
	case NRFS_GDPWR_REQ_APPLIED:
		printk("GDPWR handler - response received: 0x%x, CTX=%d\n", p_evt->type,
		       (uint32_t)context);
		break;
	case NRFS_GDPWR_REQ_REJECTED:
		printk("GDPWR handler - request rejected: 0x%x, CTX=%d\n", p_evt->type,
		       (uint32_t)context);
		break;
	default:
		printk("GDPWR handler - unexpected event: 0x%x, CTX=%d\n", p_evt->type,
		       (uint32_t)context);
		break;
	}
}

/* Required to power off the GD2 and GD3 domains
 * Will be removed when GD handling
 * is implemented in sdk-zephyr
 */
static void clear_global_power_domains_requests(void)
{
	int service_status;
	int tst_ctx = 1;

	service_status = nrfs_gdpwr_init(gdpwr_handler);
	printk("Response: %d\n", service_status);
	printk("Sending GDPWR DISABLE request for: GDPWR_POWER_DOMAIN_ACTIVE_SLOW\n");
	service_status = nrfs_gdpwr_power_request(GDPWR_POWER_DOMAIN_ACTIVE_SLOW,
						  GDPWR_POWER_REQUEST_CLEAR, (void *)tst_ctx++);
	printk("Response: %d\n", service_status);
	printk("Sending GDPWR DISABLE request for: GDPWR_POWER_DOMAIN_ACTIVE_FAST\n");
	service_status = nrfs_gdpwr_power_request(GDPWR_POWER_DOMAIN_ACTIVE_FAST,
						  GDPWR_POWER_REQUEST_CLEAR, (void *)tst_ctx++);
	printk("Response: %d\n", service_status);
	printk("Sending GDPWR DISABLE request for: GDPWR_POWER_DOMAIN_MAIN_SLOW\n");
	service_status = nrfs_gdpwr_power_request(GDPWR_POWER_DOMAIN_MAIN_SLOW,
						  GDPWR_POWER_REQUEST_CLEAR, (void *)tst_ctx);
	printk("Response: %d\n", service_status);
}

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

int main(void)
{
	int err;
	struct uart_config test_uart_config = {.baudrate = 115200,
					       .parity = UART_CFG_PARITY_ODD,
					       .stop_bits = UART_CFG_STOP_BITS_1,
					       .data_bits = UART_CFG_DATA_BITS_8,
					       .flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS};

	printk("Hello World! %s\n", CONFIG_BOARD_TARGET);
	printk("UART instance: %s\n", uart_dev->name);

	nrfs_backend_wait_for_connection(K_FOREVER);
	clear_global_power_domains_requests();
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

	while (1) {
		printk("Hello\n");

		err = uart_rx_enable(uart_dev, test_buffer, TEST_BUFFER_LEN,
				     5 * UART_ACTION_BASE_TIMEOUT_US);
		if (err != 0) {
			printk("Unexpected error when enabling UART RX: %d\n", err);
			return -1;
		}
		err = uart_tx(uart_dev, test_pattern, TEST_BUFFER_LEN, UART_ACTION_BASE_TIMEOUT_US);
		if (err != 0) {
			printk("Unexpected error when sending UART TX data: %d\n", err);
			return -1;
		}
		k_msleep(250);
		uart_rx_disable(uart_dev);
		k_msleep(250);

		printk("Good night\n");
		k_msleep(2000);
	}

	return 0;
}
