/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <app_update.h>
#include <dfu/flash_img.h>
#include <dfu/mcuboot.h>
#include <drivers/gpio.h>

struct k_work upgrade;

void upgrade_work(struct k_work *item){
	printk("Trying to request upgrade\n");
	int err = boot_request_upgrade(BOOT_UPGRADE_TEST);
	printk("upgrade requested: %d \n", err);
	if (err != 0) {
		__ASSERT(true, "reguest upgrade failed %d", err);
	}
	printk("Upgrade done\n");
}

void button_pressed(struct device *dev, struct gpio_callback *cb,
		    u32_t pins)
{
	printk("Button pressed\n");
	k_work_submit(&upgrade);

}

static struct gpio_callback button_cb_data;

void main(void)
{
	k_work_init(&upgrade, upgrade_work);
	struct device *dev_button;
	int ret;

	dev_button = device_get_binding(DT_ALIAS_SW0_GPIOS_CONTROLLER);
	if (dev_button == NULL) {
		printk("Error: didn't find %s device\n",
			DT_ALIAS_SW0_GPIOS_CONTROLLER);
		return;
	}
	ret = gpio_pin_configure(dev_button, DT_ALIAS_SW0_GPIOS_PIN,
				 DT_ALIAS_SW0_GPIOS_FLAGS | GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d '%s'\n",
			ret, DT_ALIAS_SW0_GPIOS_PIN, DT_ALIAS_SW0_LABEL);
		return;
	}
	ret = gpio_pin_interrupt_configure(dev_button, DT_ALIAS_SW0_GPIOS_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&button_cb_data, button_pressed,
			   BIT(DT_ALIAS_SW0_GPIOS_PIN));
	gpio_add_callback(dev_button, &button_cb_data);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on pin %d '%s'\n",
			ret, DT_ALIAS_SW0_GPIOS_PIN, DT_ALIAS_SW0_LABEL);
		return;
	}

	printk("Hello World! %s upgrader sample\n", CONFIG_BOARD);
	struct flash_img_context flash_img;
	int err = flash_img_init(&flash_img);
	if (err != 0) {
		__ASSERT(true, "flash_img_init error %d", err);
	}

	printk("Writing app to external flash: %d\n", app_update_bin_len);
	err = flash_img_buffered_write(&flash_img, (u8_t *)app_update_bin, app_update_bin_len, false);
	if (err != 0) {
		__ASSERT(true, "flash_img_buffered_write error %d", err);
	}

	err = flash_img_buffered_write(&flash_img, NULL, 0, true);
	if (err != 0) {
		__ASSERT(true, "flash_img_buffered_write error %d", err);
	}

#define SLEEP_TIME_MS	1
	while(true){
		k_msleep(SLEEP_TIME_MS);
	}

}

