/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file bal_structs.h
 *
 * @brief Header containing the structure declarations for the Bus Abstraction
 * Layer (BAL) of the Wi-Fi driver.
 */

#ifndef __BAL_STRUCTS_H__
#define __BAL_STRUCTS_H__

#include "osal_ops.h"
#include "bal_ops.h"

/**
 * @brief Structure holding configuration parameters for the BAL.
 */
struct nrf_wifi_bal_cfg_params {
	/** Base address of the packet RAM. */
	unsigned long addr_pktram_base;
};

/**
 * @brief Structure holding context information for the BAL.
 */
struct nrf_wifi_bal_priv {
	/** Pointer to a specific bus context. */
	void *bus_priv;
	/** Pointer to bus operations provided by a specific bus implementation. */
	struct nrf_wifi_bal_ops *ops;
	/** Callback function for device initialization. */
	enum nrf_wifi_status (*init_dev_callbk_fn)(void *ctx);
	/** Callback function for device deinitialization. */
	void (*deinit_dev_callbk_fn)(void *ctx);
	/** Callback function for handling interrupts. */
	enum nrf_wifi_status (*intr_callbk_fn)(void *ctx);
};

/**
 * @brief Structure holding the device context for the BAL.
 */
struct nrf_wifi_bal_dev_ctx {
	/** Pointer to the BAL private context. */
	struct nrf_wifi_bal_priv *bpriv;
	/** Pointer to the HAL device context. */
	void *hal_dev_ctx;
	/** Pointer to the bus device context. */
	void *bus_dev_ctx;
#ifdef NRF_WIFI_LOW_POWER
	/** Flag indicating if the RPU firmware has booted. */
	bool rpu_fw_booted;
#endif /* NRF_WIFI_LOW_POWER */
};

#endif /* __BAL_STRUCTS_H__ */
