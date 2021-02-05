/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/flash.h>
#include <nrf_modem.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_key_mgmt.h>
#include <net/socket.h>
#include <nrf_socket.h>
#include <stdio.h>
#include "update.h"

#define LED_PORT DT_GPIO_LABEL(DT_ALIAS(led0), gpios)

static const struct device *gpiob;
static struct gpio_callback gpio_cb;
static struct k_work fota_work;
static update_start_cb update_start;

#define SW0_PIN (DT_GPIO_PIN(DT_ALIAS(sw0), gpios))
#define SW0_FLAGS (DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios))
#define LED0_PIN (DT_GPIO_PIN(DT_ALIAS(led0), gpios))
#define LED0_FLAGS (GPIO_OUTPUT_ACTIVE | DT_GPIO_FLAGS(DT_ALIAS(led0), gpios))
#define LED1_PIN (DT_GPIO_PIN(DT_ALIAS(led1), gpios))
#define LED1_FLAGS (GPIO_OUTPUT_ACTIVE | DT_GPIO_FLAGS(DT_ALIAS(led1), gpios))

/**@brief Recoverable modem library error. */
void nrf_modem_recoverable_error_handler(uint32_t err)
{
	printk("Modem library recoverable error: %u\n", err);
}

int cert_provision(void)
{
	static const char cert[] = {
		#include "../cert/BaltimoreCyberTrustRoot"
	};
	BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

	int err;
	bool exists;
	uint8_t unused;

	err = modem_key_mgmt_exists(TLS_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists,
				    &unused);
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
	const struct device *dev;

	dev = device_get_binding(LED_PORT);
	if (dev == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return 1;
	}

	switch (num_leds) {
	case 0:
		return 0;
	case 2:
		gpio_pin_configure(dev, LED1_PIN, LED1_FLAGS);
		/* Fallthrough */
	case 1:
		gpio_pin_configure(dev, LED0_PIN, LED0_FLAGS);
		return 0;
	default:
		return -EINVAL;
	}
}

static void button_irq_disable(void)
{
	gpio_pin_interrupt_configure(gpiob, SW0_PIN, GPIO_INT_DISABLE);
}

static void button_irq_enable(void)
{
	gpio_pin_interrupt_configure(gpiob, SW0_PIN, GPIO_INT_EDGE_TO_ACTIVE);
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

	gpiob = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(sw0), gpios));
	if (gpiob == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return 1;
	}
	err = gpio_pin_configure(gpiob, SW0_PIN, GPIO_INPUT | SW0_FLAGS);
	if (err == 0) {
		gpio_init_callback(&gpio_cb, dfu_button_pressed, BIT(SW0_PIN));
		err = gpio_add_callback(gpiob, &gpio_cb);
	}
	if (err == 0) {
		button_irq_enable();
	}
	if (err != 0) {
		printk("Unable to configure SW0 GPIO pin!\n");
		return 1;
	}
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
#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)
	/* Initialize AT only if nrf_modem_lib_init() is manually
	 * called by the main application
	 */
	err = at_notif_init();
	__ASSERT(err == 0, "AT Notify could not be initialized.");
	err = at_cmd_init();
	__ASSERT(err == 0, "AT CMD could not be established.");
#if defined(CONFIG_USE_HTTPS)
	err = cert_provision();
	__ASSERT(err == 0, "Could not provision root CA to %d", TLS_SEC_TAG);
#endif
#endif
	printk("LTE Link Connecting ...\n");
	err = lte_lc_init_and_connect();
	__ASSERT(err == 0, "LTE link could not be established.");
	printk("LTE Link Connected!\n");
#endif
}

static void fota_work_cb(struct k_work *work)
{
	ARG_UNUSED(work);

	if (update_start != NULL) {
		update_start();
	}
}

int update_sample_init(struct update_sample_init_params *params)
{
	int err;

	if (params == NULL || params->update_start == NULL) {
		return -EINVAL;
	}

	k_work_init(&fota_work, fota_work_cb);

	update_start = params->update_start;

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

void update_sample_done(void)
{
	button_irq_enable();
}
