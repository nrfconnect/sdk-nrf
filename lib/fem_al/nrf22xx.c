/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <errno.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include "fem_interface.h"

static int8_t default_tx_output_power_get(void)
{
	return DT_PROP(DT_NODELABEL(nrf_radio_fem), output_power_dbm);
}

static const struct fem_interface_api nrf22xx_fem_api = {
	.default_tx_output_power_get = default_tx_output_power_get,
};

static int nrf22xx_fem_setup(void)
{
	return fem_interface_api_set(&nrf22xx_fem_api);
}

SYS_INIT(nrf22xx_fem_setup, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
