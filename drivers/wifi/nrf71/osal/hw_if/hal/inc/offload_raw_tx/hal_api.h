/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file hal_api.h
 *
 * @brief Header containing API declarations for the HAL Layer of the Wi-Fi driver.
 */

#ifndef __HAL_API_OFF_RAW_TX_H__
#define __HAL_API_OFF_RAW_TX_H__

#include "osal_api.h"
#include "common/rpu_if.h"
#include "bal_api.h"
#include "common/hal_structs_common.h"
#include "common/hal_api_common.h"

struct nrf_wifi_hal_dev_ctx *nrf_wifi_off_raw_tx_hal_dev_add(struct nrf_wifi_hal_priv *hpriv,
							     void *mac_dev_ctx);
#endif /* __HAL_API_OFF_RAW_TX_H__ */
