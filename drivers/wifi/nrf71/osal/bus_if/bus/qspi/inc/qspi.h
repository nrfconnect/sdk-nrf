/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file qspi.h
 *
 * @brief Header file for the QSPI bus layer specific structure declarations of the Wi-Fi driver.
 */

#ifndef __QSPI_H__
#define __QSPI_H__

/**
 * @brief Structure to hold context information for the QSPI bus.
 */
struct nrf_wifi_bus_qspi_priv {
	/** Pointer to the QSPI bus-specific context. */
	void *os_qspi_priv;

	/**
	 * @brief Interrupt callback function.
	 *
	 * This function is called when an interrupt occurs on the QSPI bus.
	 *
	 * @param hal_ctx The HAL context.
	 * @return The status of the interrupt handling.
	 */
	enum nrf_wifi_status (*intr_callbk_fn)(void *hal_ctx);

	/**
	 * @brief Configuration parameters for the QSPI bus.
	 *
	 * This structure holds the configuration parameters for the QSPI bus.
	 * Some of the elements of the structure need to be initialized during
	 * the initialization of the QSPI bus, while others need to be kept
	 * updated over the duration of the QSPI bus operation.
	 */
	struct nrf_wifi_bal_cfg_params cfg_params;
};

/**
 * @brief Structure to hold the device context for the QSPI bus.
 */
struct nrf_wifi_bus_qspi_dev_ctx {
	/** Pointer to the QSPI bus context. */
	struct nrf_wifi_bus_qspi_priv *qspi_priv;
	/** Pointer to the BAL device context. */
	void *bal_dev_ctx;
	/** Pointer to the QSPI bus-specific device context. */
	void *os_qspi_dev_ctx;

	/** Base address of the host. */
	unsigned long host_addr_base;
	/** Base address of the packet RAM. */
	unsigned long addr_pktram_base;
};

#endif /* __QSPI_H__ */
