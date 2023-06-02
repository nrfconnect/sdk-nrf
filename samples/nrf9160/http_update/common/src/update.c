/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/toolchain.h>
#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>
#include <zephyr/net/socket.h>
#include <net/fota_download.h>
#include <nrf_socket.h>
#include <stdio.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/shell/shell.h>
#include "update.h"

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback sw0_cb;
static struct k_work fota_work;
static update_start_cb update_start;
static char filename[128] = {0};

int cert_provision(void)
{
	static const char cert[] = {
		#include "../cert/AmazonRootCA1"
	};
	BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

	int err;
	bool exists;

	err = modem_key_mgmt_exists(TLS_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		printk("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(TLS_SEC_TAG,
					    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			printk("Failed to delete existing certificate, err %d\n",
			       err);
		}
	}

	printk("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert,
				   sizeof(cert) - 1);
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

static int led_init(int num_leds)
{
	switch (num_leds) {
	case 0:
		return 0;
	case 2:
		if (!device_is_ready(led1.port)) {
			return -ENODEV;
		}

		gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
		__fallthrough;
	case 1:
		if (!device_is_ready(led0.port)) {
			return -ENODEV;
		}

		gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
		return 0;
	default:
		return -EINVAL;
	}
}

static void button_irq_disable(void)
{
	gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_DISABLE);
}

static void button_irq_enable(void)
{
	gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_TO_ACTIVE);
}

void dfu_button_pressed(const struct device *gpiob, struct gpio_callback *cb,
			uint32_t pins)
{
	k_work_submit(&fota_work);
	button_irq_disable();
}

static int button_init(void)
{
	int err;

	if (!device_is_ready(sw0.port)) {
		printk("SW0 GPIO port device not ready\n");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
	if (err < 0) {
		return err;
	}

	gpio_init_callback(&sw0_cb, dfu_button_pressed, BIT(sw0.pin));

	err = gpio_add_callback(sw0.port, &sw0_cb);
	if (err < 0) {
		printk("Unable to configure SW0 GPIO pin!\n");
		return err;
	}

	button_irq_enable();

	return 0;
}

/**@brief Configures modem to provide LTE link.
 *
 * Blocks until link is successfully established.
 */
static void modem_configure(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
	BUILD_ASSERT(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
			"This sample does not support auto init and connect");
	int err;

#if defined(CONFIG_USE_HTTPS)
	err = cert_provision();
	__ASSERT(err == 0, "Could not provision root CA to %d", TLS_SEC_TAG);
#endif /* CONFIG_USE_HTTPS */

	printk("LTE Link Connecting ...\n");
	err = lte_lc_init_and_connect();
	__ASSERT(err == 0, "LTE link could not be established.");
	printk("LTE Link Connected!\n");
#endif /* CONFIG_LTE_LINK_CONTROL */
}

static void fota_work_cb(struct k_work *work)
{
	ARG_UNUSED(work);

	if (update_start != NULL) {
		update_start();
	}
}

static int shell_download(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	update_sample_start();

	return 0;
}

static int shell_reboot(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Device will now reboot");
	sys_reboot(SYS_REBOOT_WARM);

	return 0;
}

SHELL_CMD_REGISTER(reset, NULL, "For rebooting device", shell_reboot);
SHELL_CMD_REGISTER(download, NULL, "For downloading modem  firmware", shell_download);

int update_sample_init(struct update_sample_init_params *params)
{
	int err;

	if (params == NULL || params->update_start == NULL || params->filename == NULL) {
		return -EINVAL;
	}

	k_work_init(&fota_work, fota_work_cb);

	update_start = params->update_start;
	strcpy(filename, params->filename);

	modem_configure();

	err = button_init();
	if (err != 0) {
		return err;
	}

	err = led_init(params->num_leds);
	if (err != 0) {
		return err;
	}

	return 0;
}

void update_sample_start(void)
{
	int err;

	/* Functions for getting the host and file */
	err = fota_download_start(CONFIG_DOWNLOAD_HOST, filename, SEC_TAG, 0, 0);
	if (err != 0) {
		update_sample_stop();
		printk("fota_download_start() failed, err %d\n", err);
	}
}

void update_sample_stop(void)
{
	button_irq_enable();
}

void update_sample_done(void)
{
#if !defined(CONFIG_LWM2M_CARRIER)
	lte_lc_deinit();
#endif
}
