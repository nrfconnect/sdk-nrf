/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#define RESET_NODE DT_NODELABEL(nrf52840_reset)
#define BOOT_NODE DT_NODELABEL(nrf52840_boot)

#if DT_NODE_HAS_STATUS(RESET_NODE, okay) && DT_NODE_HAS_STATUS(BOOT_NODE, okay)

#define RESET_GPIO_CTRL  DT_GPIO_CTLR(RESET_NODE, gpios)
#define RESET_GPIO_PIN   DT_GPIO_PIN(RESET_NODE, gpios)
#define RESET_GPIO_FLAGS DT_GPIO_FLAGS(RESET_NODE, gpios)

#define BOOT_GPIO_CTRL  DT_GPIO_CTLR(BOOT_NODE, gpios)
#define BOOT_GPIO_PIN   DT_GPIO_PIN(BOOT_NODE, gpios)
#define BOOT_GPIO_FLAGS DT_GPIO_FLAGS(BOOT_NODE, gpios)

#define WAIT_BOOT_INTERVAL_MS 50

static int nrf52840_reset_assert(bool boot_select)
{
	const struct device *reset_port = DEVICE_DT_GET(RESET_GPIO_CTRL);
	const struct device *boot_port = DEVICE_DT_GET(BOOT_GPIO_CTRL);
	int err;

	if (!reset_port) {
		printk("reset_port not found!\n");
		return -EIO;
	}

	if (!boot_port) {
		printk("boot_port not found!\n");
		return -EIO;
	}

	if (!device_is_ready(reset_port)) {
		printk("reset_port is not ready!\n");
		return -EIO;
	}

	if (!device_is_ready(boot_port)) {
		printk("boot_port is not ready!\n");
		return -EIO;
	}

	/* Configure pin as output and initialize it to low. */
	err = gpio_pin_configure(reset_port, RESET_GPIO_PIN, GPIO_OUTPUT_LOW);
	if (err) {
		printk("Reset pin could not be configured! %d\n", err);
		return err;
	}

	if (boot_select) {
		printk("Resetting nrf52840 in MCUboot USB serial update mode\n");
		/* delay to ensure logging finishes before reset */
		k_sleep(K_SECONDS(2));
	} else {
		printk("Resetting nrf52840 for normal ops.\n");
	}
	err = gpio_pin_configure(boot_port, BOOT_GPIO_PIN, GPIO_OUTPUT |
				 GPIO_OPEN_DRAIN | GPIO_PULL_UP);
	if (err) {
		printk("Boot pin could not be configured! %d\n", err);
		return err;
	}

	err = gpio_pin_set(boot_port, BOOT_GPIO_PIN, boot_select ? 0 : 1);
	if (err) {
		printk("Boot pin could not be set! %d\n", err);
		return err;
	}

	err = gpio_pin_set(reset_port, RESET_GPIO_PIN, 1);
	if (err) {
		printk("Reset pin could not be reset! %d\n", err);
		return err;
	}
	printk("Reset the 52840 with boot set to %d\n", boot_select);
	return 0;
}

static int nrf52840_boot_select_deassert(void)
{
	const struct device *boot_port = DEVICE_DT_GET(BOOT_GPIO_CTRL);
	int err;

	if (!boot_port) {
		printk("boot_port not found!\n");
		return -EIO;
	}

	if (!device_is_ready(boot_port)) {
		printk("boot_port is not ready!\n");
		return -EIO;
	}

	err = gpio_pin_set(boot_port, BOOT_GPIO_PIN, 1);
	if (err) {
		printk("Boot pin could not be set! %d\n", err);
		return err;
	}

	k_sleep(K_MSEC(10));

	err = gpio_pin_configure(boot_port, BOOT_GPIO_PIN,
				 GPIO_INPUT | GPIO_PULL_UP);
	if (err) {
		printk("Boot pin could not be configured! %d\n", err);
		return err;
	}
	printk("Deasserted 52840 boot pin\n");
	return 0;
}

static int nrf52840_reset_deassert(void)
{
	const struct device *reset_port = DEVICE_DT_GET(RESET_GPIO_CTRL);
	int err;

	if (!reset_port) {
		printk("reset_port not found!\n");
		return -EIO;
	}

	if (!device_is_ready(reset_port)) {
		printk("reset_port is not ready!\n");
		return -EIO;
	}

	err = gpio_pin_set(reset_port, RESET_GPIO_PIN, 0);
	if (err) {
		printk("Reset pin could not be reset! %d\n", err);
		return err;
	}
	printk("Deasserted 52840 reset\n");
	return 0;
}

int nrf52840_wait_boot_complete(int timeout_ms)
{
	const struct device *boot_port = DEVICE_DT_GET(BOOT_GPIO_CTRL);
	int total_time;
	int err;

	if (!boot_port) {
		printk("boot_port not found!\n");
		return -EIO;
	}

	if (!device_is_ready(boot_port)) {
		printk("boot_port is not ready!\n");
		return -EIO;
	}

	/* make the boot select pin a pulled up input, so we can
	 * read it to see when the 52840 has rebooted into its application
	 */
	err = gpio_pin_configure(boot_port, BOOT_GPIO_PIN, GPIO_INPUT | GPIO_PULL_UP);
	if (err) {
		printk("Boot pin could not be configured! %d\n", err);
		return err;
	}

	printk("Waiting for 52840 to boot...\n");
	total_time = 0;
	do {
		k_sleep(K_MSEC(WAIT_BOOT_INTERVAL_MS));
		total_time += WAIT_BOOT_INTERVAL_MS;
		err = gpio_pin_get_raw(boot_port, BOOT_GPIO_PIN);
		if (err < 0) {
			return err;
		}
		if (err && (timeout_ms > 0) && (total_time > timeout_ms)) {
			return -ETIMEDOUT;
		}
	} while (err == 1);

	printk("52840 boot is complete\n");
	return 0;
}

int nrf52840_reset_to_mcuboot(void)
{
	int err;

	printk("Reset 52840 into MCUboot mode:\n");
	err = nrf52840_reset_assert(true);
	if (err) {
		return err;
	}
	k_sleep(K_MSEC(10));

	err = nrf52840_reset_deassert();
	if (err) {
		return err;
	}

	k_sleep(K_SECONDS(5));
	return nrf52840_boot_select_deassert();
}

int bt_hci_transport_setup(struct device *h4)
{
	char c;
	int err;

	/* Reset the nRF52840 and let it wait until the pin is
	 * pulled low again before running to main to ensure
	 * that it won't send any data until the H4 device
	 * is setup and ready to receive.
	 */
	err = nrf52840_reset_assert(false);
	if (err) {
		return err;
	}

	/* Wait for the nRF52840 peripheral to stop sending data.
	 *
	 * It is critical (!) to wait here, so that all bytes
	 * on the lines are received and drained correctly.
	 */
	k_sleep(K_MSEC(10));

	/* Drain bytes */
	while (h4 && uart_fifo_read(h4, &c, 1)) {
		continue;
	}

	/* We are ready, let the nRF52840 run to main */
	nrf52840_reset_deassert();

	return 0;
}
#else
#warning "Reset and/or boot node is missing"
#endif /* DT_NODE_HAS_STATUS(RESET_NODE, okay) && DT_NODE_HAS_STATUS(BOOT_NODE, okay) */
