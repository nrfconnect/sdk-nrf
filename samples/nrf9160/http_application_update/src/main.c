/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <stdint.h>
#include <zephyr.h>
#include <flash.h>
#include <nrf_socket.h>
#include <download_client.h>
#include <logging/log.h>
#include <gpio.h>
#include <dfu/mcuboot.h>
#include <pm_config.h>

#define LED_PORT	LED0_GPIO_CONTROLLER

static struct		device	*flash_dev;
static bool		is_flash_page_erased[FLASH_PAGE_MAX_CNT];
static volatile bool	start_dfu;
static struct		device *gpiob;
static struct		gpio_callback gpio_cb;
static u32_t		flash_address;

static int app_download_client_event_handler(struct download_client *const dfu,
			enum download_client_evt event, u32_t status);


static struct download_client dfu = {
	.host = CONFIG_DOWNLOAD_HOST,
	.resource = CONFIG_DOWNLOAD_FILE,
	.callback = app_download_client_event_handler
};


/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %u\n", err);
}

/**@brief Irrecoverable BSD library error. */
void bsd_irrecoverable_error_handler(uint32_t err)
{
	printk("bsdlib irrecoverable error: %u\n", err);

	__ASSERT_NO_MSG(false);
}


/**@brief Initialize application. */
static int app_dfu_init(void)
{
	int i;

	flash_address = PM_MCUBOOT_SECONDARY_ADDRESS;
	for (i = 0; i < FLASH_PAGE_MAX_CNT; i++) {
		is_flash_page_erased[i] = false;
	}
	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	if (flash_dev == 0) {
		printk("Nordic nRF flash driver was not found!\n");
		return 1;
	}

	int retval = download_client_init(&dfu);

	if (retval != 0) {
		printk("download_client_init() failed, err %d", retval);
		return 1;
	}
	return 0;
}


/**@brief Start transfer of the file. */
static int app_dfu_transfer_start(void)
{
	int retval = download_client_connect(&dfu);

	if (retval != 0) {
		printk("download_client_connect() failed, err %d",
			retval);
		return 1;
	}

	retval = download_client_start(&dfu);
	if (retval != 0) {
		printk("download_client_start() failed, err %d",
			retval);
		return 1;
	}
	return 0;
}

static int flash_page_erase_if_needed(u32_t address)
{
	int err;
	struct flash_pages_info info;

	err = flash_get_page_info_by_offs(flash_dev, address,
		&info);
	if (err != 0) {
		printk("flash_get_page_info_by_offs returned error %d\n",
			err);
		return 1;
	}
	if (!is_flash_page_erased[info.index]) {
		err = flash_write_protection_set(flash_dev, false);
		if (err != 0) {
			printk("flash_write_protection_set returned error %d\n",
				err);
			return 1;
		}
		err = flash_erase(flash_dev, info.start_offset, info.size);
		if (err != 0) {
			printk("flash_erase returned error %d at address %08x\n",
				err, info.start_offset);
			return 1;
		}
		is_flash_page_erased[info.index] = true;
		err = flash_write_protection_set(flash_dev, true);
		if (err != 0) {
			printk("flash_write_protection_set returned error %d\n",
				err);
			return 1;
		}
	}
	return 0;
}


static int app_download_client_event_handler(
	struct download_client *const dfu,
	enum download_client_evt event, u32_t error)
{
	int err;

