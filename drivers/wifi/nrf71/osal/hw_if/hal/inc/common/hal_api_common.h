/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file hal_api_common.h
 *
 * @brief HAL structures and API for the nRF71 Wi-Fi driver.
 */

#ifndef __HAL_API_COMMON_H__
#define __HAL_API_COMMON_H__

#include <common/log_cfg.h>
#include <nrf71_wifi_ctrl.h>
#include <common/ipc_bus.h>

/**
 * @brief Enumeration of NRF Wi-Fi HAL message types.
 */
enum NRF_WIFI_HAL_MSG_TYPE {
	/** Command control message type */
	NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL,
};

/**
 * @brief Enumeration of NRF WiFi HAL status.
 */
enum NRF_WIFI_HAL_STATUS {
	/** HAL is enabled */
	NRF_WIFI_HAL_STATUS_ENABLED,
	/** HAL is disabled */
	NRF_WIFI_HAL_STATUS_DISABLED,
};

/**
 * @brief Structure to hold context information for the HAL layer.
 */
struct nrf_wifi_hal_priv {
	/** Pointer to IPC private data */
	struct nrf_wifi_ipc_priv *ipc_priv;
	/** Number of devices */
	unsigned char num_devs;
	/** Interrupt callback function */
	enum nrf_wifi_status (*intr_callbk_fn)(void *mac_ctx,
					       void *event_data,
					       unsigned int len);
};

/**
 * @brief Structure to hold per device context information for the HAL layer.
 */
struct nrf_wifi_hal_dev_ctx {
	/** Pointer to HAL private data */
	struct nrf_wifi_hal_priv *hpriv;
	/** MAC device context */
	void *mac_dev_ctx;
	/** IPC device context */
	struct nrf_wifi_ipc_dev_ctx *ipc_dev_ctx;
	/** Device index */
	unsigned char idx;
	/** RPU information */
	void *ipc_msg;
	/** HAL lock */
	void *lock_hal;
	/** RX lock */
	void *lock_rx;
#if defined(NRF_WIFI_RPU_RECOVERY) || defined(__DOXYGEN__)
	/** RPU wake up now asserted time */
	unsigned long last_wakeup_now_asserted_time_ms;
	/** RPU wake up now deasserted time */
	unsigned long last_wakeup_now_deasserted_time_ms;
	/** RPU sleep opportunity time */
	unsigned long last_rpu_sleep_opp_time_ms;
	/** Number of watchdog timer interrupts received */
	int wdt_irq_received;
	/** Number of watchdog timer interrupts ignored */
	int wdt_irq_ignored;
#endif /* NRF_WIFI_RPU_RECOVERY */
	/** HAL status */
	enum NRF_WIFI_HAL_STATUS hal_status;
};

/**
 * @brief Initialize the HAL layer.
 *
 * @param intr_callbk_fn Callback for RPU events forwarded from IPC.
 *
 * @return Pointer to HAL private context, or NULL on failure.
 */
struct nrf_wifi_hal_priv *nrf_wifi_hal_init(
	enum nrf_wifi_status (*intr_callbk_fn)(void *dev_ctx,
					       void *event_data,
					       unsigned int len));

/**
 * @brief Deinitialize the HAL layer.
 *
 * @param hpriv Pointer returned by @ref nrf_wifi_hal_init.
 */
void nrf_wifi_hal_deinit(struct nrf_wifi_hal_priv *hpriv);

void nrf_wifi_hal_dev_rem(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

enum nrf_wifi_status nrf_wifi_hal_dev_init(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

void nrf_wifi_hal_dev_deinit(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

void nrf_wifi_hal_enable(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

void nrf_wifi_hal_disable(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

enum NRF_WIFI_HAL_STATUS nrf_wifi_hal_status_unlocked(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx);

/**
 * @brief Send a control command to the RPU over IPC.
 */
enum nrf_wifi_status nrf_wifi_hal_ctrl_cmd_send(struct nrf_wifi_hal_dev_ctx *hal_ctx,
						void *cmd,
						unsigned int cmd_size);

#endif /* __HAL_API_COMMON_H__ */
