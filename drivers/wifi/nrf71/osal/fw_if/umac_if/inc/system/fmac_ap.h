/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing SoftAP specific declarations for the FMAC IF Layer
 * of the Wi-Fi driver.
 */

#ifndef __FMAC_AP_H__
#define __FMAC_AP_H__

#include <nrf71_wifi_ctrl.h>
#include "system/fmac_structs.h"


enum nrf_wifi_status sap_client_update_pmmode(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      struct nrf_wifi_sap_client_pwrsave *config);

enum nrf_wifi_status sap_client_ps_get_frames(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      struct nrf_wifi_sap_ps_get_frames *config);
#endif /* __FMAC_AP_H__ */
