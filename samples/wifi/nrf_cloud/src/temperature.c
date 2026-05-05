/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include "temperature.h"

LOG_MODULE_REGISTER(temperature, CONFIG_WIFI_NRF_CLOUD_LOG_LEVEL);

int get_temperature(double *temp)
{
	*temp = 22.0 + (sys_rand32_get() % 100) / 40.0;
	return 0;
}
