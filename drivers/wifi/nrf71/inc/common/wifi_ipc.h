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

/**
 * @brief Open host–RPU IPC for a FMAC device context.
 *
 * Binds the shared IPC instance to @p fmac_dev_ctx, initializes IPC service
 * and locks on first use, and arms the host RX path. The operation mode must
 * have set @c fpriv->rpu_event_cb before calling this function.
 *
 * @param fmac_dev_ctx FMAC device context.
 *
 * @return @ref NRF_WIFI_STATUS_SUCCESS on success, @ref NRF_WIFI_STATUS_FAIL on failure.
 */
enum nrf_wifi_status nrf_wifi_ipc_open(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Close host–RPU IPC for a FMAC device context.
 *
 * Disarms the host RX path, tears down IPC, and frees transport locks associated
 * with the instance. No operation if IPC is not open for @p fmac_dev_ctx.
 *
 * @param fmac_dev_ctx FMAC device context.
 */
void nrf_wifi_ipc_close(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Test whether host–RPU IPC is open for a FMAC device context.
 *
 * @param fmac_dev_ctx FMAC device context.
 *
 * @return true if IPC is open for @p fmac_dev_ctx, false otherwise.
 */
bool nrf_wifi_ipc_is_open(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Send a control command to the RPU over IPC.
 *
 * @param fmac_dev_ctx FMAC device context.
 * @param cmd Pointer to the command buffer.
 * @param cmd_size Size of the command in bytes.
 *
 * @return @ref NRF_WIFI_STATUS_SUCCESS on success, @ref NRF_WIFI_STATUS_FAIL on failure.
 */
enum nrf_wifi_status nrf_wifi_ipc_cmd_send(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   void *cmd,
					   unsigned int cmd_size);

/**
 * @brief Acquire the host RX lock for a FMAC device context.
 *
 * Used from RX/TX tasklets to serialize access to the IPC RX path with the
 * transport layer. Must be paired with @ref nrf_wifi_ipc_rx_unlock.
 *
 * @param fmac_dev_ctx FMAC device context.
 */
void nrf_wifi_ipc_rx_lock(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Release the host RX lock for a FMAC device context.
 *
 * @param fmac_dev_ctx FMAC device context.
 */
void nrf_wifi_ipc_rx_unlock(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Test whether host RX processing is enabled for a FMAC device context.
 *
 * @param fmac_dev_ctx FMAC device context.
 *
 * @return true if RX is active, false otherwise.
 */
bool nrf_wifi_ipc_rx_enabled(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

#endif /* __WIFI_IPC_H__ */
