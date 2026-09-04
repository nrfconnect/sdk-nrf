/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file hal_api.h
 *
 * @brief Header containing API declarations for the HAL Layer of the Wi-Fi driver
 * in the system mode of operation.
 */

#ifndef __HAL_API_SYS_H__
#define __HAL_API_SYS_H__

#include <common/fw_if/nrf71_wifi_ctrl.h>
#include <common/hal_api_common.h>

struct nrf_wifi_hal_dev_ctx *nrf_wifi_sys_hal_dev_add(struct nrf_wifi_hal_priv *hpriv,
						      void *mac_dev_ctx);

void nrf_wifi_sys_hal_lock_rx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

void nrf_wifi_sys_hal_unlock_rx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

#endif /* __HAL_API_SYS_H__ */
