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

void test_main(void)
{
	struct flash_img_context flash_img;
	printk("Hello World! %s upgrader sample\n", CONFIG_BOARD);
	int err = flash_img_init(&flash_img);
	if (err != 0) {
		__ASSERT(true, "flash_img_init error %d", err);
	}

	printk("Writing app to external flash\n");
	err = flash_img_buffered_write(&flash_img, (u8_t *)app_update_bin, app_update_bin_len, false);
	if (err != 0) {
		__ASSERT(true, "flash_img_buffered_write error %d", err);
	}

	err = flash_img_buffered_write(&flash_img, NULL, 0, true);
	if (err != 0) {
		__ASSERT(true, "flash_img_buffered_write error %d", err);
	}

	err = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (err != 0) {
		__ASSERT(true, "reguest upgrade failed %d", err);
	}

	printk("Upgrade done\n");

}
