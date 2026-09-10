/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file wifi_ipc.h
 *
 * @brief Host–RPU IPC API for the nRF71 Wi-Fi driver (nrf_wifi_ipc_*).
 */

#ifndef __WIFI_IPC_H__
#define __WIFI_IPC_H__

#include <stdbool.h>
#include <common/status.h>

struct nrf_wifi_fmac_dev_ctx;

enum nrf_wifi_status nrf_wifi_ipc_open(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

void nrf_wifi_ipc_close(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

bool nrf_wifi_ipc_is_open(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

enum nrf_wifi_status nrf_wifi_ipc_cmd_send(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   void *cmd,
					   unsigned int cmd_size);

void nrf_wifi_ipc_rx_lock(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

void nrf_wifi_ipc_rx_unlock(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

bool nrf_wifi_ipc_rx_enabled(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

#endif /* __WIFI_IPC_H__ */
