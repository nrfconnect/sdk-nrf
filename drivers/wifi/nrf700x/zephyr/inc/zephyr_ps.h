/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing power save specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_PS_H__
#define __ZEPHYR_PS_H__

#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>

#include "osal_api.h"

int wifi_nrf_set_power_save(const struct device *dev,
			    struct wifi_ps_params *params);
#endif /*  __ZEPHYR_PS_H__ */
