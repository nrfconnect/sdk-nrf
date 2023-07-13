/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing display scan specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_DISP_SCAN_H__
#define __ZEPHYR_DISP_SCAN_H__
#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>

#include "osal_api.h"
int wifi_nrf_disp_scan_zep(const struct device *dev, struct wifi_scan_params *params,
			   scan_result_cb_t cb);

enum wifi_nrf_status wifi_nrf_disp_scan_res_get_zep(struct wifi_nrf_vif_ctx_zep *vif_ctx_zep);

void wifi_nrf_event_proc_disp_scan_res_zep(void *vif_ctx,
				struct nrf_wifi_umac_event_new_scan_display_results *scan_res,
				unsigned int event_len,
				bool is_last);

#endif /* __ZEPHYR_DISP_SCAN_H__ */
