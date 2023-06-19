/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing virtual interface (VIF) specific declarations for
 * the FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_VIF_H__
#define __FMAC_VIF_H__

#include "fmac_structs.h"

int wifi_nrf_fmac_vif_check_if_limit(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				     int if_type);

void wifi_nrf_fmac_vif_incr_if_type(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				    int if_type);

void wifi_nrf_fmac_vif_decr_if_type(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				    int if_type);

void wifi_nrf_fmac_vif_clear_ctx(void *fmac_dev_ctx,
				 unsigned char if_idx);

void wifi_nrf_fmac_vif_update_if_type(void *fmac_dev_ctx,
				      unsigned char if_idx,
				      int if_type);

unsigned int wifi_nrf_fmac_get_num_vifs(void *fmac_dev_ctx);

#endif /* __FMAC_VIF_H__ */
