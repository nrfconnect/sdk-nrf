/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>


#define DUMMY_WLAN_NAME	"wlan0"

typedef int (*dummy_api_configure_t)(const struct device *dev,
				     uint32_t dev_config);


struct dummy_wlan_api {
	dummy_api_configure_t configure;
};

static int dummy_configure(const struct device *dev, uint32_t config)
{
	return 0;
}

static const struct dummy_wlan_api funcs = {
	.configure = dummy_configure,
};

int dummy_init(const struct device *dev)
{
	return 0;
}

DEVICE_DEFINE(dummy_wlan, DUMMY_WLAN_NAME, dummy_init, NULL, NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);
