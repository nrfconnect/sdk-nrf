/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing twt specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_TWT_H__
#define __ZEPHYR_TWT_H__

#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>

#include "osal_api.h"

int wifi_nrf_set_twt(const struct device *dev, struct wifi_twt_params *twt_params);

void wifi_nrf_event_proc_twt_setup_zep(void *vif_ctx,
		struct nrf_wifi_umac_cmd_config_twt *twt_setup_info,
		unsigned int event_len);

void wifi_nrf_event_proc_twt_teardown_zep(void *vif_ctx,
		struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_info,
		unsigned int event_len);

void wifi_nrf_event_proc_twt_sleep_zep(void *vif_ctx,
		struct nrf_wifi_umac_event_twt_sleep *twt_sleep_info,
		unsigned int event_len);
#endif /*  __ZEPHYR_TWT_H__ */
