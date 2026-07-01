/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing VTF parameters specific declarations
 */

#ifndef __VTF_H__
#define __VTF_H__

#include "common/fmac_structs_common.h"

enum nrf_wifi_status nrf_wifi_fmac_config_vtf_params(struct nrf_wifi_fmac_dev_ctx *dev_ctx,
						     unsigned int voltage,
						     unsigned int temp,
						     unsigned int x0,
						     unsigned int *vtf_buffer_start_address);
#endif /* __VTF_H__ */