	switch (event) {
	case DOWNLOAD_CLIENT_EVT_DOWNLOAD_FRAG: {

		if (dfu->object_size > PM_MCUBOOT_SECONDARY_SIZE) {
			printk("Requested file too big to fit in flash\n");
			return 1;
		}
		err = flash_page_erase_if_needed(flash_address);
		if (err != 0) {
			return 1;
		}
		err = flash_write_protection_set(flash_dev, false);
		if (err != 0) {
			printk("flash_write_protection_set returned error %d\n",
				err);
			return 1;
		}
		err = flash_write(flash_dev, flash_address, dfu->fragment,
							dfu->fragment_size);
		if (err != 0) {
			printk("Flash write error %d at address %08x\n",
				err, flash_address);
			return 1;
		}
		err = flash_write_protection_set(flash_dev, true);
		if (err != 0) {
			printk("flash_write_protection_set returned error %d\n",
				err);
			return 1;
		}
		flash_address += dfu->fragment_size;
		break;
	}

	case DOWNLOAD_CLIENT_EVT_DOWNLOAD_DONE:
		flash_address = PM_MCUBOOT_SECONDARY_ADDRESS
			+PM_MCUBOOT_SECONDARY_SIZE-0x4;
		err = flash_page_erase_if_needed(flash_address);
		if (err != 0) {
			return 1;
		}
		err = boot_request_upgrade(0);
		if (err != 0) {
			printk("boot_request_upgrade returned error %d\n", err);
			return 1;
		}
		printk("DOWNLOAD_CLIENT_EVT_DOWNLOAD_DONE");
		break;

	case DOWNLOAD_CLIENT_EVT_ERROR: {
		download_client_disconnect(dfu);
		printk("Download client error, please restart "
			"the application\n");
		break;
	}
	default:
		break;
	}
	return 0;
}


/**@brief Turn on LED0 and LED1 if CONFIG_APPLICATION_VERSION
 * is 2 and LED0 otherwise.
 */
static int led_app_version(void)
{
	struct device *dev;

	dev = device_get_binding(LED_PORT);
	if (dev == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return 1;
	}

	gpio_pin_configure(dev, LED0_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(dev, LED0_GPIO_PIN, 1);

#if CONFIG_APPLICATION_VERSION == 2
	gpio_pin_configure(dev, LED1_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(dev, LED1_GPIO_PIN, 1);
#endif
	return 0;
}

void dfu_button_pressed(struct device *gpiob, struct gpio_callback *cb,
		    u32_t pins)
{
	start_dfu = true;
	gpio_pin_disable_callback(gpiob, SW0_GPIO_PIN);
}

static int dfu_button_init(void)
{
	int err;

	gpiob = device_get_binding(SW0_GPIO_CONTROLLER);
	if (gpiob == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return 1;
	}
	err = gpio_pin_configure(gpiob, SW0_GPIO_PIN,
		GPIO_DIR_IN		|
		GPIO_INT		|
		GPIO_PUD_PULL_UP|
		GPIO_INT_EDGE	|
		GPIO_INT_ACTIVE_LOW);
	if (err == 0) {
		gpio_init_callback(&gpio_cb, dfu_button_pressed,
			BIT(SW0_GPIO_PIN));
		err = gpio_add_callback(gpiob, &gpio_cb);
	}
	if (err == 0) {
		err = gpio_pin_enable_callback(gpiob, SW0_GPIO_PIN);
	}
	if (err != 0) {
		printk("Unable to configure SW0_GPIO_PIN!\n");
		return 1;
	}
	return 0;
}

static int application_update(void)
{
	int err;

	start_dfu = false;
	err = app_dfu_init();
	if (err != 0) {
		return 1;
	}

	err = app_dfu_transfer_start();
	if (err != 0) {
		return 1;
	}

	while (true) {
		if ((dfu.status == DOWNLOAD_CLIENT_STATUS_HALTED) ||
			(dfu.status == DOWNLOAD_CLIENT_ERROR)) {
			printk("Something went wrong, please restart the application\n");
			return 1;
		} else if (dfu.status ==
			DOWNLOAD_CLIENT_STATUS_DOWNLOAD_COMPLETE) {
			printk("Download complete\n");
			break;
		}
		download_client_process(&dfu);
	}
	download_client_disconnect(&dfu);
	return 0;
}


void main(void)
{
	int err;

	boot_write_img_confirmed();
	start_dfu = false;
	err = dfu_button_init();
	if (err != 0) {
		return;
	}
	err = led_app_version();
	if (err != 0) {
		return;
	}
	while (true) {
		if (start_dfu) {
			err = application_update();
			if (err != 0) {
				return;
			}
		}
	}
}
