/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing SoftAP specific declarations for the FMAC IF Layer
 * of the Wi-Fi driver.
 */

#ifndef __FMAC_AP_H__
#define __FMAC_AP_H__

#include "host_rpu_data_if.h"
#include "fmac_structs.h"


enum nrf_wifi_status sap_client_update_pmmode(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      struct nrf_wifi_sap_client_pwrsave *config);

enum nrf_wifi_status sap_client_ps_get_frames(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      struct nrf_wifi_sap_ps_get_frames *config);
#endif /* __FMAC_AP_H__ */
