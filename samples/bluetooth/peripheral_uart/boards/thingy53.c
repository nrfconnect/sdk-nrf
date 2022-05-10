/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>

#include <img_mgmt/img_mgmt.h>
#include <os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/smp_bt.h>

#include <zephyr/usb/usb_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(thingy53_setup);


static int bt_smp_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int err = 0;

	img_mgmt_register_group();
	os_mgmt_register_group();

	err = smp_bt_register();

	if (err) {
		LOG_ERR("SMP BT register failed (err: %d)", err);
	}

	return err;
}

static int usb_cdc_init(const struct device *dev)
{
	int err = usb_enable(NULL);

	if (err) {
		LOG_ERR("Failed to enable USB");
	}

	return err;
}

SYS_INIT(bt_smp_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
SYS_INIT(usb_cdc_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
