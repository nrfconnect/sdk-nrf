/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing the structure declarations for the Bus Abstraction
 * Layer (BAL) of the Wi-Fi driver.
 */

#ifndef __BAL_STRUCTS_H__
#define __BAL_STRUCTS_H__

#include "osal_ops.h"
#include "bal_ops.h"

struct nrf_wifi_bal_cfg_params {
	unsigned long addr_pktram_base;
};

/**
 * struct nrf_wifi_bal_priv - Structure to hold context information for the BAL
 * @opriv: Pointer to the OSAL context.
 * @bus_priv: Pointer to a specific bus context.
 * @ops: Pointer to bus operations to be provided by a specific bus
 *       implementation.
 *
 * This structure maintains the context information necessary for the
 * operation of the BAL. Some of the elements of the structure need to be
 * initialized during the initialization of the BAL while others need to
 * be kept updated over the duration of the BAL operation.
 */
struct nrf_wifi_bal_priv {
	struct nrf_wifi_osal_priv *opriv;
	void *bus_priv;
	struct nrf_wifi_bal_ops *ops;

	enum nrf_wifi_status (*init_dev_callbk_fn)(void *ctx);

	void (*deinit_dev_callbk_fn)(void *ctx);

	enum nrf_wifi_status (*intr_callbk_fn)(void *ctx);
};


struct nrf_wifi_bal_dev_ctx {
	struct nrf_wifi_bal_priv *bpriv;
	void *hal_dev_ctx;
	void *bus_dev_ctx;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	bool rpu_fw_booted;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
};
#endif /* __BAL_STRUCTS_H__ */
