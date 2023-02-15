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

int wifi_nrf_set_power_save_mode(const struct device *dev,
				 struct wifi_ps_mode_params *ps_mode_params);

int wifi_nrf_get_power_save_config(const struct device *dev,
				   struct wifi_ps_config *ps_config);

void wifi_nrf_event_proc_get_power_save_info(void *vif_ctx,
					     struct nrf_wifi_umac_event_power_save_info *ps_info,
					     unsigned int event_len);

int wifi_nrf_set_power_save_timeout(const struct device *dev,
				    struct wifi_ps_timeout_params *ps_timeout);
#endif /*  __ZEPHYR_PS_H__ */
