/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file ipc_bus.h
 *
 * @brief Header file for the IPC bus layer structure declarations of the Wi-Fi driver.
 */

#ifndef __IPC_BUS_H__
#define __IPC_BUS_H__

/**
 * @brief Structure to hold context information for the IPC bus.
 */
struct nrf_wifi_bus_ipc_priv {
	/**
	 * @brief Interrupt callback function.
	 *
	 * Called when an IPC event is received from the RPU.
	 */
	enum nrf_wifi_status (*intr_callbk_fn)(void *hal_ctx);

	/** Configuration parameters for the IPC bus. */
	struct nrf_wifi_bal_cfg_params cfg_params;
};

/**
 * @brief Structure to hold the device context for the IPC bus.
 */
struct nrf_wifi_bus_ipc_dev_ctx {
	/** Pointer to the IPC bus context. */
	struct nrf_wifi_bus_ipc_priv *ipc_priv;
	/** Pointer to the BAL device context. */
	void *bal_dev_ctx;

	/** Base address of the host-mapped RPU memory. */
	unsigned long host_addr_base;
	/** Base address of the packet RAM. */
	unsigned long addr_pktram_base;
};

/**
 * @brief IPC message types for host-to-RPU command send.
 *
 * Values match @ref NRF_WIFI_HAL_MSG_TYPE entries used on the send path.
 */
enum nrf_wifi_ipc_msg_type {
	/** Control-plane command. */
	NRF_WIFI_IPC_MSG_CMD_CTRL = 0,
	/** Data-path RX command. */
	NRF_WIFI_IPC_MSG_CMD_DATA_RX = 2,
	/** Data-path TX command. */
	NRF_WIFI_IPC_MSG_CMD_DATA_TX = 4,
};

#endif /* __IPC_BUS_H__ */
