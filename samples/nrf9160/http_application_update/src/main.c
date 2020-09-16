/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/flash.h>
#include <bsd.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/bsdlib.h>
#include <modem/modem_key_mgmt.h>
#include <net/fota_download.h>
#include <dfu/mcuboot.h>

#define LED_PORT	DT_GPIO_LABEL(DT_ALIAS(led0), gpios)
#define TLS_SEC_TAG 42

static const struct	device *gpiob;
static struct		gpio_callback gpio_cb;
static struct k_work	fota_work;


/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %u\n", err);
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
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists, &unused);
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
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

/**@brief Start transfer of the file. */
static void app_dfu_transfer_start(struct k_work *unused)
{
	int retval;
	int sec_tag;
	char *apn = NULL;

#ifndef CONFIG_USE_HTTPS
	sec_tag = -1;
#else
	sec_tag = TLS_SEC_TAG;
#endif

	retval = fota_download_start(CONFIG_DOWNLOAD_HOST,
				     CONFIG_DOWNLOAD_FILE,
				     sec_tag,
				     apn,
				     0);
	if (retval != 0) {
		/* Re-enable button callback */
		gpio_pin_interrupt_configure(gpiob,
					     DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
					     GPIO_INT_EDGE_TO_ACTIVE);

		printk("fota_download_start() failed, err %d\n",
			retval);
	}

}

/**@brief Turn on LED0 and LED1 if CONFIG_APPLICATION_VERSION
 * is 2 and LED0 otherwise.
 */
static int led_app_version(void)
{
	const struct device *dev;

	dev = device_get_binding(LED_PORT);
	if (dev == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return 1;
	}

	gpio_pin_configure(dev, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
			   GPIO_OUTPUT_ACTIVE |
			   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios));

#if CONFIG_APPLICATION_VERSION == 2
	gpio_pin_configure(dev, DT_GPIO_PIN(DT_ALIAS(led1), gpios),
			   GPIO_OUTPUT_ACTIVE |
			   DT_GPIO_FLAGS(DT_ALIAS(led1), gpios));
#endif
	return 0;
}

void dfu_button_pressed(const struct device *gpiob, struct gpio_callback *cb,
			uint32_t pins)
{
	k_work_submit(&fota_work);
	gpio_pin_interrupt_configure(gpiob, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				     GPIO_INT_DISABLE);
}

static int dfu_button_init(void)
{
	int err;

	gpiob = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(sw0), gpios));
	if (gpiob == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return 1;
	}
	err = gpio_pin_configure(gpiob, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				 GPIO_INPUT |
				 DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios));
	if (err == 0) {
		gpio_init_callback(&gpio_cb, dfu_button_pressed,
			BIT(DT_GPIO_PIN(DT_ALIAS(sw0), gpios)));
		err = gpio_add_callback(gpiob, &gpio_cb);
	}
	if (err == 0) {
		err = gpio_pin_interrupt_configure(gpiob,
						   DT_GPIO_PIN(DT_ALIAS(sw0),
							       gpios),
						   GPIO_INT_EDGE_TO_ACTIVE);
	}
	if (err != 0) {
		printk("Unable to configure SW0 GPIO pin!\n");
		return 1;
	}
	return 0;
}


void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		printk("Received error from fota_download\n");
		/* Fallthrough */
	case FOTA_DOWNLOAD_EVT_FINISHED:
		/* Re-enable button callback */
		gpio_pin_interrupt_configure(gpiob,
					     DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
					     GPIO_INT_EDGE_TO_ACTIVE);
		break;

	default:
		break;
	}
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
#if !defined(CONFIG_BSD_LIBRARY_SYS_INIT)
	/* Initialize AT only if bsdlib_init() is manually
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

static int application_init(void)
{
	int err;

	k_work_init(&fota_work, app_dfu_transfer_start);

	err = dfu_button_init();
	if (err != 0) {
		return err;
	}

	err = led_app_version();
	if (err != 0) {
		return err;
	}

	err = fota_download_init(fota_dl_handler);
	if (err != 0) {
		return err;
	}

	return 0;
}

void main(void)
{
	int err;

	printk("HTTP application update sample started\n");
	printk("Initializing bsdlib\n");
#if !defined(CONFIG_BSD_LIBRARY_SYS_INIT)
	err = bsdlib_init();
#else
	/* If bsdlib is initialized on post-kernel we should
	 * fetch the returned error code instead of bsdlib_init
	 */
	err = bsdlib_get_init_ret();
#endif
	switch (err) {
	case MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem will run the new firmware after reboot\n");
		k_thread_suspend(k_current_get());
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed\n");
		printk("Modem will run non-updated firmware on reboot.\n");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed\n");
		printk("Fatal error.\n");
		break;
	case -1:
		printk("Could not initialize bsdlib.\n");
		printk("Fatal error.\n");
		return;
	default:
		break;
	}
	printk("Initialized bsdlib\n");

	modem_configure();

	boot_write_img_confirmed();

	err = application_init();
	if (err != 0) {
		return;
	}

	printk("Press Button 1 to start the FOTA download\n");
}
